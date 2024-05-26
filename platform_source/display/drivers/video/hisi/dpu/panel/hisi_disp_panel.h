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
#ifndef HISI_DRV_PANEL_H
#define HISI_DRV_PANEL_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp_panel_struct.h"

struct hisi_connector_info;

struct hisi_fastboot_display_ops {
	int (*set_fastboot)(struct platform_device *pdev);
};

struct hisi_panel_ops {
	struct hisi_pipeline_ops base;
	/* backlight ops */
	struct hisi_bl_ops bl_ops;

	/* other public panel ops */
};

struct hisi_panel_device {
	struct platform_device *pdev;

	struct hisi_panel_info panel_info;
	struct hisi_connector_info *connector_info;
};

int dpu_gpio_cmds_tx(struct gpio_desc *cmds, int cnt);

static inline bool is_mipi_cmd_panel(uint32_t panel_type)
{
	return !!(panel_type & (PANEL_MIPI_CMD | PANEL_DUAL_MIPI_CMD));
}

static inline bool is_mipi_video_panel(uint32_t panel_type)
{
	return !!(panel_type & (PANEL_MIPI_VIDEO | PANEL_DUAL_MIPI_VIDEO | PANEL_RGB2MIPI));
}

static inline bool is_dual_mipi_panel(uint32_t panel_type)
{
	return !!(panel_type & (PANEL_DUAL_MIPI_VIDEO | PANEL_DUAL_MIPI_CMD));
}

#endif /* HISI_DRV_PANEL_H */
