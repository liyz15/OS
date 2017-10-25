#define _REENTRANT
#include <pthread.h>
#include <stdio.h>

#define NUM_THREADS 5
#define SLEEP_TIME 10

void *sleeping(void *);
int i;
pthread_t tid[NUM_THREADS];

int main()
{
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&tid[i], NULL, sleeping, (void *)(i+1));
    }
    for (i = 0; i < NUM_THREADS;i++)
    {
        pthread_join(tid[i], NULL);
        printf("%u thread terminated\n", i);
    }
    printf("main() reporting that all %d threads have terminated\n", i);
    return (0);
}

void *sleeping(void *arg)
{
    int sleep_time = (int)arg;
    printf("thread %u sleeping %d seconds ...\n", pthread_self(), sleep_time);
    sleep(sleep_time);
    printf("thread %u awakening\n", pthread_self());
    pthread_exit(0);
}