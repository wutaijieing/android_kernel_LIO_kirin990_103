// ********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_gf.h
// Description:
//
// Version      1.0
// Date         2019-05-16 19:21:22
// ********************************************************

#ifndef __GF_TABLE_GF_CS_H__
#define __GF_TABLE_GF_CS_H__

struct gf_size_cfg_t {
	unsigned int to_use;

	unsigned int width;
	unsigned int height;
};

struct gf_mode_cfg_t {
	unsigned int to_use;

	unsigned int mode_in;
	unsigned int mode_out;
};

struct gf_crop_cfg_t {
	unsigned int to_use;

	unsigned int crop_right;
	unsigned int crop_left;
	unsigned int crop_bottom;
	unsigned int crop_top;
};

struct gf_filter_coeff_cfg_t {
	unsigned int to_use;

	unsigned int radius;
	unsigned int filter_val;
};

struct gf_config_table_t {
	unsigned int to_use;

	struct gf_size_cfg_t               size_cfg;
	struct gf_mode_cfg_t            mode_cfg;
	struct gf_crop_cfg_t              crop_cfg;
	struct gf_filter_coeff_cfg_t   coeff_cfg;
};
#endif /*__CFG_TABLE_GF_CS_H__*/

/* ********************************* END ********************************* */

