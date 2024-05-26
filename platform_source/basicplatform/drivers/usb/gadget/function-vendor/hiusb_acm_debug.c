/*
 * hiusb_acm_debug.c
 *
 * ACM debug function
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/usb.h>

#include <platform_include/basicplatform/linux/usb/drv_acm.h>
#include <platform_include/basicplatform/linux/usb/drv_udi.h>

#include "modem_acm.h"
#include "chip_adp_acm.h"


#define FILE_MODE 0600
/* By default 9. But it's not really important. Any number is OK. */
static unsigned int g_acm_dump_port = 9;

static int hiusb_acm_cdev_show(struct seq_file *s, void *d)
{
	hiusb_do_acm_cdev_dump(s, g_acm_dump_port);
	return 0;
}

static int hiusb_debug_acm_cdev_open(struct inode *inode, struct file *file)
{
	return single_open(file, hiusb_acm_cdev_show, inode->i_private);
}

static ssize_t hiusb_debug_acm_cdev_write(struct file *file,
		const char __user *buf, size_t size, loff_t *ppos)
{
	if (buf == NULL)
		return -EINVAL;

	if (kstrtouint_from_user(buf, size, 0, &g_acm_dump_port)) {
		pr_err("[%s] set g_acm_dump_port failed\n", __func__);
		return -EINVAL;
	}

	return size;
}

static const struct file_operations hiusb_debug_acm_cdev_fops = {
	.open = hiusb_debug_acm_cdev_open,
	.read = seq_read,
	.write = hiusb_debug_acm_cdev_write,
	.release = single_release,
};


static void hiusb_do_acm_cdev_port_dump(struct seq_file *s)
{
	unsigned int port_num;

	seq_puts(s, "\n== current acm port list ==\n");
	for (port_num = 0; port_num < ACM_CDEV_COUNT; port_num++)
		seq_printf(s, "port: %u : %s\n", port_num,
			get_acm_cdev_name(port_num));
}

static int hiusb_acm_cdev_port_num_show(struct seq_file *s, void *d)
{
	hiusb_do_acm_cdev_port_dump(s);
	return 0;
}

static int hiusb_debug_acm_cdev_port_num_open(struct inode *inode,
					struct file *file)
{
	return single_open(file, hiusb_acm_cdev_port_num_show,
			inode->i_private);
}

/* acm_cdev_dump_show file ops */
static const struct file_operations hiusb_debug_acm_cdev_port_num_fops = {
	.open = hiusb_debug_acm_cdev_port_num_open,
	.read = seq_read,
	.release = single_release,
};


static void acm_cdev_ioctl_dump(struct seq_file *s)
{
#define ACM_PRINT_IOCTL(cmd, seq_file)	\
	seq_printf(seq_file, "command:%s\t\t\t\tcode:0x%x\n", #cmd, cmd)
	ACM_PRINT_IOCTL(ACM_IOCTL_SET_WRITE_CB, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_SET_READ_CB, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_SET_EVT_CB, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_SET_FREE_CB, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_WRITE_DO_COPY, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_WRITE_ASYNC, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_GET_RD_BUFF, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_RETURN_BUFF, s);
	ACM_PRINT_IOCTL(ACM_IOCTL_RELLOC_READ_BUFF, s);
#undef ACM_PRINT_IOCTL
}

static int hiusb_acm_cdev_ioctl_show(struct seq_file *s, void *d)
{
	acm_cdev_ioctl_dump(s);
	return 0;
}

static int hiusb_debug_acm_cdev_ioctl_open(struct inode *inode, struct file *file)
{
	return single_open(file, hiusb_acm_cdev_ioctl_show, inode->i_private);
}

static const struct file_operations hiusb_debug_acm_cdev_ioctl_fops = {
	.open = hiusb_debug_acm_cdev_ioctl_open,
	.read = seq_read,
	.release = single_release,
};

static int hiusb_acm_adp_show(struct seq_file *s, void *d)
{
	acm_adp_dump(s);
	return 0;
}

static int hiusb_acm_adp_open(struct inode *inode, struct file *file)
{
	return single_open(file, hiusb_acm_adp_show, inode->i_private);
}

static const struct file_operations hiusb_acm_adp_fops = {
	.open = hiusb_acm_adp_open,
	.read = seq_read,
	.release = single_release,
};

static struct dentry *gadget_debugfs_root;

void modem_acm_debugfs_init(void)
{
	struct dentry *root = NULL;
	struct dentry *file = NULL;

	if (gadget_debugfs_root != NULL) {
		pr_err("The gadget_debugfs_root was already make.\n");
		return;
	}

	/* shuld be /sys/kernel/usb/gadget/ */
	root = debugfs_create_dir("gadget", usb_debug_root);
	if (IS_ERR_OR_NULL(root)) {
		pr_err("Make \"gadget\" dir failed.\n");
		return;
	}

	gadget_debugfs_root = root;

	file = debugfs_create_file("acm_cdev_dump", FILE_MODE,
			gadget_debugfs_root, NULL, &hiusb_debug_acm_cdev_fops);
	if (file == NULL)
		pr_err("create acm_cdev_dump failed\n");

	file = debugfs_create_file("acm_cdev_port_num", FILE_MODE,
			gadget_debugfs_root, NULL, &hiusb_debug_acm_cdev_port_num_fops);
	if (file == NULL)
		pr_err("create acm_cdev_port_num failed\n");

	file = debugfs_create_file("acm_ioctl_dump", FILE_MODE,
			gadget_debugfs_root, NULL, &hiusb_debug_acm_cdev_ioctl_fops);
	if (file == NULL)
		pr_err("create acm_ioctl_dump failed\n");

	file = debugfs_create_file("acm_adp_dump", FILE_MODE,
			gadget_debugfs_root, NULL, &hiusb_acm_adp_fops);
	if (file == NULL)
		pr_err("create acm_adp_dump failed\n");
}

void modem_acm_debugfs_exit(void)
{
	if (gadget_debugfs_root != NULL) {
		debugfs_remove_recursive(gadget_debugfs_root);
		gadget_debugfs_root = NULL;
	}
}
