 /* Copyright (c) 2016-2019, Tech. Co., Ltd. All rights reserved.
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

 /*lint -e551 -e551*/
#include "dpu_fb.h"
#include "../mipi_lcd_utils.h"
#include "../../dsc/dsc_algorithm_manager.h"
#include "mipi_sharp_NT36870.h"
#include "dpu_fb_defconfig.h"

#ifdef CONFIG_LCD_KIT_DRIVER
extern int tps65132_dbg_set_bias_for_dpu(int vpos, int vneg);
#endif

#ifdef CONFIG_LCD_KIT_DRIVER
#include "lcd_kit_core.h"
#define CONFIG_TP_ERR_DEBUG
struct ts_kit_ops *ts_ops = NULL;
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

static int mipi_sharp_nt36870_panel_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return((pdev == NULL), 0, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return((dpufd == NULL), 0, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		// lcd pinctrl normal
		pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(lcd_pinctrl_normal_cmds));
		// lcd gpio request
		if (dpu_is_running_kernel_slt()) {
			gpio_cmds_tx(asic_lcd_gpio_cancle_vspn_request_cmds,
				ARRAY_SIZE(asic_lcd_gpio_cancle_vspn_request_cmds));
		} else {
			gpio_cmds_tx(asic_lcd_gpio_request_cmds,
				ARRAY_SIZE(asic_lcd_gpio_request_cmds));
		}
	} else {
		// lcd gpio request
		gpio_cmds_tx(fpga_lcd_gpio_request_cmds,
			ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
	}

	// backlight on
	dpu_lcd_backlight_on(pdev);

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return 0;
}

