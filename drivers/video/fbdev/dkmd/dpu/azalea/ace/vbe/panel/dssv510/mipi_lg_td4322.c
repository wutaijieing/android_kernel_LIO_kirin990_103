/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include "../../dpu_fb.h"
#include "../../dpu_fb_panel.h"
#include "../../dpu_mipi_dsi.h"

#define DTS_COMP_LG_TD4322	"hisilicon,mipi_lg_TD4322"
// extern bool g_lcd_control_tp_power;
static int g_lcd_fpga_flag;
#define DEBUG_EXT_PANEL_ON_DSI1

/*******************************************************************************
** Display ON/OFF Sequence begin
*/
/*lint -save -e569, -e574, -e527, -e572*/
static char te_enable[] = {
	0x35,
	0x00,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

static char display_phy1[] = {
	0xB0,
	0x04,
};
static char display_phy2[] = {
	0xB6,
	0x38,
	0x93,
	0x00,
};
static char lcd_power_on_cmd1[] = {
	0xD6,
	0x01,
};

static char color_enhancement[] = {
	0x30,
	0x00,0x00,0x02,0xA7,
};

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00,0x04,0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00,0x07,0x7F
};

static struct dsi_cmd_desc set_display_address[] = {
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},
};

static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement), color_enhancement},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_on), display_on},

};

static struct dsi_cmd_desc lg_fpga_modify_phyclk_config_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_power_on_cmd1), lcd_power_on_cmd1},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_phy1), display_phy1},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_phy2), display_phy2},
};

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 52, WAIT_TYPE_MS,sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 102, WAIT_TYPE_MS,sizeof(enter_sleep), enter_sleep},
};

/******************************************************************************
*
** Display Effect Sequence(smart color, edge enhancement, smart contrast, cabc)
*/
static char pwm_out_0x51[] = {
	0x51,
	0xFE,
};

static struct dsi_cmd_desc pwm_out_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(pwm_out_0x51), pwm_out_0x51},
};


/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
};

/*******************************************************************************
** LCD IOMUX
*/
static struct pinctrl_data pctrl;

static struct pinctrl_cmd_desc lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};

/*******************************************************************************
** LCD GPIO
*/

#define GPIO_LCD_P5V5_ENABLE_NAME	"gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_BL_ENABLE_NAME	"gpio_lcd_bl_enable"
#define GPIO_LCD_TP1V8_NAME	"gpio_lcd_tp1v8"

static uint32_t gpio_lcd_p5v5_enable;
static uint32_t gpio_lcd_n5v5_enable;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_bl_enable;

static uint32_t gpio_lcd_tp1v8;

static struct gpio_desc fpga_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* TP_1.8V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
};

static struct gpio_desc fpga_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 10,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc fpga_lcd_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc fpga_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};


static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},

	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
};

static int mipi_lg_td4322_panel_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds, \
			ARRAY_SIZE(lcd_pinctrl_normal_cmds));
		gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
			ARRAY_SIZE(asic_lcd_gpio_request_cmds));
	} else {
		gpio_cmds_tx(fpga_lcd_gpio_request_cmds, \
			ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return 0;
}

static void mipi_set_backlight(struct dpu_fb_data_type *dpufd)
{
	u32 bl_level;
	char bl_level_adjust[3] = {
		0x51,
		0x00,
		0x00,
	};
	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	bl_level = 1023;

	bl_level_adjust[1] = (bl_level >> 8) & 0xff;
	bl_level_adjust[2] = bl_level & 0x00ff;
	DPU_FB_DEBUG("bl_level is %d, bl_level_adjust[1]=0x%x, bl_level_adjust[2] = 0x%x\n",
		bl_level,bl_level_adjust[1],bl_level_adjust[2]);

	mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
		ARRAY_SIZE(lcd_bl_level_adjust), dpufd->mipi_dsi1_base);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);
}

static int mipi_lg_td4322_panel_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *dpu_mipi_dsi_base = NULL;
	uint32_t status = 0;
	uint32_t try_times = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is null.\n");
		return 0;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null.\n");
		return 0;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo == NULL) {
		DPU_FB_ERR("pinfo is null.\n");
		return 0;
	}

	DPU_FB_INFO("fb%d, +.\n", dpufd->index);

	pinfo = &(dpufd->panel_info);
