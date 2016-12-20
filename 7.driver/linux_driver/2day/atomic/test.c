#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "hello-char.h"

int main()
{
	char buf[1024] = "hello world";
	int fd = open("/dev/hello", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	sleep(10);

	close(fd);

	return 0;
}
