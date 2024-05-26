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
#ifndef DPU_FB_H
#define DPU_FB_H

#include <time.h>
#include <semaphore.h>
#include <workqueue.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>

#include "dpu_fb_struct.h"
#include "dpu.h"
#include "dpu_fb_panel_struct.h"

/*
 * FUNCTIONS PROTOTYPES
 */
extern unsigned int g_primary_lcd_xres;
extern unsigned int g_primary_lcd_yres;
extern uint64_t g_pxl_clk_rate;

extern uint8_t g_prefix_ce_support;
extern uint8_t g_prefix_sharpness1d_support;
extern uint8_t g_prefix_sharpness2d_support;

extern uint32_t g_online_cmdlist_idxs;
extern uint32_t g_offline_cmdlist_idxs;

extern uint32_t g_fpga_flag;
extern uint32_t g_fastboot_enable_flag;
extern uint32_t g_fake_lcd_flag;
extern uint32_t g_dss_version_tag;
extern uint32_t g_dss_module_resource_initialized;
extern uint32_t g_logo_buffer_base;
extern uint32_t g_logo_buffer_size;
extern uint32_t g_underflow_stop_perf_stat;
extern uint32_t g_mipi_dphy_version; /* C30:0 C50:1 */
extern uint32_t g_mipi_dphy_opt;
extern uint32_t g_chip_id;

extern uint32_t g_fastboot_already_set;
extern struct dpu_iova_info g_dpu_iova_info;
extern struct dpu_iova_info g_dpu_vm_iova_info;

extern int g_debug_online_vactive;
void de_close_update_te(struct work_struct *work);
extern struct dpu_fb_data_type *dpufd_list[DPU_FB_MAX_FBI_LIST];
extern struct semaphore g_dpufb_dss_clk_vote_sem;
extern struct mutex g_rgbw_lock;


extern uint32_t g_post_xcc_table_temp[12];

#ifdef CONFIG_REPORT_VSYNC
extern void mali_kbase_pm_report_vsync(int);
#endif
#ifdef CONFIG_MALI_FENCE_DEBUG
extern void kbase_fence_dump_in_display(void);
#endif
extern int mipi_dsi_ulps_cfg(struct dpu_fb_data_type *dpufd, int enable);
extern bool dpu_check_reg_reload_status(struct dpu_fb_data_type *dpufd);
uint32_t get_panel_xres(struct dpu_fb_data_type *dpufd);
uint32_t get_panel_yres(struct dpu_fb_data_type *dpufd);

bool is_fastboot_display_enable(void);
bool is_dss_idle_enable(void);

/* mediacomm channel manager */
int dpu_mdc_resource_init(struct dpu_fb_data_type *dpufd, unsigned int platform);
int dpu_mdc_chn_request(struct dpu_fb_data_type *dpufd, void __user *argp);
int dpu_mdc_chn_release(struct dpu_fb_data_type *dpufd, const void __user *argp);

int dpu_fb_blank_sub(struct dpu_fb_data_type *dpufd, int blank_mode);

/* backlight */
void dpufb_backlight_update(struct dpu_fb_data_type *dpufd);
void dpufb_backlight_cancel(struct dpu_fb_data_type *dpufd);
void dpufb_backlight_register(struct platform_device *pdev);
void dpufb_backlight_unregister(struct platform_device *pdev);
void dpufb_set_backlight(struct dpu_fb_data_type *dpufd, uint32_t bkl_lvl, bool enforce);
int update_cabc_pwm(struct dpu_fb_data_type *dpufd);

void bl_flicker_detector_collect_upper_bl(int level);
void bl_flicker_detector_collect_algo_delta_bl(int level);
void bl_flicker_detector_collect_device_bl(int level);

/* vsync */
void dpufb_frame_updated(struct dpu_fb_data_type *dpufd);
void dpufb_set_vsync_activate_state(struct dpu_fb_data_type *dpufd, bool infinite);
void dpufb_activate_vsync(struct dpu_fb_data_type *dpufd);
void dpufb_deactivate_vsync(struct dpu_fb_data_type *dpufd);
int dpufb_vsync_ctrl(struct dpu_fb_data_type *dpufd, const void __user *argp);
int dpufb_vsync_resume(struct dpu_fb_data_type *dpufd);
void dpufb_vsync_isr_handler(struct dpu_fb_data_type *dpufd);
void dpufb_vsync_register(struct platform_device *pdev);
void dpufb_vsync_unregister(struct platform_device *pdev);
void dpufb_vsync_disable_enter_idle(struct dpu_fb_data_type *dpufd, bool disable);
void dpufb_masklayer_backlight_flag_config(struct dpu_fb_data_type *dpufd,
	bool masklayer_backlight_flag);

