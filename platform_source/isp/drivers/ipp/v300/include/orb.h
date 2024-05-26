

#ifndef _ORB_CS_H
#define _ORB_CS_H

#include "ipp.h"
#include "orb_common.h"
#include "orb_enh.h"
#include "matcher.h"
#include "mc.h"

#define ORB_GRID_SCORE_MAX 748
#define ORB_GRID_SOCRE_REG_SIZE  (ALIGN_UP((4*ORB_GRID_SCORE_MAX), CVDR_ALIGN_BYTES))
#define ORB_GRID_KPT_NMS_REG_SIZE  (ALIGN_UP((4), CVDR_ALIGN_BYTES))

enum orb_buf_usage_e {
	BI_ORB_Y = 0,
	BO_ORB_Y,
	ORB_STREAM_MAX,
};

enum orb_stat_buff_e {
	DESC_BUFF,
	GRID_SCORE_BUFF,
	KPNUM_NMS_BUFF,
	KPT_NUM_BUFF,
	TOTAL_KPT_BUFF,
	BUFF_MAX,
};

struct orb_req_ctrl_cfg_t {
	unsigned int   pyramid_en;
	unsigned int   gsblur_en;
	unsigned int   fast_en;
	unsigned int   nms_en;
	unsigned int   orient_en;
	unsigned int   brief_en;
	unsigned int   freak_en;
	unsigned int   gridstat_en;
	unsigned int   undistort_en;
	unsigned int   first_stripe;
	unsigned int   first_layer;
};

struct orb_req_size_cfg_t {
	unsigned int           width;              // IMG_WIDTH-1
	unsigned int           height;            // IMG_HEIGHT-1
};

struct orb_req_pyramid_scaler_cfg_t {
	unsigned int  scl_inc;
	unsigned int  scl_vbottom;
	unsigned int  scl_vtop;
	unsigned int  scl_hright;
	unsigned int  scl_hleft;
};

struct orb_req_gsblur_coef_cfg_t {
	unsigned int   coeff_gauss_0;
	unsigned int   coeff_gauss_1;
	unsigned int   coeff_gauss_2;
	unsigned int   coeff_gauss_3;
};

struct orb_req_fast_coef_cfg_t {
	unsigned int   ini_th;
	unsigned int   min_th;
};

struct orb_req_nms_win_cfg_t {
	unsigned int   nmscell_h;
	unsigned int   nmscell_v;
};

struct orb_req_block_size_cfg_inv_t {
	unsigned int blk_h_size_inv;
	unsigned int blk_v_size_inv;
};

struct orb_req_octree_cfg_t {
	unsigned int max_kpnum;
	unsigned int grid_max_kpnum;
	unsigned int inc_level;
	unsigned int score_th[ORB_SCORE_TH_RANGE];
	unsigned int flag_10_11;
	struct orb_req_block_size_cfg_inv_t blk_size_inv;
};

struct orb_req_undistort_cfg_t {
	unsigned int  cx;
	unsigned int  cy;
	unsigned int  fx;
	unsigned int  fy;
	unsigned int  invfx;
	unsigned int  invfy;
	unsigned int  k1;
	unsigned int  k2;
	unsigned int  k3;
	unsigned int  p1;
	unsigned int  p2;
};

struct orb_req_pattern_cfg_t {
	unsigned int  pattern_x0[ORB_PATTERN_RANGE];
	unsigned int  pattern_y0[ORB_PATTERN_RANGE];
	unsigned int  pattern_x1[ORB_PATTERN_RANGE];
	unsigned int  pattern_y1[ORB_PATTERN_RANGE];
};

struct orb_req_block_size_t {
	unsigned int blk_h_size;
	unsigned int blk_v_size;
};

struct orb_req_block_num_t {
	unsigned int blk_h_num;
	unsigned int blk_v_num;
};

struct orb_req_gridstat_cfg_t {
	struct orb_req_block_num_t  block_num;
	struct orb_req_block_size_t block_size;
};

struct orb_reg_cfg_t {
	struct orb_req_ctrl_cfg_t                       ctrl;
	struct orb_req_size_cfg_t                      size_cfg;
	struct orb_req_pyramid_scaler_cfg_t   pyramid_scaler_cfg;
	struct orb_req_gsblur_coef_cfg_t         gsblur_coef_cfg;
	struct orb_req_fast_coef_cfg_t      fast_coef;
	struct orb_req_nms_win_cfg_t       nms_win;
	struct orb_req_gridstat_cfg_t         gridstat_cfg;
	struct orb_req_octree_cfg_t           octree_cfg;
	struct orb_req_undistort_cfg_t       undistort_cfg;
	struct orb_req_pattern_cfg_t          pattern_cfg;
};

struct req_orb_t {
	struct ipp_stream_t   streams[ORB_LAYER_MAX][ORB_STREAM_MAX];
	unsigned int     orb_stat_buff[BUFF_MAX];
	struct orb_reg_cfg_t  reg_cfg[ORB_LAYER_MAX];
	struct msg_enh_req_t	req_enh;
};

enum work_module_e {
	ENH_ONLY = 0,
	ORB_ONLY = 1,
	MATCHER_ONLY = 2,
	MC_ONLY = 3,
	ENH_ORB = 4,
	ORB_MATCHER = 5,
	ENH_ORB_MATCHER = 6,
	MATCHER_MC = 7,
	ORB_MATCHER_MC = 8,
	ENH_ORB_MATCHER_MC = 9,
	MODULE_MAX,
};

enum work_frame_e {
	CUR_ONLY = 0,
	CUR_REF = 1,
};

struct msg_req_orb_request_t {
	unsigned int    frame_num;
	unsigned int    orb_pyramid_layer;
	unsigned int    secure_flag;
	unsigned int    work_mode;// 0 : single stripe 1:multi
	unsigned int    orb_rate_value;
	enum work_module_e    work_module;
	enum work_frame_e work_frame;
	struct req_orb_t    req_orb_cur;
	struct req_orb_t    req_orb_ref;
	struct msg_req_matcher_t req_matcher;
	struct msg_req_mc_request_t req_mc;
};

int orb_request_handler(struct msg_req_orb_request_t *orb_request);

#endif
