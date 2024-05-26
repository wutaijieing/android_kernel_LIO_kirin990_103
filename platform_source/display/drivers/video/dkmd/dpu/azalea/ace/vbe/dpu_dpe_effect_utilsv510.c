/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "dpu_dpe_utils.h"
#include <linux/sysctl.h>

static unsigned int g_comform_value;
static unsigned int g_acm_state;
static unsigned int g_gmp_state;
static unsigned int g_led_rg_csc_value[9];
static unsigned int g_is_led_rg_csc_set;
unsigned int g_led_rg_para1 = 7;
unsigned int g_led_rg_para2 = 30983;
/*lint -esym(552,g_dpp_cmdlist_delay)*/
int8_t g_dpp_cmdlist_delay;

#define GMP_CNT_COFE 4913 /* 17*17*17 */
#define XCC_CNT_COFE 12
#define COLOR_RECTIFY_DEFAULT 0x8000
#define CSC_VALUE_MIN_LEN 9

void dpe_set_acm_state(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}

ssize_t dpe_show_acm_state(char *buf)
{
	ssize_t ret;

	if (buf == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "g_acm_state = %d\n", g_acm_state);

	return ret;
}

void acm_set_lut(char __iomem *address, uint32_t table[], uint32_t size)
{
	(void)address;
	(void)table;
	(void)size;
}

void acm_set_lut_hue(char __iomem *address, uint32_t table[], uint32_t size)
{
	(void)address;
	(void)table;
	(void)size;
}

void dpe_update_g_acm_state(unsigned int value)
{
	(void)value;
}

void dpe_store_ct_csc_value(struct dpu_fb_data_type *dpufd, unsigned int csc_value[], unsigned int len)
{
	struct dpu_panel_info *pinfo = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (len < CSC_VALUE_MIN_LEN) {
		DPU_FB_ERR("csc_value len is too short\n");
		return;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL)
		return;

	pinfo->xcc_table[1] = csc_value[0];
	pinfo->xcc_table[2] = csc_value[1];
	pinfo->xcc_table[3] = csc_value[2];
	pinfo->xcc_table[5] = csc_value[3];
	pinfo->xcc_table[6] = csc_value[4];
	pinfo->xcc_table[7] = csc_value[5];
	pinfo->xcc_table[9] = csc_value[6];
	pinfo->xcc_table[10] = csc_value[7];
	pinfo->xcc_table[11] = csc_value[8];
}

static void update_default_xcc_param(char __iomem *xcc_base, struct dpu_panel_info *pinfo,
	struct rgb rectify_rgb)
{
		outp32(xcc_base + XCC_COEF_00, pinfo->xcc_table[0]);
		outp32(xcc_base + XCC_COEF_01, pinfo->xcc_table[1] *
			g_led_rg_csc_value[0] / COLOR_RECTIFY_DEFAULT *
			rectify_rgb.color_temp_rectify_r / COLOR_RECTIFY_DEFAULT);
		outp32(xcc_base + XCC_COEF_02, pinfo->xcc_table[2]);
		outp32(xcc_base + XCC_COEF_03, pinfo->xcc_table[3]);
		outp32(xcc_base + XCC_COEF_10, pinfo->xcc_table[4]);
		outp32(xcc_base + XCC_COEF_11, pinfo->xcc_table[5]);
		outp32(xcc_base + XCC_COEF_12, pinfo->xcc_table[6] *
			g_led_rg_csc_value[4] / COLOR_RECTIFY_DEFAULT *
			rectify_rgb.color_temp_rectify_g / COLOR_RECTIFY_DEFAULT);
		outp32(xcc_base + XCC_COEF_13, pinfo->xcc_table[7]);
		outp32(xcc_base + XCC_COEF_20, pinfo->xcc_table[8]);
		outp32(xcc_base + XCC_COEF_21, pinfo->xcc_table[9]);
		outp32(xcc_base + XCC_COEF_22, pinfo->xcc_table[10]);
		outp32(xcc_base + XCC_COEF_23, pinfo->xcc_table[11] *
			g_led_rg_csc_value[8] / COLOR_RECTIFY_DEFAULT *
			discount_coefficient(g_comform_value) / CHANGE_MAX *
			rectify_rgb.color_temp_rectify_b / COLOR_RECTIFY_DEFAULT);
}

static uint32_t get_color_rectify_rgb(uint32_t color_rectify_rgb, uint8_t color_rectify_support)
{
	uint32_t rgb = COLOR_RECTIFY_DEFAULT;

	if (color_rectify_support && color_rectify_rgb && (color_rectify_rgb <= COLOR_RECTIFY_DEFAULT))
		rgb = color_rectify_rgb;

	return rgb;
}

