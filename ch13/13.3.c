#include "apue.h"
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
void daemonize(const char* cmd)
{
	int i, fd0, fd1, fd2;
	pid_t pid;

	struct rlimit rl;
	struct sigaction sa;
	/* clear file creation mask. */
	umask(0);
	/*
	 * Get maximum number of file descriptors.
	 */
	if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
		err_quit("%s: can't get file limit", cmd);

	/*
	 * Become a session leader to loss controlling TTY.
	 */
	if((pid = fork()) < 0)
		err_quit("%s: can't fork", cmd);
	else if(pid != 0)  //parent
		exit(0);
	setsid();

	printf("setsid success\n");
	/*
	 * Ensure future opens won't allocate controlling TTYs.
	 */
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP, &sa, NULL) < 0)
		err_quit("%s: can't ignore SIGHUP", cmd);
	if((pid = fork()) < 0)
		err_quit("%s: can't fork", cmd);
	else if(pid != 0)  //parent
		exit(0);

	printf("second child\n");
	/*
	 * change the current working directory to the root so
	 * we won't prevent file systems from being unmounted.
	 */
	if(chdir("/tmp") < 0)
		err_quit("%s: can't change directory to /", cmd);
	else
		printf("chdir successed\n");
	/*
	 * Close all open file descriptors.
	 */
	printf("SB\n");
	if(rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	printf("SIIIIIIIIB\n");
	for(i=0; i<rl.rlim_max; ++i)
	{
		if(close(i) != 0)
			printf("SLKIJSJFKDS\n");
	}

	printf("SKKKKKKKK\n");
	printf("rlimit_max:%d\n",rl.rlim_max);
	/*
	 * Attach file descriptors 0, 1 and 2 to /dev/null
	 */
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	/*
	 * Initialize to log file.
	 */
#if 1
	printf("before openlog\n");
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	printf("after openlog\n");
	if(fd0 != 0 || fd1 != 1 || fd2 != 2)
	{
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
		printf("fd error\n");
		exit(1);
	}
#endif
	printf("OKKKK\n");
#if 0
	FILE* fp;
	time_t t;
	while(1)
	{
		sleep(60); 
		fp = fopen(cmd,"a");
		if(fp >= 0)
			fprintf(fp, "im here %s\n", asctime(localtime(&t)));
		fclose(fp);
	}
#endif
}

int main(void)
{
	FILE *fp;
	char* p;
	daemonize("getlog");
	p = getlogin();
	fp = fopen("/tmp/getlog.out", "w");
	if(fp != NULL)
	{
		printf("KKKKKKKKKKKKKKK\n");
		if(p == NULL)
			fprintf(fp, "no login name\n");
		else
			fprintf(fp, "login name %s\n", p);
	}
	else
		printf("JJJJJJJJJJJJJJJJJJJJ\n");

	exit(0);
}
