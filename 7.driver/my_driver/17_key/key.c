#include <linux/kernel.h>
#include <linux/module.h>//MODULE_LICENSE()
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/sched.h>
#include <linux/irqreturn.h>
#include <asm/io.h>
//#include <mach/io.h>
#include "head.h"
#define GPG3CON 0xE03001C0
#define GPG3DAT 0xE03001C4
MODULE_LICENSE("GPL");

static int major = 250;
static int minor = 0;

static struct cdev cdev;
char buff[20] = "\0";
static int key_var;
static int flag_var;

static wait_queue_head_t readq;

unsigned int *gpg3con;
unsigned int *gpg3dat;


irqreturn_t hello_handler(int irqno,void *arg)
{
	switch(irqno){
	case IRQ_EINT(1):
		key_var = 1;
		flag_var ++;
		wake_up(&readq);
		break;
		
	case IRQ_EINT(2):
		key_var = 2;
		flag_var ++;
		wake_up(&readq);
		break;
	case IRQ_EINT(3):
		key_var = 3;
		flag_var ++;
		wake_up(&readq);
		break;
	case IRQ_EINT(4):
		key_var = 4;
		flag_var ++;
		wake_up(&readq);
		break;
	case IRQ_EINT(6):
		key_var = 6;
		flag_var ++;
		wake_up(&readq);
		break;
	case IRQ_EINT(7):
		key_var = 1;
		flag_var ++;
		wake_up(&readq);
		break;
	}


	return IRQ_HANDLED;
}

static int hello_open (struct inode *inode, struct file *file)
{
	printk("hello_open\n");
	return 0;
}

static ssize_t hello_read (struct file *file, char __user *ubuff, size_t size, loff_t *loff)
{
	int ret;	
	if(size < 0 || size > 4)	
		return -EINVAL;

	wait_event(readq,flag_var != 0);
	ret = copy_to_user(ubuff,&key_var,sizeof(int));
	flag_var --;

	printk("hello_read\n");
	return size;
}


static ssize_t hello_write (struct file *file, const char __user *ubuff, size_t size, loff_t *loff)
{
	int ret;	
	if(size > 20)
		size = 20;
	if(size < 0 )
		return -EINVAL;
	ret = copy_from_user(buff,ubuff,size);
	printk("hello_write\n");
	return size;
}


static int hello_release (struct inode *inode, struct file *file)
{
	printk("hello_release\n");

	return 0;

}


static long hello_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	int var;
	int ret;
	ret = copy_from_user(&var,(void *)arg,sizeof(arg));

	switch(cmd){
	case LED_ON:
		writel((readl(gpg3dat) & (~0xf)) | (0x1 << var),gpg3dat);
		break;
	case LED_OFF:
		writel(readl(gpg3dat) & (~ 0x1 << var),gpg3dat);
		break;
	
	}

	printk("hello_unlocked_ioctl\n");
	return 0;
}

static struct file_operations hello_ops = {
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
	.unlocked_ioctl = hello_unlocked_ioctl,

};
static int hello_init(void)
{
	int ret;

	dev_t devno = MKDEV(major,minor);

	ret = register_chrdev_region(devno,1,"hello_key");
	if(0 != ret){
		printk("register_chrdev_region fail\n");
		return -EINVAL;
	}
	
	cdev_init(&cdev,&hello_ops);

	ret = cdev_add(&cdev,devno,1);
	if(0 != ret){
		printk("cdev_add fail\n");
		goto err1;
	}

	init_waitqueue_head(&readq); //zhuyi

	ret = request_irq(IRQ_EINT(1),hello_handler,IRQF_TRIGGER_FALLING | IRQF_DISABLED,"key1",NULL);
	if(0 != ret){
		printk("request_irq fail 1\n");
		goto err2;
	}
	
	ret = request_irq(IRQ_EINT(2),hello_handler,IRQF_TRIGGER_FALLING | IRQF_DISABLED,"key2",NULL);
	if(0 != ret){
		printk("request_irq fail 2\n");
		goto err3;
	}
	ret = request_irq(IRQ_EINT(3),hello_handler,IRQF_TRIGGER_FALLING | IRQF_DISABLED,"key3",NULL);
	if(0 != ret){
		printk("request_irq fail 3\n");
		goto err4;
	}
	ret = request_irq(IRQ_EINT(4),hello_handler,IRQF_TRIGGER_FALLING | IRQF_DISABLED,"key4",NULL);
	if(0 != ret){
		printk("request_irq fail 4\n");
		goto err5;
	}
	ret = request_irq(IRQ_EINT(6),hello_handler,IRQF_TRIGGER_FALLING | IRQF_DISABLED,"key6",NULL);
	if(0 != ret){
		printk("request_irq fail 6 \n");
		goto err6;
	}
	ret = request_irq(IRQ_EINT(7),hello_handler,IRQF_TRIGGER_FALLING | IRQF_DISABLED,"key7",NULL);
	if(0 != ret){
		printk("request_irq fail 7\n");
		goto err7;
	}


	gpg3con = ioremap(GPG3CON,4);
	gpg3dat = ioremap(GPG3DAT,4);
	
	writel((readl(gpg3con) & (~0xffff)) | (0x1111),gpg3con);
	writel(readl(gpg3dat) | 0xf,gpg3dat);

	printk("hello_init\n");

	return 0;


err7:
	free_irq(IRQ_EINT(6),"key6");
err6:
	free_irq(IRQ_EINT(4),"key5");
err5:
	free_irq(IRQ_EINT(3),"key4");
err4:
	free_irq(IRQ_EINT(2),"key3");
err3:
	free_irq(IRQ_EINT(1),"key1");
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno,1);
	return ret;
}

static void hello_exit(void)
{
	dev_t devno = MKDEV(major,minor);

	iounmap(gpg3con);
	iounmap(gpg3dat);

	free_irq(IRQ_EINT(7),"key7");
	free_irq(IRQ_EINT(6),"key6");
	free_irq(IRQ_EINT(4),"key4");
	free_irq(IRQ_EINT(3),"key3");
	free_irq(IRQ_EINT(2),"key2");
	free_irq(IRQ_EINT(1),"key1");

	cdev_del(&cdev);
	
	unregister_chrdev_region(devno,1);
	
	printk("hello_exit\n");

}


module_init(hello_init);
module_exit(hello_exit);






































