// ******************************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_orb.h
// Description:
//
// Version      1.0
// Date         2017-09-25
// ******************************************************************/
#ifndef __CFG_TABLE_ORB_CS_H__
#define __CFG_TABLE_ORB_CS_H__

#include "orb_common.h"
#include "cfg_table_orb_enh.h"
#include "config_table_cvdr.h"

#define MAX_STRIPE_PER_LAYER  2

enum frame_flag_e {
    FRAME_CUR = 0,
    FRMAE_REF  = 1,
    MAX_FRAME_FLAG,
};

struct ipp_orb_stripe_info_t {
	unsigned int        stripe_cnt;
	unsigned int        stripe_height;
	unsigned int        stride;
	unsigned int        overlap_left[MAX_STRIPE_PER_LAYER];
	unsigned int        overlap_right[MAX_STRIPE_PER_LAYER];
	unsigned int        stripe_width[MAX_STRIPE_PER_LAYER];
	unsigned int        stripe_start_point[MAX_STRIPE_PER_LAYER];
	unsigned int        stripe_end_point[MAX_STRIPE_PER_LAYER];
};

struct orb_ctrl_cfg_t {
	unsigned int to_use;

	unsigned int pyramid_en;
	unsigned int gsblur_en;
	unsigned int fast_en;
	unsigned int nms_en;
	unsigned int orient_en;
	unsigned int brief_en;
	unsigned int freak_en;
	unsigned int gridstat_en;
	unsigned int undistort_en;
	unsigned int first_stripe;
	unsigned int first_layer;
};

struct orb_stat_range_t {
	unsigned int stat_left;
	unsigned int stat_right;
};

struct orb_size_cfg_t {
	unsigned int to_use;

	unsigned int width;
	unsigned int height;
	struct orb_stat_range_t stat_range;
};

struct orb_pyramid_scaler_cfg_t {
	unsigned int to_use;

	unsigned int scl_inc;
	unsigned int scl_vbottom;
	unsigned int scl_vtop;
	unsigned int scl_hright;
	unsigned int scl_hleft;
};

struct orb_gsblur_coef_cfg_t {
	unsigned int to_use;

	unsigned int coeff_gauss_0;
	unsigned int coeff_gauss_1;
	unsigned int coeff_gauss_2;
	unsigned int coeff_gauss_3;
};

struct orb_fast_coef_t {
	unsigned int to_use;

	unsigned int ini_th;
	unsigned int min_th;
};

struct orb_nms_win_t {
	unsigned int to_use;

	unsigned int nmscell_h;
	unsigned int nmscell_v;
};

struct orb_block_num_t {
	unsigned int blk_h_num;
	unsigned int blk_v_num;
};

struct orb_block_size_t {
	unsigned int blk_h_size;
	unsigned int blk_v_size;
};

struct orb_gridstat_cfg_t {
	unsigned int to_use;

	struct orb_block_num_t  block_num;
	struct orb_block_size_t block_size;
};

struct orb_layer0_start_cfg_t {
	unsigned int start_blk_h_num;
	unsigned int start_blk_h_offset;
	unsigned int start_blk_v_num;
	unsigned int start_blk_v_offset;
};

struct block_size_cfg_inv_t {
	unsigned int blk_h_size_inv;
	unsigned int blk_v_size_inv;
};

struct orb_octree_cfg_t {
	unsigned int to_use;

	unsigned int max_kpnum;
	unsigned int grid_max_kpnum;
	unsigned int inc_level;
	unsigned int score_th[ORB_SCORE_TH_RANGE];
	unsigned int flag_10_11;
	struct orb_layer0_start_cfg_t layer0_start;
	struct block_size_cfg_inv_t blk_size_inv;
};

struct orb_undistort_cfg_t {
	unsigned int to_use;

	unsigned int cx;
	unsigned int cy;
	unsigned int fx;
	unsigned int fy;
	unsigned int invfx;
	unsigned int invfy;
	unsigned int k1;
	unsigned int k2;
	unsigned int k3;
	unsigned int p1;
	unsigned int p2;
};

struct orb_pattern_cfg_t {
	unsigned int pattern_x0[ORB_PATTERN_RANGE];
	unsigned int pattern_y0[ORB_PATTERN_RANGE];
	unsigned int pattern_x1[ORB_PATTERN_RANGE];
	unsigned int pattern_y1[ORB_PATTERN_RANGE];
};

struct orb_cvdr_rd_cfg_base_t {
	unsigned int vprd_pixel_format;
	unsigned int vprd_pixel_expansion;
	unsigned int vprd_allocated_du;
	unsigned int vprd_last_page;
};

