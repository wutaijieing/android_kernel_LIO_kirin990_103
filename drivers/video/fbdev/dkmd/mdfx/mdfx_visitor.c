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

#include <linux/slab.h>

#include "mdfx_visitor.h"
#include "mdfx_tracing.h"
#include "mdfx_priv.h"

static void mdfx_visitor_free_tracing_buf(struct mdfx_visitor_node *visitor)
{
	struct mdfx_tracing_buf *tracing_buf = NULL;
	uint32_t i;

	if (IS_ERR_OR_NULL(visitor))
		return;

	for (i = 0; i < TRACING_TYPE_MAX; i++) {
		tracing_buf = &visitor->tracing[i];
		mdfx_tracing_free_buf_list(tracing_buf);
	}
}

static void mdfx_init_visitor(struct mdfx_visitor_node *visitor, dump_info_func dumper_cb, bool need_logger)
{
	struct mdfx_tracing_buf *tracing = NULL;
	uint32_t i;

	if (IS_ERR_OR_NULL(visitor))
		return;

	mdfx_pr_info("enter mdfx_init_visitor");
	/* init each actor: dumper, tracing, logger */
	visitor->dumper_cb = dumper_cb;
	for (i = 0; i < TRACING_TYPE_MAX; i++) {
		tracing = &visitor->tracing[i];
		tracing->tracing_size = 0;
		INIT_LIST_HEAD(&tracing->tracing_list);
	}

	if (need_logger)
		mdfx_logger_init(&visitor->logger, visitor->id);
}

static void mdfx_deinit_visitor(struct mdfx_visitor_node *visitor)
{
	if (IS_ERR_OR_NULL(visitor))
		return;

	mdfx_visitor_free_tracing_buf(visitor);
	mdfx_logger_deinit(&visitor->logger);

	list_del(&visitor->node);
	kfree(visitor);
	visitor = NULL;
}

static int64_t mdfx_visitor_new_one(struct mdfx_visitors_t *visitors,
	struct mdfx_visitor_desc *desc, dump_info_func dumper_cb, bool need_logger)
{
	struct mdfx_visitor_node *node = NULL;

	if (IS_ERR_OR_NULL(visitors) || IS_ERR_OR_NULL(desc))
		return INVALID_VISITOR_ID;

	node = (struct mdfx_visitor_node *)kzalloc(sizeof(*node), GFP_KERNEL);
	if (IS_ERR_OR_NULL(node)) {
		mdfx_pr_err("kzalloc fail");
		return INVALID_VISITOR_ID;
	}

	mdfx_pr_info("enter mdfx_visitor_new_one, need_logger:%d", need_logger == true ? 1 : 0);
	node->id = CREATE_VISITOR_ID(visitors->id);
	node->type = desc->type;
	node->pid = desc->pid;

	mdfx_init_visitor(node, dumper_cb, need_logger);
	list_add_tail(&node->node, &visitors->visitor_list);

	mdfx_pr_info("new visitor, desc->id=%lld, desc->type=%llu, node->id=%lld, node->pid=%lld", \
		desc->id, desc->type, node->id, node->pid);
	return node->id;
}

static struct mdfx_visitor_node* mdfx_get_visitor(struct mdfx_visitors_t *visitors, int64_t id)
{
	struct mdfx_visitor_node* _node_ = NULL;
	struct mdfx_visitor_node* visitor_node = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return NULL;

	list_for_each_entry_safe(visitor_node, _node_, &(visitors->visitor_list), node) {
		if (visitor_node->id == id)
			return visitor_node;
	}

	return NULL;
}

static struct mdfx_visitor_node* mdfx_get_visitor_with_type(struct mdfx_visitors_t *visitors, uint64_t type)
{
	struct mdfx_visitor_node* _node_ = NULL;
	struct mdfx_visitor_node* visitor_node = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return NULL;

	list_for_each_entry_safe(visitor_node, _node_, &(visitors->visitor_list), node) {
		if (visitor_node->type == type)
			return visitor_node;
	}

	return NULL;
}

int64_t mdfx_get_visitor_id(struct mdfx_visitors_t *visitors, uint64_t type)
{
	struct mdfx_visitor_node* visitor = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return INVALID_VISITOR_ID;

	visitor = mdfx_get_visitor_with_type(visitors, type);
	if (IS_ERR_OR_NULL(visitor)) {
		mdfx_pr_err("mdfx_get_visitor_with_type fail, visitor type:%llu", type);
		return INVALID_VISITOR_ID;
	}

	return visitor->id;
}

