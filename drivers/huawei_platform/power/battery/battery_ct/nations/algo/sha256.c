/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SHA256.c
 *
 * SHA256 function
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
#include "sha256.h"
#include <linux/slab.h>
#include <linux/string.h>

#define H0 0x6A09E667L
#define H1 0xBB67AE85L
#define H2 0x3C6EF372L
#define H3 0xA54FF53AL
#define H4 0x510E527FL
#define H5 0x9B05688CL
#define H6 0x1F83D9ABL
#define H7 0x5BE0CD19L
#define DIGEST_LEN 32

static const u32 g_key[64] = {
	0x428a2f98L, 0x71374491L, 0xb5c0fbcfL, 0xe9b5dba5L,
	0x3956c25bL, 0x59f111f1L, 0x923f82a4L, 0xab1c5ed5L,
	0xd807aa98L, 0x12835b01L, 0x243185beL, 0x550c7dc3L,
	0x72be5d74L, 0x80deb1feL, 0x9bdc06a7L, 0xc19bf174L,
	0xe49b69c1L, 0xefbe4786L, 0x0fc19dc6L, 0x240ca1ccL,
	0x2de92c6fL, 0x4a7484aaL, 0x5cb0a9dcL, 0x76f988daL,
	0x983e5152L, 0xa831c66dL, 0xb00327c8L, 0xbf597fc7L,
	0xc6e00bf3L, 0xd5a79147L, 0x06ca6351L, 0x14292967L,
	0x27b70a85L, 0x2e1b2138L, 0x4d2c6dfcL, 0x53380d13L,
	0x650a7354L, 0x766a0abbL, 0x81c2c92eL, 0x92722c85L,
	0xa2bfe8a1L, 0xa81a664bL, 0xc24b8b70L, 0xc76c51a3L,
	0xd192e819L, 0xd6990624L, 0xf40e3585L, 0x106aa070L,
	0x19a4c116L, 0x1e376c08L, 0x2748774cL, 0x34b0bcb5L,
	0x391c0cb3L, 0x4ed8aa4aL, 0x5b9cca4fL, 0x682e6ff3L,
	0x748f82eeL, 0x78a5636fL, 0x84c87814L, 0x8cc70208L,
	0x90befffaL, 0xa4506cebL, 0xbef9a3f7L, 0xc67178f2L
};

#define s(n, x) (((x) >> (n)) | ((x) << (32 - (n))))
#define r(n, x) ((x) >> (n))

#define ch(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define sig0(x)    (s(2, (x)) ^ s(13, (x)) ^ s(22, (x)))
#define sig1(x)    (s(6, (x)) ^ s(11, (x)) ^ s(25, (x)))
#define theta0(x)  (s(7, (x)) ^ s(18, (x)) ^ r(3, (x)))
#define theta1(x)  (s(17, (x)) ^ s(19, (x)) ^ r(10, (x)))

static void transform(struct sha256_ctx *ctx)
{
	u32 tmp[8];
	u32 t1;
	u32 t2;
	u32 j;

	for (j = 16; j < 64; j++)
		ctx->buf[j] = theta1(ctx->buf[j - 2]) + ctx->buf[j - 7] +
			theta0(ctx->buf[j - 15]) + ctx->buf[j - 16];

	tmp[0] = ctx->h[0];
	tmp[1] = ctx->h[1];
	tmp[2] = ctx->h[2];
	tmp[3] = ctx->h[3];
	tmp[4] = ctx->h[4];
	tmp[5] = ctx->h[5];
	tmp[6] = ctx->h[6];
	tmp[7] = ctx->h[7];

	for (j = 0; j < 64; j++) {
		t1 = tmp[7] + sig1(tmp[4]) + ch(tmp[4], tmp[5], tmp[6]) + g_key[j] + ctx->buf[j];
		t2 = sig0(tmp[0]) + maj(tmp[0], tmp[1], tmp[2]);
		tmp[7] = tmp[6];
		tmp[6] = tmp[5];
		tmp[5] = tmp[4];
		tmp[4] = tmp[3] + t1;
		tmp[3] = tmp[2];
		tmp[2] = tmp[1];
		tmp[1] = tmp[0];
		tmp[0] = t1 + t2;
	}

	ctx->h[0] += tmp[0];
	ctx->h[1] += tmp[1];
	ctx->h[2] += tmp[2];
	ctx->h[3] += tmp[3];
	ctx->h[4] += tmp[4];
	ctx->h[5] += tmp[5];
	ctx->h[6] += tmp[6];
	ctx->h[7] += tmp[7];
}

