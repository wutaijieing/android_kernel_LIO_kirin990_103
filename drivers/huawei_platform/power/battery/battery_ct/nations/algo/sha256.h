/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sha256.h
 *
 * sha256 head file
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
#ifndef HEADER_SHA256_H
#define HEADER_SHA256_H
#include <linux/types.h>

#include "common.h"

struct sha256_ctx {
	unsigned int len[2];
	unsigned int h[8];
	unsigned int buf[64];
};

struct hmac_sha256_ctx {
	struct sha256_ctx ctx256[1];
	u8 k0[64];
};

void ns3300_sha256_init(struct sha256_ctx *ctx);
void sha256_input(struct sha256_ctx *ctx, u8 *input, u32 input_byte_len);
void sha256_result(struct sha256_ctx *ctx, u8 digest[32], unsigned int digest_len);
void sha256_hash(u8 *input, u32 byte_len, u8 digest[32]);
void hmac_sha256_init(struct hmac_sha256_ctx *ctx, u8 *key, u32 key_byte_len);
void hmac_sha256_result(struct hmac_sha256_ctx *ctx, u8 *ret, u32 ret_byte_len);

#endif /* HEADER_SHA256_H */
