#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

static volatile sig_atomic_t sigflag; /* sig_atomic_t:写此种变量时不会被中断 */
static sigset_t newmask, oldmask, zeromask;

static void sig_usr(int signo)
{
    sigflag = 1;
}

void TELL_WAIT(void)
{
    if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
        perror("signal(SIGUSR1) error");
        exit(1);
    }
    if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
        perror("signal(SIGUSR2) error");
        exit(1);
    }
    sigemptyset(&zeromask);
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigaddset(&newmask, SIGUSR2);

    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        perror("SIG_BLOCK error");
        exit(1);
    }
}

void TELL_PARENT(pid_t pid)
{
    kill(pid, SIGUSR2);
}

void WAIT_PARENT(void)
{
    while (sigflag == 0)
        sigsuspend(&zeromask);
    sigflag = 0;

    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        perror("SIG_SETMASK error");
        exit(1);
    }
}

void TELL_CHILD(pid_t pid)
{
    kill(pid, SIGUSR1);
}

void WAIT_CHILD(void)
{
    while (sigflag == 0)
        sigsuspend(&zeromask);
    sigflag = 0;

    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        perror("SIG_SETMASK error");
        exit(1);
    }
}

int main(void)
{
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

    return 0;
}
 
