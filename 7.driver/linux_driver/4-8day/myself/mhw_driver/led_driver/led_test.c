#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "led_driver.h"

int main(int argc,const char *argv[])
{
	int fd;
	int i = 0;
	fd = open("/dev/led",O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

#if 0
	while(1)
	{
		ioctl(fd,led_on,i);
		usleep(100000);
		ioctl(fd,led_off,i);
		usleep(100000);
		i ++;
		if(i == 4)
			i = 0;
	}
#endif

	struct led_action action;

	while(1)
	{
		action.act = led_on;
		action.nr = i;
		write(fd,&action,sizeof(struct led_action));
		usleep(100000);
		action.act = led_off;
		write(fd,&action,sizeof(struct led_action));
		usleep(100000);
		i++;
		if(i == 4)
			i = 0;
	}

	return 0;
}
