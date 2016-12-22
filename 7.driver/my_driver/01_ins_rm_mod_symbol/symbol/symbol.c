#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("GPL");

static int global_data = 123;
EXPORT_SYMBOL(global_data);

int print_fun(void)
{
	printk("extern.c --->fun_called ok !!: defined in symbol.c\n");
	return 0;
}
EXPORT_SYMBOL(print_fun);

int *funp(void)
{
	int *fun_p;

	fun_p = &global_data;
	printk("fun_p called\n");

	return fun_p;
}
EXPORT_SYMBOL(funp);


int hello_init(void)
{
	printk("symbol.c: init_module\n");
	return 0;
}

void hello_exit(void)
{
	printk("symbol.c: cleanup_module\n");

}

module_init(hello_init);
module_exit(hello_exit);
