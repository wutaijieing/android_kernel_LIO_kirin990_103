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

#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/switch.h>
#include <securec.h>

#include "dkmd_connector.h"
#include "dpu_connector.h"
#include "dpu_conn_mgr.h"

#include "hdmitx_drv.h"
#include "hdmitx_aon.h"
#include "hdmitx_ddc.h"
#include "hdmitx_connector.h"
#include "hdmitx_frl.h"

#define AON_BASE_OFFSET 0x44000
#define SYSCTRL_BASE_OFFSET 0x50000
#define TRAINING_BASE_OFFSET 0x20000

#define HDMI_INT_CTRL 0x30
#define HDMI_INTR_UNMASK GENMASK(8, 5)
#define HDMI_INTR_CTRL_ALL GENMASK(4, 0)

#define HDMI_INTR_VSTART BIT(1)
#define HDMI_INTR_VSYNC BIT(2)
#define HDMI_INTR_HDCP BIT(4)

static int hdmitx_ctrl_on(struct dkmd_connector_info *pinfo)
{
	dpu_pr_info("[HDMI] +\n");
	return 0;
}

static int hdmitx_ctrl_off(struct dkmd_connector_info *pinfo)
{
	return 0;
}

static irqreturn_t dpu_hdmitx_isr(int32_t irq, void *ptr)
{
	uint32_t intr_status;
	struct dpu_connector *connector = NULL;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)ptr;
	struct hdmitx_private *hdmitx_priv = NULL;
	struct hdmitx_ctrl *hdmitx = NULL;
	uint32_t val = 0;

	dpu_check_and_return(!isr_ctrl, IRQ_NONE, err, "[DP] isr_ctrl is null pointer");
	connector = (struct dpu_connector *)isr_ctrl->parent;
	hdmitx_priv = to_hdmitx_private(connector->conn_info);
	dpu_check_and_return(!hdmitx_priv, IRQ_NONE, err, "[DP] hdmitx_priv is null pointer");
	hdmitx = &hdmitx_priv->hdmitx;

	intr_status = inp32(hdmitx->sysctrl_base + HDMI_INT_CTRL);
	outp32(hdmitx->sysctrl_base + HDMI_INT_CTRL, intr_status);

	if (intr_status & HDMI_INTR_VSYNC)
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_VSYNC);

	/* to fix : underflow intr notify */

	if (intr_status & HDMI_INTR_VSTART)
		dkmd_isr_notify_listener(isr_ctrl, DSI_INT_VACT0_START);

	return IRQ_HANDLED;
}

static void hdmitx_ctrl_isr_handle(struct dpu_connector *connector, struct dkmd_isr *isr_ctrl, bool enable)
{
	struct hdmitx_private *hdmitx_priv = to_hdmitx_private(connector->conn_info);
	struct hdmitx_ctrl *hdmitx = &hdmitx_priv->hdmitx;
	uint32_t mask = 0x0;

	mutex_lock(&hdmitx->hdmitx_mutex);
	if (!hdmitx->video_transfer_enable) {
		dpu_pr_info("[DP] hdmitx has already off\n");
		mutex_unlock(&hdmitx->hdmitx_mutex);
		return;
	}
	/* 1. interrupt mask */
	outp32(hdmitx->sysctrl_base + HDMI_INT_CTRL, mask);

	if (enable) {
		/* 2. enable irq */
		isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_ENABLE);

		/* 3. interrupt clear */
		outp32(hdmitx->sysctrl_base + HDMI_INT_CTRL, HDMI_INTR_CTRL_ALL);

		/* 4. interrupt umask */
		outp32(hdmitx->sysctrl_base + HDMI_INT_CTRL, HDMI_INTR_UNMASK);
	} else {
		/* 2. disable irq */
		isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_DISABLE);

		/* 3. interrupt clear  */
		outp32(hdmitx->sysctrl_base + HDMI_INT_CTRL, HDMI_INTR_CTRL_ALL);
	}
	mutex_unlock(&hdmitx->hdmitx_mutex);
}

static int32_t hdmitx_ctrl_isr_enable(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	hdmitx_ctrl_isr_handle(connector, isr_ctrl, true);

	return 0;
}

static int32_t hdmitx_ctrl_isr_disable(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	hdmitx_ctrl_isr_handle(connector, isr_ctrl, false);

	return 0;
}

static int32_t hdmitx_ctrl_isr_setup(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	(void)snprintf_s(isr_ctrl->irq_name, sizeof(isr_ctrl->irq_name),
		ISR_NAME_SIZE - 1, "irq_hdmi_%d", connector->pipe_sw_post_chn_idx);
	isr_ctrl->irq_no = connector->connector_irq;
	isr_ctrl->isr_fnc = dpu_hdmitx_isr;
	isr_ctrl->parent = connector;
	/* fake int mask for hdmitx */
	isr_ctrl->unmask = ~(DSI_INT_VSYNC | DSI_INT_VACT0_START | DSI_INT_UNDER_FLOW);

	return 0;
}

static void enable_hdmitx_timing_gen(struct dkmd_connector_info *pinfo)
{
	uint32_t val;
	struct dpu_connector *connector = get_primary_connector(pinfo);
	struct hdmitx_private *hdmitx_priv = to_hdmitx_private(connector->conn_info);
	struct hdmitx_ctrl *hdmitx = &hdmitx_priv->hdmitx;

	// to fix
}

