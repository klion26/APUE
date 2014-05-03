#include "apue.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stropts.h>
int isastream(int fd)
{
	return (ioctl(fd, I_CANPUT, 0) != -1);
}

int main(int argc, char* argv[])
{
	int i, fd;

	for(i=1; i<argc; ++i)
	{
		if((fd = open(argv[i], O_RDONLY)) < 0)
		{
			err_ret("%s: can't open", argv[i]);
			continue;
		}

		if(isastream(fd) == 0)
			err_ret("%s: not a stream ", argv[i]);
		else
			err_msg("%s: streams device", argv[i]);
	}
	exit(0);
}
