#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "s5pc100_pwm.h"

#define S5PC100_GPDCON		0xe0300080
#define S5PC100_TIMER_BASE	0xea000000 

#define S5PC100_TCFG0	0x00
#define S5PC100_TCFG1	0x04
#define S5PC100_TCON	0x08
#define S5PC100_TCNTB1	0x18
#define S5PC100_TCMPB1	0x1c

MODULE_LICENSE("GPL");

static int pwm_major = 250;
static int pwm_minor = 0;
static int number_of_device = 1;

static struct cdev cdev;

unsigned int *gpdcon;
void __iomem *timer_base;

static int s5pc100_pwm_open(struct inode *inode, struct file *file)
{
	writel((readl(gpdcon) & ~(0xf << 4)) | (0x2 << 4), gpdcon);
	writel(readl(timer_base + S5PC100_TCFG0) | 0xff, timer_base + S5PC100_TCFG0);	
	writel((readl(timer_base + S5PC100_TCFG1) & ~(0xf << 4)) | 0x1 << 4, timer_base + S5PC100_TCFG1);
	writel(300, timer_base + S5PC100_TCNTB1);
	writel(150, timer_base + S5PC100_TCMPB1);
	writel((readl(timer_base + S5PC100_TCON) & ~(0xf << 8)) | 0x2 << 8, timer_base + S5PC100_TCON);
//	writel((readl(timer_base + S5PC100_TCON) & ~(0xf << 8)) | 0x9 << 8, timer_base + S5PC100_TCON);

	return 0;
}

static int s5pc100_pwm_release(struct inode *inode, struct file *file)
{
	return 0;
}
	
static long s5pc100_pwm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
	case PWM_ON:
		writel((readl(timer_base + S5PC100_TCON) & ~(0xf << 8)) | 0x9 << 8, timer_base + S5PC100_TCON);
		break;
	case PWM_OFF:
		writel(readl(timer_base + S5PC100_TCON) & ~(0xf << 8), timer_base + S5PC100_TCON);
		break;
	case SET_PRE:
		writel(readl(timer_base + S5PC100_TCON) & ~(0xf << 8), timer_base + S5PC100_TCON);
		writel((readl(timer_base + S5PC100_TCFG0) & ~0xff) | arg, timer_base + S5PC100_TCFG0);	
		writel((readl(timer_base + S5PC100_TCON) & ~(0xf << 8)) | 0x9 << 8, timer_base + S5PC100_TCON);
		break;
	case SET_CNT:
		writel(arg, timer_base + S5PC100_TCNTB1);
		writel(arg / 2, timer_base + S5PC100_TCMPB1);
		break;
	}

	return 0;
};
	
static struct file_operations s5pc100_pwm_fops = {
	.owner = THIS_MODULE,
	.open = s5pc100_pwm_open,
	.release = s5pc100_pwm_release,
	.unlocked_ioctl = s5pc100_pwm_ioctl,
};

static int s5pc100_pwm_init(void)
{
	int ret;

	dev_t devno = MKDEV(pwm_major, pwm_minor);
	ret = register_chrdev_region(devno, number_of_device, "s5pc100-pwm"); 
	if(ret < 0)
	{
		printk("failed register_chrdev_region\n");
		return ret;
	}

	cdev_init(&cdev, &s5pc100_pwm_fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_device);
	if(ret < 0)
	{
		printk("failed cdev_add\n");
		goto err1;
	}

	gpdcon = ioremap(S5PC100_GPDCON, 4);
	if(gpdcon == NULL)
	{
		ret = -EINVAL;
		goto err2;
	}

	timer_base = ioremap(S5PC100_TIMER_BASE, 0x50);
	if(timer_base == NULL)
	{
		ret = -EINVAL;
		goto err3;
	}
	return 0;
err3:
	iounmap(gpdcon);
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno, number_of_device);
	return ret;
}

static void s5pc100_pwm_exit(void)
{
	dev_t devno = MKDEV(pwm_major, pwm_minor);

	iounmap(timer_base);
	iounmap(gpdcon);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, number_of_device);
}


module_init(s5pc100_pwm_init);
module_exit(s5pc100_pwm_exit);
