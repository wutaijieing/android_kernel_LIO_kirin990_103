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

#include <linux/fb.h>

#include <dpu_format.h>
#include <dpu/soc_dpu_define.h>
#include "dkmd_dpu.h"
#include "dkmd_comp.h"
#include "dkmd_base_frame.h"
#include "dkmd_base_layer.h"
#include "dkmd_network.h"
#include "dkmd_opr_id.h"
#include "operators/cmd_manager.h"
#include "dpu_fb_pan_display.h"
#include "dpu_fb.h"

#define PAN_DISPLAY_OPR_NUM 3

struct screen_format_info {
	uint32_t offset;
	int32_t format;
};

static int32_t pan_display_get_screen_format(const struct screen_format_info *format_info,
	uint32_t len, uint32_t offset)
{
	uint32_t i;

	for (i = 0; i < len; ++i) {
		if (offset == format_info[i].offset)
			return format_info[i].format;
	}

	dpu_pr_err("offset=%u not support", offset);
	return -1;
}

static int32_t pan_display_get_format(const struct fb_var_screeninfo *var)
{
	struct screen_format_info blue_info[] = {
		{8, DPU_FMT_RGBX_4444},
		{10, DPU_FMT_RGBX_5551},
		{11, DPU_FMT_BGR_565}
	};

	struct screen_format_info red_info[] = {
		{8, DPU_FMT_BGRX_4444},
		{10, DPU_FMT_BGRX_5551},
		{11, DPU_FMT_RGB_565}
	};

	if (var->bits_per_pixel == 32) {
		if (var->blue.offset == 0)
			return (var->transp.length == 8) ? DPU_FMT_BGRA_8888 : DPU_FMT_BGRX_8888;

		return (var->transp.length == 8) ? DPU_FMT_RGBA_8888 : DPU_FMT_RGBX_8888;
	} else if (var->bits_per_pixel == 16) {
		if (var->transp.offset == 12) {
			blue_info[0].format = DPU_FMT_RGBA_4444;
			blue_info[1].format = DPU_FMT_RGBA_5551;
			red_info[0].format = DPU_FMT_BGRA_4444;
			red_info[1].format = DPU_FMT_BGRA_5551;
		}

		if (var->blue.offset == 0)
			return pan_display_get_screen_format(red_info, ARRAY_SIZE(red_info), var->red.offset);

		return pan_display_get_screen_format(blue_info, ARRAY_SIZE(blue_info), var->blue.offset);
	}

	dpu_pr_err("bits_per_pixel=%u not support", var->bits_per_pixel);
	return -1;
}

static int32_t build_post_pipeline(const struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	struct dkmd_pipeline *pipeline = NULL;

	network->post_pipeline_num = 1;
	network->post_pipelines = kzalloc(network->post_pipeline_num * sizeof(*network->post_pipelines), GFP_KERNEL);
	if (!network->post_pipelines) {
		dpu_pr_err("alloc post pipeline mem fail");
		return -1;
	}

	pipeline = &network->post_pipelines[0];
	pipeline->block_layer = kzalloc(sizeof(*pipeline->block_layer), GFP_KERNEL);
	if (!pipeline->block_layer) {
		dpu_pr_err("alloc post pipeline's block_layer mem fail");
		kfree(network->post_pipelines);
		return -1;
	}
	pipeline->block_layer->layer_type = POST_LAYER;
	pipeline->block_layer->src_rect = frame->layers[0].src_rect;
	pipeline->block_layer->dst_rect = frame->layers[0].src_rect;

	pipeline->opr_num = 1;
	pipeline->opr_ids = kzalloc(pipeline->opr_num * sizeof(*pipeline->opr_ids), GFP_KERNEL);
	if (!pipeline->opr_ids) {
		dpu_pr_err("alloc post pipeline's opr_ids mem fail");
		kfree(network->post_pipelines);
		kfree(pipeline->block_layer);
		return -1;
	}

	pipeline->opr_ids[0].info.type = OPERATOR_ITFSW;
	pipeline->opr_ids[0].info.ins = frame->scene_id;

	return 0;
}

static int32_t build_pre_pipeline(const struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	struct dkmd_pipeline *pipeline = NULL;

	network->pre_pipeline_num = 1;
	network->pre_pipelines = kzalloc(network->pre_pipeline_num * sizeof(*network->pre_pipelines), GFP_KERNEL);
	if (!network->pre_pipelines) {
		dpu_pr_err("alloc pre pipeline mem fail");
		return -1;
	}

	pipeline = &network->pre_pipelines[0];
	pipeline->block_layer = &frame->layers[0];
	pipeline->opr_num = 2; // use sdma/ov as pre pipeline
	pipeline->opr_ids = kzalloc(pipeline->opr_num * sizeof(*pipeline->opr_ids), GFP_KERNEL);
	if (!pipeline->opr_ids) {
		dpu_pr_err("alloc pre pipeline's opr_ids mem fail");
		kfree(network->pre_pipelines);
		return -1;
	}
	pipeline->opr_ids[0].info.type = OPERATOR_SDMA;
	pipeline->opr_ids[0].info.ins = 0;
	pipeline->opr_ids[1].info.type = OPERATOR_OV;
	pipeline->opr_ids[1].info.ins = 0;

	return 0;
}

