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
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x6c075966, "device_create" },
	{ 0xdc5e7ab3, "__class_create" },
	{ 0x2b140f5e, "cdev_add" },
	{ 0x88dc55cf, "cdev_init" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x4f8b5ddb, "_copy_to_user" },
	{ 0x4f6b400b, "_copy_from_user" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "CF0773D43BF5BCCF642C6BF");
