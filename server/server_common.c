#include "server_common.h"
#include <errno.h>

ssize_t write_string(int fd, const char* buf)
{
    size_t n = strlen(buf);
    return write_n(fd, buf, n);
}
ssize_t write_n(int fd, const char* buf, size_t n)
{
    size_t written = 0;
    while (written < n) {
#ifndef USE_WRITE
	// write to socket using send with the flag MSG_NOSIGNAL, to
	// prevent a failed write (e.g. if the client already closed
	// the socket) from sending a SIGPIPE signal)
#ifdef DEBUG_VERBOSE
	printf("sending using send\n");
#endif
        ssize_t tmp = send(fd, buf, n-written,MSG_NOSIGNAL);
#else
#ifdef DEBUG_VERBOSE
	printf("sending using write\n");
#endif
        ssize_t tmp = write(fd, buf, n-written);
#endif
        if (tmp < 0) {
            perror("write_n ERROR");
            return tmp;
        }
        written += tmp;
    }
    return written;
}

int set_socket_sigpipe_option(int fd)
{
  int r = 0;
#ifdef SERVER_USE_SO_NOSIGPIPE
  int val = 1;
#ifdef DEBUG
  printf("setting SO_NOSIGPIPE\n");
#endif
  r = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&val, sizeof(val));
  if (r) {
    perror("set SO_NOSIGPIPE");
    r = errno;
  }
#endif
  return r;
}
