#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc,const char *argv[])
{
	int fd;
	int buf;

	fd = open("/dev/i2c",O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	while(1)
	{
		read(fd,&buf,sizeof(buf));
		printf("buf is: %#x\n",buf);

		sleep(1);
	}

	close(fd);
	return 0;
}