#ifdef DEBUG_EXT_PANEL_ON_DSI1
	dpu_mipi_dsi_base = dpufd->mipi_dsi1_base;
#else
	dpu_mipi_dsi_base = dpufd->mipi_dsi0_base;
#endif
	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		if (g_lcd_fpga_flag == 0) {
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
				ARRAY_SIZE(lcd_vcc_enable_cmds));
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));
			gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_request_cmds));
			gpio_cmds_tx(asic_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_normal_cmds));
		} else {
			gpio_cmds_tx(fpga_lcd_gpio_request_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
			gpio_cmds_tx(fpga_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_normal_cmds));
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		mipi_dsi_cmds_tx(set_display_address, \
			ARRAY_SIZE(set_display_address), dpu_mipi_dsi_base);
		mipi_dsi_cmds_tx(lcd_display_on_cmds, \
			ARRAY_SIZE(lcd_display_on_cmds), dpu_mipi_dsi_base);
		mipi_set_backlight(dpufd);
		if (g_fpga_flag == 1)
			mipi_dsi_cmds_tx(lg_fpga_modify_phyclk_config_cmds, \
				ARRAY_SIZE(lg_fpga_modify_phyclk_config_cmds), dpu_mipi_dsi_base);

		mipi_dsi_cmds_tx(pwm_out_on_cmds, \
			ARRAY_SIZE(pwm_out_on_cmds), dpu_mipi_dsi_base);

		// check lcd power state
		outp32(dpu_mipi_dsi_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
		status = inp32(dpu_mipi_dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		while (status & 0x10) {
			udelay(50);
			if (++try_times > 100) {
				DPU_FB_ERR("Read lcd power status timeout!\n");
				break;
			}

			status = inp32(dpu_mipi_dsi_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		}
		status = inp32(dpu_mipi_dsi_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
		DPU_FB_INFO("fb%d, TD4322 LCD Power State = 0x%x.\n", dpufd->index, status);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	}

	DPU_FB_INFO("fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_lg_td4322_panel_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *dpu_mipi_dsi_base = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	pinfo = &(dpufd->panel_info);
#ifdef DEBUG_EXT_PANEL_ON_DSI1
	dpu_mipi_dsi_base = dpufd->mipi_dsi1_base;
#else
	dpu_mipi_dsi_base = dpufd->mipi_dsi0_base;
#endif

	DPU_FB_INFO("fb%d, +.\n", dpufd->index);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		mipi_dsi_cmds_tx(lcd_display_off_cmds, \
			ARRAY_SIZE(lcd_display_off_cmds), dpu_mipi_dsi_base);
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (g_lcd_fpga_flag==0) {
			gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));
			gpio_cmds_tx(asic_lcd_gpio_free_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_free_cmds));
			pinctrl_cmds_tx(pdev,lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));
			mdelay(3);
			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));
		} else {
			gpio_cmds_tx(fpga_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_lowpower_cmds));
			gpio_cmds_tx(fpga_lcd_gpio_free_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_free_cmds));
		}
	} else {
		DPU_FB_ERR("failed to uninit lcd!\n");
	}

	DPU_FB_INFO("fb%d, -.\n", dpufd->index);

	return 0;
}

static int mipi_lg_td4322_panel_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
			ARRAY_SIZE(lcd_vcc_finit_cmds));
		pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
			ARRAY_SIZE(lcd_pinctrl_finit_cmds));
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return 0;
}

static int mipi_lg_td4322_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	static char last_bl_level=0;
	char __iomem *dpu_mipi_dsi_base = NULL;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

#ifdef DEBUG_EXT_PANEL_ON_DSI1
	dpu_mipi_dsi_base = dpufd->mipi_dsi1_base;
#else
	dpu_mipi_dsi_base = dpufd->mipi_dsi0_base;
