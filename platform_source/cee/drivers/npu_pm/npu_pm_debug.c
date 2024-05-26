/*
 * npu_pm_debug.c
 *
 * npu debugfs
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
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
#include "npu_pm_private.h"

struct npu_pm_debugfs_fops {
	struct file_operations fops;
	const char *file_name;
};

static struct npu_pm_dvfs_data *g_dvfs_debug_data;
static struct npu_pm_device *g_npu_pm_debug_dev;

static void npu_pm_profile_debug_show_div_reg_info(struct seq_file *s,
						     struct clk_div_regs *div_reg)
{
	seq_printf(s, "\t rd_mask 0x%x\n", div_reg->rd_mask);
	seq_printf(s, "\t wr_mask 0x%x\n", div_reg->wr_mask);
}

static void npu_pm_profile_debug_show_profile_table(struct seq_file *s,
						      struct npu_pm_module_profile *module_profile)
{
	int index;
	struct npu_pm_profile *profile = module_profile->profile_table;

	for (index = 0; index < module_profile->profile_count; index++) {
		seq_printf(s, "profile%d:\n", index);
		seq_printf(s, "\t freq    %u\n", profile->freq);
		seq_printf(s, "\t voltage %u\n", profile->voltage);
		seq_printf(s, "\t pll     %u\n", profile->pll);
		seq_printf(s, "\t div     %u\n", profile->div);
		profile++;
	}
}

static void npu_pm_profile_debug_show_module(struct seq_file *s,
					       struct npu_pm_module_profile *module_profile)
{
	seq_printf(s, "\n[%s]\n", module_profile->module_name);

	seq_printf(s, "clk_switch:\n");
	npu_pm_profile_debug_show_div_reg_info(s,
						 &module_profile->clk_switch);

	seq_printf(s, "clk_div:\n");
	npu_pm_profile_debug_show_div_reg_info(s, &module_profile->clk_div);

	seq_printf(s, "clk_div_gate:\n");
	npu_pm_profile_debug_show_div_reg_info(s,
						 &module_profile->clk_div_gate);

	seq_printf(s, "low temp freq   :%u\n", module_profile->lt_freq_uplimit);
	seq_printf(s, "low temp voltage:%u\n", module_profile->voltage_lt);

	if (IS_ERR_OR_NULL(module_profile->buck)) {
		seq_printf(s, "buck: null\n");
	} else {
		seq_printf(s, "buck step time:%d\n",
			   module_profile->buck_step_time);
		seq_printf(s, "buck hold time:%d\n",
			   module_profile->buck_hold_time);
	}

	npu_pm_profile_debug_show_profile_table(s, module_profile);
}

static int npu_pm_profile_debugfs_show(struct seq_file *s, void *data)
{
	int index;

	if (IS_ERR_OR_NULL(g_dvfs_debug_data))
		return 0;

	/* main module */
	npu_pm_profile_debug_show_module(s, g_dvfs_debug_data->main_module);

	for (index = 0; index < g_dvfs_debug_data->module_count; index++)
		if (g_dvfs_debug_data->main_module !=
		    &g_dvfs_debug_data->module_profile[index])
			npu_pm_profile_debug_show_module(s, &g_dvfs_debug_data->module_profile[index]);

	return 0;
}

static int npu_pm_profile_debugfs_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, npu_pm_profile_debugfs_show,
			   inode->i_private);
}

static ssize_t npu_pm_profile_debugfs_write(struct file *filp,
					      const char __user *buf,
					      size_t count,
					      loff_t *ppos)
{
	return count;
}

static void npu_pm_dvfs_debugfs_module_show(struct seq_file *s,
					      struct npu_pm_module_profile *module_profile)
{
	u32 clk_switch = 0;
	u32 clk_div = 0;
	u32 clk_div_gate = 0;
	unsigned long clk_rate;
	int voltage;
	int ret;

	ret = _hal_reg_read(&module_profile->clk_switch, &clk_switch);
	ret += _hal_reg_read(&module_profile->clk_div, &clk_div);
	ret += _hal_reg_read(&module_profile->clk_div_gate, &clk_div_gate);
	if (ret != 0) {
		pr_err("npu dvfs clk reg read fail\n");
		return;
	}

	clk_rate = npu_pm_get_cur_clk_rate(module_profile);
	voltage = npu_pm_get_module_voltage(module_profile) / K_RATIO;
	seq_printf(s, "[%s]:%luHz %dmV\n", module_profile->module_name,
		   clk_rate, voltage);
	seq_printf(s, "\t clk_switch   0x%x\n", clk_switch);
	seq_printf(s, "\t clk_div      0x%x\n", clk_div);
	seq_printf(s, "\t clk_div_gate 0x%x\n", clk_div_gate);
}

