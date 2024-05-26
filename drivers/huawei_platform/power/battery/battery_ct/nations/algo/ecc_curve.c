/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ecc_curve.c
 *
 * ellipse curve cryptography curve compute
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

#include "ecc_curve.h"
#include <linux/err.h>
#include <linux/slab.h>

static u32 g_ecc_p_data[ECC_MAX_WORD_LEN];
static u32 g_ecc_a_data[ECC_MAX_WORD_LEN];
static u32 g_ecc_b_data[ECC_MAX_WORD_LEN];
static u32 g_ecc_n_data[ECC_MAX_WORD_LEN];
static u32 g_ecc_h_data[ECC_MAX_WORD_LEN];
static u32 g_ecc_gx_data[ECC_MAX_WORD_LEN];
static u32 g_ecc_gy_data[ECC_MAX_WORD_LEN];

static struct bignum g_ecc_p[1] = { { g_ecc_p_data, 0, 0 } };
static struct bignum g_ecc_a[1] = { { g_ecc_a_data, 0, 0 } };
static struct bignum g_ecc_b[1] = { { g_ecc_b_data, 0, 0 } };
static struct bignum g_ecc_n[1] = { { g_ecc_n_data, 0, 0 } };
static struct bignum g_ecc_h[1] = { { g_ecc_h_data, 0, 0 } };
static struct ecc_point g_ecc_g[1] = { { { { g_ecc_gx_data, 0, 0 } },
	{ { g_ecc_gy_data, 0, 0 } } } };

static struct bignum_mont_ctx g_ecc_p_ctx[1];
static struct bignum_mont_ctx g_ecc_n_ctx[1];

static u32 g_ecc_n_byte_len;
static u32 g_ecc_p_byte_len;

static u32 g_one[ECC_MAX_WORD_LEN] = { 1 };
static struct bignum g_mont_one[1] = { { g_one, ECC_MAX_WORD_LEN, 1 } };
static u32 g_a_equ_p_sub3;

struct ecc_j_pro_point {
	struct bignum x[1];
	struct bignum y[1];
	struct bignum z[1];
};

const struct bignum *ecc_get_n(void)
{
	return g_ecc_n;
}

const struct bignum *ecc_get_h(void)
{
	return g_ecc_h;
}

const struct ecc_point *ecc_get_g(void)
{
	return g_ecc_g;
}

const struct bignum_mont_ctx *ecc_get_n_ctx(void)
{
	return g_ecc_n_ctx;
}

u32 ecc_get_p_len(void)
{
	return g_ecc_p_byte_len;
}


int ecc_u8_to_bignum(const u8 *src, struct bignum *dst)
{
	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	u8tou32(src, dst->data, g_ecc_n_byte_len);
	dst->len = g_ecc_n->len;
	bignum_clr(dst);

	if ((dst->len == 0) || (bignum_cmp(dst, g_ecc_n) >= 0))
		return -EINVAL;
	return 0;
}

int ecc_bignum_to_u8(const struct bignum *src, u8 *dst)
{
	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	u32tou8(src->data, dst, g_ecc_n_byte_len);
	return 0;
}

int ecc_u8_to_point(const u8 *src, struct ecc_point *dst)
{
	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (src[0] != 0x04)
		return -EINVAL;

	u8tou32(src + 1, dst->x->data, g_ecc_p_byte_len);
	u8tou32(src + g_ecc_p_byte_len + 1, dst->y->data, g_ecc_p_byte_len);
	dst->y->len = g_ecc_p->len;
	dst->x->len = dst->y->len;

	bignum_clr(dst->x);
	bignum_clr(dst->y);

	if ((dst->x->len == 0) || (bignum_cmp(dst->x, g_ecc_p) >= 0))
		return -EINVAL;

	if ((dst->y->len == 0) || (bignum_cmp(dst->y, g_ecc_p) >= 0))
		return -EINVAL;

	if (ecc_point_is_on_crv(dst))
		return -EINVAL;

	return 0;
}

