#include "camera.h"
#include <stdio.h>

camera* camera_open(){
	camera *cam = malloc(sizeof(camera));
	if (!cam){
		perror("Out of memory, cannot malloc camera");
		return NULL;
	}
#ifdef FAKE
#ifdef DEBUG
        printf("fake camera: camera_open\n");
#endif

	cam->frame_nbr = 0;
#endif
	cam->stream = capture_open_stream(IMAGE_JPEG, "fps=25&resolution=640x480");
	return cam;
}

void camera_close(camera* cam){
	capture_close_stream(cam->stream);
	free(cam);
}

frame* camera_get_frame(camera* cam){
	frame *f = malloc(sizeof(frame));
	f->fr = capture_get_frame(cam->stream);

#ifdef FAKE
	++cam->frame_nbr;
#endif

#ifdef DEBUG
	printf("camera_get_frame: ts=%llu\n", get_frame_timestamp(f));
#endif
	return f;
}

byte* get_frame_bytes(frame* f){
	return (byte *) capture_frame_data(f->fr);
}

size_t get_frame_size(frame* f){
	return capture_frame_size(f->fr);
}

capture_time get_frame_timestamp(frame* f){
	return capture_frame_timestamp(f->fr);
}

#ifdef MOTION_FLAG_IN_FRAMES
int get_frame_motion(frame* f){
	return f->motion;
}
#endif

size_t get_frame_height(frame* f){
	return capture_frame_height(f->fr);
}

size_t get_frame_width(frame* f){
	return capture_frame_width(f->fr);
}

void frame_free(frame* f){
	capture_frame_free(f->fr);
	free(f);
}
