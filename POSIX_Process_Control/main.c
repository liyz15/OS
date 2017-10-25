#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

main()
{
    int i;
    pid_t child;
    if ((child = fork()) == -1)
    {
        printf("Fork Error.\n");
        exit(1);
    }
    if (child == 0)
    {
        printf("Now it is in child process.\n");
        if (execl("/home/liyz/OS/POSIX_Thread_Control/test", "test", NULL) == -1)
        {
            perror("Error in child process");
            exit(1);
        }
        exit(0);
    }
    printf("Now it is in parent process.\n");
    for (i = 0; i < 10; i++)
    {
        sleep(2);
        printf("Parent in loop: %d\n", i);
        if (i == 2)
        {
            if ((child = fork()) == -1)
            {
                printf("Fork Error.\n");
                exit(1);
            }
            if (child == 0)
            {
                printf("Now it is in child process.\n");
                if (execl("/home/liyz/OS/POSIX_Thread_Control/test", "test", NULL) == -1)
                {
                    perror("Error in child process");
                    exit(1);
                }
                exit(0);
            }
        }
        if(i==3)
        {
            pid_t temp;
            temp = wait(NULL);
            printf("Child process ID: %d\n", temp);
        }
    }
    exit(0);
}