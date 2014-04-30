#include <stdio.h>
#include <pthread.h>

int cnt = 1;

void* pth_fn(void* arg)
{
	int i=0;
	for(i=0; i<10000; ++i)
		++cnt;
	return ((void*)0);
}

int main(void)
{
	pthread_t tid, tid2;
	int i;
	pthread_create(&tid, NULL, pth_fn, NULL);
	
	pthread_create(&tid2, NULL, pth_fn, NULL);
	for(i=0; i<1000; ++i)
		++cnt;

	pthread_join(tid, NULL);
	pthread_join(tid2, NULL);
	
	printf("%d\n", cnt);
	return 0;
}
