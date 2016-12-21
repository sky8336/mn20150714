#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>//和设备相关
#include <linux/cdev.h>//和设备相关

MODULE_LICENSE("GPL");

static int hello_major = 251;
static int hello_minor = 0;
static int number_of_device = 1;
static struct cdev cdev;

static struct file_operations hello_fops = {
	.owner = THIS_MODULE,
};

static int __init hello_2_init(void)
{
	int ret;
#if 0
	dev_t devno = MKDEV(hello_major, hello_minor);

	ret = register_chrdev_region(devno, number_of_device, "hello");
	if(ret < 0)
	{
		printk("failed from register_chrdev_region\n");
		return ret;
	}
#endif
#if 1
	dev_t devno;
	ret = alloc_chrdev_region(&devno, hello_minor, number_of_device, "hello");	
	if(ret < 0)
	{
		printk("failed form alloc_chrdev_region\n");
		return ret;
	}

	hello_major = MAJOR(devno);
#endif

	cdev_init(&cdev, &hello_fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_device);
	if(ret < 0)
	{
		printk("failed form cdev_add\n");
		goto err1;
	}
	

	printk(KERN_INFO "hello world\n");
	return 0;
err1:
	unregister_chrdev_region(devno, number_of_device);
	return ret;
}

static void __exit hello_2_exit(void)
{
	dev_t devno = MKDEV(hello_major, hello_minor);

	cdev_del(&cdev);
	unregister_chrdev_region(devno, number_of_device);

	printk("goodbye world\n");
}

module_init(hello_2_init);
module_exit(hello_2_exit);
