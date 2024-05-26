/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 #ifndef __CMDLIST_H__
 #define __CMDLIST_H__

#include <linux/compiler.h>
#include <linux/list.h>
#include <linux/types.h>
#include <soc_dte_define.h>

struct hisi_dm_param;

enum CMDLIST_MODE {
	CONFIG_MODE = 0,
	TRANSPORT_MODE,
};

union _cmd_flag {
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
		uint32_t cmd_mode:1;
		uint32_t valid_flag:8;  /* 0xA5 is valid */
	} bits;
	uint32_t ul32;
};

union _total_items {
	struct {
		uint32_t items_count:14;
		uint32_t dma_dst_addr:18;
	} bits;
	uint32_t ul32;
};

/* 128bit align */
union _cmd_item {
	struct {
		uint32_t addr0:18;
		uint32_t addr1:6;
		uint32_t addr2:6;
		uint32_t cnt:2;
		uint32_t data0;
		uint32_t data1;
		uint32_t data2;
	} cmd_mode_0;

	struct {
		uint32_t data0;
		uint32_t data1;
		uint32_t data2;
		uint32_t data3;
	} cmd_mode_1;
};

/* 128bit align */
struct cmd_header {
	union _cmd_flag cmd_flag;
	uint32_t next_list;
	union _total_items total_items;
	uint32_t list_addr;
};

struct cmd_item {
	union _cmd_item cmd_item;
};

struct cmdlist_client {
	struct list_head list_node;

	dma_addr_t cmd_header_addr;
	struct cmd_header *list_cmd_header;

	dma_addr_t cmd_item_addr;
	struct cmd_item *list_cmd_item;

	size_t node_size;
	size_t payload_size;

	char __iomem *base;

	uint32_t cmd_mode;

	/* each client save scene_list_header pointer */
	struct list_head *scene_list_header;
	enum SCENE_ID scene_id;

	uint32_t enable;
};

static inline struct hisi_dm_param *cmdlist_get_dm_data(struct cmdlist_client *client)
{
	if (!client)
		return NULL;

	if (client->list_cmd_header->cmd_flag.bits.cmd_mode != 1)
		return NULL;

	return (struct hisi_dm_param *)client->list_cmd_item;
}

struct cmdlist_client *dpu_cmdlist_create_client(enum SCENE_ID id, enum CMDLIST_MODE cmd_mode);
int cmdlist_release_client(struct cmdlist_client *client);
int dpu_cmdlist_set_reg(struct cmdlist_client *client, uint32_t addr, uint32_t value);
void dpu_cmdlist_dump_client(struct cmdlist_client *client);
void cmdlist_flush_client(struct cmdlist_client *client);
#endif
