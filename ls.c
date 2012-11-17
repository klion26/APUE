#include "apue.h"
#include <dirent.h>
#include <stdio.h>

int main(int argc,char **argv)
{
	DIR *dp;
	struct dirent	*dirp;
	if(2 != argc)
		{
			err_quit("usage: ls directory_name");
		}
	if((dp = opendir(argv[1])) == NULL)
		err_sys("can't open %s",argv[1]);
	while((dirp = readdir(dp)) != NULL)
		printf("%s\n",dirp->d_name);
	closedir(dp);
	return 0;
}
