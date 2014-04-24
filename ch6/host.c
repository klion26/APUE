#include <unistd.h>
#include <stdio.h>
#include <sys/utsname.h>

int main(void)
{
	struct utsname* name;
	uname(name); 	//fill name
	char host[256];
/*	printf("sysname:%s\n"
			"nodename:%s\n"
			"release:%s\n"
			"version:%s\n"
			"machine:%s\n", name->sysname, name->nodename, name->release, name->version, name->machine);*/
	printf("%s:%s\n",name->version , name->machine);
	printf("%s\n",name->machine);
	gethostname(host, 256);
	printf("%s\n", host);
	return 0;
}