int dpe_set_ct_csc_value(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *xcc_base = NULL;
	/* color temp rectify rgb default valve */
	struct rgb rectify_rgb;

	dpu_check_and_return((dpufd == NULL), -EINVAL, ERR, "dpufd is nullptr!\n");

	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return -1;
	}

	rectify_rgb.color_temp_rectify_r = get_color_rectify_rgb(pinfo->color_temp_rectify_R,
		pinfo->color_temp_rectify_support);
	rectify_rgb.color_temp_rectify_g = get_color_rectify_rgb(pinfo->color_temp_rectify_G,
		pinfo->color_temp_rectify_support);
	rectify_rgb.color_temp_rectify_b = get_color_rectify_rgb(pinfo->color_temp_rectify_B,
		pinfo->color_temp_rectify_support);

	/* XCC */
	if (pinfo->xcc_support == 1) {
		/* XCC matrix */
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			update_default_xcc_param(xcc_base, pinfo, rectify_rgb);
			dpufd->color_temperature_flag = 2; /* display effect flag */
		}
	}

	return 0;
}

ssize_t dpe_show_ct_csc_value(struct dpu_fb_data_type *dpufd, char *buf)
{
	struct dpu_panel_info *pinfo = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL)
		return 0;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		pinfo->xcc_table[1], pinfo->xcc_table[2], pinfo->xcc_table[3],
		pinfo->xcc_table[5], pinfo->xcc_table[6], pinfo->xcc_table[7],
		pinfo->xcc_table[9], pinfo->xcc_table[10], pinfo->xcc_table[11]);
}

int dpe_set_xcc_csc_value(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;

	return 0;
}

int dpe_set_comform_ct_csc_value(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *xcc_base = NULL;
	struct rgb rectify_rgb;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return -1;
	}

	rectify_rgb.color_temp_rectify_r = get_color_rectify_rgb(pinfo->color_temp_rectify_R,
		pinfo->color_temp_rectify_support);
	rectify_rgb.color_temp_rectify_g = get_color_rectify_rgb(pinfo->color_temp_rectify_G,
		pinfo->color_temp_rectify_support);
	rectify_rgb.color_temp_rectify_b = get_color_rectify_rgb(pinfo->color_temp_rectify_B,
		pinfo->color_temp_rectify_support);

	/* XCC */
	if (pinfo->xcc_support == 1) {
		/* XCC matrix */
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table)
			update_default_xcc_param(xcc_base, pinfo, rectify_rgb);
	}

	return 0;
}


void dpe_init_led_rg_ct_csc_value(void)
{
	/* led rg csc default valve */
	g_led_rg_csc_value[0] = COLOR_RECTIFY_DEFAULT;
	g_led_rg_csc_value[1] = 0;
	g_led_rg_csc_value[2] = 0;
	g_led_rg_csc_value[3] = 0;
	g_led_rg_csc_value[4] = COLOR_RECTIFY_DEFAULT;
	g_led_rg_csc_value[5] = 0;
	g_led_rg_csc_value[6] = 0;
	g_led_rg_csc_value[7] = 0;
	g_led_rg_csc_value[8] = COLOR_RECTIFY_DEFAULT;
	g_is_led_rg_csc_set = 0;
}

void dpe_store_led_rg_ct_csc_value(unsigned int csc_value[], unsigned int len)
{
	if (len < CSC_VALUE_MIN_LEN) {
		DPU_FB_ERR("csc_value len is too short\n");
		return;
	}

	g_led_rg_csc_value[0] = csc_value[0];
	g_led_rg_csc_value[1] = csc_value[1];
	g_led_rg_csc_value[2] = csc_value[2];
	g_led_rg_csc_value[3] = csc_value[3];
	g_led_rg_csc_value[4] = csc_value[4];
	g_led_rg_csc_value[5] = csc_value[5];
	g_led_rg_csc_value[6] = csc_value[6];
	g_led_rg_csc_value[7] = csc_value[7];
	g_led_rg_csc_value[8] = csc_value[8];
	g_is_led_rg_csc_set = 1;
}

int dpe_set_led_rg_ct_csc_value(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *xcc_base = NULL;
	/* color_temp_rectify_rgb default valve */
	struct rgb rectify_rgb;

	dpu_check_and_return((!dpufd), -EINVAL, ERR, "dpufd is nullptr!\n");

	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return -1;
	}

	rectify_rgb.color_temp_rectify_r = get_color_rectify_rgb(pinfo->color_temp_rectify_R,
		pinfo->color_temp_rectify_support);
	rectify_rgb.color_temp_rectify_g = get_color_rectify_rgb(pinfo->color_temp_rectify_G,
		pinfo->color_temp_rectify_support);
	rectify_rgb.color_temp_rectify_b = get_color_rectify_rgb(pinfo->color_temp_rectify_B,
		pinfo->color_temp_rectify_support);

	/* XCC */
	if (g_is_led_rg_csc_set == 1 && pinfo->xcc_support == 1) {
		DPU_FB_DEBUG("real set color temperature: g_is_led_rg_csc_set = %d, R = 0x%x, G = 0x%x, B = 0x%x .\n",
			g_is_led_rg_csc_set, g_led_rg_csc_value[0],
			g_led_rg_csc_value[4], g_led_rg_csc_value[8]);
		/* XCC matrix */
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table)
			update_default_xcc_param(xcc_base, pinfo, rectify_rgb);
	}

	return 0;
}

