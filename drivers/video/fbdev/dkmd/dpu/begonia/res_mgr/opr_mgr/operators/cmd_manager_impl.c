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

#include "cmd_manager_impl.h"
#include <linux/slab.h>
#include <dpu/dpu_dm.h>
#include <dkmd_cmdlist.h>
#include "dkmd_base_frame.h"
#include "dkmd_network.h"
#include "opr_cmd_data_interface.h"
#include "cmdlist_interface.h"
#include "dkmd_log.h"

int32_t request_scene_client(struct dkmd_base_frame *frame)
{
	frame->scene_cmdlist_id = cmdlist_create_user_client(frame->scene_id, SCENE_NOP_TYPE, 0, 0);
	if (unlikely(frame->scene_cmdlist_id == 0)) {
		dpu_pr_err("scene_id=%u create scene cmdlist client fail", frame->scene_id);
		return -1;
	}

	return 0;
}

int32_t request_dm_client(const struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	struct dpu_dm_param *dm_param = NULL;

	network->dm_cmdlist_id = cmdlist_create_user_client(frame->scene_id, DM_TRANSPORT_TYPE,
		g_dm_tlb_info[frame->scene_id].dm_data_addr, sizeof(struct dpu_dm_param));
	if (unlikely(network->dm_cmdlist_id == 0)) {
		dpu_pr_err("scene_id=%u create dm cmdlist client fail", frame->scene_id);
		return -1;
	}

	if (unlikely(cmdlist_append_client(frame->scene_id, frame->scene_cmdlist_id, network->dm_cmdlist_id))) {
		dpu_pr_err("append cmdlist client dm(%u) to scene(%u) fail", network->dm_cmdlist_id, frame->scene_cmdlist_id);
		return -1;
	}

	dm_param = (struct dpu_dm_param *)cmdlist_get_payload_addr(frame->scene_id, network->dm_cmdlist_id);
	if (unlikely(!dm_param))
		return -1;
	dm_param->scene_info.srot_number.reg.scene_id = frame->scene_id;
	dm_param->scene_info.scene_reserved.reg.scene_mode = frame->scene_mode;
	memset(&dm_param->cmdlist_addr.cmdlist_h_addr, 0xFF, sizeof(struct dpu_dm_cmdlist_addr));
	dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;

	if (!network->is_first_block) {
		dm_param = (struct dpu_dm_param *)cmdlist_get_payload_addr(frame->scene_id, network->pre_dm_cmdlist_id);
		if (unlikely(!dm_param))
			return -1;
		dm_param->cmdlist_addr.cmdlist_next_addr.reg.cmdlist_addr0 =
			cmdlist_get_phy_addr(frame->scene_id, network->pre_dm_cmdlist_id);
	}

	return 0;
}

int32_t request_reg_client(const struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	network->reg_cmdlist_id = cmdlist_create_user_client(frame->scene_id, REGISTER_CONFIG_TYPE, 0, PAGE_SIZE);
	if (unlikely(network->reg_cmdlist_id == 0)) {
		dpu_pr_err("scene_id=%u create reg cmdlist client fail", frame->scene_id);
		return -1;
	}
	return 0;
}

int32_t append_reg_client(const struct dkmd_base_frame *frame, const struct dkmd_network *network)
{
	if (unlikely(cmdlist_append_client(frame->scene_id, frame->scene_cmdlist_id, network->reg_cmdlist_id))) {
		dpu_pr_err("append cmdlist client reg(%u) to scene(%u) fail", network->reg_cmdlist_id, frame->scene_cmdlist_id);
		return -1;
	}

	return 0;
}

