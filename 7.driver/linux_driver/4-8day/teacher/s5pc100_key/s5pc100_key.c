#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

static int key_major = 250;
static int key_minor = 0;
static int number_of_device = 1;

static int key = 0xff;

static struct cdev cdev;

wait_queue_head_t readq;

struct fasync_struct *key_fasync;

struct timer_list key_timer;

static int s5pc100_key_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int s5pc100_key_release(struct inode *inode, struct file *file)
{
	return 0;
}

irqreturn_t key_interrupt(int irqno, void *devid)
{
	printk("irqno = %d\n", irqno);

	disable_irq_nosync(irqno);
	switch(irqno)
	{
	case IRQ_EINT(1): key = 1; break;
	case IRQ_EINT(2): key = 2; break;
	case IRQ_EINT(3): key = 3; break;
	case IRQ_EINT(4): key = 4; break;
	case IRQ_EINT(6): key = 5; break;
	case IRQ_EINT(7): key = 6; break;
	}

	wake_up_interruptible(&readq);

	if(key_fasync)
		kill_fasync(&key_fasync, SIGIO, POLL_IN);

	key_timer.data = irqno;
	mod_timer(&key_timer, jiffies + 10);

	return IRQ_HANDLED;
}
	
ssize_t s5pc100_key_read(struct file *file, char __user *buf, size_t count, loff_t *loff)
{
	wait_event_interruptible(readq, key != 0xff);

	if(copy_to_user(buf, (char *)&key, sizeof(key)))
		return -EINVAL;

	key = 0xff;

	return count;
}
	
int s5pc100_key_fasync(int fd, struct file *file, int on)
{
	return fasync_helper(fd, file, on, &key_fasync);
}

void s5pc100_key_timeout(unsigned long arg)
{
	printk("arg = %ld\n", arg);
	enable_irq(arg);
}
	
static struct file_operations s5pc100_key_fops = {
	.owner = THIS_MODULE,
	.open = s5pc100_key_open,
	.release = s5pc100_key_release,
	.read = s5pc100_key_read,
	.fasync = s5pc100_key_fasync,
};

static int s5pc100_key_init(void)
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

	init_waitqueue_head(&readq);
	init_timer(&key_timer);
	key_timer.function = s5pc100_key_timeout; 

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
	if(ret < 0)
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

static void s5pc100_key_exit(void)
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
