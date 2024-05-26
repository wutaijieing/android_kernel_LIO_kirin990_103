

#ifndef _REORDER_CS_H
#define _REORDER_CS_H

#include "ipp.h"
#include "orb_common.h"
#include "matcher.h"

struct reorder_req_reg_cfg_t {
	struct reorder_req_ctrl_cfg_t     reorder_ctrl_cfg;
	struct reorder_req_block_cfg_t    reorder_block_cfg;
	unsigned int           reorder_kpt_num[MATCHER_KPT_NUM];

	unsigned int           reorder_kpt_addr; // address in cmdlst_buffer
};

struct msg_req_reorder_request_t {
	unsigned int      frame_number;
	unsigned int      reorder_first_32k_page;
	struct ipp_stream_t     streams[RDR_STREAM_MAX];
	struct reorder_req_reg_cfg_t reg_cfg;
	unsigned int rdr_rate_value;
};

int reorder_request_handler(
	struct msg_req_reorder_request_t *reorder_request);

#endif
