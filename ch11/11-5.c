#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

struct foo{
	int  f_count;
	pthread_mutex_t f_lock;
	/*  more stuff here ... */
};

struct foo* foo_alloc(void)  /* allocate the object */
{
	struct foo *fp;

	if((fp = malloc(sizeof(struct foo))) != NULL)
	{
		fp->f_count = 1;
		if(pthread_mutex_init(&fp->f_lock, NULL) != 0)
		{
			free(fp);
			return NULL;
		}
		/* continue initialization */
	}
	return fp;
}

void foo_hold(struct foo* fp) 	/* add a reference to the object */
{
	pthread_mutex_lock(&fp->f_lock);
	fp->f_count++;
	pthread_mutex_unlock(&fp->f_lock);
}

void foo_rele(struct foo* fp) 	/* release a reference to the object */
{
	pthread_mutex_lock(&fp->f_lock);
	if(--fp->f_count == 0){ /* last reference */
		pthread_mutex_unlock(&fp->f_lock);
		pthread_mutex_destroy(&fp->f_lock);
		free(fp);
		fp = NULL;
	}
	else
		pthread_mutex_unlock(&fp->f_lock);
}
void* thr_fn(void* arg)
{
	int i;
	printf("in thr_fn\n");
	for(i = 0; i<10; ++i)
		foo_hold((struct foo*)arg);
	printf("after thr_fn\n");
	return ((void*)0);
}
int main(void)
{
	pthread_t tid;
	int i;
	struct foo* fp = foo_alloc();
	pthread_create(&tid, NULL, thr_fn, fp);
#if 1
	for(i=0; i<10; ++i)
	{
//		pthread_mutex_lock(&fp->f_lock);
		fp->f_count++;
//		pthread_mutex_unlock(&fp->f_lock);
	}
#endif
	pthread_join(tid, NULL);//不加join的话，可能导致主进程在线程执行前退出
	printf("%d\n",fp->f_count);
	if(fp != NULL)
	{
		free(fp);
		fp = NULL;
	}

}
