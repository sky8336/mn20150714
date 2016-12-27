#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
#include <signal.h>

#define FASYNC_USE

int fd;

#ifdef FASYNC_USE
void signal_handler(int signo)
{
	char buff[1024];
	read(fd, buff, sizeof(buff));

	printf("signo = %d\n", signo);
	printf("buff = %s\n", buff);
}
#endif

int main(int argc, const char *argv[])
{
	int flags  = 0;

	fd  = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("fail open");
		return -1;
	}

#ifdef FASYNC_USE
	signal(SIGIO, signal_handler);

	fcntl(fd, F_SETOWN, getpid());

	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags|FASYNC);
#endif

	while (1)
		sleep(1000);

	return 0;
}
