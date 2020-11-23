#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <pthread.h>
#include <semaphore.h>

#include <errno.h>



struct global_data{
    int count;
    pthread_mutex_t mtx;
    sem_t sem_a;
    sem_t sem_b;
};

void * task_a(void * ctx)
{
    struct global_data *d = ctx;
    int i;

    printf("started task_a\n");
    for(i=0; i < d->count; ++i){
	sem_wait(&d->sem_a);
	pthread_mutex_lock(&d->mtx);
	printf("task_a: %d\n",i);
	sem_post(&d->sem_b);
	pthread_mutex_unlock(&d->mtx);
    }
    return NULL;
}

void * task_b(void * ctx)
{
    struct global_data *d = ctx;
    int i;

    printf("started task_b\n");
    for(i=0; i < d->count; ++i){
	sem_wait(&d->sem_b);
	pthread_mutex_lock(&d->mtx);
	printf("task_b: %d\n",i);
	sem_post(&d->sem_a);
	pthread_mutex_unlock(&d->mtx);
    }
    return NULL;
}

int global_data_init(struct global_data *d)
{
    d->count = 10;
    pthread_mutex_init(&d->mtx, NULL);
    if(sem_init(&d->sem_a, 0, 0)){
	return 1;
    }
    if(sem_init(&d->sem_b, 0, 0)){
	return 1;
    }
   return 0; 
}

int main()
{
    struct global_data data;
    pthread_t thread_a;
    pthread_t thread_b;
    int res;
    
    if(global_data_init(&data)){
	perror("global_data_init");
	exit(1);
    }

    if((res=pthread_create(&thread_a, NULL, task_a, &data))){
	errno = res;
	perror("Failed to create thread_a\n");
	exit(2);
    }
    if((res=pthread_create(&thread_b, NULL, task_b, &data))){
	errno = res;
	perror("Failed to create thread_a\n");
	exit(3);
    }
    sleep(1);
    pthread_mutex_lock(&data.mtx);
    printf("signalling semaphore\n");
    sem_post(&data.sem_a);
    pthread_mutex_unlock(&data.mtx);
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
   return 0; 
}
