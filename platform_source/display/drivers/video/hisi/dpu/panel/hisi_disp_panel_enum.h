/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_DISP_PANEL_ENUM_H
#define HISI_DISP_PANEL_ENUM_H

/* lcd resource name */
#define LCD_BL_TYPE_NAME      "lcd-bl-type"
#define FPGA_FLAG_NAME        "fpga_flag"
#define LCD_DISPLAY_TYPE_NAME "lcd-display-type"
#define LCD_IFBC_TYPE_NAME    "lcd-ifbc-type"

/* backlight type */
#define BL_SET_BY_NONE BIT(0)
#define BL_SET_BY_PWM BIT(1)
#define BL_SET_BY_BLPWM BIT(2)
#define BL_SET_BY_MIPI BIT(3)
#define BL_SET_BY_SH_BLPWM BIT(4)

/* panel type list */
enum panel_type {
	PANEL_NO   =  BIT(0),      /* No Panel */
	PANEL_LCDC =  BIT(1),      /* internal LCDC type */
	PANEL_HDMI =  BIT(2),      /* HDMI TV */
	PANEL_MIPI_VIDEO = BIT(3), /* MIPI */
	PANEL_MIPI_CMD   = BIT(4), /* MIPI */
	PANEL_DUAL_MIPI_VIDEO  = BIT(5),  /* DUAL MIPI */
	PANEL_DUAL_MIPI_CMD    = BIT(6),  /* DUAL MIPI */
	PANEL_DP               = BIT(7),  /* DisplayPort */
	PANEL_MIPI2RGB         = BIT(8),  /* MIPI to RGB */
	PANEL_RGB2MIPI         = BIT(9),  /* RGB to MIPI */
	PANEL_OFFLINECOMPOSER  = BIT(10), /* offline composer */
	PANEL_WRITEBACK        = BIT(11), /* Wifi display */
	PANEL_MEDIACOMMON      = BIT(12), /* mediacommon */
};

/* blpwm precision type */
enum blpwm_precision_type {
	BLPWM_PRECISION_DEFAULT_TYPE = 0,
	BLPWM_PRECISION_10000_TYPE = 1,
	BLPWM_PRECISION_2048_TYPE = 2,
	BLPWM_PRECISION_4096_TYPE = 3,
};

/* LCD init step */
enum LCD_INIT_STEP {
	LCD_INIT_NONE = 0,
	LCD_INIT_POWER_ON,
	LCD_INIT_LDI_SEND_SEQUENCE,
	LCD_INIT_MIPI_LP_SEND_SEQUENCE,
	LCD_INIT_MIPI_HS_SEND_SEQUENCE,
};

/* LCD uninit step */
enum LCD_UNINIT_STEP {
	LCD_UNINIT_NONE = 0,
	LCD_UNINIT_POWER_OFF,
	LCD_UNINIT_LDI_SEND_SEQUENCE,
	LCD_UNINIT_MIPI_LP_SEND_SEQUENCE,
	LCD_UNINIT_MIPI_HS_SEND_SEQUENCE,
};

enum bl_control_mode {
	REG_ONLY_MODE = 1,
	PWM_ONLY_MODE,
	MUTI_THEN_RAMP_MODE,
	RAMP_THEN_MUTI_MODE,
	I2C_ONLY_MODE = 6,
	BLPWM_AND_CABC_MODE,
	COMMON_IC_MODE = 8,
	AMOLED_NO_BL_IC_MODE = 9,
	BLPWM_MODE = 10,
};

enum {
	WAIT_TYPE_US = 0,
	WAIT_TYPE_MS,
};

enum VSYNC_CTRL_TYPE {
	VSYNC_CTRL_NONE = 0x0,
	VSYNC_CTRL_ISR_OFF = BIT(0),
	VSYNC_CTRL_MIPI_ULPS = BIT(1),
	VSYNC_CTRL_CLK_OFF = BIT(2),
	VSYNC_CTRL_VCC_OFF = BIT(3),
};

enum IFBC_TYPE {
	IFBC_TYPE_NONE = 0,
	IFBC_TYPE_ORISE2X,
	IFBC_TYPE_ORISE3X,
	IFBC_TYPE_HIMAX2X,
	IFBC_TYPE_RSP2X,
	IFBC_TYPE_RSP3X,
	IFBC_TYPE_VESA2X_SINGLE,
	IFBC_TYPE_VESA3X_SINGLE,
	IFBC_TYPE_VESA2X_DUAL,
	IFBC_TYPE_VESA3X_DUAL,
	IFBC_TYPE_VESA3_75X_DUAL,
	IFBC_TYPE_VESA4X_SINGLE_SPR,
	IFBC_TYPE_VESA4X_DUAL_SPR,
	IFBC_TYPE_VESA2X_SINGLE_SPR,
	IFBC_TYPE_VESA2X_DUAL_SPR,
	IFBC_TYPE_VESA3_75X_SINGLE,

	IFBC_TYPE_MAX
};

/* dtype for vcc */
enum {
	DTYPE_VCC_GET,
	DTYPE_VCC_PUT,
	DTYPE_VCC_ENABLE,
	DTYPE_VCC_DISABLE,
	DTYPE_VCC_SET_VOLTAGE,
};

/* pinctrl operation */
enum {
	DTYPE_PINCTRL_GET,
	DTYPE_PINCTRL_STATE_GET,
	DTYPE_PINCTRL_SET,
	DTYPE_PINCTRL_PUT,
};

/* pinctrl state */
enum {
	DTYPE_PINCTRL_STATE_DEFAULT,
	DTYPE_PINCTRL_STATE_IDLE,
};

/* dtype for gpio */
enum {
	DTYPE_GPIO_REQUEST,
	DTYPE_GPIO_FREE,
	DTYPE_GPIO_INPUT,
	DTYPE_GPIO_OUTPUT,
};

#ifndef CONFIG_DKMD_DPU_V720
enum lcd_orientation {
	LCD_LANDSCAPE = 0,
	LCD_PORTRAIT,
};

enum lcd_format {
	LCD_RGB888 = 0,
	LCD_RGB101010,
	LCD_RGB565,
};

enum lcd_rgb_order {
	LCD_RGB = 0,
	LCD_BGR,
};
#endif

#endif /* HISI_DISP_PANEL_ENUM_H */
