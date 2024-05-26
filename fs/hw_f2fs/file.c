// SPDX-License-Identifier: GPL-2.0
/*
 *file.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */
#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include <linux/stat.h>
#include <linux/buffer_head.h>
#include <linux/writeback.h>
#include <linux/blkdev.h>
#include <linux/falloc.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/uaccess.h>
#include <linux/mount.h>
#include <linux/pagevec.h>
#include <linux/uio.h>
#include <linux/uuid.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/nls.h>
#ifdef CONFIG_HW_F2FS_CHIP_KIRIN
#include <linux/fscrypt_common.h>
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
#include <linux/sched/signal.h>
#endif
#ifdef CONFIG_IOCACHE
#include <iocache.h>
#endif

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
#include <log/hiview_hievent.h>
#endif

#include "f2fs.h"
#include "node.h"
#include "segment.h"
#include "xattr.h"
#include "acl.h"
#include "gc.h"
#include <trace/events/f2fs.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
#include <uapi/linux/f2fs.h>
#endif

#include "sdp.h"

#ifdef CONFIG_HWDPS
#include <huawei_platform/hwdps/hwdps_fs_hooks.h>
#endif

#ifdef CONFIG_F2FS_TURBO_ZONE
#include "turbo_zone.h"
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
#include <log/hiview_hievent.h>
#endif

static int delayflush;
unsigned long current_flush_merge;
#ifdef CONFIG_MAS_ORDER_PRESERVE
#define F2FS_WAIT_FSYNC_TIMEOUT 100
#define F2FS_I_FSYNC_FLAG_SCHED_WORK 1
#define F2FS_I_FSYNC_FLAG_WAITING 2
#endif

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
#define COMPRESS_BD_ID			992782006
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
#define DEDUP_COMPARE_PAGES	10
#define DEDUP_BD_ID 992782005
#define F2FS_REVOKE_DSM_LIMIT 10
#define REVOKE_FAIL_REASON_SIZE 128

#define DEDUP_META_UN_MODIFY_FL		0x1
#define DEDUP_DATA_UN_MODIFY_FL		0x2
#define DEDUP_SET_MODIFY_CHECK		0x4
#define DEDUP_GET_MODIFY_CHECK		0x8
#define DEDUP_CLEAR_MODIFY_CHECK	0x10
#define DEDUP_CLONE_META		0x20
#define DEDUP_CLONE_DATA		0x40
#define DEDUP_SYNC_DATA			0x80
#define DEDUP_LOOP_MOD			10000
#define DEDUP_MIN_SIZE			65536
#define OUTER_INODE			1
#define INNER_INODE			2
#define NORMAL_INODE			3

struct page_list {
	struct list_head list;
	struct page *page;
};
static struct kmem_cache *page_info_slab;

#define LOG_PAGE_INTO_LIST(head, page)	do {			\
	struct page_list *tmp;					\
	tmp = f2fs_kmem_cache_alloc(page_info_slab, GFP_NOFS);	\
	tmp->page = page;					\
	INIT_LIST_HEAD(&tmp->list);				\
	list_add_tail(&tmp->list, &head);			\
} while (0)

#define FREE_FIRST_PAGE_IN_LIST(head)	do {			\
	struct page_list *tmp;					\
	tmp = list_first_entry(&head, struct page_list, list);	\
	f2fs_put_page(tmp->page, 0);				\
	list_del(&tmp->list);					\
	kmem_cache_free(page_info_slab, tmp);			\
} while (0)

int create_page_info_slab(void)
{
	page_info_slab = f2fs_kmem_cache_create("f2fs_page_info_entry",
				sizeof(struct page_list));
	if (!page_info_slab)
		return -ENOMEM;

	return 0;
}

void destroy_page_info_slab(void)
{
	if (!page_info_slab)
		return;

	kmem_cache_destroy(page_info_slab);
}

/*
 * need lock_op and acquire_orphan by caller
 */
void f2fs_drop_deduped_link(struct inode *inner)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inner);

	down_write(&F2FS_I(inner)->i_sem);
	f2fs_i_links_write(inner, false);
	up_write(&F2FS_I(inner)->i_sem);

	if (inner->i_nlink == 0)
		f2fs_add_orphan_inode(inner);
	else
		f2fs_release_orphan_inode(sbi);
}

int f2fs_set_inode_addr(struct inode* inode, block_t addr)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct page *node_page;
	struct f2fs_inode *ri;
	int base = 0;
	int count = 1;
	int i;

repeat:
	node_page = f2fs_get_node_page(sbi, inode->i_ino);
	if (PTR_ERR(node_page) == -ENOMEM) {
		if (!(count++ % DEDUP_LOOP_MOD))
			f2fs_err(sbi,
				"%s: try to get node page %d", __func__, count);

		cond_resched();
		goto repeat;
	} else if (IS_ERR(node_page)) {
		f2fs_err(sbi, "%s: get node page fail", __func__);
		return PTR_ERR(node_page);
	}

	f2fs_wait_on_page_writeback(node_page, NODE, true, true);
	ri = F2FS_INODE(node_page);

	if (f2fs_has_extra_attr(inode))
		base = get_extra_isize(inode);

	for (i = 0; i < addrs_per_inode(inode); i++) {
		if (__is_valid_data_blkaddr(sbi, ri->i_addr[i + base]) &&
			f2fs_is_valid_blkaddr(sbi, ri->i_addr[i + base], DATA_GENERIC_ENHANCE))
			f2fs_err(sbi, "%s: inode[%lu] leak data addr[%d:%u]",
				__func__, inode->i_ino, i + base, ri->i_addr[i + base]);
		else
			ri->i_addr[i + base] = cpu_to_le32(addr);
	}

	for (i = 0; i < DEF_NIDS_PER_INODE; i++) {
		if (ri->i_nid[i])
			f2fs_err(sbi, "%s: inode[%lu] leak node addr[%d:%u]",
				__func__, inode->i_ino, i, ri->i_nid[i]);
		else
			ri->i_nid[i] = cpu_to_le32(0);
	}

	set_page_dirty(node_page);
	f2fs_put_page(node_page, 1);

	return 0;
}

static int __revoke_deduped_inode_begin(struct inode *dedup)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup);
	int err;

	f2fs_lock_op(sbi);

	err = f2fs_acquire_orphan_inode(sbi);	/* for layer inode */
	if (err) {
		f2fs_unlock_op(sbi);
		f2fs_err(sbi, "revoke inode[%lu] begin fail, ret:%d",
			dedup->i_ino, err);
		return err;
	}

	f2fs_add_orphan_inode(dedup);

	set_inode_flag(dedup, FI_REVOKE_DEDUP);

	f2fs_unlock_op(sbi);

	return 0;
}

static void f2fs_dedup_imonitor_send(struct inode *dedup,
		nid_t inner_ino, const char *source, char *reason)
{
	struct hiview_hievent *hi_event = NULL;
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup);
	int size;
	unsigned int ret = 0;

	hi_event = hiview_hievent_create(DEDUP_BD_ID);
	if (!hi_event) {
		f2fs_warn(sbi, "%s:create eventobj failed", __func__);
		return;
	}

	/*
	 * Since hiview just support 'int' type, we should convert
	 * file size unit from bytes to KB, avoid overflowing.
	 */
	size = min((unsigned long)(i_size_read(dedup) >> F2FS_KBYTE_SHIFT),
			(unsigned long)INT_MAX);

	ret = ret | hiview_hievent_put_integral(hi_event,
			"ReDedupFileIno", dedup->i_ino);
	ret = ret | hiview_hievent_put_integral(hi_event,
			"ReDedupInnerIno", inner_ino);
	ret = ret | hiview_hievent_put_integral(hi_event,
			"ReDedupSize", (long long)size);
	ret = ret | hiview_hievent_put_string(hi_event,
			"ReDedupReason", source);
	ret = ret | hiview_hievent_put_string(hi_event,
			"ReDedupFailedReason", reason);
	if (ret)
		goto out;

	ret = hiview_hievent_report(hi_event);
out:
	if (ret <= 0)
		f2fs_warn(sbi, "%s send hievent failed, err: %d", __func__, ret);

	hiview_hievent_destroy(hi_event);
}

/*
 * For kernel version < 5.4.0, we depend on inode lock in direct read IO
 */
static void dedup_wait_dio(struct inode *inode)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		down_write(&fi->i_gc_rwsem[READ]);
#endif
		inode_dio_wait(inode);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		up_write(&fi->i_gc_rwsem[READ]);
#endif
}
static void prepare_free_inner_inode(struct inode *inode, struct inode *inner)
{
	struct f2fs_inode_info *fi = F2FS_I(inner);

	fi->i_flags &= ~F2FS_IMMUTABLE_FL;
	f2fs_set_inode_flags(inner);
	f2fs_mark_inode_dirty_sync(inner, true);

	/*
	 * Before free inner inode, we should wait all reader of
	 * the inner complete to avoid UAF or read unexpected data.
	 */
	wait_event(fi->dedup_wq,
			atomic_read(&fi->inflight_read_io) == 0);

	dedup_wait_dio(inode);
}

static int __revoke_deduped_inode_end(struct inode *dedup)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup);
	struct f2fs_inode_info *fi = F2FS_I(dedup);
	struct inode *inner = NULL;
	int err;

	f2fs_lock_op(sbi);

	f2fs_remove_orphan_inode(sbi, dedup->i_ino);

	down_write(&fi->i_sem);
	clear_inode_flag(dedup, FI_REVOKE_DEDUP);
	clear_inode_flag(dedup, FI_DEDUPED);
	clear_inode_flag(dedup, FI_META_UN_MODIFY);
	clear_inode_flag(dedup, FI_DATA_UN_MODIFY);

	/*
	 * other reader flow:
	 * 1) lock inode
	 * 2) judge whether inner_inode is NULL
	 * 3) if no, then __iget inner inode
	 */
	inner = fi->inner_inode;
	fi->inner_inode = NULL;
	fi->dedup_cp_ver = cur_cp_version(F2FS_CKPT(sbi));
	up_write(&fi->i_sem);

	err = f2fs_acquire_orphan_inode(sbi);	/* for inner inode */
	if (err) {
		set_sbi_flag(sbi, SBI_NEED_FSCK);	/* delete inner inode */
		f2fs_set_need_fsck_report();
		f2fs_warn(sbi,
			"%s: orphan failed (ino=%lx), run fsck to fix.",
			__func__, inner->i_ino);
	} else {
		f2fs_drop_deduped_link(inner);
	}
	f2fs_unlock_op(sbi);

	trace_f2fs_dedup_revoke_inode(dedup, inner);

	if (inner->i_nlink == 0)
		prepare_free_inner_inode(dedup, inner);

	iput(inner);
	return err;
}

bool f2fs_is_hole_blkaddr(struct inode *inode, pgoff_t pgofs)
{
	struct dnode_of_data dn;
	block_t blkaddr;
	int err = 0;

	if (f2fs_has_inline_data(inode) ||
		f2fs_has_inline_dentry(inode))
		return false;

	set_new_dnode(&dn, inode, NULL, NULL, 0);
	err = f2fs_get_dnode_of_data(&dn, pgofs, LOOKUP_NODE);
	if (err && err != -ENOENT)
		return false;

	/* direct node does not exists */
	if (err == -ENOENT)
		return true;

	blkaddr = f2fs_data_blkaddr(&dn);
	f2fs_put_dnode(&dn);

	if (__is_valid_data_blkaddr(F2FS_I_SB(inode), blkaddr) &&
		!f2fs_is_valid_blkaddr(F2FS_I_SB(inode),
			blkaddr, DATA_GENERIC))
		return false;

	if (blkaddr != NULL_ADDR)
		return false;

	return true;
}

static int revoke_deduped_blocks(struct inode *dedup, pgoff_t page_idx, int len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup);
	struct address_space *mapping = dedup->i_mapping;
	pgoff_t redirty_idx = page_idx;
	int i, page_len = 0, ret = 0;
	struct dnode_of_data dn;
	filler_t *filler = NULL;
	struct page *page;
	LIST_HEAD(pages);
	bool lock2 = false;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0))
	filler = mapping->a_ops->readpage;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	__do_page_cache_readahead(mapping, NULL, page_idx, len, 0);
#else
	DEFINE_READAHEAD(ractl, NULL, mapping, page_idx);
	page_cache_ra_unbounded(&ractl, len, 0);
#endif
	/* readahead pages in file */
	for (i = 0; i < len; i++, page_idx++) {
		page = read_cache_page(mapping, page_idx, filler, NULL);
		if (IS_ERR(page)) {
			ret = PTR_ERR(page);
			goto out;
		}
		page_len++;
		LOG_PAGE_INTO_LIST(pages, page);
	}

	/* rewrite pages above */
	for (i = 0; i < page_len; i++, redirty_idx++) {
		page = find_lock_page(mapping, redirty_idx);
		if (!page) {
			ret = -ENOMEM;
			break;
		}

		if (!f2fs_is_hole_blkaddr(F2FS_I(dedup)->inner_inode, redirty_idx)) {
			lock2 = f2fs_do_map_lock(sbi, F2FS_GET_BLOCK_PRE_AIO, true,
						 false);
			set_new_dnode(&dn, dedup, NULL, NULL, 0);
			ret = f2fs_get_block(&dn, redirty_idx);
			f2fs_put_dnode(&dn);
			(void)f2fs_do_map_lock(sbi, F2FS_GET_BLOCK_PRE_AIO, false,
						   lock2);
			set_page_dirty(page);
		}

		f2fs_put_page(page, 1);
		if (ret)
			break;
	}

out:
	while (!list_empty(&pages))
		FREE_FIRST_PAGE_IN_LIST(pages);

	return ret;
}

static int __revoke_deduped_data(struct inode *dedup)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup);
	pgoff_t page_idx = 0, last_idx;
	int blk_per_seg = sbi->blocks_per_seg;
	int count;
	int ret1 = 0;
	int ret2 = 0;

	f2fs_set_inode_addr(dedup, NULL_ADDR);
	last_idx = DIV_ROUND_UP(i_size_read(dedup), PAGE_SIZE);

	count = last_idx - page_idx;
	while (count) {
		int len = min(blk_per_seg, count);
		ret1 = revoke_deduped_blocks(dedup, page_idx, len);
		if (ret1 < 0)
			break;

		filemap_fdatawrite(dedup->i_mapping);

		count -= len;
		page_idx += len;
	}

	ret2 = filemap_write_and_wait_range(dedup->i_mapping, 0,
			LLONG_MAX);
	if (ret1 || ret2)
		f2fs_warn(sbi, "%s: The deduped inode[%lu] revoked fail(errno=%d,%d).",
				__func__, dedup->i_ino, ret1, ret2);

	return ret1 ? : ret2;
}

static void _revoke_error_handle(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	f2fs_lock_op(sbi);
	f2fs_truncate_dedup_inode(inode, FI_REVOKE_DEDUP);
	f2fs_remove_orphan_inode(sbi, inode->i_ino);
	F2FS_I(inode)->dedup_cp_ver = cur_cp_version(F2FS_CKPT(sbi));
	f2fs_unlock_op(sbi);
	trace_f2fs_dedup_revoke_fail(inode, F2FS_I(inode)->inner_inode);
}

static void f2fs_dedup_revoke_event_report(struct inode *dedup,
		nid_t inner_ino, const char *source, int revoke_result)
{
	char result[REVOKE_FAIL_REASON_SIZE];

	f2fs_info(F2FS_I_SB(dedup),
		"%s trigger revoke ret[%d], inode[%lu], inner ino[%lu]",
		source, revoke_result, dedup->i_ino, inner_ino);

	snprintf(result, sizeof(result), "%d", revoke_result);
	f2fs_dedup_imonitor_send(dedup, inner_ino, source, result);
}

/*
 * need inode_lock by caller
 */
int f2fs_revoke_deduped_inode(struct inode *dedup, const char *revoke_source)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup);
	int err = 0;
	static DEFINE_RATELIMIT_STATE(revoke_bigdata_rs,
				HZ, F2FS_REVOKE_DSM_LIMIT);
	struct inode *inner_inode = NULL;
	nid_t inner_ino = 0;

	if (unlikely(f2fs_cp_error(sbi)))
		return -EIO;

	if (!f2fs_is_outer_inode(dedup))
		return -EINVAL;

	err = f2fs_dquot_initialize(dedup);
	if (err)
		return err;

	f2fs_balance_fs(sbi, true);

	inner_inode = F2FS_I(dedup)->inner_inode;
	if (inner_inode)
		inner_ino = inner_inode->i_ino;

	err = __revoke_deduped_inode_begin(dedup);
	if (err)
		goto ret;

	err = __revoke_deduped_data(dedup);
	if (err) {
		_revoke_error_handle(dedup);
		goto ret;
	}

	err = __revoke_deduped_inode_end(dedup);

ret:
	if (__ratelimit(&revoke_bigdata_rs))
		f2fs_dedup_revoke_event_report(dedup, inner_ino,
						revoke_source, err);
	return err;
}

static void f2fs_set_modify_check(struct inode *inode,
		struct f2fs_modify_check_info *info)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (info->flag & DEDUP_META_UN_MODIFY_FL) {
		if (is_inode_flag_set(inode, FI_META_UN_MODIFY))
			f2fs_err(sbi,
				"inode[%lu] had set meta unmodified flag",
				inode->i_ino);
		else
			set_inode_flag(inode, FI_META_UN_MODIFY);
	}

	if (info->flag & DEDUP_DATA_UN_MODIFY_FL) {
		if (is_inode_flag_set(inode, FI_DATA_UN_MODIFY))
			f2fs_err(sbi,
				"inode[%lu] had set data unmodified flag",
				inode->i_ino);
		else
			set_inode_flag(inode, FI_DATA_UN_MODIFY);
	}
}

static void f2fs_get_modify_check(struct inode *inode,
		struct f2fs_modify_check_info *info)
{
	memset(&(info->flag), 0, sizeof(info->flag));

	if (is_inode_flag_set(inode, FI_META_UN_MODIFY))
		info->flag = info->flag | DEDUP_META_UN_MODIFY_FL;

	if (is_inode_flag_set(inode, FI_DATA_UN_MODIFY))
		info->flag = info->flag | DEDUP_DATA_UN_MODIFY_FL;
}

static void f2fs_clear_modify_check(struct inode *inode,
		struct f2fs_modify_check_info *info)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (info->flag & DEDUP_META_UN_MODIFY_FL) {
		if (!is_inode_flag_set(inode, FI_META_UN_MODIFY)) {
			f2fs_err(sbi,
				"inode[%lu] had clear unmodified meta flag",
				inode->i_ino);
		}

		clear_inode_flag(inode, FI_META_UN_MODIFY);
	}

	if (info->flag & DEDUP_DATA_UN_MODIFY_FL) {
		if (!is_inode_flag_set(inode, FI_DATA_UN_MODIFY)) {
			f2fs_err(sbi,
				"inode[%lu] had clear unmodified data flag",
				inode->i_ino);
		}

		clear_inode_flag(inode, FI_DATA_UN_MODIFY);
	}

	f2fs_mark_inode_dirty_sync(inode, true);
}

bool f2fs_inode_support_dedup(struct f2fs_sb_info *sbi,
		struct inode *inode)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_inode *ri;

	if (!f2fs_sb_has_dedup(sbi))
		return false;

	if (!f2fs_has_extra_attr(inode))
		return false;

	if (!F2FS_FITS_IN_INODE(ri, fi->i_extra_isize, i_inner_ino))
		return false;

	return true;
}

static int f2fs_inode_param_check(struct f2fs_sb_info *sbi,
		struct inode *inode, int type)
{
	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (type != INNER_INODE && inode->i_size < DEDUP_MIN_SIZE) {
		f2fs_err(sbi, "dedup fails, inode[%lu] size < %lu bytes.",
			inode->i_ino, DEDUP_MIN_SIZE);
		return -EINVAL;
	}

	if (type == OUTER_INODE &&
		!is_inode_flag_set(inode, FI_DATA_UN_MODIFY)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] has been modified.",
			inode->i_ino);
		return -EINVAL;
	}

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_compressed_inode(inode)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] has been compressed.",
			inode->i_ino);
		return -EACCES;
	}
#endif

	if (IS_VERITY(inode)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] enable verity.",
			inode->i_ino);
		return -EACCES;
	}

	if (f2fs_is_atomic_file(inode)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] is atomic file.",
			inode->i_ino);
		return -EACCES;
	}

	if (f2fs_is_volatile_file(inode)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] is volatile file.",
			inode->i_ino);
		return -EACCES;
	}

	if (f2fs_is_pinned_file(inode)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] is pinned file.",
			inode->i_ino);
		return -EACCES;
	}

	if (type != INNER_INODE && IS_IMMUTABLE(inode)) {
		f2fs_err(sbi, "dedup fails, inode[%lu] is immutable.",
			inode->i_ino);
		return -EACCES;
	}
	return 0;
}

static int f2fs_dedup_param_check(struct f2fs_sb_info *sbi,
		struct inode *inode1, int type1,
		struct inode *inode2, int type2)
{
	int ret;

	if (inode1->i_sb != inode2->i_sb || inode1 == inode2) {
		f2fs_err(sbi, "%s: input inode[%lu] and [%lu] are illegal.",
			__func__, inode1->i_ino, inode2->i_ino);
		return -EINVAL;
	}

	if (type1 == OUTER_INODE && type2 == OUTER_INODE &&
		inode1->i_size != inode2->i_size) {
		f2fs_err(sbi,
			"dedup file size not match inode1[%lu] %u, inode2[%lu] %u",
			inode1->i_ino, inode1->i_size,
			inode2->i_ino, inode2->i_size);
		return -EINVAL;
	}

	ret = f2fs_inode_param_check(sbi, inode1, type1);
	if (ret)
		return ret;

	ret = f2fs_inode_param_check(sbi, inode2, type2);
	if (ret)
		return ret;

	return 0;
}

static void f2fs_dedup_dirty_data_report(struct inode *inode,
		const char *source, int nrpages)
{
	char result[REVOKE_FAIL_REASON_SIZE];

	snprintf(result, sizeof(result), "set unmodify, dirty pages: %d", nrpages);
	f2fs_dedup_imonitor_send(inode, 0, source, result);
}

static int f2fs_ioc_modify_check(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_modify_check_info info;
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int ret;

	if (unlikely(f2fs_cp_error(sbi)))
		return -EIO;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (!f2fs_inode_support_dedup(sbi, inode))
		return -EOPNOTSUPP;

	if (f2fs_has_inline_data(inode))
		return -EINVAL;

	if (copy_from_user(&info,
		(struct f2fs_modify_check_info __user *)arg, sizeof(info)))
		return -EFAULT;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);
	if (info.mode & DEDUP_SET_MODIFY_CHECK) {
		struct address_space *mapping = inode->i_mapping;
		bool dirty = false;
		int nrpages = 0;

		if (mapping_mapped(mapping)) {
			f2fs_err(sbi, "inode[%lu] has mapped vma", inode->i_ino);
			ret = -EBUSY;
			goto out;
		}

		ret = f2fs_inode_param_check(sbi, inode, NORMAL_INODE);
		if (ret)
			goto out;

		if (mapping_tagged(mapping, PAGECACHE_TAG_DIRTY) ||
				mapping_tagged(mapping, PAGECACHE_TAG_WRITEBACK)) {
			dirty = true;
			nrpages = get_dirty_pages(inode);
		}

		if (dirty && (info.flag & DEDUP_SYNC_DATA)) {
			f2fs_dedup_dirty_data_report(inode, __func__, nrpages);

			ret = filemap_write_and_wait_range(mapping, 0, LLONG_MAX);
			if (ret) {
				f2fs_err(sbi, "inode[%lu] write data fail(%d)\n",
						inode->i_ino, ret);
				goto out;
			}
		} else if (dirty) {
			f2fs_err(sbi, "inode[%lu] have dirty page[%d]\n",
					inode->i_ino, nrpages);
			ret = -EINVAL;
			goto out;
		}

		f2fs_set_modify_check(inode, &info);
	} else if (info.mode & DEDUP_GET_MODIFY_CHECK) {
		f2fs_get_modify_check(inode, &info);
	} else if (info.mode & DEDUP_CLEAR_MODIFY_CHECK) {
		f2fs_clear_modify_check(inode, &info);
	} else {
		ret = -EINVAL;
	}

out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);

	if (copy_to_user((struct f2fs_modify_check_info __user *)arg,
		&info, sizeof(info)))
		ret = -EFAULT;

	return ret;
}

static int f2fs_ioc_dedup_permission_check(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (unlikely(f2fs_cp_error(sbi)))
		return -EIO;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (!f2fs_inode_support_dedup(sbi, inode))
		return -EOPNOTSUPP;

	if (f2fs_has_inline_data(inode))
		return -EINVAL;

	return f2fs_inode_param_check(sbi, inode, OUTER_INODE);
}

static int f2fs_copy_data(struct inode *dst_inode,
		struct inode *src_inode, pgoff_t page_idx, int len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dst_inode);
	struct address_space *src_mapping;
	struct address_space *dst_mapping;
	filler_t *filler = NULL;
	struct page *page, *newpage;
	pgoff_t copy_idx = page_idx;
	int i, page_len = 0, ret = 0;
	struct dnode_of_data dn;
	LIST_HEAD(pages);
	bool lock2 = false;

	src_mapping = src_inode->i_mapping;
	dst_mapping = dst_inode->i_mapping;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0))
	filler = src_mapping->a_ops->readpage;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	__do_page_cache_readahead(src_mapping, NULL, page_idx, len, 0);
#else
	DEFINE_READAHEAD(ractl, NULL, src_mapping, page_idx);
	page_cache_ra_unbounded(&ractl, len, 0);
#endif
	for (i = 0; i < len; i++, page_idx++) {
		page = read_cache_page(src_mapping, page_idx, filler, NULL);
		if (IS_ERR(page)) {
			ret = PTR_ERR(page);
			goto out;
		}
		page_len++;
		LOG_PAGE_INTO_LIST(pages, page);
	}

	for (i = 0; i < page_len; i++, copy_idx++) {
		page = find_lock_page(src_mapping, copy_idx);
		if (!page) {
			ret = -ENOMEM;
			break;
		}

		if (f2fs_is_hole_blkaddr(src_inode, copy_idx)) {
			f2fs_put_page(page, 1);
			continue;
		}

		lock2 = f2fs_do_map_lock(sbi, F2FS_GET_BLOCK_PRE_AIO, true,
					 false);
		set_new_dnode(&dn, dst_inode, NULL, NULL, 0);
		ret = f2fs_get_block(&dn, copy_idx);
		f2fs_put_dnode(&dn);
		(void)f2fs_do_map_lock(sbi, F2FS_GET_BLOCK_PRE_AIO, false,
				       lock2);
		if (ret) {
			f2fs_put_page(page, 1);
			break;
		}

		newpage = f2fs_grab_cache_page(dst_mapping, copy_idx, true);
		if (!newpage) {
			ret = -ENOMEM;
			f2fs_put_page(page, 1);
			break;
		}
		f2fs_copy_page(page, newpage);

		set_page_dirty(newpage);
		f2fs_put_page(newpage, 1);
		f2fs_put_page(page, 1);
	}

out:
	while (!list_empty(&pages))
		FREE_FIRST_PAGE_IN_LIST(pages);

	return ret;
}

static int f2fs_clone_data(struct inode *dst_inode,
		struct inode *src_inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(src_inode);
	pgoff_t page_idx = 0, last_idx;
	int blk_per_seg = sbi->blocks_per_seg;
	int count;
	int ret = 0;

	f2fs_balance_fs(sbi, true);
	last_idx = DIV_ROUND_UP(i_size_read(src_inode), PAGE_SIZE);
	count = last_idx - page_idx;

	while (count) {
		int len = min(blk_per_seg, count);
		ret = f2fs_copy_data(dst_inode, src_inode, page_idx, len);
		if (ret < 0)
			break;

		filemap_fdatawrite(dst_inode->i_mapping);
		count -= len;
		page_idx += len;
	}

	if (!ret)
		ret = filemap_write_and_wait_range(dst_inode->i_mapping, 0,
				LLONG_MAX);

	return ret;
}

