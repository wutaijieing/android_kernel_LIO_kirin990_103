/* Copyright (c) 2022, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HDMIRX_COMMON_H
#define HDMIRX_COMMON_H

#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"
#include <linux/types.h>

typedef enum hdmirx_color_space_en {
	HDMIRX_COLOR_SPACE_RGB,
	HDMIRX_COLOR_SPACE_YCBCR422,
	HDMIRX_COLOR_SPACE_YCBCR444,
	HDMIRX_COLOR_SPACE_YCBCR420,
	HDMIRX_COLOR_SPACE_MAX
} hdmirx_color_space;

typedef enum ext_hdmirx_input_width {
	HDMIRX_INPUT_WIDTH_24,  /* color depth is 8 bit per channel, 24 bit per pixel */
	HDMIRX_INPUT_WIDTH_30,  /* color depth is 10 bit per channel, 30 bit per pixel */
	HDMIRX_INPUT_WIDTH_36,  /* color depth is 12 bit per channel, 36 bit per pixel */
	HDMIRX_INPUT_WIDTH_48,  /* color depth is 16 bit per channel, 48 bit per pixel */
	HDMIRX_INPUT_WIDTH_MAX
} hdmirx_input_width;

typedef enum hdmirx_oversample_en {
	HDMIRX_OVERSAMPLE_NONE,
	HDMIRX_OVERSAMPLE_2X,
	HDMIRX_OVERSAMPLE_3X,
	HDMIRX_OVERSAMPLE_4X,
	HDMIRX_OVERSAMPLE_5X,
	HDMIRX_OVERSAMPLE_6X,
	HDMIRX_OVERSAMPLE_7X,
	HDMIRX_OVERSAMPLE_8X,
	HDMIRX_OVERSAMPLE_9X,
	HDMIRX_OVERSAMPLE_10X,
	HDMIRX_OVERSAMPLE_MAX
} hdmirx_oversample;

void set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);
void hdmirx_reg_read_block(const char __iomem *addr, uint32_t offset, uint32_t *dst, uint32_t num);
void hdmirx_ctrl_irq_clear(char __iomem *addr);

#endif