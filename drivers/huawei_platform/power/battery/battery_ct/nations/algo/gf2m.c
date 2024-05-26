/* SPDX-License-Identifier: GPL-2.0 */
/*
 * gf2m.c
 *
 * gf2m function
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
#include "gf2m.h"
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "bignum.h"
#include "gf2m_common.h"

static const u32 g_gf2m_f[] = {
	0x000000C9, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000008, 0
};
static const u32 g_gf2m_a[] = {
	0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0
};
static const u32 g_gf2m_b[] = {
	0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0
};
static const u32 g_gf2m_g0x[] = {
	0x5C94EEE8, 0xDE4E6D5E, 0xAA07D793, 0x7BBC11AC, 0xFE13C053, 0x00000002, 0
};
static const u32 g_gf2m_g0y[] = {
	0xCCDAA3D9, 0x0536D538, 0x321F2E80, 0x5D38FF58, 0x89070FB0, 0x00000002, 0
};
static const u32 g_gf2m_n[] = {
	0x99F8A5EF, 0xA2E0CC0D, 0x00020108, 0x00000000, 0x00000000, 0x00000004, 0
};
static const u32 g_gf2m_h[] = {
	0x00000002
};

static struct gf2m_bigint g_gf2m_c;

#define array_len(A)        (((A) + 31) / 32)
#define double_array_len(A) ((2 * (A) + 31) / 32)
typedef unsigned int dwordvec_t[array_len(GF2M_NUM_BIT)];
typedef unsigned int double_dwordvec_t[double_array_len(GF2M_NUM_BIT)];

/* constant field element with value 0 */
static const dwordvec_t zero_element = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

static u32 gf2m_calc_bit_len(struct gf2m_bigint *a);

static void dwordvec_163_copy(dwordvec_t copy, const dwordvec_t in)
{
	copy[0] = in[0];
	copy[1] = in[1];
	copy[2] = in[2];
	copy[3] = in[3];
	copy[4] = in[4];
	copy[5] = in[5];
}

static void dwordvec_l_shift_163(dwordvec_t out, const dwordvec_t in)
{
	out[5] = (in[5] << 1) | (in[4] >> 31);
	out[4] = (in[4] << 1) | (in[3] >> 31);
	out[3] = (in[3] << 1) | (in[2] >> 31);
	out[2] = (in[2] << 1) | (in[1] >> 31);
	out[1] = (in[1] << 1) | (in[0] >> 31);
	out[0] = (in[0] << 1);
}

static void gf2_163_add(dwordvec_t out, const dwordvec_t op1,
	const dwordvec_t op2)
{
	out[0] = op1[0] ^ op2[0];
	out[1] = op1[1] ^ op2[1];
	out[2] = op1[2] ^ op2[2];
	out[3] = op1[3] ^ op2[3];
	out[4] = op1[4] ^ op2[4];
	out[5] = op1[5] ^ op2[5];
}

static void precompute(dwordvec_t table[], const dwordvec_t el)
{
	dwordvec_163_copy(table[0], zero_element);
	dwordvec_163_copy(table[1], el);
	dwordvec_l_shift_163(table[2], table[1]);
	dwordvec_l_shift_163(table[4], table[2]);
	dwordvec_l_shift_163(table[8], table[4]);
	gf2_163_add(table[3], table[2], table[1]);
	gf2_163_add(table[5], table[4], table[1]);
	gf2_163_add(table[6], table[4], table[2]);
	gf2_163_add(table[7], table[4], table[3]);
	gf2_163_add(table[9], table[8], table[1]);
	gf2_163_add(table[10], table[8], table[2]);
	gf2_163_add(table[11], table[8], table[3]);
	gf2_163_add(table[12], table[8], table[4]);
	gf2_163_add(table[13], table[8], table[5]);
	gf2_163_add(table[14], table[8], table[6]);
	gf2_163_add(table[15], table[8], table[7]);
}

