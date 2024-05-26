/*
 * freqdump_k.c
 *
 * freqdump module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <global_ddr_map.h>
#include <freqdump/freqdump_kernel.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <loadmonitor_media_kernel.h>
#include <loadmonitor_common_kernel.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>
#include <freqdump_node_dump.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)	"[freqdump&loadmonitor]:" fmt

#define STR_MAX_CHIP_TYPE_SIZE	10
#define SEC_DEBUG_FS_AUTHORITY	0660
#define MAX_LOADMONITOR_PLAT_TYPE_LEN	20
u32 g_monitor_en_flags;
static char g_chip_type_buf[STR_MAX_CHIP_TYPE_SIZE];
/* 0 : es 1 : cs 2 : none */
static unsigned int g_chip_type = CS;
struct dentry *g_freqdump_debugfs_root;
struct semaphore g_loadmonitor_sem, g_freqdump_sem;
static int g_user_app_refer_count;
static unsigned int g_freqdump_module_kernel;

static void sec_freqdump_read(void)
{
	(void)atfd_service_freqdump_smc((u64)FREQDUMP_SVC_REG_RD, (u64)g_freqdump_module_kernel,
					     freqdump_phy_addr, (u64)FREQ_READ_DATA);
}

static void sec_chip_type_read(void)
{
	int ret;

	ret = snprintf_s(g_chip_type_buf, sizeof(g_chip_type_buf),
			 sizeof(g_chip_type_buf) - 1, "%u", g_chip_type);
	if (ret == -1)
		pr_err("%s snprintf_s is err.\n", __func__);
}

#if (defined NPU_INFO_FROM_KERNEL && defined CONFIG_NPU_PM_DEBUG)
/*
 * data[4][2] used to store NPU module freq & voltage info
 * name[4][10] used to store NPU module name, max length is 9 bytes
 */
extern int get_npu_freq_info(void *data, int size, char (*name)[10],
			     int size_name);
#endif

static int freqdump_node_dump(struct seq_file *s, void *p)
{
	struct freqdump_data read_data;
	struct freqdump_and_monitor_data *freqdump_addr =
		(struct freqdump_and_monitor_data *)g_freqdump_virt_addr;

	if (freqdump_addr == NULL) {
		pr_err("%s virt_addr is null.\n", __func__);
		return -EINVAL;
	}

	down(&g_freqdump_sem);
	sec_freqdump_read();

	(void)memset_s(&read_data, sizeof(struct freqdump_data), 0, sizeof(struct freqdump_data));
	memcpy_fromio(&read_data, (void *)(&freqdump_addr->freqdump),
		      sizeof(struct freqdump_data));

#if (defined NPU_INFO_FROM_KERNEL && defined CONFIG_NPU_PM_DEBUG)
	/* Add the NPU freqinfo at the end of data struct */
	if (!!(g_freqdump_module_kernel & FREQ_NPU)) {
		if (get_npu_freq_info(read_data.npu.freq_volt,
				      sizeof(read_data.npu.freq_volt),
				      read_data.npu.name,
				      sizeof(read_data.npu.name)))
			pr_err("%s Failed to get NPU freqdump info\n", __func__);
	}
#endif

	seq_write(s, (const struct freqdump_data *)&read_data,
		  sizeof(struct freqdump_data));
	up(&g_freqdump_sem);
	return OK;
}

