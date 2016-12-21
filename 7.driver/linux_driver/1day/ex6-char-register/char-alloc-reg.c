#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

int hello_major;
int hello_minor = 0;
int number_of_devices = 1;

static int __init hello_2_init(void)
{
	int ret;
	dev_t devno;

	if((ret = alloc_chrdev_region(&devno,hello_minor,number_of_devices,"mahongwei")) < 0)
	{
		printk("fail from alloc_chrdev_region\n");
		return ret;
	}
	hello_major = MAJOR(devno);

	printk(KERN_INFO "you are apply a device already\n");
	return 0;
}

static void __exit hello_2_exit(void)
{
	dev_t devno = MKDEV(hello_major,hello_minor);
	unregister_chrdev_region(devno,number_of_devices);
	printk(KERN_INFO "zoule");
}

module_init(hello_2_init);
module_exit(hello_2_exit);

