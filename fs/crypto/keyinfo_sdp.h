/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * Description: Implementation of 1) get sdp encryption key info;
 *                                2) update sdp context.
 * Create: 2022-02-22
 */

#ifndef _KEYINFO_SDP_H
#define _KEYINFO_SDP_H

#include "sdp_internal.h"

int change_to_sdp_crypto(struct inode *inode, void *fs_data);

int check_keyring_for_sdp(const u8 descriptor[FSCRYPT_KEY_DESCRIPTOR_SIZE],
	int enforce);

int get_sdp_encrypt_flags(struct inode *inode, void *fs_data,
	u32 *flag);

int set_sdp_encryption_flags(struct inode *inode, void *fs_data,
	u32 sdp_enc_flag);

#ifdef CONFIG_HWDPS
int set_dps_attr_and_flag(struct inode *inode);
#endif

#endif