static void gf2_163_reduction(dwordvec_t out, const double_dwordvec_t temp)
{
	int i;
	u32 t;
	u32 t1 = 0UL;
	u32 t2 = 0UL;
	u32 t3;

	/*
	 * reduction modulo x^163+x^7+x^6+x^3+1
	 * x^192 = x^36+x^35+x^32+x^29
	 */
	for (i = 0; i < double_array_len(GF2M_NUM_BIT) - array_len(GF2M_NUM_BIT); i++) {
		t = temp[i + array_len(GF2M_NUM_BIT)];
		t1 ^= t << 29;                      /* x^29 */
		t2 ^= (t >> 3) ^ t ^ (t << 3) ^ (t << 4); /* x^29+x^32+x^35+x^36 */
		t3 = (t >> 29) ^ (t >> 28);               /* x^35+x^36 */
		out[i] = temp[i] ^ t1;
		t1 = t2;
		t2 = t3;
	}

	t2 = temp[array_len(GF2M_NUM_BIT) - 1] ^ t1;
	t = t2 & 0xFFFFFFF8UL;
	out[array_len(GF2M_NUM_BIT) - 1] = t2 & 0x7UL;
	out[0] ^= (t >> 3) ^ t ^ (t << 3) ^ (t << 4); /* 1+x^3+x^6+x^7 */
	out[1] ^= (t >> 29) ^ (t >> 28);              /* x^6+x^7 */
}

static void gf2_163_mul(dwordvec_t out, const dwordvec_t op1,
	const dwordvec_t op2)
{
	dwordvec_t table[16];
	double_dwordvec_t temp;
	int i;
	unsigned int j;
	int t;

	/* precomputation */
	precompute(table, op2);

	for (i = 0; i < double_array_len(GF2M_NUM_BIT); i++)
		temp[i] = 0;

	/* multiplication */
	for (j = 28; j > 0; j -= 4) {
		for (i = 0; i < array_len(GF2M_NUM_BIT) - 1; i++) {
			t = (op1[i] >> j) & 0xf;
			gf2_163_add(temp + i, temp + i, table[t]);
		}

		for (i = double_array_len(GF2M_NUM_BIT) - 1; i > 0; i--)
			temp[i] = (temp[i] << 4) | (temp[i - 1] >> 28);

		temp[0] <<= 4;
	}

	for (i = 0; i < array_len(GF2M_NUM_BIT); i++) {
		t = op1[i] & 0xf;
		gf2_163_add(temp + i, temp + i, table[t]);
	}

	/* reduction */
	gf2_163_reduction(out, temp);
}

/* c = a - b; must a>=b */
static void bigint_sub(struct gf2m_bigint *a, struct gf2m_bigint *b,
	struct gf2m_bigint *c)
{
	struct gf2m_bigint u;
	u32 temp;
	int carry = 0;
	u16 i;

	for (i = 0; i < GF2M_MAX_LONG; i++) {
		temp = b->d[i] + carry;
		u.d[i] = a->d[i] - temp;
		/* overflow handling */
		if (((temp == 0) && (carry != 0)) || (a->d[i] < temp))
			carry = 1;
		else
			carry = 0;
	}

	temp = b->d[0] + carry;
	u.d[0] = a->d[0] - temp;

	*c = u;
}

/* judge if a[] is bigger than b[], for substract */
static s8 judge_bigint(struct gf2m_bigint *a, struct gf2m_bigint *b)
{
	s16 i;

	for (i = GF2M_MAX_LONG - 1; i >= 0; i--) {
		if (a->d[i] > b->d[i])
			return 1;
		else if (a->d[i] < b->d[i])
			return -1;
	}
	return 0;
}

static void bigint_mod_val(struct gf2m_bigint *in, struct gf2m_bigint *p,
	struct gf2m_bigint *out)
{
	if (out != in)
		*out = *in;
	while (judge_bigint(out, p) >= 0)
		bigint_sub(out, p, out);
}

static void gf2m_bigint_add(struct gf2m_bigint *a, struct gf2m_bigint *b,
	struct gf2m_bigint *c)
{
	u32 j;

	for (j = 0; j < GF2M_MAX_LONG; j++)
		c->d[j] = a->d[j] ^ b->d[j];
}

