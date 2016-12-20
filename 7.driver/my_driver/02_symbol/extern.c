#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
MODULE_LICENSE("GPL");

extern int global_data;
extern int print_fun(void);
extern int *funp(void);
int hello_init(void)
{
	int *ret_fun_p;
	printk("init_module\n");
	printk("global_data = %d\n",global_data);
	print_fun();
	ret_fun_p = funp();
	printk("*fun_p = %d\n",*ret_fun_p);
	return 0;
}

void hello_exit(void)
{
	printk("cleanup_module\n");	
	
}

module_init(hello_init);
module_exit(hello_exit);
