// ********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_compare.h
// Description:
//
// Version      1.0
// Date         2017-09-25
// ********************************************************
#ifndef __CFG_TABLE_COMPARE_CS_H__
#define __CFG_TABLE_COMPARE_CS_H__

#include "orb_common.h"

#define COMPARE_KPT_NUM          187
#define CMP_INDEX_NUM_MAX     3200

struct compare_ctrl_cfg_t {
	unsigned int to_use;

	unsigned int  compare_en;
	unsigned int  descriptor_type;
};

struct compare_block_cfg_t {
	unsigned int to_use;

	unsigned int  blk_v_num;
	unsigned int  blk_h_num;
	unsigned int  blk_num;
};

struct compare_search_cfg_t {
	unsigned int to_use;

	unsigned int  v_radius;
	unsigned int  h_radius;
	unsigned int  dis_ratio;
	unsigned int  dis_threshold;
};

struct compare_stat_cfg_t {
	unsigned int to_use;

	unsigned int  stat_en;
	unsigned int  max3_ratio;
	unsigned int  bin_factor;
	unsigned int  bin_num;
};

struct compare_prefetch_cfg_t {
	unsigned int to_use;

	unsigned int  prefetch_enable;
	unsigned int  first_32k_page;
};

struct compare_kptnum_cfg_t {
	unsigned int to_use;

	unsigned int compare_ref_kpt_num[COMPARE_KPT_NUM];
	unsigned int compare_cur_kpt_num[COMPARE_KPT_NUM];
};

struct compare_total_kptnum_cfg_t {
	unsigned int to_use;

	unsigned int total_kptnum;
};

struct compare_offset_cfg_t {
	unsigned int to_use;

	signed int  cenh_offset;
	signed int  cenv_offset;
};

struct cfg_tab_compare_t {
	struct compare_ctrl_cfg_t     compare_ctrl_cfg;
	struct compare_block_cfg_t    compare_block_cfg;
	struct compare_search_cfg_t   compare_search_cfg;
	struct compare_offset_cfg_t   compare_offset_cfg;
	struct compare_stat_cfg_t     compare_stat_cfg;
	struct compare_total_kptnum_cfg_t  compare_total_kptnum_cfg;
	struct compare_prefetch_cfg_t compare_prefetch_cfg;
	struct compare_kptnum_cfg_t   compare_kptnum_cfg;

	unsigned int           cmp_cascade_en;
	unsigned int           cmp_total_kpt_addr;//address in cmdlst_buffer
	unsigned int           cmp_ref_kpt_addr;//address in cmdlst_buffer
	unsigned int           cmp_cur_kpt_addr;//address in cmdlst_buffer
};

#endif/*__CFG_TABLE_COMPARE_CS_H__*/
