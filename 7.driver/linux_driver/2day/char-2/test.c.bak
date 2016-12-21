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
	int a = 10;

	ioctl(fd, HELLO_1);
	ioctl(fd, HELLO_2, 10);

	write(fd, buf, strlen(buf));
	memset(buf, 0, sizeof(buf));
	read(fd, buf, sizeof(buf));
	printf("buf = %s\n", buf);

	close(fd);

	return 0;
}
