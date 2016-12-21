#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
#include <sys/signal.h>
int fd;

void signal_handler(int  signo)
{

	int  key_var;
	printf("signo  = %d\n",signo);
	read(fd,&key_var,sizeof(key_var));

	printf("*********************************\n");
	printf("key_var  = %d\n",key_var);
}

int main(int argc, const char *argv[])
{

	int  flags  = 0;
	char buff[30] = "hello world";

	fd  = open(argv[1],O_RDWR);
	if(fd  <  0)
	{
		perror("fail open");
		return -1;
	}


	signal(SIGIO,signal_handler);

	fcntl(fd,F_SETOWN,getpid());
	flags = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,flags|FASYNC);


	//write(fd,buff,20);
	//
	//ioctl(fd,LED_ON);
	//ioctl(fd,LED_OFF,&fd);


//	sleep(10);
	while(1)
	{

		sleep(100000);
	}

//	read(fd,buff,1024);

//	printf("buff  = %s\n",buff);

//	read(fd,buff,sizeof(buff));
//	printf("buff  = %s\n",buff);

//	sleep(3);


//	while(1);
	close(fd);
	return 0;
}
