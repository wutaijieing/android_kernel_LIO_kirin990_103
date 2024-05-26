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

#include "hdmirx_ctrl_irq.h"
#include <linux/of_irq.h>
#include "hdmirx_common.h"
#include "hisi_disp_config.h"
#include "packet/hdmirx_packet.h"
#include "video/hdmirx_video.h"
#include "hdmirx_ctrl.h"
#include "link/hdmirx_link_frl.h"
#include "video/hdmirx_video_dsc.h"

#define IRQ_HDMIRX_PACKET_NAME "irq_hdmirx_packet" // "intr_hdmi_hdcp_int_gic" // "irq_hdmirx_packet"
#define IRQ_HDMIRX_HV_NAME "irq_hdmirx_hv" // "intr_hdmi_phy_int_gic" // "irq_hdmirx_hv"

irqreturn_t hdmirx_ctrl_irq_video_detect(int irq, void *ptr)
{
	struct hdmirx_ctrl_st *hdmirx_ctrl = NULL;

	hdmirx_ctrl = (struct hdmirx_ctrl_st *)ptr;
	if (hdmirx_ctrl == NULL) {
		disp_pr_err("hdmirx_ctrl is NULL");
		return IRQ_NONE;
	}

	disp_pr_info("hdmirx_ctrl hv wake thread\n");

	hdmirx_video_detect_process(hdmirx_ctrl);

	hdmirx_packet_reg_depack(hdmirx_ctrl);

	hdmirx_frl_process(hdmirx_ctrl);

	return IRQ_WAKE_THREAD;
}

irqreturn_t hdmirx_ctrl_irq_thread_video_detect(int irq, void *ptr)
{
	struct hdmirx_ctrl_st *hdmirx = NULL;

	hdmirx = (struct hdmirx_ctrl_st *)ptr;
	if (hdmirx == NULL) {
		disp_pr_err("hdmirx_ctrl is NULL\n");
		return IRQ_NONE;
	}

	disp_pr_info("video detect thread\n");

	return IRQ_HANDLED;
}

int hdmirx_ctrl_hv_change_irq_setup(struct hdmirx_ctrl_st *hdmirx)
{
	int ret = 0;
	uint32_t irq_no = hdmirx->hvchange_irq;

	disp_pr_info("hdmirx_ctrl_hv_change_irq_setup %d\n", irq_no);

	ret = request_threaded_irq(irq_no, hdmirx_ctrl_irq_video_detect, hdmirx_ctrl_irq_thread_video_detect,
		     0, IRQ_HDMIRX_HV_NAME, (void *)hdmirx);
	if (ret != 0) {
		disp_pr_err("hdmirx request_irq failed, irq_no=%d error=%d!\n", irq_no, ret);
		return ret;
	}
	disable_irq(irq_no);

	return ret;
}

irqreturn_t hdmirx_ctrl_irq_ram_packet(int irq, void *ptr)
{
	struct hdmirx_ctrl_st *hdmirx_ctrl = NULL;
	int ret;

	hdmirx_ctrl = (struct hdmirx_ctrl_st *)ptr;
	if (hdmirx_ctrl == NULL) {
		disp_pr_err("hdmirx_ctrl is NULL");
		return IRQ_NONE;
	}

	disp_pr_info("hdmirx_ctrl_irq_ram_packet test\n");
	ret = hdmirx_packet_ram_depack(hdmirx_ctrl);

	hdmirx_video_detect_process(hdmirx_ctrl);

	hdmirx_ctrl_mute_proc(hdmirx_ctrl);

	hdmirx_video_dsc_irq_proc(hdmirx_ctrl);

	hdmirx_ctrl_irq_clear(hdmirx_ctrl->hdmirx_sysctrl_base + 0x044);

	return IRQ_HANDLED;
}

int hdmirx_ctrl_irq_config_init(struct device_node *np, struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("+++");

	hdmirx->packet_irq = irq_of_parse_and_map(np, HDMIRX_IRQ_RAM_PACKET);
	hdmirx->hvchange_irq = irq_of_parse_and_map(np, HDMIRX_IRQ_VIDEO);

	disp_pr_info("packet irq %d, hvchange irq %d\n", hdmirx->packet_irq, hdmirx->hvchange_irq);

	return 0;
}

int hdmirx_ctrl_packet_irq_setup(struct hdmirx_ctrl_st *hdmirx)
{
	int ret = 0;
	uint32_t irq_no = hdmirx->packet_irq;

	disp_pr_info("packet irq %d.\n", irq_no);

	ret = request_irq(irq_no, hdmirx_ctrl_irq_ram_packet, 0, IRQ_HDMIRX_PACKET_NAME, (void *)hdmirx);
	if (ret != 0) {
		disp_pr_err("hdmirx request_irq failed, irq_no=%d error=%d!\n", irq_no, ret);
		return ret;
	}
	disable_irq(irq_no);

	return ret;
}

int hdmirx_ctrl_irq_setup(struct hdmirx_ctrl_st *hdmirx)
{
	int ret = 0;

	hdmirx_ctrl_packet_irq_setup(hdmirx);
	hdmirx_ctrl_hv_change_irq_setup(hdmirx);

	return ret;
}

int hdmirx_ctrl_irq_enable(struct hdmirx_ctrl_st *hdmirx)
{
	int ret = 0;

	disp_pr_info("hdmirx_ctrl_irq_enable %d,%d\n", hdmirx->packet_irq, hdmirx->hvchange_irq);

	enable_irq(hdmirx->packet_irq);

	enable_irq(hdmirx->hvchange_irq);

	return ret;
}

int hdmirx_ctrl_irq_disable(struct hdmirx_ctrl_st *hdmirx)
{
	int ret = 0;

	disp_pr_info("hdmirx_ctrl_irq_disable %d,%d\n", hdmirx->packet_irq, hdmirx->hvchange_irq);

	disable_irq(hdmirx->packet_irq);

	disable_irq(hdmirx->hvchange_irq);

	return ret;
}

