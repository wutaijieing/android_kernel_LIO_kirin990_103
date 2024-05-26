/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ecc_primitive.c
 *
 * ellipse curve cryptography ecc_primitive
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

int ecvp_dsa(u8 *e1, u32 e_byte_len, u8 *pub_key, u8 *r, u8 *s)
{
	u32 e_data[ECC_MAX_WORD_LEN] = { 0 };
	u32 r_data[ECC_MAX_WORD_LEN] = { 0 };
	u32 s_data[ECC_MAX_WORD_LEN] = { 0 };
	u32 p0_xdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 p0_ydata[ECC_MAX_WORD_LEN] = { 0 };
	u32 p1_xdata[ECC_MAX_WORD_LEN] = { 0 };
	u32 p1_ydata[ECC_MAX_WORD_LEN] = { 0 };
	struct bignum e[1] = { { e_data, ECC_MAX_WORD_LEN, 0 } };
	struct bignum r_bn[1] = { { r_data, ECC_MAX_WORD_LEN, 0 } };
	struct bignum s_bn[1] = { { s_data, ECC_MAX_WORD_LEN, 0 } };
	struct ecc_point p0[1] = { { { { p0_xdata, ECC_MAX_WORD_LEN, 0 } },
		{ { p0_ydata, ECC_MAX_WORD_LEN, 0 } } } };
	struct ecc_point p1[1] = { { { { p1_xdata, ECC_MAX_WORD_LEN, 0 } },
		{ { p1_ydata, ECC_MAX_WORD_LEN, 0 } } } };
	s32 dif;
	u32 *p_e = e_data;
	int ret;

	if (!e1 || !pub_key || !r || !s) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	ret = ecc_u8_to_point(pub_key, p1);
	if (ret)
		return ret;

	ret = ecc_u8_to_bignum(r, r_bn);
	if (ret)
		return ret;

	ret = ecc_u8_to_bignum(s, s_bn);
	if (ret)
		return ret;

	u8tou32(e1, p_e, e_byte_len);
	e->len = (e_byte_len + 3) >> 2;
	bignum_clr(e);

	dif = (e_byte_len << 3) - bignum_getbitlen(ecc_get_n());
	if (dif > 0)
		bignum_shr(e, (u32)dif, e);

	bignum_modinv(s_bn, ecc_get_n(), s_bn);

	bignum_mont_start(ecc_get_n_ctx(), s_bn, s_bn);
	bignum_mont_start(ecc_get_n_ctx(), e, e);
	bignum_mont_mul(ecc_get_n_ctx(), s_bn, e, e);
	bignum_mont_finish(ecc_get_n_ctx(), e, e);
	ecc_point_mul(e, ecc_get_g(), p0);

	bignum_mont_start(ecc_get_n_ctx(), r_bn, e);
	bignum_mont_mul(ecc_get_n_ctx(), s_bn, e, e);
	bignum_mont_finish(ecc_get_n_ctx(), e, e);
	ecc_point_mul(e, p1, p1);

	ecc_point_add(p0, p1, p0);

	bignum_mod(p0->x, ecc_get_n(), p0->x);

	if (bignum_cmp(p0->x, r_bn))
		return -EINVAL;

	return 0;
}
