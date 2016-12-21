/*
 * hello.c
 *
 * Simple hello world 2.6 driver module with MODULE_ macros
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

#define DRIVER_AUTHOR "Foo bar"
#define DRIVER_DESC   "A sample driver"

MODULE_LICENSE ("GPL");
MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_DESCRIPTION (DRIVER_DESC);
MODULE_SUPPORTED_DEVICE ("TestDevice");

static int __init hello_2_init (void)
{
	printk (KERN_INFO "Hello world\n");
	return 0;
}

static void __exit hello_2_exit (void)
{
	printk (KERN_INFO "Goodbye world\n");
}

module_init (hello_2_init);
module_exit (hello_2_exit);
