/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/of.h>
#include <linux/slab.h>

#include "dpufe_dbg.h"
#include "dpufe_panel_def.h"

/*
 * vender panel info explain format:
 * &ace_panel_info {
 *    compatible = "ace_panel_info";
 *    disp_chn_num = <2>; // dsi0 and dsi1
 *    panel_num_each_chn = <2 1>; // panel num for one dsi
 *    panel_res = <xres yres w h type xres yres w h type
 *                 xres yres w h type>; // panel info:xres,yres,w,h
 * };
 */

static bool g_info_inited = false;
static uint32_t g_disp_chn_num;
static uint32_t g_panel_num_each_chn[MAX_PANEL_NUM_EACH_DISP];
static struct panel_base_info g_ace_panel_info[MAX_DISP_CHN_NUM][MAX_PANEL_NUM_EACH_DISP];

static uint32_t get_total_panel_num(uint32_t panel_num_each_chn[], uint32_t chn_num)
{
	int i;
	uint32_t ret = 0;

	for(i = 0; i < chn_num; i++)
		ret += panel_num_each_chn[i];
	return ret;
}

uint32_t get_disp_chn_num(void)
{
	dpufe_check_and_return(!g_info_inited, 0, "panel info has not inited\n");

	return g_disp_chn_num;
}

uint32_t get_panel_num_for_one_disp(int disp_chn)
{
	dpufe_check_and_return(!g_info_inited, 0, "panel info has not inited\n");
	dpufe_check_and_return(disp_chn >= MAX_DISP_CHN_NUM, 0, "disp chn is not valid\n");

	return g_panel_num_each_chn[disp_chn];
}

struct panel_base_info *get_panel_info_for_one_disp(int disp_chn)
{
	dpufe_check_and_return(!g_info_inited, NULL, "panel info has not inited\n");
	dpufe_check_and_return(disp_chn >= MAX_DISP_CHN_NUM, NULL, "disp chn is not valid\n");

	return g_ace_panel_info[disp_chn];
}

int init_panel_info_by_dts(void)
{
	struct device_node *np = NULL;
	uint32_t panel_element_num = sizeof(struct panel_base_info) / sizeof(uint32_t);
	uint32_t *panel_res = NULL;
	uint32_t *panel_res_tmp = NULL;
	uint32_t total_panel_num;
	int ret;
	int i;
	int j;

	if (g_info_inited) {
		DPUFE_INFO("panel info has inited already\n");
		return 0;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_ACE_PANEL_INFO);
	dpufe_check_and_return(!np, -1, "np is null\n");

	ret = of_property_read_u32(np, "disp_chn_num", &g_disp_chn_num);
	dpufe_check_and_return(ret != 0, -1, "failed to get disp_chn_num\n");

	ret = of_property_read_u32_array(np, "panel_num_each_chn", g_panel_num_each_chn, g_disp_chn_num);
	dpufe_check_and_return(ret != 0, -1, "failed to get panel_num_each_chn\n");
	DPUFE_DEBUG("g_disp_chn_num =%u, g_panel_num_each_chn[0] =%u, g_panel_num_each_chn[1] =%u\n",
		g_disp_chn_num, g_panel_num_each_chn[0], g_panel_num_each_chn[1]);

	total_panel_num = get_total_panel_num(g_panel_num_each_chn, g_disp_chn_num);
	DPUFE_DEBUG("total_panel_num =%u\n",total_panel_num);

	panel_res = (uint32_t *)kmalloc(MAX_DISP_CHN_NUM * MAX_PANEL_NUM_EACH_DISP * sizeof(struct panel_base_info), \
		GFP_KERNEL);
	dpufe_check_and_return(!panel_res, -1, "malloc failed\n");
	memset(panel_res, 0, MAX_DISP_CHN_NUM * MAX_PANEL_NUM_EACH_DISP * sizeof(struct panel_base_info));

	ret = of_property_read_u32_array(np, "panel_info", panel_res, panel_element_num * total_panel_num);
	if (ret != 0) {
		DPUFE_ERR("failed to get panel_res\n");
		kfree(panel_res);
		return -1;
	}
	panel_res_tmp = panel_res;

	for (i = 0; i < g_disp_chn_num; i++) {
		DPUFE_DEBUG("panel id =%u\n", g_panel_num_each_chn[i]);
		for (j = 0; j < g_panel_num_each_chn[i]; j++) {
			memcpy(&g_ace_panel_info[i][j], panel_res_tmp, sizeof(struct panel_base_info));
			panel_res_tmp += panel_element_num;
			DPUFE_DEBUG("panel id[%d] xres:%u yres:%u w:%u h:%u type:%s\n", \
				j, g_ace_panel_info[i][j].xres, g_ace_panel_info[i][j].yres,
				g_ace_panel_info[i][j].width, g_ace_panel_info[i][j].height,
				(g_ace_panel_info[i][j].panel_type == 0) ? "video" : "cmd");
		}
	}

	kfree(panel_res);
	g_info_inited = true;
	return 0;
}