static int __f2fs_ioc_clone_file(struct inode *dst_inode,
		struct inode *src_inode, struct f2fs_clone_info *info)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(src_inode);
	int ret;

	ret = f2fs_convert_inline_inode(dst_inode);
	if (ret) {
		f2fs_err(sbi,
			"inode[%lu] convert inline inode failed, ret:%d",
			dst_inode->i_ino, ret);
		return ret;
	}

	if (info->flags & DEDUP_CLONE_META) {
		dst_inode->i_uid = src_inode->i_uid;
		dst_inode->i_gid = src_inode->i_gid;
		dst_inode->i_size = src_inode->i_size;
	}

	if (info->flags & DEDUP_CLONE_DATA) {
		dst_inode->i_size = src_inode->i_size;
		ret = f2fs_clone_data(dst_inode, src_inode);
		if (ret) {
			/* No need to truncate, beacuse tmpfile will be removed. */
			f2fs_err(sbi,
				"src inode[%lu] dst inode[%lu] ioc clone failed. ret=%d",
				src_inode->i_ino, dst_inode->i_ino, ret);
			return ret;
		}
	}

	set_inode_flag(dst_inode, FI_DATA_UN_MODIFY);

	return 0;
}

static int f2fs_ioc_clone_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct inode *src_inode;
	struct f2fs_clone_info info;
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct fd f;
	int ret = 0;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (copy_from_user(&info, (struct f2fs_clone_info __user *)arg, sizeof(info)))
		return -EFAULT;

	f = fdget_pos(info.src_fd);
	if (!f.file)
		return -EBADF;

	src_inode = file_inode(f.file);
	if (inode->i_sb != src_inode->i_sb) {
		f2fs_err(sbi, "%s: files should be in same FS ino:%lu, src_ino:%lu",
				__func__, inode->i_ino, src_inode->i_ino);
		ret = -EINVAL;
		goto out;
	}

	if (!f2fs_inode_support_dedup(sbi, src_inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	ret = mnt_want_write_file(filp);
	if (ret)
		goto out;

	inode_lock(inode);
	ret = f2fs_dedup_param_check(sbi, src_inode, OUTER_INODE,
			inode, INNER_INODE);
	if (ret)
		goto unlock;

	ret = __f2fs_ioc_clone_file(inode, src_inode, &info);
	if (ret)
		goto unlock;

	F2FS_I(inode)->i_flags |= F2FS_IMMUTABLE_FL;
	f2fs_set_inode_flags(inode);
	f2fs_mark_inode_dirty_sync(inode, true);

unlock:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
out:
	fdput_pos(f);
	return ret;
}

static inline void _truncate_error_handle(struct inode *inode,
		int ret)
{
	set_sbi_flag(F2FS_I_SB(inode), SBI_NEED_FSCK);
	f2fs_set_need_fsck_report();
	f2fs_err(F2FS_I_SB(inode),
		"truncate data failed, inode:%lu ret:%d",
		inode->i_ino, ret);
}

int f2fs_truncate_dedup_inode(struct inode *inode, unsigned int flag)
{
	int ret = 0;

	if (!f2fs_is_outer_inode(inode)) {
		f2fs_err(F2FS_I_SB(inode),
			"inode:%lu is not dedup inode", inode->i_ino);
		f2fs_bug_on(F2FS_I_SB(inode), 1);
		return 0;
	}

	clear_inode_flag(inode, flag);

	ret = f2fs_truncate_blocks(inode, 0, false);
	if (ret)
		goto err;

	ret = f2fs_set_inode_addr(inode, DEDUP_ADDR);
	if (ret)
		goto err;

	return 0;
err:
	_truncate_error_handle(inode, ret);
	return ret;
}

static int is_inode_match_dir_crypt_policy(struct dentry *dir,
		struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (IS_ENCRYPTED(d_inode(dir)) &&
		!fscrypt_has_permitted_context(d_inode(dir), inode)) {
		f2fs_err(sbi, "inode[%lu] not match dir[%lu] fscrypt policy",
			inode->i_ino, d_inode(dir)->i_ino);
		return -EPERM;
	}

	return 0;
}

static int deduped_files_match_fscrypt_policy(struct file *file1,
		struct file *file2)
{
	struct dentry *dir1 = dget_parent(file_dentry(file1));
	struct dentry *dir2 = dget_parent(file_dentry(file2));
	struct inode *inode1 = file_inode(file1);
	struct inode *inode2 = file_inode(file2);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode1);
	int err = 0;

	if (IS_ENCRYPTED(d_inode(dir1)) &&
		!fscrypt_has_permitted_context(d_inode(dir1), inode2)) {
		f2fs_err(sbi, "dir[%lu] inode[%lu] and inode[%lu] fscrypt policy not match.",
			d_inode(dir1)->i_ino, inode1->i_ino, inode2->i_ino);
		err = -EPERM;
		goto out;
	}

	if (IS_ENCRYPTED(d_inode(dir2)) &&
		!fscrypt_has_permitted_context(d_inode(dir2), inode1)) {
		f2fs_err(sbi, "inode[%lu] and dir[%lu] inode[%lu] fscrypt policy not match.",
			inode1->i_ino, d_inode(dir2)->i_ino, inode2->i_ino);
		err = -EPERM;
	}

out:
	dput(dir2);
	dput(dir1);
	return err;
}

static int f2fs_compare_page(struct page *src, struct page *dst)
{
	int ret;
	char *src_kaddr = kmap_atomic(src);
	char *dst_kaddr = kmap_atomic(dst);

	flush_dcache_page(src);
	flush_dcache_page(dst);

	ret = memcmp(dst_kaddr, src_kaddr, PAGE_SIZE);
	kunmap_atomic(src_kaddr);
	kunmap_atomic(dst_kaddr);

	return ret;
}

static inline void lock_two_pages(struct page *page1, struct page *page2)
{
	lock_page(page1);
	if (page1 != page2)
		lock_page(page2);
}

static inline void unlock_two_pages(struct page *page1, struct page *page2)
{
	unlock_page(page1);
	if (page1 != page2)
		unlock_page(page2);
}

static bool dedup_file_is_same(struct inode *src, struct inode *dst, int nr_pages)
{
	struct page *src_page, *dst_page;
	pgoff_t index, last_idx;
	int i, ret;
	bool same = true;

	if (i_size_read(src) != i_size_read(dst))
		return false;

	last_idx = DIV_ROUND_UP(i_size_read(src), PAGE_SIZE);

	for (i = 0; i < nr_pages; i++) {
		index = get_random_int() % last_idx;

		src_page = read_mapping_page(src->i_mapping, index, NULL);
		if (IS_ERR(src_page)) {
			ret = PTR_ERR(src_page);
			same = false;
			break;
		}

		dst_page = read_mapping_page(dst->i_mapping, index, NULL);
		if (IS_ERR(dst_page)) {
			ret = PTR_ERR(dst_page);
			put_page(src_page);
			same = false;
			break;
		}

		lock_two_pages(src_page, dst_page);
		if (!PageUptodate(src_page) || !PageUptodate(dst_page) ||
				src_page->mapping != src->i_mapping ||
				dst_page->mapping != dst->i_mapping) {
			ret = -EINVAL;
			same = false;
			goto unlock;
		}
		ret = f2fs_compare_page(src_page, dst_page);
		if (ret)
			same = false;
unlock:
		unlock_two_pages(src_page, dst_page);
		put_page(dst_page);
		put_page(src_page);

		if (!same) {
			f2fs_err(F2FS_I_SB(src),
					"src: %lu, dst: %lu page index: %lu is diff ret[%d]",
					src->i_ino, dst->i_ino, index, ret);
			break;
		}
	}

	return same;
}

static int __f2fs_ioc_create_layered_inode(struct inode *inode,
		struct inode *inner_inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int ret;
	struct f2fs_inode_info *fi = F2FS_I(inode);

	if (!dedup_file_is_same(inode, inner_inode, DEDUP_COMPARE_PAGES))
		return -ESTALE;

	f2fs_lock_op(sbi);

	ret = f2fs_acquire_orphan_inode(sbi);
	if (ret) {
		f2fs_err(sbi,
			"create layer file acquire orphan fail, ino[%lu], inner[%lu]",
			inode->i_ino, inner_inode->i_ino);
		f2fs_unlock_op(sbi);
		return ret;
	}
	f2fs_add_orphan_inode(inode);

	down_write(&F2FS_I(inner_inode)->i_sem);
	igrab(inner_inode);
	set_inode_flag(inner_inode, FI_INNER_INODE);
	set_inode_flag(inner_inode, FI_DEDUPED);
	f2fs_i_links_write(inner_inode, true);
	up_write(&F2FS_I(inner_inode)->i_sem);

	down_write(&fi->i_sem);
	fi->inner_inode = inner_inode;
	set_inode_flag(inode, FI_DEDUPED);
	up_write(&fi->i_sem);

	f2fs_remove_orphan_inode(sbi, inner_inode->i_ino);
	f2fs_unlock_op(sbi);

	wait_event(fi->dedup_wq,
			atomic_read(&fi->inflight_read_io) == 0);
	dedup_wait_dio(inode);

	down_write(&fi->i_gc_rwsem[WRITE]);
	/* GC may dirty pages before holding lock */
	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	f2fs_lock_op(sbi);
	f2fs_remove_orphan_inode(sbi, inode->i_ino);
	ret = f2fs_truncate_dedup_inode(inode, FI_DOING_DEDUP);
	/*
	 * Since system may do checkpoint after unlock cp,
	 * we set cp_ver here to let fsync know dedup have finish.
	 */
	F2FS_I(inode)->dedup_cp_ver = cur_cp_version(F2FS_CKPT(sbi));
	f2fs_unlock_op(sbi);

out:
	up_write(&fi->i_gc_rwsem[WRITE]);
	f2fs_dedup_info(sbi, "inode[%lu] create layered success, inner[%lu] ret: %d",
			inode->i_ino, inner_inode->i_ino, ret);
	trace_f2fs_dedup_ioc_create_layered_inode(inode, inner_inode);
	return ret;
}

static int f2fs_ioc_create_layered_inode(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct inode *inner_inode;
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_dedup_src info;
	struct dentry *dir;
	struct fd f;
	int ret;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (!f2fs_inode_support_dedup(sbi, inode))
		return -EOPNOTSUPP;

	if (copy_from_user(&info, (struct f2fs_dedup_src __user *)arg, sizeof(info)))
		return -EFAULT;

	f = fdget_pos(info.inner_fd);
	if (!f.file)
		return -EBADF;

	ret = mnt_want_write_file(filp);
	if (ret)
		goto out;

	inode_lock(inode);
	if (f2fs_is_deduped_inode(inode)) {
		f2fs_err(sbi, "The inode[%lu] has been two layer file.",
			inode->i_ino);
		ret = -EINVAL;
		goto unlock;
	}

	inner_inode = file_inode(f.file);
	if (inode->i_sb != inner_inode->i_sb) {
		f2fs_err(sbi, "%s files should be in same FS ino:%lu, inner_ino:%lu",
				__func__, inode->i_ino, inner_inode->i_ino);
		ret = -EINVAL;
		goto unlock;
	}

	if (!IS_IMMUTABLE(inner_inode)) {
		f2fs_err(sbi, "create layer fail inner[%lu] is not immutable.",
			inner_inode->i_ino);
		ret = -EINVAL;
		goto unlock;
	}

	ret = f2fs_dedup_param_check(sbi, inode, OUTER_INODE,
			inner_inode, INNER_INODE);
	if (ret)
		goto unlock;

	if (inode->i_nlink == 0) {
		f2fs_err(sbi,
			"The inode[%lu] has been removed.", inode->i_ino);
		ret = -ENOENT;
		goto unlock;
	}

	dir = dget_parent(file_dentry(filp));
	ret = is_inode_match_dir_crypt_policy(dir, inner_inode);
	dput(dir);
	if (ret)
		goto unlock;

	filemap_fdatawrite(inode->i_mapping);
	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto unlock;

	ret = __f2fs_ioc_create_layered_inode(inode, inner_inode);

unlock:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
out:
	fdput_pos(f);
	return ret;
}

static int __f2fs_ioc_dedup_file(struct inode *base_inode,
		struct inode *dedup_inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup_inode);
	struct inode *inner = get_inner_inode(base_inode);
	int ret = 0;
	struct f2fs_inode_info *fi = F2FS_I(dedup_inode);

	if (!inner)
		return -EBADF;

	if (!dedup_file_is_same(base_inode, dedup_inode, DEDUP_COMPARE_PAGES)) {
		put_inner_inode(inner);
		return -ESTALE;
	}

	f2fs_lock_op(sbi);

	ret = f2fs_acquire_orphan_inode(sbi);
	if (ret) {
		f2fs_unlock_op(sbi);
		f2fs_err(sbi,
			"dedup file acquire orphan fail, ino[%lu], base ino[%lu]",
			dedup_inode->i_ino, base_inode->i_ino);
		put_inner_inode(inner);
		return ret;
	}
	f2fs_add_orphan_inode(dedup_inode);

	down_write(&fi->i_sem);
	fi->inner_inode = inner;
	set_inode_flag(dedup_inode, FI_DEDUPED);
	set_inode_flag(dedup_inode, FI_DOING_DEDUP);
	up_write(&fi->i_sem);

	down_write(&F2FS_I(inner)->i_sem);
	f2fs_i_links_write(inner, true);
	up_write(&F2FS_I(inner)->i_sem);
	f2fs_unlock_op(sbi);

	wait_event(fi->dedup_wq,
			atomic_read(&fi->inflight_read_io) == 0);
	dedup_wait_dio(dedup_inode);

	down_write(&fi->i_gc_rwsem[WRITE]);
	/* GC may dirty pages before holding lock */
	ret = filemap_write_and_wait_range(dedup_inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	f2fs_lock_op(sbi);
	f2fs_remove_orphan_inode(sbi, dedup_inode->i_ino);
	ret = f2fs_truncate_dedup_inode(dedup_inode, FI_DOING_DEDUP);
	/*
	 * Since system may do checkpoint after unlock cp,
	 * we set cp_ver here to let fsync know dedup have finish.
	 */
	F2FS_I(dedup_inode)->dedup_cp_ver = cur_cp_version(F2FS_CKPT(sbi));
	f2fs_unlock_op(sbi);
out:
	up_write(&fi->i_gc_rwsem[WRITE]);
	f2fs_dedup_info(sbi, "%s inode[%lu] dedup success, inner[%lu], ret[%d]",
			__func__, dedup_inode->i_ino, inner->i_ino, ret);
	trace_f2fs_dedup_ioc_dedup_inode(dedup_inode, inner);
	return ret;
}

static int f2fs_ioc_dedup_file(struct file *filp, unsigned long arg)
{
	struct inode *dedup_inode = file_inode(filp);
	struct inode *base_inode, *inner_inode;
	struct dentry *dir;
	struct f2fs_sb_info *sbi = F2FS_I_SB(dedup_inode);
	struct f2fs_dedup_dst info;
	struct fd f;
	int ret;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(dedup_inode))))
		return -EIO;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (!inode_owner_or_capable(dedup_inode))
		return -EACCES;

	if (!f2fs_inode_support_dedup(sbi, dedup_inode))
		return -EOPNOTSUPP;

	if (copy_from_user(&info, (struct f2fs_dedup_dst __user *)arg, sizeof(info)))
		return -EFAULT;

	f = fdget_pos(info.base_fd);
	if (!f.file)
		return -EBADF;

	base_inode = file_inode(f.file);
	if (dedup_inode->i_sb != base_inode->i_sb) {
		f2fs_err(sbi, "%s: files should be in same FS ino:%lu, base_ino:%lu",
				__func__, dedup_inode->i_ino, base_inode->i_ino);
		ret = -EINVAL;
		goto out;
	}

	if (!f2fs_inode_support_dedup(sbi, base_inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (base_inode == dedup_inode) {
		f2fs_err(sbi, "%s: input inode[%lu] and [%lu] are same.",
			__func__, base_inode->i_ino, dedup_inode->i_ino);
		ret = -EINVAL;
		goto out;
	}

	ret = mnt_want_write_file(filp);
	if (ret)
		goto out;

	inode_lock(dedup_inode);
	if (!inode_trylock(base_inode)) {
		f2fs_err(sbi, "inode[%lu] can't get lock", base_inode->i_ino);
		ret = -EAGAIN;
		goto unlock2;
	}

	if (f2fs_is_deduped_inode(dedup_inode)) {
		f2fs_err(sbi, "dedup inode[%lu] has been two layer inode",
			dedup_inode->i_ino);
		ret = -EINVAL;
		goto unlock1;
	}

	if (dedup_inode->i_nlink == 0) {
		f2fs_err(sbi,
			"dedup inode[%lu] has been removed.", dedup_inode->i_ino);
		ret = -ENOENT;
		goto unlock1;
	}

	if (!f2fs_is_outer_inode(base_inode)) {
		f2fs_err(sbi, "base inode[%lu] is not outer inode",
			base_inode->i_ino);
		ret = -EINVAL;
		goto unlock1;
	}

	ret = f2fs_dedup_param_check(sbi, base_inode, OUTER_INODE,
			dedup_inode, OUTER_INODE);
	if (ret)
		goto unlock1;

	dir = dget_parent(file_dentry(filp));
	inner_inode = get_inner_inode(base_inode);
	ret = is_inode_match_dir_crypt_policy(dir, inner_inode);
	put_inner_inode(inner_inode);
	dput(dir);
	if (ret)
		goto unlock1;

	ret = deduped_files_match_fscrypt_policy(filp, f.file);
	if (ret)
		goto unlock1;

	filemap_fdatawrite(dedup_inode->i_mapping);
	ret = filemap_write_and_wait_range(dedup_inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto unlock1;

	ret = __f2fs_ioc_dedup_file(base_inode, dedup_inode);

unlock1:
	inode_unlock(base_inode);
unlock2:
	inode_unlock(dedup_inode);
	mnt_drop_write_file(filp);
out:
	fdput_pos(f);
	return ret;
}

static int f2fs_ioc_dedup_revoke(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int ret = 0;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (!f2fs_inode_support_dedup(sbi, inode))
		return -EOPNOTSUPP;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);
	ret = f2fs_revoke_deduped_inode(inode, __func__);
	inode_unlock(inode);

	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_get_dedupd_file_info(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_dedup_file_info info = {0};
	struct inode *inner_inode;
	int ret = 0;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (!f2fs_inode_support_dedup(sbi, inode))
		return -EOPNOTSUPP;

	inode_lock(inode);

	if (!is_inode_flag_set(inode, FI_DEDUPED)) {
		info.is_layered = false;
		info.is_deduped = false;
	} else {
		info.is_layered = true;
		inner_inode = F2FS_I(inode)->inner_inode;
		if (inner_inode) {
			down_write(&F2FS_I(inner_inode)->i_sem);
			if (inner_inode->i_nlink > 1)
				info.is_deduped = true;

			info.group = inner_inode->i_ino;
			up_write(&F2FS_I(inner_inode)->i_sem);
		}
	}

	inode_unlock(inode);

	if (copy_to_user((struct f2fs_dedup_file_info __user *)arg, &info, sizeof(info)))
		ret = -EFAULT;

	return ret;
}

/* used for dedup big data statistics */
static int f2fs_ioc_get_dedup_sysinfo(struct file *filp, unsigned long arg)
{
	return 0;
}

#ifdef CONFIG_HP_FILE
static int f2fs_ioc_set_hp_file_wrapper(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	int ret = 0;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		ret = f2fs_revoke_deduped_inode(inode, __func__);
		if (ret) {
			inode_unlock(inode);
			mnt_drop_write_file(filp);
			return ret;
		}
	}
	inode_unlock(inode);
	mnt_drop_write_file(filp);

	return f2fs_ioc_set_hp_file(filp, arg);
}
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static vm_fault_t f2fs_filemap_fault(struct vm_fault *vmf)
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	vm_fault_t ret;
#else
static int f2fs_filemap_fault(struct vm_fault *vmf)
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	int ret;
#endif

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	ret = fscrypt_vm_op_check(inode);
	if (unlikely(ret & VM_FAULT_SIGSEGV)) {
		pr_err("[FBE3]%s: mmap_read_check return VM_FAULT_SIGSEGV\n",
		       __func__);
		return ret;
	}
#endif

	down_read(&F2FS_I(inode)->i_mmap_sem);
	ret = filemap_fault(vmf);
	up_read(&F2FS_I(inode)->i_mmap_sem);

	if (!ret)
		f2fs_update_iostat(F2FS_I_SB(inode), APP_MAPPED_READ_IO,
							F2FS_BLKSIZE);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	trace_f2fs_filemap_fault(inode, vmf->pgoff, (unsigned long)ret);
#endif

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static vm_fault_t f2fs_vm_page_mkwrite(struct vm_fault *vmf)
#else
static int f2fs_vm_page_mkwrite(struct vm_fault *vmf)
#endif
{
	struct page *page = vmf->page;
	struct inode *inode = file_inode(vmf->vma->vm_file);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct dnode_of_data dn;
	bool need_alloc = true;
	int err = 0;
	bool lock2 = false;

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V3
	err = fscrypt_vm_op_check(inode);
	if (unlikely(err & VM_FAULT_SIGSEGV)) {
		pr_err("[FBE3]%s: mmap_write_check return VM_FAULT_SIGSEGV\n",
			__func__);
		put_page(vmf->page);
		return err;
	}
#endif

	if (unlikely(IS_IMMUTABLE(inode)))
		return VM_FAULT_SIGBUS;

	inode_lock(inode);
	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
		err = f2fs_reserve_compress_blocks_internal(inode, VM_PAGE_WRITE);
		if (err) {
			inode_unlock(inode);
			goto err;
		}
#else
		inode_unlock(inode);
		return VM_FAULT_SIGBUS;
#endif
	}
	inode_unlock(inode);


	if (unlikely(f2fs_cp_error(sbi))) {
		err = -EIO;
		goto err;
	}

	if (!f2fs_is_checkpoint_ready(sbi)) {
		err = -ENOSPC;
		goto err;
	}

	err = f2fs_convert_inline_inode(inode);
	if (err)
		goto err;

#ifdef CONFIG_F2FS_FS_DEDUP
	inode_lock(inode);
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		err = f2fs_revoke_deduped_inode(inode, __func__);
		if (err) {
			inode_unlock(inode);
			goto err;
		}
	}
	inode_unlock(inode);
#endif

#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_compressed_inode(inode)) {
#else
	if (f2fs_compressed_file(inode)) {
#endif
		int ret = f2fs_is_compressed_cluster(inode, page->index);

		if (ret < 0) {
			err = ret;
			goto err;
		} else if (ret) {
			need_alloc = false;
		}
	}
#endif
	/* should do out of any locked page */
	if (need_alloc)
		f2fs_balance_fs(sbi, true);

	sb_start_pagefault(inode->i_sb);

	f2fs_bug_on(sbi, f2fs_has_inline_data(inode));

	file_update_time(vmf->vma->vm_file);
	down_read(&F2FS_I(inode)->i_mmap_sem);
	lock_page(page);
	if (unlikely(page->mapping != inode->i_mapping ||
			page_offset(page) > i_size_read(inode) ||
			!PageUptodate(page))) {
		unlock_page(page);
		err = -EFAULT;
		goto out_sem;
	}

	if (need_alloc) {
		/* block allocation */
		lock2 = f2fs_do_map_lock(sbi, F2FS_GET_BLOCK_PRE_AIO, true,
					 false);
		set_new_dnode(&dn, inode, NULL, NULL, 0);
		err = f2fs_get_block(&dn, page->index);
		(void)f2fs_do_map_lock(sbi, F2FS_GET_BLOCK_PRE_AIO, false,
				       lock2);
	}
#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	if (!need_alloc) {
		set_new_dnode(&dn, inode, NULL, NULL, 0);
		err = f2fs_get_dnode_of_data(&dn, page->index, LOOKUP_NODE);
		f2fs_put_dnode(&dn);
	}
#endif
	if (err) {
		unlock_page(page);
		goto out_sem;
	}
	f2fs_wait_on_page_writeback(page, DATA, false, true);

	/* wait for GCed page writeback via META_MAPPING */
	f2fs_wait_on_block_writeback(inode, dn.data_blkaddr);

	/*
	 * check to see if the page is mapped already (no holes)
	 */
	if (PageMappedToDisk(page))
		goto out_sem;

	/* page is wholly or partially inside EOF */
	if (((loff_t)(page->index + 1) << PAGE_SHIFT) >
						i_size_read(inode)) {
		loff_t offset;

		offset = i_size_read(inode) & ~PAGE_MASK;
		zero_user_segment(page, offset, PAGE_SIZE);
	}
	set_page_dirty(page);
	if (!PageUptodate(page))
		SetPageUptodate(page);

	f2fs_update_iostat(sbi, APP_MAPPED_IO, F2FS_BLKSIZE);
	f2fs_update_time(sbi, REQ_TIME);

	trace_f2fs_vm_page_mkwrite(page, DATA);
out_sem:
	up_read(&F2FS_I(inode)->i_mmap_sem);

	sb_end_pagefault(inode->i_sb);
err:
	return block_page_mkwrite_return(err);
}

static const struct vm_operations_struct f2fs_file_vm_ops = {
	.fault		= f2fs_filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite	= f2fs_vm_page_mkwrite,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
#ifdef CONFIG_SPECULATIVE_PAGE_FAULT
	.allow_speculation = filemap_allow_speculation,
#endif
#endif
};

static int get_parent_ino(struct inode *inode, nid_t *pino)
{
	struct dentry *dentry;

	/*
	 * Make sure to get the non-deleted alias.  The alias associated with
	 * the open file descriptor being fsync()'ed may be deleted already.
	 */
	dentry = d_find_alias(inode);
	if (!dentry)
		return 0;

	*pino = parent_ino(dentry);
	dput(dentry);
	return 1;
}

static inline enum cp_reason_type need_do_checkpoint(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	enum cp_reason_type cp_reason = CP_NO_NEEDED;

	if (!S_ISREG(inode->i_mode))
		cp_reason = CP_NON_REGULAR;
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	else if (f2fs_is_compressed_inode(inode))
#else
	else if (f2fs_compressed_file(inode))
#endif
		cp_reason = CP_COMPRESSED;
#ifdef CONFIG_F2FS_FS_DEDUP
	/*
	 * If inode have do dedup or revoke recently, we need to do
	 * checkpoint to avoid roll forward recovery after fsync,
	 * which may cause data inconsistency.
	 */
	else if (F2FS_I(inode)->dedup_cp_ver == cur_cp_version(F2FS_CKPT(sbi)))
		cp_reason = CP_DEDUPED;
#endif
	else if (inode->i_nlink != 1)
		cp_reason = CP_HARDLINK;
	else if (is_sbi_flag_set(sbi, SBI_NEED_CP))
		cp_reason = CP_SB_NEED_CP;
	else if (file_wrong_pino(inode))
		cp_reason = CP_WRONG_PINO;
	else if (!f2fs_space_for_roll_forward(sbi))
		cp_reason = CP_NO_SPC_ROLL;
	else if (!f2fs_is_checkpointed_node(sbi, F2FS_I(inode)->i_pino))
		cp_reason = CP_NODE_NEED_CP;
	else if (test_opt(sbi, FASTBOOT))
		cp_reason = CP_FASTBOOT_MODE;
	else if (F2FS_OPTION(sbi).active_logs == 2)
		cp_reason = CP_SPEC_LOG_NUM;
	else if (F2FS_OPTION(sbi).fsync_mode == FSYNC_MODE_STRICT &&
		f2fs_need_dentry_mark(sbi, inode->i_ino) &&
		f2fs_exist_written_data(sbi, F2FS_I(inode)->i_pino,
							TRANS_DIR_INO))
		cp_reason = CP_RECOVER_DIR;

	return cp_reason;
}

static bool need_inode_page_update(struct f2fs_sb_info *sbi, nid_t ino)
{
	struct page *i = find_get_page(NODE_MAPPING(sbi), ino);
	bool ret = false;
	/* But we need to avoid that there are some inode updates */
	if ((i && PageDirty(i)) || f2fs_need_inode_block_update(sbi, ino))
		ret = true;
	f2fs_put_page(i, 0);
	return ret;
}

static void try_to_fix_pino(struct inode *inode)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);
	nid_t pino;

	down_write(&fi->i_sem);
	if (file_wrong_pino(inode) && inode->i_nlink == 1 &&
			get_parent_ino(inode, &pino)) {
		f2fs_i_pino_write(inode, pino);
		file_got_pino(inode);
	}
	up_write(&fi->i_sem);
}

#ifdef CONFIG_MAS_ORDER_PRESERVE
void f2fs_flush_wait_fsync(struct file *file)
{
	struct inode *inode = file_inode(file);
	struct f2fs_inode_info *fi = F2FS_I(inode);

	if ((file->f_fsync_flag) && (fi->i_fsync_flag)) {
		cancel_delayed_work_sync(&fi->fsync_work);
		if (fi->i_fsync_flag) {
			spin_lock(&fi->vfs_inode.i_lock);
			fi->i_fsync_flag = F2FS_I_FSYNC_FLAG_WAITING;
			spin_unlock(&fi->vfs_inode.i_lock);

			filemap_fdatawait_range(fi->vfs_inode.i_mapping, 0, LLONG_MAX);
			spin_lock(&fi->vfs_inode.i_lock);
			fi->i_fsync_flag = 0;
			spin_unlock(&fi->vfs_inode.i_lock);
		}
	}

	if (file->f_fsync_flag) {
		spin_lock(&file->f_lock);
		file->f_fsync_flag = 0;
		spin_unlock(&file->f_lock);
	}
}

