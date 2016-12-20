#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	int key;

	fd = open("/dev/adc", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	while(1)
	{
		if(read(fd, &key, sizeof(4)) < 0)
			perror("read");
		
		printf("key = %d\n", key);
		usleep(100000);
	}

	return 0;
}
