/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bignum_mod.c
 *
 * bignum-mod-calculate
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

#include <linux/string.h>
#include <linux/err.h>
#include "bignum.h"

#define max(a, b) ((a) >= (b) ? (a) : (b))

int bignum_modadd(const struct bignum *a, const struct bignum *b,
	const struct bignum *n, struct bignum *c)
{
	struct bignum tmp[1];
	u32 tmpdata[MAXWORDLEN + 1] = { 0 };

	if (!a || !b || !n || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmp->data = tmpdata;
	tmp->maxlen = max(a->len, b->len) + 1;

	bignum_add(a, b, tmp);
	return bignum_mod(tmp, n, c);
}

int bignum_modsub(const struct bignum *a, const struct bignum *b,
	const struct bignum *n, struct bignum *c)
{
	struct bignum tmp[1];
	u32 tmpdata[MAXWORDLEN + 1];

	if (!a || !b || !n || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmp->data = tmpdata;
	tmp->maxlen = max(a->len, n->len) + 1;

	if (bignum_cmp(a, b) >= 0) {
		bignum_sub(a, b, c);
	} else {
		bignum_add(a, n, tmp);
		bignum_sub(tmp, b, c);
	}
	return 0;
}

int bignum_modmul(const struct bignum *a, const struct bignum *b,
	const struct bignum *n, struct bignum *c)
{
	struct bignum tmp[1];
	u32 tmpdata[MAXWORDLEN << 1];

	if (!a || !b || !n || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmp->data = tmpdata;
	tmp->maxlen = a->len + b->len;
	tmp->len = 0;

	bignum_mul(a, b, tmp);
	return bignum_mod(tmp, n, c);
}

int bignum_modmul2(const struct bignum *a,
	const struct bignum *n, struct bignum *c)
{
	struct bignum tmp[1];
	u32 tmpdata[MAXWORDLEN + 1];

	if (!a || !n || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmp->data = tmpdata;
	tmp->maxlen = a->len + 1;
	tmp->len = 0;

	bignum_shl(a, 1, tmp);
	if (bignum_cmp(tmp, n) >= 0)
		return bignum_sub(tmp, n, c);

	bignum_cpy(tmp, c);

	return 0;
}

int bignum_moddiv2(const struct bignum *a,
	const struct bignum *n, struct bignum *c)
{
	struct bignum tmp[1];
	u32 tmpdata[MAXWORDLEN + 1];

	if (!a || !n || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmp->data = tmpdata;
	tmp->maxlen = n->len + 1;
	tmp->len = 0;

	if (a->data[0] & 1) {
		if ((n->data[0] & 1) == 0)
			return -EINVAL;

		bignum_add(a, n, tmp);
		bignum_shr(tmp, 1, c);
	} else {
		bignum_shr(a, 1, c);
	}
	return 0;
}

static int bignum_modinv_even(const struct bignum *a,
	const struct bignum *n, struct bignum *c)
{
	u32 a1_data[MAXWORDLEN], u_data[MAXWORDLEN], v_data[MAXWORDLEN];
	u32 a_data[MAXWORDLEN + 1], b_data[MAXWORDLEN + 1];
	u32 c_data[MAXWORDLEN + 1], d_data[MAXWORDLEN + 1];
	u32 a_sign = 0;
	u32 b_sign = 0;
	u32 c_sign = 0;
	u32 d_sign = 0;
	struct bignum a1[1], u[1], v[1], a2[1], b2[1], c2[1], d2[1];
	s32 ret;

	a1->data = a1_data;
	a1->maxlen = n->len;
	bignum_mod(a, n, a1);

	u->data = u_data;
	u->maxlen = n->len;
	bignum_cpy(a1, u);

	v->data = v_data;
	v->maxlen = n->len;
	bignum_cpy(n, v);

	a2->data = a_data;
	a2->maxlen = n->len + 1;
	a2->data[0] = 1;
	a2->len = 1;
	bignum_clr(a2);

	b2->data = b_data;
	b2->maxlen = n->len + 1;
	bignum_setzero(b2);

	c2->data = c_data;
	c2->maxlen = n->len + 1;
	bignum_setzero(c2);

	d2->data = d_data;
	d2->maxlen = n->len + 1;
	d2->data[0] = 1;
	d2->len = 1;
	bignum_clr(d2);

	while (((u->data[0] != 1) || (u->len != 1)) &&
		((v->data[0] != 1) || (v->len != 1))) {
		while ((u->data[0] & 1) == 0) {
			bignum_shr(u, 1, u);
			if ((a2->data[0] & 1) || (b2->data[0] & 1)) {
				bignum_signedadd(a2, a_sign, n, 0, a2, &a_sign);
				bignum_signedsub(b2, b_sign, a1, 0, b2, &b_sign);
			}
			bignum_shr(a2, 1, a2);
			bignum_shr(b2, 1, b2);
		}
		while ((v->data[0] & 1) == 0) {
			bignum_shr(v, 1, v);
			if ((c2->data[0] & 1) || (d2->data[0] & 1)) {
				bignum_signedadd(c2, c_sign, n, 0, c2, &c_sign);
				bignum_signedsub(d2, d_sign, a1, 0, d2, &d_sign);
			}
			bignum_shr(c2, 1, c2);
			bignum_shr(d2, 1, d2);
		}

		ret = bignum_cmp(u, v);
		if (ret > 0) {
			bignum_sub(u, v, u);
			bignum_signedsub(a2, a_sign, c2, c_sign, a2, &a_sign);
			bignum_signedsub(b2, b_sign, d2, d_sign, b2, &b_sign);
		} else if (ret < 0) {
			bignum_sub(v, u, v);
			bignum_signedsub(c2, c_sign, a2, a_sign, c2, &c_sign);
			bignum_signedsub(d2, d_sign, b2, b_sign, d2, &d_sign);
		} else {
			break;
		}
	}

	if ((u->data[0] == 1) && (u->len == 1)) {
		if (a_sign) {
			bignum_mod(a2, n, a2);
			return bignum_sub(n, a2, c);
		}
			return bignum_mod(a2, n, c);
	} else if ((v->data[0] == 1) && (v->len == 1)) {
		if (c_sign) {
			bignum_mod(c2, n, c2);
			return bignum_sub(n, c2, c);
		}
			return bignum_mod(c2, n, c);
	}

	return -EINVAL;
}

static int bignum_modinv_odd(const struct bignum *a,
	const struct bignum *n, struct bignum *c)
{
	u32 u_data[MAXWORDLEN], v_data[MAXWORDLEN], x1_data[MAXWORDLEN], x2_data[MAXWORDLEN];
	struct bignum u[1], v[1], x1[1], x2[1];
	s32 ret;

	u->data = u_data;
	u->maxlen = n->len;
	bignum_mod(a, n, u);

	v->data = v_data;
	v->maxlen = n->len;
	bignum_cpy(n, v);

	x1->data = x1_data;
	x1->maxlen = n->len;
	x1->data[0] = 1;
	x1->len = 1;
	bignum_clr(x1);

	x2->data = x2_data;
	x2->maxlen = n->len;
	bignum_setzero(x2);

	while (((u->data[0] != 1) || (u->len != 1)) &&
		((v->data[0] != 1) || (v->len != 1))) {
		while ((u->data[0] & 1) == 0) {
			bignum_shr(u, 1, u);
			bignum_moddiv2(x1, n, x1);
		}
		while ((v->data[0] & 1) == 0) {
			bignum_shr(v, 1, v);
			bignum_moddiv2(x2, n, x2);
		}

		ret = bignum_cmp(u, v);
		if (ret > 0) {
			bignum_sub(u, v, u);
			bignum_modsub(x1, x2, n, x1);
		} else if (ret < 0) {
			bignum_sub(v, u, v);
			bignum_modsub(x2, x1, n, x2);
		} else {
			break;
		}
	}

	if ((u->data[0] == 1) && (u->len == 1)) {
		bignum_mod(x1, n, c);
		return 0;
	} else if ((v->data[0] == 1) && (v->len == 1)) {
		bignum_mod(x2, n, c);
		return 0;
	}
	return -EINVAL;
}

int bignum_modinv(const struct bignum *a,
	const struct bignum *n, struct bignum *c)
{
	if (!a || !n || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (a->len == 0 || n->len == 0)
		return -EINVAL;

	if (n->data[0] & 1)
		return bignum_modinv_odd(a, n, c);
	else
		return bignum_modinv_even(a, n, c);
}