int dpufb_buf_sync_handle(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);

/* control */
int dpufb_ctrl_fastboot(struct dpu_fb_data_type *dpufd);
int dpufb_ctrl_on(struct dpu_fb_data_type *dpufd);
int dpufb_ctrl_off(struct dpu_fb_data_type *dpufd);
int dpufb_ctrl_lp(struct dpu_fb_data_type *dpufd, bool lp_enter);
int dpufb_ctrl_dss_voltage_get(struct dpu_fb_data_type *dpufd, void __user *argp);
int dpufb_ctrl_dss_voltage_set(struct dpu_fb_data_type *dpufd, void __user *argp);
bool check_primary_panel_power_status(struct dpu_fb_data_type *dpufd);
int dpufb_ctrl_dss_vote_cmd_set(struct dpu_fb_data_type *dpufd, const void __user *argp);
int dpufb_fps_upt_isr_handler(struct dpu_fb_data_type *dpufd);
int dpufb_ctrl_esd(struct dpu_fb_data_type *dpufd);

typedef void (*func_set_reg)(struct dpu_fb_data_type *, char __iomem *, uint32_t, uint8_t, uint8_t);
void set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);
uint32_t set_bits32(uint32_t old_val, uint32_t val, uint8_t bw, uint8_t bs);
void dpufb_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);
uint32_t dpufb_line_length(int index, uint32_t xres, int bpp);
void dpufb_get_timestamp(struct timeval *tv);
uint32_t dpufb_timestamp_diff(struct timeval *lasttime, struct timeval *curtime);

struct platform_device *dpu_fb_device_alloc(struct dpu_fb_panel_data *pdata,
	uint32_t type, uint32_t id);
struct platform_device *dpu_fb_add_device(struct platform_device *pdev);

int dpu_fb_open_sub(struct dpu_fb_data_type *dpufd);
int dpu_fb_release_sub(struct dpu_fb_data_type *dpufd);

void dpu_fb_frame_refresh(struct dpu_fb_data_type *dpufd, char *trigger);
int dpu_alloc_cmdlist_buffer(struct dpu_fb_data_type *dpufd);
void dpu_free_cmdlist_buffer(struct dpu_fb_data_type *dpufd);

void dpu_fb_unblank_wq_handle(struct work_struct *work);
int dpu_fb_blank_device(struct dpu_fb_data_type *dpufd, int blank_mode);
int dpu_fb_blank_panel_power_on(struct dpu_fb_data_type *dpufd);
int dpu_fb_blank_panel_power_off(struct dpu_fb_data_type *dpufd);
int dpu_fb_blank_update_tui_status(struct dpu_fb_data_type *dpufd);
void dpu_fb_pm_runtime_register(struct dpu_fb_data_type *dpufd);
void dpu_fb_fnc_register_base(struct dpu_fb_data_type *dpufd);
void dpu_fb_sdp_fnc_register(struct dpu_fb_data_type *dpufd);
void dpu_fb_mdc_fnc_register(struct dpu_fb_data_type *dpufd);
void dpu_fb_aux_fnc_register(struct dpu_fb_data_type *dpufd);
void dpu_fb_common_register(struct dpu_fb_data_type *dpufd);
bool dpu_fb_img_type_valid(uint32_t fb_imgType);

void dpu_fb_data_init(struct dpu_fb_data_type *dpufd);
void dpu_fb_init_sema(struct dpu_fb_data_type *dpufd);
void dpu_fb_init_spin_lock(struct dpu_fb_data_type *dpufd);
int dpu_fb_registe_callback(struct dpu_fb_data_type *dpufd);
void dpu_fb_pdp_fnc_register(struct dpu_fb_data_type *dpufd);

int bl_lvl_map(int level);

int create_isr_query_dev(void);

u64 systime_get(void);
struct dpu_fb_data_type *dpufb_get_dpufd(uint32_t fb_idx);

#endif /* DPU_FB_H */
