#include <stdio.h>

extern char** environ;

int main()
{
	while(*environ != NULL)
		printf("%s\n", *environ++);
	return 0;
}
