#include "apue.h"
#define BUFFSIZE	4096

int main(void)
{
	int n;
	char buf[BUFFSIZE];
	//STDIN_FILENO和STDOUT_FILENO在头文件unistd.h中，apue.h已经包含了
	while((n = read(STDIN_FILENO,buf,BUFFSIZE)) > 0)
		if(write(STDOUT_FILENO,buf,n) != n)
			err_sys("write_error");

	if(n<0)
		err_sys("read error");
	return 0;
}
