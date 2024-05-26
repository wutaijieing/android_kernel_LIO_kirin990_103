/*
 * lcd_kit_displayengine.h
 *
 * diplay engine common function head in lcd
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
 *
 *
 */

#ifndef __LCD_KIT_DISPLAYENGINE_H_
#define __LCD_KIT_DISPLAYENGINE_H_

#include <drm_device.h>
#include <drm_file.h>
#include <drm/msm_drm.h>
#include "lcd_kit_common.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_core.h"
#include "displayengine_fingerprint_utils.h"

#define ALPHA_DEFAULT 10000

int display_engine_set_fp_backlight(uint32_t panel_id, void *hld, u32 level, u32 backlight_type);
void display_engine_disable_elvss_dim_write(uint32_t panel_id);
void display_engine_enable_elvss_dim_write(uint32_t panel_id);
int display_engine_fp_hbm_extern(uint32_t panel_id, void *hld, int type);
int display_engine_restore_hbm_level(uint32_t panel_id, void *hld);


/* IO interfaces: */
int display_engine_get_param(struct drm_device *dev, void *data, struct drm_file *file_priv);
int display_engine_set_param(struct drm_device *dev, void *data, struct drm_file *file_priv);

/* Feature functions: */
bool display_engine_hist_is_enable(void);
bool display_engine_hist_is_enable_change(void);
int display_engine_fp_backlight_set_sync(uint32_t panel_id, u32 level, u32 backlight_type);
void display_engine_init(uint32_t panel_id);
void display_engine_brightness_set_alpha_bypass(bool is_bypass);
int display_engine_brightness_get_mapped_level(int in_level, uint32_t panel_id);
int display_engine_brightness_get_mapped_alpha(void);
/* Handle vblank interrupt */
void display_engine_brightness_handle_vblank(ktime_t te_timestamp);
void display_engine_brightness_handle_mode_change(void);
/* True: fingerprint hbm on. False: fingerprint hbm off */
bool display_engine_brightness_is_fingerprint_hbm_enabled(void);
uint32_t display_engine_get_hbm_level(uint32_t panel_id);
uint32_t display_engine_brightness_get_mipi_level(void);
uint32_t display_engine_brightness_get_mode_in_use(void);
bool display_engine_brightness_should_do_fingerprint(void);
void display_engine_hbm_gamma_read(uint32_t panel_id, void *hld);
void display_engine_handle_power_mode_enter(int mode);
void display_engine_handle_power_mode_exit(int mode);
void display_engine_compensation_set_dbv(uint32_t in_level, uint32_t mipi_level, uint32_t panel_id);
void display_engine_compensation_set_fps(uint32_t panel_id);
void display_engine_panel_on(uint32_t panel_id);

#endif
