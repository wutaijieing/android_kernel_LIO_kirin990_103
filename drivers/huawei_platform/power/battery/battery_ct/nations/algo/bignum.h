/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bignum.h
 *
 * bignum head file for bugnum-calculate
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

#ifndef HEADER_BIGNUM_H
#define HEADER_BIGNUM_H
#include <linux/types.h>

#include "common.h"

#define BIGNUM_MAXLEN MAXWORDLEN

struct bignum {
	u32 *data;
	u32 maxlen;
	u32 len;
};

int bignum_clr(struct bignum *n);
u32 bignum_getbitval(const struct bignum *n, u32 i);
u32 bignum_getbitlen(const struct bignum *n);
s32 bignum_cmp(const struct bignum *a, const struct bignum *b);
int bignum_cpy(const struct bignum *src, struct bignum *dst);
int bignum_setzero(struct bignum *n);
int bignum_add(const struct bignum *a, const struct bignum *b,
	struct bignum *c);
int bignum_signedadd(const struct bignum *a, u32 asign, const struct bignum *b,
	u32 bsign, struct bignum *c, u32 *csign);
int bignum_addone(const struct bignum *a, struct bignum *c);
int bignum_sub(const struct bignum *a, const struct bignum *b,
	struct bignum *c);
int bignum_signedsub(const struct bignum *a, u32 asign, const struct bignum *b,
	u32 bsign, struct bignum *c, u32 *csign);
int bignum_subone(const struct bignum *a, struct bignum *c);
int bignum_mul(const struct bignum *a, const struct bignum *b,
	struct bignum *c);
int bignum_squ(const struct bignum *a, struct bignum *c);
int bignum_shl(const struct bignum *a, u32 len, struct bignum *c);
int bignum_shr(const struct bignum *a, u32 len, struct bignum *c);
int bignum_div(const struct bignum *a, const struct bignum *b, struct bignum *q,
	struct bignum *r);

#define bignum_mod(a, n, c) bignum_div(a, n, NULL, c)

int bignum_modadd(const struct bignum *a, const struct bignum *b,
	const struct bignum *n, struct bignum *c);
int bignum_modsub(const struct bignum *a, const struct bignum *b,
	const struct bignum *n, struct bignum *c);
int bignum_modmul(const struct bignum *a, const struct bignum *b,
	const struct bignum *n, struct bignum *c);
int bignum_modmul2(const struct bignum *a,
	const struct bignum *n, struct bignum *c);
int bignum_moddiv2(const struct bignum *a,
	const struct bignum *n, struct bignum *c);
int bignum_modinv(const struct bignum *a, const struct bignum *n,
	struct bignum *c);

struct bignum_mont_ctx {
	const struct bignum *n;
	u32 ni0;
	u32 rr[MAXWORDLEN];
};

int bignum_mont_set(struct bignum_mont_ctx *ctx, const struct bignum *n);
int bignum_mont_start(const struct bignum_mont_ctx *ctx,
	const struct bignum *a, struct bignum *ma);
int bignum_mont_mul(const struct bignum_mont_ctx *ctx,
	const struct bignum *ma, const struct bignum *mb,
	struct bignum *mc);
int bignum_mont_finish(const struct bignum_mont_ctx *ctx,
	const struct bignum *ma, struct bignum *a);

#endif /* HEADER_BIGNUM_H */
