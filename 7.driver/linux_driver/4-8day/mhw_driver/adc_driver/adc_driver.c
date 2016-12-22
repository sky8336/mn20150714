#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

/*定义映射的偏移值*/
#define ADCCON     0x00
#define ADCDAT0    0x0c
#define ADCMUX     0x1c
#define ADCCLRINT  0x18

MODULE_LICENSE("Dual BSD/GPL");

/*主次设备号定义*/
static int adc_major = 300;
static int adc_minor = 0;
static int number_of_device = 1;

/*定义字符设备控制块*/
static struct cdev cdev;  

/*定义时钟的结构体*/
struct clk *clk;

/*寄存器映射指针定义，存放映射后的虚拟地址*/
void __iomem *ADC_BASE;

/*定义存放获得资源的地址的结构体*/
struct resource *mem_resource,*irq_resource;

/*定义等待队列头*/
wait_queue_head_t adc_queue;

/*定义用于睡眠唤醒的条件变量*/
static int flags = 0;

/*open  release  read函数的实现*/
static int adc_open(struct inode *inode,struct file *file)
{
	printk("call adc_open\n");
	return 0;
}
static int adc_release(struct inode *inode,struct file *file)
{
	printk("call adc_release\n");
	return 0;
}
ssize_t adc_read(struct file *file,char __user *buf,size_t count,loff_t *loff)
{
	static int data;
	/*ADC转换完成后会发中断信号IRQ_ADC，cpu处理中断，在中断处理函数中唤醒等待队列*/
	writel((readl(ADC_BASE + ADCCON) & ~(0x3 << 0)) | (0x1 << 0),ADC_BASE + ADCCON); 
	wait_event_interruptible(adc_queue,flags != 0);
	data = readl(ADC_BASE + ADCDAT0) & 0xfff;
	if(copy_to_user(buf,&data,count))
		return -EFAULT;

	flags = 0;
	return 0;
}

/*对设备的操作接口*/
static struct file_operations adc_fops = {
	.owner = THIS_MODULE,
	.open = adc_open,
	.release = adc_release,
	.read = adc_read,
};

/*中断处理函数的实现*/
irqreturn_t adc_interrupt_handler(int irqno,void *devid)
{
	flags = 1;
	wake_up_interruptible(&adc_queue); //唤醒等待队列
	writel(0x1,ADC_BASE + ADCCLRINT);  //内部中断为电平触发，故需要清中断，否则会该电平一直会存在，一直会触发中断

	return IRQ_HANDLED;
}

/*probe函数的实现*/
static int adc_probe(struct platform_device *pdev)
{
	/*申请设备号并注册设备*/
	int ret;

	dev_t devno = MKDEV(adc_major,adc_minor);
	if((ret = register_chrdev_region(devno,number_of_device,"mhw_adc_driver")) < 0)
	{
		printk("Failed from register_chrdev_region\n");
		return ret;
	}
	cdev_init(&cdev,&adc_fops);
	cdev.owner = THIS_MODULE;
	if((ret = cdev_add(&cdev,devno,number_of_device)) < 0)
	{
		printk("Failed from cdev_add\n");
		goto err1;
	}

	/*初始化等待队列头*/
	init_waitqueue_head(&adc_queue);
	/*设备初始化一：打开时钟*/
    clk = clk_get(NULL,"adc");  //获得时钟
	clk_enable(clk);           //打开时钟
	/*设备初始化二：获得资源*/
	mem_resource = platform_get_resource(pdev,IORESOURCE_MEM,0);
	irq_resource = platform_get_resource(pdev,IORESOURCE_IRQ,0);
	/*设备初始化三：寄存器映射及初始化*/
	ADC_BASE = ioremap(mem_resource->start,mem_resource->end - mem_resource->start);  //寄存器映射
	if(ADC_BASE == NULL)
	{
		ret = -EINVAL;
		goto err2;
	}
	writel(readl(ADC_BASE + ADCMUX) & ~0xf,ADC_BASE + ADCMUX);
	writel((readl(ADC_BASE + ADCCON) & ~0x1FFFF) | 1 << 0 | 1 << 14 | 0xff << 6 | 1 << 16,ADC_BASE + ADCCON);
	writel(readl(ADC_BASE + ADCDAT0) & 0xfff, ADC_BASE + ADCDAT0);
	writel(readl(ADC_BASE + ADCCLRINT) | (1 << 0),ADC_BASE + ADCCLRINT);
	/*注册中断
	 *     ADC为内部中断，中断号为IRQ_ADC
	 *     内部中断为电平触发*/
	ret = request_irq(IRQ_ADC,adc_interrupt_handler,IRQF_DISABLED,"adc",NULL);
	if(ret != 0)
	{
		printk("Failed from request_irq\n");
		goto err2;
	}

	printk("platform: match ok\n");
	return 0;
err2:
	cdev_del(&cdev);
err1:
	unregister_chrdev_region(devno,number_of_device);
	return ret;
}

/*remove函数的实现*/
static int adc_remove(struct platform_device *pdev)
{
	dev_t devno =MKDEV(adc_major,adc_minor);

	clk_disable(clk);//关闭时钟
	clk_put(clk);    //释放时钟
	cdev_del(&cdev);
	unregister_chrdev_region(devno,number_of_device);
	printk("platform: driver remove\n");
	return 0;
}

/*定义platform_driver结构体*/
struct platform_driver adc_driver = {
	.driver = {
		.name = "s5pc100-adc"
	},
	.probe = adc_probe,
	.remove = adc_remove,
};

/*加载函数*/
static int __init mhw_adc_init(void)
{
	/*注册platform_driver结构体*/
	platform_driver_register(&adc_driver);
	printk("platform: driver installed\n");
	return 0;
}

/*卸载函数*/
static void __exit mhw_adc_exit(void)
{
	/*注销platform_driver结构体*/
	platform_driver_unregister(&adc_driver);
	printk("platform: driver uninstalled\n");
}

/*加载函数和卸载函数的申明*/
module_init(mhw_adc_init);
module_exit(mhw_adc_exit);