static int32_t request_opr_cfg_client(const struct dkmd_base_frame *frame, struct opr_cmd_data *cmd_data)
{
	uint32_t i;
	uint32_t cfg_cmdlist_id;
	struct opr_cmd_data_base *data = cmd_data->data;

	for (i = 0; i < data->cfg_addr_num; ++i) {
		cfg_cmdlist_id = cmdlist_create_user_client(data->scene_id, DATA_TRANSPORT_TYPE,
			data->cfg_addr_info[i].reg_addr, data->cfg_addr_info[i].reg_size);
		if (unlikely(cfg_cmdlist_id == 0)) {
			dpu_pr_err("scene_id=%u create opr cfg cmdlist client fail", data->scene_id);
			return -1;
		}
		data->cfg_addr_info[i].payload_addr = cmdlist_get_payload_addr(data->scene_id, cfg_cmdlist_id);

		if (unlikely(cmdlist_append_client(frame->scene_id, frame->scene_cmdlist_id, cfg_cmdlist_id))) {
			dpu_pr_err("append cmdlist client cfg(%u) to scene(%u) fail", cfg_cmdlist_id, frame->scene_cmdlist_id);
			return -1;
		}
		cmdlist_flush_client(frame->scene_id, cfg_cmdlist_id);
	}

	return 0;
}

void flush_all_cmdlist_client(const struct dkmd_base_frame *frame, const struct dkmd_network *network)
{
	cmdlist_flush_client(frame->scene_id, network->dm_cmdlist_id);

	if (network->is_last_block)
		cmdlist_flush_client(frame->scene_id, frame->scene_cmdlist_id);
}

static struct opr_cmd_data *set_opr_cmd_data(const struct dkmd_base_frame *frame, const struct dkmd_network *network,
	union dkmd_opr_id cur_opr_id)
{
	struct opr_cmd_data *cmd_data = get_opr_cmd_data(cur_opr_id);

	dpu_check_and_return(!cmd_data, NULL, err, "cmd_data is NULL");
	dpu_check_and_return(!cmd_data->data, NULL, err, "cmd_data->data is NULL");

	dpu_pr_debug("cur_opr_id=%#x", cur_opr_id);
	cmd_data->data->opr_id = cur_opr_id;
	cmd_data->data->scene_id = frame->scene_id;
	cmd_data->data->scene_mode = frame->scene_mode;
	cmd_data->data->dm_param =
		(struct dpu_dm_param *)cmdlist_get_payload_addr(frame->scene_id, network->dm_cmdlist_id);
	cmd_data->data->reg_cmdlist_id = network->reg_cmdlist_id;

	return cmd_data;
}

static int32_t gen_opr_cmd(struct opr_cmd_data *cmd_data, const struct dkmd_base_frame *frame,
	const struct dkmd_base_layer *base_layer, union dkmd_opr_id pre_opr_id,
	const union dkmd_opr_id *next_opr_ids, uint32_t next_opr_ids_num)
{
	struct opr_cmd_data *pre_cmd_data = NULL;
	uint32_t i;
	struct opr_cmd_data *next_cmd_datas[POST_PIPELINE_MAX_NUM] = {NULL};

	if (unlikely(!base_layer)) {
		dpu_pr_err("base_layer is NULL");
		return -1;
	}

	pre_cmd_data = get_opr_cmd_data(pre_opr_id); // sdma has no pre opr, so do not judge return value

	for (i = 0; i < next_opr_ids_num; ++i) {
		next_cmd_datas[i] = get_opr_cmd_data(next_opr_ids[i]);
		if (unlikely(!next_cmd_datas[i])) {
			dpu_pr_err("opr_id[%u]=%#x get cmd data fail", i, next_opr_ids[i]);
			return -1;
		}
	}

	if (cmd_data->get_cfg_addr &&
		unlikely(cmd_data->get_cfg_addr(&cmd_data->data->cfg_addr_info, &cmd_data->data->cfg_addr_num)))
		return -1;

	if (unlikely(request_opr_cfg_client(frame, cmd_data)))
		return -1;

	dpu_assert(!cmd_data->set_data);
	if (unlikely(cmd_data->set_data(cmd_data, base_layer, pre_cmd_data,
			(const struct opr_cmd_data **)next_cmd_datas, next_opr_ids_num)))
		return -1;

	if (cmd_data->data->cfg_addr_info)
		kfree(cmd_data->data->cfg_addr_info); // current operator's cfg_addr_info memory should be free

	return 0;
}

