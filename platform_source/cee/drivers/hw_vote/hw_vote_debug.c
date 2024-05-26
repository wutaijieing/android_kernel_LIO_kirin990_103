/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: Debugfs of Device driver for regulators in IC
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
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <hw_vote.h>
#include "hw_vote_private.h"
#include <securec.h>

#define HV_MAX_INPUT	100
#define HV_MAX_ARGC	4

#define hwdev_name(hvdev)	(IS_ERR_OR_NULL((hvdev)->name) ? "unknown" : (hvdev)->name)
#define CMD_REGISTER		"register"
#define CMD_REMOVE		"remove"
#define CMD_VOTE		"vote"

static struct dentry *g_hv_debug_dir;

static struct device g_test_dev = {
	.init_name = "hv_test",
};

static struct device *g_hv_test_dev = &g_test_dev;

static int hv_debugfs_show(struct seq_file *s, void *data)
{
	struct hv_channel *channel = NULL;
	struct hvdev *hvdev = NULL;
	unsigned int val, ch, member;

	(void)data;
	if (g_hv_channel_size == 0 || g_hv_channel_array == NULL) {
		pr_err("%s: hw vote not init\n", __func__);
		return -EINVAL;
	}

	hw_vote_mutex_lock();
	channel = g_hv_channel_array;
	for (ch = 0; ch < g_hv_channel_size; ch++) {
		seq_printf(s, "channel: %s\n", channel->name);
		seq_printf(s, "\t ratio: %d\n", channel->ratio);
		val = 0;
		(void)hv_reg_read(&channel->result, &val);
		seq_printf(s, "\t result: 0x%x\n", val);
		hvdev = channel->hvdev_head;
		for (member = 0; member < channel->hvdev_num; member++) {
			seq_printf(s, "\t %s", hwdev_name(hvdev));
			val = 0;
			(void)hv_reg_read(&hvdev->vote, &val);
			if (hvdev->dev != NULL)
				seq_printf(s, "(registered by %s)\n",
					   dev_name(hvdev->dev));
			else
				seq_printf(s, "(not registered)\n");

			seq_printf(s, "\t\t vote     :0x%x\n", val);
			seq_printf(s, "\t\t last_freq:%d\n", hvdev->last_set);
			hvdev++;
		}
		channel++;
	}
	hw_vote_mutex_unlock();

	return 0;
}

static int hv_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, hv_debugfs_show, inode->i_private);
}

static struct hv_channel *find_channel_by_name(const char *name)
{
	unsigned int ch;

	if (name == NULL)
		return NULL;

	for (ch = 0; ch < g_hv_channel_size; ch++) {
		if (strncmp(g_hv_channel_array[ch].name, name, CHANNEL_NAME_MAX) == 0)
			return &g_hv_channel_array[ch];
	}

	return NULL;
}

struct hvdev *find_hvdev_by_name_debug(struct hv_channel *channel,
				       const char *vsrc)
{
	struct hvdev *hvdev = channel->hvdev_head;
	unsigned int id;

	if (vsrc == NULL)
		return NULL;

	for (id = 0; id < channel->hvdev_num; id++) {
		if (strncmp(hvdev[id].name, vsrc, VSRC_NAME_MAX) == 0)
			return &hvdev[id];
	}

	return NULL;
}

static void register_cmd_handler(const char *ch_name, const char *vsrc_name)
{
	struct hvdev *test_hvdev = NULL;

	test_hvdev = hvdev_register(g_hv_test_dev, ch_name, vsrc_name);
	if (test_hvdev == NULL)
		pr_err("%s: fail to register debug hvdev\n", __func__);
}

static void remove_cmd_handler(const char *ch_name, const char *vsrc_name)
{
	struct hv_channel *channel = NULL;
	struct hvdev *test_hvdev = NULL;

	hw_vote_mutex_lock();
	channel = find_channel_by_name(ch_name);
	if (channel != NULL)
		test_hvdev = find_hvdev_by_name_debug(channel, vsrc_name);

	hw_vote_mutex_unlock();
	if (test_hvdev != NULL)
		(void)hvdev_remove(test_hvdev);
}

