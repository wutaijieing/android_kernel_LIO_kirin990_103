/* Copyright (c) 2022, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_HDMIRX_STRUCT_H
#define HISI_HDMIRX_STRUCT_H

#include <linux/platform_device.h>
#include <linux/types.h>
#include "hisi_chrdev.h"

struct hdmirx_ctrl_st {
	uint32_t hpd_gpio;
	uint32_t hpd_irq;

	uint32_t packet_irq;
	uint32_t hvchange_irq;

	// iomap
	void __iomem *hdmirx_aon_base; /* SOC_ACPU_HDMI_CTRL_AON_BASE_ADDR (0xEAB40000) */
	void __iomem *hdmirx_pwd_base; /* SOC_ACPU_HDMI_CTRL_PWD_BASE_ADDR (0xEAB00000) */
	void __iomem *hdmirx_sysctrl_base; /* SOC_ACPU_HDMI_CTRL_SYSCTRL_BASE_ADDR (0xEAB44000) */
	void __iomem *hdmirx_hsdt1_crg_base; /* SOC_ACPU_HSDT1_CRG_BASE_ADDR (0xEB045000) */
	void __iomem *hdmirx_ioc_base; /* SOC_ACPU_IOC_BASE_ADDR (0xFED02000) */
	void __iomem *hsdt1_sysctrl_base; /* SOC_ACPU_HSDT1_SCTRL_BASE_ADDR (0xEB040000) */
	void __iomem *hi_gpio14_base; /* SOC_ACPU_GPIO14_BASE_ADDR (0xFEC20000) */
};

struct hdmirx_chr_dev_st {
	struct platform_device *pdev;
	struct hisi_disp_chrdev chrdev;

	// hdmirx Êý¾Ý
	struct hdmirx_ctrl_st hdmirx_ctrl;
};

#endif

