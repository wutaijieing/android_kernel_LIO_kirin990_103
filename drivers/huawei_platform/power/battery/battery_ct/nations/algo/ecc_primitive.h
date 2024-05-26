/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ecc_primitive.h
 *
 * ellipse curve cryptography curve primitive head file
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
#ifndef HEADER_ECC_PRIMATIVE_H
#define HEADER_ECC_PRIMATIVE_H
#include <linux/types.h>

#include "common.h"
#include "bignum.h"
#include "ecc_curve.h"

int ecvp_dsa(u8 *e, u32 e_byte_len, u8 *pub_key, u8 *r, u8 *s);

#endif /* HEADER_ECC_PRIMATIVE_H */
