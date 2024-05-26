/* SPDX-License-Identifier: GPL-2.0 */
/*
 * common.c
 *
 * common function
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

#include "common.h"
#include <linux/string.h>

#define get_u32(n, b, i) do            \
	(n) = ((u32) (b)[(i)] << 24) |     \
	((u32) (b)[(i) + 1] << 16) |       \
	((u32) (b)[(i) + 2] <<  8) |       \
	((u32) (b)[(i) + 3]);              \
while (0)

#define put_u32(n, b, i) do {          \
	(b)[(i)] = (u8) ((n) >> 24);       \
	(b)[(i) + 1] = (u8) ((n) >> 16);   \
	(b)[(i) + 2] = (u8) ((n) >>  8);   \
	(b)[(i) + 3] = (u8) ((n));         \
} while (0)

int u32tou8(const u32 *src, u8 *dst, u32 bytelen)
{
	s32 i;
	s32 tmplen = (s32)bytelen - 4;
	u32 j = 0;
	u32 tmp[MAXWORDLEN];
	const u32 *psrc = NULL;

	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	psrc = src;

	if (bytelen > MAXBYTELEN)
		return -EINVAL;

	if (src == (u32 *)dst) {
		memcpy(tmp, src, bytelen);
		psrc = tmp;
	}

	for (i = 0; i <= tmplen; i += 4) {
		put_u32(psrc[j], dst, tmplen - i);
		j++;
	}
	tmplen = (s32)(bytelen & 3) - 1;
	for (i = 0; i <= tmplen; i++)
		dst[i] = (u8)(psrc[j] >> ((u32)(tmplen - i) << 3));

	return 0;
}

int u8tou32(const u8 *src, u32 *dst, u32 bytelen)
{
	s32 i;
	s32 tmplen = (s32)(bytelen & 3) - 1;
	u32 j = 0;
	u8 tmp[MAXBYTELEN];
	const u8 *psrc = NULL;

	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	psrc = src;

	if (bytelen > MAXBYTELEN)
		return -EINVAL;

	if (src == (u8 *)dst) {
		memcpy(tmp, src, bytelen);
		psrc = tmp;
	}
	for (i = (s32)bytelen - 4; i >= 0; i -= 4) {
		get_u32(dst[j], psrc, i);
		j++;
	}
	if (tmplen >= 0) {
		dst[j] = 0;
		for (i = tmplen; i >= 0; i--)
			dst[j] |= (u32)(psrc[i]) << ((u32)(tmplen - i) << 3);
	}

	return 0;
}

static const u8 g_bits[256] = {
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

u32 u32_getbitlen(u32 n)
{
	if (n & 0xFFFF0000) {
		if (n & 0xFF000000)
			return (g_bits[n >> 24] + 24);
		else
			return (g_bits[n >> 16] + 16);
	} else {
		if (n & 0xFF00)
			return (g_bits[n >> 8] + 8);
		else
			return g_bits[n];
	}
}