static int32_t set_head_oprs_info(union dkmd_opr_id *post_head_opr_ids, const struct dkmd_pipeline *post_pipelines,
	uint32_t post_pipelines_num)
{
	uint32_t i;
	struct opr_cmd_data *cmd_data = NULL;

	for (i = 0; i < post_pipelines_num; ++i) {
		if (unlikely(!post_pipelines[i].opr_ids || post_pipelines[i].opr_num == 0)) {
			dpu_pr_err("post_pipelines[%u].opr_ids is empty", i);
			return -1;
		}
		if (unlikely(!post_pipelines[i].block_layer)) {
			dpu_pr_err("post_pipelines[%u].block_layer is NULL", i);
			return -1;
		}
		post_head_opr_ids[i] = post_pipelines[i].opr_ids[0];
		dpu_pr_debug("post_pipeline=%u post_head_opr_id=%#x", i, post_head_opr_ids[i]);
		cmd_data = get_opr_cmd_data(post_head_opr_ids[i]);
		if (unlikely(!cmd_data))
			return -1;
		cmd_data->data->in_common_data.rect = post_pipelines[i].block_layer->src_rect;
	}

	return 0;
}

static int32_t gen_pipeline_cmd(const struct dkmd_base_frame *frame, const struct dkmd_network *network,
	const struct dkmd_pipeline *pipeline, union dkmd_opr_id *pre_opr_id)
{
	uint32_t i;
	struct opr_cmd_data *cmd_data = NULL;

	dpu_pr_debug("pipeline=%pK opr_num=%u", pipeline, pipeline->opr_num);
	if (unlikely(!pipeline->opr_ids || pipeline->opr_num == 0)) {
		dpu_pr_err("pipeline->opr_ids is empty");
		return -1;
	}

	for (i = 0; i < pipeline->opr_num - 1; ++i) {
		cmd_data = set_opr_cmd_data(frame, network, pipeline->opr_ids[i]);
		if (unlikely(!cmd_data))
			return -1;

		if (unlikely(gen_opr_cmd(cmd_data, frame, pipeline->block_layer, *pre_opr_id, &pipeline->opr_ids[i + 1], 1)))
			return -1;

		*pre_opr_id = pipeline->opr_ids[i];
	}

	return 0;
}

int32_t gen_pre_pipeline_cmd(const struct dkmd_base_frame *frame, const struct dkmd_network *network,
	const struct dkmd_pipeline *pre_pipeline, const struct dkmd_pipeline *post_pipelines, uint32_t post_pipelines_num)
{
	union dkmd_opr_id pre_opr_id;
	union dkmd_opr_id post_head_opr_ids[POST_PIPELINE_MAX_NUM] = {0};
	struct opr_cmd_data *cmd_data = NULL;

	dpu_pr_debug("gen_pre_pipeline_cmd start");
	pre_opr_id.info.type = OPERATOR_TYPE_MAX;
	if (unlikely(gen_pipeline_cmd(frame, network, pre_pipeline, &pre_opr_id))) {
		dpu_pr_err("generate pre pipeline cmd fail");
		return -1;
	}

	if (unlikely(set_head_oprs_info(post_head_opr_ids, post_pipelines, post_pipelines_num)))
		return -1;

	cmd_data = set_opr_cmd_data(frame, network, pre_pipeline->opr_ids[pre_pipeline->opr_num - 1]);
	if (unlikely(!cmd_data))
		return -1;

	if (unlikely(gen_opr_cmd(cmd_data, frame, pre_pipeline->block_layer, pre_opr_id,
			post_head_opr_ids, post_pipelines_num)))
		return -1;

	return 0;
}

int32_t gen_post_pipeline_cmd(const struct dkmd_base_frame *frame, const struct dkmd_network *network,
	union dkmd_opr_id pre_pipeline_last_opr_id, const struct dkmd_pipeline *post_pipeline)
{
	struct opr_cmd_data *cmd_data = NULL;

	dpu_pr_debug("gen_post_pipeline_cmd start");
	if (unlikely(gen_pipeline_cmd(frame, network, post_pipeline, &pre_pipeline_last_opr_id))) {
		dpu_pr_err("generate post pipeline cmd fail");
		return -1;
	}

	cmd_data = set_opr_cmd_data(frame, network, post_pipeline->opr_ids[post_pipeline->opr_num - 1]);
	if (unlikely(!cmd_data))
		return -1;

	if (unlikely(gen_opr_cmd(cmd_data, frame, post_pipeline->block_layer, pre_pipeline_last_opr_id, NULL, 0)))
		return -1;

	return 0;
}