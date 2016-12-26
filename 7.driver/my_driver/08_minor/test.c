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

#if 1 //test how to use perror
	if(0 != chmod("/dev/hello_class",0666)){
		perror("/dev/hello_class");
	}
#endif
	/*
	 * dmesg --help
	 * sudo dmesg -c : read and clear
	 * sudo dmesg -C : clear
	 * */
	//printf("\n************* sudo dmesg -c **************\n");
	//system("sudo dmesg -c");
	system("sudo dmesg -C");

	fd = open(argv[1], O_RDWR);
	if (0 > fd) {
		printf("test : open : error: fd = %d\n", fd);
		return -1;
	}


	write(fd, buff, 20);
	nbyte = read(fd, buff, sizeof(buff));

	printf("\n************* test.c : printf ************\n");
	printf("test : buff[] = %s\n",buff);
	printf("test : nbyte = %d\n",nbyte);

	ioctl(fd, LED_ON);
	sleep(1);
	ioctl(fd, LED_OFF);

	printf("\n***************** dmesg ******************\n");
	system("dmesg");

	//while(1);
	close(fd);

	return 0;
}
