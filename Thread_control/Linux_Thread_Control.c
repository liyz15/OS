#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>

#define STACK_SIZE 4096

int flag;

void *test(void *arg)
{
    int childnum;
    childnum = *(int *)arg;
    flag = 1;
    for (unsigned int i = 0; i < 10; i++)
    {
        sleep(1);
        printf("Thread %d work cycle  Loop: %u\n", childnum, i);
    }
    return NULL;
}

int main()
{
    pid_t pid;
    int childno = 1, mainnum = 0;
    void *csp, *tcsp;

    csp = (char *)malloc(STACK_SIZE);
    if (csp)
    {
        tcsp = csp + STACK_SIZE;
    }
    else
    {
        exit(errno);
    }

    flag = 0;
    childno = 1;

    if ((pid = clone((void *)&test, tcsp, CLONE_VM, (void *)&childno)) < 0)
    {
        printf("Couldn't create new therad!\n");
        exit(1);
    }
    else
    {
        while (flag == 0)
            ;
        printf("Just created thread %d\n", pid);
    }
    test(&mainnum);
    sleep(3);
    printf("Main program is now shutting down\n");
    return 0;
}