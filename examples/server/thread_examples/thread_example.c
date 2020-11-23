
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <pthread.h>
#include <errno.h>


pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;

struct global_data{
    const int count;
};

void * task_a(void * ctx)
{
    struct global_data *d = ctx;
    int i;

    printf("started task_a\n");
    for(i=0; i < d->count; ++i){
	printf("task_a: %d\n",i);
    }
    return (void*) 17;
}

void * task_b(void * ctx)
{
    struct global_data *d = ctx;
    int i;

    printf("started task_b\n");
    for(i=0; i < d->count; ++i){
	printf("task_b: %d\n",i);
    }
    return (void*) 42;
}


int main()
{
    struct global_data data = {10};
    pthread_t thread_a;
    pthread_t thread_b;
#ifdef RETURN_INT
    void *ret_a = 0;
    void *ret_b = 0;
#endif


    if(pthread_create(&thread_a, NULL, task_a, &data)){
	printf("Failed to create thread_a\n");
	exit(1);
    }
    if(pthread_create(&thread_b, NULL, task_b, &data)){
	printf("Failed to create thread_b\n");
	exit(2);
    }
#ifdef RETURN_INT
    pthread_join(thread_a, &ret_a);
    pthread_join(thread_b, &ret_b);
    printf("thread_a returned: %d\nthread_b returned: %d\n", (int)(long)ret_a, (int)(long)ret_b);
#else
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
#endif
   return 0; 
}
