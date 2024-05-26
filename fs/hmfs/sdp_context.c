/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2017. All rights reserved.
 * Description: Implementation of set/get sdp flag and context.
 * Create: 2017.11.10
 */
#include "xattr.h"
#include "sdp_context.h"

#ifdef CONFIG_SDP_ENCRYPTION
int hmfs_get_sdp_context(struct inode *inode, void *ctx, size_t len,
				void *fs_data)
{
	return hmfs_getxattr(inode, F2FS_XATTR_INDEX_ECE_ENCRYPTION,
			     F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
			     fs_data);
}

int hmfs_set_sdp_context(struct inode *inode, const void *ctx,
				size_t len, void *fs_data)
{
	return hmfs_setxattr(inode, F2FS_XATTR_INDEX_ECE_ENCRYPTION,
			     F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
			     fs_data, XATTR_CREATE);
}

int hmfs_update_sdp_context(struct inode *inode, const void *ctx,
				   size_t len, void *fs_data)
{
	return hmfs_setxattr(inode, F2FS_XATTR_INDEX_ECE_ENCRYPTION,
			     F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
			     fs_data, XATTR_REPLACE);
}

int hmfs_update_context(struct inode *inode, const void *ctx,
			       size_t len, void *fs_data)
{
	return hmfs_setxattr(inode, F2FS_XATTR_INDEX_ENCRYPTION,
			     F2FS_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len,
			     fs_data, XATTR_REPLACE);
}

int hmfs_get_sdp_encrypt_flags(struct inode *inode, void *fs_data,
				      u32 *flags)
{
	struct f2fs_xattr_header *hdr = NULL;
	struct page *xpage = NULL;
	int err = -EINVAL;

	if (!fs_data)
		down_read(&F2FS_I(inode)->i_sem);

	*flags = 0;
	hdr = hmfs_get_xattr_header(inode, (struct page *)fs_data, &xpage);
	if (IS_ERR_OR_NULL(hdr)) {
		err = (long)PTR_ERR(hdr);
		goto out_unlock;
	}

	*flags = hdr->h_xattr_flags;
	err = 0;
	f2fs_put_page(xpage, 1);
out_unlock:
	if (!fs_data)
		up_read(&F2FS_I(inode)->i_sem);
	return err;
}

int hmfs_set_sdp_encrypt_flags(struct inode *inode, void *fs_data,
				      u32 *flags)
{
	struct f2fs_sb_info *sbi = NULL;
	struct f2fs_xattr_header *hdr = NULL;
	struct page *xpage = NULL;
	int err = 0;

	if (!inode || !flags)
		return -EINVAL;

	sbi = F2FS_I_SB(inode);

	if (!fs_data) {
		f2fs_lock_op(sbi);
		down_write(&F2FS_I(inode)->i_sem);
	}

	hdr = hmfs_get_xattr_header(inode, (struct page *)fs_data, &xpage);
	if (IS_ERR_OR_NULL(hdr)) {
		err = (long)PTR_ERR(hdr);
		goto out_unlock;
	}

	hdr->h_xattr_flags = *flags;
	if (fs_data)
		set_page_dirty(fs_data);
	else if (xpage)
		set_page_dirty(xpage);

	f2fs_put_page(xpage, 1);

	hmfs_mark_inode_dirty_sync(inode, true);
	if (S_ISDIR(inode->i_mode))
		set_sbi_flag(sbi, SBI_NEED_CP);

out_unlock:
	if (!fs_data) {
		up_write(&F2FS_I(inode)->i_sem);
		f2fs_unlock_op(sbi);
	}
	return err;
}

int hmfs_operate_sdp_sem(struct inode *inode, u32 type)
{
	int err = 0;

	if (!inode)
		return -EINVAL;

	switch (type) {
	case 0:
		down_write(&F2FS_I(inode)->i_sdp_sem);
		break;
	case 1:
		up_write(&F2FS_I(inode)->i_sdp_sem);
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}
#endif

