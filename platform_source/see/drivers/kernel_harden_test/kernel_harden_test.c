/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description: Kernel Harden Test Tool
 * Create: 2017-09-01
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/thread_info.h>
#include <asm/sections.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/mm.h>
#include <slab.h>
#endif
#include <linux/slub_def.h>

#define KH_TTY_MAJOR  222
#define KH_TTY_MINORS   1

static struct tty_driver *g_kharden_tty_drv;
static struct tty_port g_kharden_tty_port;

/* use to test post-init read-only memory */
static unsigned int g_test_ro_data __ro_after_init;

/* test cmd */
enum test_cmd_code {
	/* test kernel addr validity for copy_from/to_user */
	TEST_WRAP = 0x00567801,
	TEST_NULL,
	/* test kernel memory boundary for copy_from_user */
	FROM_TEST_SLAB_CACHE = 0x10567801,
	FROM_TEST_STACK_OVERLAP,
	FROM_TEST_TEXT,
	FROM_TEST_NORMAL_ONE,
	FROM_TEST_NORMAL_TWO,
	FROM_TEST_NORMAL_THREE,
	/* test kernel memory boundary for copy_to_user */
	TO_TEST_SLAB_CACHE = 0x20567801,
	TO_TEST_STACK_OVERLAP,
	TO_TEST_TEXT,
	TO_TEST_NORMAL_ONE,
	TO_TEST_NORMAL_TWO,
	TO_TEST_NORMAL_THREE,
	/* test ro_after_init section */
	TEST_RW_WHEN_INIT = 0x30567801,
	TEST_RW_AFTER_INIT,
	/* test direct access userspace for pan */
	TEST_DIRECT_READ_USER = 0x40567801,
	TEST_DIRECT_WRITE_USER,

	/* invalid Command */
	INVALID_COMMAND = 0xffffffff,
};

/* define __check_object_size func when HARDENED USERCOPY disable */
#ifndef CONFIG_HARDENED_USERCOPY
static void __check_object_size(const void *ptr, unsigned long n,
				bool to_user)
{}
#endif

static int usercopy_wrapped_addr_test(unsigned long arg, bool to_user)
{
	void *tp = NULL; /* using void* in order to comply with the kernel */
	unsigned long len;

	tp = kzalloc(sizeof(unsigned int), GFP_KERNEL);
	if (!tp)
		return -1;

	len = 0 - (uintptr_t)tp;
	__check_object_size(tp, len, to_user);

	kfree(tp);

	/* test fail: no bug() by __check_object_size */
	return -1;
}

static int usercopy_zero_ptr_test(unsigned long arg, bool to_user)
{
	void *tp = NULL; /* using void* in order to comply with the kernel */

	tp = kzalloc(0, GFP_KERNEL);
	if (!tp)
		return -1;

	__check_object_size(tp, sizeof(unsigned int), to_user);

	kfree(tp);

	/* test fail: no bug() by __check_object_size */
	return -1;
}

static int usercopy_slab_cache_test(unsigned long arg, bool to_user)
{
	unsigned int *tp = NULL;
	unsigned long len;
	struct kmem_cache *test_cache = NULL;

	test_cache = kmem_cache_create("test_cache", sizeof(unsigned int),
				       0, SLAB_PANIC, NULL);
	if (!test_cache) {
		pr_err("%s: kmem_cache_create fail!\n", __func__);
		return -1;
	}

	tp = kmem_cache_alloc(test_cache, GFP_KERNEL);
	if (!tp) {
		pr_err("%s: kmem_cache_alloc fail!\n", __func__);
		kmem_cache_destroy(test_cache);
		return -1;
	}

	len = test_cache->size + 1;
	__check_object_size(tp, len, to_user);

	kmem_cache_free(test_cache, tp);
	kmem_cache_destroy(test_cache);

	/* test fail: no bug() by __check_object_size */
	return -1;
}

static int usercopy_stack_overlap_test(unsigned long arg, bool to_user)
{
	unsigned int tp = 0;

	__check_object_size((void *)&tp, THREAD_SIZE, to_user);

	/* test fail: no bug() by __check_object_size */
	return -1;
}

static int usercopy_text_overlap_test(unsigned long arg, bool to_user)
{
	void *tp = (void *)_stext;

	__check_object_size(tp, sizeof(unsigned int), to_user);

	/* test fail: no bug() by __check_object_size */
	return -1;
}

#define USERCOPY_TOUSER_MAGIC_NUMBER 0x56781234
#define USERCOPY_FROMUSER_MAGIC_NUMBER 0x43218765