uint64_t mdfx_get_visitor_type(struct mdfx_visitors_t *visitors, int64_t id)
{
	struct mdfx_visitor_node* _node_ = NULL;
	struct mdfx_visitor_node* visitor_node = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return 0;

	list_for_each_entry_safe(visitor_node, _node_, &(visitors->visitor_list), node) {
		if (visitor_node->id == id)
			return visitor_node->type;
	}

	return 0;
}

int64_t mdfx_get_visitor_pid(struct mdfx_visitors_t *visitors, int64_t id)
{
	struct mdfx_visitor_node* _node_ = NULL;
	struct mdfx_visitor_node* visitor_node = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return 0;

	list_for_each_entry_safe(visitor_node, _node_, &(visitors->visitor_list), node) {
		if (visitor_node->id == id)
			return visitor_node->pid;
	}

	return 0;
}

static int64_t mdfx_get_and_new_visitor_locked(struct mdfx_visitors_t *visitors,
	struct mdfx_visitor_desc *desc, dump_info_func dumper_cb, bool need_logger)
{
	int64_t visitor_id;
	struct mdfx_visitor_node *visitor = NULL;

	if (IS_ERR_OR_NULL(visitors) || IS_ERR_OR_NULL(desc))
		return INVALID_VISITOR_ID;

	mdfx_pr_info("enter mdfx_get_and_new_visitor_locked");

	if (down_trylock(&visitors->visitors_sem) != 0)
		return INVALID_VISITOR_ID;

	/* if find the visitor, return; else create a new visitor */
	visitor_id = mdfx_get_visitor_id(visitors, desc->type);
	if (visitor_id == INVALID_VISITOR_ID) {
		visitor_id = mdfx_visitor_new_one(visitors, desc, dumper_cb, need_logger);
	}
	/* same type means same visitor, if the request of add visitor with same type from userspace arrived later than
	    the request of create visitor from kernel part, update the member 'pid' */
	else if (desc->pid > 0) {
		visitor = mdfx_get_visitor(visitors, visitor_id);
		if (visitor != NULL)
			visitor->pid = desc->pid;
	}

	up(&visitors->visitors_sem);
	mdfx_pr_info("visitor_id:%lld", visitor_id);
	return visitor_id;
}

struct mdfx_tracing_buf* mdfx_visitor_get_tracing_buf(
		struct mdfx_visitors_t *visitors, int64_t id, uint32_t tracing_type)
{
	struct mdfx_visitor_node* visitor = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return NULL;

	if (tracing_type >= TRACING_TYPE_MAX) {
		mdfx_pr_err("tracing_type is wrong: %u", tracing_type);
		return NULL;
	}

	visitor = mdfx_get_visitor(visitors, id);
	if (IS_ERR_OR_NULL(visitor)) {
		mdfx_pr_err("mdfx_get_visitor fail");
		return NULL;
	}

	return &visitor->tracing[tracing_type];
}

uint64_t mdfx_visitor_get_tracing_size(struct mdfx_visitors_t *visitors, int64_t id)
{
	struct mdfx_visitor_node* visitor = NULL;
	uint32_t i;
	uint64_t len = 0;

	if (IS_ERR_OR_NULL(visitors))
		return 0;

	visitor = mdfx_get_visitor(visitors, id);
	if (IS_ERR_OR_NULL(visitor)) {
		mdfx_pr_err("mdfx_get_visitor fail");
		return 0;
	}

	for (i = TRACING_VOTE; i < TRACING_TYPE_MAX; i++)
		len += (uint64_t)(visitor->tracing[i].tracing_size);

	return len;
}

inline uint32_t mdfx_visitor_get_max_tracing_buf_size()
{
	return DEFAULT_MAX_TRACING_BUF_SIZE;
}

dump_info_func mdfx_visitor_get_dump_cb(struct mdfx_visitors_t *visitors, int64_t id)
{
	struct mdfx_visitor_node* visitor = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return NULL;

	visitor = mdfx_get_visitor(visitors, id);
	if (IS_ERR_OR_NULL(visitor)) {
		mdfx_pr_err("mdfx_get_visitor fail");
		return NULL;
	}

	return visitor->dumper_cb;
}

struct mdfx_logger_t* mfx_get_visitor_logger(int64_t id)
{
	struct mdfx_visitor_node* visitor = NULL;

	if (IS_ERR_OR_NULL(g_mdfx_data)) {
		mdfx_pr_err("g_mdfx_data fail");
		return NULL;
	}

