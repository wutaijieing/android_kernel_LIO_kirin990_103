/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/delay.h>

#include "hisi_disp_dacc.h"
#include "hisi_disp_smmu.h"
#include "layer_buf/hisi_layer_buf.h"
#include "hisi_composer_core.h"
#include "hisi_composer_init.h"
#include "hisi_comp_utils.h"
#include "block/dpu_block_algorithm.h"
#include "hisi_disp_rm_debug.h"
#include "hdmirx_init.h"

#define HISI_DPU_CMDLIST_BLOCK_MAX 28

static inline uint32_t set_operator(union operator_id *operator, uint32_t type, uint32_t id, uint32_t ops_idx)
{
	operator[ops_idx].info.type = type;
	operator[ops_idx].info.idx = id;

	disp_pr_ops(debug, operator[ops_idx]);
	disp_pr_debug("ops_idx = %d", ops_idx);
	return ++ops_idx;
}

static uint32_t hisi_get_timeout_interval(void)
{
	uint32_t timeout_interval;

	timeout_interval = 10000; //DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;

	return timeout_interval;
}

static int hisi_wait_interrupt_event(struct hisi_composer_data *ov_data, int32_t wb_type)
{
	uint32_t timeout_interval = hisi_get_timeout_interval();
	int times = 0;
	int ret;

	while (times < 50) {
		/*lint -e666*/
		ret = wait_event_interruptible_timeout(ov_data->offline_isr_info->wb_wq[wb_type],
			(ov_data->offline_isr_info->wb_done[wb_type] == 1), /* wb type status is true */
			msecs_to_jiffies(timeout_interval));
		/*lint +e666*/

		if (ret != -ERESTARTSYS)
			break;

		times++;
		mdelay(10); /* 10ms */
	}

	return ret;
}

static int hisi_check_wch_wb_finish(struct hisi_composer_data *ov_data)
{
	int ret;
	int32_t wb_type;

	wb_type = 1; //hisi_get_wb_type(pov_req);

	ret = hisi_wait_interrupt_event(ov_data, wb_type);

	ov_data->offline_isr_info->wb_flag[wb_type] = 0;
	ov_data->offline_isr_info->wb_done[wb_type] = 0;

	return ret;
}

static void hisi_offline_base_free_data(struct hisi_display_frame *frame)
{
	struct disp_overlay_block *pov_h_block_infos = NULL;
	struct ov_req_infos *pov_req = NULL;

	pov_req = &frame->ov_req;
	if (pov_req) {
		pov_h_block_infos = (struct disp_overlay_block *)(uintptr_t)(pov_req->ov_block_infos_ptr);
		if (pov_h_block_infos) {
			kfree(pov_h_block_infos);
			pov_h_block_infos = NULL;
		}
		kfree(pov_req);
	}
	pov_req = NULL;
}

static void hisi_offline_build_pre_info(struct hisi_display_frame *frame,
	struct disp_overlay_block *pov_h_block,
	int32_t wchn_idx,
	struct disp_rect *wb_block_rect)
{
	struct pipeline_src_layer *dst_layer = NULL;
	struct pipeline_src_layer *src_layer = NULL; // after block process
	struct pipeline *p_pipeline = NULL;
	int i;