int ecc_point_to_u8(const struct ecc_point *src, u8 *dst)
{
	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if ((src->x->len == 0) && (src->y->len == 0)) {
		memset(dst, 0, (g_ecc_p_byte_len << 1) + 1);
		return -EINVAL;
	}

	dst[0] = 0x04;
	u32tou8(src->x->data, dst + 1, g_ecc_p_byte_len);
	u32tou8(src->y->data, dst + 1 + g_ecc_p_byte_len, g_ecc_p_byte_len);

	return 0;
}

int ecc_load_crv_parm(u8 *p, u32 p_byte_len,
	u8 *a, u32 a_byte_len,
	u8 *b, u32 b_byte_len,
	u8 *gx, u32 gx_byte_len,
	u8 *gy, u32 gy_byte_len,
	u8 *n, u32 n_byte_len,
	u8 *h, u32 h_byte_len)
{
	if (!p || !a || !b || !gx || !gy || !n || !h) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	u8tou32(p, g_ecc_p_data, p_byte_len);
	u8tou32(a, g_ecc_a_data, a_byte_len);
	u8tou32(b, g_ecc_b_data, b_byte_len);
	u8tou32(n, g_ecc_n_data, n_byte_len);
	u8tou32(h, g_ecc_h_data, h_byte_len);
	u8tou32(gx, g_ecc_gx_data, gx_byte_len);
	u8tou32(gy, g_ecc_gy_data, gy_byte_len);

	g_ecc_p->len = (p_byte_len + 3) >> 2;
	g_ecc_p->maxlen = g_ecc_p->len;
	g_ecc_n->len = (n_byte_len + 3) >> 2;
	g_ecc_n->maxlen = g_ecc_n->len;
	g_ecc_h->len = (h_byte_len + 3) >> 2;
	g_ecc_h->maxlen = g_ecc_h->len;
	g_ecc_a->len = (a_byte_len + 3) >> 2;
	g_ecc_b->len = (b_byte_len + 3) >> 2;
	g_ecc_g->x->len = (gx_byte_len + 3) >> 2;
	g_ecc_g->y->len = (gy_byte_len + 3) >> 2;
	g_ecc_g->y->maxlen = g_ecc_p->maxlen;
	g_ecc_g->x->maxlen = g_ecc_g->y->maxlen;
	g_ecc_b->maxlen = g_ecc_g->x->maxlen;
	g_ecc_a->maxlen = g_ecc_b->maxlen;

	bignum_clr(g_ecc_p);
	bignum_clr(g_ecc_n);
	bignum_clr(g_ecc_h);
	bignum_clr(g_ecc_a);
	bignum_clr(g_ecc_b);
	bignum_clr(g_ecc_g->x);
	bignum_clr(g_ecc_g->y);

	g_ecc_n_byte_len = (bignum_getbitlen(g_ecc_n) + 7) >> 3;
	g_ecc_p_byte_len = (bignum_getbitlen(g_ecc_p) + 7) >> 3;

	if (g_ecc_a->data[0] == g_ecc_p->data[0] - 3)
		g_a_equ_p_sub3 = 1;

	bignum_mont_set(g_ecc_p_ctx, g_ecc_p);
	bignum_mont_set(g_ecc_n_ctx, g_ecc_n);

	bignum_mont_start(g_ecc_p_ctx, g_ecc_a, g_ecc_a);
	bignum_mont_start(g_ecc_p_ctx, g_ecc_b, g_ecc_b);
	bignum_mont_start(g_ecc_p_ctx, g_mont_one, g_mont_one);

	return 0;
}

int ecc_point_is_on_crv(const struct ecc_point *p)
{
	u32 r_data[ECC_MAX_WORD_LEN] = { 0 };
	u32 l_data[ECC_MAX_WORD_LEN] = { 0 };
	struct bignum r[1] = { { r_data, g_ecc_p->maxlen, 0 } };
	struct bignum l[1] = { { l_data, g_ecc_p->maxlen, 0 } };

	if (!p) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	bignum_mont_start(g_ecc_p_ctx, p->x, l);
	bignum_mont_mul(g_ecc_p_ctx, l, l, r);
	bignum_mont_mul(g_ecc_p_ctx, l, r, r);
	bignum_mont_mul(g_ecc_p_ctx, l, g_ecc_a, l);
	bignum_modadd(r, l, g_ecc_p, r);
	bignum_modadd(r, g_ecc_b, g_ecc_p, r);

	bignum_mont_start(g_ecc_p_ctx, p->y, l);
	bignum_mont_mul(g_ecc_p_ctx, l, l, l);

	if (bignum_cmp(r, l))
		return -EINVAL;

	return 0;
}

