#include "apue.h"
#include <sys/wait.h>
#include <unistd.h>
char* env_init[] = {"USER=unknown","PATH=/tmp", NULL};

int main(void)
{
	pid_t pid;
	if((pid =fork()) < 0)
		err_sys("fork error");
	else if(0 == pid)
	{
		/* echoall is the executable file generate by 8-9.c */
		if(execle("/home/klion26/echoall","echo","arg1","arg2", (char *)0, env_init) < 0)
			err_sys("execle error");
	}

	if(waitpid(pid, NULL, 0) < 0)
		err_sys("wait error");

	if((pid = fork()) < 0)
	{
		err_sys("fork error");
	} else if(0 == pid)
	{
		if(execlp("echoall","echo","only 1 arg", (char*)0)<0)
			err_sys("execlp error");
	}
	exit(0);
}
