#include "server_common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>  // for memcpy, memset, strlen
#include <poll.h>
#include <pthread.h>

#define USE_CAMERA

// some diagnostic printouts
// #define INFO

// more diagnostic printouts
// #undef DEBUG

// the strncasecmp function is not in the ISO C standard
// #define USE_POSIX_FUNCTION

#ifdef USE_CAMERA
#include "camera.h"
#endif

#ifndef MOTION_PORT
#define MOTION_PORT 9090
#endif

#define BUFSIZE 50000
struct client{
    int  connfd;
    byte sendBuff[BUFSIZE];
#ifdef USE_CAMERA
    camera* cam;
    byte* frame_data;
#endif
};

struct global_state {
    int listenfd;
    int running;
    int quit;
    pthread_t main_thread;
    pthread_t client_thread;
    pthread_t ui_thread;
    pthread_t bg_thread;
    struct pollfd pfd;
    int motionfd;
#ifdef USE_CAMERA
    camera* cam;
#endif
};


int client_write_string(struct client* client);
int client_write_n(struct client* client, size_t n);
void* serve_client(void *ctxt);
void server_quit(struct global_state* s);
void signal_to_bg_task();
int is_running(struct global_state* state);

/////////////// camera stuff

#define ERR_OPEN_STREAM 1
#define ERR_GET_FRAME 2

struct client;

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t global_cond = PTHREAD_COND_INITIALIZER;

#ifdef USE_CAMERA

/* try to open capture interface.
 * returns 0 on success
 */
int try_open_camera(struct global_state* state)
{
    state->cam = camera_open();
    if (!state->cam){ // Check if null
        printf("axism3006v: Stream is null, can't connect to camera");
        return ERR_OPEN_STREAM;
    }
    return 0;
}

void close_camera(struct global_state* state)
{
    if(state->cam) {
	camera_close(state->cam);
	state->cam = NULL;
    }
}


/* Sets up the packet structure in client->sendBuff.
 * This is a minimal HTTP header for an image/jpeg
 *
 * sets client->frame_data to the beginning of the frame
 * in the packet
 *
 * returns size of entire packet, or -1 if frame too large
 */
ssize_t setup_packet(struct client* client, uint32_t frame_sz)
{
    size_t header_size;
    snprintf(client->sendBuff, sizeof(client->sendBuff),
            "HTTP/1.0 200 OK\nContent-Length: %d\nContent-Type: image/jpeg\n\n", frame_sz);
    header_size = strlen(client->sendBuff);
    if(header_size + frame_sz > sizeof(client->sendBuff)) {
      return -1;
    }
    client->frame_data = client->sendBuff + header_size;
#ifdef DEBUG
    printf("Header size = " FORMAT_FOR_SIZE_T "\n", header_size);
#endif
    return header_size+frame_sz;
}

/* send packet with frame
 * returns 0 on success
 */
