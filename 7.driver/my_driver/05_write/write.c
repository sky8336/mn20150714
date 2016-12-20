#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define N 128
MODULE_LICENSE("GPL");

char data[N];

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

static ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *loff)
{
	if(size > N){
		size = N;
	}
	if(size < 0){
		return EINVAL;  
	}

	if(copy_to_user(buf,data,size)){
		return ENOMEM;
	}

	printk("hello_read\n");
	return size;
	
}

static ssize_t hello_write (struct file *file, const char __user *buff, size_t size, loff_t *loff)
{
	if(size > N)
		size = N;
	if(size < 0)
		return EINVAL;

	if(0 != copy_from_user(data,buff,20)){
		return ENOMEM; 
	}

	printk("hello_write\n");
	printk("hello_write : data = %s\n",data);
	return size;
}
static struct cdev cdev;
static struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
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

	printk("hello_init\n");
	return 0;
}

static void hello_exit(void)
{

	dev_t devno = MKDEV(major,minor);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,1);

	printk("hello_exit\n");	

}

module_init(hello_init);
module_exit(hello_exit);



