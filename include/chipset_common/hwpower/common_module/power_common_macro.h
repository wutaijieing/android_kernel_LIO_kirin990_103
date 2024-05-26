/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_common_macro.h
 *
 * common define for power module
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#ifndef _POWER_COMMON_MACRO_H_
#define _POWER_COMMON_MACRO_H_

/* bits & bytes length */
#define POWER_BITS_PER_BYTE                 8
#define POWER_BITS_PER_WORD                 16
#define POWER_BYTE_LEN                      1
#define POWER_WORD_LEN                      2
#define POWER_DWORD_LEN                     4

/* mask */
#define POWER_MASK_BYTE                     0xff
#define POWER_MASK_WORD                     0xffff

#define POWER_PERCENT                       100

/* base */
#define POWER_BASE_OCT                      8
#define POWER_BASE_DEC                      10
#define POWER_BASE_HEX                      16

/* convert unit */
#define POWER_UW_PER_MW                     1000 /* microwatt & milliwatt */
#define POWER_MW_PER_W                      1000 /* milliwatt & watt */
#define POWER_UV_PER_MV                     1000 /* microvolt & millivolt */
#define POWER_MV_PER_V                      1000 /* millivolt & volt */
#define POWER_UA_PER_MA                     1000 /* microampere & milliampere */
#define POWER_MA_PER_A                      1000 /* milliampere & ampere */
#define POWER_US_PER_MS                     1000 /* microsecond & millisecond */
#define POWER_MS_PER_S                      1000 /* millisecond & second */
#define POWER_MC_PER_C                      1000 /* millicenti & centi */
#define POWER_MO_PER_O                      1000 /* milliohm & ohm */
#define POWER_SEC_PER_MIN                   60 /* second & minute */
#define POWER_MIN_PER_HOUR                  60 /* minute & hour */

#endif /* _POWER_COMMON_MACRO_H_ */
