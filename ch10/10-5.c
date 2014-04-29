#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

static jmp_buf env_alarm;

static void sig_alarm(int signo)
{
	printf("before sig_alarm\n");
	longjmp(env_alarm, 1);
	printf("after sig_alarm\n");
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

int main(void)
{
	printf("before\n");
	sleep2(5);
	printf("after\n");
	return 0;
}
