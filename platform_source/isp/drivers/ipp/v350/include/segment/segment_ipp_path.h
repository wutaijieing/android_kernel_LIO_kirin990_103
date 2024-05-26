/*******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_ipp_path.h
 * Description:
 *
 * Date        2020-04-16
 ******************************************************************/

#ifndef _ORB_CS_H
#define _ORB_CS_H

#include "ipp.h"
#include "segment_orb_enh.h"
#include "segment_arfeature.h"
#include "segment_mc.h"
#include "segment_matcher.h"
#include "segment_hiof.h"

struct req_arf_t {
	struct ipp_stream_t	streams[ARFEATURE_TOTAL_MODE_MAX][ARFEATURE_STREAM_MAX];
	arfeature_reg_cfg_t	reg_cfg[ARFEATURE_TOTAL_MODE_MAX];
	unsigned int    arfeature_stat_buff[STAT_BUFF_MAX];
	struct msg_enh_req_t	req_enh;
};

struct msg_req_ipp_path_t {
	unsigned int    frame_num;
	unsigned int    mode_cnt;
	unsigned int    secure_flag;
	unsigned int    work_mode;
	unsigned int    arfeature_rate_value;
	enum work_module_e    work_module;
	enum work_frame_e work_frame; // cur ref
	struct req_arf_t    req_arf_cur;
	struct req_arf_t    req_arf_ref;
	struct msg_req_matcher_t req_matcher;
	struct msg_req_mc_request_t req_mc;
	struct msg_req_hiof_request_t req_hiof;
};

int ipp_path_request_handler(struct msg_req_ipp_path_t *ipp_path_request);

#endif
