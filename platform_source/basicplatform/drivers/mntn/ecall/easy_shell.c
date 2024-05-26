/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/kallsyms.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <linux/version.h>
#include "securec.h"

#define PR_LOG_TAG EASY_SHELL_TAG

#define ES_TTY_MAJOR		221
#define ES_TTY_MINORS		1
#define ES_TTY_IOCTL_SIGN	0x4AFA0000
#define ES_TTY_IOCTL_CALL	0x4AFA0001

#define valid_sign(tty_termios) (((unsigned int)ES_TTY_MAJOR<<24) | (unsigned int)(C_BAUD(tty_termios)))

#define SHELL_MAX_ARG_NUM	6
#define SHELL_DBG_VARIABLE	0x28465793
#define SHELL_MAX_ARG_SIZE	1024

struct call_arg {
	unsigned int sign_word;
	char *func;
	char *arg_str;
	long long output;
	union {
		long long args[SHELL_MAX_ARG_NUM];
		struct {
			long long arg1;
			long long arg2;
			long long arg3;
			long long arg4;
			long long arg5;
			long long arg6;
		};
	};
};

struct call_arg32 {
	unsigned int sign_word;
	unsigned int func;
	char *arg_str;
	long long output;
	union {
		long long args[SHELL_MAX_ARG_NUM];
		struct {
			long long arg1;
			long long arg2;
			long long arg3;
			long long arg4;
			long long arg5;
			long long arg6;
		};
	};
};

typedef long long (*func_type_str_arg) (char *arg_str, long long arg1,long long arg2,
		long long arg3,long long arg4, long long arg5,long long arg6);
typedef long long (*func_type) (long long arg1, long long arg2, long long arg3,
		long long arg4, long long arg5, long long arg6);

static struct tty_driver *shell_tty_drv;
static struct tty_port shell_tty_port;

long long dbg_value_for_ecall = 0x12345678;
void easy_shell_test(void)
{
	long long *t = NULL;

	t = &dbg_value_for_ecall;
	pr_info("t = 0x%pK\n", t);
}

void lkup(char *func_name)
{
	unsigned long address;

	if (func_name) {
#ifndef _DRV_LLT_
		address = (unsigned long)kallsyms_lookup_name(func_name);
#endif
		pr_info("lk_addr (0x%lx)%s\n", address, func_name);
	} else {
		pr_info("null func\n");
	}
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)) || (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static long long __nocfi shell_do_func_call(uintptr_t func_addr, char *arg_str, long long args[])
#else
static long long shell_do_func_call(uintptr_t func_addr, char *arg_str, long long args[])
#endif
{
	func_type_str_arg func_str_arg = NULL;
	func_type func = NULL;
	mm_segment_t fs;
	long long ret;

	fs = get_fs();
	set_fs(KERNEL_DS);
	if (args[0] == SHELL_DBG_VARIABLE) {
		ret = *(long long *)func_addr;
	} else {
		if (arg_str != NULL) {
			func_str_arg = (func_type_str_arg)func_addr;
			ret = func_str_arg(arg_str, args[0], args[1], args[2], args[3], args[4], args[5]);
		} else {
			func = (func_type)func_addr;
			ret = func(args[0], args[1], args[2], args[3], args[4], args[5]);
		}
	}
	set_fs(fs);

	return ret;
}

static int shell_call_func(uintptr_t func_ptr, uintptr_t str_arg_ptr, long long args[], long long *result)
{
	uintptr_t func_addr;
	char *func_name = NULL;
	char *str_arg = NULL;
	long long ret;

	if (func_ptr == 0) {
		pr_err("function is null\n");
		return -EFAULT;
	}

	func_name = kzalloc(SHELL_MAX_ARG_SIZE, GFP_KERNEL);
	if (!func_name) {
		pr_err("alloc memory for function name failed\n");
		return -ENOMEM;
	}

	ret = strncpy_from_user(func_name, (const char __user *)func_ptr, SHELL_MAX_ARG_SIZE);
	if (ret <= 0 || ret >= SHELL_MAX_ARG_SIZE) {
		pr_err("copy function name from user space failed\n");
		ret = -EFAULT;
		goto free_mem;
	}

	func_addr = (uintptr_t)kallsyms_lookup_name(func_name);
	if (func_addr == 0) {
		pr_err("Invalid function\n");
		ret = -EFAULT;
		goto free_mem;
	}

	if (str_arg_ptr != 0) {
		str_arg = kzalloc(SHELL_MAX_ARG_SIZE, GFP_KERNEL);
		if (!str_arg) {
			pr_err("alloc memory for string arg failed\n");
			ret = -ENOMEM;
			goto free_mem;
		}
		ret = strncpy_from_user(str_arg, (const char __user *)str_arg_ptr, SHELL_MAX_ARG_SIZE);
		if (ret <= 0 || ret >= SHELL_MAX_ARG_SIZE) {
			pr_err("copy string arg from user space failed\n");
			ret = -EFAULT;
			goto free_mem;
		}
	}
	pr_info("input parameter: arg1=%llx,arg2=%llx,arg3=%llx,arg4=%llx,arg5=%llx,arg6=%llx\n",
			args[0], args[1], args[2], args[3], args[4], args[5]);
	ret = shell_do_func_call(func_addr, str_arg, args);
	*result = ret;
	pr_info("Call %s return, value = 0x%llx\n", func_name, ret);
	ret = 0;

free_mem:
	if (func_name)
		kfree(func_name);
	if (str_arg)
		kfree(str_arg);

	return (int)ret;
}

static int shell_process(struct tty_struct *tty, unsigned long user_ptr)
{
	struct call_arg arg;
	uintptr_t output;
	long long result = 0;
	int ret;

	(void)memset_s(&arg, sizeof(arg), 0, sizeof(arg));
	if (copy_from_user((void *)&arg, (const void __user *)(uintptr_t)user_ptr, sizeof(arg))) {
		pr_err("copy data from user failed\n");
		return -EFAULT;
	}

	if (arg.sign_word & ~(unsigned int)valid_sign(tty)) {
		pr_err("Unallowed call\n");
		return -EPERM;
	}

	output = (uintptr_t)&((struct call_arg *)(uintptr_t)user_ptr)->output;
	ret = shell_call_func((uintptr_t)arg.func, (uintptr_t)arg.arg_str, arg.args, &result);
	if (ret == 0)
		put_user(result, (unsigned long __user *)output);

	return ret;
}

static int shell_process_compact(struct tty_struct *tty, unsigned long user_ptr)
{
	struct call_arg32 arg;
	uintptr_t output;
	long long result = 0;
	int ret;

	(void)memset_s(&arg, sizeof(arg), 0, sizeof(arg));
	if (copy_from_user((void *)&arg, (const void __user *)(uintptr_t)user_ptr, sizeof(arg))) {
		pr_err("copy data from user failed\n");
		return -EFAULT;
	}

	if (arg.sign_word & ~(unsigned int)valid_sign(tty)) {
		pr_err("Unallowed call\n");
		return -EPERM;
	}

	output = (uintptr_t)&((struct call_arg32 *)(uintptr_t)user_ptr)->output;
	ret = shell_call_func((uintptr_t)arg.func & 0xffffffff, 0, arg.args, &result);
	if (ret == 0)
		put_user(result, (unsigned long __user *)output);

	return ret;
}

static int __shell_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg, bool compact)
{
	int ret = -ENOIOCTLCMD;

	switch (cmd) {
	case ES_TTY_IOCTL_SIGN:
		ret = (int)valid_sign(tty);
		break;

	case ES_TTY_IOCTL_CALL:
		if (arg == 0) {
			pr_err("arg is null\n");
			return -EFAULT;
		}
		if (compact)
			ret = shell_process_compact(tty, arg);
		else
			ret = shell_process(tty, arg);
		break;
	default:
		pr_err("shell_ioctl unknown cmd\n");
		break;
	}

	return ret;
}

