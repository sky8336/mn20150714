#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "head.h"


int fd;

int main(int argc, const char *argv[])
{

	int i = 5;	
	fd = open("/dev/xhello-0",O_RDWR);	
	if(0 > fd){
		perror("open ");	
		return -1;
	}

	
	
	while(i --){
		ioctl(fd,LED_ON);
		usleep(500000);
		ioctl(fd,LED_OFF);
		usleep(500000);
	}
	
		ioctl(fd,*argv[1]);
		sleep(1);
	

	close(fd);

	return 0;

}



