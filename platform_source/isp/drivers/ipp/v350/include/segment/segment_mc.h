/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_mc.h
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/


#ifndef __MC_CS_H__
#define __MC_CS_H__

#include "ipp.h"
#include "cfg_table_mc.h"

struct buff_cfg_t {
	unsigned int ref_first_page;
	unsigned int cur_first_page;
	unsigned int inlier_num_buff;
	unsigned int h_matrix_buff;
	unsigned int coordinate_pairs_buff;
};

struct ctrl_mode_cfg_t {
	unsigned int svd_iterations;
	unsigned int max_iterations;
	unsigned int flag_10_11;
	unsigned int push_inliers_en;
	unsigned int h_cal_en;
	unsigned int cfg_mode;
	unsigned int mc_en;
};

struct inlier_thld_t {
	unsigned int       inlier_th;
};

struct kpts_num_t {
	unsigned int matched_kpts;
};

struct index_pairs_t {
	unsigned int cfg_best_dist[MC_KPT_NUM_MAX];
	unsigned int cfg_ref_index[MC_KPT_NUM_MAX];
	unsigned int cfg_cur_index[MC_KPT_NUM_MAX];
};

struct coord_pairs_t {
	unsigned int cur_cfg_coordinate_y[MC_KPT_NUM_MAX];
	unsigned int cur_cfg_coordinate_x[MC_KPT_NUM_MAX];
	unsigned int ref_cfg_coordinate_y[MC_KPT_NUM_MAX];
	unsigned int ref_cfg_coordinate_x[MC_KPT_NUM_MAX];
};

struct mc_hw_config_t {
	struct ctrl_mode_cfg_t     mc_ctrl_cfg;
	struct inlier_thld_t  mc_inlier_thld_cfg;
	struct kpts_num_t  mc_kpt_num_cfg;
	struct index_pairs_t mc_index_pairs_cfg;
	struct coord_pairs_t mc_coord_pairs_cfg;
};

struct msg_req_mc_request_t {
	unsigned int        frame_number;
	struct buff_cfg_t           mc_buff_cfg;
	struct mc_hw_config_t mc_hw_config;
	unsigned int mc_rate_value;
};

void mc_dump_request(struct msg_req_mc_request_t *req);

int  mc_request_handler(
	struct msg_req_mc_request_t *mc_request);

#endif

