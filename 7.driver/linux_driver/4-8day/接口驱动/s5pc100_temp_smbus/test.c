#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main (void) 
{
	int fd;
	int data;
	fd = open ("/dev/temp",O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(0);
	}
	while(1)
	{
		read (fd, (char *)&data, sizeof(data));
		sleep(1);
	}
	close (fd);
	printf ("/dev/temp closed :)\n");
	return 0;
}