void ns3300_sha256_init(struct sha256_ctx *ctx)
{
	u32 i;

	if (!ctx) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	for (i = 0; i < 64; i++)
		ctx->buf[i] = 0L;

	ctx->len[1] = 0;
	ctx->len[0] = ctx->len[1];

	ctx->h[0] = H0;
	ctx->h[1] = H1;
	ctx->h[2] = H2;
	ctx->h[3] = H3;
	ctx->h[4] = H4;
	ctx->h[5] = H5;
	ctx->h[6] = H6;
	ctx->h[7] = H7;
}

static void ahs256_input_byte(struct sha256_ctx *ctx, u8 byte)
{
	int cnt = (int)((ctx->len[0] >> 5) & 15);

	ctx->buf[cnt] <<= 8;
	ctx->buf[cnt] |= (u32)(byte & 0xFF);
	ctx->len[0] += 8;

	if (ctx->len[0] == 0) {
		ctx->len[1]++;
		ctx->len[0] = 0;
	}

	if ((ctx->len[0] & 511) == 0)
		transform(ctx);
}

void sha256_input(struct sha256_ctx *ctx, u8 *input, u32 input_byte_len)
{
	u32 i;

	if (!ctx || !input) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	for (i = 0; i < input_byte_len; i++)
		ahs256_input_byte(ctx, input[i]);
}

void sha256_result(struct sha256_ctx *ctx, u8 *digest, unsigned int digest_len)
{
	u32 len0, len1;
	u32 i;

	if (!ctx || !digest) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}
	if (digest_len < DIGEST_LEN) {
		hwlog_err("%s: digest_len invalid\n", __func__);
		return;
	}

	len0 = ctx->len[0];
	len1 = ctx->len[1];
	ahs256_input_byte(ctx, 0x80);
	while ((ctx->len[0] & 511) != 448)
		ahs256_input_byte(ctx, 0);

	ctx->buf[14] = len1;
	ctx->buf[15] = len0;

	transform(ctx);
	for (i = 0; i < 32; i++)
		digest[i] = (char)((ctx->h[i / 4] >> (8 * ((3 - i) & 3))) & 0xff);

	ns3300_sha256_init(ctx);
}

void sha256_hash(u8 *input, u32 byte_len, u8 *digest)
{
	struct sha256_ctx ctx[1];

	if (!input || !digest) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	ns3300_sha256_init(ctx);
	sha256_input(ctx, input, byte_len);
	sha256_result(ctx, digest, DIGEST_LEN);
}

void hmac_sha256_init(struct hmac_sha256_ctx *ctx, u8 *key, u32 key_byte_len)
{
	u32 i;

	if (!ctx || !key) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	ns3300_sha256_init(ctx->ctx256);
	memset(ctx->k0, 0, 64);

	if (key_byte_len > 64)
		sha256_hash(key, key_byte_len, ctx->k0);
	else
		memcpy(ctx->k0, key, key_byte_len);

	for (i = 0; i < 64; i++)
		ctx->k0[i] ^= 0x36;

	sha256_input(ctx->ctx256, ctx->k0, 64);

	for (i = 0; i < 64; i++)
		ctx->k0[i] ^= 0x36 ^ 0x5C;
}

void hmac_sha256_result(struct hmac_sha256_ctx *ctx, u8 *ret, u32 ret_byte_len)
{
	u8 tmp[32] = { 0 };

	if (!ctx || !ret) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	sha256_result(ctx->ctx256, tmp, DIGEST_LEN);

	sha256_input(ctx->ctx256, ctx->k0, 64);
	sha256_input(ctx->ctx256, tmp, DIGEST_LEN);
	sha256_result(ctx->ctx256, tmp, DIGEST_LEN);

	memcpy(ret, tmp, ret_byte_len <= 32 ? ret_byte_len : 32);
}