	disp_pr_info("enter+++");
	frame->pre_pipeline_count = pov_h_block->layer_nums;
	for (i = 0; i < frame->pre_pipeline_count; i++) {
		/* build prepipeline */
		p_pipeline = &(frame->pre_pipelines[i].pipeline);
		disp_pr_info("frame->pre_pipelines[%d].pipeline:0x%p\n", i, p_pipeline);
		p_pipeline->pipeline_id = i;

		uint32_t operator_count = 0;

		src_layer = &(pov_h_block->layer_infos[i]);
		if (src_layer->src_type == LAYER_SRC_TYPE_LOCAL) {
			p_pipeline->operators[operator_count].info.type = COMP_OPS_SDMA;

#ifndef CONFIG_DKMD_DPU_V720
			p_pipeline->operators[operator_count].info.idx = wchn_idx == DPU_WCHN_W1 ? 2 : 3; // i % 2 == 0 ? 2:3;
#else
			p_pipeline->operators[operator_count].info.idx = 0;
#endif
		} else {
			disp_pr_info("dsd_index=%d\n", src_layer->dsd_index);
			p_pipeline->operators[operator_count].info.type = COMP_OPS_DSD;
			p_pipeline->operators[operator_count].info.idx = src_layer->dsd_index;
		}

		operator_count++;

		if (src_layer->need_caps & CAP_UVUP) { // offline wb test uvup dblk
			disp_pr_info("wb use uvup");
			p_pipeline->operators[operator_count].info.type = COMP_OPS_UVUP;
			p_pipeline->operators[operator_count].info.idx = 0;
			operator_count++;
		}

		if (src_layer->need_caps & CAP_HIGH_SCL) {
			p_pipeline->operators[operator_count].info.type = COMP_OPS_ARSR;
			p_pipeline->operators[operator_count].info.idx = 0;
			operator_count++;
		 }

		if (src_layer->need_caps & CAP_HDR) { // dprx->dsd->hdr->ov->wch
			p_pipeline->operators[operator_count].info.type = COMP_OPS_HDR;
			p_pipeline->operators[operator_count].info.idx = 0;
			operator_count++;

		}

		p_pipeline->operators[operator_count].info.type = COMP_OPS_OVERLAY;
		p_pipeline->operators[operator_count].info.idx = 2;
		operator_count++;

		p_pipeline->operator_count = operator_count;
		disp_pr_info("operator_count = %d\n", p_pipeline->operator_count);

		dst_layer = &(frame->pre_pipelines[i].src_layer);

		memcpy(dst_layer, src_layer, sizeof(struct pipeline_src_layer));
		dst_layer->dma_sel = 1 << p_pipeline->operators[0].info.idx;
		dst_layer->wb_block_rect = *wb_block_rect;

		disp_pr_info("[DATA] dst_layer->wb_ov_block_rect w = %d, h = %d", dst_layer->wb_block_rect.w, dst_layer->wb_block_rect.h);
		disp_pr_info("layer_index = %d, dst_layer dma_id = %d", i, dst_layer->dma_sel);
		disp_pr_info("layer_index = %d, src_layer dma_id = %d", i, src_layer->dma_sel);

		if (g_debug_wch_scf) {
			dst_layer->dst_rect.w = dst_layer->src_rect.w;
			dst_layer->dst_rect.h = dst_layer->src_rect.h;
			pov_h_block->ov_block_rect.w = dst_layer->src_rect.w;
			pov_h_block->ov_block_rect.h = dst_layer->src_rect.h;
		}
	}
	disp_pr_info("exit---");
}

static void hisi_offline_build_post_info(struct hisi_display_frame *frame,
	struct ov_req_infos *pov_req_h_v, bool last_block,
	struct disp_rect *wb_ov_rect,
	struct pipeline_src_layer *src_layer)
{
	int32_t i;
	struct pipeline *post_pipeline = NULL;
	struct post_offline_info *post_info = NULL;
	uint32_t ops_count = 0;

	disp_pr_info(" ++++ ");
	frame->post_offline_count = pov_req_h_v->wb_layer_nums;
	for (i = 0; i < frame->post_offline_count; i++) {
		post_info = &frame->post_offline_pipelines[i].offline_info;
		memcpy(&post_info->wb_ov_block_rect, wb_ov_rect, sizeof(struct disp_rect));
		post_info->last_block = last_block;
		memcpy(&post_info->wb_layer, &pov_req_h_v->wb_layers[i], sizeof(struct disp_wb_layer));
		post_info->wb_type = pov_req_h_v->wb_type;
		post_info->src_layer_size.x = src_layer->src_rect.x;
		post_info->src_layer_size.y = src_layer->src_rect.y;
		post_info->src_layer_size.w = src_layer->img.width;
		post_info->src_layer_size.h = src_layer->img.height;
		disp_pr_info("wb_layer idx = %d %d wb_type = %d", post_info->wb_layer.wchn_idx, pov_req_h_v->wb_layers[i].wchn_idx,
			post_info->wb_type);

		/* build post pipeline */
		post_pipeline = &frame->post_offline_pipelines[i].pipeline;
		post_pipeline->pipeline_id = i;
		ops_count = 0;

		if (post_info->wb_layer.need_caps & CAP_UVUP)
			ops_count = set_operator(post_pipeline->operators, COMP_OPS_UVUP, 0, ops_count);

		ops_count = set_operator(post_pipeline->operators, COMP_OPS_WCH, pov_req_h_v->wb_layers[i].wchn_idx, ops_count);
		post_pipeline->operator_count = ops_count;
	}

