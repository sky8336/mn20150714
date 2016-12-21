#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc,const char *argv[])
{
	int fd;
	int buf;
	float U_mhw;
	fd = open("/dev/adc",O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	while(1)
	{
		read(fd,&buf,sizeof(buf));
		printf("buf is : %d\n",buf);
		printf("U_mhw = %f\n",(3.3*buf) / 4096);

		sleep(1);
	}
	close(fd);
	return 0;
}
