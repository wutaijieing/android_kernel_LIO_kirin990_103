// *******************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    config_table_orb_enh.h
// Description:
//
// Version      1.0
// Date         2019-03-11
// *******************************************************

#ifndef __CONFIG_TABLE_ORB_ENH_H__
#define __CONFIG_TABLE_ORB_ENH_H__

struct orb_enh_ctrl_cfg_t {
	unsigned int to_use;

	unsigned int enh_en;
	unsigned int gsblur_en;
};

struct orb_enh_image_size_t {
	unsigned int to_use;

	unsigned int width;
	unsigned int height;
};

struct orb_enh_cal_hist_cfg_t {
	unsigned int to_use;

	unsigned int blknumy;
	unsigned int blknumx;
	unsigned int blk_ysize;
	unsigned int blk_xsize;
	unsigned int edgeex_y;
	unsigned int edgeex_x;
	unsigned int climit;
};

struct orb_enh_adjust_hist_cfg_t {
	unsigned int to_use;

	unsigned int lutscale;
};

struct orb_enh_adjust_image_cfg_t {
	unsigned int to_use;

	unsigned int inv_ysize;
	unsigned int inv_xsize;
};

struct orb_enh_gsblur_coef_cfg_t {
	unsigned int to_use;

	unsigned int coeff_gauss_0;
	unsigned int coeff_gauss_1;
	unsigned int coeff_gauss_2;
	unsigned int coeff_gauss_3;
};

struct cfg_tab_orb_enh_t {
	unsigned int to_use;

	struct orb_enh_ctrl_cfg_t        ctrl_cfg;
	struct orb_enh_adjust_hist_cfg_t adjust_hist_cfg;
	struct orb_enh_gsblur_coef_cfg_t gsblur_coef_cfg;
	struct orb_enh_image_size_t       image_size;
	struct orb_enh_cal_hist_cfg_t     cal_hist_cfg;
	struct orb_enh_adjust_image_cfg_t adjust_image_cfg;
};

#endif /*__CONFIG_TABLE_ORB_ENH_H__*/
/* ********************************* END ********************************* */
