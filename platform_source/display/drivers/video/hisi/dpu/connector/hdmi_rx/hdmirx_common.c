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
#include <hdmirx_common.h>

void hdmirx_ctrl_irq_clear(char __iomem *addr)
{
	uint32_t clear_irq;

	clear_irq = inp32(addr);
	set_reg(addr, clear_irq, 32, 0);
}

void set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	dpu_set_reg(addr, val, bw, bs);
}

void hdmirx_reg_read_block(const char __iomem *addr, uint32_t offset, uint32_t *dst, uint32_t num)
{
	uint32_t cur_offset = offset;
	uint32_t cnt = 0;

	if ((dst == NULL) || (addr == NULL)) {
		return;
	}

	while (cnt < num) {
		*dst = inp32(addr + cur_offset + sizeof(uint32_t) * cnt);
		dst++;
		cnt++;
	}
}
