/*
 * mntn_switch.c
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include <linux/debugfs.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>

#include <securec.h>
#include <mntn_public_interface.h>

#define MNTN_SWITCH_INFO_NAME_MAX_LEN  30
#define MNTN_SWITCH_INVALID_INDEX      0xFF
#define FILE_ACCESS               0660
#define MNTN_SWITCH_NV_NUMBER          297

struct mntn_switch_match_info {
	u32  index;
	char name[MNTN_SWITCH_INFO_NAME_MAX_LEN];
};

static struct mntn_switch_match_info mntn_switch_sw_whitelist[] = {
	{MNTN_AP_WDT,           "ap_watchdog"},
	{MNTN_FST_DUMP_MEM,     "fastboot_dump"},
	{MNTN_GOBAL_RESETLOG,   "global_resetlog"}
};

struct mutex g_mntn_switch_access_mutex;
struct opt_nve_info_user g_mntn_switch_nve = {
	.nv_number = MNTN_SWITCH_NV_NUMBER,
	.nv_name = "HIMNTN",
	.valid_size = MNTN_BOTTOM,
};

static u32 find_mntn_switch_index(const char *name)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(mntn_switch_sw_whitelist); i++) {
		if (!strncmp(mntn_switch_sw_whitelist[i].name,
				name, strlen(mntn_switch_sw_whitelist[i].name)))
			return mntn_switch_sw_whitelist[i].index;
	}

	return MNTN_SWITCH_INVALID_INDEX;
}

static int mntn_switch_nve_write(const char *name, u32 enable)
{
	struct opt_nve_info_user *nve_info = &g_mntn_switch_nve;
	u32 index;
	int ret;

	index = find_mntn_switch_index(name);
	if (index == MNTN_SWITCH_INVALID_INDEX) {
		pr_err("[%s]invalid name\n", __func__);
		return -EPERM;
	}

	mutex_lock(&g_mntn_switch_access_mutex);

	nve_info->nv_operation = NV_READ;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]read nve error\n", __func__);
		goto error;
	}
	if (enable)
		nve_info->nv_data[index] = '1';
	else
		nve_info->nv_data[index] = '0';
	nve_info->nv_operation = NV_WRITE;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]write nve error\n", __func__);
		goto error;
	}

error:
	mutex_unlock(&g_mntn_switch_access_mutex);
	return ret;
}

static int mntn_switch_nve_read(char *buf, u32 len)
{
	struct opt_nve_info_user *nve_info = &g_mntn_switch_nve;
	int ret;

	mutex_lock(&g_mntn_switch_access_mutex);

	nve_info->nv_operation = NV_READ;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]read nve error\n", __func__);
		goto error;
	}

	ret = memcpy_s(buf, len, nve_info->nv_data, MNTN_BOTTOM);
	if (ret < 0) {
		pr_err("[%s]memcpy_s error\n", __func__);
		goto error;
	}

error:
	mutex_unlock(&g_mntn_switch_access_mutex);
	return ret;
}

static ssize_t mntn_switch_write(const char __user *userbuf,
		size_t bytes, loff_t *off, u32 enable)
{
	char tmp_buf[MNTN_SWITCH_INFO_NAME_MAX_LEN];
	ssize_t byte_writen;
	int ret;

	if (bytes >= sizeof(tmp_buf) || userbuf == NULL || off == NULL) {
		pr_err("[%s]invalid input\n", __func__);
		return -EPERM;
	}

	(void)memset_s(tmp_buf, MNTN_SWITCH_INFO_NAME_MAX_LEN, 0, MNTN_SWITCH_INFO_NAME_MAX_LEN);

	byte_writen = simple_write_to_buffer(tmp_buf,
			MNTN_SWITCH_INFO_NAME_MAX_LEN - 1,
			off, userbuf, bytes);
	if (byte_writen <= 0) {
		pr_err("[%s]simple_write_to_buffer error\n", __func__);
		return byte_writen;
	}

	ret = mntn_switch_nve_write(tmp_buf, enable);
	if (ret < 0) {
		pr_err("[%s]mntn_switch_nve_write error\n", __func__);
		return -EPERM;
	}

	return byte_writen;
}

static ssize_t mntn_switch_enable_write(struct file *file, const char __user *userbuf,
		size_t bytes, loff_t *off)
{
	return mntn_switch_write(userbuf, bytes, off, 1);
}

static ssize_t mntn_switch_disable_write(struct file *file, const char __user *userbuf,
		size_t bytes, loff_t *off)
{
	return mntn_switch_write(userbuf, bytes, off, 0);
}

static ssize_t mntn_switch_status_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	char tmp_buf[MNTN_BOTTOM + 2];
	int ret;

	if (pos == NULL || buf == NULL || file == NULL)
		return 0;

	if (*pos >= (loff_t)sizeof(tmp_buf))
		return 0;

	if (count < sizeof(tmp_buf)) {
		pr_err("[%s]no enough buffer\n", __func__);
		return -EPERM;
	}

	(void)memset_s(tmp_buf, sizeof(tmp_buf), 0x0, sizeof(tmp_buf));

	ret = mntn_switch_nve_read(tmp_buf, MNTN_BOTTOM);
	if (ret < 0) {
		pr_err("[%s]mntn_switch_nve_read error\n", __func__);
		return -EPERM;
	}
	tmp_buf[MNTN_BOTTOM] = '\n';

	return simple_read_from_buffer(buf, count, pos, tmp_buf, sizeof(tmp_buf));
}

static const struct file_operations mntn_switch_enable_ops = {
	.write = mntn_switch_enable_write,
	.read = mntn_switch_status_read,
};

static const struct file_operations mntn_switch_disable_ops = {
	.write = mntn_switch_disable_write,
	.read = mntn_switch_status_read,
};

static int mntn_switch_create_debugfs(void)
{
	struct dentry *root = NULL;
	struct dentry *enable = NULL;
	struct dentry *disable = NULL;

	root = debugfs_create_dir("mntn_switch", NULL);
	if (!root) {
		pr_err("[%s]create root dir error\n", __func__);
		return -ENOENT;
	}
	enable = debugfs_create_file("enable", FILE_ACCESS, root, NULL, &mntn_switch_enable_ops);
	if (!enable) {
		pr_err("[%s]mntn_switch enable debugfs create failed\n", __func__);
		debugfs_remove_recursive(root);
		return -ENOENT;
	}
	disable = debugfs_create_file("disable", FILE_ACCESS, root, NULL, &mntn_switch_disable_ops);
	if (!disable) {
		pr_err("[%s]mntn_switch disable debugfs create failed\n", __func__);
		debugfs_remove_recursive(root);
		return -ENOENT;
	}

	return 0;
}

static int __init mntn_switch_init(void)
{
	int ret;

	mutex_init(&g_mntn_switch_access_mutex);

	ret = mntn_switch_create_debugfs();
	if (ret < 0) {
		pr_err("[%s]mntn_switch_create_debugfs failed\n", __func__);
		return ret;
	}

	return 0;
}
late_initcall(mntn_switch_init);

