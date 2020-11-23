
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <pthread.h>
#include <errno.h>


pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;

struct global_data{
    char task;
    const int count;
};

void * task_a(void * ctx)
{
    struct global_data *d = ctx;
    int i;

    printf("started task_a\n");
    for(i=0; i < d->count; ++i){
	pthread_mutex_lock(&mtx);
	while(d->task != 'a') pthread_cond_wait(&cnd, &mtx);
	printf("task_a: %d\n",i);
	d->task = 'b';
	pthread_cond_signal(&cnd);
	pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

void * task_b(void * ctx)
{
    struct global_data *d = ctx;
    int i;

    printf("started task_b\n");
    for(i=0; i < d->count; ++i){
	pthread_mutex_lock(&mtx);
	while(d->task != 'b') pthread_cond_wait(&cnd, &mtx);
	printf("task_b: %d\n",i);
	d->task = 'a';
	pthread_cond_signal(&cnd);
	pthread_mutex_unlock(&mtx);
    }
    return NULL;
}


int main()
{
    struct global_data data = {0,10};
    pthread_t thread_a;

    pthread_t thread_b;



    if(pthread_create(&thread_a, NULL, task_a, &data)){
	printf("Failed to create thread_a\n");
	exit(1);
    }
    if(pthread_create(&thread_b, NULL, task_b, &data)){
	printf("Failed to create thread_b\n");
	exit(2);
    }
    sleep(1);
    pthread_mutex_lock(&mtx);
    printf("setting task to a\n");
    data.task = 'a';
    pthread_cond_signal(&cnd);
    pthread_mutex_unlock(&mtx);
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
   return 0; 
}
