/*
 *Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "dpu_fb.h"
#define DTS_COMP_BOE_RM69330 "hisilicon,mipi_boe_RM69330"
#define DTS_BOE_RM69330_LCD_TYPE "boe-rm69330-lcd-type"

#if defined(CONFIG_LCDKIT_DRIVER) || defined(CONFIG_LCD_KIT_DRIVER)
extern volatile int g_tskit_ic_type;
	// this type means oncell incell tddi ...
	// in order to decide the power policy between lcd & tp
#endif

#define LCD_POWER_STATUS_CHECK (0)

static char edo_poweron_param1[] = {
	0xfe,
	0x07,
};

static char edo_poweron_param2[] = {
	0x15,
	0x04,
};

static char edo_poweron_param3[] = {
	0xfe,
	0x01,
};

static char edo_poweron_param4[] = {
	0x8d,
	0x83,
};

static char edo_poweron_param5[] = {
	0x8e,
	0x04,
};

static char edo_poweron_param6[] = {
	0x66,
	0x10,
};

static char edo_poweron_param7[] = {
	0xfe,
	0x00,
};

static char edo_poweron_param8[] = {
	0x35,
	0x00,
};

static char edo_poweron_param9[] = {
	0x3a,
	0x77,
};

static char edo_poweron_param10[] = {
	0x51,
	0xff,
};

static char edo_poweron_param11[] = {
	0x2a,
	0x00, 0x0e, 0x01, 0xd3,
};

static char edo_poweron_param12[] = {
	0x2b,
	0x00, 0x00, 0x01, 0xc5,
};

static char edo_poweron_param13[] = {
	0x11,
};

static char edo_poweron_param14[] = {
	0x29,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static char boe_poweron_param1[] = {
	0xfe,
	0x01,
};

static char boe_poweron_param2[] = {
	0x18,
	0x55,
};

static char boe_poweron_param3[] = {
	0xc5,
	0x10,
};

static char boe_poweron_param4[] = {
	0x70,
	0xa8,
};

static char boe_poweron_param5[] = {
	0x72,
	0x0e,
};

static char boe_poweron_param6[] = {
	0x73,
	0x1c,
};

static char boe_poweron_param7[] = {
	0xfe,
	0x07,
};

static char boe_poweron_param8[] = {
	0x15,
	0x04,
};

static char boe_poweron_param9[] = {
	0xfe,
	0x00,
};

static char boe_poweron_param10[] = {
	0x35,
	0x00,
};

static char boe_poweron_param11[] = {
	0x53,
	0x20,
};

static char boe_poweron_param12[] = {
	0x3a,
	0x77,
};

static char boe_poweron_param13[] = {
	0x2a,
	0x00, 0x0c, 0x01, 0xd1,
};

static char boe_poweron_param14[] = {
	0x2b,
	0x00, 0x00, 0x01, 0xc5,
};

static char boe_poweron_param15[] = {
	0x11,
};

static char boe_poweron_param16[] = {
	0x29,
};

static struct dsi_cmd_desc lcd_display_on_cmds_boe[] = {
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param1), boe_poweron_param1 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param2), boe_poweron_param2 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param3), boe_poweron_param3 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param4), boe_poweron_param4 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param5), boe_poweron_param5 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param6), boe_poweron_param6 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param7), boe_poweron_param7 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param8), boe_poweron_param8 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param9), boe_poweron_param9 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param10), boe_poweron_param10 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param11), boe_poweron_param11 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param12), boe_poweron_param12 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param13), boe_poweron_param13 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param14), boe_poweron_param14 },
	{ DTYPE_DCS_WRITE, 0, 50, WAIT_TYPE_MS,
		sizeof(boe_poweron_param15), boe_poweron_param15 },
	{ DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_US,
		sizeof(boe_poweron_param16), boe_poweron_param16 },

	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep },
	{ DTYPE_DCS_WRITE, 0, 20, WAIT_TYPE_MS,
		sizeof(display_on), display_on },
};


/*******************************************************************************
 * Power OFF Sequence(Normal to power off)
 */

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{ DTYPE_DCS_WRITE, 0, 60, WAIT_TYPE_MS,
		sizeof(display_off), display_off },
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(enter_sleep), enter_sleep },
};