static void gf2m_bigint_r_shift(struct gf2m_bigint *a, struct gf2m_bigint *c)
{
	int j;
	u32 flag = 0;
	u32 temp;

	for (j = (GF2M_NUM_WORD - 1); j >= 0; j--) {
		temp = a->d[j];
		c->d[j] = (temp >> 1) | flag;
		flag = temp << 31;
	}
}

/* i: is the bit rank from the lowest bit */
static u8 gf2m_bigint_bit_value(struct gf2m_bigint *a, u32 i)
{
	u32 x = i >> 5;
	u32 y = 1 << (i & 0x1f);

	if (a->d[x] & y)
		return 1;

	return 0;
}

/*
 * finite field F2m BIGINT Multiplication
 * c = a * b (mod f)
 */
static void gf2m_bigint_mul(struct gf2m_bigint *a, struct gf2m_bigint *b,
	struct gf2m_bigint *f, struct gf2m_bigint *c)
{
	struct gf2m_bigint t1;
	struct gf2m_bigint t2;

	copy_u32(t1.d, a->d, GF2M_MAX_LONG - 1);
	copy_u32(t2.d, b->d, GF2M_MAX_LONG - 1);

	gf2_163_mul(c->d, t1.d, t2.d);

	c->d[GF2M_MAX_LONG - 1] = 0;
}

/*
 * Calculate finite field F2m BIGINT multiplicative inverse
 * t = a^-1 (mod f)
 * a > f is allowable
 */
static void gf2m_bigint_maia_inv(struct gf2m_bigint *a, struct gf2m_bigint *f,
	struct gf2m_bigint *t)
{
	struct gf2m_bigint b = { {0} };
	struct gf2m_bigint c = { {0} };
	struct gf2m_bigint u;
	struct gf2m_bigint v;
	struct gf2m_bigint x;

	/* 1. set b=1, c=0, u=a, v=f; */
	memset(&b, 0, GF2M_MAX_LONG << 2);
	memset(&c, 0, GF2M_MAX_LONG << 2);
	b.d[0] = 1;
	u = *a;
	v = *f;

gf2m_maia_2:
	/* 2. */
	while ((u.d[0] & 1) == 0) {
		/* 2.1 */
		gf2m_bigint_r_shift(&u, &u);
		/* 2.2 */
		if ((b.d[0] & 1) == 1)
			gf2m_bigint_add(&b, f, &b);
		gf2m_bigint_r_shift(&b, &b);
	}

	/* 3. */
	if ((u.d[0] == 1) && (gf2m_calc_bit_len(&u) == 1)) {
		*t = b;
		return;
	}

	/* 4. */
	if (gf2m_calc_bit_len(&u) < gf2m_calc_bit_len(&v)) {
		x = v;
		v = u;
		u = x;
		x = c;
		c = b;
		b = x;
	}

	/* 5. */
	gf2m_bigint_add(&u, &v, &u);
	gf2m_bigint_add(&b, &c, &b);

	/* 6. */
	goto gf2m_maia_2;
}

/*
 * Calculate c composite needed in point double
 * c = b^(2^(m-2))
 */
static void gf2m_calc_c(struct gf2m_bigint *b, struct gf2m_bigint *f,
	struct gf2m_bigint *c)
{
	u32 j;
	struct gf2m_bigint u;

	gf2m_bigint_mul(b, b, f, &u);
	for (j = 0; j < (GF2M_163 - 3); j++)
		gf2m_bigint_mul(&u, &u, f, &u);
	*c = u;
}

static u32 gf2m_calc_bit_len(struct gf2m_bigint *a)
{
	u32 j;
	u32 x = GF2M_NUM_WORD;

	while (a->d[x - 1] == 0)
		x--;

	if (x != 0) {
		j = 31;
		while ((a->d[x - 1] & (0x1 << j)) == 0)
			j--;
		j = ((x - 1) << 5) + j + 1;
	} else {
		j = 0;
	}
	return j;
}

static void gf2m_clear_point(struct ec_curve *a)
{
	memset(a->x.d, 0, GF2M_MAX_LONG << 2);
	memset(a->y.d, 0, GF2M_MAX_LONG << 2);
	memset(a->z.d, 0, GF2M_MAX_LONG << 2);
}

