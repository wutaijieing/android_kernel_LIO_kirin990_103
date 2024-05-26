/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: protocol header file
 * Create: 2021/12/05
 */
#ifndef __PROTOCOL_BASE_H__
#define __PROTOCOL_BASE_H__

#include "tag.h"
#include "cmd.h"
#include "sub_cmd.h"

struct pkt_header {
	unsigned char tag;
	unsigned char cmd;
	unsigned char resp: 1;
	unsigned char hw_trans_mode: 2; /* 0:IPC 1:SHMEM 2:64bitsIPC */
	unsigned char rsv: 5;
	unsigned char partial_order;
	unsigned char tranid;
	unsigned char app_tag;
	unsigned short length;
};

struct pkt_header_resp {
	unsigned char tag;
	unsigned char cmd;
	unsigned char resp;
	unsigned char partial_order;
	unsigned char tranid;
	unsigned char app_tag;
	unsigned short length;
	unsigned int errno; /* In win32, errno is function name and conflict */
};

typedef struct {
	struct pkt_header hd;
	unsigned int subcmd;
} __packed pkt_subcmd_req_t;

struct pkt_subcmd_resp {
	struct pkt_header_resp hd;
	unsigned int subcmd;
} __packed;

typedef enum {
	EN_OK = 0,
	EN_FAIL,
} err_no_t;

typedef enum {
	NO_RESP,
	RESP,
} obj_resp_t;

#endif
