/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Implementation of set/get sdp flag and context.
 * Create: 2022.03.19
 */

 #ifndef _SDP_CONTEXT_H
 #define _SDP_CONTEXT_H
 #include <linux/fs.h>
 #include <linux/hmfs_fs.h>
 #include "hmfs.h"
 #include "sdp.h"

int hmfs_get_sdp_context(struct inode *inode, void *ctx, size_t len,
		void *fs_data);
int hmfs_set_sdp_context(struct inode *inode, const void *ctx,
		size_t len, void *fs_data);
int hmfs_update_sdp_context(struct inode *inode, const void *ctx,
			size_t len, void *fs_data);
int hmfs_update_context(struct inode *inode, const void *ctx,
			size_t len, void *fs_data);
int hmfs_get_sdp_encrypt_flags(struct inode *inode, void *fs_data,
			u32 *flags);
int hmfs_set_sdp_encrypt_flags(struct inode *inode, void *fs_data,
			 u32 *flags);
int hmfs_operate_sdp_sem(struct inode *inode, u32 type);
 #endif