/* if a=0 return 1;else return 0 */
static u8 gf2m_judge_bigint_null(struct gf2m_bigint *a)
{
	u32 j;

	for (j = 0; j < GF2M_MAX_LONG; j++) {
		if (a->d[j])
			return 0;
	}
	return 1;
}

/* ecc curve point double */
static void gf2m_curve_double(struct ec_curve *x1, struct gf2m_bigint *f,
	struct ec_curve *x2)
{
	struct gf2m_bigint t1;
	struct gf2m_bigint t2;
	struct gf2m_bigint t3;
	struct gf2m_bigint t4;

	if (gf2m_judge_bigint_null(&(x1->y)) ||
		gf2m_judge_bigint_null(&(x1->z))) {
		gf2m_clear_point(x2);
		x2->y.d[0] = 1;
		x2->x.d[0] = 1;
	}
	t1 = x1->x;
	t2 = x1->y;
	t3 = x1->z;
	t4 = g_gf2m_c;

	gf2m_bigint_mul(&t2, &t3, f, &t2); /* 3 */
	gf2m_bigint_mul(&t3, &t3, f, &t3); /* 4 */
	gf2m_bigint_mul(&t3, &t4, f, &t4); /* 5 */
	gf2m_bigint_mul(&t3, &t1, f, &t3); /* 6 */

	gf2m_bigint_add(&t2, &t3, &t2); /* 7 */
	gf2m_bigint_add(&t1, &t4, &t4); /* 8 */

	gf2m_bigint_mul(&t4, &t4, f, &t4); /* 9 */
	gf2m_bigint_mul(&t4, &t4, f, &t4); /* 10 */
	gf2m_bigint_mul(&t1, &t1, f, &t1); /* 11 */
	gf2m_bigint_add(&t1, &t2, &t2);    /* 12 */
	gf2m_bigint_mul(&t2, &t4, f, &t2); /* 13 */
	gf2m_bigint_mul(&t1, &t1, f, &t1); /* 14 */
	gf2m_bigint_mul(&t1, &t3, f, &t1); /* 15 */
	gf2m_bigint_add(&t1, &t2, &t2);    /* 16 */
	x2->x = t4;
	x2->y = t2;
	x2->z = t3;
}

static void gf2m_curve_add(struct ec_curve *x0, struct ec_curve *x1,
	struct gf2m_bigint *f, struct ec_curve *x2)
{
	struct gf2m_bigint t1;
	struct gf2m_bigint t2;
	struct gf2m_bigint t3;
	struct gf2m_bigint t4;
	struct gf2m_bigint t5;
	struct gf2m_bigint t6;
	struct gf2m_bigint t7;
	struct gf2m_bigint t8;
	struct gf2m_bigint t9;

	t1 = x0->x; /* 1 */
	t2 = x0->y;
	t3 = x0->z;
	t4 = x1->x;
	t5 = x1->y;

