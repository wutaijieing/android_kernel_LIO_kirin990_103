/*
 * lcd_kit_debug_qcom.c
 *
 * lcdkit debug function for lcd driver of qcom platform
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "lcd_kit_common.h"
#include "lcd_kit_dbg.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_drm_panel.h"
#include <base.h>

static int dbg_dsi_cmds_rx(uint8_t *out, int out_len,
	struct lcd_kit_dsi_panel_cmds *cmds)
{
	struct qcom_panel_info *panel_info = NULL;

	panel_info = lcm_get_panel_info(PRIMARY_PANEL);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!out || !cmds) {
		LCD_KIT_ERR("out is null or cmds is null\n");
		return LCD_KIT_FAIL;
	}
	return lcd_kit_dsi_cmds_rx(panel_info->display, out, out_len, cmds);
}

static bool dbg_panel_power_on(void)
{
	LCD_KIT_INFO("power on: %d\n", common_ops->panel_power_on);
	return common_ops->panel_power_on;
}

static struct lcd_kit_dbg_ops qcom_dbg_ops = {
	.dbg_mipi_rx = dbg_dsi_cmds_rx,
	.panel_power_on = dbg_panel_power_on,
};

int lcd_kit_dbg_init(void)
{
	LCD_KIT_INFO("enter\n");
	return lcd_kit_debug_register(&qcom_dbg_ops);
}