void f2fs_wait_writeback_work_fn(struct work_struct *work)
{
	struct f2fs_inode_info *fi = container_of(work,
				struct f2fs_inode_info, fsync_work.work);

	spin_lock(&fi->vfs_inode.i_lock);
	fi->i_fsync_flag = F2FS_I_FSYNC_FLAG_WAITING;
	spin_unlock(&fi->vfs_inode.i_lock);

	filemap_fdatawait_range(fi->vfs_inode.i_mapping, 0, LLONG_MAX);
	spin_lock(&fi->vfs_inode.i_lock);
	fi->i_fsync_flag = 0;
	spin_unlock(&fi->vfs_inode.i_lock);
	return;
}
#endif /* CONFIG_MAS_ORDER_PRESERVE */

static int f2fs_do_sync_file(struct file *file, loff_t start, loff_t end,
						int datasync, bool atomic)
{
	struct inode *inode = file->f_mapping->host;
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	nid_t ino = inode->i_ino;
	int ret = 0;
	enum cp_reason_type cp_reason = 0;
	struct writeback_control wbc = {
		.sync_mode = WB_SYNC_ALL,
		.nr_to_write = LONG_MAX,
		.for_reclaim = 0,
	};
	u64 fsync_begin = 0;
	u64 fsync_end = 0;
	u64 wr_file_end = 0;
	u64 cp_begin = 0;
	u64 cp_end = 0;
	u64 sync_node_begin = 0;
	u64 sync_node_end = 0;
	u64 flush_begin = 0;
	u64 flush_end = 0;
	unsigned int seq_id = 0;
#ifdef CONFIG_MAS_ORDER_PRESERVE
	struct f2fs_inode_info *fi = F2FS_I(inode);
	bool f2fs_ff_enable = blk_dev_write_order_preserved(sbi->sb->s_bdev) &&
				!blk_mq_get_io_in_list_count(sbi->sb->s_bdev) &&
				(fi->i_fsync_flag != F2FS_I_FSYNC_FLAG_WAITING);
	int i;
#endif

	if (unlikely(f2fs_readonly(inode->i_sb) ||
				is_sbi_flag_set(sbi, SBI_CP_DISABLED)))
		return 0;

	trace_f2fs_sync_file_enter(inode);
	if (S_ISDIR(inode->i_mode))
		goto go_write;

	fsync_begin = local_clock();
	/* if fdatasync is triggered, let's do in-place-update */
#ifdef CONFIG_F2FS_JOURNAL_APPEND
	if (datasync || ((unsigned int)get_dirty_pages(inode) <=
		SM_I(sbi)->min_fsync_blocks &&
		(!write_opt || i_size_read(inode) >
		 (loff_t)(ADDRS_PER_INODE(inode) * PAGE_SIZE))))
#else
	if (datasync || get_dirty_pages(inode) <= SM_I(sbi)->min_fsync_blocks)
#endif
		set_inode_flag(inode, FI_NEED_IPU);
#ifdef CONFIG_MAS_ORDER_PRESERVE
	if (f2fs_ff_enable)
		ret = __filemap_fdatawrite_range(file->f_mapping,
					start, end, WB_SYNC_ALL);
	else
		ret = file_write_and_wait_range(file, start, end);
#else
	ret = file_write_and_wait_range(file, start, end);
#endif
	wr_file_end = local_clock();
	clear_inode_flag(inode, FI_NEED_IPU);

	if (ret) {
		trace_f2fs_sync_file_exit(inode, cp_reason, datasync, ret);
		return ret;
	}

	/* if the inode is dirty, let's recover all the time */
	if (!f2fs_skip_inode_update(inode, datasync)) {
		f2fs_write_inode(inode, NULL);
		goto go_write;
	}

	/*
	 * if there is no written data, don't waste time to write recovery info.
	 */
	if (!is_inode_flag_set(inode, FI_APPEND_WRITE) &&
			!f2fs_exist_written_data(sbi, ino, APPEND_INO)) {

		/* it may call write_inode just prior to fsync */
		if (need_inode_page_update(sbi, ino))
			goto go_write;

		if (is_inode_flag_set(inode, FI_UPDATE_WRITE) ||
				f2fs_exist_written_data(sbi, ino, UPDATE_INO))
			goto flush_out;
		goto out;
	}
go_write:
	/*
	 * Both of fdatasync() and fsync() are able to be recovered from
	 * sudden-power-off.
	 */
	down_read(&F2FS_I(inode)->i_sem);
	cp_reason = need_do_checkpoint(inode);
	up_read(&F2FS_I(inode)->i_sem);

	if (cp_reason) {
		/* all the dirty node pages should be flushed for POR */
		cp_begin = local_clock();
		ret = f2fs_sync_fs(inode->i_sb, 1);
		cp_end = local_clock();

		/*
		 * We've secured consistency through sync_fs. Following pino
		 * will be used only for fsynced inodes after checkpoint.
		 */
		try_to_fix_pino(inode);
		clear_inode_flag(inode, FI_APPEND_WRITE);
		clear_inode_flag(inode, FI_UPDATE_WRITE);
		goto out;
	}
#ifdef CONFIG_IOCACHE
	if (iocache_is_open())
		iocache_commit_trans(false);
#endif
sync_nodes:
	sync_node_begin = local_clock();
	atomic_inc(&sbi->wb_sync_req[NODE]);
	ret = f2fs_fsync_node_pages(sbi, inode, &wbc, atomic, &seq_id);
	atomic_dec(&sbi->wb_sync_req[NODE]);
	if (ret)
		goto out;

	/* if cp_error was enabled, we should avoid infinite loop */
	if (unlikely(f2fs_cp_error(sbi))) {
		ret = -EIO;
		goto out;
	}

	if (f2fs_need_inode_block_update(sbi, ino)) {
		f2fs_mark_inode_dirty_sync(inode, true);
		f2fs_write_inode(inode, NULL);
		goto sync_nodes;
	}

	/*
	 * If it's atomic_write, it's just fine to keep write ordering. So
	 * here we don't need to wait for node write completion, since we use
	 * node chain which serializes node blocks. If one of node writes are
	 * reordered, we can see simply broken chain, resulting in stopping
	 * roll-forward recovery. It means we'll recover all or none node blocks
	 * given fsync mark.
	 */
#ifdef CONFIG_MAS_ORDER_PRESERVE
	if (!atomic && (!f2fs_ff_enable)) {
#else
	if (!atomic) {
#endif
		ret = f2fs_wait_on_node_pages_writeback(sbi, seq_id);
		if (ret)
			goto out;
	}

	/* once recovery info is written, don't need to tack this */
	f2fs_remove_ino_entry(sbi, ino, APPEND_INO);
	clear_inode_flag(inode, FI_APPEND_WRITE);
	sync_node_end = local_clock();
flush_out:
#ifdef CONFIG_MAS_ORDER_PRESERVE
	if (!atomic && F2FS_OPTION(sbi).fsync_mode != FSYNC_MODE_NOBARRIER &&
							(!f2fs_ff_enable)) {
#else
	if (!atomic && F2FS_OPTION(sbi).fsync_mode != FSYNC_MODE_NOBARRIER) {
#endif
		flush_begin = local_clock();
		ret = f2fs_issue_flush(sbi, inode->i_ino);
		flush_end = local_clock();
	}
	else {
		delayflush++;
		if ((current_flush_merge != 0) &&
			(delayflush >= current_flush_merge)) {
			flush_begin = local_clock();
			ret = f2fs_issue_flush(sbi, inode->i_ino);
			flush_end = local_clock();
			delayflush = 1;
		}
	}
	if (!ret) {
		f2fs_remove_ino_entry(sbi, ino, UPDATE_INO);
		clear_inode_flag(inode, FI_UPDATE_WRITE);
		f2fs_remove_ino_entry(sbi, ino, FLUSH_INO);
	}
	f2fs_update_time(sbi, REQ_TIME);
out:
#ifdef CONFIG_MAS_ORDER_PRESERVE
	if (f2fs_ff_enable) {
		if (fi->i_fsync_flag == F2FS_I_FSYNC_FLAG_WAITING) {
			filemap_fdatawait_range(fi->vfs_inode.i_mapping, start, end);
		} else {
			spin_lock(&file->f_lock);
			if (!file->f_fsync_flag)
				file->f_fsync_flag = 1;
			spin_unlock(&file->f_lock);

			spin_lock(&inode->i_lock);
			if (!fi->i_fsync_flag) {
				fi->i_fsync_flag = F2FS_I_FSYNC_FLAG_SCHED_WORK;
				for (i = 1; i <= NR_CPUS; i++) {
					if (cpu_online((raw_smp_processor_id() + i) % NR_CPUS))
						break;
				}
				schedule_delayed_work_on(
					(raw_smp_processor_id() + i) % NR_CPUS,
					&fi->fsync_work, msecs_to_jiffies(10));
			}
			spin_unlock(&inode->i_lock);
		}
	}
#endif /* CONFIG_MAS_ORDER_PRESERVE */
	trace_f2fs_sync_file_exit(inode, cp_reason, datasync, ret);
	if (fsync_begin && !ret) {
		fsync_end = local_clock();
		bd_mutex_lock(&sbi->bd_mutex);
		if (S_ISREG(inode->i_mode))
			inc_bd_val(sbi, fsync_reg_file_cnt, 1);
		else if (S_ISDIR(inode->i_mode))
			inc_bd_val(sbi, fsync_dir_cnt, 1);
		inc_bd_val(sbi, fsync_time, fsync_end - fsync_begin);
		max_bd_val(sbi, max_fsync_time, fsync_end - fsync_begin);
		inc_bd_val(sbi, fsync_wr_file_time, wr_file_end - fsync_begin);
		max_bd_val(sbi, max_fsync_wr_file_time, wr_file_end -
			fsync_begin);
		inc_bd_val(sbi, fsync_cp_time, cp_end - cp_begin);
		max_bd_val(sbi, max_fsync_cp_time, cp_end - cp_begin);
		if (sync_node_end) {
			inc_bd_val(sbi, fsync_sync_node_time,
				   sync_node_end - sync_node_begin);
			max_bd_val(sbi, max_fsync_sync_node_time,
				   sync_node_end - sync_node_begin);
		}
		inc_bd_val(sbi, fsync_flush_time, flush_end - flush_begin);
		max_bd_val(sbi, max_fsync_flush_time, flush_end - flush_begin);
		bd_mutex_unlock(&sbi->bd_mutex);
	}
	return ret;
}

int f2fs_sync_file(struct file *file, loff_t start, loff_t end, int datasync)
{
	int ret;
	struct inode *inode = file_inode(file);

	if (unlikely(f2fs_cp_error(F2FS_I_SB(file_inode(file)))))
		return -EIO;

	inode_lock(inode);
	ret = f2fs_do_sync_file(file, start, end, datasync, false);
	inode_unlock(inode);

	return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
static pgoff_t __get_first_dirty_index(struct address_space *mapping,
						pgoff_t pgofs, int whence)
{
	struct page *page;
	int nr_pages;

	if (whence != SEEK_DATA)
		return 0;

	/* find first dirty page index */
	nr_pages = find_get_pages_tag(mapping, &pgofs, PAGECACHE_TAG_DIRTY,
				      1, &page);
	if (!nr_pages)
		return ULONG_MAX;
	pgofs = page->index;
	put_page(page);
	return pgofs;
}
#endif

static bool __found_offset(struct address_space *mapping, block_t blkaddr,
			pgoff_t dirty, pgoff_t index, int whence)
{
	switch (whence) {
	case SEEK_DATA:
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0))
		if (__is_valid_data_blkaddr(F2FS_I_SB(mapping->host), blkaddr))
			return true;
		if (blkaddr == NEW_ADDR &&
		    xa_get_mark(&mapping->i_pages, index, PAGECACHE_TAG_DIRTY))
			return true;
#else
		if ((blkaddr == NEW_ADDR && dirty == index) ||
		    __is_valid_data_blkaddr(F2FS_I_SB(mapping->host), blkaddr))
			return true;
#endif
		break;
	case SEEK_HOLE:
		if (blkaddr == NULL_ADDR)
			return true;
		break;
	}
	return false;
}

static loff_t f2fs_seek_block(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file->f_mapping->host;
	loff_t maxbytes = inode->i_sb->s_maxbytes;
	struct dnode_of_data dn;
	pgoff_t pgofs, end_offset;
	pgoff_t dirty = 0;
	loff_t data_ofs = offset;
	loff_t isize;
	int err = 0;
#ifdef CONFIG_F2FS_FS_DEDUP
	struct inode *inner = NULL, *exter = NULL;
#endif

	inode_lock(inode);

	isize = i_size_read(inode);
	if (offset >= isize)
		goto fail;

	/* handle inline data case */
	if (f2fs_has_inline_data(inode)) {
		if (whence == SEEK_HOLE) {
			data_ofs = isize;
			goto found;
		} else if (whence == SEEK_DATA) {
			data_ofs = offset;
			goto found;
		}
	}

	pgofs = (pgoff_t)(offset >> PAGE_SHIFT);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	dirty = __get_first_dirty_index(inode->i_mapping, pgofs, whence);
#endif
#ifdef CONFIG_F2FS_FS_DEDUP
	inner = get_inner_inode(inode);
	if (inner) {
		exter = inode;
		inode = inner;
	}
#endif
	for (; data_ofs < isize; data_ofs = (loff_t)pgofs << PAGE_SHIFT) {
		set_new_dnode(&dn, inode, NULL, NULL, 0);
		err = f2fs_get_dnode_of_data(&dn, pgofs, LOOKUP_NODE);
		if (err && err != -ENOENT) {
			goto fail;
		} else if (err == -ENOENT) {
			/* direct node does not exists */
			if (whence == SEEK_DATA) {
				pgofs = f2fs_get_next_page_offset(&dn, pgofs);
				continue;
			} else {
				goto found;
			}
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);

		/* find data/hole in dnode block */
		for (; dn.ofs_in_node < end_offset;
				dn.ofs_in_node++, pgofs++,
				data_ofs = (loff_t)pgofs << PAGE_SHIFT) {
			block_t blkaddr;

			blkaddr = f2fs_data_blkaddr(&dn);

			if (__is_valid_data_blkaddr(F2FS_I_SB(inode), blkaddr) &&
			    !f2fs_is_valid_blkaddr(F2FS_I_SB(inode), blkaddr,
						   DATA_GENERIC_ENHANCE)) {
				f2fs_put_dnode(&dn);
				goto fail;
			}

			if (__found_offset(file->f_mapping, blkaddr, dirty,
							pgofs, whence)) {
				f2fs_put_dnode(&dn);
				goto found;
			}
		}
		f2fs_put_dnode(&dn);
	}

	if (whence == SEEK_DATA)
		goto fail;
found:
	if (whence == SEEK_HOLE && data_ofs > isize)
		data_ofs = isize;
#ifdef CONFIG_F2FS_FS_DEDUP
	if (inner) {
		inode = exter;
		put_inner_inode(inner);
	}
#endif
	inode_unlock(inode);
	return vfs_setpos(file, data_ofs, maxbytes);
fail:
#ifdef CONFIG_F2FS_FS_DEDUP
	if (inner) {
		inode = exter;
		put_inner_inode(inner);
	}
#endif
	inode_unlock(inode);
	return -ENXIO;
}

static loff_t f2fs_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file->f_mapping->host;
	loff_t maxbytes = inode->i_sb->s_maxbytes;

	if (f2fs_compressed_file(inode))
		maxbytes = max_file_blocks(inode) << F2FS_BLKSIZE_BITS;

	switch (whence) {
	case SEEK_SET:
	case SEEK_CUR:
	case SEEK_END:
		return generic_file_llseek_size(file, offset, whence,
						maxbytes, i_size_read(inode));
	case SEEK_DATA:
	case SEEK_HOLE:
		if (offset < 0)
			return -ENXIO;
		return f2fs_seek_block(file, offset, whence);
	}

	return -EINVAL;
}

static int f2fs_file_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct inode *inode = file_inode(file);

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (!f2fs_is_compress_backend_ready(inode))
		return -EOPNOTSUPP;

	file_accessed(file);
	vma->vm_ops = &f2fs_file_vm_ops;
	set_inode_flag(inode, FI_MMAP_FILE);
	return 0;
}

static int f2fs_file_open(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_F2FS_FS_DEDUP
	struct inode *inner = NULL;
#endif
	int err = fscrypt_file_open(inode, filp);

	if (err)
		return err;

	if (!f2fs_is_compress_backend_ready(inode))
		return -EOPNOTSUPP;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	err = fsverity_file_open(inode, filp);
	if (err)
		return err;
#endif

#ifdef CONFIG_MAS_ORDER_PRESERVE
	filp->f_fsync_flag = 0;
#endif

	filp->f_mode |= FMODE_NOWAIT;

#ifdef CONFIG_F2FS_FS_DEDUP
	err = dquot_file_open(inode, filp);
	if (err)
		return err;

	if (f2fs_is_outer_inode(inode)) {
		inner = get_inner_inode(inode);
		if (inner)
			err = f2fs_file_open(inner, filp);
		put_inner_inode(inner);
	}
	return err;
#else
	return dquot_file_open(inode, filp);
#endif
}

void f2fs_truncate_data_blocks_range(struct dnode_of_data *dn, int count)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dn->inode);
	struct f2fs_node *raw_node;
	int nr_free = 0, ofs = dn->ofs_in_node, len = count;
	__le32 *addr;
	int base = 0;
	bool compressed_cluster = false;
	int cluster_index = 0, valid_blocks = 0;
	int cluster_size = F2FS_I(dn->inode)->i_cluster_size;
	bool released = !atomic_read(&F2FS_I(dn->inode)->i_compr_blocks);

	if (IS_INODE(dn->node_page) && f2fs_has_extra_attr(dn->inode))
		base = get_extra_isize(dn->inode);

	raw_node = F2FS_NODE(dn->node_page);
	addr = blkaddr_in_node(raw_node) + base + ofs;

	/* Assumption: truncateion starts with cluster */
	for (; count > 0; count--, addr++, dn->ofs_in_node++, cluster_index++) {
		block_t blkaddr = le32_to_cpu(*addr);
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
		if (f2fs_is_compressed_inode(dn->inode) &&
#else
		if (f2fs_compressed_file(dn->inode) &&
#endif
					!(cluster_index & (cluster_size - 1))) {
			if (compressed_cluster)
				f2fs_i_compr_blocks_update(dn->inode,
							valid_blocks, false);
			compressed_cluster = (blkaddr == COMPRESS_ADDR);
			valid_blocks = 0;
		}
		if (blkaddr == NULL_ADDR)
			continue;

		dn->data_blkaddr = NULL_ADDR;
		f2fs_set_data_blkaddr(dn);

#ifdef CONFIG_F2FS_FS_DEDUP
		if (blkaddr == DEDUP_ADDR)
			continue;
#endif

		if (__is_valid_data_blkaddr(sbi, blkaddr)) {
			if (!f2fs_is_valid_blkaddr(sbi, blkaddr,
					DATA_GENERIC_ENHANCE))
				continue;
			if (compressed_cluster)
				valid_blocks++;
		}

		if (dn->ofs_in_node == 0 && IS_INODE(dn->node_page))
			clear_inode_flag(dn->inode, FI_FIRST_BLOCK_WRITTEN);

		f2fs_invalidate_blocks(sbi, blkaddr);
		if (!released || blkaddr != COMPRESS_ADDR)
			nr_free++;
	}
	if (compressed_cluster)
		f2fs_i_compr_blocks_update(dn->inode, valid_blocks, false);

	if (nr_free) {
		pgoff_t fofs;
		/*
		 * once we invalidate valid blkaddr in range [ofs, ofs + count],
		 * we will invalidate all blkaddr in the whole range.
		 */
		fofs = f2fs_start_bidx_of_node(ofs_of_node(dn->node_page),
							dn->inode) + ofs;
		f2fs_update_extent_cache_range(dn, fofs, 0, len);
		dec_valid_block_count(sbi, dn->inode, nr_free);
	}
	dn->ofs_in_node = ofs;

	f2fs_update_time(sbi, REQ_TIME);
	trace_f2fs_truncate_data_blocks_range(dn->inode, dn->nid,
					 dn->ofs_in_node, nr_free);
}

void f2fs_truncate_data_blocks(struct dnode_of_data *dn)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	f2fs_truncate_data_blocks_range(dn, ADDRS_PER_BLOCK(dn->inode));
#else
	f2fs_truncate_data_blocks_range(dn, ADDRS_PER_BLOCK);
#endif
}

static int truncate_partial_data_page(struct inode *inode, u64 from,
								bool cache_only)
{
	loff_t offset = from & (PAGE_SIZE - 1);
	pgoff_t index = from >> PAGE_SHIFT;
	struct address_space *mapping = inode->i_mapping;
	struct page *page;

	if (!offset && !cache_only)
		return 0;

	if (cache_only) {
		page = find_lock_page(mapping, index);
		if (page && PageUptodate(page))
			goto truncate_out;
		f2fs_put_page(page, 1);
		return 0;
	}

	page = f2fs_get_lock_data_page(inode, index, true);
	if (IS_ERR(page))
		return PTR_ERR(page) == -ENOENT ? 0 : PTR_ERR(page);
truncate_out:
	f2fs_wait_on_page_writeback(page, DATA, true, true);
	zero_user(page, offset, PAGE_SIZE - offset);

	/* An encrypted inode should have a key and truncate the last page. */
	f2fs_bug_on(F2FS_I_SB(inode), cache_only && IS_ENCRYPTED(inode));
	if (!cache_only)
		set_page_dirty(page);
	f2fs_put_page(page, 1);
	return 0;
}

int f2fs_do_truncate_blocks(struct inode *inode, u64 from, bool lock)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct dnode_of_data dn;
	pgoff_t free_from;
	int count = 0, err = 0;
	struct page *ipage;
	bool truncate_page = false;

	trace_f2fs_truncate_blocks_enter(inode, from);

	free_from = (pgoff_t)F2FS_BLK_ALIGN(from);

	if (free_from >= max_file_blocks(inode))
		goto free_partial;

	if (lock)
		f2fs_lock_op(sbi);

	ipage = f2fs_get_node_page(sbi, inode->i_ino);
	if (IS_ERR(ipage)) {
		err = PTR_ERR(ipage);
		goto out;
	}

	if (f2fs_has_inline_data(inode)) {
		f2fs_truncate_inline_inode(inode, ipage, from);
		f2fs_put_page(ipage, 1);
		truncate_page = true;
		goto out;
	}

	set_new_dnode(&dn, inode, ipage, NULL, 0);
	err = f2fs_get_dnode_of_data(&dn, free_from, LOOKUP_NODE_RA);
	if (err) {
		if (err == -ENOENT)
			goto free_next;
		goto out;
	}

	count = ADDRS_PER_PAGE(dn.node_page, inode);

	count -= dn.ofs_in_node;
	f2fs_bug_on(sbi, count < 0);

	if (dn.ofs_in_node || IS_INODE(dn.node_page)) {
		f2fs_truncate_data_blocks_range(&dn, count);
		free_from += count;
	}

	f2fs_put_dnode(&dn);
free_next:
	err = f2fs_truncate_inode_blocks(inode, free_from);
out:
	if (lock)
		f2fs_unlock_op(sbi);
free_partial:
	/* lastly zero out the first data page */
	if (!err)
		err = truncate_partial_data_page(inode, from, truncate_page);

	trace_f2fs_truncate_blocks_exit(inode, err);
	return err;
}

int f2fs_truncate_blocks(struct inode *inode, u64 from, bool lock)
{
	u64 free_from = from;
	int err;

#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	/*
	 * for compressed file, only support cluster size
	 * aligned truncation.
	 */
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_compressed_inode(inode))
#else
	if (f2fs_compressed_file(inode))
#endif
		free_from = round_up(from,
				F2FS_I(inode)->i_cluster_size << PAGE_SHIFT);
#endif

	err = f2fs_do_truncate_blocks(inode, free_from, lock);
	if (err)
		return err;

#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	if (from != free_from) {
		err = f2fs_truncate_partial_cluster(inode, from, lock);
		if (err)
			return err;
	}
#endif

	return err;
}

int f2fs_truncate(struct inode *inode)
{
	int err;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
				S_ISLNK(inode->i_mode)))
		return 0;

	trace_f2fs_truncate(inode);

	if (time_to_inject(F2FS_I_SB(inode), FAULT_TRUNCATE)) {
		f2fs_show_injection_info(F2FS_I_SB(inode), FAULT_TRUNCATE);
		return -EIO;
	}

	err = f2fs_dquot_initialize(inode);
	if (err)
		return err;

	/* we should check inline_data size */
	if (!f2fs_may_inline_data(inode)) {
		err = f2fs_convert_inline_inode(inode);
		if (err)
			return err;
	}

	err = f2fs_truncate_blocks(inode, i_size_read(inode), true);
	if (err)
		return err;

	inode->i_mtime = inode->i_ctime = current_time(inode);
	f2fs_mark_inode_dirty_sync(inode, false);
	return 0;
}

int f2fs_getattr(const struct path *path, struct kstat *stat,
		 u32 request_mask, unsigned int query_flags)
{
	struct inode *inode = d_inode(path->dentry);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_inode *ri;
	unsigned int flags;
#ifdef CONFIG_F2FS_FS_DEDUP
	struct inode *inner = NULL;
#endif

	if (f2fs_has_extra_attr(inode) &&
			f2fs_sb_has_inode_crtime(F2FS_I_SB(inode)) &&
			F2FS_FITS_IN_INODE(ri, fi->i_extra_isize, i_crtime)) {
		stat->result_mask |= STATX_BTIME;
		stat->btime.tv_sec = fi->i_crtime.tv_sec;
		stat->btime.tv_nsec = fi->i_crtime.tv_nsec;
	}

	flags = fi->i_flags;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	if (flags & F2FS_COMPR_FL)
		stat->attributes |= STATX_ATTR_COMPRESSED;
#endif
	if (flags & F2FS_APPEND_FL)
		stat->attributes |= STATX_ATTR_APPEND;
	if (IS_ENCRYPTED(inode))
		stat->attributes |= STATX_ATTR_ENCRYPTED;
	if (flags & F2FS_IMMUTABLE_FL)
		stat->attributes |= STATX_ATTR_IMMUTABLE;
	if (flags & F2FS_NODUMP_FL)
		stat->attributes |= STATX_ATTR_NODUMP;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	if (IS_VERITY(inode))
		stat->attributes |= STATX_ATTR_VERITY;
#endif

	stat->attributes_mask |= (STATX_ATTR_APPEND |
				  STATX_ATTR_ENCRYPTED |
				  STATX_ATTR_IMMUTABLE |
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
				  STATX_ATTR_COMPRESSED |
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
				  STATX_ATTR_VERITY |
#endif
				  STATX_ATTR_NODUMP);

	generic_fillattr(inode, stat);

#ifdef CONFIG_F2FS_FS_DEDUP
	inner = get_inner_inode(inode);
	if (inner) {
		down_read(&F2FS_I(inner)->i_sem);
		if (inner->i_nlink == 0)
			f2fs_bug_on(F2FS_I_SB(inode), 1);
		else
			stat->blocks = inner->i_blocks / inner->i_nlink;
		up_read(&F2FS_I(inner)->i_sem);
	}
	put_inner_inode(inner);
#endif

	/* we need to show initial sectors used for inline_data/dentries */
	if ((S_ISREG(inode->i_mode) && f2fs_has_inline_data(inode)) ||
					f2fs_has_inline_dentry(inode))
		stat->blocks += (stat->size + 511) >> 9;

	return 0;
}

