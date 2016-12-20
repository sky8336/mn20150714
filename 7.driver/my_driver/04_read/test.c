#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, const char *argv[])
{
	int fd;
	int nbyte;
	char buf[1024];
	fd = open("/home/linux/my_driver/mod3/hello",O_RDWR,0664);	
	if(0 > fd){
		printf("open : error\n");
		return -1;
	}

    nbyte = read(fd,buf, sizeof(buf));
	printf("buf[] = %s\n",buf);
	printf("nbyte = %d\n",nbyte);
	while(1);
	close(fd);
	return 0;

}



