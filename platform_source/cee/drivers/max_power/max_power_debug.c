/*
 * hisi_max_power_debug.c
 *
 * max power driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#include "max_power.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <securec.h>

#define MODULE_NAME	"hisilicon,max_power"

#define MAX_INPUT_CNT	128
#define MAX_ARG		10
#define MAX_POWER_ARG	3
#define MAX_POWER_TYPE_ARG	1
#define MAX_POWER_TYPE		0
#define OUTER_LOOP_INDEX	0
#define INTER_LOOP_INDEX	1
#define GPIO_NUM		2
#define MAX_OPS		1

static struct dentry *g_max_power_dir;
static int g_max_power_type;

static int hisi_max_power_debugfs_show(struct seq_file *s, void *data)
{
	pr_debug("%s: %s\n", MODULE_NAME, __func__);
	return 0;
}

static int hisi_max_power_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, hisi_max_power_debugfs_show, inode->i_private);
}

static int parse_para(char *para_cmd, char *argv[], int max_args)
{
	int para_id = 0;

	while (*para_cmd != '\0') {
		if (para_id >= max_args)
			break;
		while (*para_cmd == ' ')
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		argv[para_id] = para_cmd;
		para_id++;

		while ((*para_cmd != ' ') && (*para_cmd != '\0'))
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		*para_cmd = '\0';
		para_cmd++;
	}
	return para_id;
}

static ssize_t init_hisi_max_power_plat(char *argv[], ssize_t count)
{
	int type;
	int ret;

	ret = kstrtou32(argv[MAX_POWER_TYPE], 0, &type);
	if (ret != 0) {
		pr_err("%s: cmd chip_type error\n", __func__);
		return -EINVAL;
	}
	g_max_power_type = type;
	max_power_int(g_max_power_type);
	pr_err("%s:chip_type:%d.\n", __func__, g_max_power_type);
	return count;
}

static ssize_t run_max_power(char *argv[], ssize_t count)
{
	u32 outer_count;
	u32 inter_count;
	u32 gpio_num_value;
	int ret;

	ret = kstrtou32(argv[OUTER_LOOP_INDEX], 0, &outer_count);
	if (ret != 0) {
		pr_err("%s: cmd outer_loop error\n", __func__);
		goto max_power_err;
	}

	ret = kstrtou32(argv[INTER_LOOP_INDEX], 0, &inter_count);
	if (ret != 0) {
		pr_err("%s: cmd inter_loop error\n", __func__);
		goto max_power_err;
	}

	ret = kstrtou32(argv[GPIO_NUM], 0, &gpio_num_value);
	if (ret != 0) {
		pr_err("%s: cmd gpio_num error\n", __func__);
		goto max_power_err;
	}

	max_power(outer_count, inter_count, gpio_num_value, g_max_power_type);
	pr_err("%s,count:%d\n", __func__, (int)count);
	return count;
max_power_err:
	return -EINVAL;
}

static ssize_t hisi_max_power_debugfs_write(struct file *filp,
					    const char __user *buf,
					    size_t count, loff_t *ppos)
{
	char debug_cmd[MAX_INPUT_CNT] = {0};
	int argc;
	char *argv[MAX_ARG] = {0};
	int ret;

	if (buf == NULL) {
		pr_err("error, buf is null\n");
		goto max_power_err;
	}

	if (copy_from_user(debug_cmd, buf,
			   min_t(size_t, sizeof(debug_cmd) - 1, count)) != 0) {
		pr_err("%s: copy buffer failed.\n", MODULE_NAME);
		goto max_power_err;
	}

	argc = parse_para(debug_cmd, argv, MAX_ARG);
	if (argc == MAX_POWER_TYPE_ARG)
		return init_hisi_max_power_plat(argv, count);

	if (argc < MAX_POWER_ARG) {
		pr_err("error, arg too few\n");
		goto max_power_err;
	}

	ret = run_max_power(argv, count);
	return ret;
max_power_err:
	return -EINVAL;
}

static const struct file_operations g_fops[MAX_OPS] = {
	{
		.open		= hisi_max_power_debugfs_open,
		.read		= seq_read,
		.write		= hisi_max_power_debugfs_write,
		.llseek		= seq_lseek,
		.release	= single_release,
	},
};

static char *g_option_str[MAX_OPS] = {
	"max_power_test",
};

static int __init hisi_max_power_init(void)
{
	int i, ret = 0;

	pr_err("%s:begin.\n", __func__);
	g_max_power_dir = debugfs_create_dir("hisi_max_power_debug", NULL);
	if (g_max_power_dir) {
		for (i = 0; i < MAX_OPS; i++) {
			if (debugfs_create_file(g_option_str[i], 0660,
						g_max_power_dir, NULL,
						&g_fops[i]) == NULL) {
				debugfs_remove_recursive(g_max_power_dir);
				ret = -ENODEV;
				goto err_alloc_nb_nor;
			}
		}
	} else {
		ret = -ENODEV;
		goto err_alloc_nb_nor;
	}
	return 0;

err_alloc_nb_nor:
	return ret;
}

fs_initcall(hisi_max_power_init);

static void __exit hisi_max_power_exit(void)
{
	if (g_max_power_dir != NULL)
		debugfs_remove_recursive(g_max_power_dir);
	g_max_power_dir = NULL;
}

module_exit(hisi_max_power_exit);

MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_DESCRIPTION("MAX POWER DEBUG DRIVER");
MODULE_LICENSE("GPL v2");
