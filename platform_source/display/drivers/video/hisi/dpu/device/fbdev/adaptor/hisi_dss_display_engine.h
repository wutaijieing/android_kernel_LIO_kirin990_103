/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2013-2020. All rights reserved.
 * Description: dss display engine interfaces
 * Author: hisilicon
 * Create: 2013-01-07
 */

#ifndef HISI_DSS_DISPLAY_ENGINE_H
#define HISI_DSS_DISPLAY_ENGINE_H

#define LCD_PANEL_VERSION_SIZE 32
#define SN_CODE_LENGTH_MAX 54
#define PANEL_NAME_LENGTH 128

enum display_engine_panel_id {
	DISPLAY_ENGINE_PANEL_MAIN = 0,
	DISPLAY_ENGINE_PANEL_FULL,
	DISPLAY_ENGINE_PANEL_NUM,
};

typedef struct display_engine_hbm_param {
	uint8_t dimming;
	uint32_t level;
	uint32_t mmi_level;
} display_engine_hbm_param_t;

typedef struct display_engine_amoled_param {
	int HBMEnable;
	int amoled_diming_enable;
	int HBM_Threshold_BackLight;
	int HBM_Threshold_Dimming;
	int HBM_Dimming_Frames;
	int HBM_Min_BackLight;
	int HBM_Max_BackLight;
	int HBM_MinLum_Regvalue;
	int HBM_MaxLum_Regvalue;
	int Hiac_DBVThres;
	int Hiac_DBV_XCCThres;
	int Hiac_DBV_XCC_MinThres;
	int Lowac_DBVThres;
	int Lowac_DBV_XCCThres;
	int Lowac_DBV_XCC_MinThres;
	int Lowac_Fixed_DBVThres;
	int dc_brightness_dimming_enable;
	int dc_brightness_dimming_enable_real;
	int dc_lowac_dbv_thre;
	int dc_lowac_fixed_dbv_thres;
	int dc_backlight_delay_us;
	int amoled_enable_from_hal;
	int dc_lowac_dbv_thres_low;
} display_engine_amoled_param_t;

typedef struct display_engine_amoled_param_sync {
	uint16_t *high_dbv_map;  /* point backlight map from level(0~10000) to dbv level */
	uint32_t map_size;
} display_engine_amoled_param_sync_t;

typedef struct display_engine_blc_param {
	uint32_t enable;
	int32_t delta;
} display_engine_blc_param_t;

typedef struct display_engine_ddic_cabc_param {
	uint32_t ddic_cabc_mode;
} display_engine_ddic_cabc_param_t;

typedef struct display_engine_ddic_color_param {
	uint32_t ddic_color_mode;
} display_engine_ddic_color_param_t;

typedef struct display_engine_ddic_rgbw_param {
	int ddic_panel_id;
	int ddic_rgbw_mode;
	int ddic_rgbw_backlight;
	int rgbw_saturation_control;
	int frame_gain_limit;
	int frame_gain_speed;
	int color_distortion_allowance;
	int pixel_gain_limit;
	int pixel_gain_speed;
	int pwm_duty_gain;
} display_engine_ddic_rgbw_param_t;

typedef struct display_engine_ddic_irc_param {
	uint32_t irc_enable;
} display_engine_ddic_irc_param_t;

typedef struct display_engine_panel_info_param {
	int width;
	int height;
	int maxluminance;
	int minluminance;
	int maxbacklight;
	int minbacklight;
	int factory_gamma_enable;
	uint16_t factory_gamma[800];//800 > 257 * 3
	char lcd_panel_version[LCD_PANEL_VERSION_SIZE];
	uint32_t sn_code_length;
	uint8_t sn_code[SN_CODE_LENGTH_MAX];
	int factory_runmode;
	char panel_name[PANEL_NAME_LENGTH];
	uint32_t board_version;
	int reserve0;
	int reserve1;
	int reserve2;
	int reserve3;
	int reserve4;
	int reserve5;
	int reserve6;
	int reserve7;
	int reserve8;
	int reserve9;
} display_engine_panel_info_param_t;

typedef struct display_engine_foldable_info {
	uint32_t dbv_acc[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t original_dbv_acc[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t screen_on_duration[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t screen_on_duration_with_hiace_enable[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t fold_num_acc;
	uint32_t hbm_acc[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
	uint32_t hbm_duration[DISPLAY_ENGINE_FOLDABLE_PANEL_NUM];
} display_engine_foldable_info_t;

typedef struct display_engine_demura {
	uint8_t *lut;
	uint32_t size;
	bool flash;
} display_engine_demura_t;

typedef struct display_engine_irdrop {
	uint32_t *params;
	uint32_t count;
} display_engine_irdrop_t;
#endif /* HISI_DSS_DISPLAY_ENGINE_H */