static int loadmonitor_node_dump(struct seq_file *s, void *p)
{
	down(&g_loadmonitor_sem);
	if (g_loadmonitor_status == LOADMONITOR_OFF) {
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (g_monitor_en_flags == ENABLE_NONE) {
		pr_err("%s loadmonitor node dump error!\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (read_format_cs_loadmonitor_data(g_monitor_en_flags) == -EINVAL) {
		pr_err("%s read loadmonitor data err!\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	seq_write(s, (const void *)(&g_loadmonitor_data_kernel),
		  sizeof(struct loadmonitor_data));

	up(&g_loadmonitor_sem);
	return OK;
}

static int loadmonitor_single_process_check(struct seq_file *s, void *p)
{
	int ret;

	down(&g_loadmonitor_sem);
	if (g_user_app_refer_count != 0)
		ret = -EINVAL;
	else
		ret = 0;
	g_user_app_refer_count = 1;
	pr_info("%s ++user app count:%d", __func__, g_user_app_refer_count);

	sec_chip_type_read();
	seq_printf(s, "%s\n", g_chip_type_buf);
	up(&g_loadmonitor_sem);
	return ret;
}

static ssize_t freqdump_module_set(struct file *filp, const char __user *ubuf, size_t cnt,
				   loff_t *ppos)
{
	char buf[20] = {0};

	if (ubuf == NULL) {
		pr_err("%s %d ubuf is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	down(&g_freqdump_sem);
	if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, cnt))) {
		pr_err("%s %d copy freqdump module flag from user fail\n", __func__, __LINE__);
		up(&g_freqdump_sem);
		return -EINVAL;
	}

	g_freqdump_module_kernel = (unsigned int)simple_strtol(buf, NULL, 10); /* Decimal */
	pr_info("%s %d freqdump module flag: 0x%x\n", __func__, __LINE__, g_freqdump_module_kernel);

	up(&g_freqdump_sem);
	return cnt;
}

static int loadmonitor_single_process_open(struct inode *inode, struct file *file)
{
	return single_open(file, loadmonitor_single_process_check, NULL);
}

static ssize_t loadmonitor_disable_write(struct file *filp,
					 const char __user *ubuf, size_t cnt,
					 loff_t *ppos)
{
	char buf[MAX_LOADMONITOR_PLAT_TYPE_LEN] = {0};

	if (ubuf == NULL)
		return -EINVAL;

	down(&g_loadmonitor_sem);
	if (g_loadmonitor_status == LOADMONITOR_OFF) {
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}
	if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, cnt))) {
		pr_err("loadmonitor_switch_write can not copy from user\n");
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	if (buf[0] == '1') {
		sec_loadmonitor_switch_disable(g_monitor_en_flags, false);

		g_loadmonitor_status = LOADMONITOR_OFF;
		g_monitor_en_flags = ENABLE_NONE;
		g_user_app_refer_count = 0;

		pr_info("%s --user app count:%d", __func__,
			g_user_app_refer_count);
	}
	up(&g_loadmonitor_sem);
	return cnt;
}

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
static ssize_t loadmonitor_get_enable_info(const char __user *ubuf, size_t cnt)
{
	int ret;
	char read_buff[USER_BUFFER_SIZE] = {0};

	if (copy_from_user(read_buff, ubuf, min_t(size_t, sizeof(read_buff) - 1, cnt))) {
		pr_err("loadmonitor can not copy from user\n");
		return -EINVAL;
	}
	read_buff[USER_BUFFER_SIZE - 1] = '\0';

	ret = sscanf_s(read_buff, "%u,%u,%u,%u,%u,%u",
		       &g_monitor_delay_value.monitor_enable_flags,
		       &g_monitor_delay_value.peri0, &g_monitor_delay_value.peri1,
		       &g_monitor_delay_value.ao, &g_monitor_delay_value.media0,
		       &g_monitor_delay_value.media1);
	if (ret == -1) {
		pr_err("%s sscanf_s is err\n", __func__);
		return -EINVAL;
	}
	return OK;
}
#else
static ssize_t loadmonitor_get_enable_info(const char __user *ubuf, size_t cnt)
{
	int ret;
	char read_buff[USER_BUFFER_SIZE] = {0};

	if (copy_from_user(read_buff, ubuf, min_t(size_t, sizeof(read_buff) - 1, cnt))) {
		pr_err("loadmonitor can not copy from user\n");
		return -EINVAL;
	}
	read_buff[USER_BUFFER_SIZE - 1] = '\0';

	ret = sscanf_s(read_buff, "%u,%u,%u,%u", &g_monitor_delay_value.monitor_enable_flags,
		       &g_monitor_delay_value.peri0, &g_monitor_delay_value.peri1,
		       &g_monitor_delay_value.ao);
	if (ret == -1) {
		pr_err("%s sscanf_s is err\n", __func__);
		return -EINVAL;
	}
	return OK;
}
#endif