	if (!gf2m_judge_bigint_null((struct gf2m_bigint *)g_gf2m_a)) /* 2 */
		memcpy(t9.d, g_gf2m_a, GF2M_NUM_WORD << 2);
	if (!((x1->z.d[0] == 1) && (gf2m_calc_bit_len(&(x1->z)) == 1))) { /* 3 */
		t6 = t1;
		gf2m_bigint_mul(&t6, &t6, f, &t7);
		gf2m_bigint_mul(&t1, &t7, f, &t1);
		gf2m_bigint_mul(&t6, &t7, f, &t7);
		gf2m_bigint_mul(&t2, &t7, f, &t2);
	}
	gf2m_bigint_mul(&t3, &t3, f, &t7); /* 4 */
	gf2m_bigint_mul(&t4, &t7, f, &t8); /* 5 */
	gf2m_bigint_add(&t1, &t8, &t1);    /* 6 */
	gf2m_bigint_mul(&t3, &t7, f, &t7); /* 7 */
	gf2m_bigint_mul(&t5, &t7, f, &t8); /* 8 */
	gf2m_bigint_add(&t2, &t8, &t2);    /* 9 */
	if (gf2m_judge_bigint_null(&t1)) { /* 10 */
		if (gf2m_judge_bigint_null(&t2)) {
			gf2m_clear_point(x2);
			return;
		}
		gf2m_clear_point(x2);
		x2->y.d[0] = 1;
		x2->x.d[0] = x2->y.d[0];
		return;
	}
	gf2m_bigint_mul(&t2, &t4, f, &t4);                             /* 11 */
	gf2m_bigint_mul(&t1, &t3, f, &t3);                             /* 12 */
	gf2m_bigint_mul(&t3, &t5, f, &t5);                             /* 13 */
	gf2m_bigint_add(&t4, &t5, &t4);                                /* 14 */
	gf2m_bigint_mul(&t3, &t3, f, &t5);                             /* 15 */
	gf2m_bigint_mul(&t4, &t5, f, &t7);                             /* 16 */
	if (!((x1->z.d[0] == 1) && (gf2m_calc_bit_len(&(x1->z)) == 1))) /* 17 */
		gf2m_bigint_mul(&t3, &t6, f, &t3);
	gf2m_bigint_add(&t2, &t3, &t4);                     /* 18 */
	gf2m_bigint_mul(&t2, &t4, f, &t2);                  /* 19 */
	gf2m_bigint_mul(&t1, &t1, f, &t5);                  /* 20 */
	gf2m_bigint_mul(&t1, &t5, f, &t1);                  /* 21 */
	if (!gf2m_judge_bigint_null((struct gf2m_bigint *)g_gf2m_a)) { /* 22 */
		gf2m_bigint_mul(&t3, &t3, f, &t8);
		gf2m_bigint_mul(&t8, &t9, f, &t9);
		gf2m_bigint_add(&t1, &t9, &t1);
	}
	gf2m_bigint_add(&t1, &t2, &t1);    /* 23 */
	gf2m_bigint_mul(&t1, &t4, f, &t4); /* 24 */
	gf2m_bigint_add(&t4, &t7, &t2);    /* 25 */
	/* 26 */
	x2->x = t1;
	x2->y = t2;
	x2->z = t3;
}

static void curve2point(struct ec_curve *c, struct ec_point *p)
{
	gf2m_bigint_maia_inv(&c->z, (struct gf2m_bigint *)g_gf2m_f, &c->z);
	gf2m_bigint_mul(&c->z, &(c->z), (struct gf2m_bigint *)g_gf2m_f, &(p->y));
	gf2m_bigint_mul(&(c->x), &(p->y), (struct gf2m_bigint *)g_gf2m_f, &(p->x));
	gf2m_bigint_mul(&(p->y), &(c->z), (struct gf2m_bigint *)g_gf2m_f, &(p->y));
	gf2m_bigint_mul(&(c->y), &(p->y), (struct gf2m_bigint *)g_gf2m_f, &(p->y));
}

/* y=k*x */
static void gf2m_point_mul(struct gf2m_bigint *k, struct ec_point *x,
	struct ec_point *y)
{
	struct gf2m_bigint *p = NULL;

	struct ec_curve c_u = { { 0 }, { 0 }, { 0 } };
	struct ec_curve c_x = { { 0 }, { 0 }, { 0 } };
	u32 i = GF2M_163 - 1;

	gf2m_calc_c((struct gf2m_bigint *)g_gf2m_b,
		(struct gf2m_bigint *)g_gf2m_f, &g_gf2m_c);

	p = (struct gf2m_bigint *)g_gf2m_f;

	if (gf2m_judge_bigint_null(k)) {
		memset(y->x.d, 0, GF2M_NUM_WORD << 2);
		memset(y->y.d, 0, GF2M_NUM_WORD << 2);
		return;
	}

	while (!gf2m_bigint_bit_value(k, i))
		i--;

	c_x.x = x->x;
	c_x.y = x->y;
	c_x.z.d[0] = 1;
	c_u = c_x;

	while (i) {
		i--;
		gf2m_curve_double(&c_u, p, &c_u);
		if (gf2m_bigint_bit_value(k, i))
			gf2m_curve_add(&c_u, &c_x, p, &c_u);
	}

	curve2point(&c_u, y);
}