#ifdef CONFIG_F2FS_FS_POSIX_ACL
static void __setattr_copy(struct inode *inode, const struct iattr *attr)
{
	unsigned int ia_valid = attr->ia_valid;

	if (ia_valid & ATTR_UID)
		inode->i_uid = attr->ia_uid;
	if (ia_valid & ATTR_GID)
		inode->i_gid = attr->ia_gid;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	if (ia_valid & ATTR_ATIME)
		inode->i_atime = timespec_trunc(attr->ia_atime,
						inode->i_sb->s_time_gran);
	if (ia_valid & ATTR_MTIME)
		inode->i_mtime = timespec_trunc(attr->ia_mtime,
						inode->i_sb->s_time_gran);
	if (ia_valid & ATTR_CTIME)
		inode->i_ctime = timespec_trunc(attr->ia_ctime,
						inode->i_sb->s_time_gran);
#else
	if (ia_valid & ATTR_ATIME)
		inode->i_atime = attr->ia_atime;
	if (ia_valid & ATTR_MTIME)
		inode->i_mtime = attr->ia_mtime;
	if (ia_valid & ATTR_CTIME)
		inode->i_ctime = attr->ia_ctime;
#endif
	if (ia_valid & ATTR_MODE) {
		umode_t mode = attr->ia_mode;

		if (!in_group_p(inode->i_gid) &&
			!capable_wrt_inode_uidgid(inode, CAP_FSETID))
			mode &= ~S_ISGID;
		set_acl_inode(inode, mode);
	}
}
#else
#define __setattr_copy setattr_copy
#endif

int f2fs_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = d_inode(dentry);
	int err;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;

	if (unlikely(IS_IMMUTABLE(inode)))
		return -EPERM;

	if (unlikely(IS_APPEND(inode) &&
			(attr->ia_valid & (ATTR_MODE | ATTR_UID |
				  ATTR_GID | ATTR_TIMES_SET))))
		return -EPERM;

	if ((attr->ia_valid & ATTR_SIZE) &&
		!f2fs_is_compress_backend_ready(inode))
		return -EOPNOTSUPP;

	err = setattr_prepare(dentry, attr);
	if (err)
		return err;

	err = fscrypt_prepare_setattr(dentry, attr);
	if (err)
		return err;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	err = fsverity_prepare_setattr(dentry, attr);
	if (err)
		return err;
#endif

	if (is_quota_modification(inode, attr)) {
		err = f2fs_dquot_initialize(inode);
		if (err)
			return err;
	}
	if ((attr->ia_valid & ATTR_UID &&
		!uid_eq(attr->ia_uid, inode->i_uid)) ||
		(attr->ia_valid & ATTR_GID &&
		!gid_eq(attr->ia_gid, inode->i_gid))) {
		f2fs_lock_op(F2FS_I_SB(inode));
		err = f2fs_dquot_transfer(inode, attr);
		if (err) {
			set_sbi_flag(F2FS_I_SB(inode),
					SBI_QUOTA_NEED_REPAIR);
			f2fs_unlock_op(F2FS_I_SB(inode));
			return err;
		}
		/*
		 * update uid/gid under lock_op(), so that dquot and inode can
		 * be updated atomically.
		 */
		if (attr->ia_valid & ATTR_UID) {
#ifdef CONFIG_HWDPS
			hwdps_update_context(inode, attr->ia_uid.val);
#endif
			inode->i_uid = attr->ia_uid;
		}
		if (attr->ia_valid & ATTR_GID)
			inode->i_gid = attr->ia_gid;
		f2fs_mark_inode_dirty_sync(inode, true);
		f2fs_unlock_op(F2FS_I_SB(inode));
	}

	if (attr->ia_valid & ATTR_SIZE) {
		loff_t old_size = i_size_read(inode);

		if (attr->ia_size > MAX_INLINE_DATA(inode)) {
			/*
			 * should convert inline inode before i_size_write to
			 * keep smaller than inline_data size with inline flag.
			 */
			err = f2fs_convert_inline_inode(inode);
			if (err)
				return err;
		}

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
		/*
		 * caller have hold inode lock
		 */
		if (attr->ia_size <= old_size &&
			is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
			err = f2fs_reserve_compress_blocks_internal(inode, SETATTR);
			if (err)
				return err;
		}
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
		/*
		 * caller have hold inode lock
		 */
		if (attr->ia_size <= old_size && f2fs_is_outer_inode(inode)) {
			mark_file_modified(inode);
			err = f2fs_revoke_deduped_inode(inode, __func__);
			if (err)
				return err;
		}
#endif

		down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
		down_write(&F2FS_I(inode)->i_mmap_sem);

		truncate_setsize(inode, attr->ia_size);

		if (attr->ia_size <= old_size)
			err = f2fs_truncate(inode);
		/*
		 * do not trim all blocks after i_size if target size is
		 * larger than i_size.
		 */
		up_write(&F2FS_I(inode)->i_mmap_sem);
		up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
		if (err)
			return err;

		spin_lock(&F2FS_I(inode)->i_size_lock);
		inode->i_mtime = inode->i_ctime = current_time(inode);
		F2FS_I(inode)->last_disk_size = i_size_read(inode);
		spin_unlock(&F2FS_I(inode)->i_size_lock);
	}

	__setattr_copy(inode, attr);

	if (attr->ia_valid & ATTR_MODE) {
		err = posix_acl_chmod(inode, f2fs_get_inode_mode(inode));

		if (is_inode_flag_set(inode, FI_ACL_MODE)) {
			if (!err)
				inode->i_mode = F2FS_I(inode)->i_acl_mode;
			clear_inode_flag(inode, FI_ACL_MODE);
		}
	}

	/* file size may changed here */
	f2fs_mark_inode_dirty_sync(inode, true);

	/* inode change will produce dirty node pages flushed by checkpoint */
	f2fs_balance_fs(F2FS_I_SB(inode), true);

	return err;
}

const struct inode_operations f2fs_file_inode_operations = {
	.getattr	= f2fs_getattr,
	.setattr	= f2fs_setattr,
	.get_acl	= f2fs_get_acl,
	.set_acl	= f2fs_set_acl,
	.listxattr	= f2fs_listxattr,
	.fiemap		= f2fs_fiemap,
};

static int fill_zero(struct inode *inode, pgoff_t index,
					loff_t start, loff_t len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct page *page;

	if (!len)
		return 0;

	f2fs_balance_fs(sbi, true);

	f2fs_lock_op(sbi);
	page = f2fs_get_new_data_page(inode, NULL, index, false);
	f2fs_unlock_op(sbi);

	if (IS_ERR(page))
		return PTR_ERR(page);

	f2fs_wait_on_page_writeback(page, DATA, true, true);
	zero_user(page, start, len);
	set_page_dirty(page);
	f2fs_put_page(page, 1);
	return 0;
}

int f2fs_truncate_hole(struct inode *inode, pgoff_t pg_start, pgoff_t pg_end)
{
	int err;

	while (pg_start < pg_end) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		err = f2fs_get_dnode_of_data(&dn, pg_start, LOOKUP_NODE);
		if (err) {
			if (err == -ENOENT) {
				pg_start = f2fs_get_next_page_offset(&dn,
								pg_start);
				continue;
			}
			return err;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, pg_end - pg_start);

		f2fs_bug_on(F2FS_I_SB(inode), count == 0 || count > end_offset);

		f2fs_truncate_data_blocks_range(&dn, count);
		f2fs_put_dnode(&dn);

		pg_start += count;
	}
	return 0;
}

static int punch_hole(struct inode *inode, loff_t offset, loff_t len)
{
	pgoff_t pg_start, pg_end;
	loff_t off_start, off_end;
	int ret;

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		return ret;

	pg_start = ((unsigned long long) offset) >> PAGE_SHIFT;
	pg_end = ((unsigned long long) offset + len) >> PAGE_SHIFT;

	off_start = offset & (PAGE_SIZE - 1);
	off_end = (offset + len) & (PAGE_SIZE - 1);

	if (pg_start == pg_end) {
		ret = fill_zero(inode, pg_start, off_start,
						off_end - off_start);
		if (ret)
			return ret;
	} else {
		if (off_start) {
			ret = fill_zero(inode, pg_start++, off_start,
						PAGE_SIZE - off_start);
			if (ret)
				return ret;
		}
		if (off_end) {
			ret = fill_zero(inode, pg_end, 0, off_end);
			if (ret)
				return ret;
		}

		if (pg_start < pg_end) {
			struct address_space *mapping = inode->i_mapping;
			loff_t blk_start, blk_end;
			struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

			f2fs_balance_fs(sbi, true);

			blk_start = (loff_t)pg_start << PAGE_SHIFT;
			blk_end = (loff_t)pg_end << PAGE_SHIFT;

			down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
			down_write(&F2FS_I(inode)->i_mmap_sem);

			truncate_inode_pages_range(mapping, blk_start,
					blk_end - 1);

			f2fs_lock_op(sbi);
			ret = f2fs_truncate_hole(inode, pg_start, pg_end);
			f2fs_unlock_op(sbi);

			up_write(&F2FS_I(inode)->i_mmap_sem);
			up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
		}
	}

	return ret;
}

static int __read_out_blkaddrs(struct inode *inode, block_t *blkaddr,
				int *do_replace, pgoff_t off, pgoff_t len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct dnode_of_data dn;
	int ret, done, i;

next_dnode:
	set_new_dnode(&dn, inode, NULL, NULL, 0);
	ret = f2fs_get_dnode_of_data(&dn, off, LOOKUP_NODE_RA);
	if (ret && ret != -ENOENT) {
		return ret;
	} else if (ret == -ENOENT) {
		if (dn.max_level == 0)
			return -ENOENT;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		done = min((pgoff_t)ADDRS_PER_BLOCK(inode) - dn.ofs_in_node,
#else
		done = min((pgoff_t)ADDRS_PER_BLOCK - dn.ofs_in_node,
#endif
									len);
		blkaddr += done;
		do_replace += done;
		goto next;
	}

	done = min((pgoff_t)ADDRS_PER_PAGE(dn.node_page, inode) -
							dn.ofs_in_node, len);
	for (i = 0; i < done; i++, blkaddr++, do_replace++, dn.ofs_in_node++) {
		*blkaddr = f2fs_data_blkaddr(&dn);

		if (__is_valid_data_blkaddr(sbi, *blkaddr) &&
			!f2fs_is_valid_blkaddr(sbi, *blkaddr,
					DATA_GENERIC_ENHANCE)) {
			f2fs_put_dnode(&dn);
			return -EFSCORRUPTED;
		}

		if (!f2fs_is_checkpointed_data(sbi, *blkaddr)) {

			if (f2fs_lfs_mode(sbi)) {
				f2fs_put_dnode(&dn);
				return -EOPNOTSUPP;
			}

			/* do not invalidate this block address */
			f2fs_update_data_blkaddr(&dn, NULL_ADDR);
			*do_replace = 1;
		}
	}
	f2fs_put_dnode(&dn);
next:
	len -= done;
	off += done;
	if (len)
		goto next_dnode;
	return 0;
}

static int __roll_back_blkaddrs(struct inode *inode, block_t *blkaddr,
				int *do_replace, pgoff_t off, int len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct dnode_of_data dn;
	int ret, i;

	for (i = 0; i < len; i++, do_replace++, blkaddr++) {
		if (*do_replace == 0)
			continue;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, off + i, LOOKUP_NODE_RA);
		if (ret) {
			dec_valid_block_count(sbi, inode, 1);
			f2fs_invalidate_blocks(sbi, *blkaddr);
		} else {
			f2fs_update_data_blkaddr(&dn, *blkaddr);
		}
		f2fs_put_dnode(&dn);
	}
	return 0;
}

static int __clone_blkaddrs(struct inode *src_inode, struct inode *dst_inode,
			block_t *blkaddr, int *do_replace,
			pgoff_t src, pgoff_t dst, pgoff_t len, bool full)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(src_inode);
	pgoff_t i = 0;
	int ret;

	while (i < len) {
		if (blkaddr[i] == NULL_ADDR && !full) {
			i++;
			continue;
		}

		if (do_replace[i] || blkaddr[i] == NULL_ADDR) {
			struct dnode_of_data dn;
			struct node_info ni;
			size_t new_size;
			pgoff_t ilen;

			set_new_dnode(&dn, dst_inode, NULL, NULL, 0);
			ret = f2fs_get_dnode_of_data(&dn, dst + i, ALLOC_NODE);
			if (ret)
				return ret;

			ret = f2fs_get_node_info(sbi, dn.nid, &ni);
			if (ret) {
				f2fs_put_dnode(&dn);
				return ret;
			}

			ilen = min((pgoff_t)
				ADDRS_PER_PAGE(dn.node_page, dst_inode) -
						dn.ofs_in_node, len - i);
			do {
				dn.data_blkaddr = f2fs_data_blkaddr(&dn);
				f2fs_truncate_data_blocks_range(&dn, 1);

				if (do_replace[i]) {
					f2fs_i_blocks_write(src_inode,
							1, false, false);
					f2fs_i_blocks_write(dst_inode,
							1, true, false);
					f2fs_replace_block(sbi, &dn, dn.data_blkaddr,
					blkaddr[i], ni.version, true, false);

					do_replace[i] = 0;
				}
				dn.ofs_in_node++;
				i++;
				new_size = (loff_t)(dst + i) << PAGE_SHIFT;
				if (dst_inode->i_size < new_size)
					f2fs_i_size_write(dst_inode, new_size);
			} while (--ilen && (do_replace[i] || blkaddr[i] == NULL_ADDR));

			f2fs_put_dnode(&dn);
		} else {
			struct page *psrc, *pdst;

			psrc = f2fs_get_lock_data_page(src_inode,
							src + i, true);
			if (IS_ERR(psrc))
				return PTR_ERR(psrc);
			pdst = f2fs_get_new_data_page(dst_inode, NULL, dst + i,
								true);
			if (IS_ERR(pdst)) {
				f2fs_put_page(psrc, 1);
				return PTR_ERR(pdst);
			}
			f2fs_copy_page(psrc, pdst);
			set_page_dirty(pdst);
			f2fs_put_page(pdst, 1);
			f2fs_put_page(psrc, 1);

			ret = f2fs_truncate_hole(src_inode,
						src + i, src + i + 1);
			if (ret)
				return ret;
			i++;
		}
	}
	return 0;
}

static int __exchange_data_block(struct inode *src_inode,
			struct inode *dst_inode, pgoff_t src, pgoff_t dst,
			pgoff_t len, bool full)
{
	block_t *src_blkaddr;
	int *do_replace;
	pgoff_t olen;
	int ret;

	while (len) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		olen = min((pgoff_t)4 * ADDRS_PER_BLOCK(src_inode), len);
#else
		olen = min((pgoff_t)4 * ADDRS_PER_BLOCK, len);
#endif

		src_blkaddr = f2fs_kvzalloc(F2FS_I_SB(src_inode),
					array_size(olen, sizeof(block_t)),
					GFP_NOFS);
		if (!src_blkaddr)
			return -ENOMEM;

		do_replace = f2fs_kvzalloc(F2FS_I_SB(src_inode),
					array_size(olen, sizeof(int)),
					GFP_NOFS);
		if (!do_replace) {
			kvfree(src_blkaddr);
			return -ENOMEM;
		}

		ret = __read_out_blkaddrs(src_inode, src_blkaddr,
					do_replace, src, olen);
		if (ret)
			goto roll_back;

		ret = __clone_blkaddrs(src_inode, dst_inode, src_blkaddr,
					do_replace, src, dst, olen, full);
		if (ret)
			goto roll_back;

		src += olen;
		dst += olen;
		len -= olen;

		kvfree(src_blkaddr);
		kvfree(do_replace);
	}
	return 0;

roll_back:
	__roll_back_blkaddrs(src_inode, src_blkaddr, do_replace, src, olen);
	kvfree(src_blkaddr);
	kvfree(do_replace);
	return ret;
}

static int f2fs_do_collapse(struct inode *inode, loff_t offset, loff_t len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t nrpages = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);
	pgoff_t start = offset >> PAGE_SHIFT;
	pgoff_t end = (offset + len) >> PAGE_SHIFT;
	int ret;

	f2fs_balance_fs(sbi, true);

	/* avoid gc operation during block exchange */
	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);

	f2fs_lock_op(sbi);
	f2fs_drop_extent_tree(inode);
	truncate_pagecache(inode, offset);
	ret = __exchange_data_block(inode, inode, end, start, nrpages - end, true);
	f2fs_unlock_op(sbi);

	up_write(&F2FS_I(inode)->i_mmap_sem);
	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	return ret;
}

static int f2fs_collapse_range(struct inode *inode, loff_t offset, loff_t len)
{
	loff_t new_size;
	int ret;

	if (offset + len >= i_size_read(inode))
		return -EINVAL;

	/* collapse range should be aligned to block size of f2fs. */
	if (offset & (F2FS_BLKSIZE - 1) || len & (F2FS_BLKSIZE - 1))
		return -EINVAL;

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		return ret;

	/* write out all dirty pages from offset */
	ret = filemap_write_and_wait_range(inode->i_mapping, offset, LLONG_MAX);
	if (ret)
		return ret;

	ret = f2fs_do_collapse(inode, offset, len);
	if (ret)
		return ret;

	/* write out all moved pages, if possible */
	down_write(&F2FS_I(inode)->i_mmap_sem);
	filemap_write_and_wait_range(inode->i_mapping, offset, LLONG_MAX);
	truncate_pagecache(inode, offset);

	new_size = i_size_read(inode) - len;
	ret = f2fs_truncate_blocks(inode, new_size, true);
	up_write(&F2FS_I(inode)->i_mmap_sem);
	if (!ret)
		f2fs_i_size_write(inode, new_size);
	return ret;
}

static int f2fs_do_zero_range(struct dnode_of_data *dn, pgoff_t start,
								pgoff_t end)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dn->inode);
	pgoff_t index = start;
	unsigned int ofs_in_node = dn->ofs_in_node;
	blkcnt_t count = 0;
	int ret;

	for (; index < end; index++, dn->ofs_in_node++) {
		if (f2fs_data_blkaddr(dn) == NULL_ADDR)
			count++;
	}

	dn->ofs_in_node = ofs_in_node;
	ret = f2fs_reserve_new_blocks(dn, count);
	if (ret)
		return ret;

	dn->ofs_in_node = ofs_in_node;
	for (index = start; index < end; index++, dn->ofs_in_node++) {
		dn->data_blkaddr = f2fs_data_blkaddr(dn);
		/*
		 * f2fs_reserve_new_blocks will not guarantee entire block
		 * allocation.
		 */
		if (dn->data_blkaddr == NULL_ADDR) {
			ret = -ENOSPC;
			break;
		}
		if (dn->data_blkaddr != NEW_ADDR) {
			f2fs_invalidate_blocks(sbi, dn->data_blkaddr);
			dn->data_blkaddr = NEW_ADDR;
			f2fs_set_data_blkaddr(dn);
		}
	}

	f2fs_update_extent_cache_range(dn, start, 0, index - start);

	return ret;
}

static int f2fs_zero_range(struct inode *inode, loff_t offset, loff_t len,
								int mode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct address_space *mapping = inode->i_mapping;
	pgoff_t index, pg_start, pg_end;
	loff_t new_size = i_size_read(inode);
	loff_t off_start, off_end;
	int ret = 0;

	ret = inode_newsize_ok(inode, (len + offset));
	if (ret)
		return ret;

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		return ret;

	ret = filemap_write_and_wait_range(mapping, offset, offset + len - 1);
	if (ret)
		return ret;

	pg_start = ((unsigned long long) offset) >> PAGE_SHIFT;
	pg_end = ((unsigned long long) offset + len) >> PAGE_SHIFT;

	off_start = offset & (PAGE_SIZE - 1);
	off_end = (offset + len) & (PAGE_SIZE - 1);

	if (pg_start == pg_end) {
		ret = fill_zero(inode, pg_start, off_start,
						off_end - off_start);
		if (ret)
			return ret;

		new_size = max_t(loff_t, new_size, offset + len);
	} else {
		if (off_start) {
			ret = fill_zero(inode, pg_start++, off_start,
						PAGE_SIZE - off_start);
			if (ret)
				return ret;

			new_size = max_t(loff_t, new_size,
					(loff_t)pg_start << PAGE_SHIFT);
		}

		for (index = pg_start; index < pg_end;) {
			struct dnode_of_data dn;
			unsigned int end_offset;
			pgoff_t end;

			down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
			down_write(&F2FS_I(inode)->i_mmap_sem);

			truncate_pagecache_range(inode,
				(loff_t)index << PAGE_SHIFT,
				((loff_t)pg_end << PAGE_SHIFT) - 1);

			f2fs_lock_op(sbi);

			set_new_dnode(&dn, inode, NULL, NULL, 0);
			ret = f2fs_get_dnode_of_data(&dn, index, ALLOC_NODE);
			if (ret) {
				f2fs_unlock_op(sbi);
				up_write(&F2FS_I(inode)->i_mmap_sem);
				up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
				goto out;
			}

			end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
			end = min(pg_end, end_offset - dn.ofs_in_node + index);

			ret = f2fs_do_zero_range(&dn, index, end);
			f2fs_put_dnode(&dn);

			f2fs_unlock_op(sbi);
			up_write(&F2FS_I(inode)->i_mmap_sem);
			up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);

			f2fs_balance_fs(sbi, dn.node_changed);

			if (ret)
				goto out;

			index = end;
			new_size = max_t(loff_t, new_size,
					(loff_t)index << PAGE_SHIFT);
		}

		if (off_end) {
			ret = fill_zero(inode, pg_end, 0, off_end);
			if (ret)
				goto out;

			new_size = max_t(loff_t, new_size, offset + len);
		}
	}

out:
	if (new_size > i_size_read(inode)) {
		if (mode & FALLOC_FL_KEEP_SIZE)
			file_set_keep_isize(inode);
		else
			f2fs_i_size_write(inode, new_size);
	}
	return ret;
}

static int f2fs_insert_range(struct inode *inode, loff_t offset, loff_t len)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t nr, pg_start, pg_end, delta, idx;
	loff_t new_size;
	int ret = 0;

	new_size = i_size_read(inode) + len;
	ret = inode_newsize_ok(inode, new_size);
	if (ret)
		return ret;

	if (offset >= i_size_read(inode))
		return -EINVAL;

	/* insert range should be aligned to block size of f2fs. */
	if (offset & (F2FS_BLKSIZE - 1) || len & (F2FS_BLKSIZE - 1))
		return -EINVAL;

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		return ret;

	f2fs_balance_fs(sbi, true);

	down_write(&F2FS_I(inode)->i_mmap_sem);
	ret = f2fs_truncate_blocks(inode, i_size_read(inode), true);
	up_write(&F2FS_I(inode)->i_mmap_sem);
	if (ret)
		return ret;

	/* write out all dirty pages from offset */
	ret = filemap_write_and_wait_range(inode->i_mapping, offset, LLONG_MAX);
	if (ret)
		return ret;

	pg_start = offset >> PAGE_SHIFT;
	pg_end = (offset + len) >> PAGE_SHIFT;
	delta = pg_end - pg_start;
	idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	/* avoid gc operation during block exchange */
	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);
	truncate_pagecache(inode, offset);

	while (!ret && idx > pg_start) {
		nr = idx - pg_start;
		if (nr > delta)
			nr = delta;
		idx -= nr;

		f2fs_lock_op(sbi);
		f2fs_drop_extent_tree(inode);

		ret = __exchange_data_block(inode, inode, idx,
					idx + delta, nr, false);
		f2fs_unlock_op(sbi);
	}
	up_write(&F2FS_I(inode)->i_mmap_sem);
	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);

	/* write out all moved pages, if possible */
	down_write(&F2FS_I(inode)->i_mmap_sem);
	filemap_write_and_wait_range(inode->i_mapping, offset, LLONG_MAX);
	truncate_pagecache(inode, offset);
	up_write(&F2FS_I(inode)->i_mmap_sem);

	if (!ret)
		f2fs_i_size_write(inode, new_size);
	return ret;
}

static int expand_inode_data(struct inode *inode, loff_t offset,
					loff_t len, int mode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_map_blocks map = { .m_next_pgofs = NULL,
			.m_next_extent = NULL, .m_seg_type = NO_CHECK_TYPE,
			.m_may_create = true };
	pgoff_t pg_start, pg_end;
	loff_t new_size = i_size_read(inode);
	loff_t off_end;
	block_t expanded = 0;
	int err;

	err = inode_newsize_ok(inode, (len + offset));
	if (err)
		return err;

	err = f2fs_convert_inline_inode(inode);
	if (err)
		return err;

	f2fs_balance_fs(sbi, true);

	pg_start = ((unsigned long long)offset) >> PAGE_SHIFT;
	pg_end = ((unsigned long long)offset + len) >> PAGE_SHIFT;
	off_end = (offset + len) & (PAGE_SIZE - 1);

	map.m_lblk = pg_start;
	map.m_len = pg_end - pg_start;
	if (off_end)
		map.m_len++;

	if (!map.m_len)
		return 0;

	if (f2fs_is_pinned_file(inode)) {
		block_t sec_blks = BLKS_PER_SEC(sbi);
		block_t sec_len = roundup(map.m_len, sec_blks);

		if (map.m_len % sbi->blocks_per_seg)
			len += sbi->blocks_per_seg;

		map.m_len = sec_blks;
next_alloc:
		if (has_not_enough_free_secs(sbi, 0,
			GET_SEC_FROM_SEG(sbi, overprovision_segments(sbi)))) {
			down_write(&sbi->gc_lock);
#ifdef CONFIG_F2FS_TURBO_ZONE
			err = f2fs_gc(sbi, true, false, false, false,
				      NULL_SEGNO);
#else
			err = f2fs_gc(sbi, true, false, false, NULL_SEGNO);
#endif
			if (err && err != -ENODATA && err != -EAGAIN)
				goto out_err;
		}

		down_write(&sbi->pin_sem);

		f2fs_lock_op(sbi);
		f2fs_allocate_new_section(sbi, CURSEG_COLD_DATA_PINNED, false);
		f2fs_unlock_op(sbi);

		map.m_seg_type = CURSEG_COLD_DATA_PINNED;
		err = f2fs_map_blocks(inode, &map, 1, F2FS_GET_BLOCK_PRE_DIO);

		up_write(&sbi->pin_sem);

		expanded += map.m_len;
		sec_len -= map.m_len;
		map.m_lblk += map.m_len;
		if (!err && sec_len)
			goto next_alloc;

		map.m_len = expanded;
	} else {
		err = f2fs_map_blocks(inode, &map, 1, F2FS_GET_BLOCK_PRE_AIO);
		expanded = map.m_len;
	}
out_err:
	if (err) {
		pgoff_t last_off;

		if (!expanded)
			return err;

		last_off = pg_start + expanded - 1;

		/* update new size to the failed position */
		new_size = (last_off == pg_end) ? offset + len :
					(loff_t)(last_off + 1) << PAGE_SHIFT;
	} else {
		new_size = ((loff_t)pg_end << PAGE_SHIFT) + off_end;
	}

	if (new_size > i_size_read(inode)) {
		if (mode & FALLOC_FL_KEEP_SIZE)
			file_set_keep_isize(inode);
		else
			f2fs_i_size_write(inode, new_size);
	}

	return err;
}

static long f2fs_fallocate(struct file *file, int mode,
				loff_t offset, loff_t len)
{
	struct inode *inode = file_inode(file);
	long ret = 0;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode))))
		return -EIO;
	if (!f2fs_is_checkpoint_ready(F2FS_I_SB(inode)))
		return -ENOSPC;
	if (!f2fs_is_compress_backend_ready(inode))
		return -EOPNOTSUPP;

	/* f2fs only support ->fallocate for regular file */
	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (IS_ENCRYPTED(inode) &&
		(mode & (FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_INSERT_RANGE)))
		return -EOPNOTSUPP;

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	file_start_write(file);
#endif

	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_compressed_inode(inode) &&
		(mode & (FALLOC_FL_PUNCH_HOLE | FALLOC_FL_COLLAPSE_RANGE |
			FALLOC_FL_ZERO_RANGE | FALLOC_FL_INSERT_RANGE))) {
		ret = f2fs_decompress_file_internal(file, FALLOCATE, mode);
		if (ret)
			goto out;
	}
#else
	if (f2fs_compressed_file(inode) &&
		(mode & (FALLOC_FL_PUNCH_HOLE | FALLOC_FL_COLLAPSE_RANGE |
			FALLOC_FL_ZERO_RANGE | FALLOC_FL_INSERT_RANGE))) {
		ret = -EOPNOTSUPP;
		goto out;
	}
#endif

	if (mode & ~(FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE |
			FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_ZERO_RANGE |
			FALLOC_FL_INSERT_RANGE)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode) &&
			f2fs_revoke_deduped_inode(inode, __func__)) {
		ret = -EIO;
		goto out;
	}
#endif

	if (mode & FALLOC_FL_PUNCH_HOLE) {
		if (offset >= inode->i_size)
			goto out;

		ret = punch_hole(inode, offset, len);
	} else if (mode & FALLOC_FL_COLLAPSE_RANGE) {
		ret = f2fs_collapse_range(inode, offset, len);
	} else if (mode & FALLOC_FL_ZERO_RANGE) {
		ret = f2fs_zero_range(inode, offset, len, mode);
	} else if (mode & FALLOC_FL_INSERT_RANGE) {
		ret = f2fs_insert_range(inode, offset, len);
	} else {
		ret = expand_inode_data(inode, offset, len, mode);
	}

	if (!ret) {
		inode->i_mtime = inode->i_ctime = current_time(inode);
		f2fs_mark_inode_dirty_sync(inode, false);
		f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);
	}

