/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "hdcp_reg.h"

static int hdcp_soft_handle_cp_irq(struct dp_ctrl *dptx)
{
	(void)dptx;
	return RET_HDCP_CP_IRQ;
}

void hdcp_cb_fun_register(struct hdcp_cb_fun *hdcp_cb)
{
	if (!hdcp_cb) {
		DPU_FB_ERR("hdcp_cb is null\n");
		return;
	}

	hdcp_cb->intr_en = NULL;
	hdcp_cb->intr_dis = NULL;
	hdcp_cb->handle_cp_irq = hdcp_soft_handle_cp_irq;
	hdcp_cb->handle_intr = NULL;
	hdcp_cb->init = NULL;
	hdcp_cb->deinit = NULL;
	hdcp_cb->check_status = NULL;
}