void mipi_sharp_nt36870_mode_switch(struct dpu_fb_data_type *dpufd, uint8_t mode_switch_to)
{
	struct dpu_panel_info *pinfo = NULL;

	if (NULL == dpufd) {
		DPU_FB_ERR("NULL dpufd .\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	if (NULL == pinfo) {
		DPU_FB_ERR("NULL pinfo .\n");
		return;
	}

	if (!pinfo->panel_mode_swtich_support) {
		DPU_FB_INFO("not srpport .\n");
		return;
	}

	DPU_FB_DEBUG("+ .\n");
	if (mode_switch_to == MODE_10BIT_VIDEO_3X) {
		mipi_dsi_cmds_tx(lcd_display_init_10bit_video_3x_dsc_cmds, \
			ARRAY_SIZE(lcd_display_init_10bit_video_3x_dsc_cmds), dpufd->mipi_dsi0_base);
	} else if (mode_switch_to == MODE_8BIT) {
		mipi_dsi_cmds_tx(lcd_display_init_8bit_cmd_3x_dsc_cmds, \
			ARRAY_SIZE(lcd_display_init_8bit_cmd_3x_dsc_cmds), dpufd->mipi_dsi0_base);
	}

	DPU_FB_DEBUG("- .\n");
	return;
}
static int mipi_sharp_nt36870_panel_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t status = 0;
	uint32_t try_times = 0;

	dpu_check_and_return((pdev == NULL), 0, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return((dpufd == NULL), 0, ERR, "dpufd is NULL\n");
	pinfo = &(dpufd->panel_info);
	dpu_check_and_return((pinfo == NULL), 0, ERR, "pinfo is NULL\n");

	DPU_FB_INFO("fb%d, +.\n", dpufd->index);

#ifdef CONFIG_TP_ERR_DEBUG
	ts_ops = ts_kit_get_ops();
#endif

	pinfo = &(dpufd->panel_info);
	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		// check lcd power state
		if (g_lcd_fpga_flag == 0) {
			// lcd vcc enable
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
				ARRAY_SIZE(lcd_vcc_enable_cmds));
			// lcd pinctrl normal
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));
			if (dpu_is_running_kernel_slt()) {
				// lcd gpio request
				gpio_cmds_tx(asic_lcd_gpio_cancle_vspn_request_cmds,
					ARRAY_SIZE(asic_lcd_gpio_cancle_vspn_request_cmds));

				// lcd gpio normal
				gpio_cmds_tx(asic_lcd_gpio_cancle_vspn_normal_cmds_sub1, \
					ARRAY_SIZE(asic_lcd_gpio_cancle_vspn_normal_cmds_sub1));
			} else {
				// lcd gpio request
				gpio_cmds_tx(asic_lcd_gpio_request_cmds,
					ARRAY_SIZE(asic_lcd_gpio_request_cmds));

				// lcd gpio normal
				gpio_cmds_tx(asic_lcd_gpio_normal_cmds_sub1, \
					ARRAY_SIZE(asic_lcd_gpio_normal_cmds_sub1));
			}
		} else {
			gpio_cmds_tx(fpga_lcd_gpio_request_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
			gpio_cmds_tx(fpga_lcd_gpio_on_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_on_cmds));
		}
	#ifdef CONFIG_LCD_KIT_DRIVER
		tps65132_dbg_set_bias_for_dpu(5800000, 5800000);
	#endif
		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;

	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (g_lcd_fpga_flag == 0)
			gpio_cmds_tx(asic_lcd_gpio_normal_cmds_sub2, \
				ARRAY_SIZE(asic_lcd_gpio_normal_cmds_sub2));

		mipi_dsi_cmds_tx(lcd_display_init_cmds_part1, \
			ARRAY_SIZE(lcd_display_init_cmds_part1), mipi_dsi0_base);
		mipi_dsi_cmds_tx(disable_auto_vbpvfp_en_nodrop_cmds, \
			ARRAY_SIZE(disable_auto_vbpvfp_en_nodrop_cmds), mipi_dsi0_base);

		mipi_dsi_cmds_tx(lcd_display_init_cmds_part2, \
			ARRAY_SIZE(lcd_display_init_cmds_part2), mipi_dsi0_base);

		if (pinfo->ifbc_type == IFBC_TYPE_VESA3X_DUAL) {
			mipi_dsi_cmds_tx(lcd_display_init_vesa3x_para, \
					ARRAY_SIZE(lcd_display_init_vesa3x_para), mipi_dsi0_base);
			if (pinfo->current_mode == MODE_10BIT_VIDEO_3X) {
				mipi_dsi_cmds_tx(lcd_display_init_10bit_video_3x_dsc_cmds, \
					ARRAY_SIZE(lcd_display_init_10bit_video_3x_dsc_cmds), mipi_dsi0_base);
			} else if (pinfo->current_mode == MODE_8BIT) {
				mipi_dsi_cmds_tx(lcd_display_init_8bit_cmd_3x_dsc_cmds, \
					ARRAY_SIZE(lcd_display_init_8bit_cmd_3x_dsc_cmds), mipi_dsi0_base);
			}
		} else if (pinfo->ifbc_type == IFBC_TYPE_VESA3_75X_DUAL) {
			mipi_dsi_cmds_tx(lcd_display_init_vesa3_75x_para, \
					ARRAY_SIZE(lcd_display_init_vesa3_75x_para), mipi_dsi0_base);
		}

		mipi_dsi_cmds_tx(lcd_display_init_cmds_part3, \
			ARRAY_SIZE(lcd_display_init_cmds_part3), mipi_dsi0_base);
		if (g_lcd_fpga_flag == 1)
			mipi_dsi_cmds_tx(lcd_display_init_cmds_part4, \
				ARRAY_SIZE(lcd_display_init_cmds_part4), mipi_dsi0_base);

		mipi_dsi_cmds_tx(lcd_display_init_cmds_part5, \
			ARRAY_SIZE(lcd_display_init_cmds_part5), mipi_dsi0_base);

	#ifdef CONFIG_TP_ERR_DEBUG
		if(ts_ops) {
			ts_ops->ts_power_notify(TS_RESUME_DEVICE, NO_SYNC);
		} else {
			DPU_FB_ERR("ts_ops is null,can't notify thp power_on\n");
		}
	#endif

		// check lcd power state
		outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
		status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		while (status & 0x10) {
			udelay(50);
			if (++try_times > 100) {
				DPU_FB_ERR("fb%d, Read lcd power status timeout!\n", dpufd->index);
				break;
			}
			status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		}
		status = inp32(mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
		DPU_FB_INFO("fb%d, SHARP_NT36870_LCD Power State = 0x%x.\n", dpufd->index, status);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;

	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
	#ifdef CONFIG_TP_ERR_DEBUG
		if(ts_ops)
			ts_ops->ts_power_notify(TS_AFTER_RESUME, NO_SYNC);
	#endif
	} else {
		DPU_FB_ERR("failed to init lcd!\n");
	}

	// backlight on
	dpu_lcd_backlight_on(pdev);
	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);

	return 0;
}


