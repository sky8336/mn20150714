#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "head.h"
#include <linux/device.h>

#define ATOMIC_USE
#ifdef ATOMIC_USE
#include <asm/atomic.h>
#endif

#define N 128
#define num_of_device 4
MODULE_LICENSE("GPL");

char data[N];

static int major = 220;
static int minor = 0;

static struct class *cls;
static struct device *device;

#ifdef ATOMIC_USE
atomic_t v = ATOMIC_INIT(1);
#endif


static int hello_open(struct inode *inode, struct file *file)
{
#ifdef ATOMIC_USE
	if (!atomic_dec_and_test(&v)) {
		atomic_inc(&v);
		return -EBUSY;
	}
#endif

	printk("hello_open\n");
	//printk("minor = %d\n", iminor(inode));	//获取次设备号
	//printk("minor = %d\n", iminor(file->f_dentry->d_inode)); //获取次设备号，第2种方法
	return 0;
}

static int hello_release(struct inode *inode, struct file *file)
{
#ifdef ATOMIC_USE
	atomic_inc(&v);
#endif

	printk("hello_release\n");

	return 0;
}

static ssize_t hello_read(struct file *file, char __user *buf,
		size_t size, loff_t *loff)
{
	if (size > N)
		size = N;
	if (size < 0)
		return -EINVAL;

	if (copy_to_user(buf, data, size))
		return -ENOMEM;

	printk("hello_read\n");
	return size;
}

static ssize_t hello_write(struct file *file, const char __user *buff,
		size_t size, loff_t *loff)
{
	if (size > N)
		size = N;
	if (size < 0)
		return -EINVAL;

	memset(data, '\0', sizeof(data));

	if (0 != copy_from_user(data, buff, size))
		return -ENOMEM;

	printk("hello_write\n");
	printk("data = %s\n", data);

	return size;
}

static long hello_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
	case LED_ON:
		printk("LED_ON\n");
		break;
	case LED_OFF:
		printk("LED_OFF\n");
		break;
	}
	printk("hello_unlocked_ioctl\n");

	return 0;
}

static struct cdev cdev;
static struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
	.unlocked_ioctl = hello_unlocked_ioctl,
};

static int hello_init(void)
{
	int ret;
	dev_t devno = MKDEV(major, minor); //申请设备号

	//alloc_chrdev_region(&devno,0,1,DEV_NAME);
	ret = register_chrdev_region(devno, num_of_device, "xhello"); //注册设备号
	if (0 != ret) {
		printk("register_chrdev_region : error\n");
		return -1;
	}

	cdev_init(&cdev,&hello_ops); //初始化cdev结构体
	ret = cdev_add(&cdev, devno, num_of_device); //注册cdev结构体
	if (0 != ret) {
		printk("cdev_add\n");
		unregister_chrdev_region(devno, num_of_device); //cdev结构体注册失败，卸载设备号
		return -1;
	}

	cls = class_create(THIS_MODULE, "xhello"); //创建一个类
	device_create(cls, device, devno + 0, NULL, "xhello-0"); //创建设备结点，次设备号加1
	device_create(cls, device, devno + 1, NULL, "xhello-1");
	device_create(cls, device, devno + 2, NULL, "xhello-2");
	device_create(cls, device, devno + 3, NULL, "xhello-3");

	printk("hello_init\n");
	return 0;
}

static void hello_exit(void)
{
	dev_t devno = MKDEV(major,minor);

	device_destroy(cls, devno + 0); //销毁设备节点
	device_destroy(cls, devno + 1);
	device_destroy(cls, devno + 2);
	device_destroy(cls, devno + 3);
	class_destroy(cls); //销毁类

	cdev_del(&cdev); //卸载cdev结构体
	unregister_chrdev_region(devno, num_of_device); //卸载设备号

	printk("hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