struct orb_cvdr_rd_lwg_t {
	unsigned int vprd_line_size;
	unsigned int vprd_horizontal_blanking;
};

struct orb_pre_cvdr_rd_fhg_t {
	unsigned int pre_vprd_frame_size;
	unsigned int pre_vprd_vertical_blanking;
};

struct orb_cvdr_rd_axi_fs_t {
	unsigned int vprd_axi_frame_start;
};

struct orb_cvdr_rd_axi_line_t {
	unsigned int vprd_line_stride;
	unsigned int vprd_line_wrap;
};

struct orb_cvdr_wr_cfg_base_t {
	unsigned int vpwr_pixel_format;
	unsigned int vpwr_pixel_expansion;
	unsigned int vpwr_last_page;
};

struct orb_pre_cvdr_wr_axi_fs_t {
	unsigned int pre_vpwr_address_frame_start;
};

struct orb_cvdr_wr_axi_line_t {
	unsigned int vpwr_line_stride;
	unsigned int vpwr_line_start_wstrb;
	unsigned int vpwr_line_wrap;
};

struct orb_cvdr_rd_cfg_t {
	struct orb_cvdr_rd_cfg_base_t rd_base_cfg;
	struct orb_cvdr_rd_lwg_t      rd_lwg;
	struct orb_pre_cvdr_rd_fhg_t      pre_rd_fhg;
	struct orb_cvdr_rd_axi_fs_t   rd_axi_fs;
	struct orb_cvdr_rd_axi_line_t rd_axi_line;
};

struct orb_cvdr_wr_cfg_t {
	struct orb_cvdr_wr_cfg_base_t wr_base_cfg;
	struct orb_pre_cvdr_wr_axi_fs_t   pre_wr_axi_fs;
	struct orb_cvdr_wr_axi_line_t wr_axi_line;
};

struct orb_cvdr_cfg_t {
	unsigned int to_use;

	struct orb_cvdr_rd_cfg_t  rd_cfg;
	struct orb_cvdr_wr_cfg_t  wr_cfg;
};

struct cfg_tab_orb_t {
	unsigned int pattern_to_use;
	struct orb_ctrl_cfg_t  ctrl;
	struct orb_size_cfg_t  size_cfg;
	struct orb_pyramid_scaler_cfg_t pyramid_scaler_cfg;
	struct orb_gsblur_coef_cfg_t    gsblur_coef_cfg;
	struct orb_fast_coef_t   fast_coef;
	struct orb_nms_win_t    nms_win;
	struct orb_gridstat_cfg_t   gridstat_cfg;
	struct orb_octree_cfg_t      octree_cfg;
	struct orb_undistort_cfg_t  undistort_cfg;
	struct orb_pattern_cfg_t     pattern_cfg;
	struct orb_cvdr_cfg_t          cvdr_cfg;
};

struct orb_schedule_flag_t {
	unsigned int enh_en_flag;
	unsigned int matcher_en_flag;
	unsigned int mc_en_flag;
	unsigned int mc_token_num;
	unsigned int matcher_update_flag;
	unsigned int rdr_total_num_addr_cur;
	unsigned int rdr_kpt_num_addr_cur;
	unsigned int rdr_total_num_addr_ref;
	unsigned int rdr_kpt_num_addr_ref;
	unsigned int rdr_frame_height_addr_cur;
	unsigned int rdr_frame_height_addr_ref;
	unsigned int cmp_total_num_addr;
	unsigned int cmp_kpt_num_addr_cur;
	unsigned int cmp_kpt_num_addr_ref;
	unsigned int cmp_frame_height_addr;
	unsigned int mc_index_addr;
	unsigned int mc_matched_num_addr;
};

struct enh_orb_cfg_t {
	struct cfg_tab_orb_enh_t   enh_cur;
	struct cfg_tab_orb_t  orb_cur[ORB_LAYER_MAX];
	struct cfg_tab_cvdr_t enh_cvdr_cur;
	struct cfg_tab_cvdr_t orb_cvdr_cur[ORB_LAYER_MAX];
	
	struct cfg_tab_orb_enh_t   enh_ref;
	struct cfg_tab_orb_t  orb_ref[ORB_LAYER_MAX];
	struct cfg_tab_cvdr_t enh_cvdr_ref;
	struct cfg_tab_cvdr_t orb_cvdr_ref[ORB_LAYER_MAX];
	
	struct orb_schedule_flag_t schedule_flag;
};
#endif/*__CFG_TABLE_ORB_CS_H__*/
