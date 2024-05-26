/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Declaration of hjpeg softreset test.
 * Create: 2017-05-03
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

#ifndef __HJPG250_SOFTRESET_TEST_H__
#define __HJPG250_SOFTRESET_TEST_H__

#define SOFT_RESET 1

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

#define SR_JPEG_TOP_BASE 0xE8004000
#define SR_CVDR_AXI_BASE 0xE8006000
struct sr_reg_sets sr_cfg_reg_sets_es[] = {
	{ SR_JPEG_TOP_BASE, 0x4, 0x00000000, 0x00010000, SR_OP_WRITE_ALL },
	{ SR_CVDR_AXI_BASE, 0x1570, 0x02000000, 0x1, SR_OP_WRITE_BIT },
	{ SR_CVDR_AXI_BASE, 0x1CC, 0x02000000, 0x1, SR_OP_WRITE_BIT },
	{ SR_JPEG_TOP_BASE, 0xC, 0x00000000, 0x1, SR_OP_READ_ALL },
	{ SR_CVDR_AXI_BASE, 0x1570, 0x01000000, 0x1, SR_OP_READ_BIT },
	{ SR_CVDR_AXI_BASE, 0x1CC, 0x01000000, 0x1, SR_OP_READ_BIT },
	{ SR_JPEG_TOP_BASE, 0x4, 0x00000001, 0x1, SR_OP_WRITE_BIT },
	{ SR_JPEG_TOP_BASE, 0x4, 0x00000001, 0x1, SR_OP_READ_BIT },
	{ SR_CVDR_AXI_BASE, 0x1570, 0x02000000, 0x0, SR_OP_WRITE_BIT },
	{ SR_CVDR_AXI_BASE, 0x1CC, 0x02000000, 0x0, SR_OP_WRITE_BIT },
	{ SR_JPEG_TOP_BASE, 0x4, 0x00000000, 0x0, SR_OP_WRITE_ALL },
};

/* for cs */
struct sr_reg_sets sr_cfg_reg_sets[] = {
	{ SR_JPEG_TOP_BASE, 0x10C, 0x00000001, 0x1, SR_OP_WRITE_BIT },
	{ SR_CVDR_AXI_BASE, 0x1570, 0x02000000, 0x1, SR_OP_WRITE_BIT },
	{ SR_CVDR_AXI_BASE, 0x1CC, 0x02000000, 0x1, SR_OP_WRITE_BIT },
	{ SR_JPEG_TOP_BASE, 0x10C, 0x00000000, 0x1, SR_OP_READ_ALL },
	{ SR_CVDR_AXI_BASE, 0x1570, 0x01000000, 0x1, SR_OP_READ_BIT },
	{ SR_CVDR_AXI_BASE, 0x1CC, 0x01000000, 0x1, SR_OP_READ_BIT },
	{ SR_JPEG_TOP_BASE, 0x104, 0x00000001, 0x1, SR_OP_WRITE_BIT },
	{ SR_JPEG_TOP_BASE, 0x104, 0x00000001, 0x1, SR_OP_READ_BIT },
	{ SR_CVDR_AXI_BASE, 0x1570, 0x02000000, 0x0, SR_OP_WRITE_BIT },
	{ SR_CVDR_AXI_BASE, 0x1CC, 0x02000000, 0x0, SR_OP_WRITE_BIT },
	{ SR_JPEG_TOP_BASE, 0x104, 0x00000000, 0x0, SR_OP_WRITE_ALL },
	{ SR_JPEG_TOP_BASE, 0x10C, 0x00000001, 0x0, SR_OP_WRITE_BIT },
};
#endif
#endif /* __HJPG250_SOFTRESET_TEST_H__ */