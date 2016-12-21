/*
 * char-read.c
 *
 * Character driver with read operation
 *
 * Copyright (C) 2005 Farsight
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

MODULE_LICENSE ("GPL");

int hello_major = 250;
int hello_minor = 0;
int number_of_devices = 1;
char data[128]="foobar not equal to barfoo";

struct cdev cdev;
dev_t dev = 0;


static int hello_open (struct inode *inode, struct file *file)
{
  printk (KERN_INFO "Hey! device opened\n");
  return 0;
}

static int hello_release (struct inode *inode, struct file *file)
{
  printk (KERN_INFO "Hmmm... device closed\n");
  return 0;
}
ssize_t hello_read (struct file *filp, char *buff, size_t count, loff_t *offp)
{
  ssize_t result = 0;
  if(count > 127) count = 127;
  if(count < 0) return -EINVAL;
  
  if (copy_to_user (buff, data, count)) 
    result = -EFAULT;
  else 
    printk (KERN_INFO "read %d bytes\n", count);
  result = count;

   return result;
}

ssize_t hello_write (struct file *filp, const char  *buf, size_t count, loff_t *f_pos)
{
  ssize_t ret = 0;
  printk (KERN_INFO "Writing %d bytes\n", count);
  if (count>127) return -ENOMEM;
  if (count<0) return -EINVAL;
  if (copy_from_user (data, buf, count)) {
    ret = -EFAULT;
  }
  else {
    data[127]='\0';
    printk (KERN_INFO"Received: %s\n", data);
    ret = count;
  }
  return ret;
}


struct file_operations hello_fops = {
  .owner = THIS_MODULE,
  .open  = hello_open,
  .release = hello_release,
  .read  = hello_read,
  .write = hello_write
};

static void char_reg_setup_cdev (void)
{
  int error, devno = MKDEV (hello_major, hello_minor);
  cdev_init (&cdev, &hello_fops);
  cdev.owner = THIS_MODULE;
  cdev.ops = &hello_fops;
  error = cdev_add (&cdev, devno , 1);
  if (error)
    printk (KERN_NOTICE "Error %d adding char_reg_setup_cdev", error);

}

static int __init hello_2_init (void)
{
  int result;

  dev = MKDEV (hello_major, hello_minor);
  result = register_chrdev_region (dev, number_of_devices, "hello");

  if (result<0) {
    printk (KERN_WARNING "hello: can't get major number %d\n", hello_major);
    return result;
  }

  char_reg_setup_cdev ();
  printk (KERN_INFO "char device registered\n");
  return 0;
}

static void __exit hello_2_exit (void)
{
  dev_t devno = MKDEV (hello_major, hello_minor);
  
  cdev_del (&cdev);

  unregister_chrdev_region (devno, number_of_devices);

}

module_init (hello_2_init);
module_exit (hello_2_exit);
