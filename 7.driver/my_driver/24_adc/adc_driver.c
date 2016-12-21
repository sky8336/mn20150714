#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/clk.h>

#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
MODULE_LICENSE("GPL");

#define ADC_MUTEX 0X1C
#define ADC_CLEAR 0X18
#define ADC_DATA0 0X0C
#define ADC_CON   0X00

int major = 250;
int minor = 0;

	
static struct cdev cdev;
int flag = 1;
static wait_queue_head_t readq;

static struct clk *clk_t;


//static unsigned long *adccon;

void __iomem *adccon ;
struct resource *irqres ;

irqreturn_t adc_handler(int irqno,void *arg)
{
	writel(0,adccon + ADC_CLEAR);
	flag = 0;
	wake_up(&readq);

	return IRQ_HANDLED;

}

static int adc_open (struct inode *inode, struct file *filp)
{
	printk("adc_open\n");
	return 0;
}
static int adc_release (struct inode *inode, struct file *filp)
{
	printk("adc_release\n");
	return 0;
}

static ssize_t adc_read(struct file *file, char __user *buff, size_t size, loff_t *loff)
{
	int data;
	int ret;

	writel(readl(adccon) | (0x1 << 0),adccon);

	wait_event(readq,flag != 1);

	data = readl(adccon + ADC_DATA0)  & 0xfff;
	ret = copy_to_user(buff,(void *)&data,sizeof(int));
	flag = 1;

	return size;

}
static struct file_operations adc_fops = {
	.open = adc_open,
	.release = adc_release,
	.read = adc_read,


};


static int s5pc100_adc_probe(struct platform_device *pdev)
{

	dev_t devno = MKDEV(major,minor);
	//	void __iomem *adccon ;
	static int ret;

	struct resource *memres = platform_get_resource(pdev,IORESOURCE_MEM,0);
	irqres = platform_get_resource(pdev,IORESOURCE_IRQ,0);

	if(memres == NULL || irqres == NULL)
		return -ENOMEM;



	adccon = ioremap(memres->start,memres->end - memres->start);
	if(adccon == NULL)
		return -EFAULT;

	ret = request_irq(irqres->start,adc_handler,IRQF_DISABLED,"ADC",NULL);
	if(ret != 0){
		goto err1;
	}

	clk_t = clk_get(NULL,"adc");
	ret = clk_enable(clk_t);
	if(ret != 0)
		goto err2;

	writel(0,ADC_MUTEX + adccon);
	writel(1 << 16 | 1 << 14 | 0xff << 6,adccon);


	ret = register_chrdev_region(devno,1,"s5pc100-adc");
	if(ret != 0)
		goto err3;

	cdev_init(&cdev,&adc_fops);
	ret = cdev_add(&cdev,devno,1);
	if(ret != 0)
		goto err4;

	init_waitqueue_head(&readq);

	printk("mach ok\n");

	return 0;


err4:
	unregister_chrdev_region(devno,1);
err3:
	clk_disable(clk_t);
err2:
	free_irq(irqres->start,"ADC");
err1:
	iounmap(adccon);


	return ret;
}
static int s5pc100_adc_remove(struct platform_device *dev)
{
	dev_t devno = MKDEV(major,minor);

	cdev_del(&cdev);
	unregister_chrdev_region(devno,1);

	clk_disable(clk_t);
	free_irq(irqres->start,"ADC");
	iounmap(adccon);


	printk("hello_remove\n");

	return 0;
}

static struct platform_driver s5pc100_adc = {
	.driver.name ="s5pc100-adc",
	.probe = s5pc100_adc_probe,
	.remove = s5pc100_adc_remove,

};

static int hello_init(void)
{
	printk("hello_init\n");
	return platform_driver_register(&s5pc100_adc);
}

static void hello_exit(void)
{

	printk("hello_exit\n");
	platform_driver_register(&s5pc100_adc);
}

module_init(hello_init);
module_exit(hello_exit);





