/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Implementation of 1) get/set sdp encryption metadata;
 *                                2) update sdp metadata.
 * Author: LAI Xinyi
 * Create: 2020-02-12
 */
#include "sdp_metadata.h"

#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include <linux/random.h>
#include "xattr.h"
#include "sdp.h"

#include <platform_include/see/fbe_ctrl.h>

int f2fs_get_sdp_metadata_context(struct inode *inode, void *ctx, size_t len,
				  void *fs_data)
{
	int ret = f2fs_getxattr(inode, F2FS_XATTR_INDEX_ENCRYPTION_METADATA,
				F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
				fs_data);
	if (ret != len)
		pr_err("%s:f2fs_getxattr error %d ino = %llu\r\n", __func__,
		       ret, inode->i_ino);

	return ret;
}

int f2fs_set_sdp_metadata_context(struct inode *inode, const void *ctx,
				  size_t len, void *fs_data)
{
	int ret = f2fs_setxattr(inode, F2FS_XATTR_INDEX_ENCRYPTION_METADATA,
				F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
				fs_data, XATTR_CREATE);
	if (ret == -EEXIST)
		ret = f2fs_update_sdp_metadata_context(inode, ctx, len,
						       fs_data);

	return ret;
}

int f2fs_update_sdp_metadata_context(struct inode *inode, const void *ctx,
				     size_t len, void *fs_data)
{
	int ret = f2fs_setxattr(inode, F2FS_XATTR_INDEX_ENCRYPTION_METADATA,
				F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
				fs_data, XATTR_REPLACE);
	if (ret)
		pr_err("%s: XATTR_REPLACE fail %d ino = %llu\r\n", __func__,
		       ret, inode->i_ino);

	return ret;
}
