/* SPDX-License-Identifier: GPL-2.0 */
/*
 * gf2m_common.h
 *
 * gf2m common function head file
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef GF2M_COMMON_H
#define GF2M_COMMON_H
#include <linux/types.h>

#include "common.h"

void copy_u8(u8 *dst, u8 *src, u32 byte_len);
void copy_u32(uint32_t *dst, u32 *src, u32 word_len);
void reverse_u8(u8 *dst, u8 *src, u32 byte_len);
void reverse_u32(u32 *dst, u32 *src, u32 word_len);

#endif /* GF2M_COMMON_H */

