/* SPDX-License-Identifier: GPL-2.0 */
/*
 * common.h
 *
 * conmon head file for common function
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

#ifndef HEADER_COMMON_H
#define HEADER_COMMON_H
#include <linux/types.h>
#include <linux/err.h>
#include <huawei_platform/log/hw_log.h>

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG battct_nations
HWLOG_REGIST();

#define MAXWORDLEN                      16
#define MAXBYTELEN                      ((MAXWORDLEN) << 2)

int u32tou8(const u32 *src, u8 *dst, u32 bytelen);
int u8tou32(const u8 *src, u32 *dst, u32 bytelen);
u32 u32_getbitlen(u32 n);
int u32_get_rand(u32 *n, u32 len);

#endif /* HEADER_COMMON_H */

