#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	int nbyte;
	char buff[20] = "hello world";

	//fd = open("/home/linux/my_driver/my_node/mod5_hello_write", O_RDWR, 0664);
	fd = open("/dev/hello_write", O_RDWR, 0664);
	if(0 > fd){
		printf("test : open : error\n");
		return -1;
	}

	write(fd,buff,20);
	nbyte = read(fd, buff, sizeof(buff));

	printf("test : buff[] = %s\n", buff);
	printf("test : nbyte = %d\n", nbyte);

	//	while(1);
	close(fd);

	return 0;
}