out:
	inode_unlock(inode);
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	file_end_write(file);
#endif

	trace_f2fs_fallocate(inode, mode, offset, len, ret);
	return ret;
}

static int f2fs_release_file(struct inode *inode, struct file *filp)
{
	/*
	 * f2fs_relase_file is called at every close calls. So we should
	 * not drop any inmemory pages by close called by other process.
	 */
	if (!(filp->f_mode & FMODE_WRITE) ||
			atomic_read(&inode->i_writecount) != 1)
		return 0;

	/* some remained atomic pages should discarded */
	if (f2fs_is_atomic_file(inode))
		f2fs_drop_inmem_pages(inode);
	if (f2fs_is_volatile_file(inode)) {
		set_inode_flag(inode, FI_DROP_CACHE);
		filemap_fdatawrite(inode->i_mapping);
		clear_inode_flag(inode, FI_DROP_CACHE);
		clear_inode_flag(inode, FI_VOLATILE_FILE);
		stat_dec_volatile_write(inode);
	}
	return 0;
}

static int f2fs_file_flush(struct file *file, fl_owner_t id)
{
	struct inode *inode = file_inode(file);

#ifdef CONFIG_MAS_ORDER_PRESERVE
	f2fs_flush_wait_fsync(file);
#endif

	/*
	 * If the process doing a transaction is crashed, we should do
	 * roll-back. Otherwise, other reader/write can see corrupted database
	 * until all the writers close its file. Since this should be done
	 * before dropping file lock, it needs to do in ->flush.
	 */
	if (f2fs_is_atomic_file(inode) &&
			F2FS_I(inode)->inmem_task == current)
		f2fs_drop_inmem_pages(inode);
	return 0;
}

static int f2fs_setflags_common(struct inode *inode, u32 iflags, u32 mask)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);
	u32 masked_flags = fi->i_flags & mask;

	/* mask can be shrunk by flags_valid selector */
	iflags &= mask;
	/* Is it quota file? Do not allow user to mess with it */
	if (IS_NOQUOTA(inode))
		return -EPERM;
	if ((iflags ^ masked_flags) & F2FS_CASEFOLD_FL) {
		if (!f2fs_sb_has_casefold(F2FS_I_SB(inode)))
			return -EOPNOTSUPP;
		if (!f2fs_empty_dir(inode))
			return -ENOTEMPTY;
	}
	if (iflags & (F2FS_COMPR_FL | F2FS_NOCOMP_FL)) {
		if (!f2fs_sb_has_compression(F2FS_I_SB(inode)))
			return -EOPNOTSUPP;
		if ((iflags & F2FS_COMPR_FL) && (iflags & F2FS_NOCOMP_FL))
			return -EINVAL;
	}
	if ((iflags ^ masked_flags) & F2FS_COMPR_FL) {
		if (masked_flags & F2FS_COMPR_FL) {
			if (!f2fs_disable_compressed_file(inode))
				return -EINVAL;
		}
		if (iflags & F2FS_NOCOMP_FL)
			return -EINVAL;
		if (iflags & F2FS_COMPR_FL) {
			if (!f2fs_may_compress(inode))
				return -EINVAL;
			if (S_ISREG(inode->i_mode) && inode->i_size)
				return -EINVAL;
			set_compress_context(inode);
		}
	}
	if ((iflags ^ masked_flags) & F2FS_NOCOMP_FL) {
		if (masked_flags & F2FS_COMPR_FL)
			return -EINVAL;
	}
	fi->i_flags = iflags | (fi->i_flags & ~mask);
	f2fs_bug_on(F2FS_I_SB(inode), (fi->i_flags & F2FS_COMPR_FL) &&
					(fi->i_flags & F2FS_NOCOMP_FL));
	if (fi->i_flags & F2FS_PROJINHERIT_FL)
		set_inode_flag(inode, FI_PROJ_INHERIT);
	else
		clear_inode_flag(inode, FI_PROJ_INHERIT);
	inode->i_ctime = current_time(inode);
	f2fs_set_inode_flags(inode);
	f2fs_mark_inode_dirty_sync(inode, true);
	return 0;
}

/* FS_IOC_GETFLAGS and FS_IOC_SETFLAGS support */

/*
 * To make a new on-disk f2fs i_flag gettable via FS_IOC_GETFLAGS, add an entry
 * for it to f2fs_fsflags_map[], and add its FS_*_FL equivalent to
 * F2FS_GETTABLE_FS_FL.  To also make it settable via FS_IOC_SETFLAGS, also add
 * its FS_*_FL equivalent to F2FS_SETTABLE_FS_FL.
 */

static const struct {
	u32 iflag;
	u32 fsflag;
} f2fs_fsflags_map[] = {
	{ F2FS_COMPR_FL,	FS_COMPR_FL },
	{ F2FS_SYNC_FL,		FS_SYNC_FL },
	{ F2FS_IMMUTABLE_FL,	FS_IMMUTABLE_FL },
	{ F2FS_APPEND_FL,	FS_APPEND_FL },
	{ F2FS_NODUMP_FL,	FS_NODUMP_FL },
	{ F2FS_NOATIME_FL,	FS_NOATIME_FL },
	{ F2FS_NOCOMP_FL,	FS_NOCOMP_FL },
	{ F2FS_INDEX_FL,	FS_INDEX_FL },
	{ F2FS_DIRSYNC_FL,	FS_DIRSYNC_FL },
	{ F2FS_PROJINHERIT_FL,	FS_PROJINHERIT_FL },
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	{ F2FS_CASEFOLD_FL,	FS_CASEFOLD_FL },
#endif
};

#define F2FS_GETTABLE_FS_FL (		\
		FS_COMPR_FL |		\
		FS_SYNC_FL |		\
		FS_IMMUTABLE_FL |	\
		FS_APPEND_FL |		\
		FS_NODUMP_FL |		\
		FS_NOATIME_FL |		\
		FS_NOCOMP_FL |		\
		FS_INDEX_FL |		\
		FS_DIRSYNC_FL |		\
		FS_PROJINHERIT_FL |	\
		FS_ENCRYPT_FL |		\
		FS_INLINE_DATA_FL |	\
		F2FS_UNRM_PHOTO_FL |	\
		F2FS_UNRM_VIDEO_FL |	\
		F2FS_UNRM_DMD_PHOTO_FL |\
		F2FS_UNRM_DMD_VIDEO_FL |\
		FS_NOCOW_FL |		\
		FS_VERITY_FL |		\
		FS_CASEFOLD_FL)

#define F2FS_SETTABLE_FS_FL (		\
		FS_COMPR_FL |		\
		FS_SYNC_FL |		\
		FS_IMMUTABLE_FL |	\
		FS_APPEND_FL |		\
		FS_NODUMP_FL |		\
		FS_NOATIME_FL |		\
		FS_NOCOMP_FL |		\
		FS_DIRSYNC_FL |		\
		F2FS_UNRM_PHOTO_FL |	\
		F2FS_UNRM_VIDEO_FL |	\
		F2FS_UNRM_DMD_PHOTO_FL |\
		F2FS_UNRM_DMD_VIDEO_FL |\
		FS_PROJINHERIT_FL |	\
		FS_CASEFOLD_FL)

/* Convert f2fs on-disk i_flags to FS_IOC_{GET,SET}FLAGS flags */
static inline u32 f2fs_iflags_to_fsflags(u32 iflags)
{
	u32 fsflags = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(f2fs_fsflags_map); i++)
		if (iflags & f2fs_fsflags_map[i].iflag)
			fsflags |= f2fs_fsflags_map[i].fsflag;

	return fsflags;
}

/* Convert FS_IOC_{GET,SET}FLAGS flags to f2fs on-disk i_flags */
static inline u32 f2fs_fsflags_to_iflags(u32 fsflags)
{
	u32 iflags = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(f2fs_fsflags_map); i++)
		if (fsflags & f2fs_fsflags_map[i].fsflag)
			iflags |= f2fs_fsflags_map[i].iflag;

	return iflags;
}

static int f2fs_ioc_getflags(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	u32 fsflags = f2fs_iflags_to_fsflags(fi->i_flags);

	if (IS_ENCRYPTED(inode))
		fsflags |= FS_ENCRYPT_FL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	if (IS_VERITY(inode))
		fsflags |= FS_VERITY_FL;
#endif
	if (f2fs_has_inline_data(inode) || f2fs_has_inline_dentry(inode))
		fsflags |= FS_INLINE_DATA_FL;
	if (is_inode_flag_set(inode, FI_PIN_FILE))
		fsflags |= FS_NOCOW_FL;

	fsflags &= F2FS_GETTABLE_FS_FL;

	return put_user(fsflags, (int __user *)arg);
}

static int f2fs_ioc_setflags(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	u32 fsflags, old_fsflags;
	u32 iflags;
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (get_user(fsflags, (int __user *)arg))
		return -EFAULT;

	if (fsflags & ~F2FS_GETTABLE_FS_FL)
		return -EOPNOTSUPP;
	fsflags &= F2FS_SETTABLE_FS_FL;

	iflags = f2fs_fsflags_to_iflags(fsflags);
	if (f2fs_mask_flags(inode->i_mode, iflags) != iflags)
		return -EOPNOTSUPP;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

	old_fsflags = f2fs_iflags_to_fsflags(fi->i_flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	ret = vfs_ioc_setflags_prepare(inode, old_fsflags, fsflags);
#else
	if ((fsflags ^ old_fsflags) & (FS_APPEND_FL | FS_IMMUTABLE_FL))
		if (!capable(CAP_LINUX_IMMUTABLE))
			ret = -EPERM;
#endif
	if (ret)
		goto out;

	ret = f2fs_setflags_common(inode, iflags,
			f2fs_fsflags_to_iflags(F2FS_SETTABLE_FS_FL));
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_getversion(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);

	return put_user(inode->i_generation, (int __user *)arg);
}

static int f2fs_ioc_start_atomic_write(struct file *filp)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (filp->f_flags & O_DIRECT)
		return -EINVAL;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		ret = f2fs_revoke_deduped_inode(inode, __func__);
		if (ret)
			goto out;
	}
#endif

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_compressed_inode(inode)) {
		ret = f2fs_decompress_file_internal(filp, ATOMIC_WRITE, 0);
		if (ret)
			goto out;
	}
#else
	f2fs_disable_compressed_file(inode);
#endif

	if (f2fs_is_atomic_file(inode)) {
		if (is_inode_flag_set(inode, FI_ATOMIC_REVOKE_REQUEST))
			ret = -EINVAL;
		goto out;
	}

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		goto out;

	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);

	/*
	 * Should wait end_io to count F2FS_WB_CP_DATA correctly by
	 * f2fs_is_atomic_file.
	 */
	if (get_dirty_pages(inode))
		f2fs_warn(F2FS_I_SB(inode), "Unexpected flush for atomic writes: ino=%lu, npages=%u",
					inode->i_ino, get_dirty_pages(inode));
	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret) {
		up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
		goto out;
	}

	spin_lock(&sbi->inode_lock[ATOMIC_FILE]);
	if (list_empty(&fi->inmem_ilist))
		list_add_tail(&fi->inmem_ilist, &sbi->inode_list[ATOMIC_FILE]);
	sbi->atomic_files++;
	spin_unlock(&sbi->inode_lock[ATOMIC_FILE]);

	/* add inode in inmem_list first and set atomic_file */
	set_inode_flag(inode, FI_ATOMIC_FILE);
	clear_inode_flag(inode, FI_ATOMIC_REVOKE_REQUEST);
	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);

	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);
	F2FS_I(inode)->inmem_task = current;
	stat_update_max_atomic_write(inode);
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_commit_atomic_write(struct file *filp)
{
	struct inode *inode = file_inode(filp);
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	inode_lock(inode);

	if (f2fs_is_volatile_file(inode)) {
		ret = -EINVAL;
		goto err_out;
	}

	if (f2fs_is_atomic_file(inode)) {
		ret = f2fs_commit_inmem_pages(inode);
		if (ret)
			goto err_out;

		ret = f2fs_do_sync_file(filp, 0, LLONG_MAX, 0, true);
		if (!ret)
			f2fs_drop_inmem_pages(inode);
	} else {
		ret = f2fs_do_sync_file(filp, 0, LLONG_MAX, 1, false);
	}
err_out:
	if (is_inode_flag_set(inode, FI_ATOMIC_REVOKE_REQUEST)) {
		clear_inode_flag(inode, FI_ATOMIC_REVOKE_REQUEST);
		ret = -EINVAL;
	}
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_start_volatile_write(struct file *filp)
{
	struct inode *inode = file_inode(filp);
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (filp->f_flags & O_DIRECT)
		return -EINVAL;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		ret = f2fs_reserve_compress_blocks_internal(inode, VOLATILE_WRITE);
		if (ret)
			goto out;
	}
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		ret = f2fs_revoke_deduped_inode(inode, __func__);
		if (ret)
			goto out;
	}
#endif

	if (f2fs_is_volatile_file(inode))
		goto out;

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		goto out;

	stat_inc_volatile_write(inode);
	stat_update_max_volatile_write(inode);

	set_inode_flag(inode, FI_VOLATILE_FILE);
	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_release_volatile_write(struct file *filp)
{
	struct inode *inode = file_inode(filp);
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

	if (!f2fs_is_volatile_file(inode))
		goto out;

	if (!f2fs_is_first_block_written(inode)) {
		ret = truncate_partial_data_page(inode, 0, true);
		goto out;
	}

	ret = punch_hole(inode, 0, F2FS_BLKSIZE);
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_abort_volatile_write(struct file *filp)
{
	struct inode *inode = file_inode(filp);
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

	if (f2fs_is_atomic_file(inode))
		f2fs_drop_inmem_pages(inode);
	if (f2fs_is_volatile_file(inode)) {
		clear_inode_flag(inode, FI_VOLATILE_FILE);
		stat_dec_volatile_write(inode);
		ret = f2fs_do_sync_file(filp, 0, LLONG_MAX, 0, true);
	}

	clear_inode_flag(inode, FI_ATOMIC_REVOKE_REQUEST);

	inode_unlock(inode);

	mnt_drop_write_file(filp);
	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);
	return ret;
}

static int f2fs_ioc_shutdown(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct super_block *sb = sbi->sb;
	__u32 in;
	int ret = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (get_user(in, (__u32 __user *)arg))
		return -EFAULT;

	if (in != F2FS_GOING_DOWN_FULLSYNC) {
		ret = mnt_want_write_file(filp);
		if (ret)
			return ret;
	}

	switch (in) {
	case F2FS_GOING_DOWN_FULLSYNC:
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
		ret = freeze_bdev(sb->s_bdev);
		if (ret)
			goto out;

		f2fs_stop_checkpoint(sbi, false);
		set_sbi_flag(sbi, SBI_IS_SHUTDOWN);
		thaw_bdev(sb->s_bdev);
#else
		sb = freeze_bdev(sb->s_bdev);
		if (IS_ERR(sb)) {
			ret = PTR_ERR(sb);
			goto out;
		}
		if (sb) {
			f2fs_stop_checkpoint(sbi, false);
			set_sbi_flag(sbi, SBI_IS_SHUTDOWN);
			thaw_bdev(sb->s_bdev, sb);
		}
#endif
		break;
	case F2FS_GOING_DOWN_METASYNC:
		/* do checkpoint only */
		ret = f2fs_sync_fs(sb, 1);
		if (ret)
			goto out;
		f2fs_stop_checkpoint(sbi, false);
		set_sbi_flag(sbi, SBI_IS_SHUTDOWN);
		break;
	case F2FS_GOING_DOWN_NOSYNC:
		f2fs_stop_checkpoint(sbi, false);
		set_sbi_flag(sbi, SBI_IS_SHUTDOWN);
		break;
	case F2FS_GOING_DOWN_METAFLUSH:
		f2fs_sync_meta_pages(sbi, META, LONG_MAX, FS_META_IO);
		f2fs_stop_checkpoint(sbi, false);
		set_sbi_flag(sbi, SBI_IS_SHUTDOWN);
		break;
	case F2FS_GOING_DOWN_NEED_FSCK:
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		set_sbi_flag(sbi, SBI_CP_DISABLED_QUICK);
		set_sbi_flag(sbi, SBI_IS_DIRTY);
		/* do checkpoint only */
		ret = f2fs_sync_fs(sb, 1);
		goto out;
	default:
		ret = -EINVAL;
		goto out;
	}

	f2fs_stop_gc_thread(sbi);
	f2fs_stop_discard_thread(sbi);

	f2fs_drop_discard_cmd(sbi);
	clear_opt(sbi, DISCARD);

	f2fs_update_time(sbi, REQ_TIME);
out:
	if (in != F2FS_GOING_DOWN_FULLSYNC)
		mnt_drop_write_file(filp);

	trace_f2fs_shutdown(sbi, in, ret);

	return ret;
}

static int f2fs_ioc_fitrim(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct super_block *sb = inode->i_sb;
	struct request_queue *q = bdev_get_queue(sb->s_bdev);
	struct fstrim_range range;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!f2fs_hw_support_discard(F2FS_SB(sb)))
		return -EOPNOTSUPP;

	if (copy_from_user(&range, (struct fstrim_range __user *)arg,
				sizeof(range)))
		return -EFAULT;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	range.minlen = max((unsigned int)range.minlen,
				q->limits.discard_granularity);
	printk_ratelimited(KERN_INFO
		"%s: Recive fstrim command from userspace!\n", __func__);
	ret = f2fs_trim_fs(F2FS_SB(sb), &range);
	mnt_drop_write_file(filp);
	if (ret < 0)
		return ret;

	if (copy_to_user((struct fstrim_range __user *)arg, &range,
				sizeof(range)))
		return -EFAULT;
	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);
	return 0;
}

static bool uuid_is_nonzero(__u8 u[16])
{
	int i;

	for (i = 0; i < 16; i++)
		if (u[i])
			return true;
	return false;
}

static int f2fs_ioc_set_encryption_policy(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);

	if (!f2fs_sb_has_encrypt(F2FS_I_SB(inode)))
		return -EOPNOTSUPP;

	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);

	return fscrypt_ioctl_set_policy(filp, (const void __user *)arg);
}

static int f2fs_ioc_get_encryption_policy(struct file *filp, unsigned long arg)
{
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;
	return fscrypt_ioctl_get_policy(filp, (void __user *)arg);
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#ifdef CONFIG_SDP_ENCRYPTION
static int f2fs_ioc_set_sdp_encryption_policy(struct file *filp,
	unsigned long arg)
{
	struct inode * inode = NULL;
	if (!filp || !arg)
		return -EINVAL;
	inode = file_inode(filp);
	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);

	return fscrypt_ioctl_set_sdp_policy(filp,
		(const void __user *)arg);
}

static int f2fs_ioc_get_sdp_encryption_policy(struct file *filp,
	unsigned long arg)
{
	return fscrypt_ioctl_get_sdp_policy(filp, (void __user *)arg);
}

static int f2fs_ioc_get_encryption_policy_type(struct file *filp,
	unsigned long arg)
{
	return fscrypt_ioctl_get_policy_type(filp, (void __user *)arg);
}
#endif

#ifdef CONFIG_HWDPS
static int f2fs_ioc_set_dps_encryption_policy(struct file *filp,
	unsigned long arg)
{
	struct inode * inode = NULL;

	if (!filp)
		return -EINVAL;
	inode = file_inode(filp);
	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);

	return fscrypt_ioctl_set_dps_policy(filp,
		(const void __user *)arg);
}
#endif
#endif

static int f2fs_ioc_get_encryption_pwsalt(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int err;

	if (!f2fs_sb_has_encrypt(sbi))
		return -EOPNOTSUPP;

	err = mnt_want_write_file(filp);
	if (err)
		return err;

	down_write(&sbi->sb_lock);

	if (uuid_is_nonzero(sbi->raw_super->encrypt_pw_salt))
		goto got_it;

	/* update superblock with uuid */
	generate_random_uuid(sbi->raw_super->encrypt_pw_salt);

	err = f2fs_commit_super(sbi, false);
	if (err) {
		/* undo new data */
		memset(sbi->raw_super->encrypt_pw_salt, 0, 16);
		goto out_err;
	}
got_it:
	if (copy_to_user((__u8 __user *)arg, sbi->raw_super->encrypt_pw_salt,
									16))
		err = -EFAULT;
out_err:
	up_write(&sbi->sb_lock);
	mnt_drop_write_file(filp);
	return err;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static int f2fs_ioc_get_encryption_policy_ex(struct file *filp,
					     unsigned long arg)
{
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fscrypt_ioctl_get_policy_ex(filp, (void __user *)arg);
}

static int f2fs_ioc_add_encryption_key(struct file *filp, unsigned long arg)
{
#ifdef CONFIG_HW_F2FS_CHIP_KIRIN
	return -ENOTTY;
#endif
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fscrypt_ioctl_add_key(filp, (void __user *)arg);
}

static int f2fs_ioc_remove_encryption_key(struct file *filp, unsigned long arg)
{
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fscrypt_ioctl_remove_key(filp, (void __user *)arg);
}

static int f2fs_ioc_remove_encryption_key_all_users(struct file *filp,
						    unsigned long arg)
{
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fscrypt_ioctl_remove_key_all_users(filp, (void __user *)arg);
}

static int f2fs_ioc_get_encryption_key_status(struct file *filp,
					      unsigned long arg)
{
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fscrypt_ioctl_get_key_status(filp, (void __user *)arg);
}

static int f2fs_ioc_get_encryption_nonce(struct file *filp, unsigned long arg)
{
	if (!f2fs_sb_has_encrypt(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fscrypt_ioctl_get_nonce(filp, (void __user *)arg);
}
#endif

static int IOC_GC_count = 0;
static int f2fs_ioc_gc(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	__u32 sync;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (get_user(sync, (__u32 __user *)arg))
		return -EFAULT;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	if (!sync) {
		if (!down_write_trylock(&sbi->gc_lock)) {
			ret = -EBUSY;
			goto out;
		}
	} else {
		down_write(&sbi->gc_lock);
	}

	f2fs_info(sbi, "IOC_GC: Size=%lldMB,Free=%lldMB,count=%d,sync=%d\n",
		blks_to_mb(sbi->user_block_count, sbi->blocksize),
		blks_to_mb(sbi->user_block_count - valid_user_blocks(sbi), sbi->blocksize),
		IOC_GC_count++, sync);
	current->flags |= PF_MUTEX_GC;
#ifdef CONFIG_F2FS_TURBO_ZONE
	ret = f2fs_gc(sbi, sync, true, false, false, NULL_SEGNO);
#else
	ret = f2fs_gc(sbi, sync, true, false, NULL_SEGNO);
#endif
	current->flags &= (~PF_MUTEX_GC);
out:
	mnt_drop_write_file(filp);
	return ret;
}

#ifdef CONFIG_F2FS_TURBO_ZONE
static bool include_resvd_device(struct f2fs_sb_info *sbi,u64 start, u64 end)
{
	int i;

	for (i = 0; i < sbi->s_ndevs; i++) {
		if (!FDEV(i).bdev && (start < FDEV(i).start_blk && end > FDEV(i).end_blk))
			return true; //lint !e1564
	}

	return false; //lint !e1564
}
#endif

static int __f2fs_ioc_gc_range(struct file *filp, struct f2fs_gc_range *range)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(file_inode(filp));
	u64 end;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	end = range->start + range->len;
	if (end < range->start || range->start < MAIN_BLKADDR(sbi) ||
					end >= MAX_BLKADDR(sbi))
		return -EINVAL;

#ifdef CONFIG_F2FS_TURBO_ZONE
	if (sbi->s_ndevs && (is_in_resvd_device(sbi, range->start) ||
		is_in_resvd_device(sbi, end) ||
		include_resvd_device(sbi, range->start, end)))
		return -EINVAL;
#endif

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

do_more:
	if (!range->sync) {
		if (!down_write_trylock(&sbi->gc_lock)) {
			ret = -EBUSY;
			goto out;
		}
	} else {
		down_write(&sbi->gc_lock);
	}
	current->flags |= PF_MUTEX_GC;
#ifdef CONFIG_F2FS_TURBO_ZONE
	ret = f2fs_gc(sbi, range->sync, true, false, false,
					GET_SEGNO(sbi, range->start));
#else
	ret = f2fs_gc(sbi, range->sync, true, false,
				GET_SEGNO(sbi, range->start));
#endif
	if (ret) {
		if (ret == -EBUSY)
			ret = -EAGAIN;
		goto out;
	}
	current->flags &= (~PF_MUTEX_GC);
	range->start += BLKS_PER_SEC(sbi);
	if (range->start <= end)
		goto do_more;
out:
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_gc_range(struct file *filp, unsigned long arg)
{
	struct f2fs_gc_range range;

	if (copy_from_user(&range, (struct f2fs_gc_range __user *)arg,
							sizeof(range)))
		return -EFAULT;
	return __f2fs_ioc_gc_range(filp, &range);
}

static int f2fs_ioc_write_checkpoint(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (unlikely(is_sbi_flag_set(sbi, SBI_CP_DISABLED))) {
		f2fs_info(sbi, "Skipping Checkpoint. Checkpoints currently disabled.");
		return -EINVAL;
	}

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	ret = f2fs_sync_fs(sbi->sb, 1);

	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_defragment_range(struct f2fs_sb_info *sbi,
					struct file *filp,
					struct f2fs_defragment *range)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_map_blocks map = { .m_next_extent = NULL,
					.m_seg_type = NO_CHECK_TYPE,
					.m_may_create = false };
	struct extent_info ei = {0, 0, 0};
	pgoff_t pg_start, pg_end, next_pgofs;
	unsigned int blk_per_seg = sbi->blocks_per_seg;
	unsigned int total = 0, sec_num;
	block_t blk_end = 0;
	bool fragmented = false;
	int err;

	/* if in-place-update policy is enabled, don't waste time here */
	if (f2fs_should_update_inplace(inode, NULL))
		return -EINVAL;

	pg_start = range->start >> PAGE_SHIFT;
	pg_end = (range->start + range->len) >> PAGE_SHIFT;

	f2fs_balance_fs(sbi, true);

	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		err = f2fs_reserve_compress_blocks_internal(inode, DEFRAGMENT_RANGE);
		if (err)
			goto out;
	}
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		err = f2fs_revoke_deduped_inode(inode, __func__);
		if (err)
			goto out;
	}
#endif

	/* writeback all dirty pages in the range */
	err = filemap_write_and_wait_range(inode->i_mapping, range->start,
						range->start + range->len - 1);
	if (err)
		goto out;

	/*
	 * lookup mapping info in extent cache, skip defragmenting if physical
	 * block addresses are continuous.
	 */
	if (f2fs_lookup_extent_cache(inode, pg_start, &ei)) {
		if (ei.fofs + ei.len >= pg_end)
			goto out;
	}

	map.m_lblk = pg_start;
	map.m_next_pgofs = &next_pgofs;

	/*
	 * lookup mapping info in dnode page cache, skip defragmenting if all
	 * physical block addresses are continuous even if there are hole(s)
	 * in logical blocks.
	 */
	while (map.m_lblk < pg_end) {
		map.m_len = pg_end - map.m_lblk;
		err = f2fs_map_blocks(inode, &map, 0, F2FS_GET_BLOCK_DEFAULT);
		if (err)
			goto out;

		if (!(map.m_flags & F2FS_MAP_FLAGS)) {
			map.m_lblk = next_pgofs;
			continue;
		}

		if (blk_end && blk_end != map.m_pblk)
			fragmented = true;

		/* record total count of block that we're going to move */
		total += map.m_len;

		blk_end = map.m_pblk + map.m_len;

		map.m_lblk += map.m_len;
	}

	if (!fragmented) {
		total = 0;
		goto out;
	}

	sec_num = DIV_ROUND_UP(total, BLKS_PER_SEC(sbi));

	/*
	 * make sure there are enough free section for LFS allocation, this can
	 * avoid defragment running in SSR mode when free section are allocated
	 * intensively
	 */
	if (has_not_enough_free_secs(sbi, 0, sec_num)) {
		err = -EAGAIN;
		goto out;
	}

	map.m_lblk = pg_start;
	map.m_len = pg_end - pg_start;
	total = 0;

	while (map.m_lblk < pg_end) {
		pgoff_t idx;
		int cnt = 0;

do_map:
		map.m_len = pg_end - map.m_lblk;
		err = f2fs_map_blocks(inode, &map, 0, F2FS_GET_BLOCK_DEFAULT);
		if (err)
			goto clear_out;

		if (!(map.m_flags & F2FS_MAP_FLAGS)) {
			map.m_lblk = next_pgofs;
			goto check;
		}

		set_inode_flag(inode, FI_DO_DEFRAG);

		idx = map.m_lblk;
		while (idx < map.m_lblk + map.m_len && cnt < blk_per_seg) {
			struct page *page;

			page = f2fs_get_lock_data_page(inode, idx, true);
			if (IS_ERR(page)) {
				err = PTR_ERR(page);
				goto clear_out;
			}

			set_page_dirty(page);
			f2fs_put_page(page, 1);

			idx++;
			cnt++;
			total++;
		}

		map.m_lblk = idx;
check:
		if (map.m_lblk < pg_end && cnt < blk_per_seg)
			goto do_map;

		clear_inode_flag(inode, FI_DO_DEFRAG);

		err = filemap_fdatawrite(inode->i_mapping);
		if (err)
			goto out;
	}
clear_out:
	clear_inode_flag(inode, FI_DO_DEFRAG);
out:
	inode_unlock(inode);
	if (!err)
		range->len = (u64)total << PAGE_SHIFT;
	return err;
}

static int f2fs_ioc_defragment(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_defragment range;
	int err;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!S_ISREG(inode->i_mode) || f2fs_is_atomic_file(inode))
		return -EINVAL;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (copy_from_user(&range, (struct f2fs_defragment __user *)arg,
							sizeof(range)))
		return -EFAULT;

	/* verify alignment of offset & size */
	if (range.start & (F2FS_BLKSIZE - 1) || range.len & (F2FS_BLKSIZE - 1))
		return -EINVAL;

	if (unlikely((range.start + range.len) >> PAGE_SHIFT >
					max_file_blocks(inode)))
		return -EINVAL;

	err = mnt_want_write_file(filp);
	if (err)
		return err;

	err = f2fs_defragment_range(sbi, filp, &range);
	mnt_drop_write_file(filp);

	f2fs_update_time(sbi, REQ_TIME);
	if (err < 0)
		return err;

	if (copy_to_user((struct f2fs_defragment __user *)arg, &range,
							sizeof(range)))
		return -EFAULT;

	return 0;
}

