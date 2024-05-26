/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#include "dkmd_log.h"
#include "dkmd_peri.h"
#include "dpu_connector.h"
#include "mipi_dsi_dev.h"
#include "dkmd_backlight_common.h"

static int dkmd_mipi_bl_set_backlight(struct dkmd_bl_ctrl *bl_ctrl, uint32_t bl_level)
{
	struct dpu_connector *connector = NULL;
	char bl_level_adjust[3] = {
		 0x51,
		 0x00,
		 0x00
	};
	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US, sizeof(bl_level_adjust), bl_level_adjust},
	};

	connector = (struct dpu_connector *)bl_ctrl->connector;
	bl_level_adjust[1] = (bl_level >> 8) & 0xff;
	bl_level_adjust[2] = bl_level & 0xff;

	composer_active_vsync(connector->conn_info, true);

	mipi_dsi_cmds_tx(lcd_bl_level_adjust, ARRAY_SIZE(lcd_bl_level_adjust),
					 connector->connector_base);

	composer_active_vsync(connector->conn_info, false);
	dpu_pr_info("[backlight] bl_level is %u, bl_level_adjust[1]=0x%x", bl_level, bl_level_adjust[1]);
	return 0;
}

void dkmd_mipi_bl_init(struct dkmd_bl_ctrl *bl_ctrl)
{
	struct dkmd_bl_ops *bl_ops = NULL;

	if (!bl_ctrl) {
		dpu_pr_err("[backlight] bl_ops is NULL\n");
		return;
	}
	bl_ops = &bl_ctrl->bl_ops;

	bl_ops->on = NULL;
	bl_ops->off = NULL;
	bl_ops->set_backlight = dkmd_mipi_bl_set_backlight;
	bl_ops->deinit = NULL;
}

