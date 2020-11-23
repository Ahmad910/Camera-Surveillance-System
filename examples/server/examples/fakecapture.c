#include <math.h>
#ifndef NO_FAKE_MOTION
#include "server_common.h"
#endif
#include <time.h>
#include "fakecapture.h"


// compile-time options:
// prefereably, set using -D<MACRONAME> in the Makefile
// or on the command line
//
// define to enable debug output,
// #define DEBUG
//
// define to use a short sequence of frames with numbers 1 -- 5,
// undefine to use an actual film sequence (man in corridor)
// #define SHORT_NUMBER_FRAME_SEQUENCE
//
// we need sockets for fake motion detection,
// define to turn fake motion detection off
// #define NO_FAKE_MOTION

#ifdef SHORT_NUMBER_FRAME_SEQUENCE
#define MEDIA_FRAME_STR "media/%d.jpg"
#define START_FRAME 1
#define NUM_FRAMES 5
#else
#define MEDIA_FRAME_STR "media/film%03d.jpg"
#ifndef START_FRAME
// to skip directly to the parts with motion,
// define START_FRAME to (a bit less than) 86
#define START_FRAME 1
#endif
#define NUM_FRAMES 247
#endif

#define FILEPATH_LENGTH 100

#ifndef NO_FAKE_MOTION
// we need sockets for fake motion detection
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
#include <errno.h>
// #include <string.h>  // for memcpy, memset, strlen

static void fake_motion_detect(int frame_nr);
static void fake_motion_init();
static void fake_motion_free();
int motionfd;
struct hostent *motion_server;
struct sockaddr_in motion_serv_addr;
#endif


// for portability with MacOs
#ifdef __MACH__
#define UNUSED_VALUE 1
#define CLOCK_MONOTONIC UNUSED_VALUE
int clock_gettime(int clk_id, struct timespec* t) {
    struct timeval now;
    int res;
    if((res=gettimeofday(&now, NULL))){
        return res;
    }
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

media_stream *
capture_open_stream(const char *media_type, const char *media_props)
{
    media_stream *res = malloc(sizeof(media_stream));
    res->frame_nr = START_FRAME;
#ifndef NO_FAKE_MOTION
   fake_motion_init();
#endif
    return res;
}

media_frame *
capture_get_frame(media_stream *stream)
{
   int fd;
   char fname[FILEPATH_LENGTH];
   fname[FILEPATH_LENGTH - 1]='\0';
   snprintf(fname, FILEPATH_LENGTH - 1, MEDIA_FRAME_STR, stream->frame_nr++);
   if(stream->frame_nr > NUM_FRAMES) {
      stream->frame_nr = START_FRAME;
   };
#ifdef INFO
   printf("trying to open %s\n", fname);
#endif
   fd = open(fname, O_RDONLY);
   if(fd < 0) {
       perror("fakecapture:capture_get_frame:open");
       return NULL;
   }
   media_frame* res = malloc(sizeof(media_frame));

   struct timespec mono_current_time;
   clock_gettime(CLOCK_MONOTONIC, &mono_current_time);
   res->time = (capture_time) mono_current_time.tv_sec * pow(10, 9) + mono_current_time.tv_nsec;

   res->sz = read(fd, res->data, FRAME_BUF_SZ);
   if(res->sz < 0) {
       perror("fakecapture:capture_get_frame:read");
       free(res);
       res=NULL;
   }
   if(close(fd)){
       perror("Warning! fakecapture:capture_get_frame:close");
   }

#ifndef NO_FAKE_MOTION
   fake_motion_detect(stream->frame_nr);
#endif

   return res;
}

/**
 * @brief Obtain the data from the media_frame.
 *
 * @param frame The media_frame to obtain data from.
 *
 * @return A pointer to the data, NULL if frame is NULL.
 */
void *
capture_frame_data(const media_frame *frame)
{
    return (void*) frame->data;
}

/**
 * @brief Obtain the data size from the media_frame.
 *
 * @param frame The media_frame to obtain data size from.
 *
 * @return The size of the data, 0 if frame is NULL.
 */
size_t
capture_frame_size(const media_frame *frame)
{
    return frame->sz;
}

 /*
 * @param frame The media_frame to obtain timestamp from.
 *
 * @return The timestamp of the data, 0 if frame is NULL.
 */
capture_time
capture_frame_timestamp(const media_frame *frame)
{
    //fprintf(stderr, "WARNING! capture_frame_timestamp not implemented\n");
    if (!frame)
        return 0;
    return frame->time;
}

/**
 * @brief Obtain the height of the media_frame.
 *
 * @param frame The media_frame to obtain height from.
 *
 * @return The height of the frame, 0 if frame is NULL.
 */
size_t
capture_frame_height(const media_frame *frame)
{
    return 480;
}

/**
 * @brief Obtain the width of the media_frame.
 *
 * @param frame The media_frame to obtain width from.
 *
 * @return The width of the frame, 0 if frame is NULL.
 */
size_t
capture_frame_width(const media_frame *frame)
{
    return 640;
}

void
capture_frame_free(media_frame *frame)
{
    free(frame);
}

void
capture_close_stream(media_stream *stream)
{
    free(stream);
#ifndef NO_FAKE_MOTION
   fake_motion_free();
#endif
}

#ifndef NO_FAKE_MOTION
#ifndef MOTION_PORT
#define MOTION_PORT 9090
#endif
static void fake_motion_notify()
{
    char msg[100];
    snprintf(msg, 100, "GET /motion?Message=%ld\n",time(0));
#ifdef DEBUG
    printf("fake_motion_notify: %s\n", msg);
#endif
    fake_motion_init();
    if (connect(motionfd, (struct sockaddr*)&motion_serv_addr, sizeof(motion_serv_addr)) < 0) {
	perror("ERROR connecting");
    } else {
	if(write_string(motionfd, msg) < 0){
	    perror("ERROR writing to motion server");
	}
	close(motionfd);
    }
}

static void fake_motion_detect(int frame_nr)
{
#ifdef SHORT_NUMBER_FRAME_SEQUENCE
// START_FRAME 1
// NUM_FRAMES 5
    fake_motion_notify();
#else
// START_FRAME 55
// NUM_FRAMES 247
    if(frame_nr > 86 && frame_nr < 219 && (frame_nr % 5 == 0)){
	fake_motion_notify();
    }
#endif

}
static void fake_motion_init()
{
    motionfd = socket(AF_INET, SOCK_STREAM, 0);
    if (motionfd < 0) {
	perror("creating motion socket");
	//TODO: set error flag to avoid using motion
    }
    motion_server = gethostbyname("localhost");

    if (motion_server == NULL) {
	fprintf(stderr,"ERROR, motion_server name not found\n");
    }
    bzero((char *) &motion_serv_addr, sizeof(motion_serv_addr));
    motion_serv_addr.sin_family = AF_INET;
    bcopy((char *)motion_server->h_addr, (char *)&motion_serv_addr.sin_addr.s_addr, motion_server->h_length);
    motion_serv_addr.sin_port = htons(MOTION_PORT);
}
static void fake_motion_free()
{
    //TODO: any more cleanup needed?
    if (close(motionfd)) {
	perror("closing motion socket");
    }
}
#endif
