#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>

int main(int argc, const char *argv[])
{
	int fd;
	int nbyte;
	char buff[20] = "hello world";
#if 0
	if(0 != chmod("/dev/hello_class",0666)){
		perror();
	}
#endif
	fd = open("/dev/hello_class",O_RDWR,0664);	
	if(0 > fd){
		printf("test : open : error\n");
		return -1;
	}
	
	write(fd,buff,20);
    nbyte = read(fd,buff, sizeof(buff));

	printf("test : buff[] = %s\n",buff);
	printf("test : nbyte = %d\n",nbyte);
	
	ioctl(fd,LED_ON);
	sleep(1);
	ioctl(fd,LED_OFF);

//	while(1);
	close(fd);

	return 0;

}



