#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "head.h"
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>



int main(int argc, const char *argv[])
{
	
	int fd;
	fd_set  readfd;
	fd_set  writefd;
	char buff[30] = "hello world";

	fd  = open(argv[1],O_RDWR);
	if(fd  <  0)
	{
		perror("fail open");
		return -1;
	}


	while(1)
	{
	
		FD_ZERO(&readfd);
		FD_ZERO(&writefd);

		FD_SET(fd,&readfd);
		FD_SET(fd,&writefd);

		select(fd+1,&readfd,&writefd,NULL,NULL);


		if(FD_ISSET(fd,&readfd))
		{
		
			printf("device can read   \n");
		}
		if(FD_ISSET(fd,&writefd))
		{
		
			printf("device can write  \n");
		}

	}


	close(fd);
	return 0;
}
