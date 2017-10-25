#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>

#define STACK_SIZE 4096


void *readOp(void *arg)
{
    double *argp = (double *)arg; //save arg, since it's global, save the pointer doesn't work.
    double p[3];
    for (unsigned int i = 0; i < 3;i++){
        p[i] = argp[i];
    }
    print("%lf\n", p[0]);
    sleep(p[1]);
    printf("Thread %lf start reading.\n", p[0]);
    printf("%lf, %lf, %lf\n", p[0], p[1], p[2]);
    return NULL;
}

int main()
{
    pid_t pid;
    // int beRead = 0, writeWaitNum = 0;
    int order;
    char op[1];
    double delay;
    double duration;
    double arg[3];
    FILE *testFile;
    testFile = fopen("ReadWriteTest.txt", "r");
    void *csp, *tcsp;

    csp = (char *)malloc(STACK_SIZE);
    // if (csp)
    // {
    //     tcsp = csp + STACK_SIZE;
    // }
    // else
    // {
    //     exit(errno);
    // }

    while (!feof(testFile))
    {
        fscanf(testFile, "%d", &order);
        fscanf(testFile, "%s", op);
        fscanf(testFile, "%lf", &delay);
        fscanf(testFile, "%lf", &duration);

        arg[0] = (double)order;
        arg[1] = delay;
        arg[2] = duration;

        if (*op == 'R')
        {
            csp = csp + STACK_SIZE;
            pid = clone((void *)&readOp, csp, CLONE_VM, (void *)&arg);
            printf("%d, %s, %lf, %lf\n", order, op, delay, duration);
            // printf("%lf\n", arg[0]);
            // printf("Thread %d created.\n", order);
        }
    }
    fclose(testFile);
    sleep(5.2);
}