#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
MODULE_LICENSE("GPL");

static int major = 250;
static int minor = 0; 

static int hello_open(struct inode *inode,struct file *fl)
{
	printk("hello_open\n");
	return 0;
}

static int hello_release (struct inode *inode, struct file *file)
{
	printk("hello_release\n");

	return 0;
}

static struct cdev cdev;
static struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.release = hello_release
};


static int hello_init(void)
{
	int ret;

	dev_t devno = MKDEV(major,minor);
	ret = register_chrdev_region(devno,1,"hello");
	if(0 != ret){
		printk("register_chrdev_region : error\n");
		return -1;
	}

	cdev_init(&cdev,&hello_ops);
	ret = cdev_add(&cdev,devno,1);
	if(0 != ret){
		printk("cdev_add\n");
		unregister_chrdev_region(devno,1);
		return -1;
	}

	printk("init_module\n");
	return 0;
}

static void hello_exit(void)
{
	cdev_del(&cdev);

	dev_t devno = MKDEV(major,minor);
	unregister_chrdev_region(devno,1);

	printk("cleanup_module\n");	
}

module_init(hello_init);
module_exit(hello_exit);



