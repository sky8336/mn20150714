#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "head.h"

#include <signal.h>

int fd;
void signal_handler(int signo)
{
	char buff[64] ;

	printf("SIGIO = %d\n",signo);

	read(fd,buff,sizeof(buff));
	printf("buff = %s\n",buff);

}

int main(int argc, const char *argv[])
{
	int flags;
	
	fd = open("/dev/xhello-0",O_RDWR);	
	if(0 > fd){
		perror("open ");	
		return -1;
	}

	signal(SIGIO,signal_handler);//内核空间资源可获得时，释放信号，接受信号执行信号处理函数
	fcntl(fd,F_SETOWN,getpid());/*不能有别的进程打断;设置文件拥有者为本进程-->，
								  内核设置file->f_owner为对应的进程号*/
	flags = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,flags | FASYNC);//O_ASYNC，
	
	
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
	sleep(1000);
//	while(1);
	close(fd);

	return 0;

}



