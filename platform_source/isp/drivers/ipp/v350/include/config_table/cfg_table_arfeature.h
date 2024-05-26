// ********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_arfeature.h
// Description:
//
// Date         2020-04-10
// ********************************************************
#ifndef __CFG_TABLE_ARFEATURE_CS_H__
#define __CFG_TABLE_ARFEATURE_CS_H__

#include "segment_arfeature.h"

#define ARFEATURE_MAX_BLK_NUM     (22*17)
#define MAX_STRIPE_PER_MODE (1)

struct ipp_arfeature_stripe_info_t {
	unsigned int        stripe_cnt;
	unsigned int        stripe_height;
	unsigned int        stride;
	unsigned int        overlap_left[MAX_STRIPE_PER_MODE];
	unsigned int        overlap_right[MAX_STRIPE_PER_MODE];
	unsigned int        stripe_width[MAX_STRIPE_PER_MODE];
	unsigned int        stripe_start_point[MAX_STRIPE_PER_MODE];
	unsigned int        stripe_end_point[MAX_STRIPE_PER_MODE];
};

struct arfeature_top_cfg_t {
	unsigned int       to_use;

	unsigned int mode;
	unsigned int orient_en;
	unsigned int layer;
	unsigned int iter_num;
	unsigned int first_detect;
};

struct arfeature_img_size_t {
	unsigned int       to_use;
	unsigned int img_width;
	unsigned int img_height;
};

struct arfeature_blk_cfg_t {
	unsigned int       to_use;
	unsigned int blk_h_num;
	unsigned int blk_v_num;
	unsigned int blk_h_size_inv;
	unsigned int blk_v_size_inv;
};

struct arfeature_detect_num_lmt_t {
	unsigned int       to_use;
	unsigned int max_kpnum;
	unsigned int cur_kpnum;
};

struct arfeature_sigma_coeff_t {
	unsigned int       to_use;
	unsigned int sigma_ori;
	unsigned int sigma_des;
};

struct arfeature_gauss_coeff_t {
	unsigned int       to_use;

	unsigned int gauss_org_0;
	unsigned int gauss_org_1;
	unsigned int gauss_org_2;
	unsigned int gauss_org_3;

	unsigned int gsblur_1st_0;
	unsigned int gsblur_1st_1;
	unsigned int gsblur_1st_2;

	unsigned int gauss_2nd_0;
	unsigned int gauss_2nd_1;
	unsigned int gauss_2nd_2;
	unsigned int gauss_2nd_3;
};

struct arfeature_dog_edge_thd_t {
	unsigned int       to_use;
	unsigned int edge_threshold;
	unsigned int dog_threshold;
};

struct arfeature_mag_thd_t {
	unsigned int       to_use;

	unsigned int mag_threshold;
};

struct arfeature_kpt_lmt_t {
	unsigned int       to_use;

	unsigned int grid_max_kpnum[ARFEATURE_MAX_BLK_NUM];
	unsigned int grid_dog_threshold[ARFEATURE_MAX_BLK_NUM];
};

struct arfeature_cvdr_rd_cfg_base_t {
	unsigned int vprd_pixel_format;
	unsigned int vprd_pixel_expansion;
	unsigned int vprd_allocated_du;
	unsigned int vprd_last_page;
};

struct arfeature_cvdr_rd_lwg_t {
	unsigned int vprd_line_size;
	unsigned int vprd_horizontal_blanking;
};

struct arfeature_pre_cvdr_rd_fhg_t {
	unsigned int pre_vprd_frame_size;
	unsigned int pre_vprd_vertical_blanking;
};

struct arfeature_cvdr_rd_axi_fs_t {
	unsigned int vprd_axi_frame_start;
};

struct arfeature_cvdr_rd_axi_line_t {
	unsigned int vprd_line_stride;
	unsigned int vprd_line_wrap;
};

struct arfeature_cvdr_wr_cfg_base_t {
	unsigned int vpwr_pixel_format;
	unsigned int vpwr_pixel_expansion;
	unsigned int vpwr_last_page;
};

struct arfeature_pre_cvdr_wr_axi_fs_t {
	unsigned int pre_vpwr_address_frame_start;
};

struct arfeature_cvdr_wr_axi_line_t {
	unsigned int vpwr_line_stride;
	unsigned int vpwr_line_start_wstrb;
	unsigned int vpwr_line_wrap;
};

struct arfeature_cvdr_rd_cfg_t {
	struct arfeature_cvdr_rd_cfg_base_t rd_base_cfg;
	struct arfeature_cvdr_rd_lwg_t      rd_lwg;
	struct arfeature_pre_cvdr_rd_fhg_t      pre_rd_fhg;
	struct arfeature_cvdr_rd_axi_fs_t   rd_axi_fs;
	struct arfeature_cvdr_rd_axi_line_t rd_axi_line;
};

struct arfeature_cvdr_wr_cfg_t {
	struct arfeature_cvdr_wr_cfg_base_t wr_base_cfg;
	struct arfeature_pre_cvdr_wr_axi_fs_t   pre_wr_axi_fs;
	struct arfeature_cvdr_wr_axi_line_t wr_axi_line;
};

struct arfeature_cvdr_cfg_t {
	unsigned int to_use;

	struct arfeature_cvdr_rd_cfg_t  rd_cfg;
	struct arfeature_cvdr_wr_cfg_t  wr_cfg;
};

struct cfg_tab_arfeature_t {
	struct arfeature_top_cfg_t  top_cfg;
	struct arfeature_img_size_t  size_cfg;
	struct arfeature_blk_cfg_t  blk_cfg;
	struct arfeature_detect_num_lmt_t  detect_num_lmt_cfg;
	struct arfeature_sigma_coeff_t  sigma_coeff_cfg;
	struct arfeature_gauss_coeff_t  gauss_coeff_cfg;
	struct arfeature_dog_edge_thd_t dog_edge_thd_cfg;
	struct arfeature_mag_thd_t  mag_thd_cfg;
	struct arfeature_kpt_lmt_t  kpt_lmt_cfg;
	struct arfeature_cvdr_cfg_t  cvdr_cfg;
};


struct seg_arfeature_cfg_t {
	struct cfg_tab_arfeature_t  arfeature_cfg[ARFEATURE_TOTAL_MODE_MAX];
	struct cfg_tab_cvdr_t arfeature_cvdr[ARFEATURE_TOTAL_MODE_MAX];
};

#endif // __CFG_TABLE_ARFEATURE_CS_H__