static int f2fs_move_file_range(struct file *file_in, loff_t pos_in,
			struct file *file_out, loff_t pos_out, size_t len)
{
	struct inode *src = file_inode(file_in);
	struct inode *dst = file_inode(file_out);
	struct f2fs_sb_info *sbi = F2FS_I_SB(src);
	size_t olen = len, dst_max_i_size = 0;
	size_t dst_osize;
	int ret;

	if (file_in->f_path.mnt != file_out->f_path.mnt ||
				src->i_sb != dst->i_sb)
		return -EXDEV;

	if (unlikely(f2fs_readonly(src->i_sb)))
		return -EROFS;

	if (!S_ISREG(src->i_mode) || !S_ISREG(dst->i_mode))
		return -EINVAL;

	if (IS_ENCRYPTED(src) || IS_ENCRYPTED(dst))
		return -EOPNOTSUPP;

	if (pos_out < 0 || pos_in < 0)
		return -EINVAL;

	if (src == dst) {
		if (pos_in == pos_out)
			return 0;
		if (pos_out > pos_in && pos_out < pos_in + len)
			return -EINVAL;
	}

	inode_lock(src);
	if (src != dst) {
		ret = -EBUSY;
		if (!inode_trylock(dst))
			goto out;
	}

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (is_inode_flag_set(src, FI_COMPRESS_RELEASED)) {
		ret = f2fs_reserve_compress_blocks_internal(src, MOVE_SRC);
		if (ret)
			goto out_unlock;
	}
	if (is_inode_flag_set(dst, FI_COMPRESS_RELEASED)) {
		ret = f2fs_reserve_compress_blocks_internal(dst, MOVE_DST);
		if (ret)
			goto out_unlock;
	}
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(src);
	if (f2fs_is_outer_inode(src)) {
		ret = f2fs_revoke_deduped_inode(src, __func__);
		if (ret)
			goto out_unlock;
	}

	mark_file_modified(dst);
	if (f2fs_is_outer_inode(dst)) {
		ret = f2fs_revoke_deduped_inode(dst, __func__);
		if (ret)
			goto out_unlock;
	}
#endif

	ret = -EINVAL;
	if (pos_in + len > src->i_size || pos_in + len < pos_in)
		goto out_unlock;
	if (len == 0)
		olen = len = src->i_size - pos_in;
	if (pos_in + len == src->i_size)
		len = ALIGN(src->i_size, F2FS_BLKSIZE) - pos_in;
	if (len == 0) {
		ret = 0;
		goto out_unlock;
	}

	dst_osize = dst->i_size;
	if (pos_out + olen > dst->i_size)
		dst_max_i_size = pos_out + olen;

	/* verify the end result is block aligned */
	if (!IS_ALIGNED(pos_in, F2FS_BLKSIZE) ||
			!IS_ALIGNED(pos_in + len, F2FS_BLKSIZE) ||
			!IS_ALIGNED(pos_out, F2FS_BLKSIZE))
		goto out_unlock;

	ret = f2fs_convert_inline_inode(src);
	if (ret)
		goto out_unlock;

	ret = f2fs_convert_inline_inode(dst);
	if (ret)
		goto out_unlock;

	/* write out all dirty pages from offset */
	ret = filemap_write_and_wait_range(src->i_mapping,
					pos_in, pos_in + len);
	if (ret)
		goto out_unlock;

	ret = filemap_write_and_wait_range(dst->i_mapping,
					pos_out, pos_out + len);
	if (ret)
		goto out_unlock;

	f2fs_balance_fs(sbi, true);

	down_write(&F2FS_I(src)->i_gc_rwsem[WRITE]);
	if (src != dst) {
		ret = -EBUSY;
		if (!down_write_trylock(&F2FS_I(dst)->i_gc_rwsem[WRITE]))
			goto out_src;
	}

	f2fs_lock_op(sbi);
	ret = __exchange_data_block(src, dst, pos_in >> F2FS_BLKSIZE_BITS,
				pos_out >> F2FS_BLKSIZE_BITS,
				len >> F2FS_BLKSIZE_BITS, false);

	if (!ret) {
		if (dst_max_i_size)
			f2fs_i_size_write(dst, dst_max_i_size);
		else if (dst_osize != dst->i_size)
			f2fs_i_size_write(dst, dst_osize);
	}
	f2fs_unlock_op(sbi);

	if (src != dst)
		up_write(&F2FS_I(dst)->i_gc_rwsem[WRITE]);
out_src:
	up_write(&F2FS_I(src)->i_gc_rwsem[WRITE]);
out_unlock:
	if (src != dst)
		inode_unlock(dst);
out:
	inode_unlock(src);
	return ret;
}

static int __f2fs_ioc_move_range(struct file *filp,
				struct f2fs_move_range *range)
{
	struct fd dst;
	int err;

	if (!(filp->f_mode & FMODE_READ) ||
			!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	dst = fdget(range->dst_fd);
	if (!dst.file)
		return -EBADF;

	if (!(dst.file->f_mode & FMODE_WRITE)) {
		err = -EBADF;
		goto err_out;
	}

	err = mnt_want_write_file(filp);
	if (err)
		goto err_out;

	err = f2fs_move_file_range(filp, range->pos_in, dst.file,
					range->pos_out, range->len);

	mnt_drop_write_file(filp);
err_out:
	fdput(dst);
	return err;
}

static int f2fs_ioc_move_range(struct file *filp, unsigned long arg)
{
	struct f2fs_move_range range;

	if (copy_from_user(&range, (struct f2fs_move_range __user *)arg,
							sizeof(range)))
		return -EFAULT;
	return __f2fs_ioc_move_range(filp, &range);
}

static int f2fs_ioc_flush_device(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct sit_info *sm = SIT_I(sbi);
	unsigned int start_segno = 0, end_segno = 0;
	unsigned int dev_start_segno = 0, dev_end_segno = 0;
	struct f2fs_flush_device range;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (unlikely(is_sbi_flag_set(sbi, SBI_CP_DISABLED)))
		return -EINVAL;

	if (copy_from_user(&range, (struct f2fs_flush_device __user *)arg,
							sizeof(range)))
		return -EFAULT;

	if (!f2fs_is_multi_device(sbi) || sbi->s_ndevs - 1 <= range.dev_num ||
			__is_large_section(sbi)) {
		f2fs_warn(sbi, "Can't flush %u in %d for segs_per_sec %u != 1",
			  range.dev_num, sbi->s_ndevs, sbi->segs_per_sec);
		return -EINVAL;
	}

#ifdef CONFIG_F2FS_TURBO_ZONE
	/* cannot flush reserved device */
	if (!FDEV(range.dev_num).bdev)
		return -EINVAL;
#endif

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	if (range.dev_num != 0)
		dev_start_segno = GET_SEGNO(sbi, FDEV(range.dev_num).start_blk);
	dev_end_segno = GET_SEGNO(sbi, FDEV(range.dev_num).end_blk);

	start_segno = sm->last_victim[FLUSH_DEVICE];
	if (start_segno < dev_start_segno || start_segno >= dev_end_segno)
		start_segno = dev_start_segno;
	end_segno = min(start_segno + range.segments, dev_end_segno);

	while (start_segno < end_segno) {
		if (!down_write_trylock(&sbi->gc_lock)) {
			ret = -EBUSY;
			goto out;
		}
		sm->last_victim[GC_CB] = end_segno + 1;
		sm->last_victim[GC_GREEDY] = end_segno + 1;
		sm->last_victim[ALLOC_NEXT] = end_segno + 1;
		current->flags |= PF_MUTEX_GC;
#ifdef CONFIG_F2FS_TURBO_ZONE
		ret = f2fs_gc(sbi, true, true, true, false, start_segno);
#else
		ret = f2fs_gc(sbi, true, true, true, start_segno);
#endif
		current->flags &= (~PF_MUTEX_GC);
		if (ret == -EAGAIN)
			ret = 0;
		else if (ret < 0)
			break;
		start_segno++;
	}
out:
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_get_features(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	u32 sb_feature = le32_to_cpu(F2FS_I_SB(inode)->raw_super->feature);

	/* Must validate to set it with SQLite behavior in Android. */
	sb_feature |= F2FS_FEATURE_ATOMIC_WRITE;

	return put_user(sb_feature, (u32 __user *)arg);
}

#ifdef CONFIG_QUOTA
int f2fs_transfer_project_quota(struct inode *inode, kprojid_t kprojid)
{
	struct dquot *transfer_to[MAXQUOTAS] = {};
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct super_block *sb = sbi->sb;
	int err = 0;

	transfer_to[PRJQUOTA] = dqget(sb, make_kqid_projid(kprojid));
	if (!IS_ERR(transfer_to[PRJQUOTA])) {
		err = __dquot_transfer(inode, transfer_to);
		if (err)
			set_sbi_flag(sbi, SBI_QUOTA_NEED_REPAIR);
		dqput(transfer_to[PRJQUOTA]);
	}
	return err;
}

static int f2fs_ioc_setproject(struct file *filp, __u32 projid)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct page *ipage;
	kprojid_t kprojid;
	int err;

	if (!f2fs_sb_has_project_quota(sbi)) {
		if (projid != F2FS_DEF_PROJID)
			return -EOPNOTSUPP;
		else
			return 0;
	}

	if (!f2fs_has_extra_attr(inode))
		return -EOPNOTSUPP;

	kprojid = make_kprojid(&init_user_ns, (projid_t)projid);

	if (projid_eq(kprojid, F2FS_I(inode)->i_projid))
		return 0;

	err = -EPERM;
	/* Is it quota file? Do not allow user to mess with it */
	if (IS_NOQUOTA(inode))
		return err;

	ipage = f2fs_get_node_page(sbi, inode->i_ino);
	if (IS_ERR(ipage))
		return PTR_ERR(ipage);

	if (!F2FS_FITS_IN_INODE(F2FS_INODE(ipage), fi->i_extra_isize,
								i_projid)) {
		err = -EOVERFLOW;
		f2fs_put_page(ipage, 1);
		return err;
	}
	f2fs_put_page(ipage, 1);

	err = dquot_initialize(inode);
	if (err)
		return err;

	f2fs_lock_op(sbi);
	err = f2fs_transfer_project_quota(inode, kprojid);
	if (err)
		goto out_unlock;

	F2FS_I(inode)->i_projid = kprojid;
	inode->i_ctime = current_time(inode);
	f2fs_mark_inode_dirty_sync(inode, true);
out_unlock:
	f2fs_unlock_op(sbi);
	return err;
}
#else
int f2fs_transfer_project_quota(struct inode *inode, kprojid_t kprojid)
{
	return 0;
}

static int f2fs_ioc_setproject(struct file *filp, __u32 projid)
{
	if (projid != F2FS_DEF_PROJID)
		return -EOPNOTSUPP;
	return 0;
}
#endif

/* FS_IOC_FSGETXATTR and FS_IOC_FSSETXATTR support */

/*
 * To make a new on-disk f2fs i_flag gettable via FS_IOC_FSGETXATTR and settable
 * via FS_IOC_FSSETXATTR, add an entry for it to f2fs_xflags_map[], and add its
 * FS_XFLAG_* equivalent to F2FS_SUPPORTED_XFLAGS.
 */

static const struct {
	u32 iflag;
	u32 xflag;
} f2fs_xflags_map[] = {
	{ F2FS_SYNC_FL,		FS_XFLAG_SYNC },
	{ F2FS_IMMUTABLE_FL,	FS_XFLAG_IMMUTABLE },
	{ F2FS_APPEND_FL,	FS_XFLAG_APPEND },
	{ F2FS_NODUMP_FL,	FS_XFLAG_NODUMP },
	{ F2FS_NOATIME_FL,	FS_XFLAG_NOATIME },
	{ F2FS_PROJINHERIT_FL,	FS_XFLAG_PROJINHERIT },
};

#define F2FS_SUPPORTED_XFLAGS (		\
		FS_XFLAG_SYNC |		\
		FS_XFLAG_IMMUTABLE |	\
		FS_XFLAG_APPEND |	\
		FS_XFLAG_NODUMP |	\
		FS_XFLAG_NOATIME |	\
		FS_XFLAG_PROJINHERIT)

/* Convert f2fs on-disk i_flags to FS_IOC_FS{GET,SET}XATTR flags */
static inline u32 f2fs_iflags_to_xflags(u32 iflags)
{
	u32 xflags = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(f2fs_xflags_map); i++)
		if (iflags & f2fs_xflags_map[i].iflag)
			xflags |= f2fs_xflags_map[i].xflag;

	return xflags;
}

/* Convert FS_IOC_FS{GET,SET}XATTR flags to f2fs on-disk i_flags */
static inline u32 f2fs_xflags_to_iflags(u32 xflags)
{
	u32 iflags = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(f2fs_xflags_map); i++)
		if (xflags & f2fs_xflags_map[i].xflag)
			iflags |= f2fs_xflags_map[i].iflag;

	return iflags;
}

static void f2fs_fill_fsxattr(struct inode *inode, struct fsxattr *fa)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	simple_fill_fsxattr(fa, f2fs_iflags_to_xflags(fi->i_flags));
#else
	memset(fa, 0, sizeof(struct fsxattr));
	fa->fsx_xflags = f2fs_iflags_to_xflags(fi->i_flags);
#endif

	if (f2fs_sb_has_project_quota(F2FS_I_SB(inode)))
		fa->fsx_projid = from_kprojid(&init_user_ns, fi->i_projid);
}

static int f2fs_ioc_fsgetxattr(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct fsxattr fa;

	f2fs_fill_fsxattr(inode, &fa);

	if (copy_to_user((struct fsxattr __user *)arg, &fa, sizeof(fa)))
		return -EFAULT;
	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
static int f2fs_ioctl_check_project(struct inode *inode, struct fsxattr *fa)
{
	/*
	 * Project Quota ID state is only allowed to change from within the init
	 * namespace. Enforce that restriction only if we are trying to change
	 * the quota ID state. Everything else is allowed in user namespaces.
	 */
	if (current_user_ns() == &init_user_ns)
		return 0;

	if (__kprojid_val(F2FS_I(inode)->i_projid) != fa->fsx_projid)
		return -EINVAL;

	if (F2FS_I(inode)->i_flags & F2FS_PROJINHERIT_FL) {
		if (!(fa->fsx_xflags & FS_XFLAG_PROJINHERIT))
			return -EINVAL;
	} else {
		if (fa->fsx_xflags & FS_XFLAG_PROJINHERIT)
			return -EINVAL;
	}

	return 0;
}
#endif

static int f2fs_ioc_fssetxattr(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct fsxattr fa;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct fsxattr old_fa;
#endif
	u32 iflags;
	int err;

	if (copy_from_user(&fa, (struct fsxattr __user *)arg, sizeof(fa)))
		return -EFAULT;

	/* Make sure caller has proper permission */
	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (fa.fsx_xflags & ~F2FS_SUPPORTED_XFLAGS)
		return -EOPNOTSUPP;

	iflags = f2fs_xflags_to_iflags(fa.fsx_xflags);
	if (f2fs_mask_flags(inode->i_mode, iflags) != iflags)
		return -EOPNOTSUPP;

	err = mnt_want_write_file(filp);
	if (err)
		return err;

	inode_lock(inode);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	f2fs_fill_fsxattr(inode, &old_fa);
	err = vfs_ioc_fssetxattr_check(inode, &old_fa, &fa);
#else
	err = f2fs_ioctl_check_project(inode, &fa);
#endif
	if (err)
		goto out;

	err = f2fs_setflags_common(inode, iflags,
			f2fs_xflags_to_iflags(F2FS_SUPPORTED_XFLAGS));
	if (err)
		goto out;

	err = f2fs_ioc_setproject(filp, fa.fsx_projid);
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return err;
}

int f2fs_pin_file_control(struct inode *inode, bool inc)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	/* Use i_gc_failures for normal file as a risk signal. */
	if (inc)
		f2fs_i_gc_failures_write(inode,
				fi->i_gc_failures[GC_FAILURE_PIN] + 1);

	if (fi->i_gc_failures[GC_FAILURE_PIN] > sbi->gc_pin_file_threshold) {
		f2fs_warn(sbi, "%s: Enable GC = ino %lx after %x GC trials",
			  __func__, inode->i_ino,
			  fi->i_gc_failures[GC_FAILURE_PIN]);
		clear_inode_flag(inode, FI_PIN_FILE);
		return -EAGAIN;
	}
	return 0;
}

static int f2fs_ioc_set_pin_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	__u32 pin;
	int ret = 0;

	if (get_user(pin, (__u32 __user *)arg))
		return -EFAULT;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (f2fs_readonly(F2FS_I_SB(inode)->sb))
		return -EROFS;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

	if (f2fs_should_update_outplace(inode, NULL)) {
		ret = -EINVAL;
		goto out;
	}

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		ret = f2fs_revoke_deduped_inode(inode, __func__);
		if (ret)
			goto out;
	}
#endif
	if (!pin) {
		clear_inode_flag(inode, FI_PIN_FILE);
		f2fs_i_gc_failures_write(inode, 0);
		goto done;
	}

	if (f2fs_pin_file_control(inode, false)) {
		ret = -EAGAIN;
		goto out;
	}

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		goto out;

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_compressed_inode(inode)) {
		ret = f2fs_decompress_file_internal(filp, PIN_FILE, 0);
		if (ret)
			goto out;
	}
#else
	if (!f2fs_disable_compressed_file(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}
#endif

	set_inode_flag(inode, FI_PIN_FILE);
	ret = F2FS_I(inode)->i_gc_failures[GC_FAILURE_PIN];
done:
	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_ioc_get_pin_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	__u32 pin = 0;

	if (is_inode_flag_set(inode, FI_PIN_FILE))
		pin = F2FS_I(inode)->i_gc_failures[GC_FAILURE_PIN];
	return put_user(pin, (u32 __user *)arg);
}

int f2fs_precache_extents(struct inode *inode)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_map_blocks map;
	pgoff_t m_next_extent;
	loff_t end;
	int err;

	if (is_inode_flag_set(inode, FI_NO_EXTENT))
		return -EOPNOTSUPP;

	map.m_lblk = 0;
	map.m_next_pgofs = NULL;
	map.m_next_extent = &m_next_extent;
	map.m_seg_type = NO_CHECK_TYPE;
	map.m_may_create = false;
	end = max_file_blocks(inode);

	while (map.m_lblk < end) {
		map.m_len = end - map.m_lblk;

		down_write(&fi->i_gc_rwsem[WRITE]);
		err = f2fs_map_blocks(inode, &map, 0, F2FS_GET_BLOCK_PRECACHE);
		up_write(&fi->i_gc_rwsem[WRITE]);
		if (err)
			return err;

		map.m_lblk = m_next_extent;
	}

	return 0;
}

static int f2fs_ioc_precache_extents(struct file *filp, unsigned long arg)
{
	return f2fs_precache_extents(file_inode(filp));
}

static int f2fs_ioc_resize_fs(struct file *filp, unsigned long arg)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(file_inode(filp));
	struct f2fs_resizefs param;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	if (copy_from_user(&param, (struct f2fs_resizefs __user *)arg,
			   sizeof(param)))
		return -EFAULT;

	return f2fs_resize_fs(sbi, param.len);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static int f2fs_ioc_enable_verity(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);

	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);

	if (!f2fs_sb_has_verity(F2FS_I_SB(inode))) {
		f2fs_warn(F2FS_I_SB(inode),
			  "Can't enable fs-verity on inode %lu: the verity feature is not enabled on this filesystem",
			  inode->i_ino);
		return -EOPNOTSUPP;
	}

	return fsverity_ioctl_enable(filp, (const void __user *)arg);
}

static int f2fs_ioc_measure_verity(struct file *filp, unsigned long arg)
{
	if (!f2fs_sb_has_verity(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fsverity_ioctl_measure(filp, (void __user *)arg);
}


static int f2fs_ioc_read_verity_metadata(struct file *filp, unsigned long arg)
{
	if (!f2fs_sb_has_verity(F2FS_I_SB(file_inode(filp))))
		return -EOPNOTSUPP;

	return fsverity_ioctl_read_metadata(filp, (const void __user *)arg);
}

static int f2fs_ioc_getfslabel(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	char *vbuf;
	int count;
	int err = 0;

	vbuf = f2fs_kzalloc(sbi, MAX_VOLUME_NAME, GFP_KERNEL);
	if (!vbuf)
		return -ENOMEM;

	down_read(&sbi->sb_lock);
	count = utf16s_to_utf8s(sbi->raw_super->volume_name,
			ARRAY_SIZE(sbi->raw_super->volume_name),
			UTF16_LITTLE_ENDIAN, vbuf, MAX_VOLUME_NAME);
	up_read(&sbi->sb_lock);

	if (copy_to_user((char __user *)arg, vbuf,
				min(FSLABEL_MAX, count)))
		err = -EFAULT;

	kfree(vbuf);
	return err;
}

static int f2fs_ioc_setfslabel(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	char *vbuf;
	int err = 0;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	vbuf = strndup_user((const char __user *)arg, FSLABEL_MAX);
	if (IS_ERR(vbuf))
		return PTR_ERR(vbuf);

	err = mnt_want_write_file(filp);
	if (err)
		goto out;

	down_write(&sbi->sb_lock);

	memset(sbi->raw_super->volume_name, 0,
			sizeof(sbi->raw_super->volume_name));
	utf8s_to_utf16s(vbuf, strlen(vbuf), UTF16_LITTLE_ENDIAN,
			sbi->raw_super->volume_name,
			ARRAY_SIZE(sbi->raw_super->volume_name));

	err = f2fs_commit_super(sbi, false);

	up_write(&sbi->sb_lock);

	mnt_drop_write_file(filp);
out:
	kfree(vbuf);
	return err;
}
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
static int f2fs_get_compress_blocks(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	__u64 blocks;

	if (!f2fs_sb_has_compression(F2FS_I_SB(inode)))
		return -EOPNOTSUPP;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	blocks = atomic_read(&F2FS_I(inode)->i_compr_blocks);
	return put_user(blocks, (u64 __user *)arg);
}

static int release_compress_blocks(struct dnode_of_data *dn, pgoff_t count)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dn->inode);
	unsigned int released_blocks = 0;
	int cluster_size = F2FS_I(dn->inode)->i_cluster_size;
	block_t blkaddr;
	int i;

	for (i = 0; i < count; i++) {
		blkaddr = data_blkaddr(dn->inode, dn->node_page,
						dn->ofs_in_node + i);
		if (!__is_valid_data_blkaddr(sbi, blkaddr))
			continue;
		if (unlikely(!f2fs_is_valid_blkaddr(sbi, blkaddr,
					DATA_GENERIC_ENHANCE)))
			return -EFSCORRUPTED;
	}

	while (count) {
		int compr_blocks = 0;

		for (i = 0; i < cluster_size; i++, dn->ofs_in_node++) {
			blkaddr = f2fs_data_blkaddr(dn);

			if (i == 0) {
				if (blkaddr == COMPRESS_ADDR)
					continue;
				dn->ofs_in_node += cluster_size;
				goto next;
			}
			if (__is_valid_data_blkaddr(sbi, blkaddr))
				compr_blocks++;

			if (blkaddr != NEW_ADDR)
				continue;

			dn->data_blkaddr = NULL_ADDR;
			f2fs_set_data_blkaddr(dn);
		}

		f2fs_i_compr_blocks_update(dn->inode, compr_blocks, false);
		dec_valid_block_count(sbi, dn->inode,
					cluster_size - compr_blocks);

		released_blocks += cluster_size - compr_blocks;
next:
		count -= cluster_size;
	}

	return released_blocks;
}

static int f2fs_release_compress_blocks(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t page_idx = 0, last_idx;
	unsigned int released_blocks = 0;
	int ret;
	int writecount;

	if (!f2fs_sb_has_compression(F2FS_I_SB(inode)))
		return -EOPNOTSUPP;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	ret = f2fs_dquot_initialize(inode);
	if (ret)
		return ret;
#endif

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	inode_lock(inode);

	writecount = atomic_read(&inode->i_writecount);
	if ((filp->f_mode & FMODE_WRITE && writecount != 1) ||
			(!(filp->f_mode & FMODE_WRITE) && writecount)) {
		ret = -EBUSY;
		goto out;
	}

	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		ret = -EINVAL;
		goto out;
	}

	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	set_inode_flag(inode, FI_COMPRESS_RELEASED);
	inode->i_ctime = current_time(inode);
	f2fs_mark_inode_dirty_sync(inode, true);

	if (!atomic_read(&F2FS_I(inode)->i_compr_blocks))
		goto out;

	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	while (page_idx < last_idx) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, page_idx, LOOKUP_NODE);
		if (ret) {
			if (ret == -ENOENT) {
				page_idx = f2fs_get_next_page_offset(&dn,
								page_idx);
				ret = 0;
				continue;
			}
			break;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, last_idx - page_idx);
		count = round_up(count, F2FS_I(inode)->i_cluster_size);

		ret = release_compress_blocks(&dn, count);

		f2fs_put_dnode(&dn);

		if (ret < 0)
			break;

		page_idx += count;
		released_blocks += ret;
	}

	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	up_write(&F2FS_I(inode)->i_mmap_sem);
out:
	inode_unlock(inode);

	mnt_drop_write_file(filp);

	if (ret >= 0) {
		ret = put_user(released_blocks, (u64 __user *)arg);
	} else if (released_blocks &&
			atomic_read(&F2FS_I(inode)->i_compr_blocks)) {
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_warn(sbi, "%s: partial blocks were released i_ino=%lx "
			"iblocks=%llu, released=%u, compr_blocks=%u, "
			"run fsck to fix.",
			__func__, inode->i_ino, inode->i_blocks,
			released_blocks,
			atomic_read(&F2FS_I(inode)->i_compr_blocks));
	}

	return ret;
}

