#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_LICENSE ("GPL");

#define LM75_REG_CONF		0x01
static const u8 LM75_REG_TEMP[3] = {
	0x00,		/* input */
	0x03,		/* max */
	0x02,		/* hyst */
};
struct lm75_data 
{
	u16 temp[3]; /* Register values,
						   0 = input
						   1 = max
						   2 = hyst */
};

static int lm75_major = 250;
static int lm75_minor = 0;
static int number_of_devices = 1;
static dev_t devno = 0;
static struct cdev cdev;
static struct i2c_client *new_client;
struct lm75_data *data;

static int lm75_read_value(struct i2c_client *client, u8 reg)
{
	int value;

	if (reg == LM75_REG_CONF)
		return i2c_smbus_read_byte_data(client, reg);

	value = i2c_smbus_read_word_data(client, reg);
	return (value < 0) ? value : swab16(value);
}

static int lm75_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	if (reg == LM75_REG_CONF)
		return i2c_smbus_write_byte_data(client, reg, value);
	else
		return i2c_smbus_write_word_data(client, reg, swab16(value));
}

static ssize_t lm75_read(struct file *file, char __user *buff, size_t count, loff_t *offset) 
{
	int status, i;
#if 0
	data = 10;
	ch = 0;
	data = s3c_adc_read(client, ch);
	printk("data0 = %d\n", data);

	if(copy_to_user(buff, (char *)&data, sizeof(data)))
		return -EFAULT;
#endif
	for(i = 0; i < ARRAY_SIZE(data->temp); i++)
	{
		status = lm75_read_value(new_client, LM75_REG_TEMP[i]);
		if(status > 0)
		{
			if(i == 0)
			printk("status = %d\n", status >> 8);
		}
	}

	return 0;
}

static int lm75_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int lm75_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations lm75_fops = {
	.owner 	= THIS_MODULE,
	.read	= lm75_read,
	.open 	= lm75_open,
	.release	= lm75_release,
};


static int lm75_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	u8 set_mask, clr_mask;
	int status, new;
	int ret = 0;

	new_client = client;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	data = kzalloc(sizeof(struct lm75_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	set_mask = 0;
	clr_mask = (1 << 0)			/* continuous conversions */
		| (1 << 6) | (1 << 5);		/* 9-bit mode */

	/* configure as specified */
	status = lm75_read_value(client, LM75_REG_CONF);
	if (status < 0) {
		printk("Can't read config? %d\n", status);
		goto err_read_config;
	}
	
	new = status & ~clr_mask;
	new |= set_mask;
	if (status != new)
		lm75_write_value(client, LM75_REG_CONF, new);


	devno = MKDEV(lm75_major, lm75_minor);

	ret = register_chrdev_region(devno, number_of_devices, "lm75");
	if(ret)
	{
		printk("failed to register device number\n");
		goto err_register_chrdev_region;
	}

	cdev_init(&cdev, &lm75_fops);
	cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev, devno, number_of_devices);
	if(ret)
	{
		printk("failed to add device\n");
		goto err_cdev_add;
	}

	return 0;
err_cdev_add:
	unregister_chrdev_region(devno, number_of_devices);
err_register_chrdev_region:
	kfree(data);
err_read_config:

	return ret;
}

static int lm75_remove(struct i2c_client *client)
{
	cdev_del(&cdev);
	unregister_chrdev_region(devno, number_of_devices);
	return 0;
}

enum lm75_type {		/* keep sorted in alphabetical order */
	lm75,
	lm75a,
};

static const struct i2c_device_id lm75_ids[] = {
	{ "lm75", lm75, },
	{ "lm75a", lm75a, },
	{ /* LIST END */ }
};

static struct i2c_driver lm75_driver = {
	.driver = {
		.name = "lm75",
	},
	.probe 		= lm75_probe,
	.remove 	= lm75_remove,
	.id_table	= lm75_ids,
};

static int __init s5pc100_lm75_init(void)
{
	return i2c_add_driver(&lm75_driver);
}

static void __exit s5pc100_lm75_exit(void)
{
	i2c_del_driver(&lm75_driver);
}

module_init(s5pc100_lm75_init);
module_exit(s5pc100_lm75_exit);
