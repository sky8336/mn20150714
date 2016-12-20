#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "head.h"
#include <string.h>


int fd;

int main(int argc, const char *argv[])
{

	int i = 0;
	int j = 0;
	char *mip;
	char *p;
	int num;
	fd = open(argv[1],O_RDWR);	
	if(0 > fd){
		perror("open ");	
		return -1;
	}
	/*
	 *第一个参数指向欲分割的字符串，第二个参数为分割字符串，strtok()
	 *发现分割字符串中任何字符时，会将该字符改为\0字符。
	 *从第二次调用开始，第一个参数为NULL，每调用成功一次，返回下一个分割后
	 *的字符串指针。
	 *
	 *
	 *
	 */
	 
#if 1

	mip = strtok(argv[1],"-"); 
	printf("mip = %s\n",mip);

	while((mip = strtok(NULL,"-"))){
		p = mip; //指向最后一个非‘\0’字符（数字字符）的指针，
		printf("mi = %s\n",mip);		
	}
	j = atoi(p);
	printf("j = %d\n",j); //将一个数字字符串转换成对应的十进制输出
	ioctl(fd,LED_ON,&j);
#endif
	
	
#if 0	
	while(1){
		ioctl(fd,LED_ON,&i);
		usleep(500000);
		ioctl(fd,LED_OFF,&i);
		usleep(500000);
		i ++;
		if(i > 3){
			i = 0;
		}
	}
#endif
	while(1);
	

	close(fd);

	return 0;

}



