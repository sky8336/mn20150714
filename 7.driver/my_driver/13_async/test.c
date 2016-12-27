#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "head.h"

#define FASYNC_USE

#ifdef FASYNC_USE
#include <signal.h>
#endif

int fd;

#ifdef FASYNC_USE
void signal_handler(int signo)
{
	char buff[64] ;

	read(fd, buff, sizeof(buff));

	printf("SIGIO = %d\n",signo);
	printf("buff = %s\n",buff);
}
#endif

int main(int argc, const char *argv[])
{
	int flags;

	fd = open("/dev/xhello-0",O_RDWR);
	if (0 > fd) {
		perror("open ");
		return -1;
	}

#ifdef FASYNC_USE
	signal(SIGIO, signal_handler);//内核空间资源可获得时，释放信号，接受信号执行信号处理函数
	fcntl(fd, F_SETOWN, getpid());/* 不能有别的进程打断;设置文件拥有者为本进程-->，
								 * 内核设置file->f_owner为对应的进程号*/
	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | FASYNC);//O_ASYNC，
#endif

	while (1)
		sleep(1000);

	close(fd);

	return 0;
}
