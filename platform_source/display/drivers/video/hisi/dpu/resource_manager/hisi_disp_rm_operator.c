/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_disp_debug.h"
#include "hisi_disp_rm_operator.h"

static struct hisi_disp_resource *hisi_rm_get_resource_by_state(struct list_head *list, uint32_t state)
{
	struct hisi_disp_resource *node = NULL;
	struct hisi_disp_resource *_node_ = NULL;

	disp_pr_info(" ++++ ");
	disp_pr_info(" list:0x%p ", list);

	list_for_each_entry_safe(node, _node_, list, list_node) {
		disp_pr_info(" node:0x%p ", node);
		if (node->curr_state == state)
			return node;
	}

	return NULL;
}

static struct hisi_disp_resource *hisi_rm_get_resource_by_id(struct list_head *list, union operator_id id)
{
	struct hisi_disp_resource *node = NULL;
	struct hisi_disp_resource *_node_ = NULL;

	disp_pr_debug(" ++++ ");

	if (!list) {
		disp_pr_err(" list is null ptr");
		return NULL;
	}

	list_for_each_entry_safe(node, _node_, list, list_node) {
		disp_pr_debug(" node->id.id:0x%x, id.id:0x%x ", node->id.id, id.id);
		if (node->id.id == id.id) {
			disp_pr_debug(" node:0x%p ", node);
			return node;
		}
	}

	return NULL;
}

static struct hisi_disp_resource *hisi_rm_reqeust_valid_resource(struct list_head *list, struct pre_pipeline *pre_pipeline)
{
	/* TODO: need get a valid operator, such as idle, or can be reused operator
	 * now, we just need idle operator.
	 */

	return hisi_rm_get_resource_by_state(list, RESOURCE_STATE_IDLE);
}

static bool hisi_rm_get_sdma_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct pre_pipeline *pre_pipeline = (struct pre_pipeline*)input;
	struct pipeline_src_layer *src_layer = NULL;
	struct hisi_disp_resource *sdma = NULL;
	struct pipeline *pipeline  = NULL;

	src_layer = &pre_pipeline->src_layer;
	if (src_layer->img.shared_fd < 0) {
		disp_pr_err(" src_layer->img.shared_fd = %d ", src_layer->img.shared_fd);
		return false;
	}

	disp_pr_debug(" src_lapre_pipeline->pipeline.operators[0].id:%u ", pre_pipeline->pipeline.operators[index]);
	sdma = hisi_rm_get_resource_by_id(&resource_type->resource_list, pre_pipeline->pipeline.operators[index]);
	if (!sdma) {
		disp_pr_info("get idle sdma fail");
		return false;
	}

	/* TODO: migrate resource FSM to another state */
	pipeline = &pre_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_debug(" id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, sdma->id.id);

	down(&sdma->sem_resouce);
	sdma->curr_state = RESOURCE_STATE_ACTIVE;
	sdma->pipeline_id = pre_pipeline->pipeline.pipeline_id;
	atomic_inc(&sdma->ref_cnt);
	up(&sdma->sem_resouce);

	return true;
}

static bool hisi_rm_get_dsd_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct pre_pipeline *pre_pipeline = (struct pre_pipeline*)input;
	struct pipeline_src_layer *src_layer = NULL;
	struct hisi_disp_resource *dsd = NULL;
	struct pipeline *pipeline  = NULL;

	src_layer = &pre_pipeline->src_layer;
	if (src_layer->img.shared_fd < 0)
		return false;

	disp_pr_debug(" src_lapre_pipeline->pipeline.operators[0].id:%u ", pre_pipeline->pipeline.operators[index]);
	dsd = hisi_rm_get_resource_by_id(&resource_type->resource_list, pre_pipeline->pipeline.operators[index]);
	if (!dsd) {
		disp_pr_info("get idle sdma fail");
		return false;
	}

	/* TODO: migrate resource FSM to another state */
	pipeline = &pre_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_debug(" id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, dsd->id.id);

	down(&dsd->sem_resouce);
	dsd->curr_state = RESOURCE_STATE_ACTIVE;
	dsd->pipeline_id = pre_pipeline->pipeline.pipeline_id;
	atomic_inc(&dsd->ref_cnt);
	up(&dsd->sem_resouce);

	return true;
}

