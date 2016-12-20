#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf6628fc9, "module_layout" },
	{ 0xd6a968fe, "cdev_del" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x6395be94, "__init_waitqueue_head" },
	{ 0x2b140f5e, "cdev_add" },
	{ 0x88dc55cf, "cdev_init" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xfa66f77c, "finish_wait" },
	{ 0x5c8b5ce8, "prepare_to_wait" },
	{ 0x1000e51, "schedule" },
	{ 0xf5d0b8cb, "current_task" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x4f8b5ddb, "_copy_to_user" },
	{ 0x831197f4, "kill_fasync" },
	{ 0xcf21d241, "__wake_up" },
	{ 0x4f6b400b, "_copy_from_user" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x99b806d4, "fasync_helper" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "6B94AE080016D60529740A8");
