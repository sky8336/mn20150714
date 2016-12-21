#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/*未实现异步通知时的应用程序*/
int main(int argc, const char *argv[])
{
	int fd;
	int key;

	fd = open("/dev/key", O_RDWR);
	if(fd < 0)
	{
		perror("open");
		exit(1);
	}

	while(1)
	{
		if(read(fd, &key, sizeof(4)) < 0)   //read为阻塞读，读到数据时打印，否则等待
			perror("read");
		
		printf("key = %d\n", key);
	}

	return 0;
}
