/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_matcher.h
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/

#ifndef _SEGMENT_MATCHER_CS_H
#define _SEGMENT_MATCHER_CS_H

#include "ipp.h"
#include "segment_common.h"
#include "cfg_table_compare.h"
#include "cfg_table_reorder.h"

enum rdr_buf_usage_e {
	BI_RDR_FP = 0,
	BO_RDR_FP_BLOCK,
	RDR_STREAM_MAX,
};

struct reorder_req_ctrl_cfg_t {
	unsigned int   reorder_en;
	unsigned int   total_kpt;
};

struct reorder_req_block_cfg_t {
	unsigned int   blk_v_num;
	unsigned int   blk_h_num;
	unsigned int   blk_num;
};

struct req_rdr_t {
	struct ipp_stream_t  streams[MATCHER_LAYER_MAX][RDR_STREAM_MAX];
	struct reorder_req_ctrl_cfg_t ctrl_cfg[MATCHER_LAYER_MAX];
	struct reorder_req_block_cfg_t block_cfg[MATCHER_LAYER_MAX];
	unsigned int reorder_kpt_num[MATCHER_LAYER_MAX][RDR_KPT_NUM_RANGE_DS4];
};

enum cmp_buf_usage_e {
	BI_CMP_REF_FP = 0,
	BI_CMP_CUR_FP,
	BO_CMP_MATCHED_INDEX_OUT,
	BO_CMP_MATCHED_DIST_OUT,
	CMP_STREAM_MAX,
};

struct compare_req_ctrl_cfg_t {
	unsigned int  compare_en;
};

struct compare_req_block_cfg_t {
	unsigned int  blk_v_num;
	unsigned int  blk_h_num;
	unsigned int  blk_num;
};

struct compare_req_search_cfg_t {
	unsigned int  v_radius;
	unsigned int  h_radius;
	unsigned int  dis_ratio;
	unsigned int  dis_threshold;
};

struct compare_req_offset_cfg_t {
	signed int  cenh_offset;
	signed int  cenv_offset;
};

struct compare_req_total_kptnum_cfg_t {
	unsigned int total_kptnum;
};

struct compare_req_stat_cfg_t {
	unsigned int  stat_en;
	unsigned int  max3_ratio;
	unsigned int  bin_factor;
	unsigned int  bin_num;
};

struct compare_req_kptnum_cfg_t {
	unsigned int   compare_ref_kpt_num[CMP_KPT_NUM_RANGE_DS4];
	unsigned int   compare_cur_kpt_num[CMP_KPT_NUM_RANGE_DS4];
};

struct req_cmp_t {
	struct ipp_stream_t             streams[MATCHER_LAYER_MAX][CMP_STREAM_MAX];
	struct compare_req_ctrl_cfg_t   ctrl_cfg[MATCHER_LAYER_MAX];
	struct compare_req_block_cfg_t  block_cfg[MATCHER_LAYER_MAX];
	struct compare_req_search_cfg_t search_cfg[MATCHER_LAYER_MAX];
	struct compare_req_offset_cfg_t offset_cfg[MATCHER_LAYER_MAX];
	struct compare_req_stat_cfg_t   stat_cfg[MATCHER_LAYER_MAX];
	struct compare_req_total_kptnum_cfg_t   total_kptnum_cfg[MATCHER_LAYER_MAX];
	struct compare_req_kptnum_cfg_t kptnum_cfg[MATCHER_LAYER_MAX];
};

struct msg_req_matcher_t {
	unsigned int  frame_number;
	unsigned int  rdr_pyramid_layer;
	unsigned int  cmp_pyramid_layer;
	unsigned int  work_mode;
	struct req_rdr_t       req_rdr;
	struct req_cmp_t     req_cmp;
	unsigned int  matcher_rate_value; // 0=TURBO,1=NORMAL,2=SVS, 3=LOW
};

void rdr_request_dump(struct msg_req_matcher_t *req_matcher);
void cmp_request_dump(struct msg_req_matcher_t *req_matcher);


int matcher_request_handler(struct msg_req_matcher_t *matcher_request);
int matcher_eof_handler(struct msg_req_matcher_t *matcher_request);

#endif
