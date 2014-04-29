#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "apue.h"
#if 1
static volatile sig_atomic_t sigflag; 	/* set nonzero by sig handler*/
static sigset_t newmask, oldmask, zeromask;
#endif

#if 1
static void sig_usr(int signo)
{
	sigflag = 1;
}
#endif

#if 1
void TELL_WAIT(void)
{
	if(signal(SIGUSR1, sig_usr) == SIG_ERR)
	{
	  err_sys("signal(SIGUSR1) error");
	  //exit(1);
	}
	if(signal(SIGUSR2, sig_usr) == SIG_ERR)
	{
	  exit(1); //err_sys("signal(SIGUSR2) error");
	}
	sigemptyset(&zeromask);
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	sigaddset(&newmask, SIGUSR2);

	/* 
	 * Block SIGUSR1 and SIGUSR2, and save current signal mask.
	 */
	if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
	{
	 // exit(1);
	  err_sys("SIG_BLOCK error");
	}
}
#endif

#if 1
void TELL_PARENT(pid_t pid)
{
	kill(pid, SIGUSR2);  	/* tell parent we're done */
}
#endif

#if 1
void WAIT_PARENT(void)
{
	while(0 == sigflag)
		sigsuspend(&zeromask); 	/* and wait for parent */
	sigflag = 0;

	/* 
	 * Reset signal mask to original value.
	 */
	if(sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
	{
	  //exit(1);
	  err_sys("SIG_SETMASK error");
	}
}
#endif

#if 1
void TELL_CHILD(pid_t pid)
{
	kill(pid, SIGUSR1); 	/* tell child we're done */
}
#endif
#if 1
void WAIT_CHILD(void)
{
	while(0 == sigflag)
		sigsuspend(&zeromask); 	/* and wait for child */
	sigflag = 0;

	/* 
	 * Reset signal mask to original value.
	 */
	if(sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
	{
	  //exit(1);
	  err_sys("SIG_SETMASK error");
	}
}
#endif


int main(void)
{
#if 1
	pid_t pid;
	FILE *fp;
	int fd;
	int i=0;
	char *child = "in child\n";
	char *parent = "in parent\n";

	if((fp = fopen("test.txt", "a")) == NULL)
	{
		perror("fopen error");
		exit(1);
	}
	fd = fileno(fp);

	fprintf(fp, "%d\n", i);
	fflush(fp);

	TELL_WAIT();
	if((pid = fork()) < 0)
	{
		err_sys("fork error");
	//	perror("fork error");
	//	exit(1);
	}
	else
	{
		if(pid == 0)
		{
			while(i < 100)
			{
				WAIT_PARENT();
				i += 2;
				fprintf(fp, "%d\t", i);
				fflush(fp);

				write(fd, child, strlen(child));
				/*fprintf(fp, "%s\n", child);*/
				/*fflush(fp);*/
				TELL_PARENT(getppid());
			}
		}
		else
		{
			++i;
			while(i < 100)
			{
				fprintf(fp, "%d\t", i);
				fflush(fp);

				i += 2;

				write(fd, parent, strlen(parent));
				TELL_CHILD(pid);
				WAIT_CHILD();
			}
		}
	}
#endif
#if 0
pid_t pid;
    FILE *fp;
    int fd;
    int i = 0;
    char *child = "in child\n";
    char *parent = "in parent\n";

    if ((fp = fopen("tmp", "a")) == NULL) {
        perror("fopen error");
        exit(1);
    }

    fd = fileno(fp);
    /* if (write(fd, pi, 1) != 1) { */
    /*     perror("write error"); */
    /*     exit(1); */
    /* } */
    fprintf(fp, "%d\n", i);
    fflush(fp);

    TELL_WAIT();
    if ((pid = fork()) < 0) { /* pid = fork()要用括号括起来 */
        perror("fork error");
        exit(1);
    } else if (pid == 0) {
        while (i < 100) {
            WAIT_PARENT();
            /* if (write(fd, pi, 1) != 1) { */
            /*     perror("write error"); */
            /*     exit(1); */
            /* } */
            i += 2;
            fprintf(fp, "%d\t", i);
            fflush(fp);

            write(fd, child, strlen(child));   
            TELL_PARENT(getppid());
        }
    } else {
        i++;
        while (i < 100) {
            /* if (write(fd, pi, 1) != 1) { */
            /*     perror("write error"); */
            /*     exit(1); */
            /* } */
            
            fprintf(fp, "%d\t", i);
            fflush(fp);
            i += 2;
            write(fd, parent, strlen(parent));
            TELL_CHILD(pid);
            WAIT_CHILD();
        }
    }
#endif
	return 0;
}
