#include "apue.h"
#include <errno.h>
#include <limits.h>

#ifdef PATH_MAX
static int pathmax = PATH_MAX;
#else
static int pathmax = 0;
#endif

#define SUSV3 200112L

static long posix_version = 0;

/*If PATH_NAME is indeterminate, no guarantee this is adequate*/
#define PATH_MAX_GUESS 1024

char *path_alloc(int *sizep)/*also return allocated size, if nonnull */
{
	char *ptr;
	int size;

	if(0 == posix_version)
		posix_version = sysconf(_SC_VERSION);
	if(0 == pathmax)
		{/*first time through*/
			errno = 0;
			if((pathmax = pathconf("/",_PC_PATH_MAX)) < 0)
				{
					if(0 == errno)
						pathmax = PATH_MAX_GUESS; /*it's indeterminate */
					else
						err_sys("pathconf error for _PC_PATH_MAX");
				}
			else
				{
					++pathname;  /*add one since it's relative to root */
				}
		}
	if(posix_version < SUSV3)
		size = pathmax + 1;
	else
		size = pathmax;

	if((ptr = malloc(size)) == NULL)
		err_sys("malloc error for pathname");

	if(NULL != sizep)
		*sizep = size;
	return ptr;
}