#endif

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		bl_level_adjust[1] = bl_level * 255 / dpufd->panel_info.bl_max;

		if (last_bl_level != bl_level_adjust[1]) {
			last_bl_level = bl_level_adjust[1];
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), dpu_mipi_dsi_base);
		}
		DPU_FB_INFO("bl_level_adjust[1] = %d.\n",bl_level_adjust[1]);
	} else {
		DPU_FB_ERR("fb%d, not support this bl_set_type:%d!\n",
			dpufd->index, dpufd->panel_info.bl_set_type);
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

static ssize_t mipi_lg_td4322_panel_lcd_model_show(struct platform_device *pdev,
	char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	ret = snprintf(buf, PAGE_SIZE, "LG_TD4322_6P0 ' CMD TFT\n");

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

static uint32_t g_read_value[4] = {0};
static uint32_t g_expected_value[4] = {0x1c, 0x00, 0x07, 0x00};
static uint32_t g_read_mask[4] = {0xFF, 0xFF, 0x07, 0xFF};
static char* g_reg_name[4] = {"power mode", "MADCTR", "pixel format", "image mode"};
static char g_lcd_reg_0a[] = {0x0a};
static char g_lcd_reg_0b[] = {0x0b};
static char g_lcd_reg_0c[] = {0x0c};
static char g_lcd_reg_0d[] = {0x0d};

static struct dsi_cmd_desc g_lcd_check_reg[] = {
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(g_lcd_reg_0a), g_lcd_reg_0a},
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(g_lcd_reg_0b), g_lcd_reg_0b},
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(g_lcd_reg_0c), g_lcd_reg_0c},
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(g_lcd_reg_0d), g_lcd_reg_0d},
};

static struct mipi_dsi_read_compare_data g_compare_data = {
	.read_value = g_read_value,
	.expected_value = g_expected_value,
	.read_mask = g_read_mask,
	.reg_name = g_reg_name,
	.log_on = 1,
	.cmds = g_lcd_check_reg,
	.cnt = ARRAY_SIZE(g_lcd_check_reg),
};

static ssize_t mipi_lg_td4322_panel_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *dpu_mipi_dsi_base = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

#ifdef DEBUG_EXT_PANEL_ON_DSI1
	dpu_mipi_dsi_base = dpufd->mipi_dsi1_base;
#else
	dpu_mipi_dsi_base = dpufd->mipi_dsi0_base;
#endif

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (!mipi_dsi_read_compare(&g_compare_data, dpu_mipi_dsi_base)) {
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	} else {
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

/*******************************************************************************
**
*/
static struct dpu_panel_info g_panel_info = {0};
static struct dpu_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_lg_td4322_panel_set_fastboot,
	.on = mipi_lg_td4322_panel_on,
	.off = mipi_lg_td4322_panel_off,
	.remove = mipi_lg_td4322_panel_remove,
	.set_backlight = mipi_lg_td4322_panel_set_backlight,
	.lcd_model_show = mipi_lg_td4322_panel_lcd_model_show,
	.lcd_check_reg  = mipi_lg_td4322_panel_lcd_check_reg_show,
};

/*******************************************************************************
**
*/
static void mipi_lg_td4322_init_panel_info(uint32_t bl_type, uint32_t lcd_display_type)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 1920;
	pinfo->width = 75;
	pinfo->height = 133;
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
	pinfo->ifbc_type = IFBC_TYPE_NONE; // lcd_ifbc_type;
	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;

	pinfo->cinema_mode_support = 0;
	pinfo->gamma_support = 0;

	// 63fps
	pinfo->ldi.h_back_porch = 43;
	pinfo->ldi.h_front_porch = 80;
	pinfo->ldi.h_pulse_width = 20;
	pinfo->ldi.v_back_porch = 50;
	pinfo->ldi.v_front_porch = 40;
	pinfo->ldi.v_pulse_width = 40;

	pinfo->pxl_clk_rate = 151 * 1000000UL;
	// mipi
	pinfo->mipi.dsi_bit_clk = 480;
	pinfo->mipi.dsi_bit_clk_val1 = 471;
	pinfo->mipi.dsi_bit_clk_val2 = 480;
	pinfo->mipi.dsi_bit_clk_val3 = 490;
	pinfo->mipi.dsi_bit_clk_val4 = 500;
	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->gmp_support = 0;
	pinfo->hiace_support = 0;
	pinfo->acm_support = 0;
	pinfo->arsr1p_sharpness_support = 0;
	// enable arsr2p sharpness
	pinfo->prefix_sharpness2D_support = 0;

	pinfo->mipi.dsi_version = DSI_1_1_VERSION;

	// non_continue adjust : measured in UI
	// JDI requires clk_post >= 60ns + 252ui, so need a clk_post_adjust more than 200ui. Here 215 is used.
	pinfo->mipi.clk_post_adjust = 215;
	pinfo->mipi.clk_pre_adjust= 0;
	pinfo->mipi.clk_t_hs_prepare_adjust= 0;
	pinfo->mipi.clk_t_lpx_adjust= 0;
	pinfo->mipi.clk_t_hs_trial_adjust= 0;
	pinfo->mipi.clk_t_hs_exit_adjust= 0;
	pinfo->mipi.clk_t_hs_zero_adjust= 0;
	pinfo->mipi.non_continue_en = 1;

	// mipi
	pinfo->mipi.lane_nums = DSI_4_LANES;

	pinfo->mipi.color_mode = DSI_24BITS_1;

	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
	pinfo->mipi.non_continue_en = 1;

	pinfo->pxl_clk_rate_div = 1;

	pinfo->vsync_ctrl_type = 0; // VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
}

