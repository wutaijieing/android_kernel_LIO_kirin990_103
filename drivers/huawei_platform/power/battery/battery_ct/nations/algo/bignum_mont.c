/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bignum_mont.c
 *
 * bignum calculate
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

#include "bignum.h"
#include <linux/string.h>

int bignum_mont_set(struct bignum_mont_ctx *ctx, const struct bignum *n)
{
	struct bignum r[1], n0[1], rr1[1];
	u32 rdata[MAXWORDLEN + 1] = { 0 };
	u32 i;

	if (!ctx || !n) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	i = u32_getbitlen(n->data[n->len - 1]);

	r->data = rdata;
	r->maxlen = MAXWORDLEN + 1;
	if (i == 32) {
		rdata[n->len] = 1;
		r->len = n->len + 1;
	} else {
		rdata[n->len - 1] = 1 << i;
		r->len = n->len;
	}

	rr1->data = ctx->rr;
	rr1->maxlen = MAXWORDLEN;
	rr1->len = 0;

	bignum_sub(r, n, rr1);

	i = (n->len << 5) + 32 - i;

	while (i) {
		bignum_modmul2(rr1, n, rr1);
		i--;
	}
	rdata[0] = 0;
	rdata[1] = 1;
	r->len = 2;

	n0->data = n->data;
	n0->maxlen = 1;
	n0->len = 1;

	bignum_modinv(r, n0, r);
	bignum_shl(r, 32, r);
	bignum_subone(r, r);
	bignum_div(r, n0, r, NULL);
	ctx->n = (struct bignum *)n;
	ctx->ni0 = rdata[0];

	return 0;
}

int bignum_mont_mul(const struct bignum_mont_ctx *ctx, const struct bignum *ma,
	const struct bignum *mb, struct bignum *mc)
{
	struct bignum t[1];
	u32 tdata[(MAXWORDLEN << 1) + 1] = { 0 };
	u64 cs;
	u32 m, carry;
	u32 i, j;

	if (!ctx || !ma || !mb || !mc) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	t->data = tdata;
	t->maxlen = (MAXWORDLEN << 1) + 1;
	t->len = 0;

	if (ma != mb)
		bignum_mul(ma, mb, t);
	else
		bignum_squ(ma, t);

	for (i = 0; i < ctx->n->len; i++) {
		cs = 0;
		m = tdata[i] * ctx->ni0;
		for (j = 0; j < ctx->n->len; j++) {
			cs = (u64)tdata[i + j] + (u64)m * (u64)ctx->n->data[j] + (cs >> 32);
			tdata[i + j] = (u32)cs;
		}

		carry = cs >> 32;
		tdata[i + ctx->n->len] += carry;
		if (tdata[i + ctx->n->len] < carry) {
			carry = 1;
			for (j = i + ctx->n->len + 1; j < t->maxlen; j++) {
				tdata[j] += carry;
				if (tdata[j])
					break;
			}
		}
	}

	for (i = 0; i <= ctx->n->len; i++)
		tdata[i] = tdata[i + ctx->n->len];

	t->len = ctx->n->len + 1;
	bignum_clr(t);

	if (bignum_cmp(t, ctx->n) >= 0)
		return bignum_sub(t, ctx->n, mc);

	mc->len = mc->maxlen <= t->len ? mc->maxlen : t->len;
	memcpy(mc->data, tdata, mc->len << 2);
	bignum_clr(mc);
	return 0;
}

int bignum_mont_start(const struct bignum_mont_ctx *ctx,
	const struct bignum *a, struct bignum *ma)
{
	struct bignum rr1[1];

	if (!ctx || !a || !ma) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	rr1->data = (u32 *)ctx->rr;
	rr1->maxlen = MAXWORDLEN;
	rr1->len = ctx->n->len;

	return bignum_mont_mul(ctx, a, rr1, ma);
}

int bignum_mont_finish(const struct bignum_mont_ctx *ctx,
	const struct bignum *ma, struct bignum *a)
{
	struct bignum one[1];
	u32 onedata[1] = { 1 };

	if (!ctx || !ma || !a) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	one->data = onedata;
	one->maxlen = 1;
	one->len = 1;

	return bignum_mont_mul(ctx, one, ma, a);
}
