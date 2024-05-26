/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef __PANEL_DRV_H__
#define __PANEL_DRV_H__

#include <dkmd_dirty.h>
#include "peri/dkmd_peri.h"
#include "dkmd_backlight_common.h"

#define LCD_BL_TYPE_NAME "lcd-bl-type"
#define LCD_DISP_TYPE_NAME "lcd-display-type"
#define LCD_IFBC_TYPE_NAME "lcd-ifbc-type"
#define LCD_BLPWM_DEV_NAME "lcd-bl-ic-name"

#define PIPE_SW_ITFSW_IDX_NAME "pipe_sw_itfch_idx"
#define PIPE_SW_POST_CHN_IDX_NAME "pipe_sw_post_chn_idx"
#define ACTIVE_FLAG_NAME "active_flag"
#define FAKE_PANEL_FLAG_NAME "fake_panel_flag"
#define FPGA_FLAG_NAME "fpga_flag"

struct panel_drv_private {
	/* would setup for dsi */
	struct dkmd_connector_info connector_info;

	/* private backlight info */
	struct dkmd_bl_info bl_info;

	/* Local refresh screen constraints */
	struct lcd_dirty_region_info dirty_region_info;

	/* private peri interface config */
	struct dsi_cmd_desc *disp_on_cmds;
	uint32_t disp_on_cmds_len;
	struct dsi_cmd_desc *disp_off_cmds;
	uint32_t disp_off_cmds_len;

	struct gpio_desc *gpio_request_cmds;
	uint32_t gpio_request_cmds_len;
	struct gpio_desc *gpio_free_cmds;
	uint32_t gpio_free_cmds_len;

	struct vcc_desc *vcc_enable_cmds;
	uint32_t vcc_enable_cmds_len;
	struct vcc_desc *vcc_disable_cmds;
	uint32_t vcc_disable_cmds_len;

	struct pinctrl_cmd_desc *pinctrl_normal_cmds;
	uint32_t pinctrl_normal_cmds_len;
	struct pinctrl_cmd_desc *pinctrl_lowpower_cmds;
	uint32_t pinctrl_lowpower_cmds_len;

	struct gpio_desc *gpio_normal_cmds;
	uint32_t gpio_normal_cmds_len;
	struct gpio_desc *gpio_lowpower_cmds;
	uint32_t gpio_lowpower_cmds_len;

	struct pinctrl_cmd_desc *pinctrl_init_cmds;
	uint32_t pinctrl_init_cmds_len;
	struct pinctrl_cmd_desc *pinctrl_finit_cmds;
	uint32_t pinctrl_finit_cmds_len;

	struct vcc_desc *vcc_init_cmds;
	uint32_t vcc_init_cmds_len;
	struct vcc_desc *vcc_finit_cmds;
	uint32_t vcc_finit_cmds_len;

	/* private backlight ctrl */
	struct dkmd_bl_ctrl bl_ctrl;

	struct platform_device *pdev;
};

struct panel_match_data {
	int32_t lcd_id;
	int32_t (*of_device_setup)(struct panel_drv_private *priv);
	void (*of_device_release)(struct panel_drv_private *priv);
};

#define panel_device_match_data(name, id, setup, release) \
	struct panel_match_data name = { .lcd_id = id, .of_device_setup = setup, .of_device_release = release }

static inline struct panel_drv_private *to_panel_private(struct dkmd_connector_info *pinfo)
{
	return container_of(pinfo, struct panel_drv_private, connector_info);
}

#endif