static int32_t build_network_info(const struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	struct dkmd_pipeline *pipeline = NULL;

	// pan display has no block segmentation, network is not only first block but last block
	network->is_first_block = true;
	network->is_last_block = true;

	if (build_pre_pipeline(frame, network))
		return -1;

	if (build_post_pipeline(frame, network)) {
		pipeline = &network->pre_pipelines[0];
		kfree(pipeline->opr_ids);
		kfree(network->pre_pipelines);
		return -1;
	}

	return 0;
}

static void destory_network_info(struct dkmd_network *network)
{
	uint32_t i;
	struct dkmd_pipeline *pipeline = NULL;

	for (i = 0; i < network->pre_pipeline_num; i++) {
		pipeline = &network->pre_pipelines[i];
		kfree(pipeline->opr_ids);
	}
	kfree(network->pre_pipelines);

	for (i = 0; i < network->post_pipeline_num; i++) {
		pipeline = &network->post_pipelines[i];
		kfree(pipeline->block_layer);
		kfree(pipeline->opr_ids);
	}
	kfree(network->post_pipelines);
}

static int32_t build_frame_info(const struct device_fb *dpu_fb, const struct fb_info *info,
	struct dkmd_base_frame *frame)
{
	struct dkmd_base_layer *layer = NULL;

	frame->layers_num = 1;
	frame->layers = kzalloc(frame->layers_num * sizeof(*frame->layers), GFP_KERNEL);
	if (!frame->layers) {
		dpu_pr_err("alloc layer mem fail");
		return -1;
	}

	layer = &frame->layers[0];
	layer->layer_type = SOURCE_LAYER;
	layer->format = pan_display_get_format(&info->var);
	if (unlikely(layer->format == -1)) {
		kfree(frame->layers);
		return -1;
	}
	layer->transform = TRANSFORM_NONE;
	layer->compress_type = COMPRESS_NONE;
	layer->iova = info->fix.smem_start +
		info->var.xoffset * (info->var.bits_per_pixel >> 3) + info->var.yoffset * info->fix.line_length;
	layer->phyaddr = 0;
	layer->stride = info->fix.line_length;
	layer->acquire_fence = -1;
	layer->dm_index = 0;
	layer->csc_mode = CSC_709_NARROW;
	layer->color_space = COLOR_SPACE_SRGB;
	layer->src_rect.right = info->var.xres;
	layer->src_rect.bottom = info->var.yres;
	layer->dst_rect = layer->src_rect;

	frame->scene_id = dpu_fb->pinfo->pipe_sw_itfch_idx;
	frame->scene_mode = SCENE_MODE_NORMAL;

	return 0;
}

static int32_t build_display_info(const struct device_fb *dpu_fb, const struct fb_info *info,
	struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	if (build_frame_info(dpu_fb, info, frame))
		return -1;

	if (build_network_info(frame, network)) {
		kfree(frame->layers);
		return -1;
	}

	return 0;
}

static void destory_display_info(struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	kfree(frame->layers);
	destory_network_info(network);
}

static int32_t execute_compose(struct composer *comp, struct dkmd_base_frame *frame, struct dkmd_network *network)
{
	uint32_t i;
	struct disp_frame present_frame;

	g_pan_display_frame_index++;
	if (unlikely(generate_network_cmd(frame, network) != 0)) {
		dpu_pr_err("generator cmd fail");
		return -1;
	}

	present_frame.cmdlist_id = frame->scene_cmdlist_id;
	present_frame.layer_count = frame->layers_num;
	present_frame.scene_id = frame->scene_id;
	present_frame.frame_index = g_pan_display_frame_index;
	for (i = 0; i < frame->layers_num; ++i) {
		present_frame.layer[i].acquired_fence = -1;
		present_frame.layer[i].share_fd = -1;
	}

	return comp->present(comp, (void *)&present_frame);
}

int32_t dpu_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int ret = -1;
	struct composer *comp = (struct composer *)info->par;
	struct device_fb *dpu_fb = NULL;
	struct dkmd_base_frame frame;
	struct dkmd_network network;

	if (unlikely(!comp)) {
		dpu_pr_err("comp is null");
		return -EINVAL;
	}

	if (unlikely(!info->fix.smem_start)) {
		dpu_pr_err("smem_start is 0");
		return -EINVAL;
	}

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}

	if (info->fix.xpanstep)
		info->var.xoffset = (var->xoffset / info->fix.xpanstep) * info->fix.xpanstep;

	if (info->fix.ypanstep)
		info->var.yoffset = (var->yoffset / info->fix.ypanstep) * info->fix.ypanstep;

	if (build_display_info(dpu_fb, info, &frame, &network))
		return -EINVAL;

	down(&comp->blank_sem);
	if (!comp->power_on) {
		dpu_pr_warn("%s is power off", comp->base.name);
		up(&comp->blank_sem);
		return -1;
	}
	dpu_pr_info("%s pan display, scene_id:%d, smem_start:%#x",
		comp->base.name, frame.scene_id, info->fix.smem_start);

	ret = execute_compose(comp, &frame, &network);
	if (likely(ret == 0))
		dpu_pr_info("%s pan display success", comp->base.name);
	else
		dpu_pr_err("%s pan display fail", comp->base.name);

	destory_display_info(&frame, &network);
	up(&comp->blank_sem);

	return ret;
}
