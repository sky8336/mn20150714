#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>


MODULE_LICENSE("GPL");

int major =255;
int minor = 0;
struct cdev  cdev;
static int hello_open (struct inode *inode, struct file *file)
{

	printk("hello_open \n");
	return 0;
}

struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open  = hello_open,


};
static int  hello_init(void)
{

	int ret;

		dev_t devno = MKDEV(major,minor);
	ret = register_chrdev_region(devno,1,"hello");
	
	if(0 != ret)
	{
	
		printk("register_chrdev_region fail  \n");
		return  0;
	}


	cdev_init(&cdev,&hello_ops);

	ret  = cdev_add(&cdev,devno,1);
	if(0 != ret)
	{
		printk("cdev_add\n");
		unregister_chrdev_region(devno,1);
		return 0;
	}
	
	printk("init_module\n");

	
	return  0;
}
static void   hello_exit(void)
{

	dev_t devno = MKDEV(major,minor);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,1);
	printk("cleanup_module\n");
}
module_init(hello_init);
module_exit(hello_exit);
