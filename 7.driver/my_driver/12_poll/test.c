#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, const char *argv[])
{
	int fd;
	fd_set readfds,writefds;

//	int nbyte;
//	char buff[20] = "hello world";
	
//	printf("\n************* sudo dmesg -c **************\n");
//	system("sudo dmesg -c");


	fd = open("/dev/xhello-0",O_RDWR);	
	if(0 > fd){
		perror("open ");	
		return -1;
	}

	
	while(1){
       FD_CLR(fd,&readfds);
       FD_CLR(fd,&writefds);

       FD_SET(fd,&readfds);
       FD_SET(fd,&writefds);

	   if(0 > select(fd + 1,&readfds,&writefds,NULL, NULL)){
		   perror("select");
		   return -1;
	   }
		if(FD_ISSET(fd,&writefds)){
			printf("you can write \n");
			sleep(1);
		}
		if(FD_ISSET(fd,&readfds)){
			printf("you can read \n");
			sleep(1);
		}
	
	}
#if 0
	write(fd,buff,20);
    nbyte = read(fd,buff, sizeof(buff));

	printf("\n************* test.c : printf ************\n");
	printf("test : buff[] = %s\n",buff);
	printf("test : nbyte = %d\n",nbyte);
	
	ioctl(fd,LED_ON);
	sleep(1);
	ioctl(fd,LED_OFF);
#endif
//	printf("\n***************** dmesg ******************\n");
//	system("dmesg");
	sleep(10);
//	while(1);
	close(fd);

	return 0;

}



