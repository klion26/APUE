#include "apue.h"

int main(int argc, char* argv[])
{
	int i;
	/* struct stat 结构保存文件信息 */
	
	struct stat buf;
	char* ptr;

	for(i=1; i<argc; ++i)
	{
		printf("%s: ", argv[i]);
		/* lstat(const char *restrict pathname, strcut stat *restrict buf);
		 * 会把pathname对应的文件的信息写入到buf中*/
		if(lstat(argv[i], &buf) < 0)
		{
			err_ret("lstat error");
			continue;
		}
		if(S_ISREG(buf.st_mode))
			ptr = "regular";
		else if(S_ISDIR(buf.st_mode))
			ptr = "directory";
		else if(S_ISCHR(buf.st_mode))
			ptr = "character special";
		else if(S_ISBLK(buf.st_mode))
			ptr = "block special";
		else if(S_ISFIFO(buf.st_mode))
			ptr = "fifo";
		else if(S_ISLNK(buf.st_mode))
			ptr = "symbolic link";
		else if(S_ISSOCK(buf.st_mode))
			ptr = "socket";
		else
			ptr = "** unkonw mode. **";
		printf("%s\n", ptr);
	}
	exit(0);
}
