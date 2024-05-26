/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/list.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "mdfx_priv.h"
#include "mdfx_utils.h"
#include "mdfx_tracing.h"
#include "mdfx_file.h"

#define MAX_TRACING_MSG_LEN 100

static uint32_t mdfx_tracing_delete_head(struct mdfx_tracing_buf *buf, uint32_t deleted_size)
{
	struct mdfx_tracing_item *tracing_node = NULL;
	struct mdfx_tracing_item *_node_ = NULL;
	uint32_t be_deleted_size = 0;
	uint32_t need_deleted_size = deleted_size;

	if (IS_ERR_OR_NULL(buf))
		return 0;

	mdfx_pr_info("enter mdfx_tracing_delete_head");
	if (buf->tracing_size < need_deleted_size)
		need_deleted_size = buf->tracing_size;

	list_for_each_entry_safe(tracing_node, _node_, &(buf->tracing_list), node) {
		if (be_deleted_size >= need_deleted_size)
			break;

		if (buf->tracing_size >= tracing_node->msg_len)
			buf->tracing_size -= tracing_node->msg_len;

		be_deleted_size += tracing_node->msg_len;
		if (tracing_node->msg)
			kfree(tracing_node->msg);

		list_del(&tracing_node->node);
		kfree(tracing_node);
		tracing_node = NULL;
	}

	if (list_empty(&buf->tracing_list))
		buf->tracing_size = 0;

	mdfx_pr_info("tracing buf len = %u", buf->tracing_size);
	return buf->tracing_size;
}

static int mdfx_tracing_append_to_visitor(struct mdfx_tracing_buf *buf, char *msg, uint32_t msg_len)
{
	uint32_t current_size;
	uint32_t max_size = mdfx_visitor_get_max_tracing_buf_size();
	struct mdfx_tracing_item *item = NULL;

	if (IS_ERR_OR_NULL(msg) || IS_ERR_OR_NULL(buf))
		return -1;

	mdfx_pr_info("enter mdfx_tracing_append_to_visitor");
	if (msg_len == 0 || msg_len >= max_size)
		return -1;

	current_size = buf->tracing_size;

	/*
	 * if buffer will overfow the max size,
	 * we need delete 1K head msg from the buffer list.
	 */
	while (current_size + msg_len > max_size)
		current_size = mdfx_tracing_delete_head(buf, DEFAULT_DELETE_HEAD_TRACING_SIZE);

	item = kzalloc(sizeof(struct mdfx_tracing_item), GFP_KERNEL);
	if (IS_ERR_OR_NULL(item))
		return -1;

	item->msg = msg;
	item->msg_len = msg_len;
	list_add_tail(&item->node, &buf->tracing_list);

	buf->tracing_size += item->msg_len;
	mdfx_pr_info("buf->tracing_size: %u", buf->tracing_size);
	return 0;
}

static int mdfx_tracing_record_info(struct mdfx_visitors_t *visitors,
		struct mdfx_tracing_desc *tracing_desc)
{
	struct mdfx_tracing_buf *tracing_buf = NULL;
	enum tracing_type t_type;

	if (IS_ERR_OR_NULL(visitors) || IS_ERR_OR_NULL(tracing_desc))
		return -1;

	mdfx_pr_info("enter mdfx_tracing_record_info");

	/* convert from TracingType to tracing_type  (1<<0, 1<<1, 1<<2) => (0, 1, 2) */
	switch (tracing_desc->tracing_type) {
	case TRACING_TYPE_VOTE:
		t_type = TRACING_VOTE;
		break;
	case TRACING_TYPE_HOTPLUG:
		t_type = TRACING_HOTPLUG;
		break;
	case TRACING_TYPE_DISPLAY_FLOW:
		t_type = TRACING_DISPLAY_FLOW;
		break;
	default:
		mdfx_pr_err("wrong tracing_desc->tracing_type: %llu", tracing_desc->tracing_type);
		return -1;
	}

	tracing_buf = mdfx_visitor_get_tracing_buf(visitors, tracing_desc->id, t_type);
	if (IS_ERR_OR_NULL(tracing_buf)) {
		mdfx_pr_err("mdfx_visitor_get_tracing_buf fail");
		return -1;
	}

	return mdfx_tracing_append_to_visitor(tracing_buf, tracing_desc->msg, tracing_desc->msg_len);
}

