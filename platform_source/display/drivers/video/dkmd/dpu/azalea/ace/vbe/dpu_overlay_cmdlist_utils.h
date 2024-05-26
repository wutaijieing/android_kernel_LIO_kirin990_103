/*  Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#ifndef _CMD_LIST_UTILS_H_
#define _CMD_LIST_UTILS_H_

#include "overlay/dpu_overlay_utils_dssv510.h"

#define DPUV320K_CHIP_ID 6261

#define CONFIG_DPU_CMDLIST_LAST_USED

#define DPU_CMDLIST_DATA_MAX 3
#define DPU_CMDLIST_NODE_MAX 28
#define DPU_CMDLIST_BLOCK_MAX 28

#define DPU_SUPPORT_DPP_MODULE_BIT(module) (BIT(module) & DPU_DPP_MAX_SUPPORT_BIT)

#define MAX_ITEM_OFFSET 0x3F
#define CMDLIST_ADDR_OFFSET 0x3FFFF
#define MAX_ITEM_INDEX (SZ_1K)

#define DPU_CMDLIST_MAX 16

enum dpp_module_idx {
	DPP_MODULE_POST_SCF = 0,
	DPP_MODULE_DBUF = 1,
	DPP_MODULE_ACM = 3,
	DPP_MODULE_ACE = 4,
	DPP_MODULE_LCP_IGM = 5,
	DPP_MODULE_LCP_GMP = 6,
	DPP_MODULE_LCP_XCC = 7,
	DPP_MODULE_GAMA = 8,
	DPP_MODULE_DITHER = 9,
	DPP_MODULE_IFBC = 10,
	DPP_MODULE_HIACE = 11,
	DPP_MODULE_POST_XCC = 12,
	DPP_MODULE_MAX = 13
};

enum wb_type {
	WB_TYPE_WCH0,
	WB_TYPE_WCH1,
	WB_TYPE_WCH2,
	WB_TYPE_WCH0_WCH1,

	WB_TYPE_MAX,
};

#ifndef CMDLIST_POOL_NEW
enum dss_cmdlist_pool_idx {
	DSS_CMDLIST_NONE = -1,
	DSS_CMDLIST_D2 = 0,
	DSS_CMDLIST_D3,
	DSS_CMDLIST_V0,
	DSS_CMDLIST_G0,
	DSS_CMDLIST_V1,
	DSS_CMDLIST_G1,
	DSS_CMDLIST_D0,
	DSS_CMDLIST_D1,

	DSS_CMDLIST_W0,
	DSS_CMDLIST_W1,

	DSS_CMDLIST_OV0,
	DSS_CMDLIST_OV1,
	DSS_CMDLIST_OV2,
	DSS_CMDLIST_OV3,

	DSS_CMDLIST_V2,
	DSS_CMDLIST_W2,
	DSS_CMDLIST_MAX,
};
#endif

enum dss_cmdlist_chn_idx {
	DSS_CMDLIST_CHN_NONE = -1,
	DSS_CMDLIST_CHN_D2 = 0,
	DSS_CMDLIST_CHN_D3,
	DSS_CMDLIST_CHN_V0,
	DSS_CMDLIST_CHN_G0,
	DSS_CMDLIST_CHN_V1,
	DSS_CMDLIST_CHN_G1,
	DSS_CMDLIST_CHN_D0,
	DSS_CMDLIST_CHN_D1,

	DSS_CMDLIST_CHN_W0,
	DSS_CMDLIST_CHN_W1,

	DSS_CMDLIST_CHN_OV0,
	DSS_CMDLIST_CHN_OV1,
	DSS_CMDLIST_CHN_OV2,
	DSS_CMDLIST_CHN_OV3,

	DSS_CMDLIST_CHN_V2,
	DSS_CMDLIST_CHN_W2,
	DSS_CMDLIST_CHN_MAX,
};

enum cmdlist_event_list {
	DSS_EVENT_LIST_D2 = 14,
	DSS_EVENT_LIST_D3 = 15,
	DSS_EVENT_LIST_V0 = 16,
	DSS_EVENT_LIST_G0 = 17,
	DSS_EVENT_LIST_V1 = 18,
	DSS_EVENT_LIST_G1 = 19,
	DSS_EVENT_LIST_D0 = 20,
	DSS_EVENT_LIST_D1 = 21,
	DSS_EVENT_LIST_V2 = 22,
	DSS_EVENT_LIST_OV0 = 24,
	DSS_EVENT_LIST_OV1 = 25,
	DSS_EVENT_LIST_OV2 = 26,
	DSS_EVENT_LIST_OV3 = 27,
	DSS_EVENT_LIST_W0 = 30,
	DSS_EVENT_LIST_W1 = 31,
	DSS_EVENT_LIST_W2 = 32,
};

union cmd_flag {
	struct {
		uint32_t exec:1;
		uint32_t last:1;
		uint32_t nop:1;
		uint32_t interrupt:1;
		uint32_t pending:1;
		uint32_t id:10;
		uint32_t event_list:6;
		uint32_t qos:1;
		uint32_t task_end:1;
		uint32_t reserved:1;
		uint32_t valid_flag:8;  /* 0xA5 is valid */
	} bits;
	uint32_t ul32;
};