#ifdef CONFIG_ENABLE_MIDEA_MONITOR
static ssize_t loadmonitor_enable_write(struct file *filp, const char __user *ubuf,
					size_t cnt, loff_t *ppos)
{
	if (ubuf == NULL)
		return -EINVAL;
	pr_err("%s down\n", __func__);

	down(&g_loadmonitor_sem);
	if (loadmonitor_get_enable_info(ubuf, cnt) != OK) {
		pr_err("%s get enable info err\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	pr_err("%s delay_time 0x%x: %u, %u, %u, %u, %u\n", __func__,
	       g_monitor_delay_value.monitor_enable_flags, g_monitor_delay_value.peri0,
	       g_monitor_delay_value.peri1, g_monitor_delay_value.ao,
	       g_monitor_delay_value.media0, g_monitor_delay_value.media1);

	if (g_monitor_delay_value.monitor_enable_flags == ENABLE_NONE) {
		pr_err("%s loadmonitor can not be enable\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	g_monitor_en_flags = g_monitor_delay_value.monitor_enable_flags;
	sec_loadmonitor_switch_enable(false);
	g_loadmonitor_status = LOADMONITOR_ON;

	up(&g_loadmonitor_sem);
	return (ssize_t)cnt;
}
#else
static ssize_t loadmonitor_enable_write(struct file *filp, const char __user *ubuf,
					size_t cnt, loff_t *ppos)
{
	if (ubuf == NULL)
		return -EINVAL;
	pr_err("%s down\n", __func__);

	down(&g_loadmonitor_sem);
	if (loadmonitor_get_enable_info(ubuf, cnt) != OK) {
		pr_err("%s get enable info err\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	pr_err("%s delay_time 0x%x: %u, %u, %u\n", __func__,
	       g_monitor_delay_value.monitor_enable_flags,
	       g_monitor_delay_value.peri0, g_monitor_delay_value.peri1,
	       g_monitor_delay_value.ao);

	if (g_monitor_delay_value.monitor_enable_flags == ENABLE_NONE) {
		pr_err("%s loadmonitor can not be enable\n", __func__);
		up(&g_loadmonitor_sem);
		return -EINVAL;
	}

	g_monitor_en_flags = g_monitor_delay_value.monitor_enable_flags;
	sec_loadmonitor_switch_enable(false);
	g_loadmonitor_status = LOADMONITOR_ON;

	up(&g_loadmonitor_sem);
	return (ssize_t)cnt;
}
#endif

static int loadmonitor_debugfs_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, loadmonitor_node_dump, NULL);
}

static int freqdump_debugfs_node_open(struct inode *inode, struct file *file)
{
	return single_open(file, freqdump_node_dump, NULL);
}

static const struct file_operations g_freqdump_fops = {
	.owner = THIS_MODULE,
	.open = freqdump_debugfs_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations g_freqdump_module_fops = {
	.owner = THIS_MODULE,
	.write = freqdump_module_set,
};

static const struct file_operations g_loadmonitor_dump_fops = {
	.owner = THIS_MODULE,
	.open = loadmonitor_debugfs_node_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations g_loadmonitor_enable_fops = {
	.owner = THIS_MODULE,
	.write = loadmonitor_enable_write,
	.open = loadmonitor_single_process_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations g_loadmonitor_disable_fops = {
	.owner = THIS_MODULE,
	.write = loadmonitor_disable_write,
};

struct file_str_ops g_file_ops[] = {
	{
		.str = "freqdump_data",
		.file_ops = &g_freqdump_fops,
	},
	{
		.str = "freqdump_module_flag",
		.file_ops = &g_freqdump_module_fops,
	},
	{
		.str = "loadmonitor_data",
		.file_ops = &g_loadmonitor_dump_fops,
	},
	{
		.str = "loadmonitor_enable",
		.file_ops = &g_loadmonitor_enable_fops,
	},
	{
		.str = "loadmonitor_disable",
		.file_ops = &g_loadmonitor_disable_fops,
	},
};

static int create_debugfs_node(void)
{
	struct dentry *freqdump_file = NULL;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(g_file_ops); i++) {
		freqdump_file = debugfs_create_file(g_file_ops[i].str,
						    SEC_DEBUG_FS_AUTHORITY,
						    g_freqdump_debugfs_root,
						    NULL,
						    g_file_ops[i].file_ops);
		if (freqdump_file == NULL) {
			pr_err("%s create file str:%s error\n", __func__,
			       g_file_ops[i].str);
			return -ENOMEM;
		}
	}
	return 0;
}

static int __init freqdump_debugfs_init(void)
{
	int ret;

	sema_init(&g_loadmonitor_sem, 1);
	sema_init(&g_freqdump_sem, 1);

	g_freqdump_debugfs_root = debugfs_create_dir("freqdump", NULL);
	if (g_freqdump_debugfs_root == NULL) {
		pr_err("%s create freqdump file error\n", __func__);
		goto dir_fail;
	}

	ret = create_debugfs_node();
	if (ret != OK)
		goto file_fail;

	ret = freqdump_node_dump_init();
	if (ret != OK)
		pr_err("%s %d freqdump node dump init fail\n", __func__, __LINE__);

	pr_err("%s success. shared mem usage:%lu%%\n", __func__,
	       sizeof(struct freqdump_and_monitor_data) * 100 /
	       SHARED_MEMORY_SIZE);
	return OK;

file_fail:
	debugfs_remove_recursive(g_freqdump_debugfs_root);
	g_freqdump_debugfs_root = NULL;
dir_fail:
	pr_err("freqdump debugfs init failed!\n");
	return -ENOENT;
}

static void __exit freqdump_debugfs_exit(void)
{
	debugfs_remove_recursive(g_freqdump_debugfs_root);
	g_freqdump_debugfs_root = NULL;
	pr_err("freqdump removed!\n");
}

module_init(freqdump_debugfs_init);
module_exit(freqdump_debugfs_exit);
