/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: header file for subcmd initial draft
 * Create: 2019-01-11
 */

#ifndef HISP350_MSG_SUBCMD_H_INCLUDED
#define HISP350_MSG_SUBCMD_H_INCLUDED

#include "hisp350_msg_subcmd_enum.h"

#define SUB_COUNT_FIRST 2
#define PIPELINE_COUNT 2
#define PARAS_LEN 400
#define EXT_ACK_PARAS_LEN 72

typedef struct _msg_req_set_pd_key_t {
	unsigned short setVal;
} msg_req_set_pd_key;

typedef struct _msg_req_get_pd_key_t {
	unsigned short getVal1;
	unsigned short getVal2;
} msg_req_get_pd_key;

typedef struct _msg_req_extend_set_t {
	unsigned int extend_cmd;
	unsigned int cam_count;
	unsigned int cam_id[PIPELINE_COUNT];
	char paras[PARAS_LEN];
} msg_req_extend_set_t;

typedef struct _msg_ack_extend_set_t {
	unsigned int extend_cmd;
	int status;
	unsigned int cam_count;
	unsigned int cam_id[PIPELINE_COUNT];
	char ack_info[EXT_ACK_PARAS_LEN];
} msg_ack_extend_set_t;

/* first expo and gain ack AP to select picture */
typedef struct capture_ack_t {
	unsigned int flow;
	unsigned int expo[SUB_COUNT_FIRST];
	unsigned int gain[SUB_COUNT_FIRST];
} capture_ack_t;

typedef struct _msg_req_extend_get_t {
	unsigned int cam_id;
	unsigned int extend_cmd;
	char *paras;
} msg_req_extend_get_t;

typedef struct _msg_ack_extend_get_t {
	unsigned int cam_id;
	unsigned int extend_cmd;
	char *paras;
	int status;
} msg_ack_extend_get_t;

#endif /* HISP350_MSG_SUBCMD_H_INCLUDED */
