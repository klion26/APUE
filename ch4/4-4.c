#include "apue.h"

int main()
{
	struct stat statbuff;

	/* turn on set-group-ID and trun off group execute */
	if(stat("foo", &statbuff) < 0)
		err_sys("stat error for foo");
	if(chmod("foo", (statbuff.st_mode & ~S_IXGRP) | S_ISGID) < 0)
		err_sys("chmod error for foo");

	/* set absolute mode to "rw-r--r--" */
	if(chmod("bar", S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0)
		err_sys("chmod error for bar");
	exit(0);
}
