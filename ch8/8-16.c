#include "apue.h"

int main(void)
{
	pid_t pid;

	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid != 0)   /* parent */
	{
		sleep(2);
		exit(2);       /* terminate with exit status 2 */
	}

	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid != 0)   /* parent */
	{
		sleep(4);
		abort();       /* terminate with core dump */
	}

	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid != 0)   /* parent */
	{
		execl("/bin/dd", "dd", "if=/etc/termcap", "of=/dev/null", NULL);
		exit(7);       /* shouldn't get here */
	}


	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(pid != 0)   /* parent */
	{
		sleep(8);
		exit(0);       /* normal exit */
	}

	sleep(6);
	kill(getpid(), SIGKILL);
	exit(6);  	/* shouldn't get here */
}
