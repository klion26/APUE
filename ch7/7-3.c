#include "apue.h"

int main(int argc, char* argv[])
{
	int i;
	/* ISO C && POSIX.1 argv[argc] == NULL */
	for(i=0; argv[i] != NULL; ++i)   /* echo all command-line args */
		printf("argv[%d]: %s\n", i, argv[i]);
	return 0;
}