static int ecc_aff_2_j_pro(const struct ecc_point *src,
	struct ecc_j_pro_point *dst)
{
	bignum_cpy(src->x, dst->x);
	bignum_cpy(src->y, dst->y);
	dst->z->data[0] = 1;
	dst->z->len = 1;

	bignum_clr(dst->x);
	bignum_clr(dst->y);
	bignum_clr(dst->z);

	bignum_mont_start(g_ecc_p_ctx, dst->x, dst->x);
	bignum_mont_start(g_ecc_p_ctx, dst->y, dst->y);
	bignum_mont_start(g_ecc_p_ctx, dst->z, dst->z);

	return 0;
}

static int ecc_j_pro_2_aff(const struct ecc_j_pro_point *src,
	struct ecc_point *dst)
{
	u32 tmp_data[ECC_MAX_WORD_LEN];
	struct bignum tmp[1] = { { tmp_data, g_ecc_p->maxlen, 0 } };

	bignum_mont_finish(g_ecc_p_ctx, src->z, tmp);
	bignum_modinv(tmp, g_ecc_p, tmp);
	bignum_mont_start(g_ecc_p_ctx, tmp, dst->y);
	bignum_mont_mul(g_ecc_p_ctx, dst->y, dst->y, dst->x);
	bignum_mont_mul(g_ecc_p_ctx, dst->y, dst->x, dst->y);
	bignum_mont_mul(g_ecc_p_ctx, src->x, dst->x, dst->x);
	bignum_mont_mul(g_ecc_p_ctx, src->y, dst->y, dst->y);
	bignum_mont_finish(g_ecc_p_ctx, dst->x, dst->x);
	bignum_mont_finish(g_ecc_p_ctx, dst->y, dst->y);

	return 0;
}

static int ecc_j_pro_point_add(struct ecc_j_pro_point *a,
	struct ecc_j_pro_point *b)
{
	u32 t0_data[ECC_MAX_WORD_LEN];
	u32 t1_data[ECC_MAX_WORD_LEN];
	u32 t2_data[ECC_MAX_WORD_LEN];
	struct bignum t0[1] = { { t0_data, g_ecc_p->maxlen, 0 } };
	struct bignum t1[1] = { { t1_data, g_ecc_p->maxlen, 0 } };
	struct bignum t2[1] = { { t2_data, g_ecc_p->maxlen, 0 } };
	s32 ret;

	ret = bignum_cmp(b->z, g_mont_one);
	if (ret) {
		bignum_mont_mul(g_ecc_p_ctx, b->z, b->z, t2);
		bignum_mont_mul(g_ecc_p_ctx, a->x, t2, a->x); /* U0 */
		bignum_mont_mul(g_ecc_p_ctx, b->z, t2, t2);
		bignum_mont_mul(g_ecc_p_ctx, a->y, t2, a->y); /* S0 */
	}

	bignum_mont_mul(g_ecc_p_ctx, a->z, a->z, t2);
	bignum_mont_mul(g_ecc_p_ctx, b->x, t2, t0); /* U1 */
	bignum_mont_mul(g_ecc_p_ctx, a->z, t2, t2);
	bignum_mont_mul(g_ecc_p_ctx, b->y, t2, t1); /* S1 */
	bignum_modsub(a->x, t0, g_ecc_p, t0);
	bignum_modsub(a->y, t1, g_ecc_p, t1);

	if (t0->len == 0) {
		if (t1->len == 0) {
			bignum_setzero(a->x);
			bignum_setzero(a->y);
			bignum_setzero(a->z);
			return -EINVAL;
		}
		a->y->data[0] = 1;
		a->x->data[0] = a->y->data[0];
		a->x->len = 1;
		a->y->len = 1;
		bignum_clr(a->x);
		bignum_clr(a->y);
		bignum_setzero(a->z);
		bignum_mont_start(g_ecc_p_ctx, a->x, a->x);
		bignum_mont_start(g_ecc_p_ctx, a->y, a->y);
		return -EINVAL;
	}