static int shell_ioctl(struct tty_struct *tty, unsigned int cmd,
		       unsigned long arg)
{
	return __shell_ioctl(tty, cmd, arg, false);
}

static long shell_compact_ioctl(struct tty_struct *tty, unsigned int cmd,
		       unsigned long arg)
{
	return __shell_ioctl(tty, cmd, arg, true);
}

static int shell_open(struct tty_struct *tty, struct file *filp)
{
	pr_info("easy shell open\n");
	return 0;
}

static void shell_close(struct tty_struct *tty, struct file *filp)
{
	pr_info("easy shell close\n");
}

static const struct tty_operations shell_ops = {
	.open = shell_open,
	.close = shell_close,
	.ioctl = shell_ioctl,
	.compat_ioctl = shell_compact_ioctl,
};

static const struct tty_port_operations shell_port_ops = {
};

static int shell_init(void)
{
	pr_info("Enter ecall init\n");

	shell_tty_drv = alloc_tty_driver(ES_TTY_MINORS);
	if (!shell_tty_drv) {
		pr_err("Cannot alloc shell tty driver\n");
		return -1;
	}

	shell_tty_drv->owner = THIS_MODULE;
	shell_tty_drv->driver_name = "ecall_serial";
	shell_tty_drv->name = "ecall_tty";
	shell_tty_drv->major = ES_TTY_MAJOR;
	shell_tty_drv->minor_start = 0;
	shell_tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	shell_tty_drv->subtype = SERIAL_TYPE_NORMAL;
	shell_tty_drv->flags = TTY_DRIVER_REAL_RAW;
	shell_tty_drv->init_termios = tty_std_termios;
	shell_tty_drv->init_termios.c_cflag =
	    B921600 | CS8 | CREAD | HUPCL | CLOCAL;

	tty_set_operations(shell_tty_drv, &shell_ops);
	tty_port_init(&shell_tty_port);
	shell_tty_port.ops = &shell_port_ops;
	tty_port_link_device(&shell_tty_port, shell_tty_drv, 0);

	if (tty_register_driver(shell_tty_drv)) {
		pr_err("Error registering shell tty driver\n");
		put_tty_driver(shell_tty_drv);
		return -1;
	}

	pr_info("Finish ecall init\n");

	return 0;
}

module_init(shell_init)
