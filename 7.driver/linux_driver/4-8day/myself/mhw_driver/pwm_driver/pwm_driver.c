#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "s5pc100_pwm.h"

#define S5PC100_GPDCON 0xE0300080
#define S5PC100_TCFG0  0xEA000000
#define S5PC100_TCFG1  0xEA000004
#define S5PC100_TCNTB1 0xEA000018
#define S5PC100_TCMPB1 0xEA00001C
#define S5PC100_TCON   0xEA000008

/*许可证申明*/
MODULE_LICENSE("GPL");

/*主次设备号定义*/
static int pwm_major = 300;
static int pwm_minor = 0;
static int number_of_device = 1;

/*字符设备控制块*/
static struct cdev cdev;

/*寄存器映射指针定义,存放映射后的虚拟地址*/
static unsigned int *pwm_gpdcon;
static unsigned int *pwm_tcfg0;
static unsigned int *pwm_tcfg1;
static unsigned int *pwm_tcntb1;
static unsigned int *pwm_tcmpb1;
static unsigned int *pwm_tcon;

/*file_operations中的函数实现*/
static int pwm_open(struct inode *inode,struct file *file)
{
	printk("call pwm_open\n");	
	
	writel((readl(pwm_gpdcon) & ~(0xf << 4)) | (0x2 << 4),pwm_gpdcon);
	writel((readl(pwm_tcfg0) & ~0xff) | 0x7f,pwm_tcfg0);
	writel((readl(pwm_tcfg1) & ~(0xf << 4)) | 0x2 << 4,pwm_tcfg1);
	writel(500,pwm_tcntb1);
	writel(250,pwm_tcmpb1);
	writel((readl(pwm_tcon) & ~(0xf << 8)) | 0x2 << 8,pwm_tcon);
	writel((readl(pwm_tcon) & ~(0xf << 8)) | 0x9 << 8,pwm_tcon);

	return 0;
}
static int pwm_release(struct inode *inode,struct file *file)
{
	printk("call pwm_release\n");
	return 0;
}
static long pwm_ioctl(struct file *file,unsigned int cmd,unsigned long arg)
{
	switch(cmd)
	{
	case PWM_ON:
		writel((readl(pwm_tcon) & ~(0xf << 8)) | (0x1 << 8),pwm_tcon);
		break;
	case PWM_OFF:
		writel((readl(pwm_tcon) & ~(0xf << 8)),pwm_tcon);
		break;
	case SET_PRE:
		writel((readl(pwm_tcon) & ~(0xf << 8)),pwm_tcon);
		writel((readl(pwm_tcfg0) & ~0xff) | arg,pwm_tcfg0);
		writel((readl(pwm_tcon) & ~(0xf << 8)) | 0x9 << 8,pwm_tcon);
		break;
	case SET_CNT:
		writel(arg,pwm_tcntb1);
		writel(arg/2,pwm_tcmpb1);
		break;
	}
	return 0;
}

/*对设备的操作接口*/
static struct file_operations pwm_fops = {
	.owner = THIS_MODULE,
	.open = pwm_open,
	.release = pwm_release,
	.unlocked_ioctl = pwm_ioctl,
};

/*模块加载函数*/
static int __init mhw_pwm_init(void)
{
	/*申请设备号*/
	int ret;
	dev_t devno = MKDEV(pwm_major,pwm_minor);
	if((ret = register_chrdev_region(devno,number_of_device,"mhw_pwm_driver")) < 0)
	{
		printk("Failed from register_chrdev_region\n");
		return ret;
	}
	/*向内核注册设备*/
	cdev_init(&cdev,&pwm_fops);
	cdev.owner = THIS_MODULE;
	if((ret = cdev_add(&cdev,devno,number_of_device)) < 0)
	{
		printk("Failed from cdev_add\n");
		goto err1;
	}

	/*设备的初始化*/
	pwm_gpdcon = ioremap(S5PC100_GPDCON,4);
	if(pwm_gpdcon == NULL)
	{
		ret = -EINVAL;
		goto err2;
	}

	pwm_tcfg0 = ioremap(S5PC100_TCFG0,4);
	if(pwm_tcfg0 == NULL)
	{
		ret = -EINVAL;
		goto err3;
	}

	pwm_tcfg1 = ioremap(S5PC100_TCFG1,4);
	if(pwm_tcfg1 == NULL)
	{
		ret = -EINVAL;
		goto err4;
	}
	
	pwm_tcntb1 = ioremap(S5PC100_TCNTB1,4);
	if(pwm_tcntb1 == NULL)
	{
		ret = -EINVAL;
		goto err5;
	}

	pwm_tcmpb1 = ioremap(S5PC100_TCMPB1,4);
	if(pwm_tcmpb1 == NULL)
	{
		ret = -EINVAL;
		goto err6;
	}

	pwm_tcon = ioremap(S5PC100_TCON,4);
	if(pwm_tcon == NULL)
	{
		ret = -EINVAL;
		goto err7;
	}


	printk(KERN_INFO "**********mhw_pwm_driver ok********\n");
	return 0;
err7:
	iounmap(pwm_tcmpb1);
err6:
	iounmap(pwm_tcntb1);
err5:
	iounmap(pwm_tcfg1);
err4:
	iounmap(pwm_tcfg0);
err3:
	iounmap(pwm_gpdcon);
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno,number_of_device);
	return ret;
}

/*模块卸载函数*/
static void __exit mhw_pwm_exit(void)
{
	dev_t devno = MKDEV(pwm_major,pwm_minor);
	iounmap(pwm_tcon);
	iounmap(pwm_tcmpb1);
	iounmap(pwm_tcntb1);
	iounmap(pwm_tcfg1);
	iounmap(pwm_tcfg0);
	iounmap(pwm_gpdcon);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,number_of_device);
	printk(KERN_INFO "***********mhw_pwm_driver end**********\n");
}

module_init(mhw_pwm_init);
module_exit(mhw_pwm_exit);
