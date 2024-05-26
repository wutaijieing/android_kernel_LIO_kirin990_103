/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/module.h>
#include "cmd_manager.h"
#include "cmd_manager_impl.h"
#include "dkmd_log.h"

int32_t generate_network_cmd(struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	uint32_t i;
	union dkmd_opr_id pre_pipeline_last_opr_id;

	if (unlikely(!frame || !network)) {
		dpu_pr_err("frame or network is NULL");
		return -1;
	}

	dpu_pr_debug("scene_id=%d scene_mode=%d layers_num=%u pre_pipeline_num=%u post_pipeline_num=%u",
		frame->scene_id, frame->scene_mode, frame->layers_num, network->pre_pipeline_num, network->post_pipeline_num);

	if (unlikely(network->pre_pipeline_num == 0)) {
		dpu_pr_err("network->pre_pipelines is empty");
		return -1;
	}

	if (unlikely(network->post_pipeline_num >= POST_PIPELINE_MAX_NUM)) {
		dpu_pr_err("network->post_pipeline_num exceed %d", POST_PIPELINE_MAX_NUM);
		return -1;
	}

	if (network->is_first_block && unlikely(request_scene_client(frame)))
		return -1;

	if (unlikely(request_dm_client(frame, network)))
		return -1;

	if (unlikely(request_reg_client(frame, network)))
		return -1;

	for (i = 0; i < network->pre_pipeline_num; ++i) {
		if (unlikely(gen_pre_pipeline_cmd(frame, network, &network->pre_pipelines[i],
						network->post_pipelines, network->post_pipeline_num))) {
			dpu_pr_err("generator pre pipeline cmd fail");
			return -1;
		}
	}

	pre_pipeline_last_opr_id = network->pre_pipelines[0].opr_ids[network->pre_pipelines[0].opr_num - 1];
	for (i = 0; i < network->post_pipeline_num; ++i) {
		if (unlikely(gen_post_pipeline_cmd(frame, network, pre_pipeline_last_opr_id, &network->post_pipelines[i]))) {
			dpu_pr_err("generator post pipeline cmd fail");
			return -1;
		}
	}

	if (unlikely(append_reg_client(frame, network)))
		return -1;

	flush_all_cmdlist_client(frame, network);

	return 0;
}

MODULE_LICENSE("GPL");