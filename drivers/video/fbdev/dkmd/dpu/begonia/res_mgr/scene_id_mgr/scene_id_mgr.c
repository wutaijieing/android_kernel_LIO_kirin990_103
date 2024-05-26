/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#include "scene_id_mgr.h"
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "res_mgr.h"
#include "dkmd_log.h"

#define DISP_LOG_TAG "RES_MGR"

static struct scene_id_node* find_scene_id_node(struct list_head *scene_id_list, int32_t scene_id)
{
	struct scene_id_node *node = NULL;
	struct scene_id_node *_node_ = NULL;

	list_for_each_entry_safe(node, _node_, scene_id_list, list_node) {
		if (!node)
			continue;

		if (node->id_info.scene_id == scene_id)
			return node;
	}

	return NULL;
}

static void free_scene_id_list(struct list_head *scene_id_list)
{
	struct scene_id_node *scene_id_node = NULL;
	struct scene_id_node *_node_ = NULL;

	list_for_each_entry_safe(scene_id_node, _node_, scene_id_list, list_node) {
		if (!scene_id_node)
			continue;

		list_del(&scene_id_node->list_node);
		kfree(scene_id_node);
		scene_id_node = NULL;
	}
}

static void reset_scene_id_list(struct list_head *scene_id_list)
{
	struct scene_id_node *scene_id_node = NULL;
	struct scene_id_node *_node_ = NULL;

	list_for_each_entry_safe(scene_id_node, _node_, scene_id_list, list_node) {
		if (!scene_id_node)
			continue;

		scene_id_node->id_info.user.user_type = SCENE_USER_NONE;
		atomic_set(&scene_id_node->ref_cnt, 0);
	}
}

static struct scene_id_node* init_scene_id_node(uint32_t scene_id, int32_t scene_type)
{
	struct scene_id_node *node = NULL;

	dpu_assert(scene_type < 0);

	node = kzalloc(sizeof(struct scene_id_node), GFP_KERNEL);
	if (!node) {
		dpu_pr_err("kzalloc scene id node fail");
		return NULL;
	}

	atomic_set(&node->ref_cnt, 0);
	node->id_info.user.user_type = SCENE_USER_NONE;
	node->id_info.user.scene_type = scene_type;
	node->id_info.scene_id = (int32_t)scene_id;
	return node;
}

static int scene_id_mgr_request_scene_id(struct dpu_scene_id_mgr *scene_id_mgr, void __user *argp)
{
	struct scene_id_info id_info;
	struct scene_id_node *scene_id_node = NULL;
	struct scene_id_user *user = NULL;

	if (copy_from_user(&id_info, argp, sizeof(id_info)) != 0) {
		dpu_pr_err("copy from user fail");
		return -1;
	}

	down(&scene_id_mgr->sem);
	scene_id_node = find_scene_id_node(&scene_id_mgr->scene_id_list, id_info.scene_id);
	if (!scene_id_node) {
		dpu_pr_err("user %d find scene id %d node fail", id_info.user.user_type, id_info.scene_id);
		up(&scene_id_mgr->sem);
		return -1;
	}

	if (scene_id_node->id_info.user.scene_type != id_info.user.scene_type) {
		dpu_pr_err("user's scene type (%d) is mismatch (%d) ", scene_id_node->id_info.user.scene_type,
				   id_info.user.scene_type);
		up(&scene_id_mgr->sem);
		return -1;
	}

	user = &scene_id_node->id_info.user;
	if ((user->user_type != SCENE_USER_NONE) && (user->user_type != id_info.user.user_type)) {
		dpu_pr_debug("scene_id = %d (user=%d, scene_type=%d) is mismatch with reqeust (user=%d, scene_type=%d)",
				     scene_id_node->id_info.scene_id, user->user_type, user->scene_type,
					 id_info.user.user_type, id_info.user.scene_type);
		id_info.result = -1;
	} else {
		user->user_type = id_info.user.user_type;
		id_info.result = 0;
		atomic_inc(&scene_id_node->ref_cnt);
	}

	if (copy_to_user(argp, &id_info, sizeof(id_info)) != 0) {
		dpu_pr_err("copy to user fail, user=%d, id=%d", id_info.user.user_type, id_info.scene_id);
		up(&scene_id_mgr->sem);
		return -1;
	}

	up(&scene_id_mgr->sem);
	return 0;
}

