#ifndef _MATCHER_CS_H
#define _MATCHER_CS_H

#include "ipp.h"
#include "orb_common.h"
#include "cfg_table_orb.h"

#define MATCHER_KPT_NUM          187

enum rdr_buf_usage_e {
	BI_RDR_FP = 0,
	BO_RDR_FP_BLOCK,
	RDR_STREAM_MAX,
};

struct reorder_req_ctrl_cfg_t {
	unsigned int   reorder_en;
	unsigned int   descriptor_type;
	unsigned int   total_kpt;
};

struct reorder_req_block_cfg_t {
	unsigned int   blk_v_num;
	unsigned int   blk_h_num;
	unsigned int   blk_num;
};

struct req_rdr_t {
	struct ipp_stream_t  streams[FRAME_NUM_MAX][RDR_STREAM_MAX];
	struct reorder_req_ctrl_cfg_t ctrl_cfg[FRAME_NUM_MAX];
	struct reorder_req_block_cfg_t block_cfg[FRAME_NUM_MAX];
	unsigned int reorder_kpt_num[FRAME_NUM_MAX][MATCHER_KPT_NUM];
};

enum cmp_buf_usage_e {
	BI_CMP_REF_FP = 0,
	BI_CMP_CUR_FP,
	BO_CMP_MATCHED_OUT,
	CMP_STREAM_MAX,
};

struct compare_req_ctrl_cfg_t {
	unsigned int  compare_en;
	unsigned int  descriptor_type;
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
	unsigned int   compare_ref_kpt_num[MATCHER_KPT_NUM];
	unsigned int   compare_cur_kpt_num[MATCHER_KPT_NUM];
};

struct req_cmp_t {
	struct ipp_stream_t             streams[FRAME_NUM_MAX][CMP_STREAM_MAX];
	struct compare_req_ctrl_cfg_t   ctrl_cfg[FRAME_NUM_MAX];
	struct compare_req_block_cfg_t  block_cfg[FRAME_NUM_MAX];
	struct compare_req_search_cfg_t search_cfg[FRAME_NUM_MAX];
	struct compare_req_offset_cfg_t offset_cfg[FRAME_NUM_MAX];
	struct compare_req_stat_cfg_t   stat_cfg[FRAME_NUM_MAX];
	struct compare_req_total_kptnum_cfg_t   total_kptnum_cfg[FRAME_NUM_MAX];
	struct compare_req_kptnum_cfg_t kptnum_cfg[FRAME_NUM_MAX];
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

#endif
