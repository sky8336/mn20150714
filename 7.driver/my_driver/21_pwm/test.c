#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "head.h"


int fd;

int main(int argc, const char *argv[])
{

	int i = 0;	
	fd = open("/dev/xhello-0",O_RDWR);	
	if(0 > fd){
		perror("open ");	
		return -1;
	}

	
	while(1){
		ioctl(fd,LED_ON,&i);
		usleep(500000);
		ioctl(fd,LED_OFF,&i);
		usleep(500000);
		i ++;
		if(i > 4){
			i = 0;
		}
	}
	
	

	close(fd);

	return 0;

}