	bignum_modmul2(a->x, g_ecc_p, a->x);
	bignum_modsub(a->x, t0, g_ecc_p, a->x); /* T */
	bignum_modmul2(a->y, g_ecc_p, a->y);
	bignum_modsub(a->y, t1, g_ecc_p, a->y); /* M */
	if (ret)
		bignum_mont_mul(g_ecc_p_ctx, a->z, b->z, a->z); /* Z0*Z1 */
	bignum_mont_mul(g_ecc_p_ctx, a->z, t0, a->z); /* X2 */
	bignum_mont_mul(g_ecc_p_ctx, t0, t0, t2); /* W^2 */
	bignum_mont_mul(g_ecc_p_ctx, t0, t2, t0); /* W^3 */
	bignum_mont_mul(g_ecc_p_ctx, a->x, t2, t2); /* T * W^2 */
	bignum_mont_mul(g_ecc_p_ctx, t1, t1, a->x); /* r^2 */
	bignum_modsub(a->x, t2, g_ecc_p, a->x); /* X2 */
	bignum_modmul2(a->x, g_ecc_p, a->x); /* 2 * X2 */
	bignum_modsub(t2, a->x, g_ecc_p, t2); /* V */
	bignum_moddiv2(a->x, g_ecc_p, a->x);
	bignum_mont_mul(g_ecc_p_ctx, t1, t2, t1); /* V*r */
	bignum_mont_mul(g_ecc_p_ctx, a->y, t0, t0); /* M*W^3 */
	bignum_modsub(t1, t0, g_ecc_p, a->y); /* 2*Y2 */
	bignum_moddiv2(a->y, g_ecc_p, a->y);

	return 0;
}

static int ecc_j_pro_point_dbl(struct ecc_j_pro_point *a)
{
	u32 t0_data[ECC_MAX_WORD_LEN] = { 0 };
	u32 t1_data[ECC_MAX_WORD_LEN] = { 0 };
	u32 t2_data[ECC_MAX_WORD_LEN];
	struct bignum t0[1] = { { t0_data, g_ecc_p->maxlen, 0 } };
	struct bignum t1[1] = { { t1_data, g_ecc_p->maxlen, 0 } };
	struct bignum t2[1] = { { t2_data, g_ecc_p->maxlen, 0 } };

	if (a->y->len == 0) {
		a->y->data[0] = 1;
		a->x->data[0] = a->y->data[0];
		a->x->len = 1;
		a->y->len = 1;
		bignum_clr(a->x);
		bignum_clr(a->y);
		bignum_setzero(a->z);
		bignum_mont_start(g_ecc_p_ctx, a->x, a->x);
		bignum_mont_start(g_ecc_p_ctx, a->y, a->y);
		return -EINVAL;
	}

	if (g_a_equ_p_sub3) {
		bignum_mont_mul(g_ecc_p_ctx, a->z, a->z, t0);
		bignum_modsub(a->x, t0, g_ecc_p, t1);
		bignum_modadd(t0, a->x, g_ecc_p, t0);
		bignum_mont_mul(g_ecc_p_ctx, t0, t1, t1);
		bignum_modmul2(t1, g_ecc_p, t0);
		bignum_modadd(t0, t1, g_ecc_p, t0);
	} else if (g_ecc_a->len == 0) {
		bignum_mont_mul(g_ecc_p_ctx, a->x, a->x, t2);
		bignum_modmul2(t2, g_ecc_p, t0);
		bignum_modadd(t0, t2, g_ecc_p, t0);
	} else {
		bignum_mont_mul(g_ecc_p_ctx, a->z, a->z, t1);
		bignum_mont_mul(g_ecc_p_ctx, t1, t1, t1);
		bignum_mont_mul(g_ecc_p_ctx, t1, g_ecc_a, t1);
		bignum_mont_mul(g_ecc_p_ctx, a->x, a->x, t2);
		bignum_modmul2(t2, g_ecc_p, t0);
		bignum_modadd(t0, t2, g_ecc_p, t0);
		bignum_modadd(t0, t1, g_ecc_p, t0);
	}

	bignum_mont_mul(g_ecc_p_ctx, a->y, a->z, a->z);
	bignum_modmul2(a->z, g_ecc_p, a->z);
	bignum_mont_mul(g_ecc_p_ctx, a->y, a->y, a->y);
	bignum_mont_mul(g_ecc_p_ctx, a->x, a->y, t1);
	bignum_modmul2(t1, g_ecc_p, t1);
	bignum_modmul2(t1, g_ecc_p, t1);
	bignum_mont_mul(g_ecc_p_ctx, t0, t0, a->x);
	bignum_modmul2(t1, g_ecc_p, t2);
	bignum_modsub(a->x, t2, g_ecc_p, a->x);
	bignum_mont_mul(g_ecc_p_ctx, a->y, a->y, a->y);
	bignum_modmul2(a->y, g_ecc_p, a->y);
	bignum_modmul2(a->y, g_ecc_p, a->y);
	bignum_modmul2(a->y, g_ecc_p, a->y);
	bignum_modsub(t1, a->x, g_ecc_p, t1);
	bignum_mont_mul(g_ecc_p_ctx, t0, t1, t1);
	bignum_modsub(t1, a->y, g_ecc_p, a->y);

	return 0;
}

