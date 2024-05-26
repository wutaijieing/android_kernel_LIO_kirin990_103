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
#include "hisi_disp_rm_debug.h"
#include "hisi_disp.h"
#include "hisi_disp_rm.h"

static int hisi_rm_debug_reqeust_pre_resource(struct hisi_display_frame *frame)
{
	struct pre_pipeline *pre_pipeline = NULL;
	struct hisi_disp_resource_type *type_operator = NULL;
	bool request_succ = false;
	uint32_t i;
	uint32_t j;

	disp_pr_debug(" ++++ ");
	disp_pr_debug(" frame->pre_pipeline_count:%u ", frame->pre_pipeline_count);

	for (i = 0; i < frame->pre_pipeline_count; i++) {
		pre_pipeline = &frame->pre_pipelines[i];

		disp_pr_debug(" pre_pipeline:0x%p, operator_count = %d", pre_pipeline, pre_pipeline->pipeline.operator_count);

		for (j = 0; j < pre_pipeline->pipeline.operator_count; j++) {
			uint32_t type = pre_pipeline->pipeline.operators[j].info.type;

			type_operator = &g_dpu_resource_manager->operator_resource[type];

			if (!type_operator->request_resource_ops)
				continue;

			disp_pr_debug(" type_operator->type:%u ", type_operator->type);
			request_succ = type_operator->request_resource_ops(type_operator, pre_pipeline, j);
			if (!request_succ) {
				disp_pr_err("request pre operator fail, i = %u, j = %u, operator_type = %u",
					i, j, type_operator->type);
				return -1;
			}
		}
	}
	disp_pr_debug(" ---- ");
	return 0;
}

static int hisi_rm_debug_request_post_online_resource(struct hisi_display_frame *frame)
{
	struct post_online_pipeline *post_pipeline = NULL;
	struct hisi_disp_resource_type *type_operator = NULL;
	bool request_succ = false;
	uint32_t i;
	uint32_t j;

	disp_pr_debug(" frame->post_online_count:%u ", frame->post_online_count);
	for (i = 0; i < frame->post_online_count; i++) {
		post_pipeline = &frame->post_online_pipelines[i];

		for (j = 0; j < post_pipeline->pipeline.operator_count; j++) {
			uint32_t type = post_pipeline->pipeline.operators[j].info.type;
			type_operator = &g_dpu_resource_manager->operator_resource[type];

			if (!type_operator->request_resource_ops)
				continue;

			request_succ = type_operator->request_resource_ops(type_operator, post_pipeline, j);
			if (!request_succ) {
				disp_pr_err("request pre operator fail, i = %u, j = %u, operator_type = %u",
					i, j, type_operator->type);
				return -1;
			}
		}
	}

	return 0;
}

static int hisi_rm_debug_request_post_offline_resource(struct hisi_display_frame *frame)
{
	struct post_offline_pipeline *post_pipeline = NULL;
	struct hisi_disp_resource_type *type_operator = NULL;
	bool request_succ = false;
	struct pipeline *p_pipeline = NULL;
	uint32_t i;
	uint32_t j;
	uint32_t type;
	disp_pr_info(" frame->post_offline_count:%u ", frame->post_offline_count);
	for (i = 0; i < frame->post_offline_count; i++) {
		post_pipeline = &frame->post_offline_pipelines[i];
		p_pipeline = &post_pipeline->pipeline;
		disp_pr_info(" pipeline operator count = %d", p_pipeline->operator_count);
		for (j = 0; j < p_pipeline->operator_count; j++) {
			type = p_pipeline->operators[j].info.type;
			disp_pr_info(" operator type = %d", type);
			type_operator = &g_dpu_resource_manager->operator_resource[type];

			if (!type_operator->request_resource_ops)
				continue;
			disp_pr_info(" type_operator->type:%u ", type_operator->type);
			request_succ = type_operator->request_resource_ops(type_operator, post_pipeline, j);
			if (!request_succ) {
				disp_pr_err("request pre operator fail, i = %u, j = %u, operator_type = %u",
					i, j, type_operator->type);
				return -1;
			}
		}
	}

	return 0;
}

int hisi_rm_debug_request_frame_resource(struct hisi_display_frame *frame)
{
	int ret;
	disp_pr_debug(" ++++ ");
	if (!g_dpu_resource_manager) {
		disp_pr_err("resource manager has not been inited yet");
		return -1;
	}

	ret = hisi_rm_debug_reqeust_pre_resource(frame);
	if (ret) {
		disp_pr_err("request pre resource fail");
		return ret;
	}

	if (frame->post_online_count > 0 && frame->post_online_count < POST_OV_ONLINE_PIPELINE_MAX_COUNT)
		ret = hisi_rm_debug_request_post_online_resource(frame);
	else if (frame->post_offline_count > 0 && frame->post_offline_count < POST_OV_OFFLINE_PIPELINE_MAX_COUNT)
		ret = hisi_rm_debug_request_post_offline_resource(frame);
	else
		ret = -1;
	disp_pr_debug(" ---- ");
	return ret;
}