/* using void* in order to comply with the kernel */
static int usercopy_normal_x_test(void *user_buf, void *p,
				  unsigned long len, bool to_user,
				  const char *prefix)
{
	int ret = 0;

	if (to_user) {
		*(unsigned int *)p = USERCOPY_TOUSER_MAGIC_NUMBER;
		if (copy_to_user(user_buf, p, len)) {
			pr_err("%s: copy_to_user fail!\n",  prefix);
			ret = -1;
		}
	} else {
		if (copy_from_user(p, user_buf, len)) {
			pr_err("%s: copy_from_user fail!\n", prefix);
			ret = -1;
		}
		if (*(unsigned int *)p != USERCOPY_FROMUSER_MAGIC_NUMBER) {
			pr_err("%s: data copy_from_user wrong!\n", prefix);
			ret = -1;
		}
	}

	return ret;
}

static int usercopy_normal_one_test(unsigned long arg, bool to_user)
{
	unsigned int *tp = NULL;
	int ret;

	if (!arg) {
		pr_err("%s: userspce input nill !\n", __func__);
		return -1;
	}

	tp = kzalloc(sizeof(unsigned int), GFP_KERNEL);
	if (!tp)
		return -1;

	ret = usercopy_normal_x_test((void __user *)(uintptr_t)arg, (void *)tp,
				     sizeof(unsigned int), to_user, __func__);

	kfree(tp);
	return ret;
}

static int usercopy_normal_two_test(unsigned long arg, bool to_user)
{
	unsigned int *tp = NULL;
	int ret = 0;

	if (!arg) {
		pr_err("%s: userspce input nill !\n", __func__);
		return -1;
	}

	tp = vmalloc(sizeof(unsigned int));
	if (!tp)
		return -1;

	ret = usercopy_normal_x_test((void __user *)(uintptr_t)arg, (void *)tp,
				     sizeof(unsigned int), to_user, __func__);

	vfree(tp);
	return ret;
}

static int usercopy_normal_three_test(unsigned long arg, bool to_user)
{
	unsigned int tp;

	if (!arg) {
		pr_err("%s: userspce input nill !\n", __func__);
		return -1;
	}

	return usercopy_normal_x_test((void __user *)(uintptr_t)arg,
				      (void *)&tp, sizeof(unsigned int),
				      to_user, __func__);
}

static int ro_read_test(void)
{
	pr_err("%s: g_test_ro_data change to 0x%x when kernel init!\n",
	       __func__, g_test_ro_data);
	return 0;
}

static int ro_write_test(void)
{
	g_test_ro_data += 1;

	/* test fail: no bug() by write ro data */
	return -1;
}

static int pan_read_test(unsigned long arg)
{
	unsigned int __user *tp = (unsigned int __user *)(uintptr_t)arg;

	pr_err("%s: direct read userspace 0x%x!\n", __func__, *tp);

	/* test fail: no bug() by direct read userspace */
	return -1;
}

static int pan_write_test(unsigned long arg)
{
	unsigned int __user *tp = (unsigned int __user *)(uintptr_t)arg;

	*tp = 0x1;

	/* test fail: no bug() by direct write userspace */
	return -1;
}

static int test_addr_valid(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case TEST_WRAP:
		ret = usercopy_wrapped_addr_test(arg, true);
		break;
	case TEST_NULL:
		ret = usercopy_zero_ptr_test(arg, true);
		break;
	default:
		pr_err("%s: unknown cmd 0x%x.\n", __func__, cmd);
		break;
	}

	return ret;
}

static int test_from_user(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case FROM_TEST_SLAB_CACHE:
		ret = usercopy_slab_cache_test(arg, false);
		break;
	case FROM_TEST_STACK_OVERLAP:
		ret = usercopy_stack_overlap_test(arg, false);
		break;
	case FROM_TEST_TEXT:
		ret = usercopy_text_overlap_test(arg, false);
		break;
	case FROM_TEST_NORMAL_ONE:
		ret = usercopy_normal_one_test(arg, false);
		break;
	case FROM_TEST_NORMAL_TWO:
		ret = usercopy_normal_two_test(arg, false);
		break;
	case FROM_TEST_NORMAL_THREE:
		ret = usercopy_normal_three_test(arg, false);
		break;
	default:
		pr_err("%s: unknown cmd 0x%x.\n", __func__, cmd);
		break;
	}

	return ret;
}

