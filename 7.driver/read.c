#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>


MODULE_LICENSE("GPL");


static int major = 250;
static int minor = 0;
char data[20]="hello world";

static struct  cdev  cdev;


static int hello_open (struct inode *inode, struct file *file)
{

	printk("hello_open  \n");
	return  0;

}
	
static int hello_release (struct inode *inode, struct file *file)
{

	printk("hello_release \n");

	return  0;
}


static ssize_t hello_read(struct file *file, char __user *buff, size_t size, loff_t *loff)
{

	int ret;

	if(size > 128)
		size = 128;
	if(size < 0)
		return EINVAL;

	ret = copy_to_user(buff,data,size);
	if(ret != 0)
	{
		return ENOMEM;

	}
	printk("hello_read  \n");
	return  size;
}

static struct file_operations  hello_ops = {

	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.release = hello_release,



};
static  int hello_init(void)
{

	int ret;

	dev_t  devno  = MKDEV(major,minor);

	ret = register_chrdev_region(devno,1,"hello");
	if(0 != ret)
	{
		printk("register_chrdev_region \n");
		return -1;
	}
	cdev_init(&cdev,&hello_ops);
	
	ret = cdev_add(&cdev,devno,1);
	if(0 != ret)
	{
	

		unregister_chrdev_region(devno,1);
		printk("cdev_add \n");
		return -1;
	}

	printk("hello_init \n");
	return  0;
}

static void hello_exit(void)
{

	dev_t  devno  = MKDEV(major,minor);

	cdev_del(&cdev);
	unregister_chrdev_region(devno,1);
	printk("hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);