/*******************************************************************************
 * LCD VCC
 */
#define VCC_LCDIO_NAME "lcdio-vcc"

static struct regulator *vcc_lcdio;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{ DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
	/* io set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio,
		1800000, 1800000, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{ DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{ DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio,
		0, 0, WAIT_TYPE_MS, 10 },
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{ DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio,
		0, 0, WAIT_TYPE_MS, 3 },
};


/*******************************************************************************
 * LCD IOMUX
 */
static struct pinctrl_data pctrl;

static struct pinctrl_cmd_desc lcd_pinctrl_init_cmds[] = {
	{ DTYPE_PINCTRL_GET, &pctrl, 0 },
	{ DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT },
	{ DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_IDLE },
};

static struct pinctrl_cmd_desc lcd_pinctrl_normal_cmds[] = {
	{ DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT },
};

static struct pinctrl_cmd_desc lcd_pinctrl_lowpower_cmds[] = {
	{ DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_IDLE },
};

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{ DTYPE_PINCTRL_PUT, &pctrl, 0 },
};


/*******************************************************************************
 * LCD GPIO
 */
#define GPIO_LCD_RESET_NAME "gpio_lcd_reset"
#define GPIO_LCD_VCI_CTRL_NAME "gpio_lcd_vci_ctrl"
#define GPIO_LCD_1V8_ENABLE_NAME "gpio_lcd_1v8en"
#define GPIO_LCD_TE "gpio_lcd_te"

static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_vci_ctrl;
static uint32_t gpio_lcd_1v8en;

static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	/* 1v8en */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_1V8_ENABLE_NAME, &gpio_lcd_1v8en, 0 },
	/* vci ctrl */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_VCI_CTRL_NAME, &gpio_lcd_vci_ctrl, 0 },
	/* reset */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0 },
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
	/* vci ctrl */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_VCI_CTRL_NAME, &gpio_lcd_vci_ctrl, 0 },
	/* 1v8en */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_1V8_ENABLE_NAME, &gpio_lcd_1v8en, 1 },
	/* reset */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0 },
};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
	/* 1v8en */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_1V8_ENABLE_NAME, &gpio_lcd_1v8en, 1 },
	/* vci ctrl */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_VCI_CTRL_NAME, &gpio_lcd_vci_ctrl, 1 },
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1 },
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	/* 1v8en */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_1V8_ENABLE_NAME, &gpio_lcd_1v8en, 0 },
	/* vci ctrl */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VCI_CTRL_NAME, &gpio_lcd_vci_ctrl, 0 },
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0 },
};

