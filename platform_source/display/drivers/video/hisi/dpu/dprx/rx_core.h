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
#ifndef __RX_CORE_H__
#define __RX_CORE_H__

#include "dprx.h"

#define DPCD_WR_BYTE_LEN 4 /* DPCD w/r in 4 byte length */

uint8_t dprx_read_dpcd(struct dprx_ctrl *dprx, uint32_t addr);
void dprx_write_dpcd(struct dprx_ctrl *dprx, uint32_t addr, uint8_t val);

void dprx_dis_reset(struct dprx_ctrl *dprx, bool benable);
void dprx_core_on(struct dprx_ctrl *dprx);
void dprx_core_off(struct dprx_ctrl *dprx);

#endif