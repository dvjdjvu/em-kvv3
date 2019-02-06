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
	{ 0x7d09bae2, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xadf42bd5, "__request_region" },
	{ 0x27a3b3fa, "kmalloc_caches" },
	{ 0x6980fe91, "param_get_int" },
	{ 0x69ec12c1, "send_sig" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x5aa3f103, "__register_chrdev" },
	{ 0x167e7f9d, "__get_user_1" },
	{ 0xff964b25, "param_set_int" },
	{ 0x7d11c268, "jiffies" },
	{ 0x9629486a, "per_cpu__cpu_number" },
	{ 0x41344088, "param_get_charp" },
	{ 0x59d8223a, "ioport_resource" },
	{ 0xb72397d5, "printk" },
	{ 0xacdeb154, "__tracepoint_module_get" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0x45450063, "mod_timer" },
	{ 0xfda85a7d, "request_threaded_irq" },
	{ 0x52069f0, "module_put" },
	{ 0x75749999, "init_task" },
	{ 0xb8aa2342, "__check_region" },
	{ 0x9dd454a7, "kmem_cache_alloc" },
	{ 0x87b67164, "pv_cpu_ops" },
	{ 0x9bce482f, "__release_region" },
	{ 0x7ecb001b, "__per_cpu_offset" },
	{ 0x6ad065f4, "param_set_charp" },
	{ 0x37a0cba, "kfree" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "B16E24FC7CAC3F1104A0904");
