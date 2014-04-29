#include "apue.h"

static void sig_usr(int);  /* one handler for both signals */

int main(void)
{
	/* 每次接到信号对其进行处理时，随即将该信号动作复位为默认值*/
	/* 下面如果发送两次SIGUSR1给程序，会出错 */
	if(signal(SIGUSR1, sig_usr) == SIG_ERR)
		err_sys("can't catch SIGUSR1");
	if(signal(SIGUSR2, sig_usr) == SIG_ERR)
		err_sys("can't catch SIGUSR2");
	for( ; ;)
		pause();
}

static void sig_usr(int signo)  /* argument is signal number */
{
	if(signo == SIGUSR1)
		printf("received SIGUSR1\n");
	else if(signo == SIGUSR2)
		printf("received SIGUSR2\n");
	else
		err_dump("received signal %d\n", signo);
}
