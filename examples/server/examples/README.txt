http_server.c: a simple example of socket communication, interfacing
to the Axis capture API and pthread programming.

A brief overview of the files:

camera.c    	: An interface towards the (fake or real) camera
camera.h

fakecapture.c   : Fake motion capture for use on the PC while testing
fakecapture.h

http_server.c   : A http server getting images from the camera. Allows
		  testing of the capture code from a web browser.

Makefile        : makefile for cross-compiling for the Axis camera
Makefile.fake   : makefile for building the fake versions for testing

motion_client.c : a minimal example of a client connecting to the
		  motion server. Provided for illustration of the protocol.

motion_server.c : A proxy server written to decouple the push notification
                  interface of the Axis camera from the student code, by
		  providing a pull interface.

server_common.c :  common routines and config
server_common.h



About the Makefile

Makefile is the file to use for cross-compiling to the Axis camera.
It is written to work in the computer labs, where the tools and
libraries are found under /usr/local/cs/rtp/ in the directories
under tools.

If compiling elsewhere, if you have the same directory structure as
in /usr/local/cs/rtp, you can use the variable COMMON_DIR to the
location of the compiler and libs.

Makefile.fake is the file to use for building the http server
with fake capture.

When building, you can set options to change its behaviour.
To use a short frame sequence of images of numbers, define
SHORT_NUMBER_FRAME_SEQUENCE, for instance with the command line

> CPPFLAGS="-DSHORT_NUMBER_FRAME_SEQUENCE" make -f Makefile.fake  

To turn off fake motion, define NO_FAKE_MOTION

Example: CPPFLAGS="-DSHORT_NUMBER_FRAME_SEQUENCE -DNO_FAKE_MOTION" make ...
(note that you need separate-D options for each macro)

To use the film sequence (of man in corridor) but skip the first part
without motion, give START_FRAME a value>1. See fakecapture.c for details.


Running

On host, start the two server processes

> ./fake_server

and

> ./motion_server_host

The servers have a simple user interface.
For instance, if you type s (and press enter) it will print some status
information. See the code for details.
To close down the servers cleanly, type q (and press enter) in the
terminal. 
For this reason, it is recommended to run the servers in separate
terminals.

Then, you can browse to localhost:9999 to get an image
and to localhost:9091 to get motion information

Note: if you run just the fake_server without running the motion_server
on the same computer, you will get the output "ERROR connecting:
Connection refused" when motion occurs, as the fake_server
also sends motion notifications to localhost:9090.

The motion client (which polls a motion server) is run
(with the default parameters of polling localhost every second) as

> ./motion_client