static int reserve_compress_blocks(struct dnode_of_data *dn, pgoff_t count)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dn->inode);
	unsigned int reserved_blocks = 0;
	int cluster_size = F2FS_I(dn->inode)->i_cluster_size;
	block_t blkaddr;
	int i;

	for (i = 0; i < count; i++) {
		blkaddr = data_blkaddr(dn->inode, dn->node_page,
						dn->ofs_in_node + i);

		if (!__is_valid_data_blkaddr(sbi, blkaddr))
			continue;
		if (unlikely(!f2fs_is_valid_blkaddr(sbi, blkaddr,
					DATA_GENERIC_ENHANCE)))
			return -EFSCORRUPTED;
	}

	while (count) {
		int compr_blocks = 0;
		blkcnt_t reserved;
		int ret;
		unsigned int ofs_in_node = dn->ofs_in_node;

		for (i = 0; i < cluster_size; i++, dn->ofs_in_node++) {
			blkaddr = f2fs_data_blkaddr(dn);

			if (i == 0) {
				if (blkaddr == COMPRESS_ADDR)
					continue;
				dn->ofs_in_node += cluster_size;
				goto next;
			}

			if (__is_valid_data_blkaddr(sbi, blkaddr)) {
				compr_blocks++;
				continue;
			}
		}

		reserved = cluster_size - compr_blocks;
		ret = inc_valid_block_count(sbi, dn->inode, &reserved);
		if (ret)
			return ret;

		if (reserved != cluster_size - compr_blocks)
			return -ENOSPC;

		f2fs_i_compr_blocks_update(dn->inode, compr_blocks, true);

		dn->ofs_in_node = ofs_in_node;
		for (i = 0; i < cluster_size; i++, dn->ofs_in_node++) {
			blkaddr = f2fs_data_blkaddr(dn);

			if (i == 0) {
				if (blkaddr == COMPRESS_ADDR)
					continue;
				dn->ofs_in_node += cluster_size;
				goto next;
			}

			if (__is_valid_data_blkaddr(sbi, blkaddr))
				continue;

			dn->data_blkaddr = NEW_ADDR;
			f2fs_set_data_blkaddr(dn);
		}

		reserved_blocks += reserved;
next:
		count -= cluster_size;
	}

	return reserved_blocks;
}

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
static int f2fs_release_reserved_blocks(struct inode *inode, pgoff_t reserved_idx)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	unsigned int released_blocks = 0;
	pgoff_t page_idx = 0;
	int ret;

	while (page_idx < reserved_idx) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, page_idx, LOOKUP_NODE);
		if (ret) {
			if (ret == -ENOENT) {
				page_idx = f2fs_get_next_page_offset(&dn, page_idx);
				ret = 0;
				continue;
			}
			break;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, reserved_idx - page_idx);
		count = round_up(count, F2FS_I(inode)->i_cluster_size);

		ret = release_compress_blocks(&dn, count);

		f2fs_put_dnode(&dn);
		f2fs_bug_on(sbi, ret < 0);

		page_idx += count;
		released_blocks += ret;
	}

	return released_blocks;
}
#endif

static int f2fs_reserve_compress_blocks(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t page_idx = 0, last_idx;
	unsigned int reserved_blocks = 0;
	int ret;

	if (!f2fs_sb_has_compression(F2FS_I_SB(inode)))
		return -EOPNOTSUPP;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	ret = f2fs_dquot_initialize(inode);
	if (ret)
		return ret;
#endif

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	if (atomic_read(&F2FS_I(inode)->i_compr_blocks))
		goto out;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	inode_lock(inode);

	if (!is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		ret = -EINVAL;
		goto unlock_inode;
	}

	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	while (page_idx < last_idx) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, page_idx, LOOKUP_NODE);
		if (ret) {
			if (ret == -ENOENT) {
				page_idx = f2fs_get_next_page_offset(&dn,
								page_idx);
				ret = 0;
				continue;
			}
			break;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, last_idx - page_idx);
		count = round_up(count, F2FS_I(inode)->i_cluster_size);

		ret = reserve_compress_blocks(&dn, count);

		f2fs_put_dnode(&dn);

		if (ret < 0)
			break;

		page_idx += count;
		reserved_blocks += ret;
	}

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (ret < 0 && reserved_blocks && atomic_read(&F2FS_I(inode)->i_compr_blocks))
		reserved_blocks -= f2fs_release_reserved_blocks(inode, page_idx);
#endif

	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	up_write(&F2FS_I(inode)->i_mmap_sem);

	if (ret >= 0) {
		clear_inode_flag(inode, FI_COMPRESS_RELEASED);
		inode->i_ctime = current_time(inode);
		f2fs_mark_inode_dirty_sync(inode, true);
	}
unlock_inode:
	inode_unlock(inode);
out:
	mnt_drop_write_file(filp);

	if (ret >= 0) {
		ret = put_user(reserved_blocks, (u64 __user *)arg);
	} else if (reserved_blocks &&
			atomic_read(&F2FS_I(inode)->i_compr_blocks)) {
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_warn(sbi, "%s: partial blocks were released i_ino=%lx "
			"iblocks=%llu, reserved=%u, compr_blocks=%u, "
			"run fsck to fix.",
			__func__, inode->i_ino, inode->i_blocks,
			reserved_blocks,
			atomic_read(&F2FS_I(inode)->i_compr_blocks));
	}

	return ret;
}
#endif

static int f2fs_ioc_hw_getflags(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	unsigned int flags = fi->i_flags;

	flags &= F2FS_UNRM_ACM_FLMASK;

	return put_user(flags, (u32 __user *)arg);
}

static int f2fs_ioc_hw_setflags(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	unsigned int oldflags;
	unsigned int flags;
	int ret;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (get_user(flags, (int __user *)arg))
		return -EFAULT;

	if (flags & ~F2FS_UNRM_ACM_FLMASK)
		return -EOPNOTSUPP;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

	oldflags = fi->i_flags;
	flags = flags & F2FS_UNRM_ACM_FLMASK;
	flags |= oldflags & ~F2FS_UNRM_ACM_FLMASK;
	fi->i_flags = flags;

	inode->i_ctime = current_time(inode);
	f2fs_set_inode_flags(inode);
	f2fs_mark_inode_dirty_sync(inode, true);
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
static int f2fs_secure_erase(struct block_device *bdev, struct inode *inode,
		pgoff_t off, block_t block, block_t len, u32 flags)
{
	struct request_queue *q = bdev_get_queue(bdev);
	sector_t sector = SECTOR_FROM_BLOCK(block);
	sector_t nr_sects = SECTOR_FROM_BLOCK(len);
	int ret = 0;

	if (!q)
		return -ENXIO;

	if (flags & F2FS_TRIM_FILE_DISCARD)
		ret = blkdev_issue_discard(bdev, sector, nr_sects, GFP_NOFS,
						blk_queue_secure_erase(q) ?
						BLKDEV_DISCARD_SECURE : 0);

	if (!ret && (flags & F2FS_TRIM_FILE_ZEROOUT)) {
		if (IS_ENCRYPTED(inode))
			ret = fscrypt_zeroout_range(inode, off, block, len);
		else
			ret = blkdev_issue_zeroout(bdev, sector, nr_sects,
					GFP_NOFS, 0);
	}

	return ret;
}

static int f2fs_sec_trim_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct address_space *mapping = inode->i_mapping;
	struct block_device *prev_bdev = NULL;
	struct f2fs_sectrim_range range;
	pgoff_t index, pg_end, prev_index = 0;
	block_t prev_block = 0, len = 0;
	loff_t end_addr;
	bool to_end = false;
	int ret = 0;

	if (!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	if (copy_from_user(&range, (struct f2fs_sectrim_range __user *)arg,
				sizeof(range)))
		return -EFAULT;

	if (range.flags == 0 || (range.flags & ~F2FS_TRIM_FILE_MASK) ||
			!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (((range.flags & F2FS_TRIM_FILE_DISCARD) &&
			!f2fs_hw_support_discard(sbi)) ||
			((range.flags & F2FS_TRIM_FILE_ZEROOUT) &&
			 IS_ENCRYPTED(inode) && f2fs_is_multi_device(sbi)))
		return -EOPNOTSUPP;

	file_start_write(filp);
	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	if (f2fs_is_atomic_file(inode) || f2fs_is_compressed_inode(inode) ||
#else
	if (f2fs_is_atomic_file(inode) || f2fs_compressed_file(inode) ||
#endif
			range.start >= inode->i_size) {
		ret = -EINVAL;
		goto err;
	}

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		ret = f2fs_revoke_deduped_inode(inode, __func__);
		if (ret)
			goto err;
	}
#endif

	if (range.len == 0)
		goto err;

	if (inode->i_size - range.start > range.len) {
		end_addr = range.start + range.len;
	} else {
		end_addr = range.len == (u64)-1 ?
			sbi->sb->s_maxbytes : inode->i_size;
		to_end = true;
	}

	if (!IS_ALIGNED(range.start, F2FS_BLKSIZE) ||
			(!to_end && !IS_ALIGNED(end_addr, F2FS_BLKSIZE))) {
		ret = -EINVAL;
		goto err;
	}

	index = F2FS_BYTES_TO_BLK(range.start);
	pg_end = DIV_ROUND_UP(end_addr, F2FS_BLKSIZE);

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		goto err;

	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);

	ret = filemap_write_and_wait_range(mapping, range.start,
			to_end ? LLONG_MAX : end_addr - 1);
	if (ret)
		goto out;

	truncate_inode_pages_range(mapping, range.start,
			to_end ? -1 : end_addr - 1);

	while (index < pg_end) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;
		int i;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, index, LOOKUP_NODE);
		if (ret) {
			if (ret == -ENOENT) {
				index = f2fs_get_next_page_offset(&dn, index);
				continue;
			}
			goto out;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, pg_end - index);
		for (i = 0; i < count; i++, index++, dn.ofs_in_node++) {
			struct block_device *cur_bdev;
			block_t blkaddr = f2fs_data_blkaddr(&dn);

			if (!__is_valid_data_blkaddr(sbi, blkaddr))
				continue;

			if (!f2fs_is_valid_blkaddr(sbi, blkaddr,
						DATA_GENERIC_ENHANCE)) {
				ret = -EFSCORRUPTED;
				f2fs_put_dnode(&dn);
				goto out;
			}

			cur_bdev = f2fs_target_device(sbi, blkaddr, NULL);
			if (f2fs_is_multi_device(sbi)) {
				int di = f2fs_target_device_index(sbi, blkaddr);

				blkaddr -= FDEV(di).start_blk;
			}

			if (len) {
				if (prev_bdev == cur_bdev &&
						index == prev_index + len &&
						blkaddr == prev_block + len) {
					len++;
				} else {
					ret = f2fs_secure_erase(prev_bdev,
						inode, prev_index, prev_block,
						len, range.flags);
					if (ret) {
						f2fs_put_dnode(&dn);
						goto out;
					}

					len = 0;
				}
			}

			if (!len) {
				prev_bdev = cur_bdev;
				prev_index = index;
				prev_block = blkaddr;
				len = 1;
			}
		}

		f2fs_put_dnode(&dn);

		if (fatal_signal_pending(current)) {
			ret = -EINTR;
			goto out;
		}
		cond_resched();
	}

	if (len)
		ret = f2fs_secure_erase(prev_bdev, inode, prev_index,
				prev_block, len, range.flags);
out:
	up_write(&F2FS_I(inode)->i_mmap_sem);
	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
err:
	inode_unlock(inode);
	file_end_write(filp);

	return ret;
}
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
static int f2fs_ioc_get_compress_option(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_comp_option option;

	if (!f2fs_sb_has_compression(F2FS_I_SB(inode)))
		return -EOPNOTSUPP;

	inode_lock_shared(inode);

	if (!f2fs_compressed_file(inode)) {
		inode_unlock_shared(inode);
		return -ENODATA;
	}

	option.algorithm = F2FS_I(inode)->i_compress_algorithm;
	option.log_cluster_size = F2FS_I(inode)->i_log_cluster_size;

	inode_unlock_shared(inode);

	if (copy_to_user((struct f2fs_comp_option __user *)arg, &option,
				sizeof(option)))
		return -EFAULT;

	return 0;
}

static int f2fs_ioc_set_compress_option(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_comp_option option;
	int ret = 0;

	if (!f2fs_sb_has_compression(sbi))
		return -EOPNOTSUPP;

	if (!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	if (copy_from_user(&option, (struct f2fs_comp_option __user *)arg,
				sizeof(option)))
		return -EFAULT;

	if (!f2fs_compressed_file(inode) ||
			option.log_cluster_size < MIN_COMPRESS_LOG_SIZE ||
			option.log_cluster_size > MAX_COMPRESS_LOG_SIZE ||
			option.algorithm >= COMPRESS_MAX)
		return -EINVAL;

	file_start_write(filp);
	inode_lock(inode);

	if (f2fs_is_mmap_file(inode) || get_dirty_pages(inode)) {
		ret = -EBUSY;
		goto out;
	}

	if (inode->i_size != 0) {
		ret = -EFBIG;
		goto out;
	}

	F2FS_I(inode)->i_compress_algorithm = option.algorithm;
	F2FS_I(inode)->i_log_cluster_size = option.log_cluster_size;
	F2FS_I(inode)->i_cluster_size = 1 << option.log_cluster_size;
	f2fs_mark_inode_dirty_sync(inode, true);

	if (!f2fs_is_compress_backend_ready(inode))
		f2fs_warn(sbi, "compression algorithm is successfully set, "
			"but current kernel doesn't support this algorithm.");
out:
	inode_unlock(inode);
	file_end_write(filp);

	return ret;
}

static int redirty_blocks(struct inode *inode, pgoff_t page_idx, int len)
{
	DEFINE_READAHEAD(ractl, NULL, inode->i_mapping, page_idx);
	struct address_space *mapping = inode->i_mapping;
	struct page *page;
	pgoff_t redirty_idx = page_idx;
	int i, page_len = 0, ret = 0;

	page_cache_ra_unbounded(&ractl, len, 0);

	for (i = 0; i < len; i++, page_idx++) {
		page = read_cache_page(mapping, page_idx, NULL, NULL);
		if (IS_ERR(page)) {
			ret = PTR_ERR(page);
			break;
		}
		page_len++;
	}

	for (i = 0; i < page_len; i++, redirty_idx++) {
		page = find_lock_page(mapping, redirty_idx);
		if (!page) {
			ret = -ENOMEM;
			break;
		}
		set_page_dirty(page);
		f2fs_put_page(page, 1);
		f2fs_put_page(page, 0);
	}

	return ret;
}

static int f2fs_ioc_decompress_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	pgoff_t page_idx = 0, last_idx;
	unsigned int blk_per_seg = sbi->blocks_per_seg;
	int cluster_size = F2FS_I(inode)->i_cluster_size;
	int count, ret;

	if (!f2fs_sb_has_compression(sbi) ||
			F2FS_OPTION(sbi).compress_mode != COMPR_MODE_USER)
		return -EOPNOTSUPP;

	if (!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	file_start_write(filp);
	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_DEDUP
	if (f2fs_is_deduped_inode(inode)) {
		ret = -EACCES;
		goto out;
	}
#endif

	if (!f2fs_is_compress_backend_ready(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (f2fs_is_mmap_file(inode)) {
		ret = -EBUSY;
		goto out;
	}

	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	if (!atomic_read(&fi->i_compr_blocks))
		goto out;

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	count = last_idx - page_idx;
	while (count) {
		int len = min(cluster_size, count);

		ret = redirty_blocks(inode, page_idx, len);
		if (ret < 0)
			break;

		if (get_dirty_pages(inode) >= blk_per_seg)
			filemap_fdatawrite(inode->i_mapping);

		count -= len;
		page_idx += len;
	}

	if (!ret)
		ret = filemap_write_and_wait_range(inode->i_mapping, 0,
							LLONG_MAX);

	if (ret)
		f2fs_warn(sbi, "%s: The file might be partially decompressed (errno=%d). Please delete the file.",
			  __func__, ret);
out:
	inode_unlock(inode);
	file_end_write(filp);

	return ret;
}

static int f2fs_ioc_compress_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t page_idx = 0, last_idx;
	unsigned int blk_per_seg = sbi->blocks_per_seg;
	int cluster_size = F2FS_I(inode)->i_cluster_size;
	int count, ret;

	if (!f2fs_sb_has_compression(sbi) ||
			F2FS_OPTION(sbi).compress_mode != COMPR_MODE_USER)
		return -EOPNOTSUPP;

	if (!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	file_start_write(filp);
	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	f2fs_lock_op(sbi);
	f2fs_drop_extent_tree(inode);
	f2fs_unlock_op(sbi);
#endif

#ifdef CONFIG_F2FS_FS_DEDUP
	if (f2fs_is_deduped_inode(inode)) {
		ret = -EACCES;
		goto out;
	}
#endif

	if (!f2fs_is_compress_backend_ready(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (f2fs_is_mmap_file(inode)) {
		ret = -EBUSY;
		goto out;
	}

	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	set_inode_flag(inode, FI_ENABLE_COMPRESS);

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	down_write(&F2FS_I(inode)->i_gc_rwsem[READ]);
#endif
	inode_dio_wait(inode);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	up_write(&F2FS_I(inode)->i_gc_rwsem[READ]);
#endif
#endif

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	count = last_idx - page_idx;
	while (count) {
		int len = min(cluster_size, count);

		ret = redirty_blocks(inode, page_idx, len);
		if (ret < 0)
			break;

		if (get_dirty_pages(inode) >= blk_per_seg)
			filemap_fdatawrite(inode->i_mapping);

		count -= len;
		page_idx += len;
	}

	if (!ret)
		ret = filemap_write_and_wait_range(inode->i_mapping, 0,
							LLONG_MAX);

	clear_inode_flag(inode, FI_ENABLE_COMPRESS);

	if (ret)
		f2fs_warn(sbi, "%s: The file might be partially compressed (errno=%d). Please delete the file.",
			  __func__, ret);
out:
	inode_unlock(inode);
	file_end_write(filp);

	return ret;
}

#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
static void f2fs_compress_imonitor_send(
		struct f2fs_sb_info *sbi, struct inode *inode,
		unsigned int reserved_blocks, int reserve_result,
		enum RELEASE_WRITE_SCENCE scene)
{
	struct hiview_hievent *hi_event = NULL;
	unsigned int ret = 0;

	hi_event = hiview_hievent_create(COMPRESS_BD_ID);
	if (!hi_event) {
		f2fs_warn(sbi, "%s:create eventobj failed", __func__);
		return;
	}

	ret = ret | hiview_hievent_put_integral(hi_event,
			"RevExeScence", scene);
	ret = ret | hiview_hievent_put_integral(hi_event,
			"RevExeFileIno", inode->i_ino);
	ret = ret | hiview_hievent_put_integral(hi_event,
			"RevAppliedSize", reserved_blocks);
	ret = ret | hiview_hievent_put_integral(hi_event,
			"RevExeResult", reserve_result);
	if (ret)
		goto out;

	ret = hiview_hievent_report(hi_event);
out:
	if (ret <= 0)
		f2fs_warn(sbi, "%s scene[%u] send hievent failed, err:%d", __func__, scene, ret);
	else
		f2fs_info(sbi, "%s scene[%u] send hievent ok", __func__, scene);
	hiview_hievent_destroy(hi_event);
}

static int f2fs_get_reserved_blocks(struct inode *inode,
		pgoff_t last_idx, pgoff_t *page_idx, unsigned int *reserved_blocks)
{
	int ret = 0;
	while ((*page_idx) < last_idx) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, (*page_idx), LOOKUP_NODE);
		if (ret) {
			if (ret == -ENOENT) {
				(*page_idx) = f2fs_get_next_page_offset(&dn, (*page_idx));
				ret = 0;
				continue;
			}
			return ret;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, last_idx - (*page_idx));
		count = round_up(count, F2FS_I(inode)->i_cluster_size);

		ret = reserve_compress_blocks(&dn, count);

		f2fs_put_dnode(&dn);

		if (ret < 0)
			return ret;

		(*page_idx) += count;
		(*reserved_blocks) += ret;
	}

	return ret;
}

/*
 * need inode_lock && FI_COMPRESS_RELEASED by caller
 */
int f2fs_reserve_compress_blocks_internal(
		struct inode *inode, enum RELEASE_WRITE_SCENCE scene)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t page_idx = 0, last_idx;
	unsigned int reserved_blocks = 0;
	int ret = 0;

	ret = f2fs_dquot_initialize(inode);
	if (ret)
		goto out;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);
	ret = f2fs_get_reserved_blocks(inode, last_idx, &page_idx, &reserved_blocks);
	if (ret < 0 && reserved_blocks && atomic_read(&F2FS_I(inode)->i_compr_blocks))
		reserved_blocks -= f2fs_release_reserved_blocks(inode, page_idx);

	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	up_write(&F2FS_I(inode)->i_mmap_sem);

	if (ret >= 0) {
		clear_inode_flag(inode, FI_COMPRESS_RELEASED);
		inode->i_ctime = current_time(inode);
		f2fs_mark_inode_dirty_sync(inode, true);
	} else {
		f2fs_warn(sbi, "%s: reserve released blocks failed ret=%d, "
			"i_ino=%lx, iblocks=%llu, reserved=%u, compr_blocks=%u\n",
			__func__, ret, inode->i_ino, inode->i_blocks,
			reserved_blocks, atomic_read(&F2FS_I(inode)->i_compr_blocks));

		if (reserved_blocks && atomic_read(&F2FS_I(inode)->i_compr_blocks))
			set_sbi_flag(sbi, SBI_NEED_FSCK);
	}

out:
	ret = (ret >= 0 ? 0 : ret);
	if (scene != DECOMPRESS)
		f2fs_compress_imonitor_send(sbi, inode, reserved_blocks, ret, scene);
	return ret;
}

static int valid_compress_blocks(struct dnode_of_data *dn, pgoff_t count)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(dn->inode);
	unsigned int valid_blocks = 0;
	int cluster_size = F2FS_I(dn->inode)->i_cluster_size;
	block_t blkaddr;
	int i;

	for (i = 0; i < count; i++) {
		blkaddr = data_blkaddr(dn->inode, dn->node_page,
						dn->ofs_in_node + i);
		if (!__is_valid_data_blkaddr(sbi, blkaddr))
			continue;
		if (unlikely(!f2fs_is_valid_blkaddr(sbi, blkaddr,
					DATA_GENERIC)))
			return -EFSCORRUPTED;
	}

	while (count) {
		int compr_blocks = 0;

		for (i = 0; i < cluster_size; i++, dn->ofs_in_node++) {
			blkaddr = f2fs_data_blkaddr(dn);

			if (i == 0) {
				if (blkaddr == COMPRESS_ADDR)
					continue;
				dn->ofs_in_node += cluster_size;
				goto next;
			}

			if (__is_valid_data_blkaddr(sbi, blkaddr))
				compr_blocks++;
		}

		valid_blocks += compr_blocks;
next:
		count -= cluster_size;
	}

	return valid_blocks;
}

static int f2fs_ioc_get_saved_blocks(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	__u64 decompr_blocks = 0;
	__u64 valid_blocks = 0;
	pgoff_t page_idx = 0;
	pgoff_t last_idx;
	int ret;
	int writecount;

	if (!f2fs_sb_has_compression(F2FS_I_SB(inode)))
		return -EOPNOTSUPP;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	inode_lock(inode);

	if (!atomic_read(&F2FS_I(inode)->i_compr_blocks) &&
		!is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		inode_unlock(inode);
		return put_user(0, (u64 __user *)arg);
	}

	writecount = atomic_read(&inode->i_writecount);
	if ((filp->f_mode & FMODE_WRITE && writecount != 1) ||
			(!(filp->f_mode & FMODE_WRITE) && writecount)) {
		ret = -EBUSY;
		goto out;
	}

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	while (page_idx < last_idx) {
		struct dnode_of_data dn;
		pgoff_t end_offset, count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret = f2fs_get_dnode_of_data(&dn, page_idx, LOOKUP_NODE);
		if (ret) {
			if (ret == -ENOENT) {
				page_idx = f2fs_get_next_page_offset(&dn, page_idx);
				ret = 0;
				continue;
			}
			break;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		count = min(end_offset - dn.ofs_in_node, last_idx - page_idx);
		count = roundup(count, F2FS_I(inode)->i_cluster_size);

		ret = valid_compress_blocks(&dn, count);

		f2fs_put_dnode(&dn);

		if (ret < 0)
			break;

		page_idx += count;
		valid_blocks += ret;
	}

out:
	inode_unlock(inode);

	decompr_blocks = (i_size_read(inode) + PAGE_SIZE - 1) / PAGE_SIZE;
	if (decompr_blocks >= valid_blocks)
		ret = put_user(decompr_blocks - valid_blocks, (u64 __user *)arg);
	else
		f2fs_warn(sbi, "%s: decompressed blocks=%u or valid blocks=%u errror.",
			__func__, decompr_blocks, valid_blocks);

	return ret;
}

static int f2fs_ioc_get_released_status(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	int released = 0;

	inode_lock(inode);
	released = is_inode_flag_set(inode, FI_COMPRESS_RELEASED);
	inode_unlock(inode);

	return put_user(released, (u8 __user *)arg);
}

static int f2fs_ioc_get_compressed_status(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	int compressed = 0;

	inode_lock(inode);

	if (F2FS_OPTION(F2FS_I_SB(inode)).compress_mode == COMPR_MODE_FS) {
		compressed = f2fs_compressed_file(inode);
		goto out;
	}

	if (!atomic_read(&F2FS_I(inode)->i_compr_blocks) &&
		!is_inode_flag_set(inode, FI_COMPRESS_RELEASED))
		compressed = 0;
	else
		compressed = 1;
out:
	inode_unlock(inode);
	return put_user(compressed, (u8 __user *)arg);
}

static int f2fs_ioc_compress_release_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t page_idx = 0;
	pgoff_t last_idx;
	unsigned int blk_per_seg = sbi->blocks_per_seg;
	int cluster_size = F2FS_I(inode)->i_cluster_size;
	int count, ret;
	unsigned int released_blocks = 0;
	int ret2 = 0;

	if (!f2fs_sb_has_compression(sbi) ||
			F2FS_OPTION(sbi).compress_mode != COMPR_MODE_USER)
		return -EOPNOTSUPP;

	if (!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	if (f2fs_has_inline_data(inode)) {
		f2fs_warn(sbi, "inline data [%lu], size:%lu.", inode->i_ino, inode->i_blocks);
		return -EOPNOTSUPP;
	}

	ret = f2fs_dquot_initialize(inode);
	if (ret)
		return ret;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	file_start_write(filp);
	inode_lock(inode);

	f2fs_lock_op(sbi);
	f2fs_drop_extent_tree(inode);
	f2fs_unlock_op(sbi);

	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		ret = 0;
		goto out;
	}

#ifdef CONFIG_F2FS_FS_DEDUP
	if (f2fs_is_deduped_inode(inode)) {
		ret = -EACCES;
		goto out;
	}
#endif

	if (!f2fs_is_compress_backend_ready(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (f2fs_is_atomic_file(inode)) {
		f2fs_err(sbi, "compress fails, atomic file [%lu].", inode->i_ino);
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (f2fs_is_pinned_file(inode)) {
		f2fs_err(sbi, "compress fails, pin file [%lu].", inode->i_ino);
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (f2fs_is_mmap_file(inode)) {
		ret = -EBUSY;
		goto out;
	}

	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	set_inode_flag(inode, FI_ENABLE_COMPRESS);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	down_write(&F2FS_I(inode)->i_gc_rwsem[READ]);
#endif
	inode_dio_wait(inode);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	up_write(&F2FS_I(inode)->i_gc_rwsem[READ]);
#endif

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	count = last_idx - page_idx;
	while (count) {
		int len = min(cluster_size, count);

		ret = redirty_blocks(inode, page_idx, len);
		if (ret < 0)
			break;

		if (get_dirty_pages(inode) >= blk_per_seg)
			filemap_fdatawrite(inode->i_mapping);

		count -= len;
		page_idx += len;
	}

	if (!ret)
		ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);

	clear_inode_flag(inode, FI_ENABLE_COMPRESS);

	if (ret) {
		f2fs_warn(sbi, "%s: The file might be partially compressed (errno=%d). "
			"Please delete the file.", __func__, ret);
		goto out;
	}

	set_inode_flag(inode, FI_COMPRESS_RELEASED);
	inode->i_ctime = current_time(inode);
	f2fs_mark_inode_dirty_sync(inode, true);

	if (!atomic_read(&F2FS_I(inode)->i_compr_blocks))
		goto out;

	down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	down_write(&F2FS_I(inode)->i_mmap_sem);
	page_idx = 0;

	while (page_idx < last_idx) {
		struct dnode_of_data dn;
		pgoff_t end_offset, cur_count;

		set_new_dnode(&dn, inode, NULL, NULL, 0);
		ret2 = f2fs_get_dnode_of_data(&dn, page_idx, LOOKUP_NODE);
		if (ret2) {
			if (ret2 == -ENOENT) {
				page_idx = f2fs_get_next_page_offset(&dn,
								page_idx);
				ret2 = 0;
				continue;
			}
			break;
		}

		end_offset = ADDRS_PER_PAGE(dn.node_page, inode);
		cur_count = min(end_offset - dn.ofs_in_node, last_idx - page_idx);
		cur_count = round_up(cur_count, F2FS_I(inode)->i_cluster_size);

		ret2 = release_compress_blocks(&dn, cur_count);

		f2fs_put_dnode(&dn);

		if (ret2 < 0)
			break;

		page_idx += cur_count;
		released_blocks += ret2;
	}

	if (atomic_read(&F2FS_I(inode)->i_compr_blocks)) {
		f2fs_warn(sbi, "%s, potential error with i_compr_blocks:%d, ino:%lu",
			  __func__, atomic_read(&F2FS_I(inode)->i_compr_blocks), inode->i_ino);
		atomic_set(&F2FS_I(inode)->i_compr_blocks, 0);
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_set_need_fsck_report();
	}

	up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
	up_write(&F2FS_I(inode)->i_mmap_sem);

out:
	inode_unlock(inode);
	file_end_write(filp);
	if (ret)
		return ret;

	if (ret2 >= 0) {
		ret2 = put_user(released_blocks, (u64 __user *)arg);
	} else if (released_blocks &&
			atomic_read(&F2FS_I(inode)->i_compr_blocks)) {
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_warn(sbi, "%s: partial blocks were released i_ino=%lx "
			"iblocks=%llu, released=%u, compr_blocks=%u, "
			"run fsck to fix.",
			__func__, inode->i_ino, inode->i_blocks,
			released_blocks,
			atomic_read(&F2FS_I(inode)->i_compr_blocks));
	}

	return ret2;
}

static int f2fs_check_decompress_file(struct file *filp)
{
	int ret = 0;
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (!f2fs_sb_has_compression(sbi) ||
			F2FS_OPTION(sbi).compress_mode != COMPR_MODE_USER)
		return -EOPNOTSUPP;

	if (!(filp->f_mode & FMODE_WRITE))
		return -EBADF;

	if (!f2fs_compressed_file(inode))
		return -EINVAL;

	if (f2fs_has_inline_data(inode)) {
		f2fs_warn(sbi, "inline data [%lu], size:%lu.", inode->i_ino, inode->i_blocks);
		return -EOPNOTSUPP;
	}

	ret = f2fs_dquot_initialize(inode);
	return ret;
}

static int f2fs_redirty_and_write_file(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	pgoff_t page_idx = 0;
	pgoff_t last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);
	int cluster_size = F2FS_I(inode)->i_cluster_size;
	unsigned int blk_per_seg = sbi->blocks_per_seg;
	int ret = 0;
	int count = last_idx - page_idx;

	while (count) {
		int len = min(cluster_size, count);
		ret = redirty_blocks(inode, page_idx, len);
		if (ret < 0)
			break;

		if (get_dirty_pages(inode) >= blk_per_seg)
			filemap_fdatawrite(inode->i_mapping);

		count -= len;
		page_idx += len;
	}

	if (!ret)
		ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);

	if (ret)
		f2fs_warn(sbi, "%s: i_ino:%d decompress err:%d.", __func__, inode->i_ino, ret);
	return ret;
}

static int f2fs_ioc_reserve_decompress_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	int ret;

	ret = f2fs_check_decompress_file(filp);
	if (ret)
		return ret;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	file_start_write(filp);
	inode_lock(inode);

#ifdef CONFIG_F2FS_FS_DEDUP
	if (f2fs_is_deduped_inode(inode)) {
		ret = -EACCES;
		goto out;
	}
#endif

	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		ret = f2fs_reserve_compress_blocks_internal(inode, DECOMPRESS);
		if (ret)
			goto out;
	}

	if (!f2fs_is_compress_backend_ready(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (f2fs_is_mmap_file(inode)) {
		ret = -EBUSY;
		goto out;
	}

	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		goto out;

	if (atomic_read(&fi->i_compr_blocks))
		ret = f2fs_redirty_and_write_file(inode);
out:
	inode_unlock(inode);
	file_end_write(filp);

	return ret;
}

static int f2fs_decompress_inode_internal(struct inode *inode)
{
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	unsigned int blk_per_seg = sbi->blocks_per_seg;
	int cluster_size = F2FS_I(inode)->i_cluster_size;
	int ret = 0;
	pgoff_t page_idx = 0;
	pgoff_t last_idx;
	int count;

	ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);
	if (ret)
		return ret;

	if (!atomic_read(&fi->i_compr_blocks))
		return ret;

	last_idx = DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE);

	count = last_idx - page_idx;
	while (count) {
		int len = min(cluster_size, count);

		ret = redirty_blocks(inode, page_idx, len);
		if (ret < 0)
			break;

		if (get_dirty_pages(inode) >= blk_per_seg)
			filemap_fdatawrite(inode->i_mapping);

		count -= len;
		page_idx += len;
	}

	if (!ret)
		ret = filemap_write_and_wait_range(inode->i_mapping, 0, LLONG_MAX);

	return ret;
}

