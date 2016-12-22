#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "led_driver.h"

#define S5PC100_GPG3CON 0xe03001c0
#define S5PC100_GPG3DAT 0xe03001c4

/*许可证申明*/
MODULE_LICENSE("GPL");

/*主次设备号定义*/
static int led_major = 300;
static int led_minor = 0;
static int number_of_device = 1;

/*字符设备控制块*/
static struct cdev cdev;

/*寄存器映射指针定义*/
static unsigned int *led_gpg3con;
static unsigned int *led_gpg3dat;

/*file_operations中的函数实现*/
static int led_open(struct inode *inode,struct file *file)
{
	printk("call led_open\n");
	return 0;
}
static int led_release(struct inode *inode,struct file *file)
{
	printk("call led_release\n");
	return 0;
}
static long led_ioctl(struct file *file,unsigned int cmd,unsigned long arg)
{
	switch(cmd)
	{
	case led_on:
		writel(readl(led_gpg3dat) | (1 << arg),led_gpg3dat);
		break;
	case led_off:
		writel(readl(led_gpg3dat) & ~(1 << arg),led_gpg3dat);
		break;
	}
	return 0;
}
static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *loff)
{
	struct led_action action;

	if(count != sizeof(struct led_action))
		return -EINVAL;
	if(copy_from_user((void *)&action,buf,sizeof(struct led_action)))
		return -EINVAL;
	if(action.act == led_on)
		writel(readl(led_gpg3dat) | (1 << action.nr),led_gpg3dat);
	else
		writel(readl(led_gpg3dat) & ~(1 << action.nr),led_gpg3dat);
	return sizeof(struct led_action);
}

/*对设备的操作接口*/
static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.unlocked_ioctl = led_ioctl,
	.write = led_write,
};

/*模块加载函数*/
static int __init mhw_led_init(void)
{
	/*申请设备号*/
	int ret;
	dev_t devno = MKDEV(led_major,led_minor);
	if((ret = register_chrdev_region(devno,number_of_device,"mhw_led_driver")) < 0)
	{
		printk("Failed from register_chrdev_region\n");
		return ret;
	}
	/*向内核注册设备*/
	cdev_init(&cdev,&led_fops);
	cdev.owner = THIS_MODULE;
	if((ret = cdev_add(&cdev,devno,number_of_device)) < 0)
	{
		printk("Failed from cdev_add\n");
		goto err1;
	}
	/*设备的初始化*/
	led_gpg3con = ioremap(0xe03001c0,4);  /*两个寄存器映射*/
	if(led_gpg3con == NULL)
	{
		ret = -EINVAL;
		goto err2;
	}
	led_gpg3dat = ioremap(0xe03001c4,4);
	if(led_gpg3dat == NULL)
	{
		ret = -EINVAL;
		goto err3;
	}

	writel((readl(led_gpg3con) & ~0xffff) | 0x1111,led_gpg3con);
	writel(readl(led_gpg3dat) | 0x0f,led_gpg3dat);

	printk(KERN_INFO "**********mhw_led_driver ok********\n");
	return 0;
err3:
	iounmap(led_gpg3con);
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno,number_of_device);
	return ret;
}

/*模块卸载函数*/
static void __exit mhw_led_exit(void)
{
	dev_t devno = MKDEV(led_major,led_minor);
	iounmap(led_gpg3dat);
	iounmap(led_gpg3con);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,number_of_device);
	printk(KERN_INFO "***********mhw_led_driver end**********\n");
}

module_init(mhw_led_init);
module_exit(mhw_led_exit);
