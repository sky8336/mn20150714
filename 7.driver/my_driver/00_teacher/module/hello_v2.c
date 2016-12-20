#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>


MODULE_LICENSE("GPL");


 int var  ;
 char *p;
 char *test;
 int array[3];
 int num;

extern  int  global_data;

module_param(var,int,S_IRUSR);
module_param(p,charp,S_IRUSR);
module_param_named(test,p,charp,0400);
module_param_array(array,int,NULL,0400);
static int  hello_init(void)
{

	int i;
	printk("var  = %d\n",var);
	printk("p  = %s\n",p);
	printk("global_data = %d\n",global_data);
	for(i = 0;i < num;i++)
	{
	
		printk("array = %d\n",array[i]);
	}
	printk("init_module\n");

	
	return  0;
}
static void   hello_exit(void)
{

	printk("cleanup_module\n");
}
module_init(hello_init);
module_exit(hello_exit);
