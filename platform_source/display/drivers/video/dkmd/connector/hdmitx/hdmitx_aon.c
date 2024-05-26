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
#include <linux/interrupt.h>

#include "dkmd_log.h"
#include "hdmitx_drv.h"
#include "hdmitx_connector.h"

#define HDMI_RST 0x64
#define AON_INTR_MASK 0x30
#define AON_INTR_STATE 0x34
#define AON_STATE 0x28

static irqreturn_t hdmi_hpd_irq(int32_t irq, void *ptr)
{
	uint32_t isr_status;
	struct hdmitx_ctrl *hdmitx = NULL;

	hdmitx = (struct hdmitx_ctrl *)ptr;
	dpu_check_and_return(!hdmitx, IRQ_NONE, err, "connector is null!");

	isr_status = inp32(hdmitx->aon_base + AON_INTR_STATE);
	if ((isr_status & BIT(0)) != 0x1) {
		dpu_pr_info("is not hpd");
		return IRQ_NONE;
	}
	return IRQ_WAKE_THREAD;
}

static irqreturn_t hdmi_hpd_thread_irq(int32_t irq, void *ptr)
{
	uint32_t isr_status;
	struct hdmitx_ctrl *hdmitx = NULL;

	hdmitx = (struct hdmitx_ctrl *)ptr;

	dpu_check_and_return(!hdmitx, IRQ_NONE, err, "connector is null!");

	isr_status = inp32(hdmitx->aon_base + AON_STATE);
	if ((isr_status & BIT(0)) == 0x1) {
		hdmitx_handle_hotplug(hdmitx);
		set_reg(hdmitx->aon_base + AON_INTR_STATE, 0x1, 1, 0);
	} else {
		hdmitx_handle_unhotplug(hdmitx);
	}
	return 0;
}

void hdmitx_hpd_setup(struct hdmitx_private *hdmitx_priv)
{
	int32_t ret;

	if (hdmitx_priv->hpd_irq_no < 0) {
		dpu_pr_warn("error irq_no: %d", hdmitx_priv->hpd_irq_no);
		return;
	}

	ret = devm_request_threaded_irq(hdmitx_priv->dev,
			hdmitx_priv->hpd_irq_no, hdmi_hpd_irq, hdmi_hpd_thread_irq,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"hdmitx_hpd", (void *)&hdmitx_priv->hdmitx);
	if (ret) {
		dpu_pr_err(" Failed to request press interupt handler!\n");
		return;
	}

	/* default disable irq */
	enable_irq(hdmitx_priv->hpd_irq_no);
}

int hdmitx_aon_init(struct hdmitx_private *hdmitx_priv)
{
	struct hdmitx_ctrl *hdmitx = NULL;

	hdmitx = &hdmitx_priv->hdmitx;
	//  aon init
	set_reg(hdmitx_priv->hsdt1_sub_harden_base + HDMI_RST, 0x3, 2, 11);

	//  AON 24MHz clock
	set_reg(hdmitx_priv->hsdt1_crg_base + 0x30, 0x1, 1, 21);

	//  enable
	set_reg(hdmitx->aon_base + AON_INTR_MASK, 0x1, 1, 0);

	hdmitx_priv->hpd_irq_no = 807;

	hdmitx_hpd_setup(hdmitx_priv);
	return 0;
}

