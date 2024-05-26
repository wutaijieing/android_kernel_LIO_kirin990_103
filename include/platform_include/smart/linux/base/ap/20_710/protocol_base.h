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
	unsigned char resp; /* value CMD_RESP means need resp, CMD_NO_RESP means need not resp */
	unsigned char partial_order;
	unsigned short tranid;
	unsigned short length;
};

struct pkt_header_resp {
	unsigned char tag;
	unsigned char cmd;
	unsigned char resp;
	unsigned char partial_order;
	unsigned short tranid;
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
