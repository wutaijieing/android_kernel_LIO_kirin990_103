/* SPDX-License-Identifier: GPL-2.0 */
/*
 * gf2m_common.c
 *
 * gf2m common function
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
#include "gf2m_common.h"

void copy_u8(u8 *dst, u8 *src, u32 byte_len)
{
	u32 i;

	if (!dst || !src) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	if (dst != src)
		for (i = 0; i < byte_len; i++)
			*(dst + i) = *(src + i);
}

void copy_u32(u32 *dst, u32 *src, u32 word_len)
{
	u32 i;

	if (!dst || !src) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	if (dst != src)
		for (i = 0; i < word_len; i++)
			*(dst + i) = *(src + i);
}

void reverse_u8(u8 *dst, u8 *src, u32 byte_len)
{
	u32 tmp;
	u32 i;

	if (!dst || !src) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	if (dst != src) {
		for (i = 0; i < byte_len; i++)
			*(dst + i) = *(src + byte_len - 1 - i);
	} else {
		for (i = 0; i < (byte_len / 2); i++) {
			tmp = *(src + i);
			*(dst + i) = *(src + byte_len - 1 - i);
			*(dst + byte_len - 1 - i) = tmp;
		}
	}
}

void reverse_u32(u32 *dst, u32 *src, u32 word_len)
{
	u32 tmp;
	u32 i;

	if (!dst || !src) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	if (dst != src) {
		for (i = 0; i < word_len; i++)
			*(dst + i) = *(src + word_len - 1 - i);
	} else {
		for (i = 0; i < (word_len / 2); i++) {
			tmp = *(src + i);
			*(dst + i) = *(src + word_len - 1 - i);
			*(dst + word_len - 1 - i) = tmp;
		}
	}
}
