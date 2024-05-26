/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Internal functions for per-f2fs sdp metadtata
 * Author: LAI Xinyi
 * Create: 2020-02-12
 */

#ifndef __SDP_METADATA_H__
#define __SDP_METADATA_H__

#include <linux/types.h>
#include <crypto/kpp.h>
#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include <linux/fscrypt_common.h>

#include "f2fs.h"
#include "sdp.h"

#define FPUBKEY_LEN PUBKEY_LEN

int f2fs_get_sdp_metadata_context(struct inode *inode, void *ctx, size_t len,
				  void *fs_data);
int f2fs_set_sdp_metadata_context(struct inode *inode, const void *ctx,
				  size_t len, void *fs_data);
int f2fs_update_sdp_metadata_context(struct inode *inode, const void *ctx,
				     size_t len, void *fs_data);
#endif
