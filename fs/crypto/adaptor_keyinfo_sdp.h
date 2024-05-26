/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * Description: Implementation of 1) fill fscyrpt_info for sdp;
 *                                2) setup inline crypt or soft crypt for sdp.
 * Create: 2022-02-20
 */

#ifndef _ADAPTOR_KEYINFO_SDP_H
#define _ADAPTOR_KEYINFO_SDP_H

#include "sdp_internal.h"
#include "fscrypt_private.h"

int init_crypt_info_from_context(struct inode *inode,
	struct fscrypt_info **crypt_info,
	const struct fscrypt_sdp_context *ctx);

void put_crypt_info_sdp(struct fscrypt_info *ci);

int fill_and_change_crypt_info(struct fscrypt_info *crypt_info, union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx, u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE]);

int fill_crypt_info(struct fscrypt_info *crypt_info, union fscrypt_context *ctx,
	struct fscrypt_sdp_context *sdp_ctx, u8 fek[FS_KEY_DERIVATION_CIPHER_SIZE], int flag);

#endif
