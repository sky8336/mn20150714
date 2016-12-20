#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "s5pc100_led.h"

#define S5PC100_GPG3CON 0xe03001c0
#define S5PC100_GPG3DAT 0xe03001c4

MODULE_LICENSE("GPL");

static int led_major = 250;
static int led_minor = 0;
static int number_of_device = 1;

static struct cdev cdev;

static unsigned int *gpg3con, *gpg3dat;

static int s5pc100_led_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int s5pc100_led_release(struct inode *inode, struct file *file)
{
	return 0;
}
	
static long s5pc100_led_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
	case LED_ON:
		writel(readl(gpg3dat) | (1 << arg), gpg3dat);
		break;
	case LED_OFF:
		writel(readl(gpg3dat) & ~(1 << arg), gpg3dat);
		break;
	}

	return 0;
}
	
static ssize_t s5pc100_led_write(struct file *file, const char __user *buf, size_t count, loff_t *loff)
{
	struct led_action action;

	if(count != sizeof(struct led_action))
		return -EINVAL;

	if(copy_from_user((char *)&action, buf, sizeof(struct led_action)))
		return -EINVAL;

	if(action.act == LED_ON)
		writel(readl(gpg3dat) | (1 << action.nr), gpg3dat);
	else
		writel(readl(gpg3dat) & ~(1 << action.nr), gpg3dat);

	return sizeof(struct led_action);
}

static struct file_operations s5pc100_led_fops = {
	.owner = THIS_MODULE,
	.open = s5pc100_led_open,
	.release = s5pc100_led_release,
	.unlocked_ioctl = s5pc100_led_unlocked_ioctl,
	.write = s5pc100_led_write,
};

static int s5pc100_led_init(void)
{
	int ret;

	dev_t devno = MKDEV(led_major, led_minor);
	ret = register_chrdev_region(devno, number_of_device, "s5pc100-led"); 
	if(ret < 0)
	{
		printk("failed register_chrdev_region\n");
		return ret;
	}

	cdev_init(&cdev, &s5pc100_led_fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_device);
	if(ret < 0)
	{
		printk("failed cdev_add\n");
		goto err1;
	}

	gpg3con = ioremap(S5PC100_GPG3CON, 4);
	if(gpg3con == NULL)
	{
		ret = -EINVAL;
		goto err2;
	}

	gpg3dat = ioremap(S5PC100_GPG3DAT, 4);
	if(gpg3dat == NULL)
	{
		ret = -EINVAL;
		goto err3;
	}

	writel((readl(gpg3con) & ~0xffff) | 0x1111, gpg3con);
	writel(readl(gpg3dat) | 0xf, gpg3dat);

	return 0;
err3:
	iounmap(gpg3con);
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno, number_of_device);
	return ret;
}

static void s5pc100_led_exit(void)
{
	dev_t devno = MKDEV(led_major, led_minor);

	iounmap(gpg3dat);
	iounmap(gpg3con);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, number_of_device);
}


module_init(s5pc100_led_init);
module_exit(s5pc100_led_exit);
