#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

static int key_major = 300;
static int key_minor = 0;
static int number_of_device = 1;

static struct cdev cdev;
//static struct fasync_struct;
int key;
int flags = 0;
/*定义等待队列头*/
wait_queue_head_t key_queue;

/*定义自旋锁*/
spinlock_t lock;

/*中断处理函数的实现*/
irqreturn_t key_interrupt_handler(int irqno,void *devid)
{
	printk("*****you entrance interrupt already*****\n");

	flags = 1;

	switch(irqno)
	{
	case IRQ_EINT(1):
		key = 1;
		break;
	case IRQ_EINT(2):
		key = 2;
		break;
	case IRQ_EINT(3):
		key = 3;
		break;
	case IRQ_EINT(4):
		key = 4;
		break;
	case IRQ_EINT(6):
		key = 5;
		break;
	case IRQ_EINT(7):
		key = 6;
		break;
	}

	/*唤醒key_read中的睡眠*/
	wake_up_interruptible(&key_queue);

	return IRQ_HANDLED;
}

/*file_operations中函数的实现*/
static int key_open(struct inode *inode,struct file *file)
{
	printk("call key_open\n");
	return 0;
}
static int key_release(struct inode *inode,struct file *file)
{
	printk("call key_release\n");
	return 0;
}
ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *loff)
{
	int data;
	/*睡眠等待唤醒*/
	wait_event_interruptible(key_queue,flags != 0);

	spin_lock_irq(&lock); //获得自旋锁
	data = key;
	spin_unlock_irq(&lock);  //释放自旋锁

	if(count != sizeof(key))
		return -EINVAL;
	if(copy_to_user(buf,&data,count))
		return -EFAULT;
	flags = 0;
	return count;
}

static struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.release = key_release,
	.read = key_read,
//	.fasync = key_fasync,
};

/*模块加载函数*/
static int __init mhw_key_init(void)
{
	int ret;

	dev_t devno = MKDEV(key_major,key_minor);
	if((ret = register_chrdev_region(devno,number_of_device,"mhw_key_driver")) < 0)
	{
		printk("Failed from register_chrdev_region\n");
		return ret;
	}

	cdev_init(&cdev,&key_fops);
	cdev.owner = THIS_MODULE;
	if((ret = cdev_add(&cdev,devno,number_of_device)) < 0)
	{
		printk("Failed from cdev_add\n");
		goto err1;
	}

	/*初始化等待队列头*/
	init_waitqueue_head(&key_queue);
	/*初始化自旋锁*/
	spin_lock_init(&lock);

	/*设备的初始化*/
	ret = request_irq(IRQ_EINT(1),key_interrupt_handler,IRQF_DISABLED | IRQF_TRIGGER_RISING,"key1",NULL); 
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
		goto err2;
	}
	ret = request_irq(IRQ_EINT(2),key_interrupt_handler,IRQF_DISABLED | IRQF_TRIGGER_FALLING,"key2",NULL); 
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
		goto err3;
	}
	ret = request_irq(IRQ_EINT(3),key_interrupt_handler,IRQF_DISABLED | IRQF_TRIGGER_FALLING,"key3",NULL); 
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
		goto err4;
	}
	ret = request_irq(IRQ_EINT(4),key_interrupt_handler,IRQF_DISABLED | IRQF_TRIGGER_FALLING,"key4",NULL); 
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
		goto err5;
	}
	ret = request_irq(IRQ_EINT(6),key_interrupt_handler,IRQF_DISABLED | IRQF_TRIGGER_FALLING,"key5",NULL); 
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
		goto err6;
	}
	ret = request_irq(IRQ_EINT(7),key_interrupt_handler,IRQF_DISABLED | IRQF_TRIGGER_FALLING,"key6",NULL); 
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
	}
		
	printk(KERN_INFO "************mhw_key_driver ok***********\n");
	return 0;
err6:
	free_irq(IRQ_EINT(6),NULL);
err5:
	free_irq(IRQ_EINT(4),NULL);
err4:
	free_irq(IRQ_EINT(3),NULL);
err3:
	free_irq(IRQ_EINT(2),NULL);
err2:
	free_irq(IRQ_EINT(1),NULL);
err1:
	unregister_chrdev_region(devno,number_of_device);
	return ret;
}

/*模块卸载函数*/
static void __exit mhw_key_exit(void)
{
	dev_t devno = MKDEV(key_major,key_minor);

	free_irq(IRQ_EINT(7),NULL);
	free_irq(IRQ_EINT(6),NULL);
	free_irq(IRQ_EINT(4),NULL);
	free_irq(IRQ_EINT(3),NULL);
	free_irq(IRQ_EINT(2),NULL);
	free_irq(IRQ_EINT(1),NULL);
	cdev_del(&cdev);
	unregister_chrdev_region(devno,number_of_device);

	printk(KERN_INFO "************mhw_key_driver end************\n");
}

module_init(mhw_key_init);
module_exit(mhw_key_exit);

