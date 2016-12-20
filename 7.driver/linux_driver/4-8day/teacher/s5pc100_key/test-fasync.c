#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>

int fd;
int key;

void sig_handler(int signo)
{
	read(fd, &key, sizeof(4));
	printf("key = %d\n", key);
}

int main(int argc, const char *argv[])
{
	int oflags;

	fd = open("/dev/key", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	fcntl(fd, F_SETOWN, getpid());
	oflags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oflags | FASYNC);


	signal(SIGIO, sig_handler);

	while(1)
	{
		sleep(100000);
	}

	return 0;
}
