#include "apue.h"
#include <setjmp.h>
#include <unistd.h>
unsigned int sleep2(unsigned int);
static void sig_int(int);
static jmp_buf env_alarm;
int main(void)
{
	unsigned int unslept;

	if(signal(SIGINT, sig_int) == SIG_ERR)
		err_sys("signal(SIGINT) error");
	unslept = sleep2(5);
	printf("sleep2 returned %u\n", unslept);
	exit(0);
}

static void sig_int(int signo)
{
	int i,j;
	volatile int k;

	/*
	 * Tune these loops to run for more than 5 seconds
	 * on whatever system this test program is run.
	 */
	printf("\nsig_int starting\n");
	for(i=0; i<300000; ++i)
		for(j=0; j<4000; ++j)
			k += i*j;
	printf("sig_int finished\n");
}

static void sig_alarm(int signo)
{
	longjmp(env_alarm, 1);
}
unsigned int sleep2(unsigned int nsecs)
{
	printf("before sleep2\n");
	if(signal(SIGALRM, sig_alarm) == SIG_ERR)
		return nsecs;
	if(setjmp(env_alarm) == 0)
	{
		alarm(nsecs); 		/* start the timer */
		pause(); 			/* next caught signal wakes us up */
	}
	printf("after sleep2\n");
	return alarm(0); 		/* turn off timer, return unslept time */
}
