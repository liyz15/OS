#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

main()
{
    int i;
    pid_t CurrentProcessID, ParentProcessID;
    CurrentProcessID = getpid();
    ParentProcessID = getppid();
    printf("Now it is in the program TEST.\n");
    for (i=0;i<10;i++)
    {
        sleep(2);
        printf("Current: %d, Parent: %d, Loop: %d\n", CurrentProcessID, ParentProcessID, i);
    }
    exit(0);
}