#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "head.h"
#include <linux/interrupt.h>
#include <linux/sched.h>




MODULE_LICENSE("GPL");



int  key_var;
int  flag  = 0;
static int major = 250;
static int minor = 0;
char data[128] = "\0";

static struct  cdev  cdev;


wait_queue_head_t  hello_readq;
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

	wait_event(hello_readq,flag != 0);

	ret = copy_to_user(buff,&key_var,size);
	if(ret != 0)
	{
		return ENOMEM;

	}

	flag  = 0;
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

	printk("data  = %s\n",data);
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
irqreturn_t hello_handler(int irqno,void  *arg)
{

	switch(irqno)
	{
	
	case IRQ_EINT(1):
		key_var  = 1;
		flag  ++;
		wake_up(&hello_readq);
		break;

	case IRQ_EINT(2):
		key_var  = 2;
		flag  ++;
		wake_up(&hello_readq);
		break;

	case IRQ_EINT(3):
		key_var  = 3;
		flag  ++;
		wake_up(&hello_readq);
		break;
	case IRQ_EINT(4):
		key_var  = 4;
		flag  ++;
		wake_up(&hello_readq);
		break;
	case IRQ_EINT(6):
		key_var  = 6;
		flag  ++;
		wake_up(&hello_readq);
		break;
	case IRQ_EINT(7):
		key_var  = 7;
		flag  ++;
		wake_up(&hello_readq);
		break;
	}
	return   IRQ_HANDLED;
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
	

		//unregister_chrdev_region(devno,1);
		printk("cdev_add \n");
		goto  err1;
	}

	init_waitqueue_head(&hello_readq);

	ret  = request_irq(IRQ_EINT(1),hello_handler,IRQF_DISABLED|IRQF_TRIGGER_FALLING,"KEY1",NULL);
	if(0 != ret)
	{
	
		printk("request_irq  IRQ_EINT  0  \n");
		goto  err2;
	}

	ret  = request_irq(IRQ_EINT(2),hello_handler,IRQF_DISABLED|IRQF_TRIGGER_FALLING,"KEY2",NULL);
	if(0 != ret)
	{
	
		printk("request_irq  IRQ_EINT  0  \n");
		goto  err3;
	}
	ret  = request_irq(IRQ_EINT(3),hello_handler,IRQF_DISABLED|IRQF_TRIGGER_FALLING,"KEY3",NULL);
	if(0 != ret)
	{
	
		printk("request_irq  IRQ_EINT  0  \n");
		goto  err4;
	}
	ret  = request_irq(IRQ_EINT(4),hello_handler,IRQF_DISABLED|IRQF_TRIGGER_FALLING,"KEY4",NULL);
	if(0 != ret)
	{
	
		printk("request_irq  IRQ_EINT  0  \n");
		goto  err5;
	}
	ret  = request_irq(IRQ_EINT(6),hello_handler,IRQF_DISABLED|IRQF_TRIGGER_FALLING,"KEY6",NULL);
	if(0 != ret)
	{
	
		printk("request_irq  IRQ_EINT  0  \n");
		goto  err6;
	}
	ret  = request_irq(IRQ_EINT(7),hello_handler,IRQF_DISABLED|IRQF_TRIGGER_FALLING,"KEY7",NULL);
	if(0 != ret)
	{
	
		printk("request_irq  IRQ_EINT  0  \n");
		goto  err7;
	}
	printk("hello_init \n");
	return  0;
err7:
	free_irq(IRQ_EINT(6),NULL);
err6:
	free_irq(IRQ_EINT(4),NULL);
err5:
	free_irq(IRQ_EINT(3),NULL);
err4:
	free_irq(IRQ_EINT(2),NULL);
err3:
	free_irq(IRQ_EINT(1),NULL);
err2:
	cdev_del(&cdev);

err1:
	unregister_chrdev_region(devno,1);
	return ret;
}

static void hello_exit(void)
{

	dev_t  devno  = MKDEV(major,minor);

	free_irq(IRQ_EINT(7),NULL);
	free_irq(IRQ_EINT(6),NULL);
	free_irq(IRQ_EINT(4),NULL);
	free_irq(IRQ_EINT(3),NULL);
	free_irq(IRQ_EINT(2),NULL);
	free_irq(IRQ_EINT(1),NULL);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,1);
	printk("hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);




