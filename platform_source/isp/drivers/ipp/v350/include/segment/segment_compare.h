/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_compare.h
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/


#ifndef _COMPARE_CS_H
#define _COMPARE_CS_H

#include "ipp.h"
#include "segment_matcher.h"

enum compare_buf_e {
	BI_COMPARE_DESC_REF,
	BI_COMPARE_DESC_CUR,
	COMPARE_STREAM_MAX,
};

struct compare_reg_cfg_t {
	struct compare_req_ctrl_cfg_t            compare_ctrl_cfg;
	struct compare_req_block_cfg_t         compare_block_cfg;
	struct compare_req_search_cfg_t      compare_search_cfg;
	struct compare_req_offset_cfg_t        compare_offset_cfg;
	struct compare_req_stat_cfg_t           compare_stat_cfg;
	struct compare_req_total_kptnum_cfg_t   compare_total_kptnum_cfg;
	struct compare_req_kptnum_cfg_t      compare_kptnum_cfg;
};

struct msg_req_compare_request_t {
	unsigned int                frame_number;
	struct ipp_stream_t               streams[COMPARE_STREAM_MAX];
	unsigned  int                compare_index;
	unsigned  int                compare_matched_kpt;
	unsigned int                 compare_matched_dist; // add in 350
	struct compare_reg_cfg_t reg_cfg;
	unsigned int                cmp_rate_value;
};

int compare_request_handler(
	struct msg_req_compare_request_t *compare_request);
#endif
