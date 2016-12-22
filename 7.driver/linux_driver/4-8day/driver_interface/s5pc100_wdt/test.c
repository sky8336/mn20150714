#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>

#define WATCHDOG_MAGIC 'k' 
#define FEED_DOG _IO(WATCHDOG_MAGIC,1)

int main(int argc,char **argv)
{
	int fd;
	int n = 10;
	//打开看门狗
	fd=open("/dev/wdt",O_RDWR);

	if(fd<0)
	{
		printf("cannot open the watchdog device\n");
		return -1;
	}

	//喂狗 ，否则系统重启
	while(n--)
	{
		ioctl(fd,FEED_DOG);
		sleep(1);
	}

	sleep(20);	
	printf("close\n");
	close(fd);
	return 0;
}
