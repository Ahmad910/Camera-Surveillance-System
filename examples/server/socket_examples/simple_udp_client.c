#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string.h>

#define BUFSZ 1024
#define SERVER_PORT 5000

/*
 * create a datagram socket 
 */
int create_socket()
{
    int fd = -1;
    
    fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    return fd;
}

#undef USE_IP_NUMBER 
static int init_remote(struct sockaddr_in* remote_end, socklen_t remote_size, const char* server_str, int port )
{
    memset((char*) &remote_end, 0, remote_size);
    remote_end->sin_family = AF_INET;
    remote_end->sin_port = htons(port);
#ifdef USE_IP_NUMBER
    if (inet_aton(server_str , &remote_end->sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        return -1;
    }
#else
    {
        struct hostent* server;
        server = gethostbyname(server_str);
        if (server == NULL) {
            fprintf(stderr,"ERROR, server name not found\n");
            return -1;
        } else {
            bcopy((char *)server->h_addr, (char *)&remote_end->sin_addr.s_addr, server->h_length);
        }
    }

#endif

    
}

/*
 * Send a datagram to server and wait for one reply 
 */
static int send_and_receive(int fd, const char* server_str, int port)
{
    char buf[BUFSZ];
    struct sockaddr_in remote_end;
    socklen_t remote_size = sizeof(remote_end);

    if(port < 0 || port > 65535) {
       errno = EINVAL;
       return -1;
    }

    printf("simple_udp_client: sending to %s:%d (fd=%d)\n",server_str,port,fd);

    init_remote(&remote_end, remote_size, server_str, port);

    
    const char* msg = "Hello, socket!\n"
                     "I am a text\n"
                      "BYE.\n";
    ssize_t len = strlen(msg);
    len = sendto(fd, msg, len, 0, (struct sockaddr*) &remote_end, remote_size);
    if(len == -1){
        perror("sendto");
        return len;
    }
    printf("simple_udp_client: waiting for datagram on fd %d\n",fd);
    
    len = recvfrom(fd, buf, BUFSZ-1, 0, (struct sockaddr*) &remote_end, &remote_size);
    if(len == -1){
        perror("recvfrom");
        return len;
    }
    buf[len] = 0; /* add null termination to string*/
    

    printf("simple_udp_client: received packet from %s:%d, len=%zu\n%s\n",
           inet_ntoa(remote_end.sin_addr),
           ntohs(remote_end.sin_port),
           len,
           buf);
    
    return 0;
}

int main(int argc, char *argv[])
{
    const char* server_str;
    int port;
    int fd;

    if(argc>=2) {
        server_str = argv[1];
    } else {
#ifdef USE_IP_NUMBER
        server_str = "127.0.0.1";
#else
        server_str = "localhost";
#endif
    }
    if(argc==3) {
        port = atoi(argv[2]);
    } else {
        port = SERVER_PORT;
    }

    // printf("simple_client: connecting to server: %s, port %d\n",server_str, port);
    fd = create_socket();

    if(fd < 0){
        perror("create_socket");
        return 1;
    }


    printf("calling send_and_receive for %s,%d\n",server_str,port);
    send_and_receive(fd, server_str, port);

    printf("simple_udp_client: closing socket: %d\n", fd);
    close(fd);

    return 0;
}

