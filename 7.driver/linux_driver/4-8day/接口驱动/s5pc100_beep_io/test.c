#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "s5pc100_beep_io.h"

int main()
{
	int i = 0;
	int n = 2;
	int dev_fd;
	dev_fd = open("/dev/beep",O_RDWR | O_NONBLOCK);
	if ( dev_fd == -1 ) {
		perror("open");
		exit(1);
	}
	while(1)
	{
		ioctl(dev_fd,BEEP_ON,0);
		sleep(1);
		ioctl(dev_fd,BEEP_OFF,0);
		sleep(1);
	}
	return 0;
}