int client_send_frame(struct client* client, frame* fr)
{
    // this should really be a compile-time check, but that is complicated
#ifndef DISABLE_SANITY_CHECKS
    if(sizeof(size_t) != sizeof(uint32_t)) {
        printf("sizeof(size_t)=%d, sizeof(uint32_t)=%d\n", sizeof(size_t), sizeof(uint32_t));
        printf("Not sending frame, size sanity check failed\n");
        return 2;
    }
#endif

    size_t frame_sz = get_frame_size(fr);
    byte* data = get_frame_bytes(fr);
    int result;

    ssize_t packet_sz = setup_packet(client, frame_sz);

    if(packet_sz < 0) {
        printf("Frame too big for send buffer(" FORMAT_FOR_SIZE_T " > " FORMAT_FOR_SIZE_T "), skipping.\n", frame_sz, sizeof(client->sendBuff));
        result = 1;
    } else {
        int written;
#ifdef DEBUG
        printf("encode size:" FORMAT_FOR_SIZE_T "\n",  frame_sz);
        printf("sizeof(size_t)=" FORMAT_FOR_SIZE_T ", sizeof(uint32_t)=" FORMAT_FOR_SIZE_T "\n", sizeof(size_t), sizeof(uint32_t));
#endif
        memcpy(client->frame_data, data, frame_sz);

        written=client_write_n(client, packet_sz);
        if(written != packet_sz) {
          printf("WARNING! packet_sz=" FORMAT_FOR_SIZE_T ", written=%d\n", packet_sz, written);
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
int try_get_frame(struct client* client)
{
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
#endif

////////////// Server stuff

int client_write_string(struct client* client)
{
  return write_string(client->connfd, client->sendBuff);
}

/*
 * Returns number of bytes written.
 * Returns -1 if an error occurs and sets errno.
 * Note: an error occurs if the web browser closes the connection
 *   (might happen when reloading too frequently)
 */
int client_write_n(struct client* client, size_t n)
{
    return write_n(client->connfd, client->sendBuff,n);
}


/* A minimal, not-even-close-to-compliant HTTP request parser.
 * Only accepts GET / and expects *exactly* one space char between GET and /
 */
int parse_http_request(const char* buf, size_t sz)
{
#ifdef USE_POSIX_FUNCTION
    int r = strncasecmp(buf, "GET ", 4);
    int rl = 1;
#else
    int r = strncmp(buf, "GET ", 4);
    int rl = strncmp(buf, "get ", 4);
#endif

    if(r && rl) {
        return 501;
    }
    r = strncmp(buf+4, "/ ", 2);
    rl = strncmp(buf+4, "/image.jpg ", 11);
    if(r && rl) {
        return 404;
    } else {
        return 0;
    }
}

static void send_internal_error(struct client* client,const char* msg)
{
    snprintf(client->sendBuff, sizeof(client->sendBuff),
	     "HTTP/1.0 500 Internal Error\nContent-Length: " FORMAT_FOR_SIZE_T "\nContent-Type: text/plain\n\n%s",
	     strlen(msg), msg);
    client_write_string(client);
}

/* The function for a thread that processes a single client request.
 * The client struct pointer is passed as void* due to the pthreads API
*/
void* serve_client(void *ctxt)
{
    char buf[1024] = {};
    struct client* client = ctxt;
    memset(client->sendBuff, 0, sizeof(client->sendBuff));
    int rres = read(client->connfd, buf, 1024);
    if(rres < 0) {
        perror("motion_task: read");
	return (void*) (intptr_t) errno;
    }
#ifdef DEBUG
    printf("read: rres = %d\n", rres);
    printf("buf[%s]\n", buf);
#endif
    int hres = parse_http_request(buf, 1024);
    if(hres == 0) {
#ifdef USE_CAMERA
	    int cres=0;
            if( !client->cam || (cres=try_get_frame(client))) {
                printf("ERROR getting frame from camera: %d\n",cres);
		send_internal_error(client, "Error getting frame from camera\n");
            } 
#else
        snprintf(client->sendBuff, sizeof(client->sendBuff), "Hello...\n");
	client_write_string(client);
#endif

    } else {
	if (hres == 404) {
	    const char* msg = "404: File not found.\n\nI only have image.jpg";
	    snprintf(client->sendBuff, sizeof(client->sendBuff),
		     "HTTP/1.0 404 Not Found\nContent-Length: " FORMAT_FOR_SIZE_T
		     "\nContent-Type: text/plain\n\n%s",
		     strlen(msg), msg);
	} else {
	    const char* msg = "501: Not implemented";
	    snprintf(client->sendBuff, sizeof(client->sendBuff),
		     "HTTP/1.0 501 Not Implemented\nContent-Length: " FORMAT_FOR_SIZE_T
		     "\nContent-Type: text/plain\n\n%s",
		     strlen(msg), msg);
	}
	client_write_string(client);
    }
    return (void*) (intptr_t) close(client->connfd);
}

/* signal the global condition variable
 */
void signal_to_bg_task()
{
    pthread_mutex_lock(&global_mutex);
    pthread_cond_broadcast(&global_cond);
    pthread_mutex_unlock(&global_mutex);
}
/* set flags to signal shutdown and
 * signal global condition variable
 */
void server_quit(struct global_state* s)
{
    printf("Quitting\n");
    pthread_mutex_lock(&global_mutex);
    s->quit=1;
    s->running=0;
    pthread_cond_broadcast(&global_cond);
    pthread_mutex_unlock(&global_mutex);
}
void* ui_task(void *ctxt)
{
  struct global_state* s = ctxt;

  while(is_running(s)){
    char c = getchar();
    switch (c){
      case 'q':
	server_quit(s);
        break;
      case 's':
        printf("I could print some status info\n");
        break;
      case 'b':
        signal_to_bg_task();
      default:
#ifdef DEBUG
        printf("ignoring %c\n", c);
#endif
        break;
    }
  }
  return 0;
}

void* bg_task(void *ctxt)
{
  struct global_state* s = ctxt;

  pthread_mutex_lock(&global_mutex);
  while(s->running){
    pthread_cond_wait(&global_cond, &global_mutex);
#ifdef INFO
    printf("bg_task: global_cond was signalled\n");
#endif
  }
  pthread_mutex_unlock(&global_mutex);
  return 0;
}

int serve_clients(struct global_state* state);

void* main_task(void *ctxt)
{
    struct global_state* state = ctxt;
    return (void*) (intptr_t) serve_clients(state);
}


int try_accept(struct global_state* state, struct client* client)
{
    int result = 0;
    if( !state->cam && try_open_camera(state)) {
	printf("Error opening camera\n");
	return 1;
    }
    client->cam = state->cam;
    client->connfd = accept(state->listenfd, (struct sockaddr*)NULL, NULL);
    if(client->connfd < 0) {
        result = errno;
    } else {
#ifdef INFO
	printf("accepted connection\n");
#endif
	// serve clients in separate thread, for four reasons
	// 1. Illustrate threading
	// 2. Not blocking the user interface while serving client
	// 3. Prepare for serving multiple clients concurrently
        // 4. The AXIS software requires capture to be run outside the main thread
        if (pthread_create(&state->client_thread, 0, serve_client, client)) {
            printf("Error pthread_create()\n");
            perror("creating");
            result = errno;
        } else {
            void* status;
            if(pthread_join(state->client_thread, &status)){
                perror("join");
                result = errno;
            } else {
#ifdef DEBUG
		printf("Join: status = %d\n", (int)(intptr_t) status);
#endif
            }
        }
    }
    return result;
}


static int create_threads(struct global_state* state)
{
    pthread_mutex_lock(&global_mutex);
    int result = 0;
    if (pthread_create(&state->bg_thread, 0, bg_task, state)) {
        printf("Error pthread_create()\n");
        perror("creating bg thread");
        result = errno;
        state->running=0;
        goto failed_to_start_bg_thread;
    }
    if (pthread_create(&state->main_thread, 0, main_task, state)) {
        printf("Error pthread_create()\n");
        perror("creating main thread");
        result = errno;
        state->running=0;
        goto failed_to_start_main_thread;
    }
    if (pthread_create(&state->ui_thread, 0, ui_task, state)) {
        printf("Error pthread_create()\n");
        perror("creating ui thread");
        result = errno;
        state->running=0;
        goto failed_to_start_ui_thread;
    }
    pthread_detach(state->ui_thread);
failed_to_start_ui_thread:
failed_to_start_main_thread:
failed_to_start_bg_thread:
    pthread_mutex_unlock(&global_mutex);
    return result;
}
static void join_bg_thread(pthread_t* bg_thread, const char* msg)
{
    void* status;
    if(pthread_join(*bg_thread, &status)){
        perror("join bg_tread");
    } else {
        printf("Join bg thread (%s): status = " FORMAT_FOR_SIZE_T "\n", msg, (intptr_t) status);
    }
}

static void client_init(struct client* client)
{
    client->cam=NULL;
    client->connfd=-1;
}
int serve_clients(struct global_state* state)
{
    int result=0;
    state->pfd.fd = state->listenfd;
    state->pfd.events=POLLIN;
    while(is_running(state)) {
        int ret; // result of poll

        struct client* client = malloc(sizeof(*client));
        if(client == NULL) {
            perror("malloc client");
            result = errno;
            goto failed_to_alloc_client;
        }
	client_init(client);
#ifdef DEBUG
	printf("serve_clients: listening\n");
#endif

        do {
            ret = poll(&state->pfd, POLLIN, 2000);
#ifdef DEBUG_VERBOSE
            printf("client poll returns %d\n", ret);
#endif
        } while(ret==0 && is_running(state));

        if(ret <0) {
            perror("poll");
            result = errno;
	    server_quit(state);
        } else if (is_running(state)) {
            if(try_accept(state, client)) {
              perror("try_accept");
              result = errno;
	      server_quit(state);
            };
        }
        free(client);
    }
failed_to_alloc_client:
    close_camera(state);
    return result;
}

void init_global_state(struct global_state* state)
{
    pthread_mutex_lock(&global_mutex);
    state->running=0;
    state->quit=0;
    state->cam=NULL;
    pthread_mutex_unlock(&global_mutex);
}

int is_running(struct global_state* state)
{
    int result=0;
    pthread_mutex_lock(&global_mutex);
    result =  state->running;
    pthread_mutex_unlock(&global_mutex);
    return result;
}
int create_socket(struct global_state* state)
{
    int reuse;
    state->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    state->motionfd = socket(AF_INET, SOCK_STREAM, 0);

    if(state->listenfd < 0 || state->motionfd < 0) {
        perror("socket");
        return errno;
    }

    reuse = 1;
    set_socket_sigpipe_option(state->listenfd);
    if(setsockopt(state->listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
      perror("setsockopt listenfd");
      return errno;
    }

    return 0;
}

/* bind state->listenfd to port and listen
 * returns 0 on success
 */
int bind_and_listen(struct global_state* state, int port)
{
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if( bind(state->listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        perror("bind listenfd");
        return errno;
    }

    if(listen(state->listenfd, 10)){
        perror("listen listenfd");
        return errno;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int port;
    struct global_state state;
    int result=0;

    if(argc==2) {
        printf("interpreting %s as port number\n", argv[1]);
        port = atoi(argv[1]);
    } else {
        port = 9999;
    }
    printf("starting on port %d\n", port);

    init_global_state(&state);

    if(create_socket(&state)) {
        goto no_server_socket;
    }

    if(bind_and_listen(&state, port)) {
        goto failed_to_listen;
    }
    pthread_mutex_lock(&global_mutex);
    state.running = 1;
    pthread_mutex_unlock(&global_mutex);

    if(create_threads(&state)) {
        goto failed_to_start_threads;
    }

    join_bg_thread(&state.bg_thread, "bg_thread");
    join_bg_thread(&state.main_thread, "main_thread");

failed_to_start_threads:
failed_to_listen:
    if(close(state.listenfd)) perror("closing listenfd");
no_server_socket:
  return result;
}