static int mipi_sharp_nt36870_panel_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	dpu_check_and_return((pdev == NULL), 0, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return((dpufd == NULL), 0, ERR, "dpufd is NULL\n");
	pinfo = &(dpufd->panel_info);

#ifdef AS_EXT_LCD_ON_ASIC
	mipi_dsi0_base = dpufd->mipi_dsi1_base;
#else
	mipi_dsi0_base = dpufd->mipi_dsi0_base;
#endif

	DPU_FB_INFO("fb%d, +.\n", dpufd->index);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
	#ifdef CONFIG_TP_ERR_DEBUG
		ts_ops = ts_kit_get_ops();
		if(ts_ops) {
			ts_ops->ts_power_notify(TS_EARLY_SUSPEND, NO_SYNC);
		} else {
			DPU_FB_ERR("ts_ops is null,can't notify thp power_off\n");
		}
	#endif
		dpu_lcd_backlight_off(pdev);

		mipi_dsi_cmds_tx(lcd_display_off_cmds, \
			ARRAY_SIZE(lcd_display_off_cmds), mipi_dsi0_base);
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
			if (dpu_is_running_kernel_slt()) {
				// lcd gpio lowpower
				gpio_cmds_tx(asic_lcd_gpio_cancle_vspn_lowpower_cmds, \
					ARRAY_SIZE(asic_lcd_gpio_cancle_vspn_lowpower_cmds));
				// lcd gpio free
				gpio_cmds_tx(asic_lcd_gpio_cancle_vspn_free_cmds, \
					ARRAY_SIZE(asic_lcd_gpio_cancle_vspn_free_cmds));
			} else {
				// lcd gpio lowpower
				gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds, \
					ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));
				// lcd gpio free
				gpio_cmds_tx(asic_lcd_gpio_free_cmds, \
					ARRAY_SIZE(asic_lcd_gpio_free_cmds));
			}
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

	DPU_FB_INFO("fb%d, -.\n", dpufd->index);

	return 0;
}

static int mipi_sharp_nt36870_panel_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return((pdev == NULL), 0, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd)
		return 0;
	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

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

static int mipi_sharp_nt36870_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	static char last_bl_level=0;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	dpu_check_and_return((pdev == NULL), -EINVAL, ERR, "pdev is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is NULL\n");

	if (dpufd->panel_info.bl_set_type & BL_SET_BY_PWM) {

		ret = dpu_pwm_set_backlight(dpufd, bl_level);

	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {

		ret = dpu_blpwm_set_backlight(dpufd, bl_level);

	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {

		ret = dpu_sh_blpwm_set_backlight(dpufd, bl_level);

	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {

		bl_level_adjust[1] = bl_level * 255 / dpufd->panel_info.bl_max;

		if (last_bl_level != bl_level_adjust[1]){
			last_bl_level = bl_level_adjust[1];
		#ifdef AS_EXT_LCD_ON_ASIC
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), dpufd->mipi_dsi1_base);
		#else
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), dpufd->mipi_dsi0_base);
		#endif
			}
		DPU_FB_DEBUG("lw bl_level_adjust[1] = %d.\n",bl_level_adjust[1]);

	} else {
		DPU_FB_ERR("fb%d, not support this bl_set_type %d!\n",
			dpufd->index, dpufd->panel_info.bl_set_type);
	}

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;

}

