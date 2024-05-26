/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 * Description: Internal functions for per-fs sdp(sensitive data protection).
 * Create: 2022-02-22
 */

#ifndef _SDP_INTERNAL_H
#define _SDP_INTERNAL_H

#include <linux/types.h>
#include <linux/fscrypt_common.h>

#ifndef CONFIG_SDP_ENCRYPTION
#define CONFIG_SDP_ENCRYPTION 1
#endif

#ifdef CONFIG_SDP_ENCRYPTION

#define FS_ENCRYPTION_CONTEXT_FORMAT_V2 2

#define FSCRYPT_SDP_ECE_ENABLE_FLAG 0x01

#define FSCRYPT_SDP_ECE_CONFIG_FLAG 0x02

#define FSCRYPT_SDP_SECE_ENABLE_FLAG 0x04

#define FSCRYPT_SDP_SECE_CONFIG_FLAG 0x08

static inline bool inode_is_sdp_encrypted(u32 flag)
{
	return (flag & 0x0f) != 0;
}

static inline bool inode_is_dps_encrypted(u32 flag)
{
	return (flag & 0xf0) != 0;
}

static inline bool inode_is_config_sdp_ece_encryption(u32 flag)
{
	return (flag & FSCRYPT_SDP_ECE_CONFIG_FLAG) != 0;
}

static inline bool inode_is_config_sdp_sece_encryption(u32 flag)
{
	return (flag & FSCRYPT_SDP_SECE_CONFIG_FLAG) != 0;
}

static inline bool inode_is_config_sdp_encryption(u32 flag)
{
	return (flag & FSCRYPT_SDP_SECE_CONFIG_FLAG) ||
		(flag & FSCRYPT_SDP_ECE_CONFIG_FLAG);
}

static inline bool inode_is_enabled_sdp_ece_encryption(u32 flag)
{
	return (flag & FSCRYPT_SDP_ECE_ENABLE_FLAG) != 0;
}

static inline bool inode_is_enabled_sdp_sece_encryption(u32 flag)
{
	return (flag & FSCRYPT_SDP_SECE_ENABLE_FLAG) != 0;
}

static inline bool inode_is_enabled_sdp_encryption(u32 flag)
{
	return (flag & FSCRYPT_SDP_ECE_ENABLE_FLAG) ||
		(flag & FSCRYPT_SDP_SECE_ENABLE_FLAG);
}

#define FSCRYPT_CE_CLASS 1
#define FSCRYPT_SDP_ECE_CLASS 2
#define FSCRYPT_SDP_SECE_CLASS 3
#define FSCRYPT_DPS_CLASS 4

#define FS_SDP_ECC_PUB_KEY_SIZE 64
#define FS_SDP_ECC_PRIV_KEY_SIZE 32
#define FS_AES_256_GCM_KEY_SIZE 32
#define FS_AES_256_CBC_KEY_SIZE 32
#define FS_AES_256_CTS_KEY_SIZE 32
#define FS_AES_256_XTS_KEY_SIZE 64

#define FS_KEY_INDEX_OFFSET 63
#define FS_KEY_CIPHER_SIZE 80
#define FS_KEY_IV_SIZE 16
#define ENHANCED_CHECK_KEYING 1
#define NO_NEED_TO_CHECK_KEYFING 0
#define FLAG_ENCRYPT 1
#define FLAG_DECRYPT 0
#define MAX_UFS_SLOT_INDEX 31
#define MAX_ECDH_SIZE 4096
#define HAS_PUB_KEY 1
#define NO_PUB_KEY 0
#define HEX_STR_PER_BYTE 2
struct fscrypt_sdp_key {
	u32 version;
	u32 sdp_class;
	u32 mode;
	u8 raw[FSCRYPT_MAX_KEY_SIZE];
	u32 size;
	u8 pub_key[FSCRYPT_MAX_KEY_SIZE];
	u32 pub_key_size;
} __packed;

struct fscrypt_sdp_context {
	u8 version;
	u8 sdp_class;
	u8 format;
	u8 contents_encryption_mode;
	u8 filenames_encryption_mode;
	u8 flags;
	u8 master_key_descriptor[FSCRYPT_KEY_DESCRIPTOR_SIZE];
	u8 nonce[FS_KEY_CIPHER_SIZE];
	u8 iv[FS_KEY_IV_SIZE];
	u8 file_pub_key[FS_SDP_ECC_PUB_KEY_SIZE];
} __packed;
#endif
#endif
