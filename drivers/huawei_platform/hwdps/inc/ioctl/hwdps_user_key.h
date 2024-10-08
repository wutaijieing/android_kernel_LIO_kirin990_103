/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2022. All rights reserved.
 * Description: This file contains the function required for operations about
 *              user key
 * Create: 2018-12-10
 */

#ifndef _HWDPS_USER_KEY_H
#define _HWDPS_USER_KEY_H

#include <linux/fscrypt.h>

#define HWDPS_USER_KEY_DESC_PREFIX_STR FSCRYPT_KEY_DESC_PREFIX
#define HWDPS_USER_KEY_DESC_PREFIX_STR_SIZE FSCRYPT_KEY_DESC_PREFIX_SIZE

#define HWDPS_USER_KEY_DESC_SUFFIX_SIZE FSCRYPT_KEY_DESCRIPTOR_SIZE
#define HWDPS_USER_KEY_DESC_SUFFIX_STR_SIZE ((HWDPS_USER_KEY_DESC_SUFFIX_SIZE \
	* 2) + 1)

#define HWDPS_USER_KEY_DESC_STR_SIZE (HWDPS_USER_KEY_DESC_PREFIX_STR_SIZE + \
	HWDPS_USER_KEY_DESC_SUFFIX_STR_SIZE)

#define HWDPS_USER_KEY_SIZE_MAX FSCRYPT_MAX_KEY_SIZE

s32 get_user_key(u8 *user_key_desc, u32 user_key_desc_len,
	u8 *user_key, u32 *user_key_len);
#endif
