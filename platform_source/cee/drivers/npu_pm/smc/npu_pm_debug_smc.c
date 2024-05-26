/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: npu debugfs
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/pm_opp.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <securec.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <npu_pm.h>
#include "npu_pm_smc.h"
#include "npu_pm_private.h"

#define NPU_POWER_CMD_MAX	4
#define NPU_TEST_CMD_MAX	7
#define NPU_VOLTAGE_CMD_MAX	50

#define LOG_EN		1
#define LOG_DIS		0

struct npu_pm_debugfs_fops {
	const struct file_operations fops;
	const char *file_name;
};

static struct npu_pm_dvfs_data *g_dvfs_debug_data;
static struct npu_pm_device *g_npu_pm_debug_dev;
static struct dentry *npu_pm_debug_dir;

static int npu_power_debugfs_show(struct seq_file *s, void *data)
{
	if (IS_ERR_OR_NULL(g_npu_pm_debug_dev))
		return 0;

	seq_printf(s,
		   "npu: %s\n",
		   g_npu_pm_debug_dev->power_on ? "on" : "off");
	seq_printf(s, "\t on :%u\n", g_npu_pm_debug_dev->power_on_count);
	seq_printf(s, "\t off:%u\n", g_npu_pm_debug_dev->power_off_count);
	seq_printf(s,
		   "\t on max time:%lu\n",
		   g_npu_pm_debug_dev->max_pwron_time);
	seq_printf(s,
		   "\t off max time:%lu\n",
		   g_npu_pm_debug_dev->max_pwroff_time);
	seq_printf(s,
		   "\t dvfs max time:%lu\n",
		   g_dvfs_debug_data->max_dvfs_time);

	return 0;
}

static int npu_power_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, npu_power_debugfs_show, inode->i_private);
}

static ssize_t npu_power_debugfs_write(struct file *filp,
					    const char __user *buf,
					    size_t count,
					    loff_t *ppos)
{
	char input[NPU_POWER_CMD_MAX] = {0};
	int ret;

	if (IS_ERR_OR_NULL(buf))
		goto out;

	ret = copy_from_user(input,
			     buf,
			     min_t(size_t, sizeof(input) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto out;
	}

	if (strncmp(input, "on", 2) == 0)
		ret = npu_pm_power_on();
	else if (strncmp(input, "off", 3) == 0)
		ret = npu_pm_power_off();

	if (ret != 0)
		pr_err("%s: pwr failed %d.\n", __func__, ret);
out:
	return count;
}

static int npu_test_debugfs_show(struct seq_file *s, void *data)
{
	if (IS_ERR_OR_NULL(g_dvfs_debug_data))
		return 0;

	seq_printf(s, "0x%x\n", g_dvfs_debug_data->test);

	return 0;
}

static int npu_test_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, npu_test_debugfs_show, inode->i_private);
}

static ssize_t npu_test_debugfs_write(struct file *filp,
					   const char __user *buf,
					   size_t count,
					   loff_t *ppos)
{
	char input[NPU_TEST_CMD_MAX] = {0};
	unsigned int ret;

	if (IS_ERR_OR_NULL(buf))
		goto out;

	ret = copy_from_user(input,
			     buf,
			     min_t(size_t, sizeof(input) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto out;
	}

	if (strncmp(input, "debug", strlen("debug")) == 0)
		g_dvfs_debug_data->test |= BIT(NPU_LOW_TEMP_DEBUG);
	else if (strncmp(input, "cancel", strlen("cancel")) == 0)
		g_dvfs_debug_data->test = 0;
	else if (strncmp(input, "low", strlen("low")) == 0)
		g_dvfs_debug_data->test |= BIT(NPU_LOW_TEMP_STATE);
	else if (strncmp(input, "normal", strlen("normal")) == 0)
		g_dvfs_debug_data->test &= ~BIT(NPU_LOW_TEMP_STATE);
	else if (strncmp(input, "logen", strlen("logen")) == 0)
		npu_pm_dbg_smc_request(LOG_EN);
	else if (strncmp(input, "logdis", strlen("logdis")) == 0)
		npu_pm_dbg_smc_request(LOG_DIS);

out:
	return count;
}

static int npu_voltage_debugfs_show(struct seq_file *s, void *data)
{
	int voltage;

	voltage = npu_pm_get_voltage_smc_request();

	seq_printf(s, "%d\n", voltage);

	return 0;
}

static int npu_voltage_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file,
			   npu_voltage_debugfs_show,
			   inode->i_private);
}

