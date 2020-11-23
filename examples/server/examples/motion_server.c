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
#include <time.h>
#include <signal.h>

/*

A simple server that receives motion notifications from the Axis camera, and
provides a minimal HTTP interface for getting motion detection info.
It is basically a decoupling of the camera (which uses push notifications and 
thus must be configured for each client) that provides a pull interface,
allowing any number of clients to query for motion events without requiring
reconfiguration of the camera.

The interface towards clients is a minimal HTTP service (default on port 9091
but the port can be set on the command line):
a GET / (or GET /motion.txt) returns a text/plain response
on the format a:l:r
where
  a is the time (in seconds) since last motion detection notification was received
  l is the local (i.e. this server's) time of the reception of the last motion event
  r is the cameras timestamp on the last motion event

The interface towards the camera (i.e., where motion detection notifications are
received) is a HTTP service (default on port 9090), expecting notifications on the format
  GET /motion?Message=tttttttttt
  where ttttttt is the camera's timestamp (in seconds in epoch) of the event


To setup motion detection notifications on camera:
In the setup interface, under events/action rules:

1.Add...
    name: something
    Condition:
	trigger: Detectors / Motion detection  (motion: Yes)
    Action:
        type: send notification
	(new recipient)
	    name: localhost9090
            type: HTTP
            URL: http://localhost:9090/motion
	    login credentails: (leave blank)
	message parameter: %s


In the above example, it is assumed that the motion_server runs on
the camera itself. If running on another machine, change localhost to the hostname
of the server.
 */


// Compile-time configuration:

// the strncasecmp function is a POSIX function and not in the ISO C standard,
// #define USE_POSIX_FUNCTION

// Sketch of running the client serving routine in a separate thread
// to illustrate the threading. Not needed, as the processing in
// the client thread is quite fast.
// #define MULTI_CLIENT_THREADS


// the motion port is used by the Axis camera for sending notifications
// and must match the configuration on the camera. This should only be
// changed when the setup of the Axis cameras is changed.

#ifndef MOTION_PORT
#define MOTION_PORT 9090
#endif

struct global_state {
    int listenfd;
    int running;
    int quit;
    pthread_t ui_thread;
    pthread_t bg_thread;
    pthread_t motion_thread;
    struct pollfd pfd;
    int motionfd;
    int motioncfd;
    struct pollfd mfd;

    long t_remote;
    long t_local;
};

#define BUFSIZE 1000
struct client{
    int  connfd;
    char sendBuff[BUFSIZE];
    struct global_state* state;
};



static int client_write_string(struct client* client);

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_cond = PTHREAD_COND_INITIALIZER;

////////////// Server stuff
static int install_handlers();

static int client_write_string(struct client* client)
{
  return write_string(client->connfd, client->sendBuff);
}

/* A minimal, not-even-close-to-compliant HTTP request parser.
 * Only accepts GET / and expects *exactly* one space char between GET and /
 */
static int parse_http_request(const char* buf, size_t sz)
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
    rl = strncmp(buf+4, "/motion.txt ", 12);
    if(r && rl) {
        return 404;
    } else {
        return 0;
    }
}

