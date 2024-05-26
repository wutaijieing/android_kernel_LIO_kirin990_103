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


#ifndef __HDMITX_DRV_H__
#define __HDMITX_DRV_H__

#include "dkmd_peri.h"
#include "dkmd_connector.h"
#include "hdmitx_ctrl.h"

#define DEV_NAME_HDMI "gfx_hdmi"
#define DTS_COMP_HDMI "dkmd,gfx_hdmi"
#define MOCK_HW_CONFIG

struct hdmitx_private {
	struct dkmd_connector_info connector_info;

	char __iomem *hdmitx_base;
	char __iomem *hsdt1_crg_base;
	char __iomem *hsdt1_sub_harden_base;

	struct clk *clk_hdmictrl_pixel;

	/* other hdmi information */
	struct hdmitx_ctrl hdmitx;

	struct device *dev;
	struct platform_device *device;
	uint32_t hpd_irq_no;
};

extern struct platform_device *g_dkmd_hdmitx_devive;
static inline struct hdmitx_private *to_hdmitx_private(struct dkmd_connector_info *info)
{
	return container_of(info, struct hdmitx_private, connector_info);
}

#endif

