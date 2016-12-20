#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>

int fd;
int key;

void sig_handler(int signo)  //信号处理函数
{
	read(fd, &key, sizeof(4));    //read为非阻塞读，cpu有驱动发来的信号时去读，没有信号时继续做其他的事情
	printf("key = %d\n", key);
}

int main(int argc, const char *argv[])
{
	int oflags;   //异步通知使用的参数

	fd = open("/dev/key", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	fcntl(fd, F_SETOWN, getpid());        //读
	oflags = fcntl(fd, F_GETFL);          //改
	fcntl(fd, F_SETFL, oflags | FASYNC);  //写


	signal(SIGIO, sig_handler);           //捕捉从驱动发来的信号并转入驱动处理函数

	while(1)
	{
		sleep(100000);
	}

	return 0;
}
