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

#include "hisi_composer_core.h"
//#include "hisi_disp_composer.h"
#include "isr/hisi_isr_utils.h"
#include "mcu/hisi_disp_mcu.h"
#include "hisi_disp_pm.h"
#include "hisi_disp_dacc.h"
#include "hisi_disp_smmu.h"
#include "hisi_disp_dpp.h"
#include "layer_buf/hisi_layer_buf.h"
#include "hisi_comp_utils.h"
#include "notify/hisi_comp_notify.h"
#include "hisi_disp_gadgets.h"
#include "hdmirx_init.h"
#include "hisi_operator_tool.h"
#include "hisi_disp_debug.h"

void hisi_online_init(struct hisi_composer_data *data)
{
	struct hisi_disp_isr_ops *isr_ops = NULL;
	uint32_t pipe_sw_post_idx = PIPE_SW_POST_CH_DSI0;

	disp_pr_info(" +++++++ \n");
	disp_pr_info(" scene_id:%d, ov_id:%d\n", data->scene_id, data->ov_id);

	if (data->fix_panel_info->type == PANEL_DP) {
		pipe_sw_post_idx = PIPE_SW_POST_CH_DP0;
		/* FIXME: manage dp irq in composer */
	} else {
		isr_ops = data->isr_ctrl[COMP_ISR_POST].isr_ops;

		hisi_disp_isr_open();
		if(isr_ops)
			isr_ops->interrupt_mask(data->ip_base_addrs[DISP_IP_BASE_DPU]);

		hisi_disp_isr_enable(&data->isr_ctrl[COMP_ISR_POST]);
		if(isr_ops) {
			isr_ops->interrupt_clear(data->ip_base_addrs[DISP_IP_BASE_DPU]);
			isr_ops->interrupt_unmask(data->ip_base_addrs[DISP_IP_BASE_DPU], data);
		}
	}

	/* dacc init if fastboot have done not  need to do*/
	hisi_disp_dacc_init(data->ip_base_addrs[DISP_IP_BASE_DPU]);

	/* smmu init */
#ifndef DEBUG_SMMU_BYPASS
	hisi_disp_smmu_init(data->ip_base_addrs[DISP_IP_BASE_DPU], data->scene_id);
#endif
	/* TODO: init other operator */
	hisi_disp_mcu_init();
	hisi_disp_dpp_on();

	/* pipeline sw init */
	platform_init_pipe_sw(data->ip_base_addrs[DISP_IP_BASE_DPU], data->scene_id, pipe_sw_post_idx);

	disp_pr_info("----Pipe-SW : Scene-%d, %s----\n", data->scene_id,
		pipe_sw_post_idx == PIPE_SW_POST_CH_DSI0 ? "DSI0" : "DP0");

	disp_pr_info(" exit ------");
}

static void hisi_online_deinit(struct hisi_composer_device *device)
{
	struct hisi_composer_data *ov_data = NULL;
	struct hisi_disp_isr_ops *isr_ops = NULL;

	disp_pr_info(" enter ++++");

	ov_data = &device->ov_data;
	if(!ov_data) {
		disp_pr_info("ov_data null");
		return;
	}
	isr_ops = ov_data->isr_ctrl[COMP_ISR_POST].isr_ops;
	if(isr_ops) {
		isr_ops->interrupt_mask(ov_data->ip_base_addrs[DISP_IP_BASE_DPU]);
	}
#ifndef DEBUG_SMMU_BYPASS
	hisi_disp_smmu_deinit(ov_data->ip_base_addrs[DISP_IP_BASE_DPU], ov_data->scene_id);
#endif
	/* disable isr */
	hisi_disp_isr_disable(&ov_data->isr_ctrl[COMP_ISR_POST]);
	disp_pr_info(" exit ------");
}

static void hisi_online_start_frame(struct hisi_composer_device *device, struct hisi_display_frame *frame)
{
	/* if support cmdlist, need to flush cmdlist to dpu
	 * else signal frame update with cpu.
	 */
	char __iomem *dpu_base_addr = device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
	char __iomem *dp_base_addr = device->ov_data.ip_base_addrs[DISP_IP_BASE_HIDPTX0];

	/* cmdlist start config */
	hisi_module_flush(device);

	disp_pr_info("src_type %d\n", frame->pre_pipelines[0].src_layer.src_type);
	if (frame->pre_pipelines[0].src_layer.src_type == LAYER_SRC_TYPE_HDMIRX)
		hdmirx_display_dss_ready();

	if (device->ov_data.fix_panel_info->type != PANEL_DP) {
		dpu_set_reg(DPU_DSI_LDI_FRM_MSK_UP_ADDR(dpu_base_addr), 0x1, 1, 0);
		dpu_set_reg(DPU_DSI_LDI_CTRL_ADDR(dpu_base_addr), 0x1, 1, 0);
	}

	/* TODO: start the frame with cpu config */
}

