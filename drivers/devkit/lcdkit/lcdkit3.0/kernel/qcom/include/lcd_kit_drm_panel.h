/*
 * lcd_kit_drm_panel.h
 *
 * lcdkit display function head file for lcd driver
 *
 * Copyright (c) 2020-2022 Huawei Technologies Co., Ltd.
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

#ifndef __LCD_KIT_DRM_PANEL_H_
#define __LCD_KIT_DRM_PANEL_H_
#include <linux/backlight.h>
#ifndef CONFIG_QCOM_PLATFORM_8425
#include <drm/drmP.h>
#endif
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "lcd_kit_common.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_core.h"
#include "lcd_kit_utils.h"
#include <dsm/dsm_pub.h>

/* macro definition */
#define DTS_COMP_LCD_KIT_PANEL_TYPE "huawei,lcd_panel_type"
#define LCD_KIT_PANEL_COMP_LENGTH 128
#define BACKLIGHT_HIGH_LEVEL 1
#define BACKLIGHT_LOW_LEVEL  2
#define REC_DMD_NO_LIMIT      (-1)
#define DMD_RECORD_BUF_LEN    100
#define RECOVERY_TIMES          3
// elvdd detect type
#define ELVDD_MIPI_CHECK_MODE   1
#define ELVDD_GPIO_CHECK_MODE   2
#define FIRST_FRAME             1
#define MAX_COUNT_FRAME         10

struct lcd_kit_disp_info *lcd_kit_get_disp_info(uint32_t panel_id);
#define disp_info	lcd_kit_get_disp_info(panel_id)
unsigned int lcm_get_panel_state(uint32_t panel_id);
void lcm_set_panel_state(uint32_t panel_id, unsigned int state);
int lcd_kit_drm_notifier_register(uint32_t panel_id, struct notifier_block *nb);
int lcd_kit_drm_notifier_unregister(uint32_t panel_id, struct notifier_block *nb);

/* enum */
enum alpm_mode {
	ALPM_DISPLAY_OFF,
	ALPM_ON_HIGH_LIGHT,
	ALPM_EXIT,
	ALPM_ON_LOW_LIGHT,
	ALPM_SINGLE_CLOCK,
	ALPM_DOUBLE_CLOCK,
	ALPM_ON_MIDDLE_LIGHT,
	ALPM_NO_LIGHT,
};

/* pcd detect */
enum pcd_check_status {
	PCD_CHECK_WAIT,
	PCD_CHECK_ON,
	PCD_CHECK_OFF,
};

enum finger_unlock_status {
	OFF_FINGER_UNLOCK,
	ON_FINGER_UNLOCK,
};

struct elvdd_detect {
	u32 support;
	u32 delay;
	u32 exp_value_mask;
	u32 exp_value;
	u32 detect_gpio;
	u32 detect_type;
	bool is_start_delay_timer;
	struct lcd_kit_dsi_panel_cmds cmds;
};

struct poweric_detect_delay {
	struct hrtimer timer;
	struct work_struct wq;
};

struct lcd_kit_disp_info {
	/* effect */
	/* gamma calibration */
	struct lcd_kit_gamma gamma_cal;
	/* oem information */
	struct lcd_kit_oem_info oeminfo;
	/* rgbw function */
	struct lcd_kit_rgbw rgbw;
	/* end */
	/* normal */
	/* lcd type */
	u32 lcd_type;
	/* panel information */
	char *compatible;
	/* product id */
	u32 product_id;
	/* vr support */
	u32 vr_support;
	/* lcd kit semaphore */
	struct semaphore lcd_kit_sem;
	/* lcd kit blank semaphore */
	struct semaphore blank_sem;
	/* lcd kit mipi mutex lock */
	struct mutex mipi_lock;
	/* lcd kit power mode lock */
	struct mutex power_mode_lock;
	/* alpm -aod */
	struct lcd_kit_alpm alpm;
	/* quickly sleep out */
	struct lcd_kit_quickly_sleep_out quickly_sleep_out;
	/* fps ctrl */
	struct lcd_kit_fps fps;
	/* project id */
	struct lcd_kit_project_id project_id;
	/* thp_second_poweroff_sem */
	struct semaphore thp_second_poweroff_sem;
	struct display_engine_ddic_rgbw_param ddic_rgbw_param;
	/* end */
	/* elvdd detect */
	struct elvdd_detect elvdd_detect;
	/* normal */
	/* pcd errflag */
	struct lcd_kit_pcd_errflag pcd_errflag;
	u8 bl_is_shield_backlight;
	u8 bl_is_start_timer;
	ktime_t alpm_start_time;
	struct mutex drm_hw_lock;
	u8 alpm_state;
	u32 boost_5v_enable;
};