static void disable_hdmitx_timing_gen(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = get_primary_connector(pinfo);
	struct hdmitx_private *hdmitx_priv = to_hdmitx_private(connector->conn_info);
	struct hdmitx_ctrl *hdmitx = &hdmitx_priv->hdmitx;

	// to fix
}

static int32_t hdmitx_ctrl_hdmitx_reset(struct dpu_connector *connector, const void *value)
{
	struct hdmitx_private *hdmitx_priv = to_hdmitx_private(connector->conn_info);
	struct hdmitx_ctrl *hdmitx = &hdmitx_priv->hdmitx;

	dpu_pr_info("[DP] clear enter");

	mutex_lock(&hdmitx->hdmitx_mutex);
	if (!hdmitx->video_transfer_enable) {
		dpu_pr_info("[DP] hdmitx has already off");
		hdmitx->hdmitx_underflow_clear = false;
		mutex_unlock(&hdmitx->hdmitx_mutex);
		return 0;
	}
	msleep(100);
	enable_hdmitx_timing_gen(connector->conn_info);
	hdmitx->hdmitx_underflow_clear = false;
	mutex_unlock(&hdmitx->hdmitx_mutex);

	dpu_pr_info("[DP] clear exit");

	return 0;
}

static struct connector_ops_handle_data dp_ops_table[] = {
	{ SETUP_ISR, hdmitx_ctrl_isr_setup },
	{ ENABLE_ISR, hdmitx_ctrl_isr_enable },
	{ DISABLE_ISR, hdmitx_ctrl_isr_disable },
	{ DO_CLEAR, hdmitx_ctrl_hdmitx_reset },
};

static int32_t hdmitx_ctrl_ops_handle(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value)
{
	struct dpu_connector *connector = NULL;
	dpu_check_and_return(!pinfo, -EINVAL, err, "pinfo is NULL!");

	connector = get_primary_connector(pinfo);
	dpu_check_and_return(!connector, -EINVAL, err, "connector is NULL!");

	return dkdm_connector_hanlde_func(dp_ops_table, ARRAY_SIZE(dp_ops_table), ops_cmd_id, connector, value);
}

static void hdmitx_ctrl_ldi_setup(struct dpu_connector *connector)
{
	struct dkmd_connector_info *pinfo = connector->conn_info;

	/* set default ldi parameter */
	pinfo->base.width = 16000;
	pinfo->base.height = 9000;
	pinfo->base.fps = 60;
	pinfo->ifbc_type = IFBC_TYPE_NONE;
	if (pinfo->base.fpga_flag == 1) {
		pinfo->base.xres = 1280;
		pinfo->base.yres = 720;

		connector->ldi.h_back_porch = 220;
		connector->ldi.h_front_porch = 110;
		connector->ldi.h_pulse_width = 40;
		connector->ldi.hsync_plr = 1;

		connector->ldi.v_back_porch = 20;
		connector->ldi.v_front_porch = 30;
		connector->ldi.v_pulse_width = 5;
		connector->ldi.vsync_plr = 1;

		connector->ldi.pxl_clk_rate = 27000000UL;
		connector->ldi.pxl_clk_rate_div = 1;
	} else {
		pinfo->base.xres = 1920;
		pinfo->base.yres = 1080;

		connector->ldi.h_back_porch = 148;
		connector->ldi.h_front_porch = 88;
		connector->ldi.h_pulse_width = 44;
		connector->ldi.hsync_plr = 1;

		connector->ldi.v_back_porch = 36;
		connector->ldi.v_front_porch = 4;
		connector->ldi.v_pulse_width = 5;
		connector->ldi.vsync_plr = 1;

		connector->ldi.pxl_clk_rate = 148500000UL;
		connector->ldi.pxl_clk_rate_div = 1;
	}
}

static void hdmitx_resource_init(struct dpu_connector *connector, struct hdmitx_ctrl *hdmitx)
{
	hdmitx->base = connector->connector_base;
	hdmitx->aon_base = hdmitx->base + AON_BASE_OFFSET;
	hdmitx->sysctrl_base = hdmitx->base + SYSCTRL_BASE_OFFSET;
	hdmitx->training_base = hdmitx->base + TRAINING_BASE_OFFSET;
}

void hdmitx_ctrl_default_setup(struct dpu_connector *connector)
{
	int ret = 0;
	struct hdmitx_private *hdmitx_priv = to_hdmitx_private(connector->conn_info);
	struct hdmitx_ctrl *hdmitx = NULL;

	dpu_check_and_no_retval(!hdmitx_priv, err, "[DP] hdmitx_priv is null pointer\n");
	hdmitx = &hdmitx_priv->hdmitx;
	hdmitx->connector = connector;
	hdmitx->irq = (uint32_t)(connector->connector_irq);
	connector->connector_base = hdmitx_priv->hdmitx_base;

	hdmitx_resource_init(connector, hdmitx);

	hdmitx_ctrl_ldi_setup(connector);
	/* udp to fix : hdmitx_ctrl_clk_setup */
	/* to fix: sdev if need */
	hdmitx_aon_init(hdmitx_priv);
	hdmitx_ddc_init(hdmitx);
	hdmitx_connector_init(hdmitx);
	hdmitx_frl_init(hdmitx);

	connector->on_func = hdmitx_ctrl_on;
	connector->off_func = hdmitx_ctrl_off;
	connector->ops_handle_func = hdmitx_ctrl_ops_handle;

	connector->conn_info->enable_ldi = enable_hdmitx_timing_gen;
	connector->conn_info->disable_ldi = disable_hdmitx_timing_gen;
}
