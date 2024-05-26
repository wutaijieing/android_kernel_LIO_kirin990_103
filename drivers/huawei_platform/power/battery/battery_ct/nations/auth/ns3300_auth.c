/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300_auth.c
 *
 * ns3300 authentication
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

#include "ns3300_auth.h"
#include <linux/slab.h>
#include <linux/string.h>
#include <huawei_platform/log/hw_log.h>

#include "../platform/board.h"
#include "../swi/bsp_swi.h"
#include "../algo/bignum.h"
#include "../algo/sha256.h"
#include "../algo/ecc_curve.h"
#include "../algo/ecc_primitive.h"
#include "../algo/gf2m.h"
#include "../algo/gf2m_common.h"
#include "ns3300_curve.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG ns3300_auth
HWLOG_REGIST();

#define UID_BYTENUM       (12u)
#define SWIBASE           0xF0
#define INDEX             (SWIBASE+0x3)
#define COMMAND           (SWIBASE+0x2)
#define ADDH              (SWIBASE+0x1)
#define ADDL              (SWIBASE+0x0)
#define WRDAT0            (SWIBASE+0x4)
#define WRDAT1            (SWIBASE+0x5)
#define WRDAT2            (SWIBASE+0x6)
#define WRDAT3            (SWIBASE+0x7)
#define WRDAT4            (SWIBASE+0x8)
#define WRDAT5            (SWIBASE+0x9)
#define WRDAT6            (SWIBASE+0xA)
#define WRDAT7            (SWIBASE+0xB)
#define ADRMAP            (SWIBASE+0xC)

static uint8_t g_swi_error_counter;

static bool check_swi_error_counter(void)
{
	uint8_t counter = 0;

	if (!swi_cmd_read_swi_error_counter(&counter))
		return false;

	hwlog_info("[%s]: err_counter=%d\n", __func__, counter);

	if (counter != g_swi_error_counter) {
		g_swi_error_counter = counter;
		return false;
	}

	return true;
}

bool ns3300_read_uid(uint8_t *uid, unsigned int uid_len)
{
	if (!uid || uid_len == 0) {
		hwlog_err("%s: param invalid\n", __func__);
		return false;
	}

	return swi_read_space(SWI_INFO_UID, UID_BYTENUM, uid);
}

bool ns3300_read_cert(uint8_t *ecp_sign, uint8_t *gf2n_publickey)
{
	if (!ecp_sign || !gf2n_publickey) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	if (!swi_read_space(SWI_INFO_SIGN, double_array_blen(GFP_256), ecp_sign))
		return false;

	if (!swi_read_space(SWI_INFO_PUBKEY, double_array_blen(GF2_163), gf2n_publickey))
		return false;

	return true;
}

bool ns3300_read_publickey(uint8_t *publickey, unsigned int pki_len)
{
	if (!publickey || pki_len == 0) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	memcpy(publickey, ecp_256_publickey, 65); /* publickey len */
	return true;
}

static bool ns3300_verify_cert(uint8_t *gfp_sign, uint8_t *gf2n_publickey, uint8_t *uid, unsigned int uid_len)
{
	struct sha256_ctx hash_ctx;
	uint8_t ub_hash_out[array_blen(ECDSA_SHA)];

	if (!gfp_sign || !gf2n_publickey || !uid) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	if (uid_len == 0) {
		hwlog_err("%s: uid_len invalid\n", __func__);
		return false;
	}
	ns3300_sha256_init(&hash_ctx);
	sha256_input(&hash_ctx, gf2n_publickey, double_array_blen(GF2_163));
	sha256_input(&hash_ctx, uid, UID_BYTENUM);
	sha256_result(&hash_ctx, ub_hash_out, sizeof(ub_hash_out));

	ecc_load_crv_parm((uint8_t *)ecp_256_p, array_blen(GFP_256), (uint8_t *)ecp_256_a,
		array_blen(GFP_256), (uint8_t *)ecp_256_b, array_blen(GFP_256),
		(uint8_t *)ecp_256_gx, array_blen(GFP_256), (uint8_t *)ecp_256_gy,
		array_blen(GFP_256), (uint8_t *)ecp_256_n, array_blen(GFP_256),
		(uint8_t *)ecp_256_h, array_blen(GFP_256));

	if (ecvp_dsa(ub_hash_out, array_blen(ECDSA_SHA), (uint8_t *)ecp_256_publickey,
		gfp_sign, gfp_sign + array_blen(GFP_256)))
		return false;

	return true;
}

