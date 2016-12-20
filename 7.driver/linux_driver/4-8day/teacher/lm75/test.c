#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	char buf[2];

	fd = open("/dev/lm75", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	while(1)
	{
		if(read(fd, buf, sizeof(buf)) < 0)
			perror("read");
		
		printf("buf[0] = %d, buf[1] = %d\n", buf[0], buf[1]);
		usleep(100000);
	}

	return 0;
}