static void vote_cmd_handler(const char *ch_name,
			     const char *vsrc_name,
			     const char *val_str)
{
	struct hv_channel *channel = NULL;
	struct hvdev *test_hvdev = NULL;
	unsigned int freq_khz;

	hw_vote_mutex_lock();
	channel = find_channel_by_name(ch_name);
	if (channel != NULL)
		test_hvdev = find_hvdev_by_name_debug(channel, vsrc_name);

	hw_vote_mutex_unlock();
	if (test_hvdev == NULL) {
		pr_err("%s: vote not find hvdev\n", __func__);
		return;
	}
	if (sscanf_s(val_str, "%u", &freq_khz) != 1) {
		pr_err("%s: freq value is wrong\n", __func__);
		return;
	}
	(void)hv_set_freq(test_hvdev, freq_khz);
}

static const char *skipchar(const char *src, char c)
{
	while (*src == c)
		src++;

	return src;
}

static int get_token(const char *src, char *buf, int size)
{
	int num = 0;
	const char *tmp = NULL;

	if (size == 0)
		return -1;

	tmp = skipchar(src, ' ');
	while ((*tmp != '\0') && (*tmp != ' ') && (*tmp != '\n')) {
		*buf = *tmp;
		tmp++;
		buf++;
		num++;
		if (num > (size - 1)) {
			pr_err("%s  line%d  %s\n", __func__, __LINE__, tmp);
			return -1;
		}
	}

	*buf = '\0';

	return (tmp - src);
}

static ssize_t hv_debugfs_write(struct file *filp,
				     const char __user *buf,
				     size_t count,
				     loff_t *ppos)
{
	char input_cmd[HV_MAX_INPUT] = {0};
	char para[HV_MAX_ARGC][HV_MAX_INPUT] = {{0}};
	char *ptr = input_cmd;
	unsigned int argc;
	int size;

	(void)filp;
	(void)ppos;
	if (buf == NULL)
		goto out;

	if (copy_from_user(input_cmd, buf,
			   min_t(size_t, sizeof(input_cmd) - 1, count))) {
		pr_err("%s: copy buffer failed.\n", __func__);
		goto out;
	}

	for (argc = 0; argc < HV_MAX_ARGC; argc++) {
		size = get_token(ptr, (char *)(para[argc]), HV_MAX_INPUT);
		if (size <= 0)
			break;
		ptr += size;
	}

	if (strncmp(para[0], CMD_REGISTER, strlen(CMD_REGISTER)) == 0)
		register_cmd_handler(para[1], para[2]);
	else if (strncmp(para[0], CMD_REMOVE, strlen(CMD_REMOVE)) == 0)
		remove_cmd_handler(para[1], para[2]);
	else if (strncmp(para[0], CMD_VOTE, strlen(CMD_VOTE)) == 0)
		vote_cmd_handler(para[1], para[2], para[3]);

out:
	return count;
}

static const struct file_operations hv_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = hv_debugfs_open,
	.read = seq_read,
	.write = hv_debugfs_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init hv_debugfs_init(void)
{
	int ret = -ENODEV;

	g_hv_debug_dir = debugfs_create_dir("hv_debug", NULL);
	if (g_hv_debug_dir != NULL) {
		if (debugfs_create_file("hv_debug", 0660, g_hv_debug_dir, NULL,
					&hv_debugfs_fops) == NULL) {
			debugfs_remove_recursive(g_hv_debug_dir);
			goto out;
		}
	} else {
		pr_err("[%s]create g_hv_debug_dir fail\n", __func__);
		goto out;
	}

	return 0;
out:
	return ret;
}
fs_initcall(hv_debugfs_init);

static void __exit hv_debugfs_exit(void)
{
	debugfs_remove_recursive(g_hv_debug_dir);
}
module_exit(hv_debugfs_exit);

MODULE_DESCRIPTION("hardware vote");
MODULE_LICENSE("GPL v2");
