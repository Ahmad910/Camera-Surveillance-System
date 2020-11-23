#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFSZ 1024
#define SERVER_PORT 5000

static int bind_socket(int fd, int port);

/*
 * create a server socket bound to port
 * and listening.
 *  
 * return positive file descriptor 
 * or negative value on error
 */
int create_socket(int port)
{
    int fd = -1;
    
    if(port < 0 || port > 65535) {
       errno = EINVAL;
       return -1;
    }
    fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(fd < 0) return fd;
    if(bind_socket(fd,port) < 0) return -1;
    printf("simple_udp_server: bound socket to port %d\n",port);
    return fd;
}

static int bind_socket(int fd, int port){

    struct sockaddr_in addr;
    // if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
    //     perror("setsockopt");
    //     return -1;
    // }
    
    /* see man page ip(7) */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(fd, (struct sockaddr*) &addr, sizeof(addr))) return -1;

#ifdef INFO
    printf("simple_server: bound fd %d to port %d\n",fd,port);
#endif
    
    return fd;
}

/*
 * Wait for one datagram and echo it back
 */
static int do_serve(int fd)
{
    char buf[BUFSZ];
    struct sockaddr_in remote_end;
    socklen_t remote_size = sizeof(remote_end);
    
   //  const char* msg = "Hello, socket!\n"
   //                    "I am a text\n"
   //                    "BYE.\n";
   // size_t len = strlen(msg);
   ssize_t len;
    
    printf("simple_server: waiting for datagram on fd %d\n",fd);
    
    len = recvfrom(fd, buf, BUFSZ-1, 0, (struct sockaddr*) &remote_end, &remote_size);
    if(len == -1){
        perror("recvfrom");
        return len;
    }
    buf[len] = 0; /* add null termination to string*/
    

    printf("simple_server: received packet from %s:%d, len=%zu\n%s\n",
           inet_ntoa(remote_end.sin_addr),
           ntohs(remote_end.sin_port),
           len,
           buf);
    
    len = sendto(fd, buf, len, 0, (struct sockaddr*) &remote_end, remote_size);
    if(len == -1){
        perror("sendto");
        return len;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char* server_name;
    int port;
    int fd;

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

    // printf("simple_client: connecting to server: %s, port %d\n",server_name, port);
    fd = create_socket(port);

    if(fd < 0){
        perror("create_socket");
        return 1;
    }

    do_serve(fd);

    printf("simple_server: closing socket: %d\n", fd);
    close(fd);

    return 0;
}