static int mipi_sharp_nt36870_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpu_check_and_return((pdev == NULL || dirty == NULL), 0, ERR, "pdev or dirty is NULL\n");
	dpufd = platform_get_drvdata(pdev);
	dpu_check_and_return((dpufd == NULL), 0, ERR, "dpufd is NULL\n");

	lcd_disp_x[1] = ((uint32_t)dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = (uint32_t)dirty->x & 0xff;
	lcd_disp_x[3] = (((uint32_t)dirty->x + (uint32_t)dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = ((uint32_t)dirty->x + (uint32_t)dirty->w - 1) & 0xff;
	lcd_disp_y[1] = ((uint32_t)dirty->y >> 8) & 0xff;
	lcd_disp_y[2] = (uint32_t)dirty->y & 0xff;
	lcd_disp_y[3] = (((uint32_t)dirty->y + (uint32_t)dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = ((uint32_t)dirty->y + (uint32_t)dirty->h - 1) & 0xff;

	DPU_FB_DEBUG("x[1] = 0x%2x, x[2] = 0x%2x, x[3] = 0x%2x, x[4] = 0x%2x.\n",
		lcd_disp_x[1], lcd_disp_x[2], lcd_disp_x[3], lcd_disp_x[4]);
	DPU_FB_DEBUG("y[1] = 0x%2x, y[2] = 0x%2x, y[3] = 0x%2x, y[4] = 0x%2x.\n",
		lcd_disp_y[1], lcd_disp_y[2], lcd_disp_y[3], lcd_disp_y[4]);

#ifdef AS_EXT_LCD_ON_ASIC
	mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), dpufd->mipi_dsi1_base);
#else
	mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), dpufd->mipi_dsi0_base);
#endif
	return 0;
}

static ssize_t mipi_sharp_nt36870_lcd_model_show(struct platform_device *pdev,
	char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret = 0;

	if (!pdev || !buf) {
		DPU_FB_ERR("pdev or buf is NULL\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);
	if (NULL == dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);

	ret = snprintf(buf, PAGE_SIZE, "sharp_NT36870 CMD TFT\n");

	DPU_FB_DEBUG("fb%d, -.\n", dpufd->index);

	return ret;
}

static ssize_t mipi_sharp_panel_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0c), lcd_reg_0c},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_ab), lcd_reg_ab},
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
	if (NULL == dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	DPU_FB_DEBUG("fb%d, +.\n", dpufd->index);
	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base)) {
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
	.set_fastboot = mipi_sharp_nt36870_panel_set_fastboot,
	.on = mipi_sharp_nt36870_panel_on,
	.off = mipi_sharp_nt36870_panel_off,
	.remove = mipi_sharp_nt36870_panel_remove,
	.set_backlight = mipi_sharp_nt36870_panel_set_backlight,
	.set_display_region = mipi_sharp_nt36870_panel_set_display_region,

	.lcd_model_show = mipi_sharp_nt36870_lcd_model_show,
	.lcd_check_reg  = mipi_sharp_panel_lcd_check_reg_show,
};

