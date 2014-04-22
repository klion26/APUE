#include "apue.h"

#define BUFFSIZE 4096

int main(void)
{
	int n;
	char buf[BUFFSIZE]; //buff array
	//STDIN_FILENO & STDOUT_FILENO are defined in unistd.h, which included by apue.h
	while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0)
	{//read from STDIN_FILENO with maximum size(BUFFSIZE)
		if(write(STDOUT_FILENO, buf, n) != n)//write buf[] to STDOUT_FILENO
			err_sys("write error");
	}

	if(n < 0)
		err_sys("read error");

	exit(0);
}