	disp_pr_info(" ---- ");
}

static int hisi_offline_block_connect(struct hisi_composer_device *com_dev)
{
	struct cmdlist_client *client = NULL;
	struct cmdlist_client *last_dm_cmdlist_node = NULL;
	struct cmdlist_client *last_cmdlist_node = NULL;
	struct hisi_dm_param *prev_block_dm = NULL;

	/* each frame or block need create cmdlist client */
	// composer_device->ov_data.scene_id = 4;
	if (com_dev->client == NULL) {
		client = dpu_cmdlist_create_client(com_dev->ov_data.scene_id, TRANSPORT_MODE);
		if (!client) {
			disp_pr_info("create cmdlist client failed! \n");
			return -1;
		}

		client->base = com_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
		com_dev->client = client;
		com_dev->ov_data.dm_param = (struct hisi_dm_param *)client->list_cmd_item;
		memset(&com_dev->ov_data.dm_param->cmdlist_addr, 0xFF, sizeof(struct hisi_dm_cmdlist_addr));
		com_dev->ov_data.dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;
		com_dev->ov_data.dm_param->cmdlist_addr.cmdlist_next_addr.value = client->cmd_header_addr;
		disp_pr_info("first block! \n");
	} else {
		last_cmdlist_node = list_last_entry(com_dev->client->scene_list_header, typeof(*client), list_node);
		last_cmdlist_node->list_cmd_header->next_list = 0;
		last_cmdlist_node->list_cmd_header->cmd_flag.bits.task_end = 1;
		last_cmdlist_node->list_cmd_header->cmd_flag.bits.last = 1;

		client = dpu_cmdlist_create_client(com_dev->ov_data.scene_id, TRANSPORT_MODE);
		if (!client) {
			disp_pr_info("create cmdlist client failed! \n");
			return -1;
		}

		prev_block_dm = com_dev->ov_data.dm_param;
#ifndef CONFIG_DKMD_DPU_V720
		prev_block_dm->cmdlist_addr.cmdlist_h_addr.reg.cmdlist_next_en = 1;
#endif
		prev_block_dm->cmdlist_addr.cmdlist_next_addr.value = client->cmd_header_addr;

		disp_pr_info("pre dm_param addr 0x%p! \n", com_dev->ov_data.dm_param);
		client->base = com_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
		com_dev->ov_data.dm_param = (struct hisi_dm_param *)client->list_cmd_item;
		disp_pr_info("cur dm_param addr 0x%p! \n", com_dev->ov_data.dm_param);
		memset(&com_dev->ov_data.dm_param->cmdlist_addr, 0xFF, sizeof(struct hisi_dm_cmdlist_addr));
		com_dev->ov_data.dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;
		com_dev->ov_data.dm_param->cmdlist_addr.cmdlist_next_addr.value = client->cmd_header_addr;

		kfree(client->scene_list_header);
		client->scene_list_header = com_dev->client->scene_list_header;
		list_add_tail(&client->list_node, com_dev->client->scene_list_header);

		dpu_cmdlist_dump_client(com_dev->client);
	}

	return 0;
}

static int hisi_offline_block_handle(struct hisi_composer_device *ov_dev, struct hisi_display_frame *frame, int block_index)
{
	struct hisi_dm_param *dm_param = NULL;
	int32_t ret;

	ret = hisi_comp_traverse_src_layer(ov_dev, frame);
	if (ret)
		return ret;

	dm_param = ov_dev->ov_data.dm_param;
	dm_param->scene_info.layer_number_union.reg.wch_number = frame->post_offline_count;
	dpu_comp_set_dm_section_info(ov_dev);

	if (block_index == 0) {
		ret = hisi_comp_traverse_offline_post(ov_dev, frame);
		if (ret)
			return ret;
	}

    return ret;
}

static int hisi_offline_block_config(struct hisi_composer_device *ov_dev, struct hisi_display_frame *frame)
{
	int ret = 0;
	int k;
	uint32_t m;
	int block_num = 0;
	struct ov_req_infos *pov_req = NULL;
	struct disp_overlay_block *pov_h_block_infos = NULL;
	struct disp_wb_layer *wb_layer4block = NULL;
	struct disp_overlay_block *pov_h_block = NULL;
	struct ov_req_infos *pov_req_h_v = NULL;
	struct disp_rect wb_ov_block_rect;
	struct hisi_composer_data *ov_data = NULL;
	bool last_block;

	disp_pr_info("frame addr = 0x%p", frame);
	pov_req = &frame->ov_req;
	pov_h_block_infos = (struct disp_overlay_block *)(uintptr_t)(pov_req->ov_block_infos_ptr);
	ov_data = &ov_dev->ov_data;
	wb_layer4block = &(pov_req->wb_layers[0]);
	pov_req_h_v = &(ov_data->ov_req);
	pov_req_h_v->ov_block_infos_ptr = (uint64_t)(uintptr_t)(ov_data->ov_block_infos);

	disp_pr_info("block_infos addr = 0x%p", pov_h_block_infos);
	disp_pr_info("enter+++");
	(void)hisi_offline_module_default(ov_data);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);
		disp_pr_info("block addr = 0x%p", pov_h_block);
		ret = get_ov_block_rect(pov_req, pov_h_block, &block_num, ov_data->ov_block_rects, false);
		if ((ret != 0) || (block_num == 0) || (block_num >= HISI_DPU_CMDLIST_BLOCK_MAX)) {
			disp_pr_err("get_ov_block_rect failed! ret=%d, block_num[%d]\n", ret, block_num);
			ret = -1;
			return ret;
		}

		disp_pr_info("block_num = %d", block_num);
		if (block_num > 1 && (wb_layer4block->need_caps != 0 || pov_h_block->layer_infos[0].transform != 0))
			g_debug_block_scl_rot = 1; /* temp test rot or scf need block */

		for (k = 0; k < block_num; k++) {
			ret = get_block_layers(pov_req, pov_h_block, *ov_data->ov_block_rects[k],
				pov_req_h_v, block_num);
			disp_check_and_return(ret != 0, -EINVAL, err, "offline get_block_layers faild!\n");

			ret = get_wb_ov_block_rect(*ov_data->ov_block_rects[k], wb_layer4block->src_rect,
					&wb_ov_block_rect, pov_req_h_v, pov_req->wb_type);
			if (ret == 0) {
				disp_pr_err("no cross! ov_block_rects[%d]{%d %d %d %d}, wb src_rect{%d %d %d %d}\n", k,
					ov_data->ov_block_rects[k]->x, ov_data->ov_block_rects[k]->y,
					ov_data->ov_block_rects[k]->w, ov_data->ov_block_rects[k]->h,
					wb_layer4block->src_rect.x, wb_layer4block->src_rect.y,
					wb_layer4block->src_rect.w, wb_layer4block->src_rect.h);
				continue;
			}
			disp_pr_info("[DATA] cur block x = %d, y = %d, w = %d, h = %d\n", wb_ov_block_rect.x, wb_ov_block_rect.y, wb_ov_block_rect.w, wb_ov_block_rect.h);
			last_block = (k == (block_num - 1)) ? true : false;

			hisi_offline_build_pre_info(frame, (struct disp_overlay_block *)(uintptr_t)pov_req_h_v->ov_block_infos_ptr, wb_layer4block->wchn_idx, &wb_ov_block_rect);
			hisi_offline_build_post_info(frame, pov_req_h_v, last_block, &wb_ov_block_rect, &(frame->pre_pipelines[0].src_layer));

			/*  debug for kernel request resource */
			ret = hisi_rm_debug_request_frame_resource(frame);
			if (ret) {
				disp_pr_err("debug request resource fail");
				return ret;
			}

			ret = hisi_offline_block_connect(ov_dev);
			if (ret != 0) {
				disp_pr_err("offline block link failed! ret=%d\n", ret);
				return -1;
			}

			ret = hisi_offline_block_handle(ov_dev, frame, m);
			if (ret != 0) {
				disp_pr_err("offline play_h_block_handle failed! ret=%d\n", ret);
				return -1;
			}

			if (k < (block_num - 1))
				(void)hisi_offline_module_default(ov_data);
		}
	}
	disp_pr_info("exit----");
	return ret;
}

