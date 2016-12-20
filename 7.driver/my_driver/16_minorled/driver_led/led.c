#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "head.h"
#include <asm/io.h>
MODULE_LICENSE("GPL");


#define GPG3CON   0xE03001c0
#define GPG3DATA   0xE03001c4
static int major = 250;
static int minor = 0;
char data[128] = "\0";

static struct  cdev  cdev;



volatile   unsigned  int * gpg3con;
volatile  unsigned  int  * gpg3data;
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
		return -EINVAL;


	printk("hello_write  \n");
	ret = copy_from_user(data,buff,size);
	if(0 != ret)
	{
	
		return -ENOMEM;
	}

	printk("data  = %s\n",data);
	return size;
}
	
static long hello_unlocked_ioctl(struct file *file, unsigned int  cmd, unsigned long  arg)
{

	int  var = 0;
	int ret;

	ret = copy_from_user(&var,(void *)arg,sizeof(var));
	if(var  < 0 || var > 4)
		return -EINVAL;


	
	if(0 != ret)
		return  -ENOMEM;



	switch(cmd)
	{
	
	case LED_ON:


		writel(readl(gpg3data)|(1 << var),gpg3data);

		printk("LED_ON \n");
		break;
	case LED_OFF:
		printk("LED_OFF \n");
		writel(readl(gpg3data)& ~(1 << var),gpg3data);
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
static  int hello_init(void)
{

	int ret;

	dev_t  devno  = MKDEV(major,minor);

	ret = register_chrdev_region(devno,1,"hello");
	if(0 != ret)
	{

//		alloc_chrdev_region(&devno,0,1,"duang");
		printk("register_chrdev_region \n");
		return  -1;

		
	}
	cdev_init(&cdev,&hello_ops);
	
	ret = cdev_add(&cdev,devno,1);
	if(0 != ret)
	{
	

		printk("cdev_add \n");
		goto  err1;
	}


	gpg3con = ioremap(GPG3CON,4);

	if(NULL  == gpg3con)
	{
		printk("ioremap  gpg3con  fail  \n");
		goto err2;

	}
	gpg3data = ioremap(GPG3DATA,4);

	if(NULL == gpg3data)
	{
	
		//取消映射
		printk("gpg3data  ioremap  fail\n");
		goto  err3;
	}





	writel((readl(gpg3con) & ~0xffff)|0x1111,gpg3con);
	writel(readl(gpg3data)|0xf,gpg3data);



	printk("hello_init \n");
	return  0;
err3:
	iounmap(gpg3con);//取消映射
err2:
	cdev_del(&cdev);
err1:
		unregister_chrdev_region(devno,1);
		return  ret;
}

static void hello_exit(void)
{

	dev_t  devno  = MKDEV(major,minor);

	iounmap(gpg3data);
	iounmap(gpg3con);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,1);
	printk("hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);