static void mdfx_tracing_write_buf_list(struct file *pfile, struct mdfx_tracing_buf *tracing_buf)
{
	struct mdfx_tracing_item *tracing_node = NULL;
	struct mdfx_tracing_item *_node_ = NULL;
	int ret;

	if (IS_ERR_OR_NULL(pfile) || IS_ERR_OR_NULL(tracing_buf))
		return;

	mdfx_pr_info("enter mdfx_tracing_write_buf_list");
	/* write node msg to file, and then free msg and node, clear the list */
	list_for_each_entry_safe(tracing_node, _node_, &(tracing_buf->tracing_list), node) {
		ret = mdfx_file_write(pfile, tracing_node->msg, tracing_node->msg_len);
		if (ret)
			mdfx_pr_err("write tracing node fail,msg=%s", tracing_node->msg);

		kfree(tracing_node->msg);
		list_del(&tracing_node->node);
		kfree(tracing_node);
		tracing_node = NULL;
	}

	tracing_buf->tracing_size = 0;
}

static struct file* mdfx_tracing_open_file(int64_t visitor_id, uint64_t tracing_types, const char *event_name)
{
	char filename[MAX_FILE_NAME_LEN] = {0};
	char include_str[MAX_FILE_NAME_LEN / 2] = {0};
	struct file *pfile = NULL;
	int ret;
	int i;
	uint32_t name_len;

	if (IS_ERR_OR_NULL(event_name))
		return NULL;

	mdfx_pr_info("enter mdfx_tracing_open_file");
	ret = snprintf(include_str, MAX_FILE_NAME_LEN / 2 - 1, "event[%s]_id[%lld]", event_name, visitor_id);
	if (ret < 0)
		return NULL;
	for (i = TRACING_VOTE; i < TRACING_TYPE_MAX; i++) {
		if (ENABLE_BIT64(tracing_types, i)) {
			name_len = strlen(include_str);
			if (include_str[name_len - 1] == ']')
				ret = snprintf(include_str + name_len, MAX_FILE_NAME_LEN / 2 - 1, "_type[%lu", BIT64(i));
			else
				ret = snprintf(include_str + name_len, MAX_FILE_NAME_LEN / 2 - 1, "-%lu", BIT64(i));
			if (ret < 0)
				return NULL;
		}
	}
	name_len = strlen(include_str);
	include_str[name_len] = ']';
	include_str[++name_len] = '\0';

	include_str[MAX_FILE_NAME_LEN / 2 - 1] = '\0';

	mdfx_file_get_name(filename, MODULE_NAME_TRACING, include_str, MAX_FILE_NAME_LEN);
	filename[MAX_FILE_NAME_LEN - 1] = '\0';

	mdfx_pr_info("filename: %s", filename);
	ret = mdfx_create_dir(filename);
	if (ret) {
		mdfx_pr_err("create dir failed %d\n", ret);
		return NULL;
	}

	pfile = mdfx_file_open(filename);
	if (IS_ERR_OR_NULL(pfile)) {
		mdfx_pr_info("open fils fail %s", filename);
		return NULL;
	}

	return pfile;
}

static int mdfx_tracing_save_for_visitor(struct mdfx_pri_data *priv_data,
	struct mdfx_event_desc *desc, int64_t visitor_id, uint64_t tracing_types)
{
	struct mdfx_tracing_buf *tracing_buf = NULL;
	struct file *pfile = NULL;
	mm_segment_t old_fs;
	uint32_t i;

	if (IS_ERR_OR_NULL(priv_data) || IS_ERR_OR_NULL(desc))
		return -1;

	mdfx_pr_info("enter mdfx_tracing_save_for_visitor");
	/* tracing information is none, don't save to file */
	if (mdfx_visitor_get_tracing_size(&priv_data->visitors, visitor_id) == 0) {
		mdfx_pr_info("mdfx_visitor_get_tracing_size is 0");
		return 0;
	}

