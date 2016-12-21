#include <stdio.h>
int main(int argc, const char *argv[])
{
	char str[] = "hello";
	char *p = str;
	int n = 10;

	printf("str: %d\t p:%d\t n:%d\n",sizeof(str),sizeof(p),sizeof(n));
	return 0;
}

