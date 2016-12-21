#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/io.h>

#include <mach/regs-gpio.h>

#include "s5pc100_beep_io.h"


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("farsight");

static int beep_major = 250;
static int beep_minor = 0;
static struct cdev beep_cdev;

static void beep_init(void)
{
	writel((readl(S5PC100_GPD_BASE) & (~0xF << 4)) | (0x1 << 4), S5PC100_GPD_BASE);
}

static void beep_on(void)
{
	writel((readl(S5PC100_GPG3_BASE + 4) | (0x1 << 1)), S5PC100_GPD_BASE + 4);
}

static void beep_off(void)
{
	writel((readl(S5PC100_GPG3_BASE + 4) & (~0x1 << 1)), S5PC100_GPD_BASE + 4);
}

static int s5pc100_beep_open(struct inode *inode, struct file *file)
{
	printk("beep: device open\n");
	beep_init();
	return 0;
}

static int s5pc100_beep_close(struct inode *inode, struct file *file)
{
	printk("pwm: device close\n");
	return 0;
}

static int s5pc100_beep_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("beep: device ioctl\n");
	switch(cmd)
	{
	case BEEP_ON:
		printk("beep: BEEP ON\n");
		beep_on();
		break;
	case BEEP_OFF:
		printk("beep: BEEP OFF\n");
		beep_off();
		break;
	default:
		printk("beep: available command\n");
		break;
	}
	return 0;
}

static struct file_operations s5pc100_beep_ops = {
	.owner 		= THIS_MODULE,
	.open 		= s5pc100_beep_open,
	.release 	= s5pc100_beep_close,
	.ioctl		= s5pc100_beep_ioctl
};

static int beep_setup_cdev(struct cdev *cdev, 
		struct file_operations *fops)
{
	int result;
	dev_t devno = MKDEV(beep_major, beep_minor);
	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	result = cdev_add(cdev, devno, 1);
	if(result)
	{
		printk("beep: cdev add failed\n");
		return result;
	}
	return 0;
}

static int __init s5pc100_beep_init(void)
{
	int result;
	dev_t devno = MKDEV(beep_major, beep_minor);
	result = register_chrdev_region(devno, 1, "s5pc100_beep");
	if(result)
	{
		printk("beep: unable to get major %d\n", beep_major);
		return result;
	}

	result = beep_setup_cdev(&beep_cdev, &s5pc100_beep_ops);
	if(result)
		return result;

	printk("beep: driver instalpwm, with major %d!\n", beep_major);
	return 0;
}

static void __exit s5pc100_beep_exit(void)
{
	dev_t devno = MKDEV(beep_major, beep_minor);
	cdev_del(&beep_cdev);
	unregister_chrdev_region(devno, 1);
	printk("beep: driver uninstalled!\n");
}

module_init(s5pc100_beep_init);
module_exit(s5pc100_beep_exit);