static bool dpu_rm_get_cld_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct pre_pipeline *pre_pipeline = (struct pre_pipeline*)input;
	struct pipeline_src_layer *src_layer = &pre_pipeline->src_layer;
	struct hisi_disp_resource *cld = NULL;
	struct pipeline *pipeline  = &pre_pipeline->pipeline;

	disp_pr_info("[CLD] src_layer:0x%p", src_layer);
	if (src_layer->img.shared_fd < 0)
		return false;

	disp_pr_info("[CLD] pre_pipeline->pipeline.operators[0].id:0x%x ", pre_pipeline->pipeline.operators[index].id);

	cld = hisi_rm_get_resource_by_id(&resource_type->resource_list, pre_pipeline->pipeline.operators[index]);
	if (!cld) {
		disp_pr_err("[CLD] get idle cld fail\n");
		return false;
	}

	/* TODO: migrate resource FSM to another state */
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_info("[CLD] id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, cld->id.id);

	down(&cld->sem_resouce);
	cld->curr_state = RESOURCE_STATE_ACTIVE;
	cld->pipeline_id = pre_pipeline->pipeline.pipeline_id;
	atomic_inc(&cld->ref_cnt);
	up(&cld->sem_resouce);

	return true;
}

static bool hisi_rm_get_overlay_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct pre_pipeline *pre_pipeline = (struct pre_pipeline*)input;
	struct pipeline_src_layer *src_layer = NULL;
	struct hisi_disp_resource *overlay = NULL;
	struct pipeline *pipeline  = NULL;

	src_layer = &pre_pipeline->src_layer;
	if (src_layer->img.shared_fd < 0)
		return false;

	disp_pr_debug(" src_lapre_pipeline->pipeline.operators[0].id:%u ", pre_pipeline->pipeline.operators[index].id);
	overlay = hisi_rm_get_resource_by_id(&resource_type->resource_list, pre_pipeline->pipeline.operators[index]);

	if (!overlay) {
		disp_pr_info("get idle overlay fail");
		return false;
	}

	/* TODO: migrate resource FSM to another state */
	pipeline = &pre_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_debug(" id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, overlay->id.id);

	down(&overlay->sem_resouce);
	overlay->curr_state = RESOURCE_STATE_ACTIVE;
	overlay->pipeline_id = pre_pipeline->pipeline.pipeline_id;
	atomic_inc(&overlay->ref_cnt);
	up(&overlay->sem_resouce);

	return true;
}

static bool hisi_rm_get_hdr_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct pre_pipeline *pre_pipeline = (struct pre_pipeline*)input;
	struct pipeline_src_layer *src_layer = &pre_pipeline->src_layer;
	struct hisi_disp_resource *hdr = NULL;
	struct pipeline *pipeline  = &pre_pipeline->pipeline;

	disp_pr_info("[hdr] src_layer:0x%p", src_layer);
	if (src_layer->img.shared_fd < 0)
		return false;

	disp_pr_info("[hdr] src_lapre_pipeline->pipeline.operators[0].id:%u ", pre_pipeline->pipeline.operators[index].id);

	hdr = hisi_rm_get_resource_by_id(&resource_type->resource_list, pre_pipeline->pipeline.operators[index]);
	if (!hdr) {
		disp_pr_err("[hdr] get idle cld fail\n");
		return false;
	}

	/* TODO: migrate resource FSM to another state */
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_info("[hdr] id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, hdr->id.id);

	down(&hdr->sem_resouce);
	hdr->curr_state = RESOURCE_STATE_ACTIVE;
	hdr->pipeline_id = pre_pipeline->pipeline.pipeline_id;
	atomic_inc(&hdr->ref_cnt);
	up(&hdr->sem_resouce);

	return true;
}

