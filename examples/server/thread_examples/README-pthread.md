# Introduction to pthreads

These three examples are aimed at showing 
* how to create threads in C using the pthread library
* how to use monitors
* how to use semaphores

Please note that when building programs using the pthread library
it must be linked with `libpthread` by providing the flag `-lpthread`.
See the Makefile for details.

# Creating threads

The  `pthread_create()` function,
```C
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                void *(*start_routine) (void *), void *arg);
```
starts a  new thread  in the
calling  process.   The  new   thread  starts  execution  by
invoking start_routine(); arg is passed as the sole argument
of start_routine().

The sytax of function pointers in C can be somewhat confusing, but the type
```C
void * (*f)(void *)
```
declares a pointer, f, which can point to a function with the signature
```C
void* function(void *p);
```
That is, a function with one `void *` parameter, and return type `void *`.

Before  returning,  a  successful call  to  `pthread_create()`
stores the ID of the new  thread in the buffer pointed to by
thread; this  identifier is used  to refer to the  thread in
subsequent calls to other pthreads functions.

The new thread terminates in one of the following ways:

 * It calls `pthread_exit(3),` specifying  an exit status value
   that is  available to another  thread in the  same process
   that calls `pthread_join(3).`

 * It returns  from `start_routine().`   This is  equivalent to
   calling  `pthread_exit(3)` with  the value  supplied in  the
   return statement.

 * It is canceled (see `pthread_cancel(3)).`

 * Any of  the threads in  the process calls `exit(3),`  or the
   main thread  performs a  return from `main().`   This causes
   the termination of all threads in the process.

## Relevant manual pages:

 pthreads(7)
 pthread_create(3) 
 pthread_attr_init(3)
 pthread_join(3)


## Minimal example

The file thread_example.c contains a minimal example, showing
how to create a thread, how to pass arguments to the thread function
and how to use `pthread_join()` to wait for a thread to finish.
The code shows two alternatives, one ignoring the return value
by passing `NULL` to `pthread_join(),` and one to get the
value (including the type casting needed to pass an int in
a void*.

# Monitors

The file thread_example_monitor.c shows how to implement
rendez-vous using a monitor (i.e., a pthread mutex and a condition variable).

Note that in pthreads, the condition variable is a separate entity,
and the tie to a certain mutex is done by passing pointers to
both the mutex and the condition variable to pthread_cond_wait().
This allows having more than one condition variable, to make
signalling more fine-grained than notifyAll() in Java.

When allocated on the stack, the mutex and condition can be given
default values using initializer macros:
```C
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;
```
Another way to initialise a mutex is to use the function
`pthread_mutex_init().` See the man page for details.

By default, a pthread mutex is not recursive, meaning that if a thread
attemtps to lock a mutex it already holds, it will deadlock. To get the
same semantics as java (i.e., a thread can lock the same mutex multiple
times) the mutex must be created as recursive. See the manual page for
details.

## Relevant manual pages:

 pthread_mutex_init(3)
 pthread_mutex_destroy(3)
 pthread_mutex_lock(3)
 pthread_mutex_trylock(3)
 pthread_mutex_unlock(3)
 pthread_mutexattr_init(3)
 pthread_mutexattr_destroy(3)


# Semaphores

The file thread_example_semaphore.c shows an example
using a mutex for mutual exclusion and a semaphore for signalling.
It also shows how to initialize a mutex that is a struct member,
using `pthread_mutex_init()`.

## Relevant manual pages:

sem_init(3)
sem_destroy(3)
sem_post(3)
sem_wait(3)