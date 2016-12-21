#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>      //wait_event_interruptible的头文件
#include <linux/interrupt.h>  //中断相关的头文件  request_irq

#include <asm/io.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");   //许可证申明

static int key_major = 250;   //申请主次设备号
static int key_minor = 0;
static int number_of_device = 1;

static int key = 0xff;     //定义全局变量用于唤醒和给应用程序传递中断号

static struct cdev cdev;   //定义设备结构体

wait_queue_head_t readq;   //全局定义等待队列头

/*异步通知步骤一：定义异步结构体，该结构体内核已经实现，直接调用即可*/
struct fasync_struct *key_fasync;

static int s5pc100_key_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int s5pc100_key_release(struct inode *inode, struct file *file)
{
	return 0;
}

irqreturn_t key_interrupt(int irqno, void *devid)   //中断处理函数，第二个参数被传为NULL 
{
	printk("irqno = %d\n", irqno);   //中断号通过申请中断号时的函数传递到这个函数里

	switch(irqno)      //判断中断号，以确定给应用程序传递什么值
	{
	case IRQ_EINT(1): key = 1; break;
	case IRQ_EINT(2): key = 2; break;
	case IRQ_EINT(3): key = 3; break;
	case IRQ_EINT(4): key = 4; break;
	case IRQ_EINT(6): key = 5; break;
	case IRQ_EINT(7): key = 6; break;
	}

	wake_up_interruptible(&readq);    //可中断唤醒，此时key被赋予了新值，即key=0xff不满足，这个条件可唤醒read中的等待队列中的进程

	if(key_fasync)  //异步通知步骤三：向应用程序发信号，告诉应用程序数据已经发上去了，应用程序捕获到该信号后可在信号处理函数中做相应操作
		kill_fasync(&key_fasync, SIGIO, POLL_IN);

	return IRQ_HANDLED;   //中断处理函数的返回值
}
	
ssize_t s5pc100_key_read(struct file *file, char __user *buf, size_t count, loff_t *loff)
{
	wait_event_interruptible(readq, key != 0xff);   //睡眠，等待唤醒，条件(key != 0xff)满足时唤醒

	if(copy_to_user(buf, (char *)&key, sizeof(key)))   //将key的值传递给应用程序，此处也可以写&key，习惯写法问题
		return -EINVAL;

	key = 0xff;           //中断唤醒后恢复原来的值以便继续睡眠

	return count;
}
	
int s5pc100_key_fasync(int fd, struct file *file, int on)  //异步通知步骤二：实现fasync函数，在函数中直接调用fasync_helper即可
{
	return fasync_helper(fd, file, on, &key_fasync);
}
	
static struct file_operations s5pc100_key_fops = {
	.owner = THIS_MODULE,
	.open = s5pc100_key_open,
	.release = s5pc100_key_release,
	.read = s5pc100_key_read,
	.fasync = s5pc100_key_fasync,  //异步通知步骤二：实现file_operations中的fasync
};

static int s5pc100_key_init(void)    //加载函数
{
	int ret;

	dev_t devno = MKDEV(key_major, key_minor);
	ret = register_chrdev_region(devno, number_of_device, "s5pc100-key"); 
	if(ret < 0)
	{
		printk("failed register_chrdev_region\n");
		return ret;
	}

	cdev_init(&cdev, &s5pc100_key_fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_device);
	if(ret < 0)
	{
		printk("failed cdev_add\n");
		goto err1;
	}

	init_waitqueue_head(&readq);     //初始化等待队列头，放在request_irq前面，若放在后面，则有可能还未初始化等待队列头就产生了中断

	/*设备初始化*/
	/*申请中断号，参数一：中断号，外部中断号为IRQ_EINT(x)  x=0~31
	 *            参数二：中断处理函数，此时会将中断号传给中断处理函数
	 *            参数三：中断类型 | 中断触发方式，此处为下降沿触发
	 *            参数四：名字，队中端本身意义不大，给外部人看
	 *            参数五：此参数会通过中断处理函数的第二个参数传递到中断处理程序中，不传时是写NULL即可
	 *            使用时必须判断返回值，最后要释放中断号
	 *            该函数的详细信息见第三天文件夹
	 *            该函数实现了对寄存器以及硬件的所有操作，故不再操作寄存器*/
	ret = request_irq(IRQ_EINT(1), key_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "key1", NULL);
	if(ret < 0)
		goto err2;
	ret = request_irq(IRQ_EINT(2), key_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "key2", NULL);
	if(ret < 0)
		goto err3;
	ret = request_irq(IRQ_EINT(3), key_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "key3", NULL);
	if(ret < 0)
		goto err4;
	ret = request_irq(IRQ_EINT(4), key_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "key4", NULL);
	if(ret < 0)
		goto err5;
	ret = request_irq(IRQ_EINT(6), key_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "key5", NULL);
	if(ret < 0)
		goto err6;
	ret = request_irq(IRQ_EINT(7), key_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, "key6", NULL);
	if(ret < 0)  //申请不成功时只需依次释放前一个已经申请成功的中断号，本次申请未成功，故不管
		goto err7;
	return 0;
err7:
	free_irq(IRQ_EINT(6), NULL);
err6:
	free_irq(IRQ_EINT(4), NULL);
err5:
	free_irq(IRQ_EINT(3), NULL);
err4:
	free_irq(IRQ_EINT(2), NULL);
err3:
	free_irq(IRQ_EINT(1), NULL);
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno, number_of_device);
	return ret;
}

static void s5pc100_key_exit(void)   //卸载函数
{
	dev_t devno = MKDEV(key_major, key_minor);

	free_irq(IRQ_EINT(1), NULL);
	free_irq(IRQ_EINT(2), NULL);
	free_irq(IRQ_EINT(3), NULL);
	free_irq(IRQ_EINT(4), NULL);
	free_irq(IRQ_EINT(6), NULL);
	free_irq(IRQ_EINT(7), NULL);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, number_of_device);
}


module_init(s5pc100_key_init);
module_exit(s5pc100_key_exit);
