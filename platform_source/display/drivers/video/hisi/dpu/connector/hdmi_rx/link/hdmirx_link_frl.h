/* Copyright (c) 2022, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HDMIRX_LINK_FRL_H
#define HDMIRX_LINK_FRL_H
#include "hdmirx_struct.h"
#include "hdmirx_common.h"

#define FRL_IRQ_MASK 0xff

typedef enum {
	HDMIRX_FRL_INT_SOURCE_FLT = 0x01,
	HDMIRX_FRL_INT_SINK_FLT = 0x02,
	HDMIRX_FRL_INT_TMDS_OK = 0x04,
	HDMIRX_FRL_INT_FRL_OK = 0x08,
	HDMIRX_FRL_INT_FLT_TIMEOUT = 0x10,
	HDMIRX_FRL_INT_FLT_READY_ERR = 0x20,
	HDMIRX_FRL_INT_SOURCE_POLL_TIMEOUT = 0x40,
	HDMIRX_FRL_INT_SOURCE_CLR_TIMEOUT = 0x80,
	HDMIRX_FRL_INT_RSCC = 0x102,
	HDMIRX_FRL_INT_FFE_LEVEL = 0x108,
	HDMIRX_FRL_INT_FLT_NO_RETRAIN = 0x110,
	HDMIRX_FRL_INT_RSED_UPDATE = 0x120,
	HDMIRX_FRL_INT_FLT_UPDATE = 0x140,
	HDMIRX_FRL_INT_FRL_START = 0x180,
	HDMIRX_FRL_INT_STATUS_UPDATE = 0x201,
	HDMIRX_FRL_INT_FRL_RATE = 0x202,
	HDMIRX_FRL_INT_SFIFO_OVER_FLOW = 0x204,
	HDMIRX_FRL_INT_RS_ENCORR_OVER = 0x208,
	HDMIRX_FRL_INT_RS_UNCORR_OVER = 0x210,
	HDMIRX_FRL_INT_LN3_NORM_DATA_ERR_EXC = 0x301,
	HDMIRX_FRL_INT_LN3_LTP_DATA_ERR_EXC = 0x302,
	HDMIRX_FRL_INT_LN2_NORM_DATA_ERR_EXC = 0x304,
	HDMIRX_FRL_INT_LN2_LTP_DATA_ERR_EXC = 0x308,
	HDMIRX_FRL_INT_LN1_NORM_DATA_ERR_EXC = 0x310,
	HDMIRX_FRL_INT_LN1_LTP_DATA_ERR_EXC = 0x320,
	HDMIRX_FRL_INT_LN0_NORM_DATA_ERR_EXC = 0x340,
	HDMIRX_FRL_INT_LN0_LTP_DATA_ERR_EXC = 0x380,
	HDMIRX_FRL_INT_BK_PREAMBLE_ERR = 0x401,
	HDMIRX_FRL_INT_AV_PREAMBLE_ERR = 0x402,
	HDMIRX_FRL_INT_DATA_ISLAND_ERR = 0x404,
	HDMIRX_FRL_INT_AV_GB_ERR = 0x408,
	HDMIRX_FRL_INT_BK_GB_ERR = 0x410,
	HDMIRX_FRL_INT_CTRL_PERIOD_ERR = 0x420,
	HDMIRX_FRL_INT_RC_ERR = 0x440,
	HDMIRX_FRL_INT_MAX
} hdmirx_frl_intr_type;

typedef enum {
	HDMIRX_FRL_TRAIN_MODE_MCU,
	HDMIRX_FRL_TRAIN_MODE_CPU,
	HDMIRX_FRL_TRAIN_MODE_MAX
} hdmirx_frl_train_mode;

typedef enum {
	FRL_STATE_IDEL,
	FRL_STATE_LTP_REQ,
	FRL_STATE_PATTEN_CHECK,
	FRL_STATE_GAP_CHECK,
	FRL_STATE_FRL_TRANS,
	FRL_STATE_DONE,
	FRL_STATE_MAX
} frl_state;

typedef enum ext_hdmirx_frl_rate {
	HDMIRX_FRL_RATE_TMDS,
	HDMIRX_FRL_RATE_FRL3L3G,
	HDMIRX_FRL_RATE_FRL3L6G,
	HDMIRX_FRL_RATE_FRL4L6G,
	HDMIRX_FRL_RATE_FRL4L8G,
	HDMIRX_FRL_RATE_FRL4L10G,
	HDMIRX_FRL_RATE_FRL4L12G,
	HDMIRX_FRL_RATE_MAX
} hdmirx_frl_rate;

typedef enum {
	FRL_LANE_0,
	FRL_LANE_1,
	FRL_LANE_2,
	FRL_LANE_3,
	FRL_LANE_MAX
} frl_lane;

typedef enum {
	HDMIRX_FRL_LTP_IDEL,
	HDMIRX_FRL_LTP_CHECKING,
	HDMIRX_FRL_LTP_DONE,
	HDMIRX_FRL_LTP_TIMEOUT,
	HDMIRX_FRL_LTP_MAX
} hdmirx_frl_ltp_state;

typedef enum {
	HDMIRX_FRL_GAP_IDEL,
	HDMIRX_FRL_GAP_CHECKING,
	HDMIRX_FRL_GAP_DONE,
	HDMIRX_FRL_GAP_TIMEOUT,
	HDMIRX_FRL_GAP_MAX
} hdmirx_frl_gap_state;

typedef struct {
	frl_state state;
	hdmirx_frl_rate rate;
	uint8_t ffe;
	uint8_t max_ffe;
	uint8_t ltp_req[FRL_LANE_MAX];
	hdmirx_frl_ltp_state ltp_state;
	hdmirx_frl_gap_state gap_state;
	struct timeval time_out;
	bool force_req;
	uint8_t check_cnt;
} hdmirx_frl_ctx;

#define LTP5 5
#define LTP6 6
#define LTP7 7
#define LTP8 8
#define NUM10B8B 512
#define PATTERN_CNT 16

int hdmirx_link_frl_init(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_link_frl_lut_data(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_link_frl_lut_check(struct hdmirx_ctrl_st *hdmirx, const uint32_t *g_lut, uint32_t len);
int hdmirx_link_frl_ltp_gap_checkcycle(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_link_frl_ltp_irq_set(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_link_frl_ready(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_process(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_req_set(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_flt_update(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_ltp_pattern_check(struct hdmirx_ctrl_st *hdmirx);
hdmirx_frl_ltp_state hdmirx_frl_pattern_switch(uint32_t temp, uint32_t offset);
hdmirx_frl_gap_state frl_get_gap_state(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_gap_state_check(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_start(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_frl_status_update(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_link_training_done(struct hdmirx_ctrl_st *hdmirx);
bool frl_get_interrupt(struct hdmirx_ctrl_st *hdmirx, hdmirx_frl_intr_type type);
void frl_clear_interrupt(struct hdmirx_ctrl_st *hdmirx, hdmirx_frl_intr_type type);
void frl_init_ltp_req(struct hdmirx_ctrl_st *hdmirx);
void frl_clear_init(struct hdmirx_ctrl_st *hdmirx);
void frl_set_state(struct hdmirx_ctrl_st *hdmirx,frl_state state);

#endif