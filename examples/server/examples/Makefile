# This makefile is for cross-compiling on student computers, i.e. login.student.lth.se


COMMON_DIR?=/usr/local/cs/rtp

COMPILER_DIR=${COMMON_DIR}/tools/comptools-mips-r12_1.2-0/comptools-mips-r12_1.2-0_amd64/mipsisa32r2el/r12
# Absolute path to the Axis Software Development Kit containing camera specific libs and headers, used for capture.h     and its corresponding object file (i.e. libcapture)
AXIS_PATH=${COMMON_DIR}/tools/emb-app-sdk_1_4/target/mipsisa32r2el-axis-linux-gnu

CC=${COMPILER_DIR}/bin/mipsisa32r2el-axis-linux-gnu-gcc
LD=${COMPILER_DIR}/bin/mipsisa32r2el-axis-linux-gnu-ld
#CC=mipsisa32r2el-axis-linux-gnu-gcc
#LD=mipsisa32r2el-axis-linux-gnu-gcc

# Set PATH to ensure J2C finds the right compiler.
#export PATH=""$COMPILER_DIR"/bin:"$PATH""
# Set to ensure compiler can find platform specific standard libs
#export LIBRARY_PATH=""$COMPILER_DIR"/lib:"$LIBRARY_PATH""

# Tell the compiler where to look for header files used in the auxilliary native methods for the camera classes
#CPPFLAGS+=-I${AXIS_PATH}/include -I${AXIS_PATH}/usr/include
CPPFLAGS+=-I${AXIS_PATH}/usr/include -I${COMPILER_DIR}/mipsisa32r2el-axis-linux-gnu/sys-include
CPPFLAGS+=-DFORMAT_FOR_SIZE_T=\"%u\"
AXIS_LIBCAP=-lcapture # This is needed for the native camera classes to fetch images
AXIS_LIBCAP_DEP=-ldbus-1 -lgobject-2.0 -ldbus-glib-1 -lgthread-2.0 -lglib-2.0 -lrapp -lpthread # Dependencies of libcapture
LDFLAGS+=${AXIS_LIBCAP} ${AXIS_LIBCAP_DEP} ${AXIS_LIBCAP} -L${AXIS_PATH}/lib -L${AXIS_PATH}/usr/lib # Specify which libs are used and set search path for them
CFLAGS+=-O3 -g -Wall
LDLIBS+=-lpthread

all: http_server motion_server

http_server: http_server.o  camera.o server_common.o
http_server.o: http_server.c camera.h 
motion_server: motion_server.o  server_common.o
motion_server.o: motion_server.c
server_common.o: server_common.c server_common.h
camera.o: camera.c camera.h fakecapture.h

clean:
	-rm http_server.o motion_server.o server_common.o camera.o

distclean: clean
	-rm http_server motion_server


.PHONY: all clean

