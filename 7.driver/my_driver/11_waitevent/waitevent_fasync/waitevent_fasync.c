#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "head.h"

#define CLASS_DEV_CREATE
#define WAITEVENT_USE
#define FASYNC_USE

#ifdef WAITEVENT_USE
#include <linux/device.h>
#endif

#ifdef WAITEVENT_USE
#include <linux/sched.h>
#include <linux/poll.h>
#endif

#define N 128
MODULE_LICENSE("GPL");

char data[N] = "\0";

static int major = 220;
static int minor = 0;

#ifdef CLASS_DEV_CREATE
static struct class *cls;
static struct device *device;
#endif

#ifdef WAITEVENT_USE
int  flag  =1;
wait_queue_head_t  hello_readq;
#endif

#ifdef FASYNC_USE
struct fasync_struct  *fasync;
#endif

static int hello_open(struct inode *inode, struct file *file)
{
	printk("hello_open\n");
	return  0;
}

static int hello_release(struct inode *inode, struct file *file)
{
	printk("hello_release\n");
	return 0;
}

static ssize_t hello_read(struct file *file, char __user *buff,
		size_t size, loff_t *loff)
{
	if (size > N)
		size = N;
	if (size < 0)
		return -EINVAL;

#ifdef WAITEVENT_USE
	wait_event_interruptible(hello_readq, flag != 1);
#endif

	if (copy_to_user(buff, data, size))
		return -ENOMEM;

#ifdef WAITEVENT_USE
	flag  = 1;
#endif
	printk("hello_read: buff = %s\n", buff);
	return  size;
}

static ssize_t hello_write(struct file *file, const char __user *buff,
		size_t size, loff_t *loff)
{
	if (size > N)
		size = N;
	if (size < 0)
		return -EINVAL;

	memset(data, '\0', sizeof(data));

	if (copy_from_user(data, buff, size))
		return -ENOMEM;

#ifdef WAITEVENT_USE
	flag = 0;
	wake_up_interruptible(&hello_readq);
#endif

#ifdef FASYNC_USE
	kill_fasync(&fasync, SIGIO, POLLIN);
#endif

	printk("hello_write\n");
	printk("data = %s\n",data);

	return size;
}

static long hello_unlocked_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
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

#ifdef FASYNC_USE
static int hello_fasync(int fd, struct file *file, int on)
{
	return fasync_helper(fd, file, on, &fasync);
}
#endif

static struct cdev cdev;
static struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
	.unlocked_ioctl = hello_unlocked_ioctl,
#ifdef FASYNC_USE
	.fasync = hello_fasync,
#endif
};

static int hello_init(void)
{
	int ret;
	dev_t  devno = MKDEV(major, minor);

	ret = register_chrdev_region(devno, 1, "hello");
	if (0 != ret) {
		//alloc_chrdev_region(&devno, 0, 1, "duang");
		printk("register_chrdev_region \n");
	}

	cdev_init(&cdev, &hello_ops);
	ret = cdev_add(&cdev, devno, 1);
	if (0 != ret) {
		unregister_chrdev_region(devno, 1);
		printk("cdev_add \n");
		return -1;
	}

#ifdef CLASS_DEV_CREATE
	cls = class_create(THIS_MODULE, "xhello"); //创建一个类
	device_create(cls, device, devno + 0, NULL, "xhello-0"); //创建设备结点，次设备号加1
	device_create(cls, device, devno + 1, NULL, "xhello-1");
	device_create(cls, device, devno + 2, NULL, "xhello-2");
	device_create(cls, device, devno + 3, NULL, "xhello-3");
#endif

#ifdef WAITEVENT_USE
	init_waitqueue_head(&hello_readq);
#endif
	printk("hello_init \n");
	return 0;
}

static void hello_exit(void)
{
	dev_t devno = MKDEV(major, minor);

#ifdef CLASS_DEV_CREATE
	device_destroy(cls, devno + 0); //销毁设备节点
	device_destroy(cls, devno + 1);
	device_destroy(cls, devno + 2);
	device_destroy(cls, devno + 3);
	class_destroy(cls); //销毁类
#endif

	cdev_del(&cdev);
	unregister_chrdev_region(devno, 1);
	printk("hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