static int mipi_lg_td4322_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;

	// g_lcd_control_tp_power = false;
	DPU_FB_INFO("mipi_lg_TD4322_6P0 + \n");
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LG_TD4322);
	if (!np) {
		DPU_FB_ERR("not found device node %s!\n", DTS_COMP_LG_TD4322);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
	if (ret) {
		DPU_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}
#ifdef DSI_1_2_VESA3X_VIDEO
	lcd_display_type = PANEL_MIPI_VIDEO;
#endif

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

	bl_type = BL_SET_BY_MIPI;
	DPU_FB_INFO("bl_type=0x%x.\n", bl_type);

	if (dpu_fb_device_probe_defer(lcd_display_type, bl_type))
		goto err_probe_defer;

	DPU_FB_DEBUG("+.\n");

	/* gpio_nums = <+5v:12, -5v:34, rst:194, bl_en:11, ID0:10, ID1:9>; */
	gpio_lcd_p5v5_enable = 269; // of_get_named_gpio(np, "gpios", 0);
	gpio_lcd_n5v5_enable = 270; // of_get_named_gpio(np, "gpios", 1);
	gpio_lcd_reset = 275; // of_get_named_gpio(np, "gpios", 2);
	gpio_lcd_bl_enable = 283; // of_get_named_gpio(np, "gpios", 3);

	DPU_FB_INFO("used gpio:[+5v: %d, -5v: %d, rst: %d, bl_en: %d].\n",gpio_lcd_p5v5_enable, gpio_lcd_n5v5_enable,
		gpio_lcd_reset, gpio_lcd_bl_enable);

#ifdef DEBUG_EXT_PANEL_ON_DSI1
	pdev->id = 2;
#else
	pdev->id = 1;
#endif
	// init lcd panel info
	mipi_lg_td4322_init_panel_info(bl_type, lcd_display_type);
	if (g_lcd_fpga_flag == 0) {
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
		if (is_fastboot_display_enable())
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds, ARRAY_SIZE(lcd_vcc_enable_cmds));
	}

	// alloc panel device data
	ret = platform_device_add_data(pdev, &g_panel_data,
		sizeof(struct dpu_fb_panel_data));
	if (ret) {
		DPU_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	if (dpu_fb_add_device(pdev) == NULL)
		goto err_device_put;

	DPU_FB_DEBUG("-.\n");

	return 0;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
err_probe_defer:
	return -EPROBE_DEFER;

	return ret;
}

static const struct of_device_id dpu_panel_match_table[] = {
	{
		.compatible = DTS_COMP_LG_TD4322,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, dpu_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_lg_td4322_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_lg_td4322",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dpu_panel_match_table),
	},
};

static int __init mipi_lg_td4322_panel_init(void)
{
	int ret = 0;
	DPU_FB_INFO("+\n");

	ret = platform_driver_register(&this_driver);
	if (ret) {
		DPU_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}
/*lint -restore*/
module_init(mipi_lg_td4322_panel_init);
