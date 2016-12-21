#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("GPL");

static int global_data = 123;
EXPORT_SYMBOL(global_data);

static int print_k(void)
{
	printk("hello_v1.c --->fun_called ok !!: defined in hello.c\n");
	return 0;
}
EXPORT_SYMBOL(print_k);

/*static*/ int init_module(void)
{
	printk("%s:%s:\n", __FILE__, __func__);
	return 0;
}

/*static*/ void cleanup_module(void)
{
	printk("%s:%s:\n", __FILE__,__func__);

}

//module_init(hello_init);
//module_exit(hello_exit);