int ecc_point_add(const struct ecc_point *a, const struct ecc_point *b,
	struct ecc_point *c)
{
	u32 a_xdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 a_ydata[ECC_MAX_WORD_LEN] = { 0 };
	u32 a_zdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 b_xdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 b_ydata[ECC_MAX_WORD_LEN] = { 0 };
	u32 b_zdata[ECC_MAX_WORD_LEN] = { 0 };
	struct ecc_j_pro_point a1[1] = { { { { a_xdata, g_ecc_p->maxlen, 0 } },
		{ { a_ydata, g_ecc_p->maxlen, 0 } },
		{ { a_zdata, g_ecc_p->maxlen, 0 } } } };
	struct ecc_j_pro_point b1[1] = { { { { b_xdata, g_ecc_p->maxlen, 0 } },
		{ { b_ydata, g_ecc_p->maxlen, 0 } },
		{ { b_zdata, g_ecc_p->maxlen, 0 } } } };

	if (!a || !b || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	ecc_aff_2_j_pro(a, a1);
	ecc_aff_2_j_pro(b, b1);

	if (bignum_cmp(a1->x, b1->x) || bignum_cmp(a1->y, b1->y))
		ecc_j_pro_point_add(a1, b1);
	else
		ecc_j_pro_point_dbl(a1);

	ecc_j_pro_2_aff(a1, c);

	return 0;
}

int ecc_point_mul(const struct bignum *k, const struct ecc_point *p,
	struct ecc_point *r)
{
	u32 i;
	u32 j0_xdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 j0_ydata[ECC_MAX_WORD_LEN] = { 0 };
	u32 j0_zdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 j1_xdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 j1_ydata[ECC_MAX_WORD_LEN] = { 0 };
	u32 j1_zdata[ECC_MAX_WORD_LEN] = { 0 };
	struct ecc_j_pro_point j0[1] = { { { { j0_xdata, g_ecc_p->maxlen, 0 } },
		{ { j0_ydata, g_ecc_p->maxlen, 0 } },
		{ { j0_zdata, g_ecc_p->maxlen, 0 } } } };
	struct ecc_j_pro_point j1[1] = { { { { j1_xdata, g_ecc_p->maxlen, 0 } },
		{ { j1_ydata, g_ecc_p->maxlen, 0 } },
		{ { j1_zdata, g_ecc_p->maxlen, 0 } } } };

	if (!k || !p || !r) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (k->len == 0) {
		bignum_setzero(r->x);
		bignum_setzero(r->y);
		return -EINVAL;
	}

	ecc_aff_2_j_pro(p, j0);
	ecc_aff_2_j_pro(p, j1);

	i = bignum_getbitlen(k) - 1;

	while (i) {
		i--;
		ecc_j_pro_point_dbl(j1);
		if (bignum_getbitval(k, i))
			ecc_j_pro_point_add(j1, j0);
	}

	ecc_j_pro_2_aff(j1, r);

	if (ecc_point_is_on_crv(r))
		return -EINVAL;
	return 0;
}
