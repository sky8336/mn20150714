/*
 * char-reg.c
 *
 * Registering a character driver 
 *
 * Copyright (C) 2005 Farsihgt
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

MODULE_LICENSE ("GPL");

int hello_major = 250;
int hello_minor = 0;
int number_of_devices = 1;

static int __init hello_2_init (void)
{
  int result;
  dev_t dev = 0;

  dev = MKDEV (hello_major, hello_minor);
  result = register_chrdev_region (dev, number_of_devices, "hello");
  if (result<0) {
    printk (KERN_WARNING "hello: can't get major number %d\n", hello_major);
    return result;
  }

  printk (KERN_INFO "Registered character driver\n");
  return 0;
}

static void __exit hello_2_exit (void)
{
  dev_t devno = MKDEV (hello_major, hello_minor);

  unregister_chrdev_region (devno, number_of_devices);

  printk (KERN_INFO "Char driver cleaned up\n");
}

module_init (hello_2_init);
module_exit (hello_2_exit);
