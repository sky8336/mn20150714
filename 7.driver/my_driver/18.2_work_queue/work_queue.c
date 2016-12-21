#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "head.h"
#include <linux/interrupt.h>
#include<linux/unistd.h>
#include <linux/delay.h>
MODULE_LICENSE("GPL");


static int major = 250;
static int minor = 0;
char data[128] = "\0";

static struct  cdev  cdev;
//static struct  tasklet_struct    my_tasklet;
static  struct work_struct  my_queue;
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

	printk("buff  = %s\n",buff);
	printk("hello_read  \n");
	return  size;
}

	
static ssize_t hello_write(struct file * file, const char __user *buff, size_t  size, loff_t *loff)
{

	int ret;


	if(size  > 128)
		size = 128;
	if(size  <  0)
		return EINVAL;


	printk("hello_write  \n");
	ret = copy_from_user(data,buff,size);
	if(0 != ret)
	{
	
		return ENOMEM;
	}

	printk("write 111111111111111 data  = %s\n",data);

//	tasklet_schedule(&my_tasklet);
	schedule_work(&my_queue);


	return size;
}
	
static long hello_unlocked_ioctl(struct file *file, unsigned int  cmd, unsigned long  arg)
{

	switch(cmd)
	{
	
	case LED_ON:
		printk("LED_ON \n");
		break;
	case LED_OFF:
		printk("LED_OFF \n");
		break;
	}
	printk("hello_unlocked_ioctl \n");
	return  0;
}
static struct file_operations  hello_ops = {

	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.release = hello_release,
	.write = hello_write,
	.unlocked_ioctl = hello_unlocked_ioctl,



};

void  hello_work(unsigned long  data)
{

	
	msleep(5);
	printk("jiffies   = %ld\n",jiffies);

}
static  int hello_init(void)
{

	int ret;

	dev_t  devno  = MKDEV(major,minor);

	ret = register_chrdev_region(devno,1,"hello");
	if(0 != ret)
	{

	//	alloc_chrdev_region(&devno,0,1,"duang");
		printk("register_chrdev_region \n");

		
	}
	cdev_init(&cdev,&hello_ops);
	
	ret = cdev_add(&cdev,devno,1);
	if(0 != ret)
	{
	

		unregister_chrdev_region(devno,1);
		printk("cdev_add \n");
		return -1;
	}

	//tasklet_init(&my_tasklet,hello_tasklet,123);
	INIT_WORK(&my_queue,(void  *)hello_work);
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




