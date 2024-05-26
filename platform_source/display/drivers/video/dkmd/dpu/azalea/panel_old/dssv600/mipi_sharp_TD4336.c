/* Copyright (c) 2016-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
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

 /*lint -e551 -e551*/

#include "dpu_fb.h"
#ifdef CONFIG_HUAWEI_TS
#include <huawei_platform/touthscreen/huawei_touchscreen.h>
#endif

#define DTS_COMP_SHARP_TD4336 "hisilicon,mipi_sharp_TD4336"
static int g_lcdFpgaFlag;

static char g_cmD0[] = {
	0x35,
	0x00,
};

static char g_cmD1[] = {
	0x36,
	0x00,
};

static char g_cmD2[] = {
	0x3A,
	0x77,
};

static char g_cmD3[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37,
};

static char g_cmD4[] = {
	0x2B,
	0x00, 0x00, 0x08, 0xC3,
};

static char g_cmD5[] = {
	0x50,
	0xFF,
};

static char g_cmD6[] = {
	0x61,
	0x00, 0x06,
};

static char g_cmD7[] = {
	0x63,
	0x24,
};

static char g_cmD8[] = {
	0x65,
	0x02,
};

static char g_cmD9[] = {
	0x6A,
	0x00,
};

static char g_cmD10[] = {
	0x6C,
	0xFF,
};

static char g_cmD11[] = {
	0x6E,
	0x00,
};

static char g_cmD12[] = {
	0x51,
	0xFF, 0xF0,
};

static char g_cmD13[] = {
	0x53,
	0x2C,

};

static char g_cmD14[] = {
	0x55,
	0x00,
};


static char g_cmD15[] = {
	0x5E,
	0x00, 0x00,
};

static char g_cmD16[] = {
	0xB0,
	0x04,
};

static char g_cmD161[] = {
	0xD6,
	0x00,
};

static char g_cmD162[] = {
	0xE7,
	0x50, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,
};

static char g_cmD17[] = {
	0xD6,
	0x00,
};

static char g_cmD18[] = {
	0xE7,
	0x50, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,
};

static char g_cmD19[] = {
	0xF0,
	0xDF, 0x0F, 0xFF,
};

static char g_cmD20[] = {
	0xF0,
	0xDF, 0x10, 0xFF,
};

static char g_cmD21[] = {
	0xF0,
	0xDF, 0x11, 0xFF,
};

static char g_cmD22[] = {
	0xF0,
	0xDF, 0x12, 0xFF,
};

static char g_cmD23[] = {
	0xF0,
	0xDF, 0x13, 0xFF,
};

static char g_cmD24[] = {
	0x29,
	0x00,
};
static char g_cmD25[] = {
	0x11,
};
static char g_cmD26[] = {
	0x29,
};

static struct dsi_cmd_desc g_displayOnCmds[] = {
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD0), g_cmD0},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD1), g_cmD1},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD2), g_cmD2},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD3), g_cmD3},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD4), g_cmD4},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD5), g_cmD5},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD6), g_cmD6},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD7), g_cmD7},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD8), g_cmD8},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD9), g_cmD9},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD10), g_cmD10},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD11), g_cmD11},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD12), g_cmD12},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD13), g_cmD13},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD14), g_cmD14},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD15), g_cmD15},
	{0x29, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD16), g_cmD16},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD161), g_cmD161},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD162), g_cmD162},
	{0x15, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD17), g_cmD17},
	{0x39, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD18), g_cmD18},
	{0x29, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD19), g_cmD19},
	{0x29, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD20), g_cmD20},
	{0x29, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD21), g_cmD21},
	{0x29, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD22), g_cmD22},
	{0x29, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD23), g_cmD23},
	{0x05, 0, 10, WAIT_TYPE_US,
		sizeof(g_cmD24), g_cmD24},
	{0x05, 0, 0x78, WAIT_TYPE_US,
		sizeof(g_cmD25), g_cmD25},
	{0x05, 0, 0x4B, WAIT_TYPE_MS,
		sizeof(g_cmD26), g_cmD26},
};

