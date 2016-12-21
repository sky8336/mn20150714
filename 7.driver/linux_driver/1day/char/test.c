#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main()
{
	int fd = open("/dev/hello", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	close(fd);

	return 0;
}
