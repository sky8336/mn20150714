#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
MODULE_LICENSE("GPL");

extern int global_data;
extern int print_k(void);

int init_module(void)
{
	printk("init_module\n");
	printk("global_data = %d\n",global_data);
	print_k();
	return 0;
}

void cleanup_module(void)
{
	printk("cleanup_module\n");	
	
}

//module_init(hello_init);
//module_exit(hello_exit);
