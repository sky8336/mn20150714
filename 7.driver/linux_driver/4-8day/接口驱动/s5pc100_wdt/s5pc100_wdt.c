#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <plat/regs-watchdog.h>
#include <plat/map-base.h>
#include <linux/clk.h>

#define WATCHDOG_MAGIC 'k'  
#define FEED_DOG _IO(WATCHDOG_MAGIC,1)

#define WATCHDOG_MAJOR 256
#define DEVICE_NAME "s3c2410_watchdog"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("farsight");
MODULE_DESCRIPTION("s5pc100 Watchdog");

#define S5PC100_PA_WTCON 0xEA200000
#define S5PC100_PA_WTDAT 0xEA200004
#define S5PC100_PA_WTCNT 0xEA200008

unsigned int *S5PC100_WTCON;
unsigned int *S5PC100_WTCNT;
unsigned int *S5PC100_WTDAT;

struct clk		*clk;

static int watchdog_major = WATCHDOG_MAJOR;

static struct cdev watchdog_cdev;

static int watchdog_open(struct inode *inode ,struct file *file)	
{
	int ret;
	clk = clk_get(NULL, "watchdog");
	if (IS_ERR(clk)) {
		printk("failed to get watchdog clock\n");
		ret = PTR_ERR(clk);
		return ret;
	}

	clk_enable(clk);

	writel(0, S5PC100_WTCON); //�رտ��Ź�
	writel(0x8000, S5PC100_WTCNT);//���ü����Ĵ���������������ݵݼ�Ϊ0ʱ������㻹û��������������ϵͳ����������
	writel(0x8000, S5PC100_WTDAT);//���ݼĴ�����Ҫ�ǰ�����װ�ڵ������Ĵ�������ȥ��

	writel(S3C2410_WTCON_ENABLE|S3C2410_WTCON_DIV64|S3C2410_WTCON_RSTEN |
		     S3C2410_WTCON_PRESCALE(0x80), S5PC100_WTCON);//���ÿ��ƼĴ��������ԣ�����ο�datasheet����Ҫ�ǵõ�t_watchdogʱ�����ڡ�

	printk("S5PC100_WTCON = %x\n", readl(S5PC100_WTCON));

	printk(KERN_NOTICE"open the watchdog now!\n");
	return 0;
}


static int watchdog_release(struct inode *inode,struct file *file)
{
	clk_disable(clk);
	return 0;
}
//ι��
static int watchdog_ioctl(struct inode *inode,struct file *file,unsigned int cmd,unsigned long arg)

{	
	switch(cmd)
		{
			case FEED_DOG:
				writel(0x8000,S5PC100_WTCNT); //�������������Ĵ�����дֵ���������ݼ�Ϊ0ʱ��ϵͳ����������
				printk("S5PC100_WTCNT = %d\n", readl(S5PC100_WTCNT));
				break;
		}
	return 0;	
}
//���豸ע�ᵽϵͳ֮��
static void watchdog_setup_dev(struct cdev *dev,int minor,struct file_operations *fops)
{
	int err;
	int devno=MKDEV(watchdog_major,minor);
	cdev_init(dev,fops); 
	dev->owner=THIS_MODULE;
	dev->ops=fops; 
	err=cdev_add(dev,devno,1);
	if(err)
	printk(KERN_INFO"Error %d adding watchdog %d\n",err,minor);
}

static struct file_operations watchdog_remap_ops={
	.owner=THIS_MODULE,
	.open=watchdog_open, 
	.release=watchdog_release, 
	.ioctl=watchdog_ioctl,
};
//ע���豸����������Ҫ������豸�ŵ�ע��
static int __init s3c2410_watchdog_init(void)
{
	int result;
	
	dev_t dev = MKDEV(watchdog_major,0);
	
	if(watchdog_major)
		result = register_chrdev_region(dev,1,DEVICE_NAME);
	else
	{	
		result = alloc_chrdev_region(&dev,0,1,DEVICE_NAME);
		watchdog_major = MAJOR(dev);
	}
	
	if(result<0)
	{
		 printk(KERN_WARNING"watchdog:unable to get major %d\n",watchdog_major);		
	  	 return result;
	}
	if(watchdog_major == 0)
		watchdog_major = result;
	printk(KERN_NOTICE"[DEBUG] watchdog device major is %d\n",watchdog_major);
	watchdog_setup_dev(&watchdog_cdev,0,&watchdog_remap_ops);

	S5PC100_WTCON = ioremap(S5PC100_PA_WTCON, 4);
	S5PC100_WTCNT = ioremap(S5PC100_PA_WTCNT, 4);
	S5PC100_WTDAT = ioremap(S5PC100_PA_WTDAT, 4);
	
	return 0;
}
//����ģ��ж��
static void s3c2410_watchdog_exit(void)
{
	iounmap(S5PC100_WTCON);
	iounmap(S5PC100_WTCNT);
	iounmap(S5PC100_WTDAT);
	cdev_del(&watchdog_cdev);
	unregister_chrdev_region(MKDEV(watchdog_major,0),1);
	printk("watchdog device uninstalled\n");
}

module_init(s3c2410_watchdog_init);
module_exit(s3c2410_watchdog_exit);
