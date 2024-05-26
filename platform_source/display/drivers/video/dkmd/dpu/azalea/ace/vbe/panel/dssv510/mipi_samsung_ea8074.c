/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../../dpu_fb.h"
#include "../../dpu_fb_panel.h"
#include "../../dpu_mipi_dsi.h"

#define DTS_COMP_SAMSUNG_EA8074 "hisilicon,mipi_samsung_EA8074"
static int g_lcd_fpga_flag;

/*lint -save -e569, -e574, -e527, -e572*/
/*******************************************************************************
** Power ON/OFF Sequence(sleep mode to Normal mode) begin
*/

static char display_off[] = {
	0x28,
};
static char enter_sleep[] = {
	0x10,
};

static char g_cmd0[] = {
	0x11,
	0x00,
};

static char g_cmd1[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37,
};

static char g_cmd2[] = {
	0x2B,
	0x00, 0x00, 0x08, 0xBF,
};

static char g_cmd3[] = {
	0x35,
	0x00,
};

static char g_cmd4[] = {
	0x53,
	0x20,
};

static char g_cmd5[] = {
	0x51,
	0x00, 0x00,
//	0xFF, 0x0F,

};

static char g_cmd6[] = {
	0x55,
	0x00,
};

static char g_cmd7[] = {
	0xF0,
	0x5A, 0x5A,
};

static char g_cmd8[] = {
	0xE2,
	0x01, 0x41,
};

static char g_cmd9[] = {
	0xB0,
	0x06,
};

static char g_cmd10[] = {
	0xEF,
	0x35,
};

static char g_cmd11[] = {
	0xCC,
	0x55, 0x12,
};

static char g_cmd12[] = {
	0xFC,
	0x5A, 0x5A,
};

static char g_cmd13[] = {
	0xB0,
	0x01,
};

static char g_cmd14[] = {
	0xD2,
	0x20,
};

static char g_cmd15[] = {
	0xB0,
	0x05,
};

static char g_cmd16[] = {
	0xD2,
	0x40,
};

static char g_cmd17[] = {
	0xFC,
	0xA5, 0xA5,
};

static char g_cmd18[] = {
	0xF0,
	0xA5, 0xA5,
};

static char g_cmd19[] = {
	0x2C,
	0x00,
};

static char g_cmd20[] = {
	0x29,
	0x00,
};

static char g_cmd21[] = {
	0xF0,
	0x5A, 0x5A,
};

static char g_cmd22[] = {
	0xB0,
	0x05,
};

static char g_cmd23[] = {
	0xB1,
	0x40,
};

static char g_cmd24[] = {
	0xB0,
	0x03,
};

static char g_cmd25[] = {
	0xB6,
	0xA2,
};

static char g_cmd26[] = {
	0xF0,
	0xA5, 0xA5,
};

static char g_cmd27[] = {
	0x29,
	0x00,
};


static struct dsi_cmd_desc display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_MS,
		sizeof(g_cmd0), g_cmd0},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd1), g_cmd1},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd2), g_cmd2},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd3), g_cmd3},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd4), g_cmd4},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd5), g_cmd5},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd6), g_cmd6},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd7), g_cmd7},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd8), g_cmd8},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd9), g_cmd9},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd10), g_cmd10},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd11), g_cmd11},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd12), g_cmd12},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd13), g_cmd13},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd14), g_cmd14},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd15), g_cmd15},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd16), g_cmd16},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd17), g_cmd17},
	{DTYPE_DCS_LWRITE, 0,110, WAIT_TYPE_MS,
		sizeof(g_cmd18), g_cmd18},
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd19), g_cmd19},
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd20), g_cmd20},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd21), g_cmd21},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd22), g_cmd22},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd23), g_cmd23},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd24), g_cmd24},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd25), g_cmd25},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd26), g_cmd26},
	{DTYPE_DCS_WRITE, 0,120, WAIT_TYPE_US,
			sizeof(g_cmd27), g_cmd27},
};


static struct dsi_cmd_desc lcd_display_off_cmd[] = {
	{DTYPE_DCS_WRITE, 0, 1, WAIT_TYPE_MS, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(enter_sleep), enter_sleep},
};