/*******************************************************************************
**
*/
static void display_effect_param_init(struct dpu_panel_info *pinfo)
{
	if (!pinfo)
		return;

	pinfo->prefix_sharpness2D_support = 0;
	pinfo->arsr1p_sharpness_support = 0;
	pinfo->acm_support = 0;
	pinfo->gamma_support = 0;
	pinfo->gmp_support = 0;
	pinfo->xcc_support = 0;
	pinfo->hiace_support = 0;
	pinfo->post_xcc_support = 0;

	if (pinfo->gmp_support == 1) {
		pinfo->gmp_lut_table_low32bit = gmp_lut_table_low32bit;
		pinfo->gmp_lut_table_high4bit = gmp_lut_table_high4bit;
		pinfo->gmp_lut_table_len = ARRAY_SIZE(gmp_lut_table_low32bit);
	}

	if (pinfo->gamma_support == 1) {
		pinfo->igm_lut_table_R = igm_lut_table_R;
		pinfo->igm_lut_table_G = igm_lut_table_G;
		pinfo->igm_lut_table_B = igm_lut_table_B;
		pinfo->igm_lut_table_len = ARRAY_SIZE(igm_lut_table_R);
		pinfo->gamma_lut_table_R = gamma_lut_table_R;
		pinfo->gamma_lut_table_G = gamma_lut_table_G;
		pinfo->gamma_lut_table_B = gamma_lut_table_B;
		pinfo->gamma_lut_table_len = ARRAY_SIZE(gamma_lut_table_R);
		pinfo->xcc_support = 1;
		pinfo->xcc_table = xcc_table;
		pinfo->xcc_table_len = ARRAY_SIZE(xcc_table);
	}

	if (pinfo->post_xcc_support == 1) {
		pinfo->post_xcc_table = xcc_table;
		pinfo->post_xcc_table_len = ARRAY_SIZE(xcc_table);
}

	if (pinfo->hiace_support == 1) {
		pinfo->noisereduction_support = 1;
		pinfo->hiace_param.iGlobalHistBlackPos = 16;
		pinfo->hiace_param.iGlobalHistWhitePos = 240;
		pinfo->hiace_param.iGlobalHistBlackWeight = 51;
		pinfo->hiace_param.iGlobalHistWhiteWeight = 51;
		pinfo->hiace_param.iGlobalHistZeroCutRatio = 486;
		pinfo->hiace_param.iGlobalHistSlopeCutRatio = 410;
		pinfo->hiace_param.iMaxLcdLuminance = 500;
		pinfo->hiace_param.iMinLcdLuminance = 3;
		strncpy(pinfo->hiace_param.chCfgName, "/product/etc/display/effect/algorithm/hdr_engine_MHA.xml", sizeof(pinfo->hiace_param.chCfgName) - 1);
	}

	return;
}

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

static void panel_mode_swtich_para_store(struct dpu_panel_info *pinfo)
{
	if (NULL == pinfo)
		return;

	/*59Hz, soc-ic vporch ok*/
	pinfo->ldi.hbp_store_vid = 50;
	pinfo->ldi.hfp_store_vid = 200;
	pinfo->ldi.hpw_store_vid = 50;
	pinfo->ldi.vbp_store_vid = 70;
	pinfo->ldi.vfp_store_vid = 28;
	pinfo->ldi.vpw_store_vid = 18;
	pinfo->ldi.pxl_clk_store_vid = 332 * 1000000UL;

	pinfo->ldi.hbp_store_cmd = pinfo->ldi.h_back_porch;
	pinfo->ldi.hfp_store_cmd = pinfo->ldi.h_front_porch;
	pinfo->ldi.hpw_store_cmd = pinfo->ldi.h_pulse_width;
	pinfo->ldi.vbp_store_cmd = pinfo->ldi.v_back_porch ;
	pinfo->ldi.vfp_store_cmd = pinfo->ldi.v_front_porch;
	pinfo->ldi.vpw_store_cmd = pinfo->ldi.v_pulse_width;
	pinfo->ldi.pxl_clk_store_cmd = pinfo->pxl_clk_rate;

	return;
}

static void dsc_config_initial(struct dpu_panel_info *pinfo)
{
	struct dsc_algorithm_manager *pt= get_dsc_algorithm_manager_instance();
	struct input_dsc_info input_dsc_info;

	dpu_check_and_no_retval((pt == NULL) || (pinfo == NULL), ERR, "[DSC] NULL Pointer");
	input_dsc_info.dsc_version = DSC_VERSION_V_1_1;
	input_dsc_info.format = DSC_RGB;
	input_dsc_info.pic_width = pinfo->xres;
	input_dsc_info.pic_height = pinfo->yres;
	input_dsc_info.slice_width = input_dsc_info.pic_width / 2;
	// slice height is same with DDIC
	input_dsc_info.slice_height = 7 + 1;
	input_dsc_info.dsc_bpp = DSC_10BPP;
	input_dsc_info.dsc_bpc = DSC_10BPC;
	input_dsc_info.block_pred_enable = 1;
	input_dsc_info.linebuf_depth = LINEBUF_DEPTH_11;
	input_dsc_info.gen_rc_params = DSC_NOT_GENERATE_RC_PARAMETERS;
	pt->vesa_dsc_info_calc(&input_dsc_info, &(pinfo->panel_dsc_info.dsc_info), NULL);
	pinfo->vesa_dsc.bits_per_pixel = DSC_0BPP;
}

