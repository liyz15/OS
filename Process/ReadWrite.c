#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 18

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readerWait = PTHREAD_MUTEX_INITIALIZER;

int reading = 0, writing = 0, readerWaitNum = 0;
// Any global variable that can be modified should be protected.

void *readOp(void *arg)
{
    double *p = (double *)arg;

    pthread_mutex_lock(&mutex);
    printf("Thread %d created.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);

    sleep(p[1]);

    pthread_mutex_lock(&mutex);
    printf("Thread %d asked for reading.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&readerWait);
    readerWaitNum++;
    pthread_mutex_unlock(&readerWait);

    while (writing > 0)
        ;
    
    pthread_mutex_lock(&readerWait);
    reading++;
    pthread_mutex_unlock(&readerWait);

    pthread_mutex_lock(&readerWait);
    readerWaitNum--;
    pthread_mutex_unlock(&readerWait);

    pthread_mutex_lock(&mutex);
    printf("Thread %d started reading.\n", (int)p[0]);
    // printf("%d, %d, %d\n", reading, writing, readerWaitNum);
    pthread_mutex_unlock(&mutex);

    sleep(p[2]);

    pthread_mutex_lock(&mutex);
    printf("Thread %d finished reading.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);

    // pthread_mutex_lock(&readerWait);
    reading--;
    // pthread_mutex_unlock(&readerWait);
    return NULL;
}
void *writeOp(void *arg)
{
    double *p = (double *)arg;

    pthread_mutex_lock(&mutex);
    printf("Thread %d created.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);
    sleep(p[1]);

    pthread_mutex_lock(&mutex);
    printf("Thread %d asked for writing.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);

    while (readerWaitNum > 0 || reading > 0)
        ;
    pthread_mutex_lock(&writeMutex);
    writing++;

    pthread_mutex_lock(&mutex);
    printf("Thread %d started writing.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);

    sleep(p[2]);

    pthread_mutex_lock(&mutex);
    printf("Thread %d finished writing.\n", (int)p[0]);
    pthread_mutex_unlock(&mutex);

    writing--;
    pthread_mutex_unlock(&writeMutex);
    return NULL;
}

pthread_t tid[NUM_THREADS];
int main()
{
    int order, opNum = 0;
    char op[1];
    double delay;
    double duration;
    double arg[18][3];
    FILE *testFile;
    testFile = fopen("ReadWriteTest.txt", "r");

    while (!feof(testFile))
    {

        fscanf(testFile, "%d", &order);
        fscanf(testFile, "%s", op);
        fscanf(testFile, "%lf", &delay);
        fscanf(testFile, "%lf", &duration);

        arg[opNum][0] = (double)order;
        arg[opNum][1] = delay;
        arg[opNum][2] = duration;

        if (*op == 'R')
        {
            pthread_create(&tid[opNum], NULL, readOp, (void *)arg[opNum]);
        }
        else if (*op == 'W')
        {
            pthread_create(&tid[opNum], NULL, writeOp, (void *)arg[opNum]);
        }
        opNum++;
    }
    for (unsigned int j = 0; j < opNum; j++)
    {
        pthread_join(tid[j], NULL);
        // printf("%u thread terminated\n", j);
    }
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&writeMutex);
    pthread_mutex_destroy(&readerWait);
}