#include <signal.h>
#include <unistd.h>
#include <stdio.h>
static void sig_alrm(int signo)
{
	/* nothing to do, just return to wake up the pause */
	printf("in sig_alrm\n");
}

unsigned int sleep1(unsigned int nsecs)
{
	printf("before sleep1\n");
	if(signal(SIGALRM, sig_alrm) == SIG_ERR)
		return (nsecs);
	alarm(nsecs);   /* start the timer */
	pause();       /* next caught signal wakes us up */
	printf("after sleep1\n");
	return (alarm(0));  /* turn off timer, return unslept time */
}

int main(void)
{
	printf("before\n");
	sleep1(5);
	printf("after\n");
	return 0;
}