static int mipi_boe_panel_set_backlight(struct platform_device *pdev,
	uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	char payload[2] = {0, 0};
	struct dsi_cmd_desc bl_cmd[] = {
		{ DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(payload), payload },
	};
	if (!pdev) {
		DPU_FB_INFO("pdev is NULL\n");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_INFO("dpufd is NULL\n");
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
		bl_cmd[0].payload[0] = 0x51;
		bl_cmd[0].payload[1] = bl_level * 255 /
					dpufd->panel_info.bl_max;

		mipi_dsi_cmds_tx(bl_cmd, ARRAY_SIZE(bl_cmd),
			dpufd->mipi_dsi0_base);
	} else {
		DPU_FB_ERR("fb%d, not support this bl_set_type = %d!\n",
			dpufd->index, dpufd->panel_info.bl_set_type);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static int mipi_boe_panel_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_INFO("pdev is NULL!\n");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_INFO("dpufd is NULL!\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	// lcd pinctrl normal
	pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
		ARRAY_SIZE(lcd_pinctrl_normal_cmds));
	// lcd gpio request
	gpio_cmds_tx(asic_lcd_gpio_request_cmds,
		ARRAY_SIZE(asic_lcd_gpio_request_cmds));

	// backlight on
	dpu_lcd_backlight_on(pdev);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static int mipi_boe_panel_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t status = 0;
	uint32_t try_times = 0;

	if (!pdev) {
		DPU_FB_INFO("pdev is NULL!\n");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_INFO("dpufd is NULL!\n");
		return -EINVAL;
	}
	DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);

	pinfo = &(dpufd->panel_info);
	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		// lcd vcc enable
		vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
			ARRAY_SIZE(lcd_vcc_enable_cmds));

		udelay(100); // wait pll clk
		set_reg(dpufd->pctrl_base + PERI_CTRL33, 0x0, 1, 13);
		if (is_dual_mipi_panel(dpufd))
			set_reg(dpufd->pctrl_base + PERI_CTRL30, 0x0, 1, 29);

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		DPU_FB_INFO("LCD_INIT_MIPI_LP_SEND_SEQUENCE\n");
		// lcd pinctrl normal
		pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(lcd_pinctrl_normal_cmds));

		// lcd gpio request
		gpio_cmds_tx(asic_lcd_gpio_request_cmds,
			ARRAY_SIZE(asic_lcd_gpio_request_cmds));

		// lcd gpio normal
		gpio_cmds_tx(asic_lcd_gpio_normal_cmds,
			ARRAY_SIZE(asic_lcd_gpio_normal_cmds));

		DPU_FB_INFO("lcd display on sequence\n");
		// lcd display on sequence
		mipi_dsi_cmds_tx(lcd_display_on_cmds_boe,
			ARRAY_SIZE(lcd_display_on_cmds_boe), mipi_dsi0_base);
		udelay(1000);

		// check lcd power state
		outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
		status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		while (status & 0x10) {
			udelay(50);
			if (++try_times > 100) {
				DPU_FB_ERR("Read lcd power status timeout!\n");
				break;
			}

			status = inp32(mipi_dsi0_base +
					MIPIDSI_CMD_PKT_STATUS_OFFSET);
		}
		status = inp32(mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
		DPU_FB_INFO("LCD Power State = 0x%x\n", status);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		;
	} else {
		DPU_FB_ERR("failed to init lcd!\n");
	}

	/* backlight on */
	dpu_lcd_backlight_on(pdev);

	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_boe_panel_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_INFO("pdev is NULL!\n");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!pdev) {
		DPU_FB_INFO("dpufd is NULL!\n");
		return -EINVAL;
	}
	DPU_FB_INFO("fb%d, +!\n", dpufd->index);

	// backlight off
	dpu_lcd_backlight_off(pdev);

	// lcd display off sequence
	mipi_dsi_cmds_tx(lcd_display_off_cmds,
		ARRAY_SIZE(lcd_display_off_cmds), dpufd->mipi_dsi0_base);

	/* switch to cmd mode */
	set_reg(dpufd->mipi_dsi0_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(dpufd->mipi_dsi0_base +
		MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(dpufd->mipi_dsi0_base +
		MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(dpufd->mipi_dsi0_base +
		MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);

	/* disable generate High Speed clock */
	set_reg(dpufd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
	udelay(10);

	// lcd gpio lowpower
	gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds,
		ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));
	// lcd gpio free
	gpio_cmds_tx(asic_lcd_gpio_free_cmds,
		ARRAY_SIZE(asic_lcd_gpio_free_cmds));

	// lcd pinctrl lowpower
	pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
		ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

	mdelay(3);
	// lcd vcc disable
	vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
		ARRAY_SIZE(lcd_vcc_disable_cmds));
	DPU_FB_INFO("fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_boe_panel_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_INFO("pdev is NULL\n");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);


	// lcd vcc finit
	vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
		ARRAY_SIZE(lcd_vcc_finit_cmds));

	// lcd pinctrl finit
	pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
		ARRAY_SIZE(lcd_pinctrl_finit_cmds));

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}



/******************************************************************************/
static ssize_t mipi_boe_panel_lcd_model_show(struct platform_device *pdev,
	char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	if (!pdev || !buf) {
		DPU_FB_ERR("pdev or buf is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	ret = snprintf(buf, PAGE_SIZE,
		"boe_rm69330 5P93 VIDEO TFT 454 x 454\n");

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static ssize_t mipi_boe_panel_lcd_cabc_mode_show(struct platform_device *pdev,
	char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static ssize_t mipi_boe_panel_lcd_cabc_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static ssize_t mipi_boe_panel_lcd_check_reg_show
	(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[4] = {0};
	uint32_t expected_value[4] = {0x9c, 0x00, 0x07, 0x00};
	uint32_t read_mask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	char *reg_name[4] = {"power mode", "MADCTR",
				"pixel format", "image mode"};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0b[] = {0x0b};
	char lcd_reg_0c[] = {0x0c};
	char lcd_reg_0d[] = {0x0d};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a },
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b },
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0c), lcd_reg_0c },
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d },
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
		DPU_FB_ERR("pdev or buf is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base))
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00, 0x08, 0x6F
};

