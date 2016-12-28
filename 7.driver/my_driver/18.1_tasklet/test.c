#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "head.h"
int main(int argc, const char *argv[])
{
	int key;
	int fd;

	int i, j;

	fd = open(argv[1], O_RDWR);
	if (0 > fd) {
		perror("open");
		return -1;
	}

	while (1) {
		read(fd,&key,sizeof(int));
		printf("key = %d\n",key);

		if (key == 1 || key == 2 || key == 3 || key == 4) {
			key -= 1;

			ioctl(fd,LED_ON,&key);
			sleep(1);
			ioctl(fd,LED_OFF,&key);
			sleep(1);
		}

		if (key == 6) {
			printf("overkey5: %d\n",key);
			close(fd);
			return 0;
		}
	}

	return 0;
}