static int test_to_user(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case TO_TEST_SLAB_CACHE:
		ret = usercopy_slab_cache_test(arg, true);
		break;
	case TO_TEST_STACK_OVERLAP:
		ret = usercopy_stack_overlap_test(arg, true);
		break;
	case TO_TEST_TEXT:
		ret = usercopy_text_overlap_test(arg, true);
		break;
	case TO_TEST_NORMAL_ONE:
		ret = usercopy_normal_one_test(arg, true);
		break;
	case TO_TEST_NORMAL_TWO:
		ret = usercopy_normal_two_test(arg, true);
		break;
	case TO_TEST_NORMAL_THREE:
		ret = usercopy_normal_three_test(arg, true);
		break;
	default:
		pr_err("%s: unknown cmd 0x%x.\n", __func__, cmd);
		break;
	}

	return ret;
}

static int test_ro_after_init(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case TEST_RW_WHEN_INIT:
		ret = ro_read_test();
		break;
	case TEST_RW_AFTER_INIT:
		ret = ro_write_test();
		break;
	default:
		pr_err("%s: unknown cmd 0x%x.\n", __func__, cmd);
		break;
	}

	(void)arg;

	return ret;
}

static int test_pan(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	switch (cmd) {
	case TEST_DIRECT_READ_USER:
		ret = pan_read_test(arg);
		break;
	case TEST_DIRECT_WRITE_USER:
		ret = pan_write_test(arg);
		break;
	default:
		pr_err("%s: unknown cmd 0x%x.\n", __func__, cmd);
		break;
	}

	return ret;
}

static int kernel_harden_test_open(struct tty_struct *tty, struct file *filp)
{
	pr_err("kharden tty driver open");
	return 0;
}

static void kernel_harden_test_close(struct tty_struct *tty, struct file *filp)
{
	pr_err("kharden tty driver close");
}

static int kernel_harden_test_ioctl(struct tty_struct *tty, unsigned int cmd,
				    unsigned long arg)
{
	int ret = -1;

	/*
	 * the following number, such as 0xFFFFFF00 and 0x00567800,
	 * refers to "enum test_cmd_code"
	 */
	switch (cmd  & 0xFFFFFF00) {
	case 0x00567800:
		ret = test_addr_valid(cmd, arg);
		break;
	case 0x10567800:
		ret = test_from_user(cmd, arg);
		break;
	case 0x20567800:
		ret = test_to_user(cmd, arg);
		break;
	case 0x30567800:
		ret = test_ro_after_init(cmd, arg);
		break;
	case 0x40567800:
		ret = test_pan(cmd, arg);
		break;
	default:
		pr_err("%s: unknown cmd 0x%x.\n", __func__, cmd);
		break;
	}

	return ret;
}

static const struct tty_operations g_kharden_ops = {
	.open = kernel_harden_test_open,
	.close = kernel_harden_test_close,
	.ioctl = kernel_harden_test_ioctl,
};

static const struct tty_port_operations g_kharden_port_ops = {
};

static int __init kernel_harden_test_init(void)
{
	pr_err("Enter %s.\n", __func__);

	g_kharden_tty_drv = alloc_tty_driver(KH_TTY_MINORS);
	if (!g_kharden_tty_drv) {
		pr_err("[kernel_harden_test] alloc tty driver fail!\n");
		return -1;
	}

	g_kharden_tty_drv->owner = THIS_MODULE;
	g_kharden_tty_drv->driver_name = "kharden_serial";
	g_kharden_tty_drv->name = "kharden_tty";
	g_kharden_tty_drv->major = KH_TTY_MAJOR;
	g_kharden_tty_drv->minor_start = 0;
	g_kharden_tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	g_kharden_tty_drv->subtype = SERIAL_TYPE_NORMAL;
	g_kharden_tty_drv->flags = TTY_DRIVER_REAL_RAW;
	g_kharden_tty_drv->init_termios = tty_std_termios;
	g_kharden_tty_drv->init_termios.c_cflag =
	    B921600 | CS8 | CREAD | HUPCL | CLOCAL;

	tty_set_operations(g_kharden_tty_drv, &g_kharden_ops);
	tty_port_init(&g_kharden_tty_port);
	g_kharden_tty_port.ops = &g_kharden_port_ops;
	tty_port_link_device(&g_kharden_tty_port, g_kharden_tty_drv, 0);

	if (tty_register_driver(g_kharden_tty_drv)) {
		pr_err("[kernel_harden_test] register tty driver fail!\n");
		put_tty_driver(g_kharden_tty_drv);
		return -1;
	}

	/* use for test: write when kernel init */
	g_test_ro_data = 0x00001234; /* an unused flag */

	pr_err("Finish %s.\n", __func__);
	return 0;
}

static void __exit kernel_harden_test_exit(void)
{
	tty_unregister_driver(g_kharden_tty_drv);
	put_tty_driver(g_kharden_tty_drv);
}

module_init(kernel_harden_test_init);
module_exit(kernel_harden_test_exit);
MODULE_LICENSE("GPL");