union total_items {
	struct {
		uint32_t count:14;
		uint32_t reserved:18;
	} bits;
	uint32_t ul32;
};

union reg_addr {
	struct {
		uint32_t add0:18;
		uint32_t add1:6;
		uint32_t add2:6;
		uint32_t cnt:2;
	} bits;
	uint32_t ul32;
};

typedef struct cmd_item {
	union reg_addr reg_addr;
	uint32_t data0;
	uint32_t data1;
	uint32_t data2;
} cmd_item_t;

typedef struct cmd_header {
	union cmd_flag flag;
	uint32_t next_list;
	union total_items total_items;
	uint32_t list_addr;  /* 128bit align */
} cmd_header_t;

enum dss_cmdlist_node_valid {
	CMDLIST_NODE_INVALID = 0x0,
	CMDLIST_NODE_VALID = 0xA5,
};

enum dss_cmdlist_node_type {
	CMDLIST_NODE_NONE = 0x0,
	CMDLIST_NODE_NOP = 0x1,
	CMDLIST_NODE_FRAME = 0x2,
};

enum dss_cmdlist_status {
	E_STATUS_IDLE = 0x0,
	E_STATUS_WAIT = 0x1,
	E_STATUS_OTHER,
};

/*
 * for normal node,all variable should be filled.
 * for NOP node, just the list_header,header_buffer_handle,list_node, node_flag should be filled.
 * node_type must be CMDLIST_NODE_NOP when it is NOP node.
 * And item_buffer_handle in NOP node should be NULL.
 */
typedef struct dss_cmdlist_node {
	struct list_head list_node;

	phys_addr_t header_phys;
	cmd_header_t *list_header;
	size_t header_len;

	phys_addr_t item_phys;
	cmd_item_t *list_item;
	size_t item_len;

	uint32_t item_index;
	int item_flag;
	uint32_t node_type;
	int is_used;
	int reserved;
} dss_cmdlist_node_t;

typedef struct dss_cmdlist_heads {
	struct list_head cmdlist_head;

	dss_cmdlist_node_t *cmdlist_nodes[DPU_CMDLIST_NODE_MAX];
} dss_cmdlist_heads_t;

typedef struct dss_cmdlist_data {
	dss_cmdlist_heads_t *cmdlist_heads[DPU_CMDLIST_MAX];
	struct list_head cmdlist_head_temp[DPU_CMDLIST_MAX];
	dss_cmdlist_node_t *cmdlist_nodes_temp[DPU_CMDLIST_MAX][DPU_CMDLIST_NODE_MAX];
} dss_cmdlist_data_t;

