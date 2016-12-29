#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
MODULE_LICENSE("GPL");


static void	hello_release(struct device *dev)
{
	printk("hello_release\n");
}

static struct platform_device hello_device = {
	.name = "platform_test",
	.id = -1,
	.dev.release = hello_release,
};

static int hello_init(void)
{
	printk("platform_device : hello_init\n");
	return platform_device_register(&hello_device);
}

static void hello_exit(void)
{
	printk("platform_device : hello_exit\n");
	platform_device_unregister(&hello_device);
}

module_init(hello_init);
module_exit(hello_exit);
