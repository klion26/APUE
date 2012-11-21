#include "apue.h"
#include <errno.h>
#include <limits.h>
#include <sys/resource.h> /*rlimit需要这个头文件*/

#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

/*
 * If OPEN_MAX is indeterminate, we're not 
 * guaranteed that this is adequate.
 */
#define OPEN_MAX_GUESS 256
long open_max(void)
{
	if(0 == openmax)
		{/* first time through */
			errno = 0;
			if((openmax = sysconf(_SC_OPEN_MAX)) < 0)
				{
					if(0 == errno)
						openmax = OPEN_MAX_GUESS;  /* it's indeterminate */
					else
						err_sys("sysconf error for _SC_OPEN_MAX");
				}
		}
	return openmax;
}
long open_max_improve(void)
{
	struct rlimit r1;
	
	if((openmax = sysconf(_SC_OPEN_MAX)) < 0 || openmax == LONG_MAX)
		{
			if(getrlimit(RLIMIT_NOFILE,&r1) < 0)
				err_sys("can't get file limit");
			if(r1.rlim_max == RLIM_INFINITY)
				openmax = OPEN_MAX_GUESS;
			else
				openmax = r1.rlim_max;
		}
	return openmax;
}

int main(void)
{
	printf("file open max is %d\n",open_max());
}
