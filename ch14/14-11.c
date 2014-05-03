#include "apue.h"

ssize_t readn(int filedes, void* ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	nleft = n;
	while(nleft > 0)
	{
		if((nread = read(filedes, ptr, nleft)) < 0)
		{
			if(nleft == n)
				return -1;
			else
				break;
		}
		else
		{
			if(nread == 0) 	/* EOF */
				break;
			else
			{
				nleft -= nread;
				ptr += nread;
			}
		}
	}
	return n-nleft;
}

ssize_t writen(int fd, const void* ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while(nleft > 0)
	{
		if((nwritten = write(fd, ptr, nleft)) < 0)
		{
			if(nleft == n)
				return -1; 	/* error, return -1 */
			else
				break;
		}
		else
		{
			if(nwritten == 0)
				break;
			else
			{
				nleft -= nwritten;
				ptr += nwritten;
			}
		}
	}
	return n-nleft; 	
}
