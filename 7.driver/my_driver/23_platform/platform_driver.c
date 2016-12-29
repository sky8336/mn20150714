#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
MODULE_LICENSE("GPL");

static int hello_probe(struct platform_device *dev)
{
	printk("mach ok\n");
	return 0;
}

static int hello_remove(struct platform_device *dev)
{
	printk("hello_remove\n");
	return 0;
}

static struct platform_driver hello_driver = {
	.driver.name ="platform_test",
	.probe = hello_probe,
	.remove = hello_remove,
};

static int hello_init(void)
{
	printk("hello_init\n");
	return platform_driver_register(&hello_driver);
}

static void hello_exit(void)
{
	printk("hello_exit\n");
	platform_driver_unregister(&hello_driver);
}

module_init(hello_init);
module_exit(hello_exit);
