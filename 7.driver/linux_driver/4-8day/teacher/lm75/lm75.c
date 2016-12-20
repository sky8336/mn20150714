#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

static int lm75_major = 250;
static int lm75_minor = 0;
static int num = 1;

static struct cdev cdev;

struct i2c_client *lm75_client;

static int lm75_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int lm75_release(struct inode *inode, struct file *file)
{
	return 0;
}
	
static ssize_t lm75_read(struct file *file, char __user *buf, size_t count, loff_t *loff)
{
	char buf1[1] = {0};
	char buf2[2];

	struct i2c_msg msg[2] = {
		{lm75_client->addr, 0, 1, buf1},
		{lm75_client->addr, I2C_M_RD, 2, buf2},
	};

	i2c_transfer(lm75_client->adapter, msg, ARRAY_SIZE(msg));

	if(copy_to_user(buf, buf2, 2))
		return -EINVAL;

	return 2;
}

static struct file_operations lm75_fops = {
	.owner = THIS_MODULE,
	.open = lm75_open,
	.release = lm75_release,
	.read = lm75_read,
};

static int lm75_probe(struct i2c_client *client, 
		const struct i2c_device_id *id)
{
	int ret; 

	dev_t devno = MKDEV(lm75_major,lm75_minor);

	ret = register_chrdev_region(devno, num, "lm75");
	if(ret < 0)	
		return ret;

	cdev_init(&cdev, &lm75_fops);
	cdev.owner = THIS_MODULE;
	cdev_add(&cdev, devno, num);

	lm75_client = client;

	printk("client->name = %s\n", client->name);
	return 0;
}

static int lm75_remove(struct i2c_client *client)
{
	dev_t devno = MKDEV(lm75_major,lm75_minor);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, num);
	return 0;
}

static const struct i2c_device_id lm75_i2c_id[] = {
	{"lm75", 0},
};

static struct i2c_driver lm75_driver = {
	.driver = {
		.name = "lm75",
		.owner = THIS_MODULE,
	},
	.probe = lm75_probe,
	.remove = lm75_remove,
	.id_table = lm75_i2c_id,
}; 

static int lm75_init(void)
{
	return i2c_add_driver(&lm75_driver);
}

static void lm75_exit(void)
{
	i2c_del_driver(&lm75_driver);
}

module_init(lm75_init);
module_exit(lm75_exit);
