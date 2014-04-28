#include "apue.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
int main(void)
{
	pid_t pid;
	pid_t session;
	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid > 0)   /* parent */
	{
	}else 				/* child */
	{
		session = setsid();
		if(session < 0)
		{
			printf("setsid error\n");
			exit(1);
		}
		else
		{
			printf("sid is %d\n", getsid(0));
			/* /dev/tty 是控制终端的同义语, 打开失败的话，表示没有控制终端*/
			if(open("/dev/tty", O_RDWR) < 0)
			{
				printf("no controlling termina\n");
				exit(1);
			}
			exit(0);
		}
	}

	return 0;
}
