#include "apue.h"
#include <fcntl.h>

static volatile sig_atomic_t sigflag; 	/* set nonzero by sig handler*/
static sigset_t newmask, oldmask, zeromask;

static void sig_usr(int signo)
{
	sigflag = 1;
}

void TELL_WAIT(void)
{
	if(signal(SIGUSR1, sig_usr) == SIG_ERR)
		err_sys("signal(SIGUSR1) error");
	if(signal(SIGUSR2, sig_usr) == SIG_ERR)
		err_sys("signal(SIGUSR2) error");
	sigemptyset(&zeromask);
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	sigaddset(&newmask, SIGUSR2);

	/* 
	 * Block SIGUSR1 and SIGUSR2, and save current signal mask.
	 */
	if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error");
}

void TELL_PARENT(pid_t pid)
{
	kill(pid, SIGUSR2);  	/* tell parent we're done */
}

void WAIT_PARENT(void)
{
	while(0 == sigflag)
		sigsuspend(&zeromask); 	/* and wait for parent */
	sigflag = 0;

	/* 
	 * Reset signal mask to original value.
	 */
	if(sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
}

void TELL_CHILD(pid_t pid)
{
	kill(pid, SIGUSR1); 	/* tell child we're done */
}

void WAIT_CHILD(void)
{
	while(0 == sigflag)
		sigsuspend(&zeromask); 	/* and wait for child */
	sigflag = 0;

	/* 
	 * Reset signal mask to original value.
	 */
	if(sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
}

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type = type; 	/* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start = offset; 	/* byte offset, relative to l_whence */
	lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = len; 		/* #byte (0 means to EOF */
	
	return (fcntl(fd, cmd, &lock));
}

static void lockabyte(const char* name, int fd, off_t offset)
{
	if(writew_lock(fd, offset, SEEK_SET, 1) < 0)
		err_sys("%s: writew_lock error", name);
	printf("%s: got the lock, byte %ld\n", name, offset);
}

int main(void)
{
	int fd;
	pid_t pid;

	if((fd = creat("templock", FILE_MODE)) < 0)
		err_sys("creat error");
	if(write(fd, "ab", 2) != 2)
		err_sys("write error");

	TELL_WAIT();
	if((pid = fork()) < 0)
		err_sys("fork error");
	else if(0 == pid)
	{
		lockabyte("child", fd, 0);
		TELL_PARENT(getppid());
		WAIT_PARENT();
		lockabyte("child", fd, 1);
	}
	else
	{
		lockabyte("parent", fd, 1);
		TELL_CHILD(pid);
		WAIT_CHILD();
		lockabyte("parent", fd, 0);
	}
	exit(0);
}