static int scene_id_mgr_release_scene_id(struct dpu_scene_id_mgr *scene_id_mgr, void __user *argp)
{
	struct scene_id_info id_info;
	struct scene_id_node *scene_id_node = NULL;
	struct scene_id_user *user = NULL;

	if (copy_from_user(&id_info, argp, sizeof(id_info)) != 0) {
		dpu_pr_err("copy from user fail");
		return -1;
	}

	down(&scene_id_mgr->sem);
	scene_id_node = find_scene_id_node(&scene_id_mgr->scene_id_list, id_info.scene_id);
	if (!scene_id_node) {
		dpu_pr_err("user %d find scene id %d node fail", id_info.user.user_type, id_info.scene_id);
		up(&scene_id_mgr->sem);
		return -1;
	}

	user = &scene_id_node->id_info.user;
	if (atomic_read(&scene_id_node->ref_cnt) == 0) {
		dpu_pr_err("user %d ref_cnt is 0, release fail", user->user_type);
		up(&scene_id_mgr->sem);
		return -1;
	}

	if (user->user_type == id_info.user.user_type) {
		if (atomic_dec_and_test(&scene_id_node->ref_cnt)) {
			dpu_pr_debug("user %d release scene_id %d succ", user->user_type, id_info.scene_id);
			user->user_type = SCENE_USER_NONE;
		}
	}
	up(&scene_id_mgr->sem);
	return 0;
}

static long scene_id_mgr_ioctl(struct dpu_res_resouce_node *res_node, uint32_t cmd, void __user *argp)
{
	struct dpu_scene_id_mgr *scene_id_mgr = NULL;

	if (!res_node || !res_node->data || !argp) {
		dpu_pr_err("res_node or node data or argp is NULL");
		return -EINVAL;
	}

	scene_id_mgr = (struct dpu_scene_id_mgr*)res_node->data;
	switch (cmd) {
	case RES_REQUEST_SCENE_ID:
		return scene_id_mgr_request_scene_id(scene_id_mgr, argp);
	case RES_RELEASE_SCENE_ID:
		return scene_id_mgr_release_scene_id(scene_id_mgr, argp);
	default:
		dpu_pr_info("scene_id mgr unsupport cmd, need processed by other module, cmd = 0x%x", cmd);
		return 1;
	}

	return 0;
}

static void* scene_id_mgr_init(struct dpu_res_data *rm_data)
{
	uint32_t i;
	struct scene_id_node *scene_id_node = NULL;
	struct dpu_scene_id_mgr *scene_id_mgr = kzalloc(sizeof(struct dpu_scene_id_mgr), GFP_KERNEL);
	if (!scene_id_mgr)
		return NULL;

	sema_init(&scene_id_mgr->sem, 1);
	INIT_LIST_HEAD(&scene_id_mgr->scene_id_list);

	dpu_assert(rm_data->offline_scene_id_count > DTS_OFFLINE_SCENE_ID_MAX);

	for (i = 0; i < rm_data->offline_scene_id_count; i++) {
		scene_id_node = init_scene_id_node(rm_data->offline_scene_ids[i], SCENE_TYPE_OFFLINE);
		if (unlikely(!scene_id_node)) {
			dpu_pr_err("init scene id node fail, i = %u, scene_id = %u", i, rm_data->offline_scene_ids[i]);
			continue;
		}

		list_add_tail(&scene_id_node->list_node, &scene_id_mgr->scene_id_list);
	}

	return scene_id_mgr;
}

static void scene_id_mgr_deinit(void *data)
{
	struct dpu_scene_id_mgr *scene_id_mgr = (struct dpu_scene_id_mgr *)data;

	if (!scene_id_mgr)
		return;

	down(&scene_id_mgr->sem);
	free_scene_id_list(&(scene_id_mgr->scene_id_list));
	up(&scene_id_mgr->sem);

	kfree(scene_id_mgr);
}

static void scene_id_mgr_reset(void *data)
{
	struct dpu_scene_id_mgr *scene_id_mgr = (struct dpu_scene_id_mgr *)data;

	dpu_pr_info("reset scene_id_mgr=%pK", scene_id_mgr);

	if (!scene_id_mgr)
		return;

	down(&scene_id_mgr->sem);
	reset_scene_id_list(&(scene_id_mgr->scene_id_list));
	up(&scene_id_mgr->sem);
}

void dpu_res_register_scene_id_mgr(struct list_head *resource_head)
{
	struct dpu_res_resouce_node *scene_mgr_node = kzalloc(sizeof(struct dpu_res_resouce_node), GFP_KERNEL);
	if (!scene_mgr_node)
		return;

	scene_mgr_node->res_type = RES_SCENE_ID;
	atomic_set(&scene_mgr_node->res_ref_cnt, 0);

	scene_mgr_node->init = scene_id_mgr_init;
	scene_mgr_node->deinit = scene_id_mgr_deinit;
	scene_mgr_node->reset = scene_id_mgr_reset;
	scene_mgr_node->ioctl = scene_id_mgr_ioctl;

	list_add_tail(&scene_mgr_node->list_node, resource_head);
	dpu_pr_info("dpu_res_register_opr_mgr success! scene_id_mgr_node addr=%pK", scene_mgr_node);
}