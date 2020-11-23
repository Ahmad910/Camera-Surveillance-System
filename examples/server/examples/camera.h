#ifdef FAKE
#include "fakecapture.h"
#else
#include "capture.h"
#endif

#define IMAGE_JPEG "image/jpeg"

#undef MOTION_FLAG_IN_FRAMES
/* A wrapper for the Axis capture API
 */

struct camera{
#ifdef FAKE
    int frame_nbr; // current position in fake stream
                   // use this for faking motion detection
                   // and for returning the correct "next" frame
#endif
    media_stream* stream;
};

// possibly
typedef struct camera camera;
typedef struct frame frame;
typedef char byte;

struct frame{
    media_frame* fr;
#ifdef MOTION_FLAG_IN_FRAMES
    int motion;
#endif
};

struct camera* camera_open();
void camera_close(struct camera*);

struct frame* camera_get_frame(struct camera*);

byte* get_frame_bytes(struct frame*);

size_t get_frame_size(struct frame*);

capture_time get_frame_timestamp(struct frame*);

#ifdef MOTION_FLAG_IN_FRAMES
int get_frame_motion(struct frame*);
#endif

size_t get_frame_height(struct frame*);

size_t get_frame_width(struct frame*);

void frame_free(struct frame*);


