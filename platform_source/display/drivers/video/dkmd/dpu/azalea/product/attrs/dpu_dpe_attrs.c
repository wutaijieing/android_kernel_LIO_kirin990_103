/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 */

#include "dpu_fb.h"

int dpe_get_lcd_id(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	return panel_next_get_lcd_id(pdev);
}

int dpe_panel_switch(struct platform_device *pdev, uint32_t fold_status)
{
	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}

	return panel_next_panel_switch(pdev, fold_status);
}

struct dpu_panel_info* dpe_get_panel_info(struct platform_device *pdev, uint32_t panel_id)
{
	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return NULL;
	}

	return panel_next_get_panel_info(pdev, panel_id);
}

ssize_t dpe_lcd_model_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_model_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_check_reg(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_mipi_detect_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_mipi_detect(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_hkadc_debug_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_hkadc_debug_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_hkadc_debug_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_hkadc_debug_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_gram_check_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_gram_check_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_gram_check_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_gram_check_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_dynamic_sram_checksum_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_dynamic_sram_checksum_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_dynamic_sram_checksum_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_dynamic_sram_checksum_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}


ssize_t dpe_lcd_color_temperature_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = dpe_show_ct_csc_value(dpufd, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	return ret;
}

ssize_t dpe_lcd_color_temperature_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t flag;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int csc_value[100];
	char *cur = NULL;
	char *token = NULL;
	int i = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	cur = (char *)buf;
	token = strsep(&cur, ",");
	while (token != NULL) {
		csc_value[i++] = simple_strtol(token, NULL, 0);
		token = strsep(&cur, ",");
		if (i >= 100)
			return count;
	}

	// if the csc_value is not proper, maybe cause the blank screen issue.
	// now only use csc_value[0] ~ csc_value[8] from user, correspond R,G,B,
	// and csc_value[0], csc_value[4], csc_value[8] are not zero.
	for (i = 0; i < CSC_VALUE_NUM; i++) {
		DPU_FB_DEBUG("csc_value[%d] = %u\n", i, csc_value[i]);
		if ((csc_value[i] > CSC_VALUE_MAX) ||
			(!(i % 4) && !csc_value[i])) {
			DPU_FB_ERR("csc_value[%d] = %u, not support\n",
				i, csc_value[i]);
			return count;
		}
	}

	if (pinfo->xcc_support) {
		dpe_store_ct_csc_value(dpufd, csc_value, ARRAY_SIZE(csc_value));
		if (pinfo->xcc_set_in_isr_support) {
			dpufd->xcc_coef_set = 1;
			dpufd->color_temperature_flag = 2;
		} else {
			flag = dpe_set_ct_csc_value(dpufd);
		}
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_lcd_ic_color_enhancement_mode_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pedv NULL Pointer!\n");
		return 0;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd NULL Pointer!\n");
		return 0;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_ic_color_enhancement_mode_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_ic_color_enhancement_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev NULL Pointer!\n");
		return 0;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd NULL Pointer!\n");
		return 0;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_ic_color_enhancement_mode_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_led_rg_lcd_color_temperature_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = dpe_show_led_rg_ct_csc_value(buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	return ret;
}

ssize_t dpe_led_rg_lcd_color_temperature_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t flag;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int csc_value[100];
	char *token = NULL;
	char *cur = NULL;
	int i = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	cur = (char *)buf;
	token = strsep(&cur, ",");
	while (token != NULL) {
		csc_value[i++] = simple_strtol(token, NULL, 0);
		token = strsep(&cur, ",");
		if (i >= 100)
			return count;
	}

	// if the csc_value is not proper, maybe cause the blank screen issue.
	// now only use csc_value[0] ~ csc_value[8] from user, correspond R,G,B,
	// and csc_value[0], csc_value[4], csc_value[8] are not zero.
	for (i = 0; i < CSC_VALUE_NUM; i++) {
		DPU_FB_DEBUG("csc_value[%d] = %u\n", i, csc_value[i]);
		if ((csc_value[i] > CSC_VALUE_MAX) ||
			(!(i % 4) && !csc_value[i])) {
			DPU_FB_ERR("csc_value[%d] = %u, not support\n",
				i, csc_value[i]);
			return count;
		}
	}

	if (pinfo->color_temperature_support) {
		dpe_store_led_rg_ct_csc_value(csc_value, ARRAY_SIZE(csc_value));
		flag = dpe_set_led_rg_ct_csc_value(dpufd);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_lcd_comform_mode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = dpe_show_comform_ct_csc_value(dpufd, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	return ret;
}

ssize_t dpe_lcd_comform_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t flag;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int val = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (pinfo->comform_mode_support) {
		val = simple_strtoul(buf, NULL, 0);
		if (val <= COMFORM_MAX) {
			DPU_FB_INFO("comform_mode = %d !\n", val);
			dpe_update_g_comform_discount(val);
			if (pinfo->xcc_set_in_isr_support)
				dpufd->xcc_coef_set = 1;
			else
				flag = dpe_set_comform_ct_csc_value(dpufd);
		} else {
			DPU_FB_ERR("comform_mode = %d, not support!\n", val);
		}
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_lcd_cinema_mode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = dpe_show_cinema_value(dpufd, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	return ret;
}

ssize_t dpe_lcd_cinema_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int val = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (pinfo->cinema_mode_support) {
		val = simple_strtoul(buf, NULL, 0);
		if (val == 1 || val == 0) {
			DPU_FB_INFO("cinema_mode = %d !\n", val);
			dpe_set_cinema(dpufd, val);
		} else {
			DPU_FB_ERR("cinema_mode = %d, not support\n", val);
		}
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_lcd_support_mode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_support_mode_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_support_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_support_mode_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_voltage_enable_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_voltage_enable_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_bist_check(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_bist_check(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_sleep_ctrl_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_sleep_ctrl_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_sleep_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_sleep_ctrl_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_test_config_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_test_config_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_test_config_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_test_config_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_reg_read_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_reg_read_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_reg_read_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_reg_read_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_support_checkmode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_support_checkmode_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_lp2hs_mipi_check_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_lp2hs_mipi_check_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_lp2hs_mipi_check_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_lp2hs_mipi_check_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_amoled_pcd_errflag_check(struct platform_device *pdev, char *buf)
{
	ssize_t ret;

	ret = panel_next_amoled_pcd_errflag_check(pdev, buf);
	return ret;
}
ssize_t dpe_lcd_hbm_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_hbm_ctrl_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_hbm_ctrl_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_hbm_ctrl_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_acl_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("lcd_acl_ctrl_store pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("lcd_acl_ctrl_store buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("lcd_acl_ctrl_store dpufd NULL Pointer!\n");
		return -1;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_acl_ctrl_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_acl_ctrl_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("lcd_acl_ctrl_show pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("lcd_acl_ctrl_show buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("lcd_acl_ctrl_show dpufd NULL Pointer!\n");
		return -1;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_lcd_acl_ctrl_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_acm_state_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("acm_state_show pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("acm_state_show buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("acm_state_show dpufd NULL Pointer!\n");
		return -1;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = dpe_show_acm_state(buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_acm_state_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int val = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("acm_state_store pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("acm_state_store buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("acm_state_store dpufd NULL Pointer!\n");
		return -1;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (pinfo->acm_support) {
		val = simple_strtoul(buf, NULL, 0);
		dpe_update_g_acm_state(val);
		dpe_set_acm_state(dpufd);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_gmp_state_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("gmp_state_show pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("gmp_state_show buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("gmp_state_show dpufd NULL Pointer!\n");
		return -1;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = dpe_show_gmp_state(buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}
ssize_t dpe_gmp_state_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int val = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("gmp_state_store pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("gmp_state_store buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("gmp_state_store dpufd NULL Pointer!\n");
		return -1;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (pinfo->gmp_support) {
		val = simple_strtoul(buf, NULL, 0);
		dpe_update_g_gmp_state(val);
		dpe_set_gmp_state(dpufd);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_alpm_setting_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("alpm_setting_store pdev NULL Pointer!\n");
		return -1;
	}

	if (buf == NULL) {
		DPU_FB_ERR("alpm_setting_store buf NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("alpm_setting_store dpufd NULL Pointer!\n");
		return -1;
	}

	pinfo = &(dpufd->panel_info);

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	panel_next_alpm_setting_store(pdev, buf, count);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return count;
}

ssize_t dpe_sharpness2d_table_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_sharpness2d_table_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}
ssize_t dpe_sharpness2d_table_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_sharpness2d_table_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_panel_info_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_panel_info_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_mipi_dsi_bit_clk_upt_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_mipi_dsi_bit_clk_upt_show(pdev, buf);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_mipi_dsi_bit_clk_upt_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
	ret = panel_next_mipi_dsi_bit_clk_upt_store(pdev, buf, count);
	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

ssize_t dpe_lcd_xcc_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int flag = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	unsigned int csc_value[100] = {0};
	char *cur = NULL;
	char *token = NULL;
	int i = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("pdev NULL Pointer!\n");
		return -1;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd NULL Pointer!\n");
		return -1;
	}

	pinfo = &(dpufd->panel_info);
	if (pinfo == NULL) {
		DPU_FB_ERR("pinfo NULL Pointer!\n");
		return -1;
	}
	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	cur = (char *)buf;
	token = strsep(&cur, ",");
	while (token != NULL) {
		csc_value[i++] = simple_strtol(token, NULL, 0);
		token = strsep(&cur, ",");
		if (i >= 100)
			return count;
	}

	if (pinfo->xcc_support) {
		dpe_store_ct_csc_value(dpufd, csc_value, ARRAY_SIZE(csc_value));
		flag = dpe_set_ct_csc_value(dpufd);
	}

	DPU_FB_DEBUG("fb%d, flag = %d\n", dpufd->index, flag);
	return count;
}

