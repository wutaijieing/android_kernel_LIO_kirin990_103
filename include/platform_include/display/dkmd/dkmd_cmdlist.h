/**
 * @file dkmd_cmdlist.h
 * @brief The ioctl the interface file for cmdlist node.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef __DKMD_CMDLIST_H__
#define __DKMD_CMDLIST_H__

#include <linux/types.h>
#include <linux/bits.h>

#define CMDLIST_IOCTL_MAGIC 'C'

#define CMDLIST_CREATE_CLIENT _IOWR(CMDLIST_IOCTL_MAGIC, 0x1, struct cmdlist_node_client)
#define CMDLIST_SIGNAL_CLIENT _IOWR(CMDLIST_IOCTL_MAGIC, 0x2, struct cmdlist_node_client)
#define CMDLIST_RELEASE_CLIENT _IOWR(CMDLIST_IOCTL_MAGIC, 0x3, struct cmdlist_node_client)
#define CMDLIST_APPEND_NEXT_CLIENT _IOWR(CMDLIST_IOCTL_MAGIC, 0x4, struct cmdlist_node_client)
#define CMDLIST_DUMP_USER_CLIENT _IOWR(CMDLIST_IOCTL_MAGIC, 0x5, struct cmdlist_node_client)
#define CMDLIST_DUMP_SCENE_CLIENT _IOWR(CMDLIST_IOCTL_MAGIC, 0x6, struct cmdlist_node_client)
#define CMDLIST_INFO_GET _IOWR(CMDLIST_IOCTL_MAGIC, 0x8, unsigned)

enum {
	REGISTER_CONFIG_TYPE = BIT(0),
	DATA_TRANSPORT_TYPE = BIT(1),
	DM_TRANSPORT_TYPE = BIT(2),
	SCENE_NOP_TYPE = BIT(3),
	OPR_REUSE_TYPE = BIT(4),
};

struct cmdlist_node_client {
	uint32_t id;

	uint32_t type;
	uint32_t scene_id;
	uint32_t node_size;
	uint32_t dst_addr;

	uint32_t append_next_id;
	uint64_t viraddr;
	uint32_t phyaddr;
	uint32_t valid_payload_size;
};

struct cmdlist_info {
	uint64_t viraddr_base;
	uint32_t pool_size;
	int32_t reserved;
};

#endif
