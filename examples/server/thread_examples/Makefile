CC=gcc

PRGS=thread_example thread_example_monitor thread_example_semaphore
.PHONY: all clean distclean
all: $(PRGS)

# CPPFLAGS+=-DDEBUG
CFLAGS+=-g -Wall -Werror -pedantic -pedantic-errors
LDLIBS=-lpthread

%: %.c

clean:

distclean: clean
	-rm $(PRGS)
