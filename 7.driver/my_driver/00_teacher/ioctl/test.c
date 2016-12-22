#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
int main(int argc, const char *argv[])
{
	
	int fd;
	char buff[30] = "hello world";

	fd  = open("/dev/hello",O_RDWR);
	if(fd  <  0)
	{
		perror("failt open");
		return -1;
	}

	//write(fd,buff,20);
	//
	ioctl(fd,LED_ON);
	sleep(3);
	ioctl(fd,LED_OFF);



//	read(fd,buff,1024);

//	printf("buff  = %s\n",buff);

//	read(fd,buff,sizeof(buff));
//	printf("buff  = %s\n",buff);

//	sleep(3);


//	while(1);
	close(fd);
	return 0;
}