static int npu_pm_dvfs_debugfs_show(struct seq_file *s, void *data)
{
	int index;

	if (IS_ERR_OR_NULL(g_dvfs_debug_data))
		return 0;

	seq_printf(s, "npu dvfs state:\n");
	seq_printf(s, "dvfs max time:%lu\n", g_dvfs_debug_data->max_dvfs_time);

	/* main module */
	npu_pm_dvfs_debugfs_module_show(s, g_dvfs_debug_data->main_module);

	for (index = 0; index < g_dvfs_debug_data->module_count; index++)
		if (g_dvfs_debug_data->main_module !=
		    &g_dvfs_debug_data->module_profile[index])
			npu_pm_dvfs_debugfs_module_show(s, &g_dvfs_debug_data->module_profile[index]);

	return 0;
}

static bool npu_pm_check_freq_info(const void *data,
				     char (*name)[MAX_MODULE_NAME_LEN])
{
	if (IS_ERR_OR_NULL(g_npu_pm_debug_dev) ||
	    IS_ERR_OR_NULL(data) ||
	    IS_ERR_OR_NULL(g_dvfs_debug_data) ||
	    IS_ERR_OR_NULL(name)) {
		pr_err("NPU_PM init failed\n");
		return false;
	}

	return true;
}

int get_npu_freq_info(void *data, int size,
		      char (*name)[MAX_MODULE_NAME_LEN], int size_name)
{
	int ret;
	int index;
	int length;
	int module_cnt;
	unsigned int npu[MAX_MODULE_COUNT][2];
	struct npu_pm_module_profile *module_profile = NULL;

	if (!npu_pm_check_freq_info(data, name))
		return -EINVAL;

	module_cnt = g_dvfs_debug_data->module_count < MAX_MODULE_COUNT ?
		     g_dvfs_debug_data->module_count : MAX_MODULE_COUNT;

	/*
	 * Get the NPU module name info and share to freqdump tool,
	 * currently support MAX 6 modules, Max name length is 9 bytes
	 */
	for (index = 0; index < module_cnt && (MAX_MODULE_NAME_LEN * index) < size_name; index++) {
		module_profile = &g_dvfs_debug_data->module_profile[index];
		if (strlen(module_profile->module_name) < MAX_MODULE_NAME_LEN) {
			ret = strcpy_s((char *)(name + index),
				       MAX_MODULE_NAME_LEN,
				       module_profile->module_name);
			if (ret != EOK) {
				pr_err("NPU_PM strcpy_s failed, ret%d\n", ret);
				return ret;
			}
		} else {
			ret = strncpy_s((char *)(name + index),
					MAX_MODULE_NAME_LEN,
					module_profile->module_name,
					MAX_MODULE_NAME_LEN - 1);
			if (ret != EOK) {
				pr_err("NPU_PM strncpy_s failed, ret%d\n", ret);
				return ret;
			}
			((char *)(name + index))[MAX_MODULE_NAME_LEN - 1] = '\0';
		}
	}

	ret = memset_s(npu, sizeof(npu), 0, sizeof(npu));
	if (ret != EOK) {
		pr_err("NPU_PM memset_s failed, ret = %d\n", ret);
		return ret;
	}
	/*
	 * Power on status return NPU_subsys clock info,
	 * else set clock info 0
	 */
	if (g_npu_pm_debug_dev->power_on) {
		for (index = 0; index < module_cnt; index++) {
			module_profile =
				&g_dvfs_debug_data->module_profile[index];
			npu[index][0] =
				npu_pm_get_cur_clk_rate(module_profile);
			npu[index][1] =
				(unsigned int)npu_pm_get_module_voltage(module_profile) /
				1000;
		}
	}
	length = min_t(int, (int)sizeof(npu), size);
	ret = memcpy_s(data, size, (void *)npu, length);
	if (ret != EOK) {
		pr_err("NPU_PM memcpy_s failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int npu_pm_dvfs_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, npu_pm_dvfs_debugfs_show,
			   inode->i_private);
}

static ssize_t npu_pm_dvfs_debugfs_write(struct file *filp,
					   const char __user *buf,
					   size_t count,
					   loff_t *ppos)
{
	return count;
}

static int npu_power_debugfs_show(struct seq_file *s, void *data)
{
	if (IS_ERR_OR_NULL(g_npu_pm_debug_dev))
		return 0;

	seq_printf(s, "npu: %s\n", g_npu_pm_debug_dev->power_on ? "on" : "off");
	seq_printf(s, "\t on :%u\n", g_npu_pm_debug_dev->power_on_count);
	seq_printf(s, "\t off:%u\n", g_npu_pm_debug_dev->power_off_count);
	seq_printf(s, "\t on max time:%lu\n",
		   g_npu_pm_debug_dev->max_pwron_time);
	seq_printf(s, "\t off max time:%lu\n",
		   g_npu_pm_debug_dev->max_pwroff_time);

	return 0;
}

static int npu_power_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, npu_power_debugfs_show,
			   inode->i_private);
}