/* Power OFF Sequence(Normal to power off) */
static char g_displayOff[] = {
	0x28,
};

static char g_enterSleep[] = {
	0x10,
};

static struct dsi_cmd_desc g_lcdDisplayOffCmds[] = {
	{DTYPE_DCS_WRITE, 0, 60, WAIT_TYPE_MS,
		sizeof(g_displayOff), g_displayOff},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(g_enterSleep), g_enterSleep}
};


/* LCD VCC */
#define VCC_LCDIO_NAME  "lcdio-vcc"
#define VCC_LCDDIG_NAME "lcddig-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcddig;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},

	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 10},
};

static struct vcc_desc lcd_vcc_init_dig_cmds[] = {
	/* vci1v2 get */
	{DTYPE_VCC_GET, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0},

	/* vci1v2 set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDDIG_NAME, &vcc_lcddig, 1200000, 1200000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_dig_cmds[] = {
	/* vci1v2 put */
	{DTYPE_VCC_PUT, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_dig_cmds[] = {
	/* vci1v2 enable */
	{DTYPE_VCC_ENABLE, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 10},
};

/* LCD IOMUX */
static struct pinctrl_data g_pctrl;

static struct pinctrl_cmd_desc g_lcdPinctrlInitCmds[] = {
	{DTYPE_PINCTRL_GET, &g_pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcdPinctrlNormalCmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc g_lcdPinctrlLowpowerCmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcdPinctrlFinitCmds[] = {
	{DTYPE_PINCTRL_PUT, &g_pctrl, 0},
};

/* LCD GPIO */
#define GPIO_LCD_P5V5_ENABLE_NAME	"gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_BL_ENABLE_NAME	"gpio_lcd_bl_enable"
#define GPIO_TP_RESET_NAME	"gpio_tp_reset"

static uint32_t g_gpioLcdP5v5Enable;
static uint32_t g_gpioLcdN5v5Enable;
static uint32_t g_gpioLcdReset;
static uint32_t g_gpioLcdBlEnable;
static uint32_t g_gpioTpReset;

static struct gpio_desc g_asicLcdGpioRequestCmds[] = {
	/* tp reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_TP_RESET_NAME, &g_gpioTpReset, 0},
	/* lcd reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 0},
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_gpioLcdP5v5Enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_gpioLcdN5v5Enable, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &g_gpioLcdBlEnable, 0},
};

static struct gpio_desc g_asicLcdGpioFreeCmds[] = {
	/* tp reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_TP_RESET_NAME, &g_gpioTpReset, 0},
	/* lcd reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 0},
	/* AVDD_5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_gpioLcdP5v5Enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_gpioLcdN5v5Enable, 0},
	/* backlight enable */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_BL_ENABLE_NAME, &g_gpioLcdBlEnable, 0},
};

static struct gpio_desc g_asicLcdGpioNormalCmds[] = {
	/* tp reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_TP_RESET_NAME, &g_gpioTpReset, 1},

	/* lcd reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 1},

	/* AVDD_5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_gpioLcdP5v5Enable, 1},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_gpioLcdN5v5Enable, 1},

	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &g_gpioLcdBlEnable, 1},
};

static struct gpio_desc g_asicLcdGpioLowpowerCmds[] = {
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &g_gpioLcdBlEnable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_gpioLcdN5v5Enable, 0},
	/* AVDD_5.5V*/
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_gpioLcdP5v5Enable, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 0},

	/* backlight enable input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_BL_ENABLE_NAME, &g_gpioLcdBlEnable, 0},
	/* AVEE_-5.5V input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_gpioLcdN5v5Enable, 0},
	/* AVDD_5.5V input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_gpioLcdP5v5Enable, 0},
	/* reset input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &g_gpioLcdReset, 0},
};

static int mipi_sharp_TD4366_panel_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	pinctrl_cmds_tx(pdev, g_lcdPinctrlNormalCmds,
		ARRAY_SIZE(g_lcdPinctrlNormalCmds));

	gpio_cmds_tx(g_asicLcdGpioRequestCmds,
		ARRAY_SIZE(g_asicLcdGpioRequestCmds));

	// backlight on
	dpu_lcd_backlight_on(pdev);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static int mipi_sharp_TD4366_panel_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi1_base = NULL;
	uint32_t status = 0;
	uint32_t try_times = 0;

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	if (!pinfo) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_INFO("External LCD:fb%d, +\n", dpufd->index);

	pinfo = &(dpufd->panel_info);
	mipi_dsi1_base = dpufd->mipi_dsi1_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
			ARRAY_SIZE(lcd_vcc_enable_cmds));
		pinctrl_cmds_tx(pdev, g_lcdPinctrlNormalCmds,
			ARRAY_SIZE(g_lcdPinctrlNormalCmds));

		gpio_cmds_tx(g_asicLcdGpioRequestCmds,
			ARRAY_SIZE(g_asicLcdGpioRequestCmds));

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		gpio_cmds_tx(g_asicLcdGpioNormalCmds,
			ARRAY_SIZE(g_asicLcdGpioNormalCmds));
		mdelay(20);
		vcc_cmds_tx(pdev, lcd_vcc_enable_dig_cmds,
			ARRAY_SIZE(lcd_vcc_enable_dig_cmds));
		mdelay(20);

		mipi_dsi_cmds_tx(g_displayOnCmds,
			ARRAY_SIZE(g_displayOnCmds), mipi_dsi1_base);

		// check lcd power state
		outp32(mipi_dsi1_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
		status = inp32(mipi_dsi1_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		while (status & 0x10) {
			udelay(50);
			if (++try_times > 100) {
				DPU_FB_ERR("Read lcd power status timeout!\n");
				break;
			}

			status = inp32(mipi_dsi1_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		}
		status = inp32(mipi_dsi1_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
		DPU_FB_INFO("External LCD Power State = 0x%x\n", status);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		;
	} else {
		DPU_FB_ERR("failed to init lcd!\n");
	}

	DPU_FB_INFO("External LCD: fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_sharp_TD4366_panel_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi1_base = NULL;

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);
	mipi_dsi1_base = dpufd->mipi_dsi1_base;

	DPU_FB_INFO("External LCD: fb%d, +!\n", dpufd->index);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		dpu_lcd_backlight_off(pdev);
		mipi_dsi_cmds_tx(g_lcdDisplayOffCmds, ARRAY_SIZE(g_lcdDisplayOffCmds), mipi_dsi1_base);
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		gpio_cmds_tx(g_asicLcdGpioLowpowerCmds,
			ARRAY_SIZE(g_asicLcdGpioLowpowerCmds));

		gpio_cmds_tx(g_asicLcdGpioFreeCmds,
			ARRAY_SIZE(g_asicLcdGpioFreeCmds));

		pinctrl_cmds_tx(pdev, g_lcdPinctrlLowpowerCmds,
			ARRAY_SIZE(g_lcdPinctrlLowpowerCmds));

		mdelay(3);
	} else {
		DPU_FB_ERR("failed to uninit lcd!\n");
	}

	DPU_FB_INFO("External LCD: fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_sharp_TD4366_panel_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev ) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	// lcd vcc finit
	vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
		ARRAY_SIZE(lcd_vcc_finit_cmds));

	vcc_cmds_tx(pdev, lcd_vcc_finit_dig_cmds,
		ARRAY_SIZE(lcd_vcc_finit_dig_cmds));

	// lcd pinctrl finit
	pinctrl_cmds_tx(pdev, g_lcdPinctrlFinitCmds,
		ARRAY_SIZE(g_lcdPinctrlFinitCmds));

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static int mipi_sharp_TD4366_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	static char last_bl_level;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (dpufd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		ret = dpu_pwm_set_backlight(dpufd, bl_level);
	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {
		ret = dpu_blpwm_set_backlight(dpufd, bl_level);
	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {
		ret = dpu_sh_blpwm_set_backlight(dpufd, bl_level);
	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		bl_level_adjust[1] = bl_level * 255 / dpufd->panel_info.bl_max;
		if (last_bl_level != bl_level_adjust[1]) {
			last_bl_level = bl_level_adjust[1];
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, ARRAY_SIZE(lcd_bl_level_adjust), dpufd->mipi_dsi1_base);
		}
	} else {
		DPU_FB_ERR("fb%d, not support this bl_set_type(%d)!\n",
			dpufd->index, dpufd->panel_info.bl_set_type);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;

}

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00, 0x07, 0x7F
};

static struct dsi_cmd_desc set_display_address[] = {
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},
};

static int mipi_sharp_TD4366_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if ((!pdev) || (!dirty)) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	/*lint -e574 */
	if (((dirty->x % 2) != 0) || ((dirty->y % 2) != 0) ||
		((dirty->w % 2) != 0) || ((dirty->h % 2) != 0) ||
		(dirty->x >= pinfo->xres) || (dirty->w > pinfo->xres) || ((dirty->x + dirty->w) > pinfo->xres) ||
		(dirty->y >= pinfo->yres) || (dirty->h > pinfo->yres) || ((dirty->y + dirty->h) > pinfo->yres)) {
		DPU_FB_ERR("dirty_region(%d,%d, %d,%d) not support!\n",
			dirty->x, dirty->y, dirty->w, dirty->h);
	}
	/*lint +e574 */

	lcd_disp_x[1] = ((uint32_t)dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = (uint32_t)dirty->x & 0xff;
	lcd_disp_x[3] = (((uint32_t)dirty->x + (uint32_t)dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = ((uint32_t)dirty->x + (uint32_t)dirty->w - 1) & 0xff;
	lcd_disp_y[1] = ((uint32_t)dirty->y >> 8) & 0xff;
	lcd_disp_y[2] = (uint32_t)dirty->y & 0xff;
	lcd_disp_y[3] = (((uint32_t)dirty->y + (uint32_t)dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = ((uint32_t)dirty->y + (uint32_t)dirty->h - 1) & 0xff;

	mipi_dsi_cmds_tx(set_display_address, ARRAY_SIZE(set_display_address), dpufd->mipi_dsi1_base);

	return 0;
}

static ssize_t mipi_sharp_TD4366_panel_lcd_model_show(struct platform_device *pdev,
	char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	if (!pdev || !buf) {
		DPU_FB_ERR("pdev or buf is NULL\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	ret = snprintf(buf, PAGE_SIZE, "sharp_TD4366_NT35695_cut3_1 6' g_cmD-VIDEO TFT External LCD\n");

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static ssize_t mipi_sharp_TD4366_panel_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *mipi_dsi1_base = NULL;
	uint32_t read_value[4] = {0};
	uint32_t expected_value[4] = {0x1c, 0x00, 0x77, 0x00};
	uint32_t read_mask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	char *reg_name[4] = {"power mode", "MADCTR", "pixel format", "image mode"};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0b[] = {0x0b};
	char lcd_reg_0c[] = {0x0c};
	char lcd_reg_0d[] = {0x0d};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0c), lcd_reg_0c},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d},
	};

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = 1,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	if (!pdev || !buf) {
		DPU_FB_ERR("pdev or buf is NULL\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	mipi_dsi1_base = dpufd->mipi_dsi1_base;

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (!mipi_dsi_read_compare(&data, mipi_dsi1_base))
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static struct dpu_panel_info g_panelInfo = {0};
static struct dpu_fb_panel_data g_panelData = {
	.panel_info = &g_panelInfo,
	.set_fastboot = mipi_sharp_TD4366_panel_set_fastboot,
	.on = mipi_sharp_TD4366_panel_on,
	.off = mipi_sharp_TD4366_panel_off,
	.remove = mipi_sharp_TD4366_panel_remove,
	.set_backlight = mipi_sharp_TD4366_panel_set_backlight,
	.set_display_region = mipi_sharp_TD4366_panel_set_display_region,

	.lcd_model_show = mipi_sharp_TD4366_panel_lcd_model_show,
	.lcd_check_reg  = mipi_sharp_TD4366_panel_lcd_check_reg_show,
};

static int lcd_voltage_relevant_init(struct platform_device *pdev)
{
	int ret = 0;

	if (g_lcdFpgaFlag == 0) {
		ret = vcc_cmds_tx(pdev, lcd_vcc_init_cmds,
			ARRAY_SIZE(lcd_vcc_init_cmds));
		if (ret != 0) {
			DPU_FB_ERR("LCD vcc init failed!\n");
			return ret;
		}

		ret = vcc_cmds_tx(pdev, lcd_vcc_init_dig_cmds,
			ARRAY_SIZE(lcd_vcc_init_dig_cmds));
		if (ret != 0) {
			DPU_FB_ERR("LCD vcc init dig failed!\n");
			return ret;
		}

		ret = pinctrl_cmds_tx(pdev, g_lcdPinctrlInitCmds,
			ARRAY_SIZE(g_lcdPinctrlInitCmds));
		if (ret != 0) {
			DPU_FB_ERR("Init pinctrl failed, defer\n");
			return ret;
		}
	}
	return ret;
}

static void mipi_lcd_get_gpio(struct device_node *np)
{
	g_gpioLcdP5v5Enable = of_get_named_gpio(np, "gpios", 0);
	g_gpioLcdN5v5Enable = of_get_named_gpio(np, "gpios", 1);
	g_gpioLcdReset = of_get_named_gpio(np, "gpios", 2);
	g_gpioTpReset = of_get_named_gpio(np, "gpios", 3);
	g_gpioLcdBlEnable = of_get_named_gpio(np, "gpios", 4);

	DPU_FB_INFO("vsp=%d, vsn=%d, lcd_rst=%d, bl_enable=%d, tp_rst=%d!\n",
		g_gpioLcdP5v5Enable, g_gpioLcdN5v5Enable, g_gpioLcdReset, g_gpioLcdBlEnable, g_gpioTpReset);
}

static void mipi_lcd_get_info_from_dts(struct device_node *np, uint32_t *bl_type,
	uint32_t *lcd_display_type, uint32_t *lcd_ifbc_type)
{
	int ret;

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, lcd_display_type);
	if (ret) {
		DPU_FB_ERR("get lcd_display_type failed!\n");
		*lcd_display_type = PANEL_MIPI_VIDEO;
	}

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME, lcd_ifbc_type);
	if (ret) {
		DPU_FB_ERR("get ifbc_type failed!\n");
		*lcd_ifbc_type = IFBC_TYPE_NONE;
	}

	ret = of_property_read_u32(np, "pipe-switch-connector", &g_dsi_pipe_switch_connector);
	if (ret) {
		DPU_FB_ERR("get dsi_pipe_switch_connector failed!\n");
		g_dsi_pipe_switch_connector = PIPE_SWITCH_CONNECT_DSI1;
	}
	DPU_FB_INFO("g_dsi_pipe_switch_connector : %d\n", g_dsi_pipe_switch_connector);

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, bl_type);
	if (ret) {
		DPU_FB_ERR("get lcd_bl_type failed!\n");
		*bl_type = BL_SET_BY_MIPI;
	}
	DPU_FB_INFO("lcd_display_type=0x%x lcd_ifbc_type=0x%x bl_type=0x%x\n",
		*lcd_display_type, *lcd_ifbc_type, *bl_type);

	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcdFpgaFlag);  // lint !e64
	if (ret)
		DPU_FB_WARNING("need to get g_lcdFpgaFlag resource in fpga, not needed in asic!\n");
	DPU_FB_INFO("g_lcdFpgaFlag=%d\n", g_lcdFpgaFlag);

	mipi_lcd_get_gpio(np);
	DPU_FB_DEBUG("-\n");
}

static void mipi_lcd_init_panel_info(struct dpu_panel_info *pinfo,
	uint32_t bl_type, uint32_t lcd_display_type)
{
	pinfo->xres = 1080;
	pinfo->yres = 2244;
	pinfo->width = 74;
	pinfo->height = 149;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;

#ifdef CONFIG_BACKLIGHT_10000
	pinfo->bl_min = 157;
	pinfo->bl_max = 9960;
	pinfo->bl_default = 4000;
	pinfo->blpwm_precision_type = BLPWM_PRECISION_10000_TYPE;
#else
	pinfo->bl_min = 1;
	pinfo->bl_max = 255;
	pinfo->bl_default = 102;
#endif

	pinfo->type = lcd_display_type;
	pinfo->ifbc_type = IFBC_TYPE_NONE;
	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;

	pinfo->vsync_ctrl_type = 0;
	pinfo->pxl_clk_rate_div = 1;
}

static void mipi_lcd_init_timing(struct dpu_panel_info *pinfo)
{
	// parameters supported by ic spec
	// 63fps
	pinfo->ldi.h_back_porch = 16;
	pinfo->ldi.h_front_porch = 220;
	pinfo->ldi.h_pulse_width = 8;
	pinfo->ldi.v_back_porch = 36;
	pinfo->ldi.v_front_porch = 36;
	pinfo->ldi.v_pulse_width = 30;

	pinfo->pxl_clk_rate = 166 * 1000000UL;
	// mipi
	pinfo->mipi.dsi_bit_clk = 500;
	pinfo->mipi.dsi_bit_clk_val1 = 471;
	pinfo->mipi.dsi_bit_clk_val2 = 480;
	pinfo->mipi.dsi_bit_clk_val3 = 490;
	pinfo->mipi.dsi_bit_clk_val4 = 500;
	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->mipi.dsi_version = DSI_1_1_VERSION;

	// non_continue adjust : measured in UI
	// sharp_TD4366 requires clk_post >= 60ns + 252ui, so need a clk_post_adjust more than 200ui. Here 215 is used.
	pinfo->mipi.clk_post_adjust = 215;
	pinfo->mipi.clk_pre_adjust = 0;
	pinfo->mipi.clk_t_hs_prepare_adjust = 0;
	pinfo->mipi.clk_t_lpx_adjust = 0;
	pinfo->mipi.clk_t_hs_trial_adjust = 0;
	pinfo->mipi.clk_t_hs_exit_adjust = 0;
	pinfo->mipi.clk_t_hs_zero_adjust = 0;
	pinfo->mipi.non_continue_en = 1;

	// mipi
	pinfo->mipi.lane_nums = DSI_4_LANES;

	pinfo->mipi.color_mode = DSI_24BITS_1;

	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
}

static int mipi_sharp_TD4366_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;

	DPU_FB_INFO("External LCD (sharp_TD4366_NT35695_cut3_1) probe +\n");

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SHARP_TD4336);
	if (!np) {
		DPU_FB_ERR("not found device node %s!\n", DTS_COMP_SHARP_TD4336);
		return 0;
	}

	mipi_lcd_get_info_from_dts(np, &bl_type, &lcd_display_type, &lcd_ifbc_type);

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return 0;
	}

	pdev->id = 1;
	// init lcd panel info
	pinfo = g_panelData.panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));

	mipi_lcd_init_panel_info(pinfo, bl_type, lcd_display_type);

	mipi_lcd_init_timing(pinfo);

	ret = lcd_voltage_relevant_init(pdev);
	if (ret != 0)
		return ret;

	// alloc panel device data
	ret = platform_device_add_data(pdev, &g_panelData,
		sizeof(struct dpu_fb_panel_data));
	if (ret) {
		DPU_FB_ERR("platform_device_add_data failed!\n");
		platform_device_put(pdev);
	}

	dpu_fb_add_device(pdev);

	DPU_FB_DEBUG("-\n");

	return 0;
}

static const struct of_device_id g_dpuPanelMatchTable[] = {
	{
		.compatible = DTS_COMP_SHARP_TD4336,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, g_dpuPanelMatchTable);

static struct platform_driver g_thisDriver = {
	.probe = mipi_sharp_TD4366_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_sharp_TD4366",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_dpuPanelMatchTable),
	},
};

static int __init mipi_lcd_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&g_thisDriver);
	if (ret) {
		DPU_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_lcd_panel_init);
/* lint +e569 +e574 +e551 */