static char lcd_bl_ic_name_buf[LCD_BL_IC_NAME_MAX];

static void mipi_sharp_nt36870_init_panel_info(struct dpu_panel_info *pinfo, uint32_t bl_type)
{
	pinfo->xres = 1440;
	pinfo->yres = 2560;
	pinfo->width = 75;
	pinfo->height = 133;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;

	if (!strncmp(lcd_bl_ic_name_buf, "LM36923YFFR", strlen("LM36923YFFR"))) {
		pinfo->bl_min = 55;
		/* 10000stage 7992,2048stage 1973 for 450nit */
		pinfo->bl_max = 7992;
		pinfo->bl_default = 4000;
		pinfo->blpwm_precision_type = BLPWM_PRECISION_2048_TYPE;
		pinfo->bl_ic_ctrl_mode = REG_ONLY_MODE;
		gpio_lcd_bl_enable = 0;//GPIO_000 can be used ?
		DPU_FB_INFO("LM36923, set gpio_lcd_bl_enable[%d], do not ctrl gpio_bl_en.\n", gpio_lcd_bl_enable);
	} else {
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
	}

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;

	/* for 10bit_video - 8bit_cmd mode switch */
	pinfo->panel_mode_swtich_support = 1;
	pinfo->current_mode = MODE_8BIT;
	pinfo->mode_switch_to = pinfo->current_mode;

	if (pinfo->current_mode == MODE_10BIT_VIDEO_3X) {
		pinfo->type = PANEL_MIPI_VIDEO;
	} else {
		pinfo->type = PANEL_MIPI_CMD;
	}
}

