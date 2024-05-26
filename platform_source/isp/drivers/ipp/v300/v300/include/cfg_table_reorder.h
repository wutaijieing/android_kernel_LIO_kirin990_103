// ***************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_reorder.h
// Description:
//
// Version      1.0
// Date         2017-09-25
// ***************************************************
#ifndef __CFG_TABLE_REORDER_CS_H__
#define __CFG_TABLE_REORDER_CS_H__

#include "orb_common.h"

#define REORDER_KPT_NUM          187

enum ipp_descriptor_type_e {
	IPP_FREAK_MODE  = 0,
	IPP_BRIEF_MODE  = 1,
};

struct reorder_ctrl_cfg_t {
	unsigned int to_use;

	unsigned int  reorder_en;
	unsigned int  descriptor_type;
};

struct reorder_block_cfg_t {
	unsigned int to_use;

	unsigned int  blk_v_num;
	unsigned int  blk_h_num;
	unsigned int  blk_num;
};

struct reorder_total_kpt_num_t {
	unsigned int to_use;

	unsigned int total_kpts;
};

struct reorder_prefetch_cfg_t {
	unsigned int to_use;

	unsigned int  prefetch_enable;
	unsigned int  first_32k_page;
};

struct reorder_kptnum_cfg_t {
	unsigned int to_use;

	unsigned int reorder_kpt_num[REORDER_KPT_NUM];
};

struct cfg_tab_reorder_t {
	struct reorder_ctrl_cfg_t     reorder_ctrl_cfg;
	struct reorder_block_cfg_t    reorder_block_cfg;
	struct reorder_total_kpt_num_t reorder_total_kpt_num;
	struct reorder_prefetch_cfg_t reorder_prefetch_cfg;
	struct reorder_kptnum_cfg_t   reorder_kptnum_cfg;

	unsigned int           rdr_cascade_en;
	unsigned int           rdr_kpt_num_addr;//address in cmdlst_buffer
	unsigned int           rdr_total_num_addr;//address in cmdlst_buffer
};

#endif/*__CFG_TABLE_REORDER_CS_H__*/