	visitor = mdfx_get_visitor(&g_mdfx_data->visitors, id);
	if (IS_ERR_OR_NULL(visitor)) {
		mdfx_pr_err("mdfx_get_visitor fail");
		return NULL;
	}

	return &visitor->logger;
}

int mdfx_add_visitor(struct mdfx_pri_data *mdfx_data, void __user *argp)
{
	struct mdfx_visitor_desc desc;
	struct mdfx_visitors_t *visitors = NULL;
	int32_t visitor_id;

	if (!mdfx_data)
		return -1;

	if (!argp)
		return -1;

	if (copy_from_user(&desc, argp, sizeof(desc))) {
		mdfx_pr_err("copy from user fail");
		return -1;
	}

	mdfx_pr_info("enter mdfx_add_visitor");
	visitors = &mdfx_data->visitors;
	visitor_id = mdfx_get_and_new_visitor_locked(visitors, &desc, NULL, false);
	if (visitor_id == INVALID_VISITOR_ID) {
		mdfx_pr_err("add visitor fail, id=-1, type=%llu, pid=%lld", desc.type, desc.pid);
	}

	desc.id = visitor_id;
	if (copy_to_user(argp, &desc, sizeof(desc)))
		return -1;

	return 0;
}

int mdfx_remove_visitor(struct mdfx_pri_data *mdfx_data, const void __user *argp)
{
	int32_t visitor_id;
	struct mdfx_visitors_t *visitors = NULL;
	struct mdfx_visitor_node *visitor = NULL;

	if (!mdfx_data)
		return -1;

	if (!argp)
		return -1;

	if (copy_from_user(&visitor_id, argp, sizeof(visitor_id))) {
		mdfx_pr_err("copy from user fail");
		return -1;
	}

	visitors = &mdfx_data->visitors;
	down(&visitors->visitors_sem);
	visitor = mdfx_get_visitor(visitors, visitor_id);
	if (IS_ERR_OR_NULL(visitor)) {
		mdfx_pr_err("mdfx_get_visitor fail");
		up(&visitors->visitors_sem);
		return -1;
	}
	mdfx_deinit_visitor(visitor);
	up(&visitors->visitors_sem);

	return 0;
}

void mdfx_visitors_init(struct mdfx_visitors_t *visitors)
{
	if (IS_ERR_OR_NULL(visitors))
		return;

	sema_init(&visitors->visitors_sem, 1);
	INIT_LIST_HEAD(&visitors->visitor_list);
	atomic_set(&visitors->id, 0);
}

void mdfx_visitors_deinit(struct mdfx_visitors_t *visitors)
{
	struct mdfx_visitor_node* _node_ = NULL;
	struct mdfx_visitor_node* visitor_node = NULL;

	if (IS_ERR_OR_NULL(visitors))
		return;

	down(&visitors->visitors_sem);
	list_for_each_entry_safe(visitor_node, _node_, &(visitors->visitor_list), node) {
		mdfx_deinit_visitor(visitor_node);
	}
	up(&visitors->visitors_sem);
}

/* kernel caller create visitor,
 * if success, return visitor id,
 * else return -1
 * kernel caller not need pid, so use -1
 */
int64_t mdfx_create_visitor(uint64_t type, dump_info_func dumper_cb)
{
	struct mdfx_visitors_t *visitors = NULL;
	struct mdfx_visitor_desc desc;

	if (IS_ERR_OR_NULL(g_mdfx_data) || !mdfx_ready) {
		mdfx_pr_err("g_mdfx_data fail");
		return INVALID_VISITOR_ID;
	}

	mdfx_pr_info("enter mdfx_create_visitor");
	visitors = &g_mdfx_data->visitors;
	desc.id = -1;
	desc.type = type;
	desc.pid = -1;
	return mdfx_get_and_new_visitor_locked(visitors, &desc, dumper_cb, true);
}
EXPORT_SYMBOL(mdfx_create_visitor); //lint !e580

/*
 * remove kernel caller visitor, id must be bigger than -1
 */
void mdfx_destroy_visitor(int64_t id)
{
	struct mdfx_visitors_t *visitors = NULL;
	struct mdfx_visitor_node *visitor = NULL;

	if (IS_ERR_OR_NULL(g_mdfx_data) || !mdfx_ready) {
		mdfx_pr_err("g_mdfx_data fail");
		return;
	}

	visitors = &g_mdfx_data->visitors;
	down(&visitors->visitors_sem);
	visitor = mdfx_get_visitor(visitors, id);
	mdfx_deinit_visitor(visitor);
	up(&visitors->visitors_sem);
}
EXPORT_SYMBOL(mdfx_destroy_visitor); //lint !e580