static void gf2m_point_add(struct ec_point *a, struct ec_point *b,
	struct ec_point *out)
{
	struct ec_curve tmp1 = { { {0} }, { {0} }, { {0} } };
	struct ec_curve tmp2 = { { {0} }, { {0} }, { {0} } };

	tmp1.x = a->x;
	tmp1.y = a->y;
	tmp1.z.d[0] = 1;
	tmp2.x = b->x;
	tmp2.y = b->y;
	tmp2.z.d[0] = 1;

	gf2m_curve_add(&tmp1, &tmp2, (struct gf2m_bigint *)g_gf2m_f, &tmp1);

	curve2point(&tmp1, out);
}

/*
 * y*y+x*y=x*x*x+a*x*x+b;
 * return: 0-pass, 1-fail
 */
static u8 gf2m_test_point(struct ec_point *x, struct gf2m_bigint *f)
{
	struct gf2m_bigint *a = (struct gf2m_bigint *)g_gf2m_a;
	struct gf2m_bigint *b = (struct gf2m_bigint *)g_gf2m_b;
	struct gf2m_bigint t0;
	struct gf2m_bigint t1;
	struct gf2m_bigint tl;

	gf2m_bigint_mul(&(x->y), &(x->y), f, &t0);
	gf2m_bigint_mul(&(x->x), &(x->y), f, &t1);
	gf2m_bigint_add(&t0, &t1, &tl);
	gf2m_bigint_mul(&(x->x), &(x->x), f, &t0);
	gf2m_bigint_mul(&(x->x), &t0, f, &t1);
	gf2m_bigint_mul(a, &t0, f, &t0);
	gf2m_bigint_add(&t0, &t1, &t0);
	gf2m_bigint_add(&t0, b, &t0);

	if (memcmp(tl.d, t0.d, GF2M_NUM_WORD << 2))
		return 1;

	return 0;
}

static void get_rand_bigint(struct gf2m_bigint *a)
{
	uint32_t offset;

	u32_get_rand(a->d, GF2M_NUM_WORD);
	a->d[GF2M_NUM_WORD] = 0;

	offset = GF2M_NUM_BIT % 32;
	if (offset)
		a->d[GF2M_NUM_WORD - 1] &= ((0x1UL << offset) - 1);

	bigint_mod_val(a, (struct gf2m_bigint *)g_gf2m_n, a);
}

