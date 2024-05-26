/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description:
 */
#include <linux/of.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include "npu_common.h"
#include "npu_bbit_debugfs.h"
#include "npu_bbit_core.h"
#include "npu_bbit_hwts_config.h"
#include "npu_kprobe_debugfs.h"

struct npu_bbit_debugfs_fops {
	struct file_operations fops;
	const char *file_name;
};

struct npu_bbit_result {
	int valid; // indicate result valid or invalid
	int result;
};

static struct dentry *g_npu_bbit_debugfs_dir;
static struct npu_bbit_result g_npu_bbit_result;
static int npu_bbit_debugfs_show(struct seq_file *s, void *data);
static int npu_bbit_debugfs_open(struct inode *inode, struct file *file);

static const struct npu_bbit_debugfs_fops g_npu_bbit_debugfs[] = {
	{
		{
			.owner   = THIS_MODULE,
			.open	= npu_bbit_debugfs_open,
			.read	= npu_bbit_core_read,
			.write   = npu_bbit_core_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name	   = "core",
	},
	{
		{
			.owner   = THIS_MODULE,
			.open	= npu_bbit_debugfs_open,
			.read	= npu_bbit_hwts_read,
			.write   = npu_bbit_hwts_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name	   = "hiail",
	},
	{
		{
			.owner   = THIS_MODULE,
			.open	= npu_bbit_debugfs_open,
			.read	= npu_kprobe_read,
			.write   = npu_kprobe_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name	   = "kprobe",
	},
};

void npu_bbit_reset_result(void)
{
	g_npu_bbit_result.valid = 0;
	g_npu_bbit_result.result = 0;
}

void npu_bbit_set_result(int result)
{
	g_npu_bbit_result.valid = BBIT_RESULT_VALID;
	g_npu_bbit_result.result = result;
}

int npu_bbit_get_result(int *result)
{
	if (result == NULL) {
		npu_drv_warn("invalid pointer\n");
		return -1;
	}
	if (g_npu_bbit_result.valid != BBIT_RESULT_VALID) {
		npu_drv_warn("invalid result flag\n");
		return -1;
	}

	*result = g_npu_bbit_result.result;
	return 0;
}

int npu_bbit_debugfs_init(void)
{
	unsigned int i;
	struct dentry *bbit_dir = g_npu_bbit_debugfs_dir;

	npu_drv_warn("init npu bbit debugfs\n");
	if (bbit_dir != NULL) {
		npu_drv_err("invalid bbit dir\n");
		return -EINVAL;
	}

	bbit_dir = debugfs_create_dir("npu_bbit_debugfs", DEBUGFS_ROOT_DIR);
	if (IS_ERR_OR_NULL(bbit_dir)) {
		npu_drv_err("create npu_bbit_debug fail\n");
		return -EINVAL;
	}

	for (i = 0; i < sizeof(g_npu_bbit_debugfs) / sizeof(g_npu_bbit_debugfs[0]);
		i++)
		debugfs_create_file(g_npu_bbit_debugfs[i].file_name, 00660,
			bbit_dir, NULL, &(g_npu_bbit_debugfs[i].fops));

	return 0;
}

static int npu_bbit_debugfs_show(struct seq_file *s, void *data)
{
	unused(data);
	if (s == NULL)
		return -1;

	seq_printf(s, "open bbit debugfs\n");
	return 0;
}

static int npu_bbit_debugfs_open(struct inode *inode, struct file *file)
{
	if ((inode == NULL) || (file == NULL)) {
		npu_drv_err("invalid para\n");
		return -1;
	}
	npu_bbit_reset_result();
	return single_open(file, npu_bbit_debugfs_show, inode->i_private);
}

