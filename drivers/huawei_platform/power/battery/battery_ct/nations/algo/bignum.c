/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bignum.c
 *
 * bignum calulate
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

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "bignum.h"

#define singleadd(a, b, c, carry)               \
do {                                            \
	(c) = (a) + (carry);                    \
	if (c)                                  \
		(carry) = 0;                    \
	(c) += (b);                             \
	if ((c) < (b))                          \
		(carry) = 1;                    \
} while (0)

#define singlesub(a, b, c, borrow)              \
do {                                            \
	(c) = (b) + (borrow);                   \
	if ((c) >= (borrow)) {                  \
		(borrow) = (a) >= (c) ? 0 : 1;  \
		(c) = (a) - (c);                \
	} else {                                \
		(c) = (a);                      \
	}                                       \
} while (0)

static void singlemul(u32 a, u32 b, u32 *h, u32 *l)
{
	u64 m = (u64)a * b;

	*h = (u32)(m >> 32);
	*l = (u32)m;
}

static void singlesqu(u32 a, u32 b, u32 *h, u32 *l, u32 *r2)
{
	u64 m = (u64)a * b;

	if (m >> 63)
		(*r2)++;

	*h = (u32)(m >> 31);
	*l = (u32)(m << 1);
}

int bignum_clr(struct bignum *n)
{
	s32 i;

	if (!n) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	i = (s32)n->len - 1;

	memset(n->data + n->len, 0, (n->maxlen - n->len) << 2);

	while (i >= 0 && n->data[i] == 0)
		i--;

	n->len = (u32)i + 1;

	return 0;
}

u32 bignum_getbitval(const struct bignum *n, u32 i)
{
	u32 x = i >> 5;
	u32 y = 1 << (i & 31);

	if (!n) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return 0;
	}

	if (x >= n->len)
		return 0;

	return (n->data[x] & y) ? 1 : 0;
}

u32 bignum_getbitlen(const struct bignum *n)
{
	s32 i;

	if (!n) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return 0;
	}

	i = n->len - 1;

	while (i >= 0 && n->data[i] == 0)
		i--;

	if (i < 0)
		return 0;

	return ((i << 5) + u32_getbitlen(n->data[i]));
}

s32 bignum_cmp(const struct bignum *a, const struct bignum *b)
{
	s32 i;

	if (!a || !b) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (a->len > b->len)
		return 1;

	if (b->len > a->len)
		return -1;

	for (i = a->len - 1; i >= 0; i--) {
		if (a->data[i] > b->data[i])
			return 1;

		if (a->data[i] < b->data[i])
			return -1;
	}
	return 0;
}

int bignum_cpy(const struct bignum *src, struct bignum *dst)
{
	if (!src || !dst) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	dst->len = min(src->len, dst->maxlen);
	memcpy(dst->data, src->data, dst->len << 2);
	bignum_clr(dst);
	return 0;
}

int bignum_setzero(struct bignum *n)
{
	if (!n)
		return -EINVAL;

	memset(n->data, 0, n->maxlen << 2);
	n->len = 0;
	return 0;
}

