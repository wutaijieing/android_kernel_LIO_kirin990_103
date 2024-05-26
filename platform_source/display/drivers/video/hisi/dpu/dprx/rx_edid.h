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

#ifndef __RX_EDID_H__
#define __RX_EDID_H__
#include <linux/types.h>

#define EDID_BLOCK_SIZE 128
#define MAX_EDID_BLOCK_NUM 4 /* 4 Blocks --> 512 bytes */

enum block_types {
	MAIN_BLOCK = 0,
	CEA_EXTENSION_BLOCK = 1,
	DISPLAYID_EXTENSION_BLOCK = 2,
	RESERVED_BLOCK = 3,
};

struct edid_block {
	uint8_t payload[EDID_BLOCK_SIZE];
};

struct rx_edid {
	uint8_t valid_block_num;
	struct edid_block block[MAX_EDID_BLOCK_NUM];
};

void rx_edid_init(struct rx_edid *edid, uint8_t block_num, uint32_t timing_code);
void rx_edid_dump(struct rx_edid* edid);

#endif
