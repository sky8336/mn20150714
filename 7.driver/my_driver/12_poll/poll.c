#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "head.h"
#include <linux/device.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/poll.h>

#define N 128
#define num_of_device 4
MODULE_LICENSE("GPL");

char data[N] = "\0";

static int major = 250;
static int minor = 0;


static struct class *cls;
static struct device *device;

//int flag = 1; //未打开
//spinlock_t lock; //初始化自旋锁

int flag_rw = 1; //写
wait_queue_head_t hello_readq; //定义“等待队列头”
wait_queue_head_t hello_writeq;

static int hello_open(struct inode *inode,struct file *file)
{
#if 0
	spin_lock(&lock); //获得自旋锁
	if(flag != 1){
		spin_unlock(&lock); 
		return -EBUSY;
	}
	
	flag --;
	spin_unlock(&lock); //释放自旋锁
#endif
	printk("hello_open\n");
//	printk("minor = %d\n",iminor(inode)); 	//获取次设备号
//	printk("minor = %d\n",iminor(file->f_dentry->d_inode)); //获取次设备号，第2种方法
	return 0;
}

static int hello_release (struct inode *inode, struct file *file)
{
//	flag ++;

	printk("hello_release\n");

	return 0;
}

static ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *loff)
{
	if(size > N){
		size = N;
	}
	if(size < 0){
		return EINVAL;  
	}
	
	wait_event_interruptible(hello_readq,flag_rw != 1); //等待事件，flag_rw != 1 唤醒条件

	if(copy_to_user(buf,data,size)){
		return ENOMEM;
	}
	
	flag_rw = 1;
	wake_up_interruptible(&hello_writeq);
	printk("hello_read\n");
	return size;

}

static ssize_t hello_write (struct file *file, const char __user *buff, size_t size, loff_t *loff)
{
	wait_event_interruptible(hello_readq,flag_rw != 0);

	if(size > N)
		size = N;
	if(size < 0)
		return EINVAL;

	if(0 != copy_from_user(data,buff,20)){
		return ENOMEM; 
	}

	flag_rw = 0;
	wake_up_interruptible(&hello_readq); //唤醒队列

	printk("hello_write\n");
	printk("data = %s\n",data);
	return size;
}
static long hello_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
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

static unsigned int hello_poll (struct file *file, struct poll_table_struct *table)
{
	int mask = 0;
	poll_wait(file,&hello_readq,table);
	poll_wait(file,&hello_writeq,table);

	if(flag_rw != 0){
		mask |= POLLOUT;
	}
	if(flag_rw != 1)
	{
		mask |= POLLIN;
	}
	return mask;
}

static struct cdev cdev;
static struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
	.unlocked_ioctl = hello_unlocked_ioctl,
	.poll = hello_poll,
};

static int hello_init(void) //自定义加载函数
{
	int ret;
	dev_t devno = MKDEV(major,minor); //申请设备号

	ret = register_chrdev_region(devno,num_of_device,"xhello"); //注册设备号
	if(0 != ret){
//		alloc_chrdev_region(&devno,0,1,DEV_NAME); //自动分配设备号
		printk("register_chrdev_region : error\n");
		return -1;
	}

	cdev_init(&cdev,&hello_ops); //初始化cdev结构体
	ret = cdev_add(&cdev,devno,num_of_device); //注册cdev结构体
	if(0 != ret){
		printk("cdev_add\n");
		unregister_chrdev_region(devno,num_of_device); //cdev结构体注册失败，卸载设备号
		return -1;
	}

	cls = class_create(THIS_MODULE,"xhello"); //创建一个类
	device_create(cls, device,devno + 0,NULL,"xhello-0"); //创建设备结点，次设备号加1
	device_create(cls, device,devno + 1,NULL,"xhello-1");
	device_create(cls, device,devno + 2,NULL,"xhello-2");
	device_create(cls, device,devno + 3,NULL,"xhello-3");

//	spin_lock_init(&lock); //初始化自旋锁
	init_waitqueue_head(&hello_readq); //初始化“等待队列头”
	init_waitqueue_head(&hello_writeq); //初始化“等待队列头”
	printk("hello_init\n");
	return 0;
}

static void hello_exit(void) //自定义卸载函数
{

	dev_t devno = MKDEV(major,minor);

	device_destroy(cls, devno + 0); //销毁设备节点
	device_destroy(cls, devno + 1);
	device_destroy(cls, devno + 2);
	device_destroy(cls, devno + 3);
	class_destroy(cls); //销毁类
	
	cdev_del(&cdev); //卸载cdev结构体
	unregister_chrdev_region(devno,num_of_device); //卸载设备号

	printk("hello_exit\n");	

}

module_init(hello_init);
module_exit(hello_exit);