bool ns3300_verify_cert_with_publickey(uint8_t *gfp_sign, uint8_t *gf2n_publickey,
	uint8_t *uid, uint8_t *publickey)
{
	struct sha256_ctx hash_ctx;
	uint8_t ub_hash_out[array_blen(ECDSA_SHA)];

	if (!gfp_sign || !gf2n_publickey || !uid || !publickey) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	ns3300_sha256_init(&hash_ctx);
	sha256_input(&hash_ctx, gf2n_publickey, double_array_blen(GF2_163));
	sha256_input(&hash_ctx, uid, UID_BYTENUM);
	sha256_result(&hash_ctx, ub_hash_out, sizeof(ub_hash_out));

	ecc_load_crv_parm((uint8_t *)ecp_256_p, array_blen(GFP_256), (uint8_t *)ecp_256_a,
		array_blen(GFP_256), (uint8_t *)ecp_256_b, array_blen(GFP_256),
		(uint8_t *)ecp_256_gx, array_blen(GFP_256), (uint8_t *)ecp_256_gy,
		array_blen(GFP_256), (uint8_t *)ecp_256_n, array_blen(GFP_256),
		(uint8_t *)ecp_256_h, array_blen(GFP_256));

	if (ecvp_dsa(ub_hash_out, array_blen(ECDSA_SHA), (uint8_t *)publickey, gfp_sign,
		gfp_sign + array_blen(GFP_256)))
		return false;

	return true;
}

bool ns3300_genarate_challenge(uint8_t *gf2n_challenge, unsigned int clg_len)
{
	uint32_t uw_rand[array_blen(ECDSA_SHA)];

	if (!gf2n_challenge || (clg_len == 0)) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	u32_get_rand(uw_rand, array_wlen(ECDSA_SHA));

	sha256_hash((uint8_t *)uw_rand, array_blen(ECDSA_SHA), gf2n_challenge);

	return true;
}

static void ns3300_truncate_challenge(uint8_t *out, unsigned int out_len, uint8_t *in)
{
	struct bignum c;
	uint32_t ub_challenge[array_wlen(GF2_163)];
	uint32_t offset = (array_wlen(GF2_163) << 2) - array_blen(GF2_163);

	memset((uint8_t *)ub_challenge, 0, offset);
	memcpy((uint8_t *)ub_challenge + offset, in, array_blen(GF2_163));

	reverse_u8((uint8_t *)ub_challenge, (uint8_t *)ub_challenge, array_wlen(GF2_163) << 2);
	c.data = ub_challenge;
	c.maxlen = c.len = array_wlen(GF2_163);
	bignum_shr(&c, ((array_blen(GF2_163) << 3) - GF2_163), &c);

	memcpy(out, (uint8_t *)ub_challenge, array_blen(GF2_163));
}

static bool ns3300_send_challenge(uint8_t *gf2n_challenge_in)
{
	uint8_t gf2n_challenge[array_blen(GF2_163)];
	uint32_t i, j;

	ns3300_truncate_challenge(gf2n_challenge, sizeof(gf2n_challenge), gf2n_challenge_in);
	for (i = 0; i < 3; i++) {
		check_swi_error_counter();
		for (j = 0; j < array_blen(GF2_163); j++)
			swi_write_byte(SAC_IADR, gf2n_challenge[j]);

		swi_write_byte(SAC_IADR, 0x0u);

		if (check_swi_error_counter()) {
			swi_cmd_enable_ecc();
			return true;
		}

		swi_cmd_soft_reset();
	}

	return false;
}