int ec2m_163_dsa_sign(unsigned char *e, unsigned int e_byte_len,
	unsigned char *pri_key, unsigned char *r, unsigned char *s)
{
	struct gf2m_bigint k;
	struct ec_point g;
	struct bignum a;
	struct bignum b;
	struct bignum n;
	struct bignum c;
	struct bignum e1;
	uint32_t i;
	uint32_t bit_len;
	uint32_t a_data[GF2M_MAX_LONG] = { 0 };
	uint32_t c_data[GF2M_MAX_LONG] = { 0 };
	uint32_t offset1 = (GF2M_NUM_WORD << 2) - GF2M_NUM_BYTE;
	uint32_t offset2;
	uint32_t e_data[HASH_MAX_LONG] = { 0 };

	if (!e || !pri_key || !r || !s) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	copy_u32(g.x.d, (uint32_t *)g_gf2m_g0x, GF2M_MAX_LONG);
	copy_u32(g.y.d, (uint32_t *)g_gf2m_g0y, GF2M_MAX_LONG);

	if (gf2m_test_point(&g, (struct gf2m_bigint *)g_gf2m_f))
		return -EINVAL;

	get_rand_bigint(&k);
	gf2m_point_mul(&k, &g, &g); /* pa = k * g = (x1, y1) */

	a.data = g.x.d;
	a.len = GF2M_MAX_LONG - 1;
	a.maxlen = a.len;
	n.data = (uint32_t *)g_gf2m_n;
	n.len = GF2M_MAX_LONG - 1;
	n.maxlen = n.len;
	c.data = c_data;
	c.len = GF2M_MAX_LONG - 1;
	c.maxlen = c.len;

	bignum_mod(&a, &n, &c); /* r = x1 mod n */

	memcpy(r, (uint8_t *)c_data, GF2M_NUM_BYTE);
	reverse_u8(r, r, GF2M_NUM_BYTE);

	b.data = k.d;
	b.len = GF2M_MAX_LONG - 1;
	b.maxlen = b.len;
	bignum_modinv(&b, &n, &b); /* k = k^(-1) mod n */

	memset((uint8_t *)a_data, 0, offset1);
	memcpy((uint8_t *)a_data + offset1, pri_key, GF2M_NUM_BYTE);
	reverse_u8((uint8_t *)a_data, (uint8_t *)a_data, (GF2M_NUM_WORD << 2));

	if (gf2m_judge_bigint_null((struct gf2m_bigint *)a_data))
		return -EINVAL;

	if (judge_bigint((struct gf2m_bigint *)a_data,
		(struct gf2m_bigint *)g_gf2m_n) >= 0)
		return -EINVAL;

	a.data = a_data;
	a.len = GF2M_MAX_LONG - 1;
	a.maxlen = a.len;
	bignum_modmul(&c, &a, &n, &c); /* s = (r * dA) mod n */

	if (e_byte_len <= GF2M_NUM_BYTE) {
		offset2 = GF2M_NUM_BYTE - e_byte_len;
		memset((uint8_t *)e_data, 0, offset1 + offset2);
		memcpy((uint8_t *)e_data + offset1 + offset2, e, e_byte_len);
		reverse_u8((uint8_t *)e_data, (uint8_t *)e_data,
			(GF2M_NUM_WORD << 2));
	} else {
		memset((uint8_t *)e_data, 0, offset1);
		memcpy((uint8_t *)e_data + offset1, e, GF2M_NUM_BYTE);
		reverse_u8((uint8_t *)e_data, (uint8_t *)e_data,
			(GF2M_NUM_WORD << 2));
		bit_len = gf2m_calc_bit_len((struct gf2m_bigint *)g_gf2m_n);
		for (i = 0; i < ((GF2M_NUM_BYTE << 3) - bit_len); i++)
			gf2m_bigint_r_shift((struct gf2m_bigint *)e_data,
				(struct gf2m_bigint *)e_data);
	}

	e1.data = e_data;
	e1.len = GF2M_MAX_LONG - 1;
	e1.maxlen = e1.len;
	bignum_modadd(&e1, &c, &n, &c); /* s = (e + r * dA)mod n */

	bignum_modmul(&b, &c, &n, &c); /* s = k^(-1) * (e + r * dA)mod n */

	memcpy(s, (uint8_t *)c_data, GF2M_NUM_BYTE);
	reverse_u8(s, s, GF2M_NUM_BYTE);

	return 0;
}