int bignum_add(const struct bignum *a, const struct bignum *b, struct bignum *c)
{
	u32 i;
	u32 carry = 0;
	u32 tmp, tmplen;
	const struct bignum *swap = NULL;

	if (!a || !b || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (a->len < b->len) {
		swap = a;
		a = b;
		b = swap;
	}

	tmplen = min(b->len, c->maxlen);

	for (i = 0; i < tmplen; i++) {
		singleadd(a->data[i], b->data[i], tmp, carry);
		c->data[i] = tmp;
	}

	tmplen = min(a->len, c->maxlen);

	for (i = b->len; i < tmplen; i++) {
		c->data[i] = a->data[i] + carry;
		if (carry && c->data[i])
			carry = 0;
	}

	if (carry && tmplen < c->maxlen)
		c->data[tmplen++] = 1;

	c->len = tmplen;
	bignum_clr(c);

	return 0;
}

int bignum_signedadd(const struct bignum *a, u32 asign, const struct bignum *b,
	u32 bsign, struct bignum *c, u32 *csign)
{
	if (!a || !b || !c || !csign) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (asign == bsign) {
		*csign = asign;
		return bignum_add(a, b, c);
	} else if (bignum_cmp(a, b) >= 0) {
		*csign = asign;
		return bignum_sub(a, b, c);
	}
	*csign = bsign;
	return bignum_sub(b, a, c);
}

int bignum_addone(const struct bignum *a, struct bignum *c)
{
	u32 i;

	if (!a || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	c->len = min(a->len, c->maxlen);

	for (i = 0; i < c->len; i++) {
		c->data[i] = a->data[i] + 1;
		if (c->data[i])
			break;
	}

	if (i == c->len) {
		if (c->len < c->maxlen)
			c->data[c->len++] = 1;
	} else if (i < c->len) {
		i++;
		memcpy(c->data + i, a->data + i, (c->len - i) << 2);
	} else {
		return -EINVAL;
	}

	bignum_clr(c);

	return 0;
}

int bignum_sub(const struct bignum *a, const struct bignum *b, struct bignum *c)
{
	u32 i;
	u32 borrow = 0;
	u32 tmp, tmplen;
	s32 ret;

	if (!a || !b || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	ret = bignum_cmp(a, b);
	if (ret <= 0) {
		bignum_setzero(c);
		return (ret == 0 ? 0 : -EINVAL);
	}

	tmplen = min(b->len, c->maxlen);

	for (i = 0; i < tmplen; i++) {
		singlesub(a->data[i], b->data[i], tmp, borrow);
		c->data[i] = tmp;
	}

	tmplen = min(a->len, c->maxlen);

	for (i = b->len; i < tmplen; i++) {
		c->data[i] = a->data[i] - borrow;
		if (borrow && c->data[i] != (u32)-1)
			borrow = 0;
	}

	c->len = tmplen;
	bignum_clr(c);

	return 0;
}

int bignum_signedsub(const struct bignum *a, u32 asign, const struct bignum *b,
	u32 bsign, struct bignum *c, u32 *csign)
{
	if (!a || !b || !c || !csign) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (asign == bsign) {
		if (bignum_cmp(a, b) >= 0) {
			*csign = asign;
			return bignum_sub(a, b, c);
		}
		*csign = !asign;
		return bignum_sub(b, a, c);
	}
	*csign = asign;
	return bignum_add(a, b, c);
}

int bignum_subone(const struct bignum *a, struct bignum *c)
{
	u32 i;
	u32 tmplen;

	if (!a || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (a->len == 0) {
		c->len = 0;
		bignum_clr(c);
		return -EINVAL;
	}

	tmplen = min(a->len, c->maxlen);

	for (i = 0; i < tmplen; i++) {
		c->data[i] = a->data[i] - 1;
		if (c->data[i] != (u32)-1)
			break;
	}

	if (i >= a->len) {
		c->len = 0;
		bignum_clr(c);
		return -EINVAL;
	} else if (i < c->len) {
		i++;
		memcpy(c->data + i, a->data + i, (c->len - i) << 2);
	}

	c->len = tmplen;
	bignum_clr(c);

	return 0;
}

int bignum_mul(const struct bignum *a, const struct bignum *b, struct bignum *c)
{
	u32 k, i, j;
	u32 r0 = 0;
	u32 r1 = 0;
	u32 r2 = 0;
	u32 u, v, carry;
	u32 tmp[MAXWORDLEN << 1];
	u32 tmplen;

	if (!a || !b || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmplen = a->len + b->len - 1;

	if (a->len == 0 || b->len == 0) {
		bignum_setzero(c);
		return 0;
	}

	for (k = 0; k < tmplen; k++) {
		i = k < b->len ? 0 : (k - b->len + 1);
		j = k - i;
		while (i < a->len && j < b->len) {
			carry = 0;
			singlemul(a->data[i], b->data[j], &u, &v);
			singleadd(r0, v, r0, carry);
			singleadd(r1, u, r1, carry);
			r2 += carry;
			i++;
			j--;
		}
		tmp[k] = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}

	tmp[tmplen++] = r0;

	c->len = min(tmplen, c->maxlen);
	memcpy(c->data, tmp, c->len << 2);
	bignum_clr(c);

	return 0;
}

int bignum_squ(const struct bignum *a, struct bignum *c)
{
	u32 k, i, j;
	u32 r0 = 0;
	u32 r1 = 0;
	u32 r2 = 0;
	u32 u, v, carry;
	u32 tmp[MAXWORDLEN << 1];
	u32 tmplen;

	if (!a || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	tmplen = (a->len << 1) - 1;
	if (a->len == 0) {
		bignum_setzero(c);
		return 0;
	}

	for (k = 0; k < tmplen; k++) {
		i = k < a->len ? 0 : (k - a->len + 1);
		j = k - i;
		while (i < a->len && j < a->len && i <= j) {
			carry = 0;
			if (i < j)
				singlesqu(a->data[i], a->data[j], &u, &v, &r2);
			else
				singlemul(a->data[i], a->data[j], &u, &v);

			singleadd(r0, v, r0, carry);
			singleadd(r1, u, r1, carry);
			r2 += carry;
			i++;
			j--;
		}
		tmp[k] = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}

	tmp[tmplen++] = r0;

	c->len = min(tmplen, c->maxlen);
	memcpy(c->data, tmp, c->len << 2);
	bignum_clr(c);

	return 0;
}

int bignum_shl(const struct bignum *a, u32 len, struct bignum *c)
{
	s32 i;
	u32 wordlen = len >> 5;
	u32 bitlen = len & 31;

	if (!a || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (bitlen) {
		if (c->maxlen > a->len + wordlen) {
			c->len = a->len + wordlen + 1;
			c->data[c->len - 1] = 0;
		} else {
			c->len = c->maxlen;
			c->data[c->len - 1] =
				a->data[c->len - wordlen - 1] << bitlen;
		}

		for (i = c->len - wordlen - 2; i >= 0; i--) {
			c->data[i + wordlen + 1] |= a->data[i] >> (32 - bitlen);
			c->data[i + wordlen] = a->data[i] << bitlen;
		}
	} else {
		c->len = min(a->len + wordlen, c->maxlen);
		for (i = c->len - wordlen - 1; i >= 0; i--)
			c->data[i + wordlen] = a->data[i];
	}

	if (c->maxlen > wordlen)
		memset(c->data, 0, wordlen << 2);

	bignum_clr(c);

	return 0;
}

int bignum_shr(const struct bignum *a, u32 len, struct bignum *c)
{
	u32 i;
	u32 wordlen = len >> 5;
	u32 bitlen = len & 31;
	u32 tmplen;

	if (!a || !c) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	if (c->maxlen == 0)
		return -EINVAL;

	if (len >= bignum_getbitlen(a)) {
		bignum_setzero(c);
		return 0;
	}

	tmplen = min(a->len - wordlen, c->maxlen);

	if (bitlen) {
		c->data[0] = a->data[wordlen] >> bitlen;
		for (i = 1; i < tmplen; i++) {
			c->data[i - 1] |= a->data[i + wordlen] << (32 - bitlen);
			c->data[i] = a->data[i + wordlen] >> bitlen;
		}
		if (tmplen < a->len - wordlen)
			c->data[c->len - 1] |=
				a->data[c->len + wordlen] << (32 - bitlen);
	} else {
		for (i = wordlen; i < tmplen; i++)
			c->data[i] = a->data[i + wordlen];
	}

	c->len = tmplen;
	bignum_clr(c);

	return 0;
}

static void bignum_div_word(const struct bignum *a, u32 b,
	u32 *q, unsigned int q_len,
	u32 *r, unsigned int r_len)
{
	s32 i;
	u64 dbla;
	u64 tmpr = 0;

	if ((r_len == 0) || (q_len == 0)) {
		hwlog_err("%s: len invalid\n", __func__);
		return;
	}

	for (i = a->len - 1; i >= 0; i--) {
		dbla = (tmpr << 32) | a->data[i];
		if (b == 0) {
			hwlog_err("%s: The dividend is 0\n", __func__);
			return;
		}
		q[i] = (u32)(dbla / b);
		tmpr = dbla % b;
	}
	*r = (u32)tmpr;
}

static void bignum_div_words(const struct bignum *a, const struct bignum *b,
	struct bignum *q, struct bignum *r, struct bignum *tr,
	u32 *tqdata, u32 *trdata, u32 *qwordlen)
{
	u32 tmpwordlen;
	u32 x, y;
	s32 i;

	memset(tqdata, 0, *qwordlen << 2);
	tr->data = trdata;
	tr->maxlen = b->len + 1;
	if (u32_getbitlen(a->data[a->len - 1]) >
		u32_getbitlen(b->data[b->len - 1])) {
		tr->len = b->len - 1;
		tqdata[*qwordlen] = 0;
		(*qwordlen)++;
	} else {
		tr->len = b->len;
	}

	tmpwordlen = *qwordlen;
	memcpy(trdata, a->data + tmpwordlen, tr->len << 2);
	if (bignum_cmp(tr, b) >= 0) {
		bignum_sub(tr, b, tr);
		tqdata[tmpwordlen] = 1;
		(*qwordlen)++;
	}
	for (i = (tmpwordlen << 5) - 1; i >= 0; i--) {
		bignum_shl(tr, 1, tr);
		x = i >> 5;
		y = 1 << (i & 31);
		if (a->data[x] & y)
			tr->data[0] |= 1;
		if (bignum_cmp(tr, b) >= 0) {
			bignum_sub(tr, b, tr);
			tqdata[x] |= y;
		}
	}
	if (q) {
		q->len = q->maxlen <= *qwordlen ? q->maxlen : *qwordlen;
		memcpy(q->data, tqdata, q->len << 2);
		bignum_clr(q);
	}
	if (r) {
		r->len = r->maxlen <= tr->len ? r->maxlen : tr->len;
		memcpy(r->data, trdata, r->len << 2);
		bignum_clr(r);
	}
}

int bignum_div(const struct bignum *a, const struct bignum *b, struct bignum *q,
	struct bignum *r)
{
	struct bignum tr[1];
	u32 tqdata[MAXWORDLEN], trdata[MAXWORDLEN + 1];
	u32 qwordlen;
	s32 ret;

	if (!a || !b) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	qwordlen = a->len - b->len;
	if (b->len == 0) {
		q->len = 0;
		bignum_clr(q);
		r->len = 0;
		bignum_clr(r);
		return -EINVAL;
	}

	if (b->len == 1) {
		bignum_div_word(a, b->data[0], tqdata, sizeof(tqdata), trdata, sizeof(trdata));
		if (q) {
			q->len = q->maxlen <= a->len ? q->maxlen : a->len;
			memcpy(q->data, tqdata, q->len << 2);
			bignum_clr(q);
		}
		if (r && r->maxlen) {
			r->len = 1;
			r->data[0] = trdata[0];
			bignum_clr(r);
		}
		return 0;
	}

	ret = bignum_cmp(a, b);
	if (ret == -1) {
		if (q) {
			q->len = 0;
			bignum_clr(q);
		}
		if (r && a != r) {
			r->len = r->maxlen <= a->len ? r->maxlen : a->len;
			memcpy(r->data, a->data, r->len << 2);
			bignum_clr(r);
		}
		return 0;
	} else if (ret == 0) {
		if (q && q->maxlen) {
			q->len = 1;
			q->data[0] = 1;
			bignum_clr(q);
		}
		if (r) {
			r->len = 0;
			bignum_clr(r);
		}
		return 0;
	}

	bignum_div_words(a, b, q, r, tr, tqdata, trdata, &qwordlen);
	return 0;
}
