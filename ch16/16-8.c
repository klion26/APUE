#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/wait.h>

#define MAXADDRLEN    256
#define BUFLEN  128

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

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
	if(chdir("/") < 0)
		err_quit("%s: can't change directory to /", cmd);
	else
		printf("chdir successed\n");
	/*
	 * Close all open file descriptors.
	 */
	if(rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for(i=0; i<rl.rlim_max; ++i)
		close(i);

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
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if(fd0 != 0 || fd1 != 1 || fd2 != 2)
	{
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
		exit(1);
	}
#endif
}

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
	int fd;
	int err = 0;

	if((fd = socket(addr->sa_family, type, 0)) < 0)
		return -1;
	if((bind(fd, addr, alen)) < 0)
	{
		err = errno;
		goto errout;
	}
	if(type == SOCK_STREAM || type == SOCK_SEQPACKET)
	{
		if(listen(fd, qlen) < 0)
		{
			err = errno;
			goto errout;
		}
	}
	return fd;
errout:
	close(fd);
	errno = err;
	return -1;
}

void server(int sockfd)
{
	int n;
	socklen_t alen;
	FILE *fp;
	char buf[BUFLEN];
	char abuf[MAXADDRLEN];

	for(;;)
	{
		alen = MAXADDRLEN;
		if((n = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *)abuf, &alen)) < 0)
		{
			syslog(LOG_ERR, "ruptimed: recvfrom error: %s", strerror(errno));
			exit(1);
		}
		if((fp = popen("/usr/bin/uptime", "r") == NULL))
		{
			sprintf(buf, "error: %s\n", strerror(errno));
			sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)abuf, alen);
		}
		else
		{
			if(fgets(buf, BUFLEN, fp) != NULL)
				sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)abuf, alen);
			pclose(fp);
		}
	}
}

int main(int argc, char* argv[])
{
	struct addrinfo *ailist, *aip;
	struct addrinfo hint;
	int 			sockfd, err, n;
	char 			*host;

	if(argc != 1)
		err_quit("usage: ruptimed");
#ifdef _SC_HOST_NAME_MAX
	n = sysconf(_SC_HOST_NAME_MAX);
	if(n < 0) 	/* best guess */
#endif
		n = HOST_NAME_MAX;
	host = malloc(n);
	if(host == NULL)
		err_sys("malloc error");
	if(gethostname(host, n) < 0)
		err_sys("gethostname error");
	printf("%s\n", host);
	daemonize("ruptimed");
	hint.ai_flags = AI_CANONNAME;  
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;
	if((err = getaddrinfo(host, "ruptime", &hint, &ailist)) != 0) //change
	{
		syslog(LOG_ERR, "ruptimed: getaddrinfo error: %s", gai_strerror(err));
		exit(1);
	}
	for(aip = ailist; aip != NULL; aip = aip->ai_next)
	{
		if((sockfd = initserver(SOCK_DGRAM, aip->ai_addr, aip->ai_addrlen, 0)) >= 0)
		{
			server(sockfd);
			exit(0);
		}
	}
	exit(1);
}