static bool swi_read_response(uint8_t *gf2n_response)
{
	uint8_t value = 0x0;
	int i = 0;
	int j;

	for (j = 0; j < (array_blen(GF2_163) >> 1); j++) {
		if (!swi_send_rra_rd(SAC_OADR, &value))
			return false;

		*(gf2n_response + i++) = value;

		if (!swi_send_rra_rd(SWI_DATA1, &value))
			return false;

		*(gf2n_response + i++) = value;
	}

	if (!swi_send_rra_rd(SAC_OADR, &value))
		return false;

	*(gf2n_response + i++) = value;

	for (j = 0; j < (array_blen(GF2_163) >> 1); j++) {
		if (!swi_send_rra_rd(SAC_OADR, &value))
			return false;

		*(gf2n_response + i++) = value;

		if (!swi_send_rra_rd(SWI_DATA1, &value))
			return false;

		*(gf2n_response + i++) = value;
	}

	if (!swi_send_rra_rd(SAC_OADR, &value))
		return false;

	*(gf2n_response + i++) = value;

	reverse_u8(gf2n_response, gf2n_response, array_blen(GF2_163));
	reverse_u8(gf2n_response + array_blen(GF2_163), gf2n_response + array_blen(GF2_163),
		array_blen(GF2_163));

	return true;
}

static bool ns3300_get_response(uint8_t *gf2n_response)
{
	if (!swi_read_response(gf2n_response))
		return false;

	return true;
}

bool ns3300_send_challenge_and_get_response(uint8_t *gf2n_challenge, unsigned int clg_len, uint8_t *gf2n_response)
{
	bool ret = false;

	if (!gf2n_challenge || !gf2n_response) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	if (clg_len == 0) {
		hwlog_err("%s: clg_len invalid\n", __func__);
		return false;
	}
	ret = ns3300_send_challenge(gf2n_challenge);
	if (!ret)
		return false;

	ret = ns3300_get_response(gf2n_response);
	if (!ret)
		return false;

	return true;
}

static bool ns3300_verify_response(uint8_t *gf2n_challenge, unsigned int clg_len, uint8_t *gf2n_response,
	uint8_t *gf2n_publickey)
{
	uint8_t ub_publibkey[1 + double_array_blen(GF2_163)];

	if (!gf2n_challenge || !gf2n_response || !gf2n_publickey) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	if (clg_len > array_blen(ECDSA_SHA)) {
		hwlog_err("%s: clg_len out of range\n", __func__);
		return false;
	}
	ub_publibkey[0] = 0x04u;

	memcpy(ub_publibkey + 1, gf2n_publickey, double_array_blen(GF2_163));

	if (ec2m_163_dsa_verify(gf2n_challenge, array_blen(ECDSA_SHA), ub_publibkey,
		gf2n_response, gf2n_response + array_blen(GF2_163)))
		return false;

	return true;
}

bool ns3300_do_authentication(void)
{
	uint8_t ub_uid[UID_BYTENUM];
	uint8_t gfp_sign[double_array_blen(GFP_256)];
	uint8_t gf2n_publickey[double_array_blen(GF2_163)];
	uint8_t gf2n_challenge[array_blen(ECDSA_SHA)];
	uint8_t gf2n_response[double_array_blen(GF2_163)];

	if (!ns3300_read_uid(ub_uid, sizeof(ub_uid)))
		return false;

	if (!ns3300_read_cert(gfp_sign, gf2n_publickey))
		return false;

	if (!ns3300_verify_cert(gfp_sign, gf2n_publickey, ub_uid, sizeof(ub_uid)))
		return false;

	if (!ns3300_genarate_challenge(gf2n_challenge, sizeof(gf2n_challenge)))
		return false;

	if (!ns3300_send_challenge_and_get_response(gf2n_challenge, sizeof(gf2n_challenge), gf2n_response))
		return false;

	if (!ns3300_verify_response(gf2n_challenge, sizeof(gf2n_challenge), gf2n_response, gf2n_publickey))
		return false;

	return true;
}
