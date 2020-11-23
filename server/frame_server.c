#include "server_common.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "camera.h"

#define USE_CAMERA

#define BUFSIZE 50000
#define ERR_OPEN_STREAM 1
#define ERR_GET_FRAME 2
#define MODE_IDLE 1
#define MODE_MOVIE 2

struct client {
  int connfd;
  byte sendBuff[BUFSIZE];
  camera* cam;
  byte* frame_data;
  int mode;
};

struct global_state {
  int client_connfd;    // Allowing the mode thread to access the connection
  int proposed_mode;    // The mode thread's idea of what mode we should be in
  int listenfd;
  int running;
  int quit;
  pthread_t frame_thread;
  pthread_t mode_thread;
  camera* cam;
};

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t global_cond = PTHREAD_COND_INITIALIZER;

void* frame_task(void *ctxt);
void* mode_task(void *ctxt);
int serve(struct global_state* state);
int retrieve_mode(struct global_state* state);
int bind_server_socket(struct global_state* state, int port);


void init_global_state(struct global_state* state){
  pthread_mutex_lock(&global_mutex);
  state->running=0;
  state->client_connfd=-1;
  state->quit=0;
  state->cam=NULL;
  state->proposed_mode=1;
  pthread_mutex_unlock(&global_mutex);
}

static void client_init(struct client* client)
{
  client->mode=MODE_IDLE;
  client->cam = NULL;
  client->connfd = -1;
}

int is_running(struct global_state* state){
  int result=0;
  pthread_mutex_lock(&global_mutex);
  result = state->running;
  pthread_mutex_unlock(&global_mutex);
  return result;
}

int try_open_camera(struct global_state* state){
  pthread_mutex_lock(&global_mutex);
  state->cam = camera_open();
  if (!state->cam){
    printf("axism3006v: Stream is null, can't connect to camera");
    return ERR_OPEN_STREAM;
  }
  pthread_mutex_unlock(&global_mutex);
  return 0;
}

void close_camera(struct global_state* state){
  pthread_mutex_lock(&global_mutex);

  if(state->cam) {
    camera_close(state->cam);
    state->cam = NULL;
  }
  pthread_mutex_unlock(&global_mutex);
}

int client_write_string(struct client* client)
{
  return write_string(client->connfd, client->sendBuff);
}

int client_write_n(struct client* client, size_t n)
{
  return write_n(client->connfd, client->sendBuff,n);
}

