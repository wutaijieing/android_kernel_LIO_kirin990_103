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

#ifndef __PANEL_DEV_H__
#define __PANEL_DEV_H__

#include <linux/types.h>
#include <linux/platform_device.h>

#include "panel_drv.h"

#define DTS_COMP_FAKE_PANEL "dkmd,mipi_fake_panel"
#define DTS_COMP_PANEL_TD4322 "dkmd,panel_td4322"
#define DTS_COMP_PANEL_NT37700P "dkmd,panel_nt37700p"
#define DTS_COMP_PANEL_NT37800A "dkmd,panel_nt37800a"
#define DTS_COMP_PANEL_NT37701_BRQ "dkmd,panel_nt37701_brq"
#define DTS_COMP_PANEL_RM69091 "dkmd,panel_rm69091"
#define DTS_COMP_PANEL_HX5293 "dkmd,panel_hx5293"
#define DTS_COMP_PANEL_NT36870 "dkmd,panel_nt36870"

/* Record the screen ID information, natural growth */
enum {
	FAKE_PANEL_ID = 0,
	PANEL_TD4322_ID,
	PANEL_NT37700P_ID,
	PANEL_NT37800A_ID,
	PANEL_NT37701_BRQ_ID,
	PANEL_RM69091_ID,
	PANEL_HX5293_ID,
	PANEL_NT36870_ID,
	PANEL_MAX_ID,
};

int32_t panel_base_of_device_setup(struct panel_drv_private *priv);
void panel_base_of_device_release(struct panel_drv_private *priv);

void panel_drv_data_setup(struct panel_drv_private *priv, struct device_node *np);
void panel_dev_data_setup(struct panel_drv_private *priv);

extern struct panel_match_data td4322_panel_info;
extern struct panel_match_data nt37700p_panel_info;
extern struct panel_match_data nt37800a_panel_info;
extern struct panel_match_data nt37701_brq_panel_info;
extern struct panel_match_data rm69091_panel_info;
extern struct panel_match_data hx5293_panel_info;
extern struct panel_match_data nt36870_panel_info;

#endif