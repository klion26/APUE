#include <pwd.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

struct passwd* getpwnam(const char* name)
{
	struct passwd* ptr;

	setpwent();
	while((ptr = getpwent()) != NULL)
		if(strcmp(name, ptr->pw_name) == 0)
			break;
	endpwent();
	return ptr;
}

int main(int argc, char* argv[])
{
	struct passwd* ptr = getpwnam(argv[1]);
	printf("%s:%s:%d:%d:%s:%s:%s\n", ptr->pw_name,ptr->pw_passwd,ptr->pw_uid, ptr->pw_gid, ptr->pw_gecos, ptr->pw_dir, ptr->pw_shell);
	return 0;
}
