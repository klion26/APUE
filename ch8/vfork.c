#include "apue.h"

int main(void)
{
	pid_t pid;
	int cnt = 0;
	pid = vfork();
	if(0 == pid)
	{
		++cnt;
		printf("cnt = %d\n", cnt);
		printf("child:%d\n", getpid);
	}
	else
	{
		++cnt;
		printf("cnt = %d\n", cnt);
		printf("father:%d\n", getpid);
	}

	_exit(0);
}