ssize_t dpe_show_led_rg_ct_csc_value(char *buf)
{
	if (!buf) {
		DPU_FB_ERR("buf, NUll pointer warning\n");
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		g_led_rg_para1, g_led_rg_para2,
		g_led_rg_csc_value[0], g_led_rg_csc_value[1], g_led_rg_csc_value[2],
		g_led_rg_csc_value[3], g_led_rg_csc_value[4], g_led_rg_csc_value[5],
		g_led_rg_csc_value[6], g_led_rg_csc_value[7], g_led_rg_csc_value[8]);
}

ssize_t dpe_show_comform_ct_csc_value(struct dpu_fb_data_type *dpufd, char *buf)
{
	struct dpu_panel_info *pinfo = NULL;

	dpu_check_and_return(!dpufd || !buf, 0, ERR, "dpufd or buf NUll pointer warning\n");

	pinfo = &(dpufd->panel_info);
	if (pinfo->xcc_support == 0 || !pinfo->xcc_table)
		return 0;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,g_comform_value = %d\n",
		pinfo->xcc_table[1], pinfo->xcc_table[2], pinfo->xcc_table[3],
		pinfo->xcc_table[5], pinfo->xcc_table[6], pinfo->xcc_table[7],
		pinfo->xcc_table[9], pinfo->xcc_table[10], pinfo->xcc_table[11],
		g_comform_value);
}

void dpe_update_g_comform_discount(unsigned int value)
{
	g_comform_value = value;
	DPU_FB_INFO("g_comform_value = %d\n", g_comform_value);
}

ssize_t dpe_show_cinema_value(struct dpu_fb_data_type *dpufd, char *buf)
{
	dpu_check_and_return((!dpufd || !buf), 0, ERR, "dpufd or buf is NULL\n");

	return snprintf(buf, PAGE_SIZE, "gamma type is = %d\n", dpufd->panel_info.gamma_type);
}

int dpe_set_cinema(struct dpu_fb_data_type *dpufd, unsigned int value)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd, NUll pointer warning.\n");
		return -1;
	}

	if (dpufd->panel_info.gamma_type == value) {
		DPU_FB_DEBUG("fb%d, cinema mode is already in %d!\n", dpufd->index, value);
		return 0;
	}

	dpufd->panel_info.gamma_type = value;

	return 0;
}

void dpe_update_g_gmp_state(unsigned int value)
{
	(void)value;
}

void dpe_set_gmp_state(struct dpu_fb_data_type *dpufd)
{
	(void)dpufd;
}

ssize_t dpe_show_gmp_state(char *buf)
{
	ssize_t ret;

	if (!buf) {
		DPU_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "g_gmp_state = %d\n", g_gmp_state);

	return ret;
}

/* For some malfunctioning machines, after a deep sleep, the memory of the gama / degama
 * lut table exits shutdown slowly.
 * At this time, the software setting lut table will not take effect,. It will be random values in the hardware.
 * In order to avoid this kind of problem, once it is detected that the lut table does not take effect as expected,
 * software delay 60ms,
 * then set gama / degama lut table again. 60ms is the empirical value estimated by the software.
 */
#define LUT_CHECK_STEP 0x8
#define LUT_LOW_MASK 0xfff
#define LUT_SET_STEP 0x2
#define LUT_DELAY_TIME 60
void init_dpp(struct dpu_fb_data_type *dpufd)
{
	char __iomem *disp_ch_base = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL point!\n");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		disp_ch_base = dpufd->dss_base + DSS_DISP_CH0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		disp_ch_base = dpufd->dss_base + DSS_DISP_CH2_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}
	DPU_FB_DEBUG("fb%d +\n", dpufd->index);

	outp32(disp_ch_base + IMG_SIZE_BEF_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));
	outp32(disp_ch_base + IMG_SIZE_AFT_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));
	outp32(disp_ch_base + IMG_SIZE_AFT_IFBCSW, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));

#ifdef CONFIG_DPU_FB_DPP_COLORBAR_USED
	DPU_FB_DEBUG("fb%d enter dpp colorbar mode\n", dpufd->index);
	outp32(disp_ch_base + DPP_CLRBAR_CTRL, (DSS_HEIGHT(pinfo->xres / 4 / 3) << 24) | (0 << 1) | 0x1);
	set_reg(disp_ch_base + DPP_CLRBAR_1ST_CLR, 0x3FF00000, 30, 0);  /* Red */
	set_reg(disp_ch_base + DPP_CLRBAR_2ND_CLR, 0x000FFC00, 30, 0);  /* Green */
	set_reg(disp_ch_base + DPP_CLRBAR_3RD_CLR, 0x000003FF, 30, 0);  /* Blue */
#endif
}
