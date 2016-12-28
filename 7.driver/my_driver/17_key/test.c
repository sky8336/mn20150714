#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
#include <stdio.h>

int main(int argc, const char *argv[])
{
	int fd;
	int key = 0;
	char buff[30] = "hello world";

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("fail open");
		return -1;
	}

	while (1) {
		read(fd, &key, sizeof(int));
		printf("key = %d\n",key);
	}

	close(fd);
	return 0;
}
