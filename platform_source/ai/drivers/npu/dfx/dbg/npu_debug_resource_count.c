/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * npu_debug_resource_count.c
 *
 * about npu debug resource count
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
#include "npu_debug_resource_count.h"

#include <linux/of.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/debugfs.h>

#include "npu_dfx.h"
#include "npu_resmem.h"
#include "npu_log.h"
#include "npu_platform_register.h"
#include "npu_common.h"
#include "npu_id_allocator.h"
#include "npu_proc_ctx.h"

struct npu_resource_debugfs_fops {
	struct file_operations fops;
	const char *file_name;
};

typedef struct npu_id_type_to_name {
	npu_id_type type;
	char *name;
} npu_id_type_to_name_t;

static const npu_id_type_to_name_t npu_id_type_name_map[] = {
	{ NPU_ID_TYPE_STREAM,            "non_sink_stream" },
	{ NPU_ID_TYPE_SINK_LONG_STREAM,  "sink_long_stream" },
	{ NPU_ID_TYPE_SINK_STREAM,       "sink_stream" },
	{ NPU_ID_TYPE_MODEL,             "model" },
	{ NPU_ID_TYPE_TASK,              "task" },
	{ NPU_ID_TYPE_EVENT,             "event" },
	{ NPU_ID_TYPE_HWTS_EVENT,        "hwts_event" },
	{ NPU_ID_TYPE_NOTIFY,            "notify" },
	{ NPU_ID_TYPE_MAX,               "undefined" }
};

static char *npu_id_type_to_string(npu_id_type type)
{
	u16 idx;

	for (idx = 0; idx < NPU_ID_TYPE_MAX; idx += 1) {
		if (type == npu_id_type_name_map[idx].type)
			return npu_id_type_name_map[idx].name;
	}

	return "undefined";
}

static struct dentry *npu_resource_debug_dir;

static const struct npu_resource_debugfs_fops g_npu_resource_debugfs[] = {
	{
		{
			.owner   = THIS_MODULE,
			.open = npu_resource_debugfs_open,
			.read = seq_read,
			.llseek = seq_lseek,
			.release = single_release,
		},
		.file_name = "resource",
	},
};

int npu_debug_init(void)
{
	unsigned int i;

	if (npu_resource_debug_dir != NULL)
		return -EINVAL;

	npu_resource_debug_dir = debugfs_create_dir("npu_debug", NULL);
	if (IS_ERR_OR_NULL(npu_resource_debug_dir)) {
		npu_drv_err("create npu_debug_dir fail\n");
		return -EINVAL;
	}

	for (i = 0; i < sizeof(g_npu_resource_debugfs) /
		sizeof(g_npu_resource_debugfs[0]); i++)
		debugfs_create_file(g_npu_resource_debugfs[i].file_name, 00660,
			npu_resource_debug_dir, NULL,
			&(g_npu_resource_debugfs[i].fops));

	return 0;
}

int list_count(struct list_head *list)
{
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	int counter = 0;

	if (list_empty_careful(list)) {
		npu_drv_warn("list is empty\n");
		return 0;
	}
	list_for_each_safe(pos, n, list)
		counter++;
	return counter;
}

static void npu_resource_debugfs_print_proc(struct seq_file *s,
	struct npu_proc_ctx *proc_ctx)
{
	seq_printf(s, "  process pid: %d\n", proc_ctx->pid);
	seq_printf(s, "    sq_num: %u\n", proc_ctx->sq_num);
	seq_printf(s, "    cq_num: %u\n", proc_ctx->cq_num);
	seq_printf(s, "    sink_stream_num: %u\n", proc_ctx->sink_stream_num);
	seq_printf(s, "    stream_num: %u\n", proc_ctx->stream_num);
	seq_printf(s, "    event_num: %u\n", proc_ctx->event_num);
	seq_printf(s, "    hwts_event_num: %u\n", proc_ctx->hwts_event_num);
	seq_printf(s, "    model_num: %u\n", proc_ctx->model_num);
	seq_printf(s, "    task_num: %u\n", proc_ctx->task_num);
}

static int npu_resource_debugfs_show(struct seq_file *s, void *data)
{
	const u8 dev_id = 0;
	struct npu_dev_ctx *cur_dev_ctx = NULL;
	struct npu_proc_ctx *proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;
	struct npu_id_allocator *id_allocator = NULL;
	char *type_name = NULL;
	int count;
	u16 idx;
	unused(data);

	cur_dev_ctx = get_dev_ctx_by_id(dev_id);
	if (IS_ERR_OR_NULL(cur_dev_ctx))
		return 0;

	seq_printf(s, "devid: %d\n", cur_dev_ctx->devid);
	seq_printf(s, "plat_type: %d\n", cur_dev_ctx->plat_type);
	seq_printf(s, "sq_num: %d/%d\n", cur_dev_ctx->sq_num, NPU_MAX_SQ_NUM);
	seq_printf(s, "cq_num: %d/%d\n", cur_dev_ctx->cq_num, NPU_MAX_CQ_NUM);

	for (idx = NPU_ID_TYPE_STREAM; idx < NPU_ID_TYPE_NOTIFY; idx += 1) {
		if (idx == NPU_ID_TYPE_SINK_LONG_STREAM &&
			NPU_MAX_SINK_LONG_STREAM_ID == 0)
			continue;
		id_allocator = &(cur_dev_ctx->id_allocator[idx]);
		type_name = npu_id_type_to_string(idx);
		mutex_lock(&id_allocator->lock);
		count = list_count(&id_allocator->available_list);
		mutex_unlock(&id_allocator->lock);
		seq_printf(s, "%s_num: %u/%u\n", type_name,
			id_allocator->available_id_num, id_allocator->entity_num);
		seq_printf(s, "%s_available_list_counter: %u/%u\n",
			type_name, count, id_allocator->entity_num);
	}

	mutex_lock(&cur_dev_ctx->npu_power_mutex);
	seq_printf(s, "\nnpu power stage: %d\n", cur_dev_ctx->power_stage);
	seq_printf(s, "npu pm work_mode: %u\n", cur_dev_ctx->pm.work_mode);
	for (idx = 0; idx < NPU_WORKMODE_MAX; idx++)
		seq_printf(s, "npu pm wm_cnt[%u]: %u\n", idx, cur_dev_ctx->pm.wm_cnt[idx]);

	seq_printf(s, "\nproc_ctx_list_counter: %u\n",
		list_count(&cur_dev_ctx->proc_ctx_list));
	list_for_each_entry_safe(proc_ctx, next_proc_ctx,
		&cur_dev_ctx->proc_ctx_list, dev_ctx_list) {
		npu_resource_debugfs_print_proc(s, proc_ctx);
	}

	seq_printf(s, "\nrubbish_proc_ctx_list_counter: %u\n",
		list_count(&cur_dev_ctx->rubbish_proc_ctx_list));
	list_for_each_entry_safe(proc_ctx, next_proc_ctx,
		&cur_dev_ctx->rubbish_proc_ctx_list, dev_ctx_list) {
		npu_resource_debugfs_print_proc(s, proc_ctx);
	}
	mutex_unlock(&cur_dev_ctx->npu_power_mutex);

	return 0;
}

static int npu_resource_debugfs_open(struct inode *inode,
	struct file *file)
{
	return single_open(file, npu_resource_debugfs_show, inode->i_private);
}