static ssize_t npu_voltage_debugfs_write(struct file *filp,
					      const char __user *buf,
					      size_t count,
					      loff_t *ppos)
{
	char input[NPU_VOLTAGE_CMD_MAX] = {'\0'};
	unsigned int voltage = 0;
	int ret;

	if (IS_ERR_OR_NULL(buf))
		return count;

	ret = copy_from_user(input,
			     buf,
			     min_t(size_t, sizeof(input) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", __func__);
		return count;
	}

	ret = sscanf_s(input, "%u", &voltage);
	if (ret != 1) {
		pr_err("%s: para wrong\n", __func__);
		return count;
	}

	ret = npu_pm_set_voltage_smc_request(voltage);
	if (ret != 0)
		pr_err("%s set npu voltage fail%d\n", __func__, ret);

	return count;
}

static int npu_temp_debugfs_show(struct seq_file *s, void *data)
{
	if (IS_ERR_OR_NULL(g_dvfs_debug_data))
		return 0;

	seq_printf(s, "%d\n", g_dvfs_debug_data->temp_test);

	return 0;
}

static int npu_temp_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, npu_temp_debugfs_show, inode->i_private);
}

static ssize_t npu_temp_debugfs_write(struct file *filp,
					   const char __user *buf,
					   size_t count,
					   loff_t *ppos)
{
	char input[NPU_VOLTAGE_CMD_MAX] = {'\0'};
	int temp = 0;
	int ret;

	if (IS_ERR_OR_NULL(buf))
		return count;

	ret = (int)copy_from_user(input,
			     buf,
			     min_t(size_t, sizeof(input) - 1, count));
	if (ret != 0) {
		pr_err("%s: copy buffer failed.\n", __func__);
		return count;
	}

	ret = sscanf_s(input, "%d", &temp);
	if (ret != 1) {
		pr_err("%s: para wrong\n", __func__);
		return count;
	}

	g_dvfs_debug_data->temp_test = temp;

	return count;
}

static const struct npu_pm_debugfs_fops g_npu_pm_debugfs[] = {
	{
		{
			.owner   = THIS_MODULE,
			.open    = npu_power_debugfs_open,
			.read    = seq_read,
			.write   = npu_power_debugfs_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name       = "power",
	},
	{
		{
			.owner   = THIS_MODULE,
			.open    = npu_test_debugfs_open,
			.read    = seq_read,
			.write   = npu_test_debugfs_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name       = "test",
	},
	{
		{
			.owner   = THIS_MODULE,
			.open    = npu_voltage_debugfs_open,
			.read    = seq_read,
			.write   = npu_voltage_debugfs_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name       = "voltage",
	},
	{
		{
			.owner   = THIS_MODULE,
			.open    = npu_temp_debugfs_open,
			.read    = seq_read,
			.write   = npu_temp_debugfs_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name       = "temp_test",
	},
};

void npu_pm_debugfs_init(struct npu_pm_device *pmdev)
{
	unsigned int i;

	if (npu_pm_debug_dir != NULL)
		return;

	npu_pm_debug_dir = debugfs_create_dir("npu_pm_debug", NULL);
	if (IS_ERR_OR_NULL(npu_pm_debug_dir)) {
		pr_err("[%s]create npu_pm_debug_dir fail\n", __func__);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(g_npu_pm_debugfs); i++)
		debugfs_create_file(g_npu_pm_debugfs[i].file_name,
				    0660,
				    npu_pm_debug_dir,
				    NULL,
				    &g_npu_pm_debugfs[i].fops);

	g_dvfs_debug_data = pmdev->dvfs_data;
	g_npu_pm_debug_dev = pmdev;
}

void npu_pm_debugfs_exit(void)
{
	if (npu_pm_debug_dir != NULL)
		debugfs_remove_recursive(npu_pm_debug_dir);

	g_dvfs_debug_data = NULL;
	g_npu_pm_debug_dev = NULL;
}
