#include "apue.h"
int main(void)
{
	printf("read uid = %d, effective uid = %d\n", getuid(), geteuid());
	exit(0);
}
