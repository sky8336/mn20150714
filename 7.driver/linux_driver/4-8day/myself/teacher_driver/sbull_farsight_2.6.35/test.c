#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

main()
{
	int fd,n;
	char str[]="hello";
	//fd=open("/dev/sbulla",O_RDWR);
	if((fd=open("/dev/sbulla",O_RDWR))<0)
	{
		perror("open erro\n");
	}
	n=strlen(str);
	printf("fd=%d n=%d\n",fd,n);
	lseek(fd,5120,SEEK_SET);
	write(fd,str,n);
	close(fd);
	
}
