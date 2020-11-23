/*
A simple example of a motion detection client that periodically connects
to the motion server and prints the response on stdout.
 */
#include "server_common.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef MOTION_CLIENT_PORT
#define MOTION_CLIENT_PORT 9091
#endif

void init(const char* server_name, int port);
int poll_motion(int period);
static int install_handlers();
static void cleanup(int);
static void socket_init();
static void socket_close();
static void motion_get();


int running=1;
int motionfd=-1;
struct hostent *motion_server;
struct sockaddr_in motion_serv_addr;

void init(const char* server_name, int port)
{
    motion_server = gethostbyname(server_name);
    if (motion_server == NULL) {
	fprintf(stderr,"ERROR, motion_server name not found\n");
	exit(1);
    } else {
	bzero((char *) &motion_serv_addr, sizeof(motion_serv_addr));
	motion_serv_addr.sin_family = AF_INET;
	bcopy((char *)motion_server->h_addr, (char *)&motion_serv_addr.sin_addr.s_addr, motion_server->h_length);
	motion_serv_addr.sin_port = htons(port);
    }
}

int main(int argc, char *argv[])
{
    int period=1000;
    const char* server_name;
    int port;

    if(argc>=2) {
	period = atoi(argv[1]);
    }
    if(argc>=3) {
	server_name = argv[2];
    } else {
	server_name = "localhost";
    }
    if(argc==4) {
	port = atoi(argv[3]);
    } else {
	port = MOTION_CLIENT_PORT;
    }

    printf("polling motion server: %s, port %d\n",server_name, port);

    init(server_name, port);

    if (install_handlers()){
	perror("install_handlers");
    } else {
	poll_motion(period);
    }
    return 0;
}

// poll motion detection server. period in ms
int poll_motion(int period)
{
    while(running){
	printf("polling motion server\n");
	motion_get();
	usleep(1000*period);
    }
    return 0;
}

#define BUFSZ 100
static void motion_get()
{
    char msg[BUFSZ];
    snprintf(msg, BUFSZ, "GET /motion.txt \n");
#ifdef DEBUG
    printf("motion_get: %s\n", msg);
#endif
    socket_init();
    if (connect(motionfd, (struct sockaddr*)&motion_serv_addr, sizeof(motion_serv_addr)) < 0) {
	perror("ERROR connecting");
    } else {
	if(write_string(motionfd, msg) < 0){
	    perror("ERROR writing to motion server");
	} else {
	    int res = read(motionfd, msg, BUFSZ-1);
	    if(res < 0) {
		perror("ERROR reading from motion server");
	    } else {
		msg[res]='\0'; // ensure msg is null terminated
		printf("response: %s\n",msg);
	    }

	}
	socket_close();
    }
}

static void socket_init()
{
    motionfd = socket(AF_INET, SOCK_STREAM, 0);
    if (motionfd < 0) {
	perror("creating motion socket");
    }
}
static void socket_close()
{
    if (motionfd) {
	if(close(motionfd)){
	    perror("closing motion socket");
	}
    }
    motionfd=0;
}

// signal handler to do cleanup
static void cleanup(int sig)
{
    running=0;
    socket_close();

}

// install signal handlers for cleaning up on ctrl-c
static int install_handlers()
{
    void (*ret)(int) = signal(SIGINT,cleanup);
    return ret == SIG_ERR;
}
