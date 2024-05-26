/*
 * health_grade.c
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
#include "health_grade.h"
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>

#include <securec.h>

#define HEALTH_INFO_ACCESS 0660
#define HEALTH_GRADE_ACCESS 0660
#define SOFTLOCK_PANIC_EN_ACESS 0440

struct mutex g_nve_access_mutex;
struct opt_nve_info_user g_health_grade_nve = {
	.nv_number = HEALTH_GRADE_CPU_NVE_NUM,
	.nv_name = HEALTH_GRADE_CPU_NVE_NAME,
	.valid_size = sizeof(struct health_grade_nve_data),
};

static int health_info_nve_write(enum health_grade_core core,
		u8 profile, u8 vol_off, u8 all_passed)
{
	struct health_grade_nve_data *nve_data = NULL;
	struct opt_nve_info_user *nve_info = &g_health_grade_nve;
	int ret;

	if (core >= HG_TYPE_MAX || profile >= HEALTH_GRADE_PROFILE_MAX ||
			vol_off >= HEALTH_INFO_VOL_OFFSET_MAX) {
		pr_err("[%s]param check error(%u, %u, %u)\n", __func__, core,
			profile, vol_off);
		return -1;
	}

	mutex_lock(&g_nve_access_mutex);

	nve_info->nv_operation = NV_READ;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]read nve error\n", __func__);
		goto error;
	}
	nve_data = (struct health_grade_nve_data *)&nve_info->nv_data[0];
	nve_data->info[core][profile] = health_info_pack(1, vol_off, all_passed);
	nve_info->nv_operation = NV_WRITE;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]write nve error\n", __func__);
		goto error;
	}

error:
	mutex_unlock(&g_nve_access_mutex);
	return ret;
}

static int health_grade_nve_write(enum health_grade_core core, u8 grade)
{
	struct health_grade_nve_data *nve_data = NULL;
	struct opt_nve_info_user *nve_info = &g_health_grade_nve;
	int ret;

	if (core >= HG_TYPE_MAX || grade >= HG_GRADE_MAX) {
		pr_err("[%s]param check error(%u, %u)\n", __func__, core, grade);
		return -1;
	}

	mutex_lock(&g_nve_access_mutex);

	nve_info->nv_operation = NV_READ;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]read nve error\n", __func__);
		goto error;
	}
	nve_data = (struct health_grade_nve_data *)&nve_info->nv_data[0];
	nve_data->grade[core] = health_grade_pack(1, grade);

	nve_info->nv_operation = NV_WRITE;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]write nve error\n", __func__);
		goto error;
	}

error:
	mutex_unlock(&g_nve_access_mutex);
	return ret;
}

static int health_nve_read(u8 tag, u8 *data, u32 len)
{
	struct opt_nve_info_user *nve_info = &g_health_grade_nve;
	struct health_grade_nve_data *nve_data = NULL;
	u8 *src = NULL;
	u32 src_len;
	int ret;

	mutex_lock(&g_nve_access_mutex);

	nve_info->nv_operation = NV_READ;
	ret = nve_direct_access_interface(nve_info);
	if (ret < 0) {
		pr_err("[%s]read nve error\n", __func__);
		goto error;
	}

	nve_data = (struct health_grade_nve_data *)&nve_info->nv_data[0];
	if (tag == HEALTH_INFO_TAG) {
		src = (u8 *)&nve_data->info[0][0];
		src_len = sizeof(nve_data->info);
	} else if (tag == HEALTH_GRADE_TAG) {
		src = nve_data->grade;
		src_len = sizeof(nve_data->grade);
	} else {
		pr_err("[%s]invalid tag\n", __func__);
		goto error;
	}
	ret = memcpy_s(data, len, src, src_len);
	if (ret < 0) {
		pr_err("[%s]memcpy_s info error\n", __func__);
		goto error;
	}

error:
	mutex_unlock(&g_nve_access_mutex);
	return ret;
}

static ssize_t health_info_write(struct file *file, const char __user *userbuf,
		size_t bytes, loff_t *off)
{
	char tmp_buf[HEALTH_INFO_INPUT_MAX];
	ssize_t byte_writen;
	int ret;
	u8 core, profile, vol_off, all_passed;

	if (bytes >= sizeof(tmp_buf) || file == NULL || userbuf == NULL || off == NULL) {
		pr_err("[%s]invalid input\n", __func__);
		return -EPERM;
	}

	(void)memset_s(tmp_buf, HEALTH_INFO_INPUT_MAX, 0, HEALTH_INFO_INPUT_MAX);

	byte_writen = simple_write_to_buffer(tmp_buf,
			HEALTH_INFO_INPUT_MAX,
			off, userbuf, bytes);
	if (byte_writen <= 0) {
		pr_err("[%s]simple_write_to_buffer error\n", __func__);
		return -EPERM;
	}

	ret = sscanf_s(tmp_buf, "%hhu %hhu %hhu %hhu", &core, &profile, &vol_off, &all_passed);
	if (ret != 4) {
		pr_err("[%s]sscanf_s input data error\n", __func__);
		return -EPERM;
	}

	ret = health_info_nve_write(core, profile, vol_off, all_passed);
	if (ret < 0) {
		pr_err("[%s]health_info_nve_write error\n", __func__);
		return -EPERM;
	}

	return byte_writen;
}

static ssize_t health_info_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	u8 health_info[HG_TYPE_MAX][HEALTH_GRADE_PROFILE_MAX];
	int ret;

	if (pos == NULL || buf == NULL || file == NULL)
		return 0;

	if (*pos >= (loff_t)sizeof(health_info))
		return 0;

	if (count < sizeof(health_info)) {
		pr_err("[%s]user buffer not enough(%ld)\n", __func__, count);
		return -EFAULT;
	}

	ret = health_nve_read(HEALTH_INFO_TAG, (u8 *)&health_info[0][0], sizeof(health_info));
	if (ret < 0) {
		pr_err("[%s]health_info_nve_read error\n", __func__);
		return -EFAULT;
	}

	return simple_read_from_buffer(buf, count, pos, health_info, sizeof(health_info));
}

static ssize_t health_grade_write(struct file *file, const char __user *userbuf,
		size_t bytes, loff_t *off)
{
	char tmp_buf[HEALTH_GRADE_INPUT_MAX];
	ssize_t byte_writen;
	int ret;
	u8 core, grade;

	if (bytes >= sizeof(tmp_buf) || file == NULL || userbuf == NULL || off == NULL) {
		pr_err("[%s]invalid input\n", __func__);
		return -EPERM;
	}

	(void)memset_s(tmp_buf, HEALTH_GRADE_INPUT_MAX, 0, HEALTH_GRADE_INPUT_MAX);

	byte_writen = simple_write_to_buffer(tmp_buf,
			HEALTH_GRADE_INPUT_MAX,
			off, userbuf, bytes);
	if (byte_writen <= 0) {
		pr_err("[%s]simple_write_to_buffer error\n", __func__);
		return -EPERM;
	}

	ret = sscanf_s(tmp_buf, "%hhu %hhu", &core, &grade);
	if (ret != 2) {
		pr_err("[%s]sscanf_s input data error\n", __func__);
		return -EPERM;
	}

	ret = health_grade_nve_write(core, grade);
	if (ret < 0) {
		pr_err("[%s]health_grade_nve_write error\n", __func__);
		return -EPERM;
	}

	return byte_writen;
}

static ssize_t health_grade_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	u8 health_grade[HG_TYPE_MAX];
	int ret;

	if (pos == NULL || buf == NULL || file == NULL)
		return 0;

	if (*pos >= (loff_t)sizeof(health_grade))
		return 0;

	if (count < sizeof(health_grade)) {
		pr_err("[%s]user buffer not enough(%ld)\n", __func__, count);
		return -EFAULT;
	}

	ret = health_nve_read(HEALTH_GRADE_TAG, health_grade, sizeof(health_grade));
	if (ret < 0) {
		pr_err("[%s]health_info_nve_read error\n", __func__);
		return -EFAULT;
	}

	return simple_read_from_buffer(buf, count, pos, health_grade, sizeof(health_grade));
}

static ssize_t softlock_panic_enable(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	watchdog_lockup_panic_config(1);
	return 0;
}

static const struct file_operations health_info_fops = {
	.write = health_info_write,
	.read = health_info_read,
};

static const struct file_operations health_grade_fops = {
	.write = health_grade_write,
	.read = health_grade_read,
};

static const struct file_operations softlock_panic_fops = {
	.read = softlock_panic_enable,
};

static int health_grade_create_debugfs(void)
{
	struct dentry *root = NULL;
	struct dentry *info = NULL;
	struct dentry *grade = NULL;
	struct dentry *softlock_panic = NULL;

	root = debugfs_create_dir("health_grade", NULL);
	if (!root) {
		pr_err("[%s]create root dir error\n", __func__);
		return -ENOENT;
	}
	info = debugfs_create_file("health_info", HEALTH_INFO_ACCESS, root, NULL, &health_info_fops);
	if (!info) {
		pr_err("[%s]health_info debugfs create failed\n", __func__);
		debugfs_remove_recursive(root);
		return -ENOENT;
	}
	grade = debugfs_create_file("health_grade", HEALTH_GRADE_ACCESS, root, NULL, &health_grade_fops);
	if (!grade) {
		pr_err("[%s]health_grade debugfs create failed\n", __func__);
		debugfs_remove_recursive(root);
		return -ENOENT;
	}
	softlock_panic = debugfs_create_file("softlock_panic_en", SOFTLOCK_PANIC_EN_ACESS, root, NULL, &softlock_panic_fops);
	if (!softlock_panic) {
		pr_err("[%s]softlock_panic_en debugfs create failed\n", __func__);
		debugfs_remove_recursive(root);
		return -ENOENT;
	}

	return 0;
}

static int __init health_grade_init(void)
{
	int ret;

	mutex_init(&g_nve_access_mutex);

	ret = health_grade_create_debugfs();
	if (ret < 0) {
		pr_err("[%s]health_grade_create_debugfs failed\n", __func__);
		return ret;
	}

	return 0;
}
late_initcall(health_grade_init);

MODULE_DESCRIPTION("HEALTH GRADE MODULE");
