#include "apue.h"
#include <pthread.h>

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

void preparing(void)
{
	printf("preparing locks ... \n");
	pthread_mutex_lock(&lock1);
	pthread_mutex_lock(&lock2);
}

void parent(void)
{
	printf("parent unlocks ... \n");
	pthread_mutex_unlock(&lock1);
	pthread_mutex_unlock(&lock2);
}
void child(void)
{
	printf("child unlocks ... \n");
	pthread_mutex_unlock(&lock1);
	pthread_mutex_unlock(&lock2);
}

void* thr_fn(void* arg)
{
	printf("thread started...\n");
	pause();
	return 0;
}
int main(void)
{
	int err;
	pthread_t tid;
	pid_t pid;

	if((err = pthread_create(&tid, NULL, thr_fn, 0)) != 0)
		err_exit(err, "pthread_create error");

	if((err = pthread_atfork(preparing, parent, child)) != 0)
		err_exit(err, "pthread_atfork error");

	sleep(2); 	
	printf("parent about to fork...\n");
	if((pid = fork()) < 0)
		err_quit("fork failed\n");
	else if(pid == 0)
		printf("child returned from fork\n");
	else
		printf("parent returned from fork\n");
	return 0;
}
