#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#define S5PC100_ADCCON  	0x00
#define S5PC100_ADCDAT0  	0x0C
#define S5PC100_ADCCLRINT  	0x18
#define S5PC100_ADCMUX  	0x1C

MODULE_LICENSE("GPL");

static struct resource *mem_resource, *irq_resource;
static void __iomem *adc_base;
static struct clk *clk;

static int adc_major = 250;
static int adc_minor = 0;
static int num_of_device = 1;

static struct cdev cdev;

wait_queue_head_t readq;
static int flags = 0;

static int s5pc100_adc_open(struct inode *inode, struct file *file)
{
	return 0;
}
	
static int s5pc100_adc_release(struct inode *inode, struct file *file)
{
	return 0;
}
	
static ssize_t s5pc100_adc_read(struct file *file, char __user *buf, size_t count, loff_t *loff)
{
	int data;

	writel(0, adc_base + S5PC100_ADCMUX);
	writel(1 << 0 | 1<< 14 | 0xff << 6 | 1 << 16, adc_base + S5PC100_ADCCON);
	wait_event_interruptible(readq, flags != 0);

	data = readl(adc_base + S5PC100_ADCDAT0) & 0xfff;
	flags = 0;

	if(copy_to_user(buf, (char *)&data, sizeof(data)))
		return -EINVAL;

	return sizeof(data);
}

static struct file_operations s5pc100_adc_fops = {
	.owner = THIS_MODULE,
	.open = s5pc100_adc_open,
	.release = s5pc100_adc_release,
	.read = s5pc100_adc_read
};

static irqreturn_t adc_interrupt(int irqno, void *devid)
{
	flags = 1;
	wake_up_interruptible(&readq);
	writel(0, adc_base + S5PC100_ADCCLRINT);
	return IRQ_HANDLED;
}

static int s5pc100_adc_probe(struct platform_device *pdev)
{
	int ret;
	dev_t devno = MKDEV(adc_major, adc_minor);

	init_waitqueue_head(&readq);

	mem_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq_resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if((mem_resource == NULL) || (irq_resource == NULL))
		return -ENODEV;

	adc_base = ioremap(mem_resource->start, mem_resource->end - mem_resource->start);
	if(adc_base == NULL)
		return -ENOMEM;

	ret =request_irq(irq_resource->start, adc_interrupt, IRQF_DISABLED, "adc", NULL);
	if(ret < 0)
		goto err1;

	clk = clk_get(NULL, "adc");
	if(IS_ERR(clk))
	{
		dev_err(&pdev->dev, "cannot get clock\n");
		ret = -ENOENT;
		goto err2;
	}
	clk_enable(clk);

	ret = register_chrdev_region(devno, num_of_device, "s5pc100-adc");
	if(ret < 0)
		goto err3;

	cdev_init(&cdev, &s5pc100_adc_fops);
	cdev.owner = THIS_MODULE;
	cdev_add(&cdev, devno, num_of_device);

	printk("match ok\n");
	return 0;
err3:
	clk_disable(clk);
	clk_put(clk);
err2:
	free_irq(irq_resource->start, NULL);
err1:
	iounmap(adc_base);
	return ret;
}

static int s5pc100_adc_remove(struct platform_device *pdev)
{
	dev_t devno = MKDEV(adc_major, adc_minor);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, num_of_device);
	clk_disable(clk);
	clk_put(clk);
	free_irq(irq_resource->start, NULL);
	iounmap(adc_base);
	return 0;
}

static struct platform_driver s5pc100_adc_driver = {
	.driver.name = "s5pc100-adc",
	.probe = s5pc100_adc_probe,
	.remove = s5pc100_adc_remove,
};

static int s5pc100_adc_init(void)
{
	return platform_driver_register(&s5pc100_adc_driver);
}

static void s5pc100_adc_exit(void)
{
	platform_driver_unregister(&s5pc100_adc_driver);
}

module_init(s5pc100_adc_init);
module_exit(s5pc100_adc_exit);
