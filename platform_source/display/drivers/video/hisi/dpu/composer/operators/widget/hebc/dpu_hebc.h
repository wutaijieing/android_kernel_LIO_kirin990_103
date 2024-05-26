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

#ifndef DPU_HEBC_H
#define DPU_HEBC_H

#define BITS_PER_BYTE 8
#define DMA_ALIGN_BYTES (128 / BITS_PER_BYTE)

#define HEBC_HEADER_SBLOCK_SIZE         4
#define HEBC_PLD_SBLOCK_SIZE            512
#define HEBC_PLD_SBLOCK_SIZE_RGB565     256
#define HEBC_HEADER_ADDR_ALIGN          4
#define HEBC_HEADER_STRIDE_ALIGN        32
#define HEBC_PAYLOAD_ADDR_ALIGN         512
#define HEBC_PAYLOAD_STRIDE_ALIGN       512

#define HEBC_RGB_HEADER_STRIDE_MAXIMUM   2048
#define HEBC_Y_HEADER_STRIDE_MAXIMUM     2048
#define HEBC_C_HEADER_STRIDE_MAXIMUM     2048
#define HEBC_RGB_PLD_STRIDE_MAXIMUM   (256 * 1024)
#define HEBC_Y_PLD_STRIDE_MAXIMUM     (256 * 1024)
#define HEBC_C_PLD_STRIDE_MAXIMUM     (256 * 1024)

#define HEBC_CROP_MAX        63
#define HEBC_PIC_WIDTH_ROT_MAX_RGB  2048
#define HEBC_PIC_HEIGHT_ROT_MAX_RGB 2048
#define HEBC_PIC_WIDTH_ROT_MAX_YUV  4096
#define HEBC_PIC_HEIGHT_ROT_MAX_YUV 4096
#define HEBC_YUV_TRANS 1

enum SUPER_BLOCK_TYPE {
	SUPER_BLOCK_TYPE_NORMAL = 0,
	SUPER_BLOCK_TYPE_ROT
};

enum {
	HEBC_NO_TAG,
	HEBC_TAG_MODE
};

enum {
	HEBC_NO_SCRAMBLE,
	HEBC_SCRAMBLE_128,
	HEBC_SCRAMBLE_256,
	HEBC_SCRAMBLE_512
};

enum {
	SBLOCK_TYPE_32X4 = 0,
	SBLOCK_TYPE_64X8 = 0,
	SBLOCK_TYPE_32X8 = 0,
	SBLOCK_TYPE_16X8_UP_DOWN = 1,
	SBLOCK_TYPE_16X8_LEFT_RIGHT = 2,
};

enum HEBC_SBLOCK {
	HEBC_SBLOCK_RGB_WIDTH = 32,
	HEBC_SBLOCK_RGB_HEIGHT = 4,
	HEBC_SBLOCK_RGB_WIDTH_ROT = 8,
	HEBC_SBLOCK_RGB_HEIGHT_ROT = 16,

	HEBC_SBLOCK_Y8_WIDTH = 64,
	HEBC_SBLOCK_Y8_HEIGHT = 8,
	HEBC_SBLOCK_Y8_WIDTH_ROT = 8,
	HEBC_SBLOCK_Y8_HEIGHT_ROT = 64,
	HEBC_SBLOCK_C8_WIDTH = 32,
	HEBC_SBLOCK_C8_HEIGHT = 8,

	HEBC_SBLOCK_Y10_WIDTH = 32,
	HEBC_SBLOCK_Y10_HEIGHT = 8,
	HEBC_SBLOCK_Y10_WIDTH_ROT = 8,
	HEBC_SBLOCK_Y10_HEIGHT_ROT = 32,
	HEBC_SBLOCK_C10_WIDTH = 16,
	HEBC_SBLOCK_C10_HEIGHT = 8,
};
void dpu_hebc_get_basic_align_info(uint32_t is_yuv_semi_planar, uint32_t is_super_block_rot, uint32_t format,
	uint32_t *width_align, uint32_t *height_align);

#endif