static int hisi_offline_add_clear_module_reg_node(struct post_offline_pipeline* offline_pipeline,
	struct hisi_composer_data *ov_data)
{
	int ret;

	ret = hisi_offline_module_default(ov_data);
	if (ret != 0) {
		disp_pr_err("hisi_offline_module_init failed! ret=%d\n", ret);
		return ret;
	}

	// TODO: prev_module_set_regs

	return 0;
}

static int hisi_offline_init(struct hisi_composer_data *data)
{
	int ret;
	struct hisi_disp_isr_ops *isr_ops = NULL;
	char __iomem *dpu_base = data->ip_base_addrs[DISP_IP_BASE_DPU];

	disp_pr_info("enter+++");

	if (!dpu_base) {
		disp_pr_err("dpu_base_addr is null");
		return -1;
	}

	if (!data) {
		disp_pr_err("ov_data is null");
		return -1;
	}

	hisi_disp_isr_open();

	isr_ops = data->isr_ctrl[COMP_ISR_PRE].isr_ops;
	if(isr_ops) {
		isr_ops->interrupt_mask(dpu_base);
	}

	hisi_disp_isr_enable(&data->isr_ctrl[COMP_ISR_PRE]);

	if(isr_ops) {
		isr_ops->interrupt_clear(dpu_base);
		isr_ops->interrupt_unmask(dpu_base, data);
	}

	hisi_disp_dacc_init(data->ip_base_addrs[DISP_IP_BASE_DPU]);

#ifndef DEBUG_SMMU_BYPASS
	/* smmu init */
	hisi_disp_smmu_init(dpu_base,data->scene_id);
#endif
	ret = hisi_offline_reg_modules_init(data);
	if (ret != 0)
		return ret;

	disp_pr_info("exit----");
	return 0;
}

static void hisi_offline_deinit(struct hisi_composer_device *device)
{
	struct hisi_composer_data *ov_data = NULL;
	struct hisi_disp_isr_ops *isr_ops = NULL;

	disp_pr_info(" enter ++++");
	ov_data = &device->ov_data;
	if (!ov_data) {
		disp_pr_err("ov_data is null");
		return;
	}

#ifndef DEBUG_SMMU_BYPASS
	// FIXME: temporary mask smmu-deinit request, avoid unwanted turn-off to SMMU
	// hisi_disp_smmu_deinit(device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU], device->ov_data.scene_id);
#endif

	hisi_disp_isr_disable(&ov_data->isr_ctrl[COMP_ISR_PRE]);
	disp_pr_info(" exit ----");

}

static int hisi_offline_base_prepare_on(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *ov_device = NULL;
	struct hisi_composer_data *ov_data = NULL;
	int ret;
	disp_pr_info("enter+++");
	ov_device = platform_get_drvdata(pdev);
	ov_data = &ov_device->ov_data;

	disp_pr_info("exit---");
	return 0;
}

static int hisi_offline_base_composer_on(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *ov_device = NULL;
	struct hisi_composer_data *ov_data = NULL;
	int ret;

	disp_pr_info(" enter++\n");

	ov_device = platform_get_drvdata(pdev);
	ov_data = &ov_device->ov_data;

	/* 1, enable regulator */
	hisi_disp_pm_enable_regulators(ov_data->regulator_bits);

	/* 2,
	 * 2.1 enable clk
	 * 2.2 set clk rate
	 */
	hisi_disp_pm_enable_clks(ov_data->clk_bits);
	hisi_disp_pm_set_clks_default_rate(ov_data->clk_bits);

	/* TODO: config dte inner lowpower */

	/* 3, config dte register, and init the configuration */
	ret = hisi_offline_init(ov_data);
	//if (ret != 0)
		//return ret;
	/*  notify dpp on event */

	disp_pr_info(" exit--\n");
	return 0;
}

