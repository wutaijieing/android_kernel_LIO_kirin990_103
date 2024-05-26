/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_CONNECTOR_UTILS_H
#define HISI_CONNECTOR_UTILS_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp_panel.h"
#include "hisi_chrdev.h"
#include "hisi_disp_bl.h"

extern uint32_t g_connector_id;

#define HISI_CONNECTOR_SLAVER_MAX 3
#define hisi_get_connector_id()  ++g_connector_id

enum {
	HISI_CONNECTOR_TYPE_MIPI = 0,
	HISI_CONNECTOR_TYPE_DP,
	HISI_CONNECTOR_TYPE_EDP,
	HISI_CONNECTOR_TYPE_MAX,
};

struct hisi_connector_base_addr {
	char __iomem *dpu_base_addr;
	char __iomem *peri_crg_base;
	char __iomem *hsdt1_crg_base;
};

struct hisi_connector_info {
	uint32_t connector_type;
	char __iomem *connector_base;

	ulong clk_bits;
	uint32_t power_step_cnt;
	uint32_t current_step;
};

struct hisi_connector_priv_ops {
	power_cb connector_on;
	power_cb connector_off;
	present_cb connector_present;
};

struct hisi_connector_component {
	uint32_t connector_id;
	struct hisi_connector_info *connector_info;
	struct hisi_connector_priv_ops connector_ops;
	struct hisi_connector_device *parent;
};

struct hisi_connector_device {
	struct platform_device *pdev;
	struct hisi_disp_chrdev mipi_chrdev;
	struct hisi_panel_info *fix_panel_info;
	struct hisi_connector_base_addr base_addr;

	struct hisi_connector_component master;

	uint32_t slaver_count;
	struct hisi_connector_component slavers[HISI_CONNECTOR_SLAVER_MAX];

	struct platform_device *next_dev;
	struct hisi_pipeline_ops *next_dev_ops;

	/* backlight info */
	struct hisi_bl_ctrl bl_ctrl;
	struct semaphore brightness_esd_sem;
};

typedef void (*connector_register_ops_cb)(struct hisi_connector_device *connector);

void hisi_drv_connector_register_ops(struct hisi_pipeline_ops **connector_ops);
void hisi_drv_connector_register_bl_ops(struct hisi_bl_ops **connector_bl_ops);


#endif /* HISI_CONNECTOR_UTILS_H */