	pfile = mdfx_tracing_open_file(visitor_id, tracing_types, desc->event_name);
	if (IS_ERR_OR_NULL(pfile)) {
		mdfx_pr_err("open tracing file error");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	/* tracing_type maybe is 1 << 0, 1 << 1, 1 << 2 or their OR operation result */
	for (i = TRACING_VOTE; i < TRACING_TYPE_MAX; i++) {
		if (!ENABLE_BIT64(tracing_types, i))
			continue;

		tracing_buf = mdfx_visitor_get_tracing_buf(&priv_data->visitors, visitor_id, i);
		if (IS_ERR_OR_NULL(tracing_buf)) {
			mdfx_pr_info("get tracing buf fail, id=%lld, type=0x%x", visitor_id, i);
			continue;
		}

		mdfx_tracing_write_buf_list(pfile, tracing_buf);
	}

	set_fs(old_fs);
	filp_close(pfile, NULL);

	return 0;
}

/*
 * tracing maybe need save all visitors's tracing information of
 * the relevance types which assigned be desc.
 * dynamic allocated memory space that param data pointed, take care of releasing it.
 */
static void mdfx_tracing_save_info(void *data)
{
	int64_t visitor_id;
	uint32_t type_bit;
	uint64_t visitor_type;
	struct mdfx_event_desc *desc = NULL;
	uint64_t detail;

	if (IS_ERR_OR_NULL(g_mdfx_data)) {
		mdfx_pr_err("g_mdfx_data is null");
		return;
	}

	if (IS_ERR_OR_NULL(data)) {
		mdfx_pr_err("data is null");
		return;
	}
	desc = (struct mdfx_event_desc *)data;

	detail = mdfx_event_get_action_detail(desc, ACTOR_TRACING);
	if (detail == 0) {
		mdfx_pr_err("detail == 0");
		kfree(desc->detail);
		kfree(desc);
		return;
	}
	mdfx_pr_info("enter mdfx_tracing_save_info, detail=%llu", detail);

	visitor_type = desc->relevance_visitor_types;
	mdfx_pr_info("visitor_type: %llu", visitor_type);
	for (type_bit = 0;  visitor_type != 0; type_bit++) {
		if (!ENABLE_BIT64(visitor_type, type_bit))
			continue;

		mdfx_pr_info("call mdfx_get_visitor_id, type:%llu", BIT64(type_bit));
		visitor_id = mdfx_get_visitor_id(&g_mdfx_data->visitors, BIT64(type_bit));
		if (visitor_id != INVALID_VISITOR_ID)
			mdfx_tracing_save_for_visitor(g_mdfx_data, desc, visitor_id, detail);

		DISABLE_BIT64(visitor_type, type_bit);
	}
	kfree(desc->detail);
	kfree(desc);
}

static int mdfx_tracing_event_act(struct mdfx_pri_data *mdfx_data, struct mdfx_event_desc *desc)
{
	struct mdfx_event_desc *desc_buf = NULL;

	if (IS_ERR_OR_NULL(mdfx_data)) {
		mdfx_pr_err("mdfx_data is null");
		return -1;
	}

	if (IS_ERR_OR_NULL(desc)) {
		mdfx_pr_err("desc is null");
		return -1;
	}

	if (!MDFX_HAS_CAPABILITY(mdfx_data->mdfx_caps, MDFX_CAP_TRACING))
		return 0;

	mdfx_pr_info("enter mdfx_tracing_event_act");
	desc_buf = mdfx_copy_event_desc(desc);
	if (IS_ERR_OR_NULL(desc_buf))
		return -1;

	mdfx_saver_triggle_thread(&mdfx_data->trace_saving_thread, desc_buf, mdfx_tracing_save_info);
	mdfx_pr_info("triggle trace_saving_thread");
	return 0;
}

static bool mdfx_tracing_valid_parameter(struct mdfx_tracing_desc *desc)
{
	if (IS_ERR_OR_NULL(desc))
		return false;

	if (desc->id <= INVALID_VISITOR_ID) {
		mdfx_pr_err("invalid id =%" PRIu64, desc->id);
		return false;
	}

	if (desc->tracing_type == 0) {
		mdfx_pr_err("invalid id =0x%" PRIx64, desc->tracing_type);
		return false;
	}

	if (desc->msg_len <= 0 || desc->msg_len >= MAX_TRACING_MSG_LEN) {
		mdfx_pr_err("invalid msg len = %u", desc->msg_len);
		return false;
	}

	if (!desc->msg) {
		mdfx_pr_err("invalid msg NULL");
		return false;
	}

	return true;
}

static int mdfx_tracing_build_msg(struct mdfx_tracing_desc *desc, const char *msg)
{
	uint32_t msg_len;
	char temp_msg[MAX_TRACING_MSG_LEN] = {0};
	int ret;

	if (IS_ERR_OR_NULL(msg) || IS_ERR_OR_NULL(desc))
		return -1;

	if (desc->msg_len >= MAX_TRACING_MSG_LEN)
		return -1;

	mdfx_pr_info("enter mdfx_tracing_build_msg");
	msg_len = mdfx_get_msg_header(temp_msg, MAX_TRACING_MSG_LEN);
	if (msg_len >= MAX_TRACING_MSG_LEN) {
		temp_msg[MAX_TRACING_MSG_LEN - 1] = '\0';
		mdfx_pr_err("get head fail %s", temp_msg);
		return -1;
	}

	if (msg_len + desc->msg_len > MAX_TRACING_MSG_LEN)
		return -1;

	temp_msg[msg_len] = '\0';

	msg_len = msg_len + desc->msg_len + 2;
	desc->msg = kmalloc(msg_len, GFP_KERNEL);
	if (IS_ERR_OR_NULL(desc->msg))
		return -1;
	memset(desc->msg, 0, msg_len);

	ret = snprintf(desc->msg, msg_len, "%s %s\n", temp_msg, msg);
	if (ret < 0 || msg_len == 0) {
		kfree(desc->msg);
		desc->msg = NULL;
		return -1;
	}

	desc->msg[msg_len - 1] = '\0';

	mdfx_pr_info("msg=%s, msg_len=%u", desc->msg, msg_len);
	return 0;
}

/* allocate buffer for mediadfx server */
static int mdfx_tracing_trace_info(struct mdfx_pri_data *mdfx_data, const void __user *argp)
{
	int ret;
	struct mdfx_tracing_desc tracing_desc;
	char *msg = NULL;

	if (IS_ERR_OR_NULL(mdfx_data)) {
		mdfx_pr_err("memory already allocated for mediadfx server\n");
		return -ENOMEM;
	}

	if (!argp) {
		mdfx_pr_err("argp NULL Pointer\n");
		return -EINVAL;
	}

	ret = copy_from_user(&tracing_desc, argp, sizeof(tracing_desc));
	if (ret) {
		mdfx_pr_err("copy_from_user failed!ret=%d!\n", ret);
		return ret;
	}

	if (!mdfx_tracing_valid_parameter(&tracing_desc)) {
		mdfx_pr_err("invalid parameter, id=%lld, msg len=%u!\n",
				tracing_desc.id, tracing_desc.msg_len);
		return -1;
	}

	mdfx_pr_info("enter mdfx_tracing_trace_info");
	/* this message will be push to tracing list,
	 * it will be free at saving to file,
	 * or tracing buffer is overflow
	 */
	msg = kzalloc(tracing_desc.msg_len + 1, GFP_KERNEL);
	if (IS_ERR_OR_NULL(msg))
		return -1;

	ret = copy_from_user(msg, tracing_desc.msg, tracing_desc.msg_len);
	if (ret) {
		mdfx_pr_err("copy from user msg fail");
		kfree(msg);
		msg = NULL;
		return -1;
	}
	tracing_desc.msg = msg;
	mdfx_pr_info("tracing_desc.msg: %s", tracing_desc.msg);

	ret = mdfx_tracing_record_info(&mdfx_data->visitors, &tracing_desc);
	if (ret) {
		mdfx_pr_err("save tracing failed! ret=%d!\n", ret);
		kfree(msg);
		msg = NULL;
		tracing_desc.msg = NULL;
		return -1;
	}

	return 0;
}

struct mdfx_actor_ops_t tracing_ops = {
	.act = mdfx_tracing_event_act,
	.do_ioctl = mdfx_tracing_trace_info,
};

void mdfx_tracing_init_actor(struct mdfx_actor_t *actor)
{
	if (IS_ERR_OR_NULL(actor))
		return;

	actor->actor_type = ACTOR_TRACING;
	actor->ops = &tracing_ops;
}

void mdfx_tracing_free_buf_list(struct mdfx_tracing_buf *tracing_buf)
{
	struct mdfx_tracing_item *tracing_node = NULL;
	struct mdfx_tracing_item *_node_ = NULL;

	if (IS_ERR_OR_NULL(tracing_buf))
		return;

	list_for_each_entry_safe(tracing_node, _node_, &(tracing_buf->tracing_list), node) {
		kfree(tracing_node->msg);
		list_del(&tracing_node->node);
		kfree(tracing_node);
		tracing_node = NULL;
	}

	tracing_buf->tracing_size = 0;
}

void mdfx_tracing_point(int64_t visitor_id, uint64_t tracing_type, char *msg)
{
	struct mdfx_tracing_desc desc = {0};

	if (IS_ERR_OR_NULL(g_mdfx_data) || IS_ERR_OR_NULL(msg))
		return;

	if (!MDFX_HAS_CAPABILITY(g_mdfx_data->mdfx_caps, MDFX_CAP_TRACING))
		return;

	desc.id = visitor_id;
	desc.tracing_type = tracing_type;
	desc.msg_len = strlen(msg);
	desc.msg = msg;

	if (!mdfx_tracing_valid_parameter(&desc)) {
		mdfx_pr_err("invalid parameter, id=%lld, msg len=%u!\n",
			desc.id, desc.msg_len);
		return;
	}

	if (mdfx_tracing_build_msg(&desc, msg)) {
		mdfx_pr_err("build msg fail");
		return;
	}

	if (mdfx_tracing_record_info(&g_mdfx_data->visitors, &desc)) {
		mdfx_pr_err("tracing fail, id=%" PRIu64 "msg=%s", desc.id, desc.msg);

		if (desc.msg) {
			kfree(desc.msg);
			desc.msg = NULL;
		}
	}
}

