#include "apue.h"
#include <pthread.h>

int quitflag; 	/* set nonzero by thread */
sigset_t mask;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait = PTHREAD_COND_INITIALIZER;

void* thr_fn(void* arg)
{
	int err, signo;

	for(;;)
	{
		err = sigwait(&mask, &signo);
		if(0 != err)
			err_exit(err, "sigwait failed");
		switch(signo)
		{
			case SIGINT:
				printf("\ninterrupt\n");
				break;
			case SIGQUIT:
				pthread_mutex_lock(&lock);
				quitflag = 1;
				pthread_mutex_unlock(&lock);
				pthread_cond_signal(&wait);
				return 0;
			default:
				printf("unexcepted signal: %d\n", signo);
				exit(0);
		}
	}
}

int main(void)
{
	int err;
	sigset_t oldmask;
	pthread_t tid;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);

	if((err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0)
		err_exit(err, "pthread_sigmask error");

	err = pthread_create(&tid, NULL, thr_fn, NULL);
	if(err != 0)
		err_exit(err, "can't create pthread");

	pthread_mutex_lock(&lock);
	while(quitflag == 0)
		pthread_cond_wait(&wait, &lock);
	pthread_mutex_unlock(&lock);

	quitflag = 0;

	if((err = pthread_sigmask(SIG_BLOCK, &oldmask, NULL)) != 0)
		err_exit(err, "pthread_sigmask error");

	exit(0);
}
