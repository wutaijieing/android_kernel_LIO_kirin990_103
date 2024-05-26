/*
 * hisi_factory_svfd_data.c
 *
 * read factory svfd data
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#define SVFD_NV_NUMBER                  257
#define SVFD_NV_NAME                    "SVFD_PI"
#define SVFD_NV_SIZE                    104
#define SVFD_PROTECTED_DATA_SIZE        40

#define ERROR_READ_NV        (-1)
#define ERROR_WRITE_NV       (-2)

static DEFINE_MUTEX(g_hisi_factory_svfd_mutex);

static int hisi_svfd_nve_read(char *buf, u32 len)
{
	struct opt_nve_info_user nve_info = {
		.nv_operation = NV_READ,
		.nv_number = SVFD_NV_NUMBER,
		.nv_name = SVFD_NV_NAME,
		.valid_size = SVFD_NV_SIZE,
	};
	int ret;

	ret = nve_direct_access_interface(&nve_info);
	if (ret != 0) {
		pr_err("[%s]read nve error\n", __func__);
		return ERROR_READ_NV;
	}

	ret = memcpy_s(buf, len, nve_info.nv_data, SVFD_NV_SIZE);
	if (ret != EOK) {
		pr_err("[%s]copy memory error\n", __func__);
		return ERROR_READ_NV;
	}

	return 0;
}

static int hisi_svfd_nve_write(const char *buf, u32 len)
{
	struct opt_nve_info_user nve_info = {
		.nv_operation = NV_WRITE,
		.nv_number = SVFD_NV_NUMBER,
		.nv_name = SVFD_NV_NAME,
		.valid_size = SVFD_NV_SIZE,
	};
	int ret;

	ret = memcpy_s(nve_info.nv_data, SVFD_NV_SIZE, buf, len);
	if (ret != EOK) {
		pr_err("[%s]copy memory error\n", __func__);
		return ERROR_WRITE_NV;
	}

	ret = nve_direct_access_interface(&nve_info);
	if (ret != 0) {
		pr_err("[%s]write nve error\n", __func__);
		return ERROR_WRITE_NV;
	}

	return 0;
}

#define SIZE_LF_STREND        2
static ssize_t hisi_factory_svfd_read(struct file *file,
				      char __user *buf,
				      size_t count,
				      loff_t *pos)
{
	char tmp_buf[SVFD_NV_SIZE + SIZE_LF_STREND];
	int ret;

	if (!file || !buf || !pos) {
		pr_err("[%s]pointer params are null\n", __func__);
		return -EPERM;
	}

	if (*pos >= (loff_t)sizeof(tmp_buf) || count == 0)
		return 0;

	(void)memset_s(tmp_buf, sizeof(tmp_buf), 0x0, sizeof(tmp_buf));

	mutex_lock(&g_hisi_factory_svfd_mutex);
	ret = hisi_svfd_nve_read(tmp_buf, SVFD_NV_SIZE);
	mutex_unlock(&g_hisi_factory_svfd_mutex);

	if (ret != 0) {
		pr_err("[%s]read svfd data error\n", __func__);
		return -EIO;
	}
	tmp_buf[SVFD_NV_SIZE] = '\n';

	return simple_read_from_buffer(buf, count, pos, tmp_buf,
				       sizeof(tmp_buf));
}

static ssize_t hisi_factory_svfd_write(struct file *file,
				       const char __user *userbuf,
				       size_t bytes,
				       loff_t *off)
{
	char tmp_buf[SVFD_NV_SIZE];
	loff_t off_local;
	ssize_t count;
	int ret;

	if (!file || !userbuf || !off) {
		pr_err("[%s]pointer params are null\n", __func__);
		return -EPERM;
	}

	if (*off >= (loff_t)sizeof(tmp_buf) || bytes == 0)
		return 0;

	if (*off < SVFD_PROTECTED_DATA_SIZE) {
		pr_err("[%s]the first %d bytes of data are protected\n",
		       __func__, SVFD_PROTECTED_DATA_SIZE);
		return -EPERM;
	}

	(void)memset_s(tmp_buf, sizeof(tmp_buf), 0x0, sizeof(tmp_buf));

	mutex_lock(&g_hisi_factory_svfd_mutex);
	ret = hisi_svfd_nve_read(tmp_buf, SVFD_NV_SIZE);
	if (ret != 0) {
		pr_err("[%s]read svfd data error\n", __func__);
		count = -EIO;
		goto error;
	}

	off_local = *off;
	count = simple_write_to_buffer(tmp_buf, sizeof(tmp_buf), &off_local,
				       userbuf, bytes);
	if (count <= 0)
		goto error;

	ret = hisi_svfd_nve_write(tmp_buf, SVFD_NV_SIZE);
	if (ret != 0) {
		pr_err("[%s]write svfd data error\n", __func__);
		count = -EIO;
		goto error;
	}
	*off = off_local;
error:
	mutex_unlock(&g_hisi_factory_svfd_mutex);
	return count;
}

static loff_t hisi_factory_svfd_llseek(struct file *file, loff_t offset,
				       int whence)
{
	loff_t pos = 0;

	if (!file) {
		pr_err("[%s]pointer param is null\n", __func__);
		return -EPERM;
	}

	switch (whence) {
	case SEEK_SET:
		pos = offset;
		break;
	case SEEK_CUR:
		pos = file->f_pos + offset;
		break;
	case SEEK_END:
		pos = SVFD_NV_SIZE + offset;
		break;
	default:
		return -EINVAL;
	}

	if (pos < 0 || pos > SVFD_NV_SIZE)
		return -EINVAL;

	file->f_pos = pos;
	return pos;
}

static const struct file_operations hisi_factory_svfd_ops = {
	.read = hisi_factory_svfd_read,
	.write = hisi_factory_svfd_write,
	.llseek = hisi_factory_svfd_llseek,
};

#define PERM_READ_WRITE        0660
static int hisi_factory_svfd_create_debugfs(void)
{
	struct dentry *root = NULL;
	struct dentry *data = NULL;

	root = debugfs_create_dir("hisi_factory_svfd", NULL);
	if (!root) {
		pr_err("[%s]create root dir failed\n", __func__);
		return -ENOENT;
	}
	data = debugfs_create_file("data", PERM_READ_WRITE, root, NULL,
				   &hisi_factory_svfd_ops);
	if (!data) {
		pr_err("[%s]create file failed\n", __func__);
		debugfs_remove_recursive(root);
		return -ENOENT;
	}

	return 0;
}

static int __init hisi_factory_svfd_init(void)
{
	int ret;

	ret = hisi_factory_svfd_create_debugfs();
	if (ret != 0) {
		pr_err("[%s]factory svfd init failed\n", __func__);
		return ret;
	}

	return 0;
}
late_initcall(hisi_factory_svfd_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("read factory svfd data");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
