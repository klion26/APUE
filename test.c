#include "apue.h"
#include <fcntl.h>
int main(void)
{
	int oldfd;
	oldfd = open("app_log",(O_RDWR | O_CREAT),0644);
	dup2(oldfd,1);
	close(oldfd);
	write(1,"JFJF\0",10);
	oldfd = dup(10);
	printf("%d\n",oldfd);
	return 0;
}