static void mipi_sharp_nt36870_init_ldi_dsi_param(struct dpu_panel_info *pinfo)
{
	if (g_lcd_fpga_flag == 1) {
		if (pinfo->type == PANEL_MIPI_CMD) {
			pinfo->ldi.h_back_porch = 96;//0x6ff
			pinfo->ldi.h_front_porch = 1068; //108
			pinfo->ldi.h_pulse_width = 48;
			pinfo->ldi.v_back_porch = 12;
			pinfo->ldi.v_front_porch = 14;
			pinfo->ldi.v_pulse_width = 4;

			pinfo->mipi.dsi_bit_clk = 240;
			pinfo->mipi.dsi_bit_clk_val1 = 220;
			pinfo->mipi.dsi_bit_clk_val2 = 230;
			pinfo->mipi.dsi_bit_clk_val3 = 240;
			pinfo->mipi.dsi_bit_clk_val4 = 250;
			pinfo->mipi.dsi_bit_clk_val5 = 260;
			pinfo->dsi_bit_clk_upt_support = 0;
			pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
			pinfo->pxl_clk_rate = 20* 1000000UL;
		} else {
			pinfo->ldi.h_back_porch = 96;//0x6ff
			pinfo->ldi.h_front_porch = 108; //108
			pinfo->ldi.h_pulse_width = 48;
			pinfo->ldi.v_back_porch = 12;
			pinfo->ldi.v_front_porch = 14;
			pinfo->ldi.v_pulse_width = 4;

			pinfo->mipi.dsi_bit_clk = 240;
			pinfo->mipi.dsi_bit_clk_val1 = 220;
			pinfo->mipi.dsi_bit_clk_val2 = 230;
			pinfo->mipi.dsi_bit_clk_val3 = 240;
			pinfo->mipi.dsi_bit_clk_val4 = 250;
			pinfo->mipi.dsi_bit_clk_val5 = 260;
			pinfo->dsi_bit_clk_upt_support = 0;
			pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
			pinfo->pxl_clk_rate = 20 * 1000000UL;
		}

		} else {
			if (pinfo->type == PANEL_MIPI_CMD) {
				pinfo->ldi.h_back_porch = 30;
				pinfo->ldi.h_front_porch = 96;
				pinfo->ldi.h_pulse_width = 30;
				pinfo->ldi.v_back_porch = 60;
				pinfo->ldi.v_front_porch = 14;
				pinfo->ldi.v_pulse_width = 30;
				pinfo->mipi.dsi_bit_clk = 648;

				pinfo->dsi_bit_clk_upt_support = 0;
				pinfo->mipi.dsi_bit_clk_val1 = 658;
				pinfo->mipi.dsi_bit_clk_val2 = 645;
				pinfo->mipi.dsi_bit_clk_val3 = 630;
				pinfo->mipi.dsi_bit_clk_val4 = 580;

				pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
				pinfo->pxl_clk_rate = 277 * 1000000UL;
			} else {
				pinfo->ldi.h_back_porch = 40;
				pinfo->ldi.h_front_porch = 180;
				pinfo->ldi.h_pulse_width = 40;
				pinfo->ldi.v_back_porch = 70;
				pinfo->ldi.v_front_porch = 28;
				pinfo->ldi.v_pulse_width = 18;
				pinfo->pxl_clk_rate = 332 * 1000000UL;
				pinfo->mipi.dsi_bit_clk = 850; // 750
				pinfo->mipi.dsi_bit_clk_val1 = 758;
				pinfo->mipi.dsi_bit_clk_val2 = 745;
				pinfo->mipi.dsi_bit_clk_val3 = 730;
				pinfo->mipi.dsi_bit_clk_val4 = 780;
				pinfo->dsi_bit_clk_upt_support = 0;
				pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
				pinfo->mipi.dsi_bit_clk_default = pinfo->mipi.dsi_bit_clk;

				pinfo->hisync_mode = 0;
				pinfo->vsync_delay_time = 0;
				pinfo->video_idle_mode = 0;
			}
		}
	pinfo->mipi.dsi_version = DSI_1_1_VERSION;
}

static void mipi_sharp_nt36870_init_dirty_region(struct dpu_panel_info *pinfo)
{
	pinfo->dirty_region_updt_support = 0;
	pinfo->dirty_region_info.left_align = -1;
	pinfo->dirty_region_info.right_align = -1;
	pinfo->dirty_region_info.top_align = 32;
	pinfo->dirty_region_info.bottom_align = 32;
	pinfo->dirty_region_info.w_align = -1;
	pinfo->dirty_region_info.h_align = -1;
	pinfo->dirty_region_info.w_min = 1440;
	pinfo->dirty_region_info.h_min = 32;
	pinfo->dirty_region_info.top_start = -1;
	pinfo->dirty_region_info.bottom_start = -1;
}

static void mipi_sharp_nt36870_mipi_set(struct dpu_panel_info *pinfo)
{
	pinfo->mipi.clk_post_adjust = 215;
	pinfo->mipi.clk_pre_adjust= 0;
	pinfo->mipi.clk_t_hs_prepare_adjust= 0;
	pinfo->mipi.clk_t_lpx_adjust= 0;
	pinfo->mipi.clk_t_hs_trial_adjust= 0;
	pinfo->mipi.clk_t_hs_exit_adjust= 0;
	pinfo->mipi.clk_t_hs_zero_adjust= 0;
	pinfo->mipi.non_continue_en = 1;

	//mipi
	pinfo->mipi.lane_nums = DSI_2_LANES;//DSI_3_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;//DSI_DSC24_COMPRESSED_DATA

	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
	pinfo->mipi.phy_mode = CPHY_MODE;
}

