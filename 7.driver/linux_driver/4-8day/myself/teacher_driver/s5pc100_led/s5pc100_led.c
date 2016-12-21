#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>    //与文件操作相关的头文件
#include <linux/cdev.h>  //与设备相关的头文件

#include <asm/io.h>      //readl和writel的头文件
#include <asm/uaccess.h> //write中与用户交互时的头文件

#include "s5pc100_led.h"  //定义命令的头文件

#define S5PC100_GPG3CON 0xe03001c0       //定义宏，用于内存映射时填写物理地址
#define S5PC100_GPG3DAT 0xe03001c4

MODULE_LICENSE("GPL");   //许可证申明

static int led_major = 250;  //申请主次设备号
static int led_minor = 0;
static int number_of_device = 1;

static struct cdev cdev;  //向内核添加的设备结构体

static unsigned int *gpg3con, *gpg3dat;   //用于接收内存映射时返回的虚拟地址的指针

static int s5pc100_led_open(struct inode *inode, struct file *file)  //被用户的open调用
{
	return 0;
}

static int s5pc100_led_release(struct inode *inode, struct file *file)  //被用户的close调用
{
	return 0;
}
	
//方法一：用ioctl传递命令
static long s5pc100_led_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
	case LED_ON:
		writel(readl(gpg3dat) | (1 << arg), gpg3dat);  //操作虚拟地址，操作系统屏蔽了硬件的实现细节，向驱动提供逻辑地址
		break;
	case LED_OFF:
		writel(readl(gpg3dat) & ~(1 << arg), gpg3dat);
		break;
	}

	return 0;
}
	
//方法二：用write传递命令
static ssize_t s5pc100_led_write(struct file *file, const char __user *buf, size_t count, loff_t *loff)
{
	struct led_action action;

	if(count != sizeof(struct led_action))   //传送的数据大小为结构体大小
		return -EINVAL;

	if(copy_from_user((char *)&action, buf, sizeof(struct led_action)))  //从用户空间拷贝数据到内核空间，成功返回0，失败返回一个正数
		return -EINVAL;                                                  //返回值有点另类

	if(action.act == LED_ON)
		writel(readl(gpg3dat) | (1 << action.nr), gpg3dat);
	else
		writel(readl(gpg3dat) & ~(1 << action.nr), gpg3dat);

	return sizeof(struct led_action);
}

//内核提供给应用程序的调用接口
static struct file_operations s5pc100_led_fops = {
	.owner = THIS_MODULE,
	.open = s5pc100_led_open,
	.release = s5pc100_led_release,
	.unlocked_ioctl = s5pc100_led_unlocked_ioctl,
	.write = s5pc100_led_write,
};

static int s5pc100_led_init(void)      //模块加载函数
{
	int ret;

	dev_t devno = MKDEV(led_major, led_minor);
	ret = register_chrdev_region(devno, number_of_device, "s5pc100-led"); 
	if(ret < 0)
	{
		printk("failed register_chrdev_region\n");
		return ret;
	}

	cdev_init(&cdev, &s5pc100_led_fops);    //初始化设备
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_device);   //向内核添加设备
	if(ret < 0)
	{
		printk("failed cdev_add\n");
		goto err1;
	}

	                //(要映射的物理地址，映射的大小单位是字节)
	gpg3con = ioremap(S5PC100_GPG3CON, 4);   //内存映射，返回一个指向虚拟地址的指针，此处可以用int接收，也可以用void接收
	if(gpg3con == NULL)                      //int接收时下一个加1即可，因为int型加1表示加了4字节，用void接收时下一个加4，void只加1字节
	{                                        //加1或加4后就可将两个映射合并为一个映射 
		ret = -EINVAL;
		goto err2;
	}

	gpg3dat = ioremap(S5PC100_GPG3DAT, 4);
	if(gpg3dat == NULL)     //内存映射需要判断返回值以确定是否映射成功，映射不成功时返回一个空指针
	{
		ret = -EINVAL;
		goto err3;
	}

	writel((readl(gpg3con) & ~0xffff) | 0x1111, gpg3con);  //通过操作虚拟地址初始化GPG3CON和GPG3DAT寄存器
	writel(readl(gpg3dat) | 0xf, gpg3dat);

	return 0;
err3:
	iounmap(gpg3con);  //映射GPG3DAT失败时需要取消已经映射好的GPG3CON,顺寻向下执行并释放设备和设备号并返回错误码
err2:
	cdev_del(&cdev);   //映射GPG3CON失败时需要释放设备，顺序向下执行并释放设备和设备号并返回错误码
err1:
	unregister_chrdev_region(devno, number_of_device);  //向内核注册设备失败时需要释放设备号并返回错误码
	return ret;
}

static void s5pc100_led_exit(void)   //模块卸载函数
{
	dev_t devno = MKDEV(led_major, led_minor);

	iounmap(gpg3dat);
	iounmap(gpg3con);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, number_of_device);
}

//加载函数和卸载函数的申明
module_init(s5pc100_led_init);
module_exit(s5pc100_led_exit);