typedef struct dss_cmdlist_info {
	struct semaphore cmdlist_wb_common_sem;  /* for primary offline & copybit */
	struct semaphore cmdlist_wb_sem[WB_TYPE_MAX];
	wait_queue_head_t cmdlist_wb_wq[WB_TYPE_MAX];
	uint32_t cmdlist_wb_done[WB_TYPE_MAX];
	uint32_t cmdlist_wb_flag[WB_TYPE_MAX];
} dss_cmdlist_info_t;

typedef struct dss_copybit_info {
	struct semaphore copybit_sem;
	wait_queue_head_t copybit_wq;
	uint32_t copybit_flag;
	uint32_t copybit_done;
} dss_copybit_info_t;

typedef struct dss_media_common_info {
	wait_queue_head_t mdc_wq;
	uint32_t mdc_flag;
	uint32_t mdc_done;
} dss_media_common_info_t;

typedef struct dss_wb_info {
	uint32_t to_be_continued;
	uint32_t cmdlist_idxs;
	uint32_t wb_compose_type;
	uint32_t mctl_idx;
} dss_wb_info_t;

struct dss_cmdlist_idxs {
	uint32_t *cmdlist_pre_idxs;
	uint32_t *cmdlist_idxs;
	int *enable_cmdlist;
};

extern dss_cmdlist_data_t *g_cmdlist_data;
extern uint32_t g_cmdlist_chn_map[DSS_CMDLIST_MAX];
extern uint32_t g_cmdlist_eventlist_map[DSS_CMDLIST_MAX];

struct dpu_fb_data_type;
/******************************************************************************
 * FUNCTIONS PROTOTYPES
 */
int32_t dpu_get_cmdlist_idx_by_chnidx(uint32_t chnidx);
int32_t dpu_get_cmdlist_idx_by_ovlidx(uint32_t ovlidx);
uint32_t dpu_get_cmdlist_chnidx(uint32_t poolidx);
void dpu_cmdlist_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *addr, uint32_t value, uint8_t bw, uint8_t bs);
void dpu_cmdlist_flush_cache(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs);

dss_cmdlist_node_t *dpu_cmdlist_node_alloc(struct dpu_fb_data_type *dpufd, size_t header_len, size_t item_len);
void dpu_cmdlist_node_free(struct dpu_fb_data_type *dpufd, dss_cmdlist_node_t *node);

uint32_t dpu_cmdlist_get_cmdlist_need_start(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs);

int dpu_cmdlist_get_cmdlist_idxs(dss_overlay_t *pov_req, uint32_t *cmdlist_pre_idxs,
	uint32_t *cmdlist_idxs);
void dpu_cmdlist_data_get_online(struct dpu_fb_data_type *dpufd);
void dpu_cmdlist_data_get_offline(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);

int dpu_cmdlist_add_nop_node(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs, int pending, int reserved);
int dpu_cmdlist_add_new_node(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs,
	union cmd_flag flag, int remove);
int dpu_cmdlist_del_all_node(struct list_head *cmdlist_heads);

int dpu_cmdlist_config_start(struct dpu_fb_data_type *dpufd, int mctl_idx, uint32_t cmdlist_idxs,
	uint32_t wb_compose_type);

int dpu_cmdlist_config_stop(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs);
void dpu_cmdlist_config_reset(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, uint32_t cmdlist_idxs);

int dpu_cmdlist_del_node(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req, uint32_t cmdlist_idxs);
int dpu_cmdlist_check_cmdlist_state(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs);

int dpu_cmdlist_exec(struct dpu_fb_data_type *dpufd, uint32_t cmdlist_idxs);
void dpu_cmdlist_qos_on(struct dpu_fb_data_type *dpufd);
int dpu_cmdlist_dump_all_node(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req,
	uint32_t cmdlist_idxs);

int dpu_cmdlist_init(struct dpu_fb_data_type *dpufd);
int dpu_cmdlist_deinit(struct dpu_fb_data_type *dpufd);
bool dpu_check_cmdlist_paremeters_validate(uint8_t bw, uint8_t bs);

#endif

