#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc,const char *argv[])
{
	int fd;
	int buf;
	fd = open("dev/key",O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	while(1)
	{
		read(fd,&buf,sizeof(buf));
	 	printf("key:%d\n",buf);
	}
	return 0;
}
