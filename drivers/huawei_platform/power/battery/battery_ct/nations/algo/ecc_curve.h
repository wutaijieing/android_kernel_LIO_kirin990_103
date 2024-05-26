/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ecc_curve.h
 *
 * ellipse curve cryptography curve compute head file
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

#ifndef HEADER_ECC_CURVE_H
#define HEADER_ECC_CURVE_H
#include <linux/types.h>

#include "common.h"
#include "bignum.h"

#define ECC_MAX_WORD_LEN 8
#define ECC_MAX_BYTE_LEN 32
#define ECC_MAX_BIT_LEN  256

struct ecc_point {
	struct bignum x[1];
	struct bignum y[1];
};

int ecc_load_crv_parm(u8 *p, u32 p_byte_len,
	u8 *a, u32 a_byte_len,
	u8 *b, u32 b_byte_len,
	u8 *gx, u32 gx_byte_len,
	u8 *gy, u32 gy_byte_len,
	u8 *n, u32 n_byte_len,
	u8 *h, u32 h_byte_len);
int ecc_point_is_on_crv(const struct ecc_point *p);
int ecc_point_add(const struct ecc_point *a, const struct ecc_point *b,
	struct ecc_point *c);
int ecc_point_mul(const struct bignum *k, const struct ecc_point *p,
	struct ecc_point *r);
const struct bignum *ecc_get_n(void);
const struct bignum *ecc_get_h(void);
const struct ecc_point *ecc_get_g(void);
const struct bignum_mont_ctx *ecc_get_n_ctx(void);
u32 ecc_get_p_len(void);
int ecc_u8_to_bignum(const u8 *src, struct bignum *dst);
int ecc_bignum_to_u8(const struct bignum *src, u8 *dst);
int ecc_u8_to_point(const u8 *src, struct ecc_point *dst);
int ecc_point_to_u8(const struct ecc_point *src, u8 *dst);

#endif /* HEADER_ECC_CURVE_H */
