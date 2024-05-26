/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#ifndef DPU_FB_PANEL_H
#define DPU_FB_PANEL_H

#include "linux/platform_device.h"
#include "dpu_fb_panel_struct.h"
#include "../include/dpu_communi_def.h"

/* record lcd display fps support value
 */
enum {
	DISPLAY_FPS_DEFAULT = 0,
	DISPLAY_FPS_90HZ = 1,
	DISPLAY_FPS_120HZ = 2,
	DISPLAY_FPS_MAX,
};

/*
 * FUNCTIONS PROTOTYPES
 */

extern uint32_t g_dts_resouce_ready;
extern mipi_ifbc_division_t g_mipi_ifbc_division[MIPI_DPHY_NUM][IFBC_TYPE_MAX];
struct dpu_fb_data_type;
int resource_cmds_tx(struct platform_device *pdev,
	struct resource_desc *cmds, int cnt);
int vcc_cmds_tx(struct platform_device *pdev, struct vcc_desc *cmds, int cnt);
int pinctrl_cmds_tx(struct platform_device *pdev, struct pinctrl_cmd_desc *cmds, int cnt);
int gpio_cmds_tx(struct gpio_desc *cmds, int cnt);

int panel_next_set_fastboot(struct platform_device *pdev);
int panel_next_on(struct platform_device *pdev);
int panel_next_off(struct platform_device *pdev);
int panel_next_lp_ctrl(struct platform_device *pdev, bool lp_enter);
int panel_next_remove(struct platform_device *pdev);
int panel_next_set_backlight(struct platform_device *pdev, uint32_t bl_level);
int panel_next_lcd_fps_scence_handle(struct platform_device *pdev, uint32_t scence);
int panel_next_lcd_fps_updt_handle(struct platform_device *pdev);
int panel_next_vsync_ctrl(struct platform_device *pdev, int enable);
int panel_next_esd_handle(struct platform_device *pdev);

bool is_mipi_cmd_panel(struct dpu_fb_data_type *dpufd);
bool is_mipi_cmd_panel_ext(struct dpu_panel_info *pinfo);
bool is_mipi_video_panel(struct dpu_fb_data_type *dpufd);
bool is_mipi_panel(struct dpu_fb_data_type *dpufd);
bool is_dual_mipi_panel(struct dpu_fb_data_type *dpufd);
bool need_config_dsi0(struct dpu_fb_data_type *dpufd);
bool need_config_dsi1(struct dpu_fb_data_type *dpufd);
bool is_dual_mipi_panel_ext(struct dpu_panel_info *pinfo);
bool is_ifbc_panel(struct dpu_fb_data_type *dpufd);
bool is_ifbc_vesa_panel(struct dpu_fb_data_type *dpufd);
bool is_single_slice_dsc_panel(struct dpu_fb_data_type *dpufd);
bool is_dsi0_pipe_switch_connector(struct dpu_fb_data_type *dpufd);
bool is_dsi1_pipe_switch_connector(struct dpu_fb_data_type *dpufd);
bool mipi_panel_check_reg(struct dpu_fb_data_type *dpufd, uint32_t *read_value, int buf_len);
int mipi_ifbc_get_rect(struct dpu_fb_data_type *dpufd, struct dss_rect *rect);
bool is_dpu_writeback_panel(struct dpu_fb_data_type *dpufd);
void dpufb_snd_cmd_before_frame(struct dpu_fb_data_type *dpufd);

void dpu_fb_device_set_status0(uint32_t status);
int dpu_fb_device_set_status1(struct dpu_fb_data_type *dpufd);
bool dpu_fb_device_probe_defer(uint32_t panel_type, uint32_t bl_type);

int32_t bl_config_max_value(void);

void dpufd_get_panel_info(struct dpu_fb_data_type *dpufd, struct dpu_panel_info *pinfo,
	uint32_t fold_disp_panel_id);

#endif /* DPU_FB_PANEL_H */