static bool hisi_rm_request_pre_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct pre_pipeline *pre_pipeline = (struct pre_pipeline*)input;
	struct pipeline_src_layer *src_layer = NULL;
	struct hisi_disp_resource *resource = NULL;
	struct pipeline *pipeline  = NULL;

	src_layer = &pre_pipeline->src_layer;
	if (src_layer->img.shared_fd < 0)
		return false;

	resource = hisi_rm_get_resource_by_id(&resource_type->resource_list, pre_pipeline->pipeline.operators[index]);
	if (!resource) {
		disp_pr_info("get idle sdma fail");
		return false;
	}

	/* TODO: migrate resource FSM to another state */
	pipeline = &pre_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_debug(" id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, resource->id.id);
	pipeline->operators[pipeline->operator_count].id = resource->id.id;

	down(&resource->sem_resouce);
	resource->curr_state = RESOURCE_STATE_ACTIVE;
	resource->pipeline_id = pre_pipeline->pipeline.pipeline_id;
	atomic_inc(&resource->ref_cnt);
	up(&resource->sem_resouce);

	return true;
}

static bool hisi_rm_itfsw_request(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct post_online_pipeline *post_pipeline = (struct post_online_pipeline *)input;
	struct pipeline *pipeline  = NULL;
	struct hisi_disp_resource *itfsw = NULL;
	union operator_id itfsw2_id;

	/* TODO: now itfsw mustbe itfsw2 or itfsw3 */
	itfsw = hisi_rm_get_resource_by_id(&resource_type->resource_list, post_pipeline->pipeline.operators[index]);
	if (!itfsw) {
		disp_pr_err("get itfsw2 resource fail");
		return false;
	}

	pipeline = &post_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);

	down(&itfsw->sem_resouce);
	itfsw->pipeline_id = pipeline->pipeline_id;
	itfsw->curr_state = RESOURCE_STATE_ACTIVE;
	atomic_inc(&itfsw->ref_cnt);
	up(&itfsw->sem_resouce);

	return true;
}


static bool hisi_rm_get_uvup_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct hisi_disp_resource *uvup = NULL;
	struct pipeline *pipeline  = (struct pipeline*)input;

	disp_pr_info("pipeline.operators[0].id:%u ", pipeline->operators[index]);
	uvup = hisi_rm_get_resource_by_id(&resource_type->resource_list, pipeline->operators[index]);
	if (!uvup) {
		disp_pr_info("get idle uvup fail");
		return false;
	}

	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_info(" id:0x%x, id:0x%x ", pipeline->operators[pipeline->operator_count].id, uvup->id.id);

	down(&uvup->sem_resouce);
	uvup->curr_state = RESOURCE_STATE_ACTIVE;
	uvup->pipeline_id = pipeline->pipeline_id;
	atomic_inc(&uvup->ref_cnt);
	up(&uvup->sem_resouce);

	return true;
}

static bool hisi_rm_get_dpp_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct post_online_pipeline *post_pipeline = (struct post_online_pipeline *)input;
	struct pipeline *pipeline  = NULL;
	struct hisi_disp_resource *dpp = NULL;

	dpp = hisi_rm_get_resource_by_id(&resource_type->resource_list, post_pipeline->pipeline.operators[index]);
	if (!dpp) {
		disp_pr_err("get dpp resource fail");
		return false;
	}

	pipeline = &post_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);

	disp_pr_info("get dpp operator successl");

	down(&dpp->sem_resouce);
	dpp->pipeline_id = pipeline->pipeline_id;
	dpp->curr_state = RESOURCE_STATE_ACTIVE;
	atomic_inc(&dpp->ref_cnt);
	up(&dpp->sem_resouce);

	return true;
}

static bool hisi_rm_get_ddic_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct post_online_pipeline *post_pipeline = (struct post_online_pipeline *)input;
	struct pipeline *pipeline  = NULL;
	struct hisi_disp_resource *ddic = NULL;

	ddic = hisi_rm_get_resource_by_id(&resource_type->resource_list, post_pipeline->pipeline.operators[index]);
	if (!ddic) {
		disp_pr_err("get ddic resource fail");
		return false;
	}

	pipeline = &post_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);

	disp_pr_info("get ddic operator successl");

	down(&ddic->sem_resouce);
	ddic->pipeline_id = pipeline->pipeline_id;
	ddic->curr_state = RESOURCE_STATE_ACTIVE;
	atomic_inc(&ddic->ref_cnt);
	up(&ddic->sem_resouce);

	return true;
}

static bool hisi_rm_get_dsc_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct post_online_pipeline *post_pipeline = (struct post_online_pipeline *)input;
	struct pipeline *pipeline  = NULL;
	struct hisi_disp_resource *dsc = NULL;

	dsc = hisi_rm_get_resource_by_id(&resource_type->resource_list, post_pipeline->pipeline.operators[index]);
	if (!dsc) {
		disp_pr_err("get dsc resource fail");
		return false;
	}

	pipeline = &post_pipeline->pipeline;

	disp_assert_if_cond(pipeline->operator_count >= PRE_PIPELINE_OPERATORS_MAX_COUNT);
	disp_pr_info("get dsc operator successl");

	down(&dsc->sem_resouce);
	dsc->pipeline_id = pipeline->pipeline_id;
	dsc->curr_state = RESOURCE_STATE_ACTIVE;
	atomic_inc(&dsc->ref_cnt);
	up(&dsc->sem_resouce);

	return true;
}

static bool hisi_rm_get_wch_operator(struct hisi_disp_resource_type *resource_type, void *input, uint32_t index)
{
	struct post_offline_pipeline *post_pipeline = (struct post_offline_pipeline *)input;
	struct pipeline *pipeline  = NULL;
	struct hisi_disp_resource *wch = NULL;

	wch = hisi_rm_get_resource_by_id(&resource_type->resource_list, post_pipeline->pipeline.operators[index]);
	if (!wch) {
		disp_pr_err("get wch resource fail");
		return false;
	}

	pipeline = &post_pipeline->pipeline;
	disp_assert_if_cond(pipeline->operator_count >= POST_OV_OFFLINE_PIPELINE_MAX_COUNT);

	disp_pr_info("get wch operator successl");

	down(&wch->sem_resouce);
	wch->pipeline_id = pipeline->pipeline_id;
	wch->curr_state = RESOURCE_STATE_ACTIVE;
	atomic_inc(&wch->ref_cnt);
	up(&wch->sem_resouce);

	return true;
}

/* TODO: now pre pipeline operators have the same request function, we need to implement each operator's function */
static request_resource_cb g_operator_resouce_match_tbl[COMP_OPS_TYPE_MAX] = {
	hisi_rm_get_sdma_operator, NULL, NULL, hisi_rm_request_pre_operator, hisi_rm_request_pre_operator,
	hisi_rm_get_hdr_operator, dpu_rm_get_cld_operator, hisi_rm_get_uvup_operator, hisi_rm_get_overlay_operator,
	hisi_rm_get_dpp_operator, hisi_rm_get_ddic_operator, hisi_rm_get_dsc_operator, NULL, NULL, NULL,
	hisi_rm_get_wch_operator, hisi_rm_itfsw_request, hisi_rm_get_dsd_operator
};

struct hisi_disp_resource *hisi_disp_rm_create_operator(union operator_id id, struct hisi_comp_operator *operator_data)
{
	struct hisi_disp_resource *operator = NULL;

	operator = kzalloc(sizeof(*operator), GFP_KERNEL);
	if (!operator) {
		disp_pr_err("alloc operator %u resource fail", id.id);
		return NULL;
	}

	operator->id.id = id.id;
	operator->data = operator_data;
	atomic_set(&(operator->ref_cnt), 1);
	sema_init(&operator->sem_resouce, 1);
	operator->curr_state = RESOURCE_STATE_IDLE;
	return operator;
}

request_resource_cb hisi_disp_rm_get_request_func(uint32_t type)
{
	disp_pr_info(" type = %u ", type);
	return g_operator_resouce_match_tbl[type % COMP_OPS_TYPE_MAX];
}