#define NPU_POWER_CMD_MAX	4
static ssize_t npu_power_debugfs_write(struct file *filp,
					    const char __user *buf,
					    size_t count,
					    loff_t *ppos)
{
	char input[NPU_POWER_CMD_MAX] = {0};
	int ret = 0;

	if (IS_ERR_OR_NULL(buf))
		goto out;

	if (copy_from_user(input, buf,
			   min_t(size_t, sizeof(input) - 1, count))) {
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
	return single_open(file, npu_test_debugfs_show,
			   inode->i_private);
}

#define NPU_TEST_CMD_MAX	7
static ssize_t npu_test_debugfs_write(struct file *filp,
					   const char __user *buf,
					   size_t count, loff_t *ppos)
{
	char input[NPU_TEST_CMD_MAX] = {0};

	if (IS_ERR_OR_NULL(buf))
		goto out;

	if (copy_from_user(input, buf,
			   min_t(size_t, sizeof(input) - 1, count))) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto out;
	}

	if (strncmp(input, "debug", 5) == 0)
		g_dvfs_debug_data->test |= BIT(NPU_LOW_TEMP_DEBUG);
	else if (strncmp(input, "cancel", 6) == 0)
		g_dvfs_debug_data->test = 0;
	else if (strncmp(input, "low", 3) == 0)
		g_dvfs_debug_data->test |= BIT(NPU_LOW_TEMP_STATE);
	else if (strncmp(input, "normal", 6) == 0)
		g_dvfs_debug_data->test &= ~BIT(NPU_LOW_TEMP_STATE);

out:
	return count;
}

static int npu_voltage_debugfs_show(struct seq_file *s, void *data)
{
	return 0;
}

static int npu_voltage_debugfs_open(struct inode *inode,
					 struct file *file)
{
	return single_open(file, npu_voltage_debugfs_show,
			   inode->i_private);
}

#define NPU_VOLTAGE_CMD_MAX	50
static ssize_t npu_voltage_debugfs_write(struct file *filp,
					      const char __user *buf,
					      size_t count, loff_t *ppos)
{
	char input[NPU_VOLTAGE_CMD_MAX] = {'\0'};
	char name[NPU_VOLTAGE_CMD_MAX] = {'\0'};
	struct npu_pm_module_profile *module_profile = NULL;
	int index;
	unsigned int voltage = 0;
	int ret;

	if (IS_ERR_OR_NULL(buf))
		goto out;

	if (copy_from_user(input, buf,
			   min_t(size_t, sizeof(input) - 1, count))) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto out;
	}

	ret = sscanf_s(input, "%s%u", name, NPU_VOLTAGE_CMD_MAX - 1, &voltage);
	if (ret != 2) {
		pr_err("%s: para wrong\n", __func__);
		goto out;
	}

	for (index = 0; index < g_dvfs_debug_data->module_count; index++) {
		module_profile = &g_dvfs_debug_data->module_profile[index];

		if (strncmp(name, module_profile->module_name,
			    NPU_VOLTAGE_CMD_MAX) == 0) {
			ret = npu_pm_set_module_voltage(module_profile,
							  voltage);
			if (ret)
				pr_err("%s set voltage%u fail\n",
				       __func__, voltage);
		}
	}
out:
	return count;
}

static const struct npu_pm_debugfs_fops g_npu_pm_debugfs[] = {
	{
		{
			.owner   = THIS_MODULE,
			.open    = npu_pm_profile_debugfs_open,
			.read    = seq_read,
			.write   = npu_pm_profile_debugfs_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name       = "profile",
	},
	{
		{
			.owner   = THIS_MODULE,
			.open    = npu_pm_dvfs_debugfs_open,
			.read    = seq_read,
			.write   = npu_pm_dvfs_debugfs_write,
			.llseek  = seq_lseek,
			.release = single_release,
		},
		.file_name       = "dvfs",
	},
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
};

static struct dentry *npu_pm_debug_dir;

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
				    00660, npu_pm_debug_dir,
				    NULL, &g_npu_pm_debugfs[i].fops);

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
