// ******************************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_vbk.h
// Description:
//
// Version      1.0
// Date         2018-06-19
// ******************************************************************

#ifndef __CFG_TABLE_VBK_CS_H__
#define __CFG_TABLE_VBK_CS_H__

#define SIGMA2ALPHA_NUM 256

struct vbk_ctrl_cfg_t {
	unsigned int to_use;

	unsigned int  op_mode;
	unsigned int  y_ds16_gauss_en;
	unsigned int  uv_ds16_gauss_en;
	unsigned int  sigma_gauss_en;
	unsigned int  sigma2alpha_en;
	unsigned int  in_width_ds4;
	unsigned int  in_height_ds4;
};

struct vbk_y_sigma_gauss_coeff_t {
	unsigned int to_use;

	unsigned int g00;
	unsigned int g01;
	unsigned int g02;
	unsigned int g03;
	unsigned int g04;
	unsigned int inv;
};

struct vbk_uv_gauss_coeff_t {
	unsigned int to_use;

	unsigned int g00;
	unsigned int g01;
	unsigned int g02;
	unsigned int inv;
};

struct vbk_addnoise_t {
	unsigned int to_use;

	unsigned int addnoise_th;
};

struct vbk_sigma2alpha_t {
	unsigned int to_use;

	unsigned int sigma2alpha[SIGMA2ALPHA_NUM];
};

struct vbk_foremask_cfg_t {
	unsigned int to_use;

	unsigned int foremask_coeff;
	unsigned int foremask_th;
	unsigned int foremask_weighted_filter_en;
	unsigned int foremask_dilation_radius;
};

struct vbk_crop_cfg_t {
	unsigned int to_use;

	unsigned int crop_right;
	unsigned int crop_left;
	unsigned int crop_bottom;
	unsigned int crop_top;
};

struct vbk_config_table_t {
	unsigned int to_use;

	struct vbk_ctrl_cfg_t vbk_ctrl;
	struct vbk_y_sigma_gauss_coeff_t yds16_gauss_coeff;
	struct vbk_y_sigma_gauss_coeff_t sigma_gauss_coeff;
	struct vbk_uv_gauss_coeff_t  uvds16_gauss_coeff;
	struct vbk_addnoise_t addnoise;
	struct vbk_sigma2alpha_t sigma2alpha;
	struct vbk_foremask_cfg_t  foremask_cfg;
	struct vbk_crop_cfg_t  y_crop_cfg;
	struct vbk_crop_cfg_t  uv_crop_cfg;
	struct vbk_crop_cfg_t  tf_crop_cfg;
};

#endif/*__CFG_TABLE_VBK_CS_H__*/