static void hisi_online_flush_base_layer(struct hisi_composer_device *ov_device)
{
	/* TODO */
	disp_pr_info(" +++++++\n");
	disp_pr_info(" ---- ");
}

static int hisi_online_base_prepare_on(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *ov_device = NULL;
	struct hisi_composer_data *ov_data = NULL;

	disp_pr_info(" +++++++\n");

	ov_device = platform_get_drvdata(pdev);
	ov_data = &ov_device->ov_data;

	/* 1, enable regulator */
	hisi_disp_pm_enable_regulators(ov_data->regulator_bits);

	/* 2, set clk rate
	 * enable clk
	 */
	//hisi_disp_pm_set_clks_default_rate(ov_data->clk_bits);
	//hisi_disp_pm_enable_clks(ov_data->clk_bits);

	/* TODO: config dte inner lowpower */
	//hisi_disp_pm_set_lp(ov_data->ip_base_addrs[DISP_IP_BASE_DPU]); ybr
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_on(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *ov_device = NULL;
	struct hisi_composer_data *ov_data = NULL;
	int ret;
	disp_pr_info(" +++++++\n");
	ov_device = platform_get_drvdata(pdev);
	ov_data = &ov_device->ov_data;

	/* 1, enable regulator */
	// hisi_disp_pm_enable_regulators(ov_data->regulator_bits);

	/* 2, set clk rate
	 * enable clk
	 */
	// hisi_disp_pm_set_clks_default_rate(ov_data->clk_bits);
	// hisi_disp_pm_enable_clks(ov_data->clk_bits);

	/* TODO: config dte inner lowpower */
	// hisi_disp_pm_set_lp(ov_data->ip_base_addrs[DISP_IP_BASE_DPU]); ybr

	/* 3, config dte register, and init the configuration */
	hisi_online_init(ov_data);

	/* notify dpp on event */
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_post_on(uint32_t ov_id, struct platform_device *pdev)
{
	disp_pr_info(" +++++++\n");
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_prepare_off(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *ov_device = NULL;
	disp_pr_info(" +++++++\n");

	ov_device = platform_get_drvdata(pdev);

	/* TODO: display one empty frame, flush base */
	hisi_online_flush_base_layer(ov_device);

	/* TODO: mask isr and disable isr */
	hisi_disp_isr_close();

	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_off(uint32_t ov_id, struct platform_device *pdev)
{
	struct hisi_composer_device *ov_device = NULL;
	disp_pr_info(" +++++++\n");

	ov_device = platform_get_drvdata(pdev);

	/* TODO: deinit overlay */
	hisi_online_deinit(ov_device);

	/* TODO: disable clks and regulators */
	// hisi_disp_pm_disable_clks(ov_device->ov_data.clk_bits);
	hisi_disp_pm_disable_regulators(ov_device->ov_data.regulator_bits);

	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_post_off(uint32_t ov_id, struct platform_device *pdev)
{
	/* TODO: clear overlay context */
	disp_pr_info(" +++++++\n");

	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_prepare(struct hisi_composer_device *composer_device, void *data)
{
	struct cmdlist_client *client = NULL;
	struct hisi_display_frame *frame = data;
	int ret;

	disp_pr_info(" ++++ ");

	/* each frame need create cmdlist client */
	// composer_device->ov_data.scene_id = 3;
	if (composer_device->client) {
		cmdlist_release_client(composer_device->client);
		composer_device->client = NULL;
	}

	client = dpu_cmdlist_create_client(composer_device->ov_data.scene_id, TRANSPORT_MODE);
	if (!client) {
		disp_pr_err("create cmdlist client failed! \n");
		return -1;
	}

	client->base = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
	composer_device->client = client;
	composer_device->ov_data.dm_param = (struct hisi_dm_param *)client->list_cmd_item;
	memset(&composer_device->ov_data.dm_param->cmdlist_addr, 0xFF, sizeof(struct hisi_dm_cmdlist_addr));
	composer_device->ov_data.dm_param->cmdlist_addr.cmdlist_h_addr.value = 0;

	disp_pr_info("set cmdlist_next_addr for scene %d, ov %d\n",
		composer_device->ov_data.scene_id, composer_device->ov_data.ov_id);

	composer_device->ov_data.dm_param->cmdlist_addr.cmdlist_next_addr.value = client->cmd_header_addr;

#ifndef ARSR_REUSE
	ret = hisi_layerbuf_lock_pre_buf(&composer_device->pre_layer_buf_list, frame);
	if (ret) {
		return ret;
	}
#endif
	disp_pr_info(" ---- ");

	return 0;
}

static int hisi_online_base_present(struct hisi_composer_device *composer_device, void *data)
{
	struct hisi_display_frame *frame = (struct hisi_display_frame *)data;
	int32_t ret;

	disp_pr_info(" ++++ ");
	ret = hisi_comp_traverse_src_layer(composer_device, frame);
	if (ret)
		goto present_err;

	ret = hisi_comp_traverse_online_post(composer_device, frame);
	if (ret)
		goto present_err;

	dpu_comp_set_dm_section_info(composer_device);

	if (composer_device->ov_data.fix_panel_info->type != PANEL_DP) { /* FIXME: add dp buffer manage */
		hisi_layerbuf_move_buf_to_tl(&composer_device->pre_layer_buf_list, &composer_device->timeline);
	}

#ifdef LOCAL_DIMMING_GOLDEN_TEST
	if (g_local_dimming_en) {
		struct pipeline_src_layer *src_layer = &frame->pre_pipelines[0].src_layer;
		dpu_init_dump_layer(src_layer, NULL);
	}
#endif

	/* TODO: start display the frame */
	hisi_online_start_frame(composer_device, frame);
	disp_pr_info(" ---- ");
	return 0;

present_err:
	hisi_layerbuf_unlock_buf(&composer_device->pre_layer_buf_list);
	return -1;
}

#ifdef LOCAL_DIMMING_GOLDEN_TEST
static void dump_ld_data(struct hisi_composer_device *composer_device, void *data)
{
	char __iomem *dpu_base_addr = composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];
	uint32_t ld_irq_status = inp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C0);

	int wait_cnt = 0;
	while ((ld_irq_status & 0x2) == 0) {
		mdelay(100);
		ld_irq_status = inp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C0);
		wait_cnt++;
		if (wait_cnt > 50)
			break;
	}

	while (get_dptx_vsync_cnt() > 0 && get_dptx_vsync_cnt() < get_ld_frm_cnt())
		mdelay(100);

	disp_pr_info("ld_irq_status:0x%x, wait_cnt:%d, g_vsync_cnt:%d", ld_irq_status, wait_cnt, get_dptx_vsync_cnt());

	struct hisi_display_frame *frame = (struct hisi_display_frame *)data;
	struct pipeline_src_layer *src_layer = &frame->pre_pipelines[0].src_layer;
	dpu_dump_layer(src_layer, NULL);

	outp32(dpu_base_addr + DSS_GLB0_OFFSET + 0x4C0, 0xF);
}
#endif

static int hisi_online_base_post_present(struct hisi_composer_device *composer_device, void *data)
{
	struct disp_event online_play_event;

	disp_pr_info(" ++++ ");
	/* here, notify listener to handle online play, such as set backlight */
	online_play_event.comp = composer_device;
	composer_notifier_call_chain(DISP_EVENT_ONLINE_PLAY, &online_play_event);

#ifdef LOCAL_DIMMING_GOLDEN_TEST
	if (g_local_dimming_en)
		dump_ld_data(composer_device, data);
#endif
	disp_pr_info(" ---- ");
	return 0;
}

void hisi_online_composer_register_private_ops(struct hisi_composer_priv_ops *priv_ops)
{
	disp_pr_info("enter +++++++, priv_ops=%p \n", priv_ops);

	priv_ops->prepare_on = hisi_online_base_prepare_on;
	priv_ops->overlay_on = hisi_online_base_on;
	priv_ops->post_ov_on = hisi_online_base_post_on;

	priv_ops->prepare_off = hisi_online_base_prepare_off;
	priv_ops->overlay_off = hisi_online_base_off;
	priv_ops->post_ov_off = hisi_online_base_post_off;

	priv_ops->overlay_prepare = hisi_online_base_prepare;
	priv_ops->overlay_present = hisi_online_base_present;
	priv_ops->overlay_post = hisi_online_base_post_present;
}