ssize_t setup_packet(struct client* client, uint32_t frame_sz)
{
  size_t header_size;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  int64_t millisecondsSinceEpoch = (int64_t)
    ((unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000);
  int32_t s_mode = (int32_t) client->mode;
  int32_t s_frame_sz = (int32_t) frame_sz;

  // Fills the send buffer with zeroes
  memset(client->sendBuff, 0, sizeof(client->sendBuff));

  // Puts frame header into buffer: timestamp, mode, frame size
  memcpy(client->sendBuff, &millisecondsSinceEpoch, sizeof(int64_t));
  memcpy(client->sendBuff+sizeof(int64_t), &s_mode, sizeof(int32_t));
  memcpy(client->sendBuff+sizeof(int64_t)+sizeof(int32_t),
         &s_frame_sz, sizeof(int32_t));

  // The header size is static in size (16 bytes)
  header_size = sizeof(int64_t)+sizeof(int32_t)+sizeof(int32_t);
  if(header_size + frame_sz > sizeof(client->sendBuff)) {
    perror("Send buffer not large enough for message!");
    return -1;
  }
  client->frame_data = client->sendBuff + header_size;
  return header_size+frame_sz;
}

int client_send_frame(struct client* client, frame* fr)
{
  #ifndef DISABLE_SANITY_CHECKS
  if(sizeof(size_t) != sizeof(uint32_t)) {
    printf("Not sending frame, size sanity check failed\n");
    return 2;
  }
  #endif

  size_t frame_sz = get_frame_size(fr);
  byte* data = get_frame_bytes(fr);
  int result;

  ssize_t packet_sz = setup_packet(client, frame_sz);

  if(packet_sz < 0) {
    printf("Frame too big for send buffer");
    result = 1;
  } else {
    int written;
    memcpy(client->frame_data, data, frame_sz);

    written=client_write_n(client, packet_sz);
    if(written != packet_sz) {
      printf("WARNING packet size not equal to written size");
      result = 3;
    } else {
      result = 0;
    }
  }
  return result;
}

/* get frame from camera and send to client
* returns zero on success
*/
int try_send_frame(struct client* client){

  int result=-1;
  frame *fr = fr = camera_get_frame(client->cam);

  if(fr) {
    if((result = client_send_frame(client, fr))) {
      printf("Warning: client_send_frame returned %d\n", result);
    }
    frame_free(fr);
  } else {
    return ERR_GET_FRAME;
  }
  return result;
}


static int create_threads(struct global_state* state){
  pthread_mutex_lock(&global_mutex);
  int result = 0;
  if (pthread_create(&state->frame_thread, 0, frame_task, state)) {
    printf("Error pthread_create()\n");
    perror("creating frame thread");
    result = errno;
    state->running=0;
  }
  if (pthread_create(&state->mode_thread, 0, mode_task, state)) {
    printf("Error pthread_create()\n");
    perror("creating mode thread");
    result = errno;
    state->running=0;
  }

  pthread_mutex_unlock(&global_mutex);
  return result;
}

void* frame_task(void *ctxt){
  struct global_state* state = ctxt;
  return (void*) (intptr_t) serve(state);
}

void* mode_task(void *ctxt)
{
  struct global_state* state = ctxt;
  return (void*) (intptr_t) retrieve_mode(state);
}

/* The method mode_thread is executing. Reads the socket connection for ints.
*  1 represents MODE_IDLE and 2 represents MODE_MOVIE.
*/
int retrieve_mode(struct global_state* state){
  while(1337){
    char retrieve_buf[128];
    memset(retrieve_buf, 0, sizeof(retrieve_buf));

    pthread_mutex_lock(&global_mutex);
    // Wait for a signal to start indicating a client has connected
    while(state->running == 0){
      pthread_cond_wait(&global_cond, &global_mutex);
    }
    // Get the fd from the state which has been put there by the frame thread
    int fd = state->client_connfd;
    pthread_mutex_unlock(&global_mutex);

    if(fd > 0){
      // Read from the socket's fd to capture client ordered modes
      ssize_t bytesReceived = recv(fd, retrieve_buf, 127, 0);
      retrieve_buf[127] = '\0';
      if (bytesReceived < 0){
        printf("Failed to recv mode\n");
      } else {
        // Convert the byte representation of the received mode to an int
        int32_t recv_mode = retrieve_buf[0] | ( (int)retrieve_buf[1] << 8 ) |
              ( (int)retrieve_buf[2] << 16 ) | ( (int)retrieve_buf[3] << 24 );

        // If the received mode is not 1 or 2, something strange has been sent
        // which is to be ignored as it's not a correct mode.
        if (recv_mode == MODE_IDLE || recv_mode == MODE_MOVIE){
          printf("Mode thread received: %d\n", recv_mode);
          pthread_mutex_lock(&global_mutex);
          state->proposed_mode = recv_mode;

          // If the received mode is movie we may have to wake up the
          // frame thread from its sleep. This is done by signaling global_cond.
          if (recv_mode == MODE_MOVIE){
            pthread_cond_signal(&global_cond);
          }
          pthread_mutex_unlock(&global_mutex);
        }
      }
    }
  }
  return 0;
}

/*
* create a server socket bound to port
* and listening.
*
* return positive file descriptor
* or negative value on error
*/
int create_server_socket(int port, struct global_state* state){
  pthread_mutex_lock(&global_mutex);
  state->listenfd = -1;

  if(port < 0 || port > 65535) {
    errno = EINVAL;
    pthread_mutex_unlock(&global_mutex);
    return -1;
  }
  state->listenfd = socket(AF_INET,SOCK_STREAM,0);

  if(state->listenfd < 0){
    printf("Incorrect listenfd\n");
    pthread_mutex_unlock(&global_mutex);
    return -1;
  }
  if(bind_server_socket(state,port)){
    printf("Unable to bind server, the adress is already used. Try changing the port.\n");
    pthread_mutex_unlock(&global_mutex);
    return -1;
  }
  if(listen(state->listenfd,10)){
    printf("Error in listen\n");
    pthread_mutex_unlock(&global_mutex);
    return -1;
  }
  pthread_mutex_unlock(&global_mutex);
  return 0;
}

int bind_server_socket(struct global_state* state, int port){
  struct sockaddr_in serv_addr;

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  if( bind(state->listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
    perror("bind listenfd");
    return errno;
  }
  return 0;
}

/* The method frame_thread is executing. Writes frames consisting of
*  <timestamp><current mode><frame size><frame bytes> to client after a
* successful connection has been made. When in MODE_IDLE frames are sent
* every 5 seconds and in MODE_MOVIE the frames are sent as fast as possible.
*/
int serve(struct global_state* state){
  struct client* client = malloc(sizeof(*client));
  struct timeval tv;
  struct timespec ts;
  int64_t last_send = 0;
  int rt;

  // The equivalent of a thread's run loop
  while(1337){
    // Do some initializations for every time we receive a new connection
    client_init(client);
    if(try_open_camera(state)){
      printf("Error opening camera\n");
      return 1;
    }
    client->cam = state->cam;
    printf("Accepting connections on fd %d\n",state->listenfd);
    if((client->connfd = accept(state->listenfd, (struct sockaddr*)NULL, NULL)) < 0){
      printf("Errorneous connection; quitting!\n");
      return -1;
    }
    printf("Client connected!\n");

    // Some attributes of state are set and global_cond is signalled to
    // wake up the mode thread which has been sleeping waiting for a client.
    pthread_mutex_lock(&global_mutex);
    state->proposed_mode = 1;
    state->running = 1;
    state->client_connfd = client->connfd;
    pthread_cond_signal(&global_cond);
    pthread_mutex_unlock(&global_mutex);

    // The loop for actually sending frames to the client after a connection
    // has been made
    while(is_running(state)){
      pthread_mutex_lock(&global_mutex);
      if (state->proposed_mode != client->mode){
        printf("Changing mode from %d to %d\n", client->mode, state->proposed_mode);
        client->mode = state->proposed_mode;
      }
      pthread_mutex_unlock(&global_mutex);

      // As using movie_mode on the fake_server results in a way too high fps,
      // we add an artificial delay to better simulate an actual connection.
      #ifdef FAKE
      usleep(50000);
      #endif

      // Sleep so we send 0.2 FPS in idle mode in "just" 26 SLOC :)
      if (client->mode == MODE_IDLE){
        gettimeofday(&tv, NULL);
        int64_t current_time = (int64_t)
          ((unsigned long long)(tv.tv_sec) * 1000 +
          (unsigned long long)(tv.tv_usec) / 1000);
        int64_t t_sleep = last_send + 5000 - current_time;

        // We create a timespec since timedwait needs it.
        // It takes the current time and then we add the time to sleep
        ts.tv_sec  = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;

        // Add as many nanoseconds to ts as we need to sleep. t_sleep is ms.
        // Because nsec only takes up to 1 sec in nsec we need to subtract
        // the whole secs and add it to tv_sec
        while (t_sleep >= 1000){
          ts.tv_sec += 1;
          t_sleep -= 1000;
        }

        // tv_nsec has a max value of 999999999, else we get EINVAL
        if (ts.tv_nsec + t_sleep*1000000 > 999999999){
          ts.tv_sec += 1;
          ts.tv_nsec  = ts.tv_nsec + t_sleep*1000000 - 1000000000;
        } else {
          ts.tv_nsec += t_sleep*1000000;
        }

        if (t_sleep > 0){
          pthread_mutex_lock(&global_mutex);
          // Sleep as many seconds as ts represents or until awaken by signal
          // Due to spurious wakeups we need to do put this timedwait inside a
          // while.
          // It escapes the while if rt == ETIMEDOUT, which means the timer
          // has run out or if the proposed mode is 2, which means
          // the client has ordered movie mode.
          do {
            rt = pthread_cond_timedwait(&global_cond, &global_mutex, &ts);
          } while (rt != ETIMEDOUT && state->proposed_mode != 2);
          pthread_mutex_unlock(&global_mutex);
        }
      }

      // Get the time the frame is sent to know when to send the next one
      gettimeofday(&tv, NULL);
      last_send = (int64_t)
        ((unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000);

      // If sending the frame to the client fails we need to close the
      // connection as something has gone horribly wrong. state->running is
      // set to 0 as to break the send loop.
      if (try_send_frame(client)) {
        pthread_mutex_lock(&global_mutex);
        state->running = 0;
        pthread_mutex_unlock(&global_mutex);
        printf("Closing clientfd (%d)\n",client->connfd);
        close(client->connfd);
      }
    }
  }
}


int main(int argc, char *argv[]){
  printf("Starting server..\n");
  int port;
  struct global_state state;

  if(argc==2) {
    printf("Interpreting %s as port number\n", argv[1]);
    port = atoi(argv[1]);
  } else {
    port = 9999;
    printf("No port supplied, using default port %d\n", port);
  }

  init_global_state(&state);
  printf("Creating server socket..\n");
  if(create_server_socket(port, &state)){
    printf("Quitting due to create_server_socket error.\n");
    return 1;
  }

  if(state.listenfd < 0){
    printf("Create server socket error!\n");
    return 1;
  }

  printf("Creating threads..\n");
  create_threads(&state);

  // Turn the motion server port (frame server port + 1) into a string
  int ms_port_length = snprintf( NULL, 0, "%d", port + 1 );
  char* ms_port_str = malloc( ms_port_length + 1 );
  snprintf(ms_port_str, ms_port_length + 1, "%d", port + 1 );

  // Declare variables to allow execv to start the motion server in a new process
  char *my_args[5];
  my_args[0] = "./motion_server";
  my_args[1] = ms_port_str;
  my_args[2] = NULL;
  my_args[3] = NULL;
  my_args[4] = NULL;
  pid_t pid;
  int fd;

  switch ((pid = fork()))
  {
    case -1:
      perror ("Could not create a new process, exiting.");
      exit(EXIT_FAILURE);
      break;
    case 0:
      // Start the motion server process and suppress its output
      printf("Starting motion server on port %s..\n", ms_port_str);
      fd = open("/dev/null", O_WRONLY);
      dup2(fd, 1);
      execv ("./motion_server", my_args);
      close(fd);
      printf("The motion server has shut down. Quitting.\n");
      exit(EXIT_FAILURE);
      break;
    default:
      break;
  }

  // Termination
  pthread_join(state.frame_thread, NULL);
  pthread_join(state.mode_thread, NULL);
  printf("Terminated threads!\n");
  printf("Closing socket: %d\n", state.listenfd);
  close(state.listenfd);
  close_camera(&state);

  return 0;
}
