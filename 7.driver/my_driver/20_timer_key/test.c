#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "head.h"

#include <signal.h>

int fd;
int key_off = 0;

void signal_handler(int signo)
{
	int key_var;

	printf("signo = %d\n",signo);

	read(fd,&key_var,sizeof(int));
	
	printf("*********************\n");
	printf("key_var = %d\n",key_var);

	if(key_var == 7)
		key_off = 1;

	key_var %= 4;
	ioctl(fd,LED_ON,&key_var);




}

int main(int argc, const char *argv[])
{
	int flags = 0;

	fd = open(argv[1],O_RDWR);
	if(0 > fd){
		perror("open");
		return -1;
	}
	
	signal(SIGIO,signal_handler);

	fcntl(fd,F_SETOWN,getpid());
	flags = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,flags | FASYNC);

	while(1){
		if(key_off){
			printf("key_off\n");
			break;
		}
		sleep(1);
	}

	close(fd);

	return 0;
}


