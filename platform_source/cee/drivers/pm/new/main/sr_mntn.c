/*
 * sr_mntn.c
 *
 * suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
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

#include "pm.h"
#include "pub_def.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/debugfs/debugfs.h"

#include <securec.h>

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>

static PLIST_HEAD(g_sr_ops_list);
static DEFINE_MUTEX(g_sr_ops_lock);

static bool is_ops_exist(struct sr_mntn *new)
{
	struct sr_mntn *ops = NULL;

	plist_for_each_entry(ops, &g_sr_ops_list, node) {
		if (ops == new)
			return true;
	}

	return false;
}

int register_sr_mntn(struct sr_mntn *ops, enum sr_mntn_prio prio)
{
	if (unlikely((ops == NULL) || (ops->owner == NULL)))
		return -EINVAL;

	if (unlikely(prio > SR_MNTN_PRIO_L)) {
		lp_err("register %s failed: invalid prio:%d", ops->owner, prio);
		return -EINVAL;
	}

	plist_node_init(&ops->node, prio);

	mutex_lock(&g_sr_ops_lock);

	if (is_ops_exist(ops)) {
		mutex_unlock(&g_sr_ops_lock);
		lp_err("register %s failed: alread exist", ops->owner);
		return -EINVAL;
	}

	plist_add(&ops->node, &g_sr_ops_list);

	mutex_unlock(&g_sr_ops_lock);

	lp_crit("%s register success, prio is %d", ops->owner, prio);

	return 0;
}

void unregister_sr_mntn(struct sr_mntn *ops)
{
	if (unlikely((ops == NULL)))
		return;

	mutex_lock(&g_sr_ops_lock);

	plist_del(&ops->node, &g_sr_ops_list);

	mutex_unlock(&g_sr_ops_lock);

	lp_crit("%s unregister success", ops->owner);
}

void lowpm_sr_mntn_suspend(void)
{
	struct sr_mntn *ops = NULL;
	int ret;

	mutex_lock(&g_sr_ops_lock);

	plist_for_each_entry(ops, &g_sr_ops_list, node) {
		if (ops->suspend == NULL)
			continue;

		if (!ops->enable) {
			lp_info("sr [%s] ops disabled", ops->owner);
			continue;
		}

		ret = ops->suspend();
		if (ret != 0)
			lp_err("sr [%s] ops suspend failed", ops->owner);
	}

	mutex_unlock(&g_sr_ops_lock);
}

void lowpm_sr_mntn_resume(void)
{
	struct sr_mntn *ops = NULL;
	int ret;

	mutex_lock(&g_sr_ops_lock);

	plist_for_each_entry(ops, &g_sr_ops_list, node) {
		if (ops->resume == NULL)
			continue;

		if (!ops->enable) {
			lp_info("sr [%s] ops disabled", ops->owner);
			continue;
		}

		ret = ops->resume();
		if (ret != 0)
			lp_err("sr [%s] ops resume failed", ops->owner);
	}

	mutex_unlock(&g_sr_ops_lock);
}

static void sr_mntn_print_callback(struct seq_file *s, int width, unsigned long addr)
{
	char buf[KSYM_SYMBOL_LEN] = "not register";

	if (addr != 0)
		(void)sprint_symbol(buf, addr);

	lp_msg_cont(s, "%*s", width, buf);
}

static void print_usage(struct seq_file *s)
{
	lp_msg(s, "%s", "Usage:");
	lp_msg(s, "%s", "to enable a mntn module: echo \"enable xxx\" > sr_mntn");
	lp_msg(s, "%s", "to disable a mntn module: echo \"disable xxx\" > sr_mntn");
	lp_msg(s, "%s", "to get a mntn module: cat sr_mntn");
	lp_msg(s, "%s", "Note: xxx is the column mntn_module of \"cat sr_mntn\"");
}

static int sr_mntn_show(struct seq_file *s, void *data)
{
	struct sr_mntn *ops = NULL;
	const int align_left = -1;
	const int name_width_align_left = 16 * align_left;
	const int prio_width_align_left = 12 * align_left;
	const int status_width_align_left = 16 * align_left;
	const int callback_width_align_left = 32 * align_left;

	no_used(data);

	lp_msg(s, "%*s%*s%*s%*s%*s",
	       name_width_align_left, "mntn_module",
	       prio_width_align_left, "prio",
	       status_width_align_left, "status",
	       callback_width_align_left, "suspend",
	       callback_width_align_left, "resume");

	mutex_lock(&g_sr_ops_lock);

	plist_for_each_entry(ops, &g_sr_ops_list, node) {
		if (ops == NULL) {
			lp_msg(s, "null position");
			continue;
		}
		lp_msg_cont(s, "%*s", name_width_align_left, ops->owner);
		lp_msg_cont(s, "%*d", prio_width_align_left, ops->node.prio);
		lp_msg_cont(s, "%*s", status_width_align_left, ops->enable ? "enable" : "disable");
		sr_mntn_print_callback(s, callback_width_align_left, (uintptr_t)ops->suspend);
		sr_mntn_print_callback(s, callback_width_align_left, (uintptr_t)ops->resume);

		lp_msg_cont(s, "\n");
	}

	mutex_unlock(&g_sr_ops_lock);

	print_usage(s);

	return 0;
}

static int set_sr_mntn_status(const char *status, const char *name)
{
	bool enable = false;
	struct sr_mntn *ops = NULL;

	if (strcmp(status, "enable") == 0)
		enable = true;
	else if (strcmp(status, "disable") == 0)
		enable = false;
	else
		return -EINVAL;

	mutex_lock(&g_sr_ops_lock);

	plist_for_each_entry(ops, &g_sr_ops_list, node) {
		if (strcmp(name, ops->owner) == 0) {
			ops->enable = enable;
			mutex_unlock(&g_sr_ops_lock);
			return 0;
		}
	}

	mutex_unlock(&g_sr_ops_lock);

	lp_err("no such module: %s", name);

	return -EINVAL;
}

#define STATUS_BUF_LEN	8
#define NAME_BUF_LEN	16
#define INPUT_BUF_LEN	(STATUS_BUF_LEN + NAME_BUF_LEN + 2)

static ssize_t sr_mntn_cfg(struct seq_file *s, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	const int buf_len = STATUS_BUF_LEN + NAME_BUF_LEN;
	const int correct_columns = 2;

	char input_buf[INPUT_BUF_LEN] = {0};
	char status_buf[STATUS_BUF_LEN + 1] = {0};
	char name_buf[NAME_BUF_LEN + 1] = {0};

	if (count > buf_len) {
		lp_err("the input content is too long");
		return -EINVAL;
	}

	if (buffer != NULL && copy_from_user(input_buf, buffer, count) != 0) {
		lp_err("internal error: copy mem fail");
		return -ENOMEM;
	}
	input_buf[count] = '\0';

	if (sscanf_s(input_buf, "%s %s", status_buf, STATUS_BUF_LEN, name_buf, NAME_BUF_LEN) != correct_columns) {
		lp_err("invalid input");
		return -EINVAL;
	}

	if (set_sr_mntn_status(status_buf, name_buf) != 0) {
		lp_err("config failed: %s", input_buf);
		return -EFAULT;
	}

	*ppos += count;

	lp_info("%s success", input_buf);

	return count;
}

static const struct lowpm_debugdir g_lpwpm_debugfs_sr_mntn = {
	.dir = "lowpm_func",
	.files = {
		{"sr_mntn", 0600, sr_mntn_show, sr_mntn_cfg},
		{},
	},
};

static __init int init_sr_mntn(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_sr_mntn);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn);
