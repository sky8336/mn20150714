#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#define POLL_USE

int main(int argc, const char *argv[])
{
	int fd;

#ifdef POLL_USE
	fd_set readfds, writefds;
#endif


	fd = open("/dev/xhello-0", O_RDWR);
	if (0 > fd) {
		perror("open /dev/xhello-0");
		return -1;
	}


#ifdef POLL_USE
	while (1) {
		FD_CLR(fd, &readfds);
		FD_CLR(fd, &writefds);

		FD_SET(fd, &readfds);
		FD_SET(fd, &writefds);

		if (0 > select(fd + 1, &readfds, &writefds, NULL, NULL)) {
			perror("select");
			return -1;
		}
		if (FD_ISSET(fd, &writefds)) {
			printf("you can write\n");
			sleep(1);
		}
		if (FD_ISSET(fd, &readfds)) {
			printf("you can read\n");
			sleep(1);
		}
	}
#endif

	close(fd);

	return 0;
}