long f2fs_decompress_file_internal(struct file *file,
		enum RELEASE_WRITE_SCENCE scene, int mode)
{
	struct inode *inode = file_inode(file);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	long ret = 0;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;

	ret = f2fs_dquot_initialize(inode);
	if (ret)
		return ret;

	f2fs_balance_fs(F2FS_I_SB(inode), true);

	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
		ret = f2fs_reserve_compress_blocks_internal(inode, scene);
		if (ret) {
			f2fs_warn(sbi, "%s mode:%d, reserve failed:%d", __func__, mode, ret);
			return ret;
		}
	}

#ifdef CONFIG_F2FS_FS_DEDUP
	if (f2fs_is_deduped_inode(inode)) {
		ret = -EACCES;
		f2fs_warn(sbi, "%s dedup file", __func__);
		return ret;
	}
#endif

	if (f2fs_is_mmap_file(inode)) {
		ret = -EBUSY;
		return ret;
	}

	ret = f2fs_decompress_inode_internal(inode);
	if (ret)
		f2fs_warn(sbi, "%s mode:%d decompressed err:%d", __func__, mode, ret);
	else
		f2fs_info(sbi, "%s mode:%d decompressed succ", __func__, mode);

	return ret;
}
#endif /* CONFIG_F2FS_FS_COMPRESSION_EX */
#endif

/*
 * NOTE: when we try to define a new ioctl cmd, which will
 * modify inode data, we need to do revoke for deduped inode.
 */
static long __f2fs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case FS_IOC_GETFLAGS:
		return f2fs_ioc_getflags(filp, arg);
	case FS_IOC_SETFLAGS:
		return f2fs_ioc_setflags(filp, arg);
	case FS_IOC_GETVERSION:
		return f2fs_ioc_getversion(filp, arg);
	case F2FS_IOC_START_ATOMIC_WRITE:
		return f2fs_ioc_start_atomic_write(filp);
	case F2FS_IOC_COMMIT_ATOMIC_WRITE:
		return f2fs_ioc_commit_atomic_write(filp);
	case F2FS_IOC_START_VOLATILE_WRITE:
		return f2fs_ioc_start_volatile_write(filp);
	case F2FS_IOC_RELEASE_VOLATILE_WRITE:
		return f2fs_ioc_release_volatile_write(filp);
	case F2FS_IOC_ABORT_VOLATILE_WRITE:
		return f2fs_ioc_abort_volatile_write(filp);
	case F2FS_IOC_SHUTDOWN:
		return f2fs_ioc_shutdown(filp, arg);
	case FITRIM:
		return f2fs_ioc_fitrim(filp, arg);
	case FS_IOC_SET_ENCRYPTION_POLICY:
		return f2fs_ioc_set_encryption_policy(filp, arg);
	case FS_IOC_GET_ENCRYPTION_POLICY:
		return f2fs_ioc_get_encryption_policy(filp, arg);
	case FS_IOC_GET_ENCRYPTION_PWSALT:
		return f2fs_ioc_get_encryption_pwsalt(filp, arg);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	case FS_IOC_GET_ENCRYPTION_POLICY_EX:
		return f2fs_ioc_get_encryption_policy_ex(filp, arg);
	case FS_IOC_ADD_ENCRYPTION_KEY:
		return f2fs_ioc_add_encryption_key(filp, arg);
	case FS_IOC_REMOVE_ENCRYPTION_KEY:
		return f2fs_ioc_remove_encryption_key(filp, arg);
	case FS_IOC_REMOVE_ENCRYPTION_KEY_ALL_USERS:
		return f2fs_ioc_remove_encryption_key_all_users(filp, arg);
	case FS_IOC_GET_ENCRYPTION_KEY_STATUS:
		return f2fs_ioc_get_encryption_key_status(filp, arg);
	case FS_IOC_GET_ENCRYPTION_NONCE:
		return f2fs_ioc_get_encryption_nonce(filp, arg);
#endif
	case F2FS_IOC_GARBAGE_COLLECT:
		return f2fs_ioc_gc(filp, arg);
	case F2FS_IOC_GARBAGE_COLLECT_RANGE:
		return f2fs_ioc_gc_range(filp, arg);
	case F2FS_IOC_WRITE_CHECKPOINT:
		return f2fs_ioc_write_checkpoint(filp, arg);
	case F2FS_IOC_DEFRAGMENT:
		return f2fs_ioc_defragment(filp, arg);
	case F2FS_IOC_MOVE_RANGE:
		return f2fs_ioc_move_range(filp, arg);
	case F2FS_IOC_FLUSH_DEVICE:
		return f2fs_ioc_flush_device(filp, arg);
	case F2FS_IOC_GET_FEATURES:
		return f2fs_ioc_get_features(filp, arg);
	case FS_IOC_FSGETXATTR:
		return f2fs_ioc_fsgetxattr(filp, arg);
	case FS_IOC_FSSETXATTR:
		return f2fs_ioc_fssetxattr(filp, arg);
	case F2FS_IOC_GET_PIN_FILE:
		return f2fs_ioc_get_pin_file(filp, arg);
	case F2FS_IOC_SET_PIN_FILE:
		return f2fs_ioc_set_pin_file(filp, arg);
	case F2FS_IOC_PRECACHE_EXTENTS:
		return f2fs_ioc_precache_extents(filp, arg);
	case F2FS_IOC_RESIZE_FS:
		return f2fs_ioc_resize_fs(filp, arg);
#ifdef CONFIG_HP_FILE
	case F2FS_IOC_HP_PREALLOC:
#ifdef CONFIG_F2FS_FS_DEDUP
		return f2fs_ioc_set_hp_file_wrapper(filp, arg);
#else
		return f2fs_ioc_set_hp_file(filp, arg);
#endif
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	case FS_IOC_ENABLE_VERITY:
		return f2fs_ioc_enable_verity(filp, arg);
	case FS_IOC_MEASURE_VERITY:
		return f2fs_ioc_measure_verity(filp, arg);
	case FS_IOC_GETFSLABEL:
		return f2fs_ioc_getfslabel(filp, arg);
	case FS_IOC_SETFSLABEL:
		return f2fs_ioc_setfslabel(filp, arg);
#endif
	case F2FS_IOC_HW_GETFLAGS:
		return f2fs_ioc_hw_getflags(filp, arg);
	case F2FS_IOC_HW_SETFLAGS:
		return f2fs_ioc_hw_setflags(filp, arg);
#ifdef CONFIG_F2FS_TURBO_ZONE
	case F2FS_IOC_GET_TZ_KEY_FILE:
		return f2fs_ioc_get_turbo_file(filp, arg, FI_TZ_KEY_FILE);
	case F2FS_IOC_SET_TZ_KEY_FILE:
		return f2fs_ioc_set_turbo_file(filp, arg, FI_TZ_KEY_FILE);
	case F2FS_IOC_GET_TZ_AGING_FILE:
		return f2fs_ioc_get_turbo_file(filp, arg, FI_TZ_AGING_FILE);
	case F2FS_IOC_SET_TZ_AGING_FILE:
		return f2fs_ioc_set_turbo_file(filp, arg, FI_TZ_AGING_FILE);
	case F2FS_IOC_GET_TZ_FREE_BLOCKS:
		return f2fs_ioc_get_turbo_free_blocks(filp, arg);
	case F2FS_IOC_GET_TZ_STATUS:
		return f2fs_ioc_get_turbo_status(filp, arg);
	case F2FS_IOC_SET_TZ_RETURN:
		return f2fs_ioc_set_turbo_return(filp, arg);
	case F2FS_IOC_MIGRATE_FILE:
		return f2fs_ioc_migrate_file(filp, arg);
	case F2FS_IOC_SET_TZ_FORCE_CLOSE:
		return f2fs_ioc_set_tz_force_close(filp);
	case F2FS_IOC_GET_TZ_SPACE_INFO:
		return f2fs_ioc_get_tz_space_info(filp, arg);
	case F2FS_IOC_GET_TZ_INFO:
		return f2fs_ioc_get_tz_info(filp, arg);
#endif
#ifdef CONFIG_F2FS_TURBO_ZONE_V2
	case F2FS_IOC_GET_TZ_SLC_INFO:
		return f2fs_ioc_get_tz_slc_info(filp, arg);
#ifdef CONFIG_F2FS_CHECK_FS
	case F2FS_IOC_GET_TZ_BLK_INFO:
		return f2fs_ioc_get_tz_blk_info(filp, arg);
#endif
#endif
#ifdef CONFIG_SDP_ENCRYPTION
	case F2FS_IOC_SET_SDP_ENCRYPTION_POLICY:
		return f2fs_ioc_set_sdp_encryption_policy(filp, arg);
	case F2FS_IOC_GET_SDP_ENCRYPTION_POLICY:
		return f2fs_ioc_get_sdp_encryption_policy(filp, arg);
	case F2FS_IOC_GET_ENCRYPTION_POLICY_TYPE:
		return f2fs_ioc_get_encryption_policy_type(filp, arg);
#endif
#ifdef CONFIG_HWDPS
	case F2FS_IOC_SET_DPS_ENCRYPTION_POLICY:
		return f2fs_ioc_set_dps_encryption_policy(filp, arg);
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	case FS_IOC_READ_VERITY_METADATA:
		return f2fs_ioc_read_verity_metadata(filp, arg);
	case F2FS_IOC_GET_COMPRESS_BLOCKS:
		return f2fs_get_compress_blocks(filp, arg);
	case F2FS_IOC_RELEASE_COMPRESS_BLOCKS:
		return f2fs_release_compress_blocks(filp, arg);
	case F2FS_IOC_RESERVE_COMPRESS_BLOCKS:
		return f2fs_reserve_compress_blocks(filp, arg);
	case F2FS_IOC_SEC_TRIM_FILE:
		return f2fs_sec_trim_file(filp, arg);
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	case F2FS_IOC_GET_SAVED_BLOCKS:
		return f2fs_ioc_get_saved_blocks(filp, arg);
	case F2FS_IOC_GET_RELEASED_STATUS:
		return f2fs_ioc_get_released_status(filp, arg);
	case F2FS_IOC_COMPRESS_RELEASE_FILE:
		return f2fs_ioc_compress_release_file(filp, arg);
	case F2FS_IOC_GET_COMPRESSED_STATUS:
		return f2fs_ioc_get_compressed_status(filp, arg);
	case F2FS_IOC_RESERVE_DECOMPRESS_FILE:
		return f2fs_ioc_reserve_decompress_file(filp, arg);
#endif
	case F2FS_IOC_GET_COMPRESS_OPTION:
		return f2fs_ioc_get_compress_option(filp, arg);
	case F2FS_IOC_SET_COMPRESS_OPTION:
		return f2fs_ioc_set_compress_option(filp, arg);
	case F2FS_IOC_DECOMPRESS_FILE:
		return f2fs_ioc_decompress_file(filp, arg);
	case F2FS_IOC_COMPRESS_FILE:
		return f2fs_ioc_compress_file(filp, arg);
#endif
#ifdef CONFIG_F2FS_FS_DEDUP
	case F2FS_IOC_DEDUP_CREATE:
		return f2fs_ioc_create_layered_inode(filp, arg);
	case F2FS_IOC_DEDUP_FILE:
		return f2fs_ioc_dedup_file(filp, arg);
	case F2FS_IOC_DEDUP_REVOKE:
		return f2fs_ioc_dedup_revoke(filp, arg);
	case F2FS_IOC_CLONE_FILE:
		return f2fs_ioc_clone_file(filp, arg);
	case F2FS_IOC_MODIFY_CHECK:
		return f2fs_ioc_modify_check(filp, arg);
	case F2FS_IOC_DEDUP_PERM_CHECK:
		return f2fs_ioc_dedup_permission_check(filp, arg);
	case F2FS_IOC_DEDUP_GET_FILE_INFO:
		return f2fs_ioc_get_dedupd_file_info(filp, arg);
	case F2FS_IOC_DEDUP_GET_SYS_INFO:
		return f2fs_ioc_get_dedup_sysinfo(filp, arg);
#endif
	default:
		return -ENOTTY;
	}
}

long f2fs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (unlikely(f2fs_cp_error(F2FS_I_SB(file_inode(filp)))))
		return -EIO;
	if (!f2fs_is_checkpoint_ready(F2FS_I_SB(file_inode(filp))))
		return -ENOSPC;

	return __f2fs_ioctl(filp, cmd, arg);
}

static ssize_t f2fs_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	int ret;

	if (!f2fs_is_compress_backend_ready(inode))
		return -EOPNOTSUPP;

	ret = generic_file_read_iter(iocb, iter);

	if (ret > 0)
		f2fs_update_iostat(F2FS_I_SB(inode), APP_READ_IO, ret);

	return ret;
}

static ssize_t f2fs_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	ssize_t ret;

	if (unlikely(f2fs_cp_error(F2FS_I_SB(inode)))) {
		ret = -EIO;
		goto out;
	}

	if (!f2fs_is_compress_backend_ready(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (iocb->ki_flags & IOCB_NOWAIT) {
		if (!inode_trylock(inode)) {
			ret = -EAGAIN;
			goto out;
		}
	} else {
		inode_lock(inode);
	}

	if (unlikely(IS_IMMUTABLE(inode))) {
		ret = -EPERM;
		goto unlock;
	}

	if (is_inode_flag_set(inode, FI_COMPRESS_RELEASED)) {
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
		ret = f2fs_reserve_compress_blocks_internal(inode, FILE_WRITE_ITER);
		if (ret)
			goto unlock;
#else
		ret = -EPERM;
		goto unlock;
#endif
	}

#ifdef CONFIG_F2FS_FS_DEDUP
	mark_file_modified(inode);
	if (f2fs_is_outer_inode(inode)) {
		ret = f2fs_revoke_deduped_inode(inode, __func__);
		if (ret) {
			inode_unlock(inode);
			goto out;
		}
	}
#endif

	ret = generic_write_checks(iocb, from);
	if (ret > 0) {
		bool preallocated = false;
		size_t target_size = 0;
		int err;

		if (iov_iter_fault_in_readable(from, iov_iter_count(from)))
			set_inode_flag(inode, FI_NO_PREALLOC);

		if ((iocb->ki_flags & IOCB_NOWAIT)) {
			if (!f2fs_overwrite_io(inode, iocb->ki_pos,
						iov_iter_count(from)) ||
				f2fs_has_inline_data(inode) ||
				f2fs_force_buffered_io(inode, iocb, from)) {
				clear_inode_flag(inode, FI_NO_PREALLOC);
				inode_unlock(inode);
				ret = -EAGAIN;
				goto out;
			}
			goto write;
		}

		if (is_inode_flag_set(inode, FI_NO_PREALLOC))
			goto write;

		if (iocb->ki_flags & IOCB_DIRECT) {
			/*
			 * Convert inline data for Direct I/O before entering
			 * f2fs_direct_IO().
			 */
			err = f2fs_convert_inline_inode(inode);
			if (err)
				goto out_err;
			/*
			 * If force_buffere_io() is true, we have to allocate
			 * blocks all the time, since f2fs_direct_IO will fall
			 * back to buffered IO.
			 */
			if (!f2fs_force_buffered_io(inode, iocb, from) &&
					allow_outplace_dio(inode, iocb, from))
				goto write;
		}
		preallocated = true;
		target_size = iocb->ki_pos + iov_iter_count(from);

		err = f2fs_preallocate_blocks(iocb, from);
		if (err) {
out_err:
			clear_inode_flag(inode, FI_NO_PREALLOC);
			inode_unlock(inode);
			ret = err;
			goto out;
		}
write:
		ret = __generic_file_write_iter(iocb, from);
		clear_inode_flag(inode, FI_NO_PREALLOC);

		/* if we couldn't write data, we should deallocate blocks. */
		if (preallocated && i_size_read(inode) < target_size) {
			down_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
			down_write(&F2FS_I(inode)->i_mmap_sem);
			f2fs_truncate(inode);
			up_write(&F2FS_I(inode)->i_mmap_sem);
			up_write(&F2FS_I(inode)->i_gc_rwsem[WRITE]);
		}

		if (ret > 0)
			f2fs_update_iostat(F2FS_I_SB(inode), APP_WRITE_IO, ret);
	}
unlock:
	inode_unlock(inode);
out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	trace_f2fs_file_write_iter(inode, iocb->ki_pos,
					iov_iter_count(from), ret);
#endif
	if (ret > 0)
		ret = generic_write_sync(iocb, ret);
	return ret;
}

#ifdef CONFIG_COMPAT
struct compat_f2fs_gc_range {
	u32 sync;
	compat_u64 start;
	compat_u64 len;
};
#define F2FS_IOC32_GARBAGE_COLLECT_RANGE	_IOW(F2FS_IOCTL_MAGIC, 11,\
						struct compat_f2fs_gc_range)

static int f2fs_compat_ioc_gc_range(struct file *file, unsigned long arg)
{
	struct compat_f2fs_gc_range __user *urange;
	struct f2fs_gc_range range;
	int err;

	urange = compat_ptr(arg);
	err = get_user(range.sync, &urange->sync);
	err |= get_user(range.start, &urange->start);
	err |= get_user(range.len, &urange->len);
	if (err)
		return -EFAULT;

	return __f2fs_ioc_gc_range(file, &range);
}

struct compat_f2fs_move_range {
	u32 dst_fd;
	compat_u64 pos_in;
	compat_u64 pos_out;
	compat_u64 len;
};
#define F2FS_IOC32_MOVE_RANGE		_IOWR(F2FS_IOCTL_MAGIC, 9,	\
					struct compat_f2fs_move_range)

static int f2fs_compat_ioc_move_range(struct file *file, unsigned long arg)
{
	struct compat_f2fs_move_range __user *urange;
	struct f2fs_move_range range;
	int err;

	urange = compat_ptr(arg);
	err = get_user(range.dst_fd, &urange->dst_fd);
	err |= get_user(range.pos_in, &urange->pos_in);
	err |= get_user(range.pos_out, &urange->pos_out);
	err |= get_user(range.len, &urange->len);
	if (err)
		return -EFAULT;

	return __f2fs_ioc_move_range(file, &range);
}

long f2fs_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (unlikely(f2fs_cp_error(F2FS_I_SB(file_inode(file)))))
		return -EIO;
	if (!f2fs_is_checkpoint_ready(F2FS_I_SB(file_inode(file))))
		return -ENOSPC;

	switch (cmd) {
	case FS_IOC32_GETFLAGS:
		cmd = FS_IOC_GETFLAGS;
		break;
	case FS_IOC32_SETFLAGS:
		cmd = FS_IOC_SETFLAGS;
		break;
	case FS_IOC32_GETVERSION:
		cmd = FS_IOC_GETVERSION;
		break;
	case F2FS_IOC32_GARBAGE_COLLECT_RANGE:
		return f2fs_compat_ioc_gc_range(file, arg);
	case F2FS_IOC32_MOVE_RANGE:
		return f2fs_compat_ioc_move_range(file, arg);
	case F2FS_IOC_START_ATOMIC_WRITE:
	case F2FS_IOC_COMMIT_ATOMIC_WRITE:
	case F2FS_IOC_START_VOLATILE_WRITE:
	case F2FS_IOC_RELEASE_VOLATILE_WRITE:
	case F2FS_IOC_ABORT_VOLATILE_WRITE:
	case F2FS_IOC_SHUTDOWN:
	case FITRIM:
	case FS_IOC_SET_ENCRYPTION_POLICY:
	case FS_IOC_GET_ENCRYPTION_PWSALT:
	case FS_IOC_GET_ENCRYPTION_POLICY:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	case FS_IOC_GET_ENCRYPTION_POLICY_EX:
	case FS_IOC_ADD_ENCRYPTION_KEY:
	case FS_IOC_REMOVE_ENCRYPTION_KEY:
	case FS_IOC_REMOVE_ENCRYPTION_KEY_ALL_USERS:
	case FS_IOC_GET_ENCRYPTION_KEY_STATUS:
	case FS_IOC_GET_ENCRYPTION_NONCE:
#endif
	case F2FS_IOC_GARBAGE_COLLECT:
	case F2FS_IOC_WRITE_CHECKPOINT:
	case F2FS_IOC_DEFRAGMENT:
	case F2FS_IOC_FLUSH_DEVICE:
	case F2FS_IOC_GET_FEATURES:
	case FS_IOC_FSGETXATTR:
	case FS_IOC_FSSETXATTR:
	case F2FS_IOC_GET_PIN_FILE:
	case F2FS_IOC_SET_PIN_FILE:
	case F2FS_IOC_PRECACHE_EXTENTS:
	case F2FS_IOC_RESIZE_FS:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	case FS_IOC_ENABLE_VERITY:
	case FS_IOC_MEASURE_VERITY:
	case FS_IOC_GETFSLABEL:
	case FS_IOC_SETFSLABEL:
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	case FS_IOC_READ_VERITY_METADATA:
	case F2FS_IOC_GET_COMPRESS_BLOCKS:
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
	case F2FS_IOC_GET_SAVED_BLOCKS:
	case F2FS_IOC_GET_RELEASED_STATUS:
	case F2FS_IOC_COMPRESS_RELEASE_FILE:
	case F2FS_IOC_GET_COMPRESSED_STATUS:
	case F2FS_IOC_RESERVE_DECOMPRESS_FILE:
#endif
	case F2FS_IOC_RELEASE_COMPRESS_BLOCKS:
	case F2FS_IOC_RESERVE_COMPRESS_BLOCKS:
#endif
#ifdef CONFIG_F2FS_TURBO_ZONE
	case F2FS_IOC_GET_TZ_KEY_FILE:
	case F2FS_IOC_SET_TZ_KEY_FILE:
	case F2FS_IOC_GET_TZ_AGING_FILE:
	case F2FS_IOC_SET_TZ_AGING_FILE:
	case F2FS_IOC_GET_TZ_FREE_BLOCKS:
	case F2FS_IOC_GET_TZ_STATUS:
	case F2FS_IOC_SET_TZ_RETURN:
	case F2FS_IOC_MIGRATE_FILE:
	case F2FS_IOC_SET_TZ_FORCE_CLOSE:
	case F2FS_IOC_GET_TZ_SPACE_INFO:
	case F2FS_IOC_GET_TZ_INFO:
#endif
#ifdef CONFIG_F2FS_TURBO_ZONE_V2
	case F2FS_IOC_GET_TZ_SLC_INFO:
#ifdef CONFIG_F2FS_CHECK_FS
	case F2FS_IOC_GET_TZ_BLK_INFO:
#endif
#endif
#ifdef CONFIG_SDP_ENCRYPTION
	case F2FS_IOC_SET_SDP_ENCRYPTION_POLICY:
	case F2FS_IOC_GET_ENCRYPTION_POLICY_TYPE:
	case F2FS_IOC_GET_SDP_ENCRYPTION_POLICY:
#ifdef CONFIG_HWDPS
	case F2FS_IOC_SET_DPS_ENCRYPTION_POLICY:
#endif
#endif
		break;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	case F2FS_IOC_SEC_TRIM_FILE:
	case F2FS_IOC_SET_COMPRESS_OPTION:
	case F2FS_IOC_GET_COMPRESS_OPTION:
	case F2FS_IOC_DECOMPRESS_FILE:
	case F2FS_IOC_COMPRESS_FILE:
		break;
#endif
	default:
		return -ENOIOCTLCMD;
	}
	return __f2fs_ioctl(file, cmd, (unsigned long) compat_ptr(arg));
}
#endif

const struct file_operations f2fs_file_operations = {
	.llseek		= f2fs_llseek,
	.read_iter	= f2fs_file_read_iter,
	.write_iter	= f2fs_file_write_iter,
	.open		= f2fs_file_open,
	.release	= f2fs_release_file,
	.mmap		= f2fs_file_mmap,
	.flush		= f2fs_file_flush,
	.fsync		= f2fs_sync_file,
	.fallocate	= f2fs_fallocate,
	.unlocked_ioctl	= f2fs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= f2fs_compat_ioctl,
#endif
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
};