static int hisi_offline_base_prepare_off(uint32_t ov_id, struct platform_device *pdev)
{
	disp_pr_info(" +++++++\n");
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_offline_base_composer_off(uint32_t ov_id, struct platform_device *pdev)
{
	disp_pr_info(" +++++++\n");
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_offline_base_prepare(struct hisi_composer_device *composer_device, void *data)
{
	struct hisi_display_frame *frame = data;
	struct hisi_composer_data *ov_data = NULL;
	int ret;
	disp_pr_info(" +++++++\n");

	ov_data = &composer_device->ov_data;

	ret = hisi_offline_init(ov_data);
	if (ret != 0)
		return ret;

	if (composer_device->client) {
		cmdlist_release_client(composer_device->client);
		composer_device->client = NULL;
	}

	// TODO: offline_handle_device_power

	//ret = hisi_layerbuf_lock_post_wb_buf(&ov_device->post_wb_buf_list, frame);
	//if (ret) {
		//return ret;
	//}

	// why no need block_lock
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_offline_base_present(struct hisi_composer_device *composer_dev, void *data)
{
	struct hisi_display_frame *frame = (struct hisi_display_frame *)data;
	int32_t ret;
	disp_pr_info(" +++++++\n");

	ret = hisi_offline_block_config(composer_dev, frame);
	if (ret)
		goto present_err;


	// hisi_layerbuf_move_buf_to_tl(&composer_dev->post_wb_buf_list, &composer_dev->timeline);

	/* cmdlist start config */
	hisi_module_flush(composer_dev);

	if (frame->pre_pipelines[0].src_layer.src_type == LAYER_SRC_TYPE_HDMIRX)
		hdmirx_display_dss_ready();

	dpu_dump_layer(NULL, &composer_dev->ov_data.ov_req.wb_layers[0]);
	disp_pr_info(" ---- ");

	return 0;

present_err:
	// hisi_layerbuf_unlock_buf(&composer_dev->post_wb_buf_list);
	return -1;
}

static int hisi_offline_base_post(struct hisi_composer_device *composer_dev, void *data)
{
	struct hisi_composer_data *ov_data = &composer_dev->ov_data;
	struct hisi_display_frame *frame = (struct hisi_display_frame *)data;
	int wb_type = 1; // tmp for wch1
	int ret;
	disp_pr_info("enter+++\n");

	down(&(ov_data->offline_isr_info->wb_sem[wb_type]));
	ov_data->offline_isr_info->wb_flag[wb_type] = 1;
	ov_data->offline_isr_info->wb_done[wb_type] = 0;

	ret = hisi_check_wch_wb_finish(ov_data);

	// hisi_offline_base_free_data(frame);
	up(&(ov_data->offline_isr_info->wb_sem[wb_type]));

	hisi_offline_deinit(composer_dev);
	disp_pr_info("exit---\n");
	// hisi_layerbuf_unlock_buf(&composer_dev->post_wb_buf_list);
	return 0;
}

void hisi_offline_composer_register_private_ops(struct hisi_composer_priv_ops *priv_ops)
{
	disp_pr_info("enter +++++++, priv_ops=%p \n", priv_ops);

	priv_ops->prepare_on = hisi_offline_base_prepare_on;
	priv_ops->overlay_on = hisi_offline_base_composer_on;

	priv_ops->prepare_off = hisi_offline_base_prepare_off;
	priv_ops->overlay_off = hisi_offline_base_composer_off;

	priv_ops->overlay_prepare = hisi_offline_base_prepare;
	priv_ops->overlay_present = hisi_offline_base_present;
	priv_ops->overlay_post = hisi_offline_base_post;
}