/* A function that serves one client request, parsing the request and sending a response.
 * The client struct pointer is passed as void* due to the pthreads API
*/
static void* serve_client(void *ctxt)
{
    char buf[1024] = {0};
    struct client* client = ctxt;
    memset(client->sendBuff, '0', sizeof(client->sendBuff));
    int rres = read(client->connfd, buf, 1024);
    if( rres < 0) {
        perror("serve_client: read");
	return (void*) (intptr_t) errno;
    }
#ifdef DEBUG
    printf("read: rres = %d\n", rres);
    printf("buf[%s]\n", buf);
#endif
    int hres = parse_http_request(buf, 1024);
    if(hres == 0) {
	char msg[100];
	long tdiff = time(0)-client->state->t_local;
	snprintf(msg,sizeof(msg),"%ld:%ld:%ld",
		 tdiff,client->state->t_local,client->state->t_remote);
        snprintf(client->sendBuff, sizeof(client->sendBuff),
		 "HTTP/1.0 200 OK\nContent-Length:" FORMAT_FOR_SIZE_T
		 "\nContent-Type: text/plain\n\n%s",
		 strlen(msg), msg);
    } else if (hres == 404) {
        const char* msg = "404: File not found";
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
    int result =  close(client->connfd);
#ifdef MULTI_CLIENT_THREADS
    free(client);
#endif
    return (void*) (intptr_t) result;
}

static void signal_to_bg_task()
{
    pthread_mutex_lock(&global_mutex);
    pthread_cond_broadcast(&global_cond);
    pthread_mutex_unlock(&global_mutex);
}

static void do_quit(struct global_state* s)
{
    printf("Quitting\n");
    pthread_mutex_lock(&global_mutex);
    s->quit=1;
    s->running=0;
    pthread_cond_broadcast(&global_cond);
    pthread_mutex_unlock(&global_mutex);
}

static void* ui_task(void *ctxt)
{
  struct global_state* s = ctxt;

  while(s->running){
    char c = getchar();
    switch (c){
      case 'q':
	do_quit(s);
        break;
      case 's':
        printf("Motion detection server\n");
	long tdiff = time(0)-s->t_local;
        printf("Time since motion detected: %ld\n",tdiff);
        printf("Local timestamp of last motion event: %ld\n",s->t_local);
        printf("Camera's timestamp of last motion event: %ld\n",s->t_remote);
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

static void* bg_task(void *ctxt)
{
  struct global_state* s = ctxt;

  while(s->running){
    pthread_mutex_lock(&global_mutex);
    pthread_cond_wait(&global_cond, &global_mutex);
#ifdef INFO
    printf("bg_task: global_cond was signalled\n");
#endif
    pthread_mutex_unlock(&global_mutex);
  }
  return 0;
}

/* hack for receiving motion notifications
*/
static long parse_motion_request(const char* buf, size_t sz, struct global_state* state )
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
    r = strncmp(buf+4, "/motion", 7);
    if(r) {
        return 404;
    } else if (r==0) {
#ifdef INFO
      printf("parse_motion_request: MOTION\n");
#endif
      r = strncmp(buf+11, "?Message=",9);
      if(r){
	printf("Warning: parse_motion_request: no Message param, setting to zero\n");
	state-> t_remote = 0; 
      } else {
	const char* c = buf+11+9;
	state-> t_remote = atol(c);

#ifdef INFO
	printf("parse_motion_request: timestamp=%ld\n",state->t_remote);
#endif
      }
      state-> t_local = time(0);
      return 1;
    }else {
        return 0;
    }
}


static int try_accept_motion(struct global_state* state)
{
    int result = 0;
    state->motioncfd = accept(state->motionfd, (struct sockaddr*)NULL, NULL);
    if(state->motioncfd < 0) {
        result = errno;
    } else {
#ifdef DEBUG
      printf("try_accept_motion: accepted connection\n");
#endif
    }
    return result;
}


/* The function for the thread that gets motion notifications.
 * The state struct pointer is passed as void* due to the pthreads API
*/

static void* motion_task(void *ctxt)
{
    struct global_state* state = ctxt;
    char buf[1024] ={0};
    intptr_t result=0;

    state->mfd.fd = state->motionfd;
    state->mfd.events=POLLIN;
    while(state->running) {
        int ret; // result of poll

#ifdef DEBUG
	printf("motion_task: listening\n");
#endif
        do {
            ret = poll(&state->mfd, POLLIN, 200);
#ifdef DEBUG_VERBOSE
            printf("motion_task poll returns %d\n", ret);
#endif
        } while(ret==0 && state->running);

        if(ret <0) {
            perror("motion_task poll");
            result = errno;
            goto failed_poll;
        } else if (state->running) {
	    intptr_t res=0;
            if(try_accept_motion(state)) {
              perror("motion_task try_accept");
              result = errno;
            } else {
	      ssize_t rres = read(state->motioncfd, buf, 1024);
	      if(rres < 0) {
		perror("motion_task: read");
		result = errno;
		goto failed_read;
	      }
#ifdef DEBUG
	      printf("motion_task read: rres = " FORMAT_FOR_SIZE_T "\n", rres);
	      printf("buf[%s]\n", buf);
#endif
	      int hres = parse_motion_request(buf, 1024, state);
	      if(hres == 1) {
#ifdef INFO
		  printf("motion_task: MOTION\n");
#endif
	      }
	      printf("sending response\n");
	      write_string(state->motioncfd,"HTTP/1.0 200 OK\nContent-Length: 0\n");
	      if((res = close(state->motioncfd)) ) {
	      	perror("failed to close motioncfd");
	      	return (void*) res;
	      }
	    }
        }
    }
failed_poll:
failed_read:
    printf("motion_task exiting\n");
    return (void*) result;
}



static int try_accept(struct global_state* state)
{
    int result = 0;
#ifdef MULTI_CLIENT_THREADS
    // allocate client context. This should be freed in the client thread
    struct client* client = malloc(sizeof(*client));
    if(client == NULL) {
	perror("malloc client");
	result = errno;
	goto failed_to_alloc_client;
    }
#else
    struct client c; // if single-threaded, the client can be on the stack
    struct client* client = &c;
#endif
    client->state = state;
    client->connfd = accept(state->listenfd, (struct sockaddr*)NULL, NULL);
    if(client->connfd < 0) {
        result = errno;
    } else {
#ifdef INFO
	printf("accepted connection\n");
#endif
#ifdef MULTI_CLIENT_THREADS
	pthread_t local_thread;
	pthread_t* client_thread=&local_thread;
	if (pthread_create(client_thread, 0, serve_client, client)) {
            perror("create client_thread");
            result = errno;
        } else if(pthread_detach(*client_thread)) {
	        perror("pthread_detach");
		result = errno;
	}
#else
	serve_client(client);
#endif
    }
#ifdef MULTI_CLIENT_THREADS
failed_to_alloc_client:
#endif
    return result;
}

static int create_bg_threads(struct global_state* state)
{
    int result = 0;
    if (pthread_create(&state->bg_thread, 0, bg_task, state)) {
        perror("creating bg thread");
        result = errno;
        goto failed_to_start_bg_thread;
    }
    if (pthread_create(&state->ui_thread, 0, ui_task, state)) {
        perror("creating ui thread");
        result = errno;
        state->running=0;
        goto failed_to_start_ui_thread;
    }
    if (pthread_create(&state->motion_thread, 0, motion_task, state)) {
        perror("creating motion thread");
        result = errno;
        state->running=0;
        goto failed_to_start_motion_thread;
    }
    pthread_detach(state->ui_thread);
failed_to_start_motion_thread:
failed_to_start_ui_thread:
failed_to_start_bg_thread:
    return result;
}
static void join_bg_thread(pthread_t* bg_thread, const char* msg)
{
    void* status;
    if(pthread_join(*bg_thread, &status)){
        perror("join bg_tread");
    } else {
        printf("Join bg thread (%s): status = " FORMAT_FOR_SIZE_T "\n",
	       msg, (intptr_t) status);
    }
}
int serve_clients(struct global_state* state)
{
    int result=0;
    state->pfd.fd = state->listenfd;
    state->pfd.events=POLLIN;
    while(state->running) {
        int ret; // result of poll

#ifdef DEBUG
	printf("serve_clients: listening\n");
#endif

        do {
            ret = poll(&state->pfd, POLLIN, 2000);
#ifdef DEBUG_VERBOSE
            printf("client poll returns %d\n", ret);
#endif
        } while(ret==0 && state->running);

        if(ret <0) {
            perror("poll");
            result = errno;
            goto failed_poll;
        } else if (state->running) {
            if(try_accept(state)) {
              perror("try_accept");
              result = errno;
            };
        }
    }
failed_poll:
    return result;
}

void init_global_state(struct global_state* state)
{
    pthread_mutex_lock(&global_mutex);
    state->running=0;
    state->quit=0;
    state->t_local=0;
    state->t_remote=0;
    pthread_mutex_unlock(&global_mutex);
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
    if(setsockopt(state->listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
      perror("setsockopt listenfd");
      return errno;
    }

    if(setsockopt(state->motionfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))){
      perror("setsockopt motionfd");
      return errno;
    }
    set_socket_sigpipe_option(state->listenfd);
    set_socket_sigpipe_option(state->motionfd);
    return 0;
}

int bind_and_listen(struct global_state* state, int port, int motion_port)
{
    struct sockaddr_in serv_addr;
    struct sockaddr_in motion_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    memset(&motion_addr, 0, sizeof(motion_addr));
    motion_addr.sin_family = AF_INET;
    motion_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    motion_addr.sin_port = htons(motion_port);

    if( bind(state->listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        perror("bind listenfd");
        return errno;
    }

    if(listen(state->listenfd, 10)){
        perror("listen listenfd");
        return errno;
    }
    if( bind(state->motionfd, (struct sockaddr*)&motion_addr, sizeof(motion_addr))) {
        perror("bind motionfd");
        return errno;
    }

    if(listen(state->motionfd, 10)){
        perror("listen motionfd");
        return errno;
    }
    return 0;
}

struct global_state state;

int main(int argc, char *argv[])
{
    int port;
    int result=0;

    if(argc==2) {
        printf("interpreting %s as port number\n", argv[1]);
        port = atoi(argv[1]);
    } else {
        port = 9091;
    }
    printf("starting on port %d (motion port=%d)\n", port,MOTION_PORT);

    if(install_handlers()) perror("installing signal handlers:");

    init_global_state(&state);

    if(create_socket(&state)) {
        goto no_server_socket;
    }

    if(bind_and_listen(&state, port, MOTION_PORT)) {
        goto failed_to_listen;
    }
    pthread_mutex_lock(&global_mutex);
    state.running = 1;
    pthread_mutex_unlock(&global_mutex);

    if(create_bg_threads(&state)) {
        goto failed_to_start_bg_threads;
    }

    serve_clients(&state);

    join_bg_thread(&state.bg_thread, "bg_thread");
    join_bg_thread(&state.motion_thread, "motion_thread");

failed_to_start_bg_threads:
failed_to_listen:
    if(close(state.listenfd)) perror("closing listenfd");
    if(close(state.motionfd)) perror("closing motionfd");
no_server_socket:
  return result;
}

// signal handler to do a clean shutdown on Ctrl-C
static void handle_sigint(int sig)
{
    do_quit(&state);
}

// install signal handlers
static int install_handlers()
{
    void (*ret)(int) = signal(SIGINT,handle_sigint);
    return ret == SIG_ERR;
}
