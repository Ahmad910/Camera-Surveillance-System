/*
A minimal example of a TCP client
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef SERVER_PORT
#define SERVER_PORT 5000
#endif

void init(const char* server_name, int port);
static void socket_init();
static void socket_close();
static void make_request();


int sockfd=-1;
struct hostent *server;
struct sockaddr_in serv_addr;

void init(const char* server_name, int port)
{
    server = gethostbyname(server_name);
    if (server == NULL) {
        fprintf(stderr,"ERROR, server name not found\n");
        exit(1);
    } else {
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(port);
    }
}

int main(int argc, char *argv[])
{
    const char* server_name;
    int port;

    if(argc>=2) {
        server_name = argv[2];
    } else {
        server_name = "localhost";
    }
    if(argc==3) {
        port = atoi(argv[3]);
    } else {
        port = SERVER_PORT;
    }

    printf("simple_tcp_client: connecting to server: %s, port %d\n",server_name, port);

    init(server_name, port);
    make_request();

    return 0;
}

#define BUFSZ 100
static void make_request()
{
    char msg[BUFSZ];
#ifdef DEBUG
    printf("simple_tcp_client: connecting\n");
#endif
    socket_init();
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
    } else {
        int res = read(sockfd, msg, BUFSZ-1);
        if(res < 0) {
            perror("ERROR reading from motion server");
        } else {
            msg[res]='\0'; /* ensure msg is null terminated */
            printf("simple_tcp_client: response: %s\n",msg);
        }
        socket_close();
    }
}

static void socket_init()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("creating motion socket");
    }
}
static void socket_close()
{
    if (sockfd) {
        if(close(sockfd)){
            perror("closing motion socket");
        }
    }
    sockfd=-1;
}
