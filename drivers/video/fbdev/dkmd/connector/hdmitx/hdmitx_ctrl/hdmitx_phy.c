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
#include <linux/bitops.h>
#include <linux/delay.h>

#include "dkmd_log.h"
#include "hdmitx_ctrl.h"

#define FPGA_PHY_CTRL0 0x84
#define HDMI_PHY_MASK GENMASK(4, 0)

int hdmitx_phy_init(struct hdmitx_ctrl *hdmitx)
{
	uint32_t val;
	uint32_t mask = HDMI_PHY_MASK;
	uint32_t cnt = 0;

	/* FPGA PHY init process */
	hdmi_writel(hdmitx->sysctrl_base, FPGA_PHY_CTRL0, 0x001FC000);
	hdmi_writel(hdmitx->sysctrl_base, FPGA_PHY_CTRL0, 0x0);

	do {
		val = hdmi_readl(hdmitx->sysctrl_base, FPGA_PHY_CTRL0);
		if ((val & mask) == 0x3FFF) {
			dpu_pr_info("FPGA PHY init OK\n");
			return 0;
		}
		cnt++;
		mdelay(1000);
	} while (cnt <= 10);

	dpu_pr_err("FPGA PHY init timed out\n");
	return -1;
}