static struct dsi_cmd_desc set_display_address[] = {
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y },
};

static int mipi_boe_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (!pdev || !dirty) {
		DPU_FB_ERR("pdev or dirty is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (((dirty->x % 2) != 0) || ((dirty->y % 2) != 0) ||
		((dirty->w % 2) != 0) || ((dirty->h % 2) != 0) ||
		(dirty->x >= pinfo->xres) || (dirty->w > pinfo->xres) ||
		((dirty->x + dirty->w) > pinfo->xres)
		|| (dirty->y >= pinfo->yres) || (dirty->h > pinfo->yres) ||
		((dirty->y + dirty->h) > pinfo->yres)) {
		DPU_FB_ERR("dirty_region[%d,%d, %d,%d] not support!\n",
			dirty->x, dirty->y, dirty->w, dirty->h);

		return -EINVAL;
	}

	if (dpufd->panel_info.ifbc_type != IFBC_TYPE_NONE)
		dirty->y /= 2;

	lcd_disp_x[1] = ((uint32_t)dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = (uint32_t)dirty->x & 0xff;
	lcd_disp_x[3] = ((uint32_t)(dirty->x + dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = (uint32_t)(dirty->x + dirty->w - 1) & 0xff;
	lcd_disp_y[1] = ((uint32_t)dirty->y >> 8) & 0xff;
	lcd_disp_y[2] = (uint32_t)dirty->y & 0xff;
	lcd_disp_y[3] = ((uint32_t)(dirty->y + dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = (uint32_t)(dirty->y + dirty->h - 1) & 0xff;

	mipi_dsi_cmds_tx(set_display_address,
		ARRAY_SIZE(set_display_address), dpufd->mipi_dsi0_base);

	return 0;
}

static struct dpu_panel_info g_panel_info = {0};
static struct dpu_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_boe_panel_set_fastboot,
	.on = mipi_boe_panel_on,
	.off = mipi_boe_panel_off,
	.remove = mipi_boe_panel_remove,
	.set_backlight = mipi_boe_panel_set_backlight,
	.lcd_model_show = mipi_boe_panel_lcd_model_show,
	.lcd_cabc_mode_show = mipi_boe_panel_lcd_cabc_mode_show,
	.lcd_cabc_mode_store = mipi_boe_panel_lcd_cabc_mode_store,
	.lcd_check_reg = mipi_boe_panel_lcd_check_reg_show,
	.set_display_region = mipi_boe_panel_set_display_region,
};


/*******************************************************************************
 *
 */
static int mipi_boe_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;
	const char *lcd_bl_ic_name;
	char lcd_bl_ic_name_buf[LCD_BL_IC_NAME_MAX];
	int value = 0;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_BOE_RM69330);
	if (!np) {
		DPU_FB_ERR("NOT FOUND device node %s!\n",
			DTS_COMP_BOE_RM69330);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME,
		&lcd_display_type);
	if (ret) {
		DPU_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME, &lcd_ifbc_type);
	if (ret) {
		DPU_FB_ERR("get ifbc_type failed!\n");
		lcd_ifbc_type = IFBC_TYPE_NONE;
	}

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &bl_type);
	if (ret) {
		DPU_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	DPU_FB_INFO("bl_type=0x%x", bl_type);

#if defined(CONFIG_LCDKIT_DRIVER) || defined(CONFIG_LCD_KIT_DRIVER)
	ret = of_property_read_u32(np, DTS_BOE_RM69330_LCD_TYPE, &value);
	if (ret)
		DPU_FB_ERR("get g_tskit_ic_type failed!\n");
	else
		g_tskit_ic_type = value;

	DPU_FB_INFO("g_tskit_ic_type = 0x%x", g_tskit_ic_type);
#endif

	if (dpu_fb_device_probe_defer(lcd_display_type, bl_type))
		goto err_probe_defer;

	DPU_FB_DEBUG("+\n");

	DPU_FB_INFO("%s\n", DTS_COMP_BOE_RM69330);

	ret = of_property_read_string_index(np, "lcd-bl-ic-name",
		0, &lcd_bl_ic_name);
	if (ret != 0)
		memcpy(lcd_bl_ic_name_buf, "INVALID", strlen("INVALID") + 1);
	else
		memcpy(lcd_bl_ic_name_buf, lcd_bl_ic_name,
			strlen(lcd_bl_ic_name) + 1);

	// gpio_23_5
	gpio_lcd_vci_ctrl = of_get_named_gpio(np, "gpios", 0);
	// gpio_4_7
	gpio_lcd_1v8en = of_get_named_gpio(np, "gpios", 1);
	// gpio_1_6
	gpio_lcd_reset = of_get_named_gpio(np, "gpios", 2);

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return 0;
	}

	pdev->id = 1;
	// init lcd panel info
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));
	pinfo->xres = 454;
	pinfo->yres = 454;
	pinfo->width = 35;
	pinfo->height = 35;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;

	if (pinfo->bl_set_type == BL_SET_BY_BLPWM)
		pinfo->blpwm_input_ena = 0;

	pinfo->bl_min = 1;
	pinfo->bl_max = 255;
	pinfo->bl_default = 102;

	pinfo->type = lcd_display_type;
	pinfo->ifbc_type = lcd_ifbc_type;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;

	pinfo->arsr1p_sharpness_support = 0;
	pinfo->prefix_ce_support = 0;
	pinfo->prefix_sharpness1D_support = 0;
	pinfo->prefix_sharpness2D_support = 0;

	// ldi
	// 60fps
	pinfo->ldi.h_back_porch = 60;
	pinfo->ldi.h_front_porch = 60;
	pinfo->ldi.h_pulse_width = 15;
	pinfo->ldi.v_back_porch = 50;
	pinfo->ldi.v_front_porch = 50;
	pinfo->ldi.v_pulse_width = 4;

	// mipi
	pinfo->mipi.dsi_bit_clk = 192;
	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->non_check_ldi_porch = 1;
	pinfo->mipi.non_continue_en = 0;
	pinfo->mipi.lp11_flag = 0;

	pinfo->pxl_clk_rate = 20 * 1000000UL;

	// mipi
	pinfo->mipi.clk_post_adjust = 0;
	pinfo->mipi.lane_nums = DSI_1_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 8 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
	pinfo->mipi.txoff_rxulps_en = 1;

	pinfo->pxl_clk_rate_div = 1;
	pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
	pinfo->dirty_region_updt_support = 0;

	// lcd vcc init
	ret = vcc_cmds_tx(pdev, lcd_vcc_init_cmds,
		ARRAY_SIZE(lcd_vcc_init_cmds));
	if (ret != 0) {
		DPU_FB_ERR("LCD vcc init failed!\n");
		goto err_return;
	}

	// lcd pinctrl init
	ret = pinctrl_cmds_tx(pdev, lcd_pinctrl_init_cmds,
		ARRAY_SIZE(lcd_pinctrl_init_cmds));
	if (ret != 0) {
		DPU_FB_ERR("Init pinctrl failed, defer\n");
		goto err_return;
	}

	// lcd vcc enable
	if (is_fastboot_display_enable()) {
		vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
			ARRAY_SIZE(lcd_vcc_enable_cmds));
	}

	// alloc panel device data
	ret = platform_device_add_data(pdev, &g_panel_data,
		sizeof(struct dpu_fb_panel_data));
	if (ret) {
		DPU_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	dpu_fb_add_device(pdev);

	DPU_FB_DEBUG("-\n");

	return 0;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
err_probe_defer:
	return -EPROBE_DEFER;
}

static const struct of_device_id dpu_panel_match_table[] = {
	{
		.compatible = DTS_COMP_BOE_RM69330,
		.data = NULL,
	},
};
MODULE_DEVICE_TABLE(of, dpu_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_boe_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_boe_RM69330",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dpu_panel_match_table),
	},
};

static int __init mipi_lcd_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		DPU_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_lcd_panel_init);