/*
** Power ON/OFF Sequence end
*******************************************************************************/


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
#define GPIO_AMOLED_VCC3V1_NAME	"gpio_amoled_vcc3v1"
#define GPIO_AMOLED_VCC1V8_NAME	"gpio_amoled_vcc1v8"
#define GPIO_AMOLED_RESET_NAME	"gpio_amoled_reset"
#define GPIO_AMOLED_TE0_NAME	"gpio_amoled_te0"
#define GPIO_AMOLED_VCI3V3_NAME	"gpio_amoled_vci3v3"


static uint32_t gpio_amoled_vcc3v1;
static uint32_t gpio_amoled_vcc1v8;
static uint32_t gpio_amoled_reset;
static uint32_t gpio_amoled_pcd;
static uint32_t gpio_amoled_te0;
static uint32_t gpio_amoled_err;
static uint32_t gpio_amoled_vci3v3;


static struct gpio_desc fpga_lcd_gpio_request_cmds[] = {
	/* vcc3v1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC3V1_NAME, &gpio_amoled_vcc3v1, 0},
	/* vcc1v8 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC1V8_NAME, &gpio_amoled_vcc1v8, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
			GPIO_AMOLED_VCI3V3_NAME, &gpio_amoled_vci3v3, 0},

	/* backlight enable */

	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},

};

static struct gpio_desc fpga_lcd_gpio_normal_cmds[] = {
	/* vci3v3 enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,
		GPIO_AMOLED_VCI3V3_NAME, &gpio_amoled_vci3v3, 1},
	/* vcc1v8 enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,
		GPIO_AMOLED_VCC1V8_NAME, &gpio_amoled_vcc1v8, 1},
	/* vcc3v1 enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_VCC3V1_NAME, &gpio_amoled_vcc3v1, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	/* backlight enable */
};


static struct gpio_desc lcd_gpio_off_cmds[] = {

};

static struct gpio_desc lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_VCI3V3_NAME, &gpio_amoled_vci3v3, 0},

	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},

};


