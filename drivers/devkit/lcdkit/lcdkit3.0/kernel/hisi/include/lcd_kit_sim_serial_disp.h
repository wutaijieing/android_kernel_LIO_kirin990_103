/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef LCD_KIT_SIM_SERIAL_PANEL_H
#define LCD_KIT_SIM_SERIAL_PANEL_H

#include "lcd_kit_ext_disp.h"
#include "hisi_fb.h"
#include "lcd_kit_common.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_dbg.h"
#include <huawei_platform/log/log_jank.h>
#include "global_ddr_map.h"
#include "lcd_kit_utils.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_sysfs.h"
#ifdef LV_GET_LCDBK_ON
#include <huawei_platform/inputhub/sensor_feima_ext.h>
#endif
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>

#define MAX_SIM_SERIAL_PANEL 2
#define MAX_LOGICAL_PANEL 2
#define FB_INDEX_MAX 5
#define LCD_CB_BYPASS 1
#define LCD_ALPM_SETTING_DEFAULT 2
#define LCD_SIM_ALPM_ON 1
#define BL_MAX_LEVEL 255
#define BL_CALI_DATA_LEN 256

extern unsigned int get_pd_charge_flag(void);

enum {
	LCD_KIT_FUNC_SET_FASTBOOT,
	LCD_KIT_FUNC_ON,
	LCD_KIT_FUNC_OFF,
	LCD_KIT_FUNC_SET_BACKLIGHT,
	LCD_KIT_FUNC_MAX,
};

struct callback_data {
	uint32_t func_id;
	void *data;
};

struct lcd_callback {
	int (*func)(struct platform_device *pdev, struct callback_data *data);
	struct list_head list;
};

struct lcd_physical_setting {
	struct list_head preproc_callback_head[LCD_KIT_FUNC_MAX];
	struct list_head postproc_callback_head[LCD_KIT_FUNC_MAX];
#if defined(CONFIG_DPU_FB_AP_AOD)
	uint32_t alpm_mode;
#endif
	uint8_t *backlight_cali_map;
};

int lcd_kit_regist_preproc(uint32_t panel_id, uint32_t func_id,
	struct lcd_callback *cb);
int lcd_kit_regist_postproc(uint32_t panel_id, uint32_t func_id,
	struct lcd_callback *cb);
int lcd_kit_sim_serial_panel_init(void);
uint32_t lcd_kit_get_alpm_setting_status(void);

#endif
