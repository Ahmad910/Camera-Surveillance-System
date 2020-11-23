#ifndef _SERVER_COMMON_H
#define _SERVER_COMMON_H
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/*
 * Writes n bytes starting at buf to fd, by calling write_n
 * Returns number of bytes written.
 * Returns a negative value if an error occurs and sets errno.
 */
ssize_t write_string(int fd, const char* buf);

/*
 * Writes n bytes starting at buf to fd.
 * Does multiple write calls if needed. 
 * Returns number of bytes written.
 * Returns a negative value if an error occurs and sets errno.
 * Note: an error occurs if the web browser closes the connection
 *   (might happen when reloading too frequently)
 */
ssize_t write_n(int fd, const char* buf, size_t n);




// preprocessor hacks to make the code somewhat portable

// A macro defining the correct format string for printing
// a size_t value using printf
// This is used as follows: (assuming a variable size_t s)
// printf("the size is " FORMAT_FOR_SIZE_T " bytes\n", s);
// which is equivalent to 
// printf("the size is %zu bytes\n", s);
// as in C, adjacent string literals in the source code
// are concatenated by the compiler to a single string literal.
// I.e., // "ab" "cd" "ef"   is eqivalent to "abcdef"
#ifndef FORMAT_FOR_SIZE_T
#define FORMAT_FOR_SIZE_T "%zu"
#endif


// kludge to make send ignoring SIGPIPE compile
// on both linux and MacOs
// server_common.h contains some logic attempting to make this
// portable (linux defines MSG_NOSIGNAL, )

// If you get compilation errors on this or something
// related to the send call, a simple/crude fix 
// is to use write instead by defining USE_WRITE
// (typically by adding -DUSE_WRITE) to CPPFLAGS in your Makefile
// or on the command line
#ifndef USE_WRITE
#include <sys/socket.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#ifdef SO_NOSIGPIPE
#define SERVER_USE_SO_NOSIGPIPE
#else
#warning "Cannot block SIGPIPE, using write instead of send"
#define USE_WRITE
#endif
#endif
#endif

/* aux function to be called when setting up a socket.
   Sets SO_NOSIGPIPE if needed (e.g., on MacOS)
   Doesn't do anything on Linux
   returns 0 on successs, errno on error
 */
int set_socket_sigpipe_option(int fd);

#endif