extern struct completion aod_lp1_done;
extern struct completion lcd_panel_init_done[];
extern struct completion first_frame_done;
struct qcom_panel_info *lcm_get_panel_info(uint32_t panel_id);
int lcd_kit_init(struct dsi_panel *panel);
void lcd_kit_power_init(uint32_t panel_id, struct dsi_panel *panel);
int lcd_kit_sysfs_init(void);
unsigned int lcm_get_panel_backlight_max_level(void);
#if defined CONFIG_HUAWEI_DSM
struct dsm_client *lcd_kit_get_lcd_dsm_client(void);
#endif
int is_mipi_cmd_panel(uint32_t panel_id);
int lcd_kit_rgbw_set_handle(uint32_t panel_id);
int panel_drm_hbm_set(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int lcm_display_engine_init(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int lcm_display_engine_get_panel_info(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int lcm_rgbw_mode_get_param(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int lcm_rgbw_mode_set_param(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
void lcd_kit_disp_on_check_delay(void);
int panel_drm_hbm_set_for_fingerprint(bool is_enable);
int lcd_kit_panel_pre_prepare(struct dsi_panel *panel);
int lcd_kit_panel_prepare(struct dsi_panel *panel);
int lcd_kit_panel_enable(struct dsi_panel *panel);
void lcd_kit_panel_post_enable(struct dsi_display *display);
int lcd_kit_panel_pre_disable(struct dsi_panel *panel);
int lcd_kit_panel_disable(struct dsi_panel *panel);
int lcd_kit_panel_unprepare(struct dsi_panel *panel);
int lcd_kit_panel_post_unprepare(struct dsi_panel *panel);
int lcd_kit_bl_ic_set_backlight(unsigned int bl_level);
int lcd_kit_dsi_panel_update_backlight(struct dsi_panel *panel,
	unsigned int bl_lvl);
uint32_t lcd_get_active_panel_id(void);
uint32_t lcd_kit_get_current_panel_id(struct dsi_panel *panel);
void lcd_kit_set_pinctrl_status(const char *pinctrl_status);
void lcd_kit_pinctrl_put(void);
int lcd_kit_panel_set_ddic_cmd(struct dsi_panel *panel, enum dsi_cmd_set_type type);
void lcd_kit_esd_recover_bl(struct dsi_panel *panel, u32 *bl_lvl);
void lcd_kit_esd_recover_bl_after_frame(void);
void lcd_kit_record_frame_number(void);
void lcd_kit_esd_recovery_enable(struct dsi_panel *panel);
void lcd_kit_esd_backlight_enable(struct dsi_panel *panel);
void lcd_kit_get_esd_brightness(struct dsi_panel *panel, int *brightness);
void lcd_kit_msg_set(struct mipi_dsi_msg *msg);
void lcd_kit_crtc_info_store(struct drm_plane *plane);
void lcd_kit_fold_lp_handle(void);
void lcd_kit_set_fold_info(void);
uint32_t lcd_kit_get_current_brightness(uint32_t panel_id);
#endif
