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
	{ 0x98397cc5, "module_layout" },
	{ 0x3ec8886f, "param_ops_int" },
	{ 0x37a0cba, "kfree" },
	{ 0x47c4a86d, "cdev_del" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x8c71be19, "cdev_add" },
	{ 0x9f4ce64, "cdev_init" },
	{ 0x156b0a42, "kmem_cache_alloc_trace" },
	{ 0x992847d2, "kmalloc_caches" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x50eedeb8, "printk" },
	{ 0x8834396c, "mod_timer" },
	{ 0xbe2c0274, "add_timer" },
	{ 0x7d11c268, "jiffies" },
	{ 0xfb0e29f, "init_timer_key" },
	{ 0xc996d097, "del_timer" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "9D43218FFA5D7942E1D9EE3");
