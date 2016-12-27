#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "head.h"

#define CLASS_DEV_CREATE
#define WAITEVENT_USE
#define POLL_USE
#define FASYNC_USE
#define IOREMAP_LED

#ifdef CLASS_DEV_CREATE
#include <linux/device.h>
#endif

#ifdef WAITEVENT_USE
#include <linux/sched.h>
#endif

#if (defined FASYNC_USE) ||(defined POLL_USE)
#include <linux/poll.h>
#endif

#ifdef IOREMAP_LED
#include <asm/io.h>
#endif

#define N 128
#define num_of_device 4

#ifdef IOREMAP_LED
#define GPG3CON  0xE03001C0 //LED
#define GPG3DAT  0xE03001C4

#define GPH0CON  0xE0300C00 //key
#define GPH0DAT  0xE0300C04
#endif
MODULE_LICENSE("GPL");

char data[N] = "\0";

static int major = 220;
static int minor = 0;

#ifdef CLASS_DEV_CREATE
static struct class *cls;
static struct device *device;
#endif

#ifdef WAITEVENT_USE
int flag_rw = 1; //写
static wait_queue_head_t hello_readq; //定义“等待队列头”
static wait_queue_head_t hello_writeq;
#endif

#ifdef FASYNC_USE
static struct fasync_struct *fap;
#endif

#ifdef IOREMAP_LED
unsigned int *gpg3con;
unsigned int *gpg3dat;
#endif

static int hello_open(struct inode *inode, struct file *file)
{
	printk("hello_open\n");
	return 0;
}

static int hello_release(struct inode *inode, struct file *file)
{
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

#ifdef WAITEVENT_USE
	wait_event_interruptible(hello_readq, flag_rw != 1); //等待事件，flag_rw != 1 唤醒条件
#endif

	if (copy_to_user(buf, data, size))
		return -ENOMEM;

#ifdef WAITEVENT_USE
	flag_rw = 1;
	wake_up_interruptible(&hello_writeq);
#endif
	printk("hello_read\n");
	return size;
}

static ssize_t hello_write(struct file *file, const char __user *buf,
		size_t size, loff_t *loff)
{
	wait_event_interruptible(hello_readq,flag_rw != 0);

	if (size > N)
		size = N;
	if (size < 0)
		return -EINVAL;

	if (copy_from_user(data, buf, size))
		return -ENOMEM;

#ifdef WAITEVENT_USE
	flag_rw = 0;
	wake_up_interruptible(&hello_readq); //唤醒队列
#endif

#ifdef FASYNC_USE
	kill_fasync(&fap, SIGIO, POLLIN);
#endif

	printk("hello_write\n");
	printk("data = %s\n", data);

	return size;
}

static long hello_unlocked_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	switch(cmd) {
	case LED_ON:
		printk("LED_ON\n");
#ifdef IOREMAP_LED
		*gpg3dat |= 0xf;
#endif

		break;
	case LED_OFF:
		printk("LED_OFF\n");
#ifdef IOREMAP_LED
		*gpg3dat &= (~ 0xf);
#endif
		break;
	}
	printk("hello_unlocked_ioctl\n");
	return 0;
}

#ifdef POLL_USE
static unsigned int hello_poll(struct file *file, struct poll_table_struct *table)
{
	int mask = 0;
	poll_wait(file,&hello_readq,table);
	poll_wait(file,&hello_writeq,table);

	if (flag_rw != 0)
		mask |= POLLOUT;
	if (flag_rw != 1)
		mask |= POLLIN;

	return mask;
}
#endif

#ifdef FASYNC_USE
static int hello_fasync(int fd, struct file *file, int on)
{
	return (fasync_helper(fd,file,on,&fap));
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
#ifdef POLL_USE
	.poll = hello_poll,
#endif
#ifdef FASYNC_USE
	.fasync = hello_fasync,
#endif
};
//自定义加载函数
static int hello_init(void)
{
	int ret;
	dev_t devno = MKDEV(major, minor); //申请设备号

	ret = register_chrdev_region(devno, num_of_device, "xhello"); //注册设备号
	if (0 != ret) {
		//alloc_chrdev_region(&devno,0,1,DEV_NAME); //自动分配设备号
		printk("register_chrdev_region : error\n");
		return -1;
	}

	cdev_init(&cdev, &hello_ops); //初始化cdev结构体
	ret = cdev_add(&cdev, devno, num_of_device); //注册cdev结构体
	if (0 != ret) {
		unregister_chrdev_region(devno, num_of_device); //cdev结构体注册失败，卸载设备号
		printk("cdev_add error\n");
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
	init_waitqueue_head(&hello_readq); //初始化“等待队列头”
	init_waitqueue_head(&hello_writeq); //初始化“等待队列头”
#endif

#ifdef IOREMAP_LED
	gpg3con = ioremap(GPG3CON, 4); //LED:虚拟定制映射，第一个参数为物理地址，第二个为映射字节大小
	gpg3dat = ioremap(GPG3DAT, 4);

	*gpg3con = ((*gpg3con) & (~0xffff)) | 0x1111;
	*gpg3dat = ((*gpg3dat) & (~0xf)) | 0xf;

	//gph0con = ioremap(GPH0CON,4); //key
	//gph0dat = ioremap(GPH0DAT,4);
#endif

	printk("hello_init\n");
	return 0;
}
//自定义卸载函数
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

	cdev_del(&cdev); //卸载cdev结构体
	unregister_chrdev_region(devno, num_of_device); //卸载设备号

	printk("hello_exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
