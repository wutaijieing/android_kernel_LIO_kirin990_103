/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Implement of cvdr config.
 * Create: 2017-02-28
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __INCLUDE_HJPG160_SOFTRESET_TEST_H__
#define __INCLUDE_HJPG160_SOFTRESET_TEST_H__

#ifdef SOFT_RESET
struct sr_reg_sets {
	u32 base;
	u32 offset;
	u32 bit_flag;
	u32 value;
	u32 op_flag;
};

enum SR_OP_FLAG {
	SR_OP_WRITE_ALL,
	SR_OP_READ_ALL,
	SR_OP_WRITE_BIT,
	SR_OP_READ_BIT,
};

#define SR_JPEG_TOP_BASE 0xE8304000
#define SR_CVDR_AXI_BASE 0xE8302000

struct sr_reg_sets sr_cfg_reg_sets[] = {
	{SR_JPEG_TOP_BASE, 0x4  , 0x00000000, 0x00010000, SR_OP_WRITE_ALL},
	{SR_CVDR_AXI_BASE, 0xA00, 0x02000000, 0x1 , SR_OP_WRITE_BIT},
	{SR_CVDR_AXI_BASE, 0x28 , 0x02000000, 0x1 , SR_OP_WRITE_BIT},
	{SR_JPEG_TOP_BASE, 0xC  , 0x00000000, 0x1 , SR_OP_READ_ALL },
	{SR_CVDR_AXI_BASE, 0xA00, 0x01000000, 0x1 , SR_OP_READ_BIT },
	{SR_CVDR_AXI_BASE, 0x28 , 0x01000000, 0x1 , SR_OP_READ_BIT },
	{SR_JPEG_TOP_BASE, 0x4  , 0x00000001, 0x1 , SR_OP_WRITE_BIT},
	{SR_JPEG_TOP_BASE, 0x4  , 0x00000001, 0x1 , SR_OP_READ_BIT },
	{SR_CVDR_AXI_BASE, 0xA00, 0x02000000, 0x0 , SR_OP_WRITE_BIT},
	{SR_CVDR_AXI_BASE, 0x28 , 0x02000000, 0x0 , SR_OP_WRITE_BIT},
	{SR_JPEG_TOP_BASE, 0x4  , 0x00000000, 0x0 , SR_OP_WRITE_ALL},
};
#endif
#endif /* __INCLUDE_HJPG160_SOFTRESET_TEST_H__ */