int ec2m_163_dsa_verify(unsigned char *e1, unsigned int e_byte_len,
	unsigned char *pub_key, unsigned char *r, unsigned char *s)
{
	struct ec_point pa;
	struct ec_point pb;
	struct bignum a;
	struct bignum b;
	struct bignum n;
	struct bignum c;
	struct bignum e;
	uint32_t a_data[GF2M_MAX_LONG] = { 0 };
	uint32_t b_data[GF2M_MAX_LONG] = { 0 };
	uint32_t offset1 = (GF2M_NUM_WORD << 2) - GF2M_NUM_BYTE;
	uint32_t offset2;
	uint32_t e_data[HASH_MAX_LONG] = { 0 };
	uint32_t i;
	uint32_t bit_len;

	if (!e1 || !pub_key || !r || !s) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	n.data = (uint32_t *)g_gf2m_n;
	n.len = GF2M_MAX_LONG - 1;
	n.maxlen = n.len;

	memset((uint8_t *)a_data, 0, offset1);
	memcpy((uint8_t *)a_data + offset1, r, GF2M_NUM_BYTE);
	reverse_u8((uint8_t *)a_data, (uint8_t *)a_data, (GF2M_NUM_WORD << 2));

	memset((uint8_t *)b_data, 0, offset1);
	memcpy((uint8_t *)b_data + offset1, s, GF2M_NUM_BYTE);
	reverse_u8((uint8_t *)b_data, (uint8_t *)b_data, (GF2M_NUM_WORD << 2));

	copy_u32(pa.x.d, (uint32_t *)g_gf2m_g0x, GF2M_MAX_LONG);
	copy_u32(pa.y.d, (uint32_t *)g_gf2m_g0y, GF2M_MAX_LONG);

	if (gf2m_test_point(&pa, (struct gf2m_bigint *)g_gf2m_f))
		return -EINVAL;

	memset((uint8_t *)pb.x.d, 0, offset1);
	memcpy((uint8_t *)pb.x.d + offset1, pub_key + 1, GF2M_NUM_BYTE);
	reverse_u8((uint8_t *)pb.x.d, (uint8_t *)pb.x.d, (GF2M_NUM_WORD << 2));

	memset((uint8_t *)pb.y.d, 0, offset1);
	memcpy((uint8_t *)pb.y.d + offset1, pub_key + 1 + GF2M_NUM_BYTE,
		GF2M_NUM_BYTE);
	reverse_u8((uint8_t *)pb.y.d, (uint8_t *)pb.y.d, (GF2M_NUM_WORD << 2));

	if (gf2m_test_point(&pb, (struct gf2m_bigint *)g_gf2m_f))
		return -EINVAL;

	/* make sure r in [1, n-1] */
	if (gf2m_judge_bigint_null((struct gf2m_bigint *)a_data))
		return -EINVAL;

	if (judge_bigint((struct gf2m_bigint *)a_data,
		(struct gf2m_bigint *)g_gf2m_n) >= 0)
		return -EINVAL;

	/* make sure s in [1, n-1] */
	if (gf2m_judge_bigint_null((struct gf2m_bigint *)b_data))
		return -EINVAL;

	if (judge_bigint((struct gf2m_bigint *)b_data,
		(struct gf2m_bigint *)g_gf2m_n) >= 0)
		return -EINVAL;

	b.data = b_data;
	b.len = GF2M_MAX_LONG - 1;
	b.maxlen = b.len;
	bignum_modinv(&b, &n, &b); /* w = s^(-1) mod n */

	if (e_byte_len <= GF2M_NUM_BYTE) {
		offset2 = GF2M_NUM_BYTE - e_byte_len;
		memset((uint8_t *)e_data, 0, offset1 + offset2);
		memcpy((uint8_t *)e_data + offset1 + offset2, e1, e_byte_len);
		reverse_u8((uint8_t *)e_data, (uint8_t *)e_data,
			(GF2M_NUM_WORD << 2));
	} else {
		memset((uint8_t *)e_data, 0, offset1);
		memcpy((uint8_t *)e_data + offset1, e1, GF2M_NUM_BYTE);
		reverse_u8((uint8_t *)e_data, (uint8_t *)e_data,
			(GF2M_NUM_WORD << 2));
		bit_len = gf2m_calc_bit_len((struct gf2m_bigint *)g_gf2m_n);
		for (i = 0; i < ((GF2M_NUM_BYTE << 3) - bit_len); i++)
			gf2m_bigint_r_shift((struct gf2m_bigint *)e_data,
				(struct gf2m_bigint *)e_data);
	}

	e.data = e_data;
	e.len = GF2M_MAX_LONG - 1;
	e.maxlen = e.len;
	bignum_modmul(&e, &b, &n, &e); /* u1 = e * w mod n */

	a.data = a_data;
	a.len = GF2M_MAX_LONG - 1;
	a.maxlen = a.len;
	bignum_modmul(&a, &b, &n, &b); /* u2 = r * w mod n */
	/* U1 = u1 * g = (x1, y1) */
	gf2m_point_mul((struct gf2m_bigint *)e.data, &pa, &pa);
	/* U2 = u2 * Q, Q is PubKey */
	gf2m_point_mul((struct gf2m_bigint *)b_data, &pb, &pb);

	gf2m_point_add(&pa, &pb, &pb); /* U1 + U2 = (x1, y1) */

	c.data = pb.x.d;
	c.len = GF2M_MAX_LONG - 1;
	c.maxlen = c.len;
	bignum_mod(&c, &n, &c); /* x1 mod n */

	if (judge_bigint((struct gf2m_bigint *)pb.x.d,
		(struct gf2m_bigint *)a_data))
		return -EINVAL;

	return 0;
}