static void mipi_sharp_nt36870_gpio_set(struct device_node *np)
{
	if (g_lcd_fpga_flag == 1) {
		gpio_lcd_p5v8_enable = of_get_named_gpio(np, "gpios", 0);
		gpio_lcd_n5v8_enable = of_get_named_gpio(np, "gpios", 1);
		gpio_lcd_reset = of_get_named_gpio(np, "gpios", 2);
		gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 3);
		gpio_lcd_1v8 = of_get_named_gpio(np, "gpios", 4);
	} else {
		//033, 283, 279, 282, 015, 281, 003
		gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 0);
		gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 1);
		gpio_lcd_reset = of_get_named_gpio(np, "gpios", 2);
		gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 3);
		gpio_lcd_id0 = of_get_named_gpio(np, "gpios", 4);
	}
}

static int mipi_sharp_nt36870_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;
	const char *lcd_bl_ic_name = NULL;

	DPU_FB_INFO("+!\n");
	g_mipi_lcd_name = SHARP_2LANE_NT36870;
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SHARP_NT36870);
	if (!np) {
		DPU_FB_ERR("not found device node %s!\n", DTS_COMP_SHARP_NT36870);
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

	ret = of_property_read_string_index(np, "lcd-bl-ic-name", 0, &lcd_bl_ic_name);
	if (ret != 0) {
		memcpy(lcd_bl_ic_name_buf, "INVALID", strlen("INVALID"));
	} else {
		if (strlen(lcd_bl_ic_name) >= LCD_BL_IC_NAME_MAX) {
			DPU_FB_ERR("The length of the lcd_bl_ic_name is greater than LCD_BL_IC_NAME_MAX\n");
			goto err_return;
		}
		memcpy(lcd_bl_ic_name_buf, lcd_bl_ic_name, strlen(lcd_bl_ic_name) + 1);
	}

	if (dpu_fb_device_probe_defer(lcd_display_type, bl_type))
		goto err_probe_defer;

	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcd_fpga_flag);//lint !e64
	if (ret)
		DPU_FB_WARNING("need to get g_lcd_fpga_flag resource in fpga, not needed in asic!\n");

	mipi_sharp_nt36870_gpio_set(np);

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return 0;
	}

	pdev->id = 1;
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));
	mipi_sharp_nt36870_init_panel_info(pinfo,bl_type);
	mipi_sharp_nt36870_init_ldi_dsi_param(pinfo);

	if (pinfo->panel_mode_swtich_support)
		panel_mode_swtich_para_store(pinfo);

	mipi_sharp_nt36870_mipi_set(pinfo);
	display_effect_param_init(pinfo);

	if (pinfo->type == PANEL_MIPI_CMD)
		mipi_sharp_nt36870_init_dirty_region(pinfo);
	/* The host processor must wait for more than 15us from the end of write data transfer to a command 2Ah/2Bh */
	if (pinfo->dirty_region_updt_support == 1)
		pinfo->mipi.hs_wr_to_time = 17000;        /* measured in nS */

	pinfo->ifbc_type = IFBC_TYPE_VESA3X_DUAL;//IFBC_TYPE_VESA3_75X_DUAL;//
	pinfo->pxl_clk_rate_div = 3;

	if (pinfo->current_mode == MODE_10BIT_VIDEO_3X) {
		dsc_config_initial(pinfo);
	} else {
		get_vesa_dsc_para(pinfo, pinfo->current_mode);
	}
	if (g_lcd_fpga_flag == 0) {
		if (pinfo->pxl_clk_rate_div > 1) {
			pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
		}
	}

	ret = lcd_voltage_relevant_init(pdev);
	if (ret != 0) {
		goto err_return;
	}

	ret = platform_device_add_data(pdev, &g_panel_data,
		sizeof(struct dpu_fb_panel_data));
	if (ret) {
		DPU_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	dpu_fb_add_device(pdev);

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
		.compatible = DTS_COMP_SHARP_NT36870,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, dpu_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_sharp_nt36870_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_sharp_NT36870",
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
/*lint +e551 +e551*/
#pragma GCC diagnostic pop
