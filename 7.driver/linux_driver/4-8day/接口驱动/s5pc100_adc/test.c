#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main (void) 
{
	int fd;
	int data;
	fd = open ("/dev/adc",O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(0);
	}
	while(1)
	{
		read (fd, (char *)&data, sizeof(data));
		printf("Voltage = %.2f\n", 3.3/4096*data);
		sleep(1);
	}
	close (fd);
	printf ("/dev/adc closed :)\n");
	return 0;
}

