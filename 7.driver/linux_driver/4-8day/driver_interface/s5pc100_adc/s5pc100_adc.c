#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <plat/adc.h>
#include <plat/regs-adc.h>

MODULE_LICENSE ("GPL");

int adc_major = 250;
int adc_minor = 0;
int number_of_devices = 1;
struct s3c_adc_client *client;

struct cdev cdev;
dev_t devno = 0;

static ssize_t adc_convert_read(struct file *file, char __user *buff, size_t count, loff_t *offset) 
{
	unsigned data;
	unsigned ch;
	data = 10;
	ch = 0;
	data = s3c_adc_read(client, ch);
	printk("data0 = %d\n", data);

	if(copy_to_user(buff, (char *)&data, sizeof(data)))
		return -EFAULT;

	return 0;
}

static int adc_convert_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int adc_convert_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations adc_convert_fops = {
	.owner 	= THIS_MODULE,
	.read	= adc_convert_read,
	.open 	= adc_convert_open,
	.release	= adc_convert_release,
};


static int __devinit adc_convert_probe( struct platform_device *pdev )
{
	struct device *dev = &pdev->dev;
	int ret = -EINVAL;

	printk("function = %s\n", __func__);
	devno = MKDEV(adc_major, adc_minor);

	ret = register_chrdev_region(devno, number_of_devices, "adc_convert");
	if( ret )
	{
		dev_err(dev, "failed to register device number\n");
		goto err_register_chrdev_region;
	}

	cdev_init(&cdev, &adc_convert_fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_devices);
	if( ret )
	{
		dev_err(dev, "failed to add device\n");
		goto err_cdev_add;
	}

	client = s3c_adc_register (pdev, NULL, NULL, 0);

	if(IS_ERR( client ))
	{
		dev_err(dev, "failed to register adc client\n");
		goto err_s3c_adc_register;
	}

	return 0;

err_s3c_adc_register:
	cdev_del( &cdev );
err_cdev_add:
	unregister_chrdev_region(devno, number_of_devices);
err_register_chrdev_region:
	return ret; 
}


static int __devexit adc_convert_remove(struct platform_device *pdev)
{
	s3c_adc_release(client);
	cdev_del( &cdev );
	unregister_chrdev_region(devno, number_of_devices);
	return 0;
}

static struct platform_driver adc_convert_driver = {
	.driver = {
		.name 	= "adc_convert",
		.owner 	= THIS_MODULE,
	},
	.probe	= adc_convert_probe,
	.remove = __devexit_p(adc_convert_remove)
};

static int __init adc_convert_init (void)
{
	return platform_driver_register( &adc_convert_driver );
}

static void __exit adc_convert_exit (void) 
{
	platform_driver_unregister( &adc_convert_driver );
}

module_init (adc_convert_init);
module_exit (adc_convert_exit);

