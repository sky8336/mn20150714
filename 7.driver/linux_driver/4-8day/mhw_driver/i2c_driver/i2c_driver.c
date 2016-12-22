#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

MODULE_LICENSE("GPL");

/*主次设备号定义*/
static int i2c_major = 300;
static int i2c_minor = 0;
static int number_of_device = 1;

/*定义字符设备控制块*/
static struct cdev cdev;

/*定义一个全局指针接收局部指针*/
struct i2c_client *mhw_client;

/*open release read函数的实现*/
static int i2c_open(struct inode *inode,struct file *file)
{
	printk("call i2c_open\n");
	return 0;
}
static int i2c_release(struct inode *inode,struct file *file)
{
	printk("call i2c_release\n");
	return 0;
}
ssize_t i2c_read(struct file *file,char __user *buf,size_t count,loff_t *loff)
{
	char buf1[1] = {0};
	char buf2[2];
	struct i2c_msg msg[2] = {
		[0] = {
			.addr = mhw_client->addr,
			.flags = 0,
			.len = 1,
			.buf = buf1,
		},
		[1] = {
			.addr = mhw_client->addr,
			.flags = I2C_M_RD,
			.len = 2,
			.buf = buf2,
		},
	};

	i2c_transfer(mhw_client->adapter,msg,ARRAY_SIZE(msg));
	if(copy_to_user(buf,buf2,sizeof(buf)))
		return -EINVAL;

	return count;
}

/*定义file_operations结构体*/
static struct file_operations i2c_fops = {
	.owner = THIS_MODULE,
	.open = i2c_open,
	.release = i2c_release,
	.read = i2c_read,
};

/*probe函数的实现*/
static int i2c_probe(struct i2c_client *lm75_client,const struct i2c_device_id *lm75_id)
{
	int ret;

	dev_t devno = MKDEV(i2c_major,i2c_minor);
	if((ret = register_chrdev_region(devno,number_of_device,"mhw_i2c_driver")) < 0)
	{
		printk("Failed from register_chrdev_region\n");
		return ret;
	}
	
	cdev_init(&cdev,&i2c_fops);
	cdev.owner = THIS_MODULE;
	if((ret = cdev_add(&cdev,devno,number_of_device)) < 0)
	{
		printk("Failed fron adev_add\n");
		goto err1;
	}
	
	/*局部指针变成全局指针*/
	mhw_client = lm75_client;

	printk("i2c_driver: match ok\n");
	return 0;
err1:
	unregister_chrdev_region(devno,number_of_device);
	return ret;
}

/*remove函数的实现*/
static int i2c_remove(struct i2c_client *lm75_client)
{
	dev_t devno = MKDEV(i2c_major,i2c_minor);

	cdev_del(&cdev);
	unregister_chrdev_region(devno,number_of_device);
	
	printk("i2c_driver: driver remove\n");
	return 0;
}

/*和i2c_board_info中的名字去匹配*/
static const struct i2c_device_id lm75_i2c_id[] = {
	{"lm75",0},
};

/*定义i2c_driver结构体*/
static struct i2c_driver lm75_driver = {
	.driver.name = "lm75",
	.probe = i2c_probe,
	.remove = i2c_remove,
	.id_table = lm75_i2c_id,
};

/*加载函数*/
static int __init mhw_i2c_init(void)
{
	/*注册i2c_driver结构体*/
	i2c_add_driver(&lm75_driver);
	printk("i2c_driver installed\n");
	return 0;
}

/*卸载函数*/
static void __exit mhw_i2c_exit(void)
{
	/*注销i2c_driver结构体*/
	i2c_del_driver(&lm75_driver);
	printk("i2c_driver uninstalled\n");
}

/*加载函数和卸载函数的申明*/
module_init(mhw_i2c_init);
module_exit(mhw_i2c_exit);