static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	/* vci 3.3v */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCI3V3_NAME, &gpio_amoled_vci3v3, 0},

	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},

};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_VCI3V3_NAME, &gpio_amoled_vci3v3, 1},

	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	/* backlight enable */

};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	/* backlight enable */
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
	/* vci 3.3V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_vci3v3, 0},

};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_VCI3V3_NAME, &gpio_amoled_vci3v3, 0},

	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},

};

static int mipi_samsung_ea8074_panel_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		// lcd pinctrl normal
		pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(lcd_pinctrl_normal_cmds));
		// lcd gpio request
		gpio_cmds_tx(asic_lcd_gpio_request_cmds,
			ARRAY_SIZE(asic_lcd_gpio_request_cmds));
	} else {
		// lcd gpio request
		gpio_cmds_tx(fpga_lcd_gpio_request_cmds,
			ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static uint32_t mipi_read_ic_regs(char __iomem * mipi_dsi0_base, uint32_t rd_cmd)
{
	uint32_t status = 0;
	uint32_t try_times = 0;

	outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, rd_cmd);
	status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	while (status & 0x10) {
		udelay(50);
		if (++try_times > 100) {
			DPU_FB_ERR("Read AMOLED reg:0x%x timeout!\n",rd_cmd>>8);
			break;
		}
		status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	}
	status = inp32(mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
	DPU_FB_INFO("Read AMOLED reg:0x%x = 0x%x\n", rd_cmd>>8, status);

	return 0;
}

static void mipi_samsung_set_backlight(struct dpu_fb_data_type *dpufd)
{
	unsigned int bl_level;
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
	DPU_FB_DEBUG("bl_level is %u, bl_level_adjust[1]=0x%x, bl_level_adjust[2] = 0x%x\n",
		bl_level,bl_level_adjust[1],bl_level_adjust[2]);

	mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
		ARRAY_SIZE(lcd_bl_level_adjust), dpufd->mipi_dsi0_base);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
}

static int mipi_samsung_ea8074_panel_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;

#ifdef CONFIG_TP_ERR_DEBUG
	struct ts_kit_ops *ts_ops = NULL;
#endif
	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	pinfo = &(dpufd->panel_info);

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

#ifdef CONFIG_TP_ERR_DEBUG
	ts_ops = ts_kit_get_ops();
#endif
	pinfo = &(dpufd->panel_info);
	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		if (g_lcd_fpga_flag == 0) {
			// lcd vcc enable
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
				ARRAY_SIZE(lcd_vcc_enable_cmds));

			// lcd pinctrl normal
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));

			// lcd gpio request
			gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_request_cmds));

			// lcd gpio normal
			gpio_cmds_tx(asic_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_normal_cmds));
		} else {
			// lcd gpio request
			gpio_cmds_tx(fpga_lcd_gpio_request_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_request_cmds));

			// lcd gpio normal
			gpio_cmds_tx(fpga_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_normal_cmds));
		}

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		mdelay(10);

		// lcd display on sequence
		mipi_dsi_cmds_tx(display_on_cmds, \
			ARRAY_SIZE(display_on_cmds), mipi_dsi0_base);
		mipi_samsung_set_backlight(dpufd);
#ifdef CONFIG_TP_ERR_DEBUG
		if(ts_ops) {
			ts_ops->ts_power_notify(TS_RESUME_DEVICE, NO_SYNC);
		} else {
			DPU_FB_ERR("ts_ops is null,can't notify thp power_on\n");
		}
#endif

		mipi_read_ic_regs(mipi_dsi0_base, 0x0a06);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
#ifdef CONFIG_TP_ERR_DEBUG
		if(ts_ops)
			ts_ops->ts_power_notify(TS_AFTER_RESUME, NO_SYNC);
#endif
	} else {
		DPU_FB_ERR("failed to init lcd!\n");
	}

	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}

static int mipi_samsung_ea8074_panel_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
#ifdef CONFIG_TP_ERR_DEBUG
	struct ts_kit_ops *ts_ops = NULL;
#endif
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	pinfo = &(dpufd->panel_info);

	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
	#ifdef CONFIG_TP_ERR_DEBUG
		ts_ops = ts_kit_get_ops();
		if(ts_ops) {
			ts_ops->ts_power_notify(TS_EARLY_SUSPEND, NO_SYNC);
		} else {
			DPU_FB_ERR("ts_ops is null,can't notify thp power_off\n");
		}
	#endif
		mipi_dsi_cmds_tx(lcd_display_off_cmd, \
				ARRAY_SIZE(lcd_display_off_cmd), mipi_dsi0_base);

		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	#ifdef CONFIG_TP_ERR_DEBUG
		if(ts_ops) {
			ts_ops->ts_power_notify(TS_BEFORE_SUSPEND, NO_SYNC);
			ts_ops->ts_power_notify(TS_SUSPEND_DEVICE, NO_SYNC);
		}
	#endif
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (g_lcd_fpga_flag==0) {
			// lcd gpio lowpower
			gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));
			// lcd gpio free
			gpio_cmds_tx(asic_lcd_gpio_free_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_free_cmds));
			// lcd pinctrl lowpower
			pinctrl_cmds_tx(pdev,lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

			mdelay(3);
			// lcd vcc disable
			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));
		} else {
			// lcd gpio lowpower
			gpio_cmds_tx(lcd_gpio_off_cmds, \
				ARRAY_SIZE(lcd_gpio_off_cmds));
			// lcd gpio free
			gpio_cmds_tx(lcd_gpio_free_cmds, \
				ARRAY_SIZE(lcd_gpio_free_cmds));
		}
	} else {
		DPU_FB_ERR("failed to uninit lcd!\n");
	}

	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return 0;
}

static int mipi_samsung_ea8074_panel_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		// lcd vcc finit
		vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
			ARRAY_SIZE(lcd_vcc_finit_cmds));

		// lcd pinctrl finit
		pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
			ARRAY_SIZE(lcd_pinctrl_finit_cmds));
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);
	return 0;
}

static int mipi_samsung_ea8074_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	static uint32_t last_bl_level = 0;

	char bl_level_adjust[3] = {
		0x51,
		0x00,
		0x00,
	};
	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	if (bl_level > dpufd->panel_info.bl_max)
		bl_level = dpufd->panel_info.bl_max;

	if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		bl_level_adjust[1] = (bl_level >> 8) & 0xff;
		bl_level_adjust[2] = bl_level & 0x00ff;
		DPU_FB_DEBUG("bl_level is %d, bl_level_adjust[1]=0x%x, bl_level_adjust[2] = 0x%x\n",
			bl_level,bl_level_adjust[1],bl_level_adjust[2]);

		if (last_bl_level != bl_level){
			last_bl_level = bl_level;
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), dpufd->mipi_dsi0_base);
		}
	} else {
		DPU_FB_ERR("fb%d, not support this bl_set_type %d!\n",
			dpufd->index, dpufd->panel_info.bl_set_type);
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);
	return ret;
}

static ssize_t mipi_samsung_ea8074_lcd_model_show(struct platform_device *pdev,
	char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is null\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return(!dpufd, -1, ERR, "dpufd is null\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	ret = snprintf(buf, PAGE_SIZE, "samsung_EA8074 CMD AMOLED PANEL\n");

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

static struct dpu_panel_info g_panel_info = {0};
static struct dpu_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_samsung_ea8074_panel_set_fastboot,
	.on = mipi_samsung_ea8074_panel_on,
	.off = mipi_samsung_ea8074_panel_off,
	.remove = mipi_samsung_ea8074_panel_remove,
	.set_backlight = mipi_samsung_ea8074_panel_set_backlight,
	.lcd_model_show = mipi_samsung_ea8074_lcd_model_show,
};

static int lcd_voltage_relevant_init(struct platform_device *pdev)
{
	int ret = 0;

	if (g_lcd_fpga_flag == 0) {
		ret = vcc_cmds_tx(pdev, lcd_vcc_init_cmds,
			ARRAY_SIZE(lcd_vcc_init_cmds));
		if (ret != 0) {
			DPU_FB_ERR("LCD vcc init failed!\n");
			return ret;
		}

		ret = pinctrl_cmds_tx(pdev, lcd_pinctrl_init_cmds,
			ARRAY_SIZE(lcd_pinctrl_init_cmds));
		if (ret != 0) {
			DPU_FB_ERR("Init pinctrl failed, defer\n");
			return ret;
		}

		if (is_fastboot_display_enable())
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds, ARRAY_SIZE(lcd_vcc_enable_cmds));
	}

	return ret;
}

static void mipi_samsung_ea8074_init_panel_info(uint32_t bl_type, uint32_t lcd_display_type)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 2240;
	pinfo->width = 67;
	pinfo->height = 139;
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
	pinfo->bl_min = 12;
	pinfo->bl_max = 1023;
	pinfo->bl_default = 410;
#endif
	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;

	pinfo->current_mode = MODE_8BIT;
	pinfo->type = lcd_display_type;

	if (g_lcd_fpga_flag == 1) {
		pinfo->ldi.h_back_porch = 23;
		pinfo->ldi.h_front_porch = 50;
		pinfo->ldi.h_pulse_width = 20;
		pinfo->ldi.v_back_porch = 12;
		pinfo->ldi.v_front_porch = 14;
		pinfo->ldi.v_pulse_width = 4;

		pinfo->mipi.dsi_bit_clk = 120;
		pinfo->mipi.dsi_bit_clk_val1 = 110;
		pinfo->mipi.dsi_bit_clk_val2 = 130;
		pinfo->mipi.dsi_bit_clk_val3 = 140;
		pinfo->mipi.dsi_bit_clk_val4 = 150;
		pinfo->dsi_bit_clk_upt_support = 0;
		pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
		pinfo->mipi.non_continue_en = 0;

		pinfo->pxl_clk_rate = 20 * 1000000UL;
	} else {
		if (is_mipi_cmd_panel_ext(pinfo)) {
			pinfo->ldi.h_back_porch = 20;
			pinfo->ldi.h_front_porch = 30;
			pinfo->ldi.h_pulse_width = 10;
			pinfo->ldi.v_back_porch = 36;
			pinfo->ldi.v_front_porch = 20;
			pinfo->ldi.v_pulse_width = 30;

			pinfo->pxl_clk_rate = 166 * 1000000UL;
		} else {
			pinfo->ldi.h_back_porch = 96;
			pinfo->ldi.h_front_porch = 108;
			pinfo->ldi.h_pulse_width = 48;
			pinfo->ldi.v_back_porch = 12;
			pinfo->ldi.v_front_porch = 14;
			pinfo->ldi.v_pulse_width = 4;

			pinfo->pxl_clk_rate = 288 * 1000000;
		}

		pinfo->mipi.dsi_bit_clk = 500;
		pinfo->mipi.dsi_bit_clk_val1 = 471;
		pinfo->mipi.dsi_bit_clk_val2 = 480;
		pinfo->mipi.dsi_bit_clk_val3 = 490;
		pinfo->mipi.dsi_bit_clk_val4 = 500;
		pinfo->dsi_bit_clk_upt_support = 1;
		pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
		pinfo->mipi.non_continue_en = 0;
	}

	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
	pinfo->mipi.phy_mode = DPHY_MODE;
	pinfo->mipi.dsi_version = DSI_1_1_VERSION;
	pinfo->mipi.non_continue_en = 1;

	pinfo->ifbc_type = IFBC_TYPE_NONE;
	pinfo->pxl_clk_rate_div = 1;

	if (pinfo->type == PANEL_MIPI_CMD)
		pinfo->vsync_ctrl_type = 0; // VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
}

static int mipi_samsung_ea8074_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;

	DPU_FB_INFO("+\n");
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SAMSUNG_EA8074);
	if (!np) {
		DPU_FB_ERR("not found device node %s!\n", DTS_COMP_SAMSUNG_EA8074);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
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
	DPU_FB_INFO("bl_type=0x%x.\n", bl_type);

	if (dpu_fb_device_probe_defer(lcd_display_type, bl_type))
		goto err_probe_defer;

	DPU_FB_DEBUG("+.\n");

	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcd_fpga_flag);//lint !e64
	if (ret)
		DPU_FB_WARNING("need to get g_lcd_fpga_flag resource in fpga, not needed in asic!\n");

	DPU_FB_INFO("g_lcd_fpga_flag=%d.\n", g_lcd_fpga_flag);

	if (g_lcd_fpga_flag == 1) {
		gpio_amoled_reset = of_get_named_gpio(np, "gpios", 0);
		gpio_amoled_te0 = of_get_named_gpio(np, "gpios", 2);
		gpio_amoled_vci3v3 = of_get_named_gpio(np, "gpios", 4);
	} else {
		gpio_amoled_reset = 280; // of_get_named_gpio(np, "gpios", 0);
		gpio_amoled_pcd = 281; // of_get_named_gpio(np, "gpios", 1);
		gpio_amoled_te0 = 3; // of_get_named_gpio(np, "gpios", 2);
		gpio_amoled_err = 160; // of_get_named_gpio(np, "gpios", 3);
		gpio_amoled_vci3v3 = 256; // of_get_named_gpio(np, "gpios", 4);
	}
	DPU_FB_INFO("gpio_amoled_reset= %d, gpio_amoled_pcd= %d, gpio_amoled_te0= %d, "
		"gpio_amoled_err= %d, gpio_amoled_vci3v3= %d \n",
		gpio_amoled_reset, gpio_amoled_pcd, gpio_amoled_te0, gpio_amoled_err, gpio_amoled_vci3v3);

	pdev->id = 1;
	// init lcd panel info
	mipi_samsung_ea8074_init_panel_info(bl_type, lcd_display_type);

	ret = lcd_voltage_relevant_init(pdev);
	if (ret != 0)
		goto err_return;

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
}

static const struct of_device_id dpu_panel_match_table[] = {
	{
		.compatible = DTS_COMP_SAMSUNG_EA8074,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, dpu_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_samsung_ea8074_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_samsung_EA8074",
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

/*lint -restore*/
module_init(mipi_lcd_panel_init);
