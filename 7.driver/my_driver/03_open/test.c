#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;

	fd = open("/home/linux/my_driver/mod3/hello",O_RDWR,0664);
	if(0 > fd){
		printf("open: fd = %d\n", fd);
		return -1;
	}

	sleep(3);
	close(fd);

	return 0;

}
