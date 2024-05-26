// SPDX-License-Identifier: GPL-2.0
/*
 * f2fs sysfs interface
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 * Copyright (c) 2017 Chao Yu <chao@kernel.org>
 */
#include <linux/compiler.h>
#include <linux/proc_fs.h>
#include <linux/f2fs_fs.h>
#include <linux/seq_file.h>
#include <linux/ioprio.h>
#include <linux/sysfs.h>
#ifdef CONFIG_UNICODE
#include <linux/unicode.h>
#endif

#include "f2fs.h"
#include "segment.h"
#include "gc.h"
#include <trace/events/f2fs.h>
#ifdef CONFIG_F2FS_TURBO_ZONE
#include "turbo_zone.h"
#endif

static struct proc_dir_entry *f2fs_proc_root;

/* Sysfs support for f2fs */
enum {
	GC_THREAD,	/* struct f2fs_gc_thread */
	SM_INFO,	/* struct f2fs_sm_info */
	DCC_INFO,	/* struct discard_cmd_control */
	NM_INFO,	/* struct f2fs_nm_info */
	F2FS_SBI,	/* struct f2fs_sb_info */
#ifdef CONFIG_F2FS_STAT_FS
	STAT_INFO,	/* struct f2fs_stat_info */
#endif
#ifdef CONFIG_F2FS_FAULT_INJECTION
	FAULT_INFO_RATE,	/* struct f2fs_fault_info */
	FAULT_INFO_TYPE,	/* struct f2fs_fault_info */
#endif
	RESERVED_BLOCKS,	/* struct f2fs_sb_info */
	CPRC_INFO,	/* struct ckpt_req_control */
	ATGC_INFO,	/* struct atgc_management */
#ifdef CONFIG_F2FS_GRADING_SSR
	F2FS_HOT_COLD_PARAMS,
#endif
};

struct f2fs_attr {
	struct attribute attr;
	ssize_t (*show)(struct f2fs_attr *, struct f2fs_sb_info *, char *);
	ssize_t (*store)(struct f2fs_attr *, struct f2fs_sb_info *,
			 const char *, size_t);
	int struct_type;
	int offset;
	int id;
};

static ssize_t f2fs_sbi_show(struct f2fs_attr *a,
			     struct f2fs_sb_info *sbi, char *buf);

static unsigned char *__struct_ptr(struct f2fs_sb_info *sbi, int struct_type)
{
	if (struct_type == GC_THREAD)
		return (unsigned char *)&sbi->gc_thread;
	else if (struct_type == SM_INFO)
		return (unsigned char *)SM_I(sbi);
	else if (struct_type == DCC_INFO)
		return (unsigned char *)SM_I(sbi)->dcc_info;
	else if (struct_type == NM_INFO)
		return (unsigned char *)NM_I(sbi);
	else if (struct_type == F2FS_SBI || struct_type == RESERVED_BLOCKS)
		return (unsigned char *)sbi;
#ifdef CONFIG_F2FS_GRADING_SSR
	else if (struct_type == F2FS_HOT_COLD_PARAMS)
		return (unsigned char*)&sbi->hot_cold_params;
#endif
#ifdef CONFIG_F2FS_FAULT_INJECTION
	else if (struct_type == FAULT_INFO_RATE ||
					struct_type == FAULT_INFO_TYPE)
		return (unsigned char *)&F2FS_OPTION(sbi).fault_info;
#endif
#ifdef CONFIG_F2FS_STAT_FS
	else if (struct_type == STAT_INFO)
		return (unsigned char *)F2FS_STAT(sbi);
#endif
	else if (struct_type == CPRC_INFO)
		return (unsigned char *)&sbi->cprc_info;
	else if (struct_type == ATGC_INFO)
		return (unsigned char *)&sbi->am;
	return NULL;
}

static ssize_t dirty_segments_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%llu\n",
			(unsigned long long)(dirty_segments(sbi)));
}

static ssize_t current_flush_merge_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n",
						(current_flush_merge));
}

static ssize_t current_flush_merge_store(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, const char *buf, size_t len)
{
	int ret;
	unsigned long t = 0;

	ret = kstrtoul(skip_spaces(buf), 0, &t);
	if (ret == 0 && t < 65536)
		current_flush_merge = t;

	return len;
}

static ssize_t free_segments_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%llu\n",
			(unsigned long long)(free_segments(sbi)));
}

static ssize_t ovp_segments_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%llu\n",
			(unsigned long long)(overprovision_segments(sbi)));
}

static ssize_t lifetime_write_kbytes_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%llu\n",
			(unsigned long long)(sbi->kbytes_written +
			((f2fs_get_sectors_written(sbi) -
				sbi->sectors_written_start) >> 1)));
}

static ssize_t sb_status_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%lx\n", sbi->s_flag);
}

static ssize_t features_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	int len = 0;

	if (f2fs_sb_has_encrypt(sbi))
		len += scnprintf(buf, PAGE_SIZE - len, "%s",
						"encryption");
	if (f2fs_sb_has_blkzoned(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "blkzoned");
	if (f2fs_sb_has_extra_attr(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "extra_attr");
	if (f2fs_sb_has_project_quota(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "projquota");
	if (f2fs_sb_has_inode_chksum(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "inode_checksum");
	if (f2fs_sb_has_flexible_inline_xattr(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "flexible_inline_xattr");
	if (f2fs_sb_has_quota_ino(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "quota_ino");
	if (f2fs_sb_has_inode_crtime(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "inode_crtime");
	if (f2fs_sb_has_lost_found(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "lost_found");
	if (f2fs_sb_has_verity(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "verity");
	if (f2fs_sb_has_sb_chksum(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "sb_checksum");
	if (f2fs_sb_has_casefold(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "casefold");
	if (f2fs_sb_has_readonly(sbi))
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "readonly");
	if (f2fs_sb_has_compression(sbi)) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "compression");
#ifdef CONFIG_F2FS_FS_COMPRESSION_EX
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "hw_compression");
#endif
	}
#ifdef CONFIG_F2FS_FS_DEDUP
	if (f2fs_sb_has_dedup(sbi))
		len += snprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "hw_dedup");
#endif
	len += scnprintf(buf + len, PAGE_SIZE - len, "%s%s",
				len ? ", " : "", "pin_file");
	len += scnprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

static ssize_t current_reserved_blocks_show(struct f2fs_attr *a,
					struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%u\n", sbi->current_reserved_blocks);
}

static ssize_t unusable_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	block_t unusable;

	if (test_opt(sbi, DISABLE_CHECKPOINT))
		unusable = sbi->unusable_block_count;
	else
		unusable = f2fs_get_unusable_blocks(sbi);
	return sprintf(buf, "%llu\n", (unsigned long long)unusable);
}


static ssize_t encoding_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
#ifdef CONFIG_UNICODE
	struct super_block *sb = sbi->sb;

	if (f2fs_sb_has_casefold(sbi))
		return snprintf(buf, PAGE_SIZE, "%s (%d.%d.%d)\n",
			sb->s_encoding->charset,
			(sb->s_encoding->version >> 16) & 0xff,
			(sb->s_encoding->version >> 8) & 0xff,
			sb->s_encoding->version & 0xff);
#endif
	return sprintf(buf, "(none)");
}

static ssize_t mounted_time_sec_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "%llu", SIT_I(sbi)->mounted_time);
}

#ifdef CONFIG_F2FS_STAT_FS
static ssize_t moved_blocks_foreground_show(struct f2fs_attr *a,
				struct f2fs_sb_info *sbi, char *buf)
{
	struct f2fs_stat_info *si = F2FS_STAT(sbi);

	return sprintf(buf, "%llu\n",
		(unsigned long long)(si->tot_blks -
			(si->bg_data_blks + si->bg_node_blks)));
}

static ssize_t moved_blocks_background_show(struct f2fs_attr *a,
				struct f2fs_sb_info *sbi, char *buf)
{
	struct f2fs_stat_info *si = F2FS_STAT(sbi);

	return sprintf(buf, "%llu\n",
		(unsigned long long)(si->bg_data_blks + si->bg_node_blks));
}

static ssize_t avg_vblocks_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	struct f2fs_stat_info *si = F2FS_STAT(sbi);

	si->dirty_count = dirty_segments(sbi);
	f2fs_update_sit_info(sbi);
	return sprintf(buf, "%llu\n", (unsigned long long)(si->avg_vblocks));
}
#endif

static ssize_t main_blkaddr_show(struct f2fs_attr *a,
				struct f2fs_sb_info *sbi, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%llu\n",
			(unsigned long long)MAIN_BLKADDR(sbi));
}

static ssize_t f2fs_sbi_show(struct f2fs_attr *a,
			struct f2fs_sb_info *sbi, char *buf)
{
	unsigned char *ptr = NULL;
	unsigned int *ui;

	ptr = __struct_ptr(sbi, a->struct_type);
	if (!ptr)
		return -EINVAL;

	if (!strcmp(a->attr.name, "extension_list")) {
		__u8 (*extlist)[F2FS_EXTENSION_LEN] =
					sbi->raw_super->extension_list;
		int cold_count = le32_to_cpu(sbi->raw_super->extension_count);
		int hot_count = sbi->raw_super->hot_ext_count;
		int len = 0, i;

		len += scnprintf(buf + len, PAGE_SIZE - len,
						"cold file extension:\n");
		for (i = 0; i < cold_count; i++)
			len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
								extlist[i]);

		len += scnprintf(buf + len, PAGE_SIZE - len,
						"hot file extension:\n");
		for (i = cold_count; i < cold_count + hot_count; i++)
			len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n",
								extlist[i]);
		return len;
	}

	if (!strcmp(a->attr.name, "ckpt_thread_ioprio")) {
		struct ckpt_req_control *cprc = &sbi->cprc_info;
		int len = 0;
		int class = IOPRIO_PRIO_CLASS(cprc->ckpt_thread_ioprio);
		int data = IOPRIO_PRIO_DATA(cprc->ckpt_thread_ioprio);

		if (class == IOPRIO_CLASS_RT)
			len += scnprintf(buf + len, PAGE_SIZE - len, "rt,");
		else if (class == IOPRIO_CLASS_BE)
			len += scnprintf(buf + len, PAGE_SIZE - len, "be,");
		else
			return -EINVAL;

		len += scnprintf(buf + len, PAGE_SIZE - len, "%d\n", data);
		return len;
	}

#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	if (!strcmp(a->attr.name, "compr_written_block"))
		return sysfs_emit(buf, "%llu\n", sbi->compr_written_block);

	if (!strcmp(a->attr.name, "compr_saved_block"))
		return sysfs_emit(buf, "%llu\n", sbi->compr_saved_block);

	if (!strcmp(a->attr.name, "compr_new_inode"))
		return sysfs_emit(buf, "%u\n", sbi->compr_new_inode);
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	if (!strcmp(a->attr.name, "gc_segment_mode"))
		return sysfs_emit(buf, "%u\n", sbi->gc_segment_mode);

	if (!strcmp(a->attr.name, "gc_reclaimed_segments")) {
		return sysfs_emit(buf, "%u\n",
			sbi->gc_reclaimed_segs[sbi->gc_segment_mode]);
	}
#endif

	ui = (unsigned int *)(ptr + a->offset);

	return sprintf(buf, "%u\n", *ui);
}

static ssize_t __sbi_store(struct f2fs_attr *a,
			struct f2fs_sb_info *sbi,
			const char *buf, size_t count)
{
	unsigned char *ptr;
	unsigned long t;
	unsigned int *ui;
	struct f2fs_gc_kthread *gc_th = &sbi->gc_thread;
	ssize_t ret;

	ptr = __struct_ptr(sbi, a->struct_type);
	if (!ptr)
		return -EINVAL;

	if (!strcmp(a->attr.name, "extension_list")) {
		const char *name = strim((char *)buf);
		bool set = true, hot;

		if (!strncmp(name, "[h]", 3))
			hot = true;
		else if (!strncmp(name, "[c]", 3))
			hot = false;
		else
			return -EINVAL;

		name += 3;

		if (*name == '!') {
			name++;
			set = false;
		}

		if (strlen(name) >= F2FS_EXTENSION_LEN)
			return -EINVAL;

		down_write(&sbi->sb_lock);

		ret = f2fs_update_extension_list(sbi, name, hot, set);
		if (ret)
			goto out;

		ret = f2fs_commit_super(sbi, false);
		if (ret)
			f2fs_update_extension_list(sbi, name, hot, !set);
out:
		up_write(&sbi->sb_lock);
		return ret ? ret : count;
	}

	if (!strcmp(a->attr.name, "ckpt_thread_ioprio")) {
		const char *name = strim((char *)buf);
		struct ckpt_req_control *cprc = &sbi->cprc_info;
		int class;
		long data;
		int ret;

		if (!strncmp(name, "rt,", 3))
			class = IOPRIO_CLASS_RT;
		else if (!strncmp(name, "be,", 3))
			class = IOPRIO_CLASS_BE;
		else
			return -EINVAL;

		name += 3;
		ret = kstrtol(name, 10, &data);
		if (ret)
			return ret;
		if (data >= IOPRIO_BE_NR || data < 0)
			return -EINVAL;

		cprc->ckpt_thread_ioprio = IOPRIO_PRIO_VALUE(class, data);
		if (test_opt(sbi, MERGE_CHECKPOINT)) {
			ret = set_task_ioprio(cprc->f2fs_issue_ckpt,
					cprc->ckpt_thread_ioprio);
			if (ret)
				return ret;
		}

		return count;
	}

	ui = (unsigned int *)(ptr + a->offset);

	ret = kstrtoul(skip_spaces(buf), 0, &t);
	if (ret < 0)
		return ret;
#ifdef CONFIG_F2FS_FAULT_INJECTION
	if (a->struct_type == FAULT_INFO_TYPE && t >= (1 << FAULT_MAX))
		return -EINVAL;
	if (a->struct_type == FAULT_INFO_RATE && t >= UINT_MAX)
		return -EINVAL;
#endif
	if (a->struct_type == RESERVED_BLOCKS) {
		spin_lock(&sbi->stat_lock);
		if (t > (unsigned long)(sbi->user_block_count -
				F2FS_OPTION(sbi).root_reserved_blocks)) {
			spin_unlock(&sbi->stat_lock);
			return -EINVAL;
		}
		*ui = t;
		sbi->current_reserved_blocks = min(sbi->reserved_blocks,
				sbi->user_block_count - valid_user_blocks(sbi));
		spin_unlock(&sbi->stat_lock);
		return count;
	}

	if (!strcmp(a->attr.name, "discard_granularity")) {
		if (t == 0 || t > MAX_PLIST_NUM)
			return -EINVAL;
		if (t == *ui)
			return count;
		*ui = t;
		return count;
	}

	if (!strcmp(a->attr.name, "migration_granularity")) {
		if (t == 0 || t > sbi->segs_per_sec)
			return -EINVAL;
	}

	if (!strcmp(a->attr.name, "trim_sections"))
		return -EINVAL;

	if (!strcmp(a->attr.name, "gc_urgent")) {
		if (t == 0) {
			sbi->gc_mode = GC_NORMAL;
		} else if (t == 1) {
			sbi->gc_mode = GC_URGENT_HIGH;
			if (gc_th) {
				gc_th->gc_wake = 1;
				wake_up_interruptible_all(
					&gc_th->gc_wait_queue_head);
				wake_up_discard_thread(sbi, true);
			}
		} else if (t == 2) {
			sbi->gc_mode = GC_URGENT_LOW;
		} else {
			return -EINVAL;
		}
		return count;
	}
	if (!strcmp(a->attr.name, "gc_idle")) {
		if (t == GC_IDLE_CB) {
			sbi->gc_mode = GC_IDLE_CB;
		} else if (t == GC_IDLE_GREEDY) {
			sbi->gc_mode = GC_IDLE_GREEDY;
		} else if (t == GC_IDLE_AT) {
			if (!sbi->am.atgc_enabled)
				return -EINVAL;
			sbi->gc_mode = GC_AT;
		} else {
			sbi->gc_mode = GC_NORMAL;
		}
		return count;
	}

	if (!strcmp(a->attr.name, "iostat_enable")) {
		sbi->iostat_enable = !!t;
		if (!sbi->iostat_enable)
			f2fs_reset_iostat(sbi);
		return count;
	}

	if (!strcmp(a->attr.name, "iostat_period_ms")) {
		if (t < MIN_IOSTAT_PERIOD_MS || t > MAX_IOSTAT_PERIOD_MS)
			return -EINVAL;
		spin_lock(&sbi->iostat_lock);
		sbi->iostat_period_ms = (unsigned int)t;
		spin_unlock(&sbi->iostat_lock);
		return count;
	}

#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	if (!strcmp(a->attr.name, "compr_written_block") ||
		!strcmp(a->attr.name, "compr_saved_block")) {
		if (t != 0)
			return -EINVAL;
		sbi->compr_written_block = 0;
		sbi->compr_saved_block = 0;
		return count;
	}

	if (!strcmp(a->attr.name, "compr_new_inode")) {
		if (t != 0)
			return -EINVAL;
		sbi->compr_new_inode = 0;
		return count;
	}
#endif

	if (!strcmp(a->attr.name, "atgc_candidate_ratio")) {
		if (t > 100)
			return -EINVAL;
		sbi->am.candidate_ratio = t;
		return count;
	}

	if (!strcmp(a->attr.name, "atgc_age_weight")) {
		if (t > 100)
			return -EINVAL;
		sbi->am.age_weight = t;
		return count;
	}

	if (!strcmp(a->attr.name, "gc_segment_mode")) {
		if (t < MAX_GC_MODE)
			sbi->gc_segment_mode = t;
		else
			return -EINVAL;
		return count;
	}

	if (!strcmp(a->attr.name, "gc_reclaimed_segments")) {
		if (t != 0)
			return -EINVAL;
		sbi->gc_reclaimed_segs[sbi->gc_segment_mode] = 0;
		return count;
	}

	*ui = (unsigned int)t;

	return count;
}

static ssize_t f2fs_sbi_store(struct f2fs_attr *a,
			struct f2fs_sb_info *sbi,
			const char *buf, size_t count)
{
	ssize_t ret;
	bool gc_entry = (!strcmp(a->attr.name, "gc_urgent") ||
					a->struct_type == GC_THREAD);

	if (gc_entry) {
		if (!down_read_trylock(&sbi->sb->s_umount))
			return -EAGAIN;
	}
	ret = __sbi_store(a, sbi, buf, count);
	if (gc_entry)
		up_read(&sbi->sb->s_umount);

	return ret;
}

static ssize_t f2fs_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->show ? a->show(a, sbi, buf) : 0;
}

static ssize_t f2fs_attr_store(struct kobject *kobj, struct attribute *attr,
						const char *buf, size_t len)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
									s_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->store ? a->store(a, sbi, buf, len) : 0;
}

static void f2fs_sb_release(struct kobject *kobj)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_kobj);
	complete(&sbi->s_kobj_unregister);
}

/*
 * Note that there are three feature list entries:
 * 1) /sys/features
 *   : shows runtime features supported by in-kernel f2fs along with Kconfig.
 *     - ref. F2FS_FEATURE_RO_ATTR()
 *
 * 2) /sys/$s_id/features <deprecated>
 *   : shows on-disk features enabled by mkfs.f2fs, used for old kernels. This
 *     won't add new feature anymore, and thus, users should check entries in 3)
 *     instead of this 2).
 *
 * 3) /sys/$s_id/feature_list
 *   : shows on-disk features enabled by mkfs.f2fs per instance, which follows
 *     sysfs entry rule where each entry should expose single value.
 *     This list covers old feature list provided by 2) and beyond. Therefore,
 *     please add new on-disk feature in this list only.
 *     - ref. F2FS_SB_FEATURE_RO_ATTR()
 */

static ssize_t f2fs_feature_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	return sprintf(buf, "supported\n");
}

#define F2FS_FEATURE_RO_ATTR(_name)				\
static struct f2fs_attr f2fs_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = 0444 },	\
	.show	= f2fs_feature_show,				\
}

static ssize_t f2fs_sb_feature_show(struct f2fs_attr *a,
		struct f2fs_sb_info *sbi, char *buf)
{
	if (F2FS_HAS_FEATURE(sbi, a->id))
		return sprintf(buf, "supported\n");
	return sprintf(buf, "unsupported\n");
}

#define F2FS_SB_FEATURE_RO_ATTR(_name, _feat)			\
static struct f2fs_attr f2fs_attr_sb_##_name = {		\
	.attr = {.name = __stringify(_name), .mode = 0444 },	\
	.show	= f2fs_sb_feature_show,				\
	.id	= F2FS_FEATURE_##_feat,				\
}

#define F2FS_ATTR_OFFSET(_struct_type, _name, _mode, _show, _store, _offset) \
static struct f2fs_attr f2fs_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = _mode },	\
	.show	= _show,					\
	.store	= _store,					\
	.struct_type = _struct_type,				\
	.offset = _offset					\
}

#define F2FS_RW_ATTR(struct_type, struct_name, name, elname)	\
	F2FS_ATTR_OFFSET(struct_type, name, 0644,		\
		f2fs_sbi_show, f2fs_sbi_store,			\
		offsetof(struct struct_name, elname))

#define F2FS_GENERAL_RO_ATTR(name) \
static struct f2fs_attr f2fs_attr_##name = __ATTR(name, 0444, name##_show, NULL)

#define F2FS_GENERAL_RW_ATTR(name) \
static struct f2fs_attr f2fs_attr_##name = __ATTR(name, 0644, \
		name##_show, name##_store)

#define F2FS_STAT_ATTR(_struct_type, _struct_name, _name, _elname)	\
static struct f2fs_attr f2fs_attr_##_name = {			\
	.attr = {.name = __stringify(_name), .mode = 0444 },	\
	.show = f2fs_sbi_show,					\
	.struct_type = _struct_type,				\
	.offset = offsetof(struct _struct_name, _elname),       \
}

F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_urgent_sleep_time,
							urgent_sleep_time);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_min_sleep_time, min_sleep_time);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_max_sleep_time, max_sleep_time);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_no_gc_sleep_time, no_gc_sleep_time);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_idle, gc_mode);
F2FS_RW_ATTR(GC_THREAD, f2fs_gc_kthread, gc_preference, gc_preference);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_urgent, gc_mode);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, reclaim_segments, rec_prefree_segments);
F2FS_RW_ATTR(DCC_INFO, discard_cmd_control, max_small_discards, max_discards);
F2FS_RW_ATTR(DCC_INFO, discard_cmd_control, discard_granularity, discard_granularity);
F2FS_RW_ATTR(RESERVED_BLOCKS, f2fs_sb_info, reserved_blocks, reserved_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, batched_trim_sections, trim_sections);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, ipu_policy, ipu_policy);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_ipu_util, min_ipu_util);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_fsync_blocks, min_fsync_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_seq_blocks, min_seq_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_hot_blocks, min_hot_blocks);
F2FS_RW_ATTR(SM_INFO, f2fs_sm_info, min_ssr_sections, min_ssr_sections);
F2FS_RW_ATTR(NM_INFO, f2fs_nm_info, ram_thresh, ram_thresh);
F2FS_RW_ATTR(NM_INFO, f2fs_nm_info, ra_nid_pages, ra_nid_pages);
F2FS_RW_ATTR(NM_INFO, f2fs_nm_info, dirty_nats_ratio, dirty_nats_ratio);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, max_victim_search, max_victim_search);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, migration_granularity, migration_granularity);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, dir_level, dir_level);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_test_cond, gc_test_cond);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, cp_interval, interval_time[CP_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, idle_interval, interval_time[REQ_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, discard_idle_interval,
					interval_time[DISCARD_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_idle_interval, interval_time[GC_TIME]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info,
		umount_discard_timeout, interval_time[UMOUNT_DISCARD_TIMEOUT]);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, iostat_enable, iostat_enable);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, iostat_period_ms, iostat_period_ms);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, readdir_ra, readdir_ra);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, max_io_bytes, max_io_bytes);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_pin_file_thresh, gc_pin_file_threshold);
F2FS_RW_ATTR(F2FS_SBI, f2fs_super_block, extension_list, extension_list);
#ifdef CONFIG_F2FS_GRADING_SSR
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_hot_data_lower_limit, hot_data_lower_limit);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_hot_data_waterline, hot_data_waterline);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_warm_data_lower_limit, warm_data_lower_limit);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_warm_data_waterline, warm_data_waterline);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_hot_node_lower_limit, hot_node_lower_limit);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_hot_node_waterline, hot_node_waterline);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_warm_node_lower_limit, warm_node_lower_limit);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_warm_node_waterline, warm_node_waterline);
F2FS_RW_ATTR(F2FS_HOT_COLD_PARAMS, f2fs_hot_cold_params, hc_enable, enable);
#endif
#ifdef CONFIG_F2FS_FAULT_INJECTION
F2FS_RW_ATTR(FAULT_INFO_RATE, f2fs_fault_info, inject_rate, inject_rate);
F2FS_RW_ATTR(FAULT_INFO_TYPE, f2fs_fault_info, inject_type, inject_type);
#endif
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, data_io_flag, data_io_flag);
F2FS_RW_ATTR(CPRC_INFO, ckpt_req_control, ckpt_thread_ioprio, ckpt_thread_ioprio);
F2FS_GENERAL_RO_ATTR(dirty_segments);
F2FS_GENERAL_RO_ATTR(free_segments);
F2FS_GENERAL_RO_ATTR(ovp_segments);
F2FS_GENERAL_RO_ATTR(lifetime_write_kbytes);
F2FS_GENERAL_RO_ATTR(features);
F2FS_GENERAL_RO_ATTR(current_reserved_blocks);
F2FS_GENERAL_RO_ATTR(unusable);
F2FS_GENERAL_RO_ATTR(encoding);
F2FS_GENERAL_RO_ATTR(mounted_time_sec);
F2FS_GENERAL_RO_ATTR(main_blkaddr);
#ifdef CONFIG_F2FS_STAT_FS
F2FS_STAT_ATTR(STAT_INFO, f2fs_stat_info, cp_foreground_calls, cp_count);
F2FS_STAT_ATTR(STAT_INFO, f2fs_stat_info, cp_background_calls, bg_cp_count);
F2FS_STAT_ATTR(STAT_INFO, f2fs_stat_info, gc_foreground_calls, call_count);
F2FS_STAT_ATTR(STAT_INFO, f2fs_stat_info, gc_background_calls, bg_gc);
F2FS_GENERAL_RO_ATTR(moved_blocks_background);
F2FS_GENERAL_RO_ATTR(moved_blocks_foreground);
F2FS_GENERAL_RO_ATTR(avg_vblocks);
#endif

F2FS_GENERAL_RW_ATTR(current_flush_merge);

#ifdef CONFIG_FS_ENCRYPTION
F2FS_FEATURE_RO_ATTR(encryption);
F2FS_FEATURE_RO_ATTR(test_dummy_encryption_v2);
#ifdef CONFIG_UNICODE
F2FS_FEATURE_RO_ATTR(encrypted_casefold);
#endif
#endif /* CONFIG_FS_ENCRYPTION */
#ifdef CONFIG_BLK_DEV_ZONED
F2FS_FEATURE_RO_ATTR(block_zoned);
#endif
F2FS_FEATURE_RO_ATTR(atomic_write);
F2FS_FEATURE_RO_ATTR(extra_attr);
F2FS_FEATURE_RO_ATTR(project_quota);
F2FS_FEATURE_RO_ATTR(inode_checksum);
F2FS_FEATURE_RO_ATTR(flexible_inline_xattr);
F2FS_FEATURE_RO_ATTR(quota_ino);
F2FS_FEATURE_RO_ATTR(inode_crtime);
F2FS_FEATURE_RO_ATTR(lost_found);
#ifdef CONFIG_FS_VERITY
F2FS_FEATURE_RO_ATTR(verity);
#endif
F2FS_FEATURE_RO_ATTR(sb_checksum);
#ifdef CONFIG_UNICODE
F2FS_FEATURE_RO_ATTR(casefold);
#endif
F2FS_FEATURE_RO_ATTR(readonly);
#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
F2FS_FEATURE_RO_ATTR(compression);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, compr_written_block, compr_written_block);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, compr_saved_block, compr_saved_block);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, compr_new_inode, compr_new_inode);
 #endif
F2FS_FEATURE_RO_ATTR(pin_file);
#ifdef CONFIG_F2FS_FS_DEDUP
F2FS_FEATURE_RO_ATTR(hw_dedup);
#endif

/* For ATGC */
F2FS_RW_ATTR(ATGC_INFO, atgc_management, atgc_candidate_ratio, candidate_ratio);
F2FS_RW_ATTR(ATGC_INFO, atgc_management, atgc_candidate_count, max_candidate_count);
F2FS_RW_ATTR(ATGC_INFO, atgc_management, atgc_age_weight, age_weight);
F2FS_RW_ATTR(ATGC_INFO, atgc_management, atgc_age_threshold, age_threshold);

F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_segment_mode, gc_segment_mode);
F2FS_RW_ATTR(F2FS_SBI, f2fs_sb_info, gc_reclaimed_segments, gc_reclaimed_segs);

#define ATTR_LIST(name) (&f2fs_attr_##name.attr)
static struct attribute *f2fs_attrs[] = {
	ATTR_LIST(gc_urgent_sleep_time),
	ATTR_LIST(gc_min_sleep_time),
	ATTR_LIST(gc_max_sleep_time),
	ATTR_LIST(gc_no_gc_sleep_time),
	ATTR_LIST(gc_idle),
	ATTR_LIST(gc_preference),
	ATTR_LIST(gc_urgent),
	ATTR_LIST(reclaim_segments),
	ATTR_LIST(main_blkaddr),
	ATTR_LIST(max_small_discards),
	ATTR_LIST(discard_granularity),
	ATTR_LIST(batched_trim_sections),
	ATTR_LIST(ipu_policy),
	ATTR_LIST(min_ipu_util),
	ATTR_LIST(min_fsync_blocks),
	ATTR_LIST(min_seq_blocks),
	ATTR_LIST(min_hot_blocks),
	ATTR_LIST(min_ssr_sections),
	ATTR_LIST(max_victim_search),
	ATTR_LIST(migration_granularity),
	ATTR_LIST(dir_level),
	ATTR_LIST(gc_test_cond),
	ATTR_LIST(ram_thresh),
	ATTR_LIST(ra_nid_pages),
	ATTR_LIST(dirty_nats_ratio),
	ATTR_LIST(cp_interval),
	ATTR_LIST(idle_interval),
	ATTR_LIST(discard_idle_interval),
	ATTR_LIST(gc_idle_interval),
	ATTR_LIST(umount_discard_timeout),
	ATTR_LIST(iostat_enable),
	ATTR_LIST(iostat_period_ms),
	ATTR_LIST(readdir_ra),
	ATTR_LIST(max_io_bytes),
	ATTR_LIST(gc_pin_file_thresh),
	ATTR_LIST(extension_list),
#ifdef CONFIG_F2FS_FAULT_INJECTION
	ATTR_LIST(inject_rate),
	ATTR_LIST(inject_type),
#endif
	ATTR_LIST(data_io_flag),
	ATTR_LIST(ckpt_thread_ioprio),
	ATTR_LIST(dirty_segments),
	ATTR_LIST(free_segments),
	ATTR_LIST(ovp_segments),
	ATTR_LIST(unusable),
	ATTR_LIST(lifetime_write_kbytes),
	ATTR_LIST(features),
	ATTR_LIST(reserved_blocks),
	ATTR_LIST(current_reserved_blocks),
	ATTR_LIST(encoding),
	ATTR_LIST(mounted_time_sec),
#ifdef CONFIG_F2FS_STAT_FS
	ATTR_LIST(cp_foreground_calls),
	ATTR_LIST(cp_background_calls),
	ATTR_LIST(gc_foreground_calls),
	ATTR_LIST(gc_background_calls),
	ATTR_LIST(moved_blocks_foreground),
	ATTR_LIST(moved_blocks_background),
	ATTR_LIST(avg_vblocks),
#endif
#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	ATTR_LIST(compr_written_block),
	ATTR_LIST(compr_saved_block),
	ATTR_LIST(compr_new_inode),
#endif
	/* For ATGC */
	ATTR_LIST(atgc_candidate_ratio),
	ATTR_LIST(atgc_candidate_count),
	ATTR_LIST(atgc_age_weight),
	ATTR_LIST(atgc_age_threshold),
	ATTR_LIST(gc_segment_mode),
	ATTR_LIST(gc_reclaimed_segments),
	ATTR_LIST(current_flush_merge),
#ifdef CONFIG_F2FS_GRADING_SSR
	ATTR_LIST(hc_hot_data_lower_limit),
	ATTR_LIST(hc_hot_data_waterline),
	ATTR_LIST(hc_warm_data_lower_limit),
	ATTR_LIST(hc_warm_data_waterline),
	ATTR_LIST(hc_hot_node_lower_limit),
	ATTR_LIST(hc_hot_node_waterline),
	ATTR_LIST(hc_warm_node_lower_limit),
	ATTR_LIST(hc_warm_node_waterline),
	ATTR_LIST(hc_enable),
#endif
	NULL,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
ATTRIBUTE_GROUPS(f2fs);
#endif

static struct attribute *f2fs_feat_attrs[] = {
#ifdef CONFIG_FS_ENCRYPTION
	ATTR_LIST(encryption),
	ATTR_LIST(test_dummy_encryption_v2),
#ifdef CONFIG_UNICODE
	ATTR_LIST(encrypted_casefold),
#endif
#endif /* CONFIG_FS_ENCRYPTION */
#ifdef CONFIG_BLK_DEV_ZONED
	ATTR_LIST(block_zoned),
#endif
	ATTR_LIST(atomic_write),
	ATTR_LIST(extra_attr),
	ATTR_LIST(project_quota),
	ATTR_LIST(inode_checksum),
	ATTR_LIST(flexible_inline_xattr),
	ATTR_LIST(quota_ino),
	ATTR_LIST(inode_crtime),
	ATTR_LIST(lost_found),
#ifdef CONFIG_FS_VERITY
	ATTR_LIST(verity),
#endif
	ATTR_LIST(sb_checksum),
#ifdef CONFIG_UNICODE
	ATTR_LIST(casefold),
#endif
	ATTR_LIST(readonly),
#if defined(CONFIG_F2FS_FS_COMPRESSION) || defined(CONFIG_F2FS_FS_COMPRESSION_EX)
	ATTR_LIST(compression),
#endif
	ATTR_LIST(pin_file),
#ifdef CONFIG_F2FS_FS_DEDUP
	ATTR_LIST(hw_dedup),
#endif
	NULL,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
ATTRIBUTE_GROUPS(f2fs_feat);
#endif

F2FS_GENERAL_RO_ATTR(sb_status);
static struct attribute *f2fs_stat_attrs[] = {
	ATTR_LIST(sb_status),
	NULL,
};
ATTRIBUTE_GROUPS(f2fs_stat);

F2FS_SB_FEATURE_RO_ATTR(encryption, ENCRYPT);
F2FS_SB_FEATURE_RO_ATTR(block_zoned, BLKZONED);
F2FS_SB_FEATURE_RO_ATTR(extra_attr, EXTRA_ATTR);
F2FS_SB_FEATURE_RO_ATTR(project_quota, PRJQUOTA);
F2FS_SB_FEATURE_RO_ATTR(inode_checksum, INODE_CHKSUM);
F2FS_SB_FEATURE_RO_ATTR(flexible_inline_xattr, FLEXIBLE_INLINE_XATTR);
F2FS_SB_FEATURE_RO_ATTR(quota_ino, QUOTA_INO);
F2FS_SB_FEATURE_RO_ATTR(inode_crtime, INODE_CRTIME);
F2FS_SB_FEATURE_RO_ATTR(lost_found, LOST_FOUND);
F2FS_SB_FEATURE_RO_ATTR(verity, VERITY);
F2FS_SB_FEATURE_RO_ATTR(sb_checksum, SB_CHKSUM);
F2FS_SB_FEATURE_RO_ATTR(casefold, CASEFOLD);
F2FS_SB_FEATURE_RO_ATTR(compression, COMPRESSION);
F2FS_SB_FEATURE_RO_ATTR(readonly, RO);

static struct attribute *f2fs_sb_feat_attrs[] = {
	ATTR_LIST(sb_encryption),
	ATTR_LIST(sb_block_zoned),
	ATTR_LIST(sb_extra_attr),
	ATTR_LIST(sb_project_quota),
	ATTR_LIST(sb_inode_checksum),
	ATTR_LIST(sb_flexible_inline_xattr),
	ATTR_LIST(sb_quota_ino),
	ATTR_LIST(sb_inode_crtime),
	ATTR_LIST(sb_lost_found),
	ATTR_LIST(sb_verity),
	ATTR_LIST(sb_sb_checksum),
	ATTR_LIST(sb_casefold),
	ATTR_LIST(sb_compression),
	ATTR_LIST(sb_readonly),
	NULL,
};
ATTRIBUTE_GROUPS(f2fs_sb_feat);

static const struct sysfs_ops f2fs_attr_ops = {
	.show	= f2fs_attr_show,
	.store	= f2fs_attr_store,
};

static struct kobj_type f2fs_sb_ktype = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	.default_groups = f2fs_groups,
#else
	.default_attrs  = f2fs_attrs,
#endif
	.sysfs_ops	= &f2fs_attr_ops,
	.release	= f2fs_sb_release,
};

static struct kobj_type f2fs_ktype = {
	.sysfs_ops	= &f2fs_attr_ops,
};

static struct kset f2fs_kset = {
	.kobj	= {.ktype = &f2fs_ktype},
};

static struct kobj_type f2fs_feat_ktype = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	.default_groups = f2fs_feat_groups,
#else
	.default_attrs  = f2fs_feat_attrs,
#endif
	.sysfs_ops	= &f2fs_attr_ops,
};

static struct kobject f2fs_feat = {
	.kset	= &f2fs_kset,
};

static ssize_t f2fs_stat_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_stat_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->show ? a->show(a, sbi, buf) : 0;
}

static ssize_t f2fs_stat_attr_store(struct kobject *kobj, struct attribute *attr,
						const char *buf, size_t len)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_stat_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->store ? a->store(a, sbi, buf, len) : 0;
}

static void f2fs_stat_kobj_release(struct kobject *kobj)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
								s_stat_kobj);
	complete(&sbi->s_stat_kobj_unregister);
}

static const struct sysfs_ops f2fs_stat_attr_ops = {
	.show	= f2fs_stat_attr_show,
	.store	= f2fs_stat_attr_store,
};

static struct kobj_type f2fs_stat_ktype = {
	.default_groups = f2fs_stat_groups,
	.sysfs_ops	= &f2fs_stat_attr_ops,
	.release	= f2fs_stat_kobj_release,
};

static ssize_t f2fs_sb_feat_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
							s_feature_list_kobj);
	struct f2fs_attr *a = container_of(attr, struct f2fs_attr, attr);

	return a->show ? a->show(a, sbi, buf) : 0;
}

static void f2fs_feature_list_kobj_release(struct kobject *kobj)
{
	struct f2fs_sb_info *sbi = container_of(kobj, struct f2fs_sb_info,
							s_feature_list_kobj);
	complete(&sbi->s_feature_list_kobj_unregister);
}

static const struct sysfs_ops f2fs_feature_list_attr_ops = {
	.show	= f2fs_sb_feat_attr_show,
};

static struct kobj_type f2fs_feature_list_ktype = {
	.default_groups = f2fs_sb_feat_groups,
	.sysfs_ops	= &f2fs_feature_list_attr_ops,
	.release	= f2fs_feature_list_kobj_release,
};

static int __maybe_unused segment_info_seq_show(struct seq_file *seq,
						void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	unsigned int total_segs =
			le32_to_cpu(sbi->raw_super->segment_count_main);
	int i;

	seq_puts(seq, "format: segment_type|valid_blocks\n"
		"segment_type(0:HD, 1:WD, 2:CD, 3:HN, 4:WN, 5:CN)\n");

	for (i = 0; i < total_segs; i++) {
		struct seg_entry *se = get_seg_entry(sbi, i);

		if ((i % 10) == 0)
			seq_printf(seq, "%-10d", i);
		seq_printf(seq, "%d|%-3u", se->type, se->valid_blocks);
		if ((i % 10) == 9 || i == (total_segs - 1))
			seq_putc(seq, '\n');
		else
			seq_putc(seq, ' ');
	}

	return 0;
}

static int __maybe_unused segment_bits_seq_show(struct seq_file *seq,
						void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	unsigned int total_segs =
			le32_to_cpu(sbi->raw_super->segment_count_main);
	int i, j;

	seq_puts(seq, "format: segment_type|valid_blocks|bitmaps\n"
		"segment_type(0:HD, 1:WD, 2:CD, 3:HN, 4:WN, 5:CN)\n");

	for (i = 0; i < total_segs; i++) {
		struct seg_entry *se = get_seg_entry(sbi, i);

		seq_printf(seq, "%-10d", i);
		seq_printf(seq, "%d|%-3u|", se->type, se->valid_blocks);
		for (j = 0; j < SIT_VBLOCK_MAP_SIZE; j++)
			seq_printf(seq, " %.2x", se->cur_valid_map[j]);
		seq_putc(seq, '\n');
	}
	return 0;
}

void f2fs_record_iostat(struct f2fs_sb_info *sbi)
{
	unsigned long long iostat_diff[NR_IO_TYPE];
	int i;

	if (time_is_after_jiffies(sbi->iostat_next_period))
		return;

	/* Need double check under the lock */
	spin_lock(&sbi->iostat_lock);
	if (time_is_after_jiffies(sbi->iostat_next_period)) {
		spin_unlock(&sbi->iostat_lock);
		return;
	}
	sbi->iostat_next_period = jiffies +
				msecs_to_jiffies(sbi->iostat_period_ms);

	for (i = 0; i < NR_IO_TYPE; i++) {
		iostat_diff[i] = sbi->rw_iostat[i] -
				sbi->prev_rw_iostat[i];
		sbi->prev_rw_iostat[i] = sbi->rw_iostat[i];
	}
	spin_unlock(&sbi->iostat_lock);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	trace_f2fs_iostat(sbi, iostat_diff);
#endif
}

static int __maybe_unused iostat_info_seq_show(struct seq_file *seq,
					       void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	time64_t now = ktime_get_real_seconds();

	if (!sbi->iostat_enable)
		return 0;

	seq_printf(seq, "time:		%-16llu\n", now);

	/* print app write IOs */
	seq_puts(seq, "[WRITE]\n");
	seq_printf(seq, "app buffered:	%-16llu\n",
				sbi->rw_iostat[APP_BUFFERED_IO]);
	seq_printf(seq, "app direct:	%-16llu\n",
				sbi->rw_iostat[APP_DIRECT_IO]);
	seq_printf(seq, "app mapped:	%-16llu\n",
				sbi->rw_iostat[APP_MAPPED_IO]);

	/* print fs write IOs */
	seq_printf(seq, "fs data:	%-16llu\n",
				sbi->rw_iostat[FS_DATA_IO]);
	seq_printf(seq, "fs node:	%-16llu\n",
				sbi->rw_iostat[FS_NODE_IO]);
	seq_printf(seq, "fs meta:	%-16llu\n",
				sbi->rw_iostat[FS_META_IO]);
	seq_printf(seq, "fs gc data:	%-16llu\n",
				sbi->rw_iostat[FS_GC_DATA_IO]);
	seq_printf(seq, "fs gc node:	%-16llu\n",
				sbi->rw_iostat[FS_GC_NODE_IO]);
	seq_printf(seq, "fs cp data:	%-16llu\n",
				sbi->rw_iostat[FS_CP_DATA_IO]);
	seq_printf(seq, "fs cp node:	%-16llu\n",
				sbi->rw_iostat[FS_CP_NODE_IO]);
	seq_printf(seq, "fs cp meta:	%-16llu\n",
				sbi->rw_iostat[FS_CP_META_IO]);

	/* print app read IOs */
	seq_printf(seq, "app buffered:	%-16llu\n",
				sbi->rw_iostat[APP_BUFFERED_READ_IO]);
	seq_printf(seq, "app direct:	%-16llu\n",
				sbi->rw_iostat[APP_DIRECT_READ_IO]);
	seq_printf(seq, "app mapped:	%-16llu\n",
				sbi->rw_iostat[APP_MAPPED_READ_IO]);

	/* print fs read IOs */
	seq_puts(seq, "[READ]\n");
	seq_printf(seq, "fs data:	%-16llu\n",
				sbi->rw_iostat[FS_DATA_READ_IO]);
	seq_printf(seq, "fs gc data:	%-16llu\n",
				sbi->rw_iostat[FS_GDATA_READ_IO]);
	seq_printf(seq, "fs compr_data:	%-16llu\n",
				sbi->rw_iostat[FS_CDATA_READ_IO]);
	seq_printf(seq, "fs node:	%-16llu\n",
				sbi->rw_iostat[FS_NODE_READ_IO]);
	seq_printf(seq, "fs meta:	%-16llu\n",
				sbi->rw_iostat[FS_META_READ_IO]);

	/* print other IOs */
	seq_puts(seq, "[OTHER]\n");
	seq_printf(seq, "fs discard:	%-16llu\n",
				sbi->rw_iostat[FS_DISCARD]);

	return 0;
}

static int __maybe_unused victim_bits_seq_show(struct seq_file *seq,
						void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct dirty_seglist_info *dirty_i = DIRTY_I(sbi);
	int i;

	seq_puts(seq, "format: victim_secmap bitmaps\n");

	for (i = 0; i < MAIN_SECS(sbi); i++) {
		if ((i % 10) == 0)
			seq_printf(seq, "%-10d", i);
		seq_printf(seq, "%d", test_bit(i, dirty_i->victim_secmap) ? 1 : 0);
		if ((i % 10) == 9 || i == (MAIN_SECS(sbi) - 1))
			seq_putc(seq, '\n');
		else
			seq_putc(seq, ' ');
	}
	return 0;
}

static int undiscard_info_seq_show(struct seq_file *seq, void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct dirty_seglist_info *dirty_i = DIRTY_I(sbi);
	struct sit_info *sit_i = SIT_I(sbi);
	unsigned int total_segs =
		le32_to_cpu(sbi->raw_super->segment_count_main);
	unsigned int total = 0;
	unsigned int i, j;

	if (!f2fs_realtime_discard_enable(sbi))
		goto out;

	for (i = 0; i < total_segs; i++) {
		struct seg_entry *se = get_seg_entry(sbi, i);
		unsigned int entries = SIT_VBLOCK_MAP_SIZE /
			sizeof(unsigned long);
		unsigned int max_blocks = sbi->blocks_per_seg;
		unsigned long *ckpt_map = (unsigned long *)se->ckpt_valid_map;
		unsigned long *discard_map = (unsigned long *)se->discard_map;
		unsigned long *dmap = SIT_I(sbi)->tmp_map;
		int start = 0, end = -1;

		down_write(&sit_i->sentry_lock);
		if (se->valid_blocks == max_blocks) {
			up_write(&sit_i->sentry_lock);
			continue;
		}

		if (se->valid_blocks == 0) {
			mutex_lock(&dirty_i->seglist_lock);
			if (test_bit((int)i, dirty_i->dirty_segmap[PRE]))
				total += 512;
			mutex_unlock(&dirty_i->seglist_lock);
		} else {
			for (j = 0; j < entries; j++)
				dmap[j] = ~ckpt_map[j] & ~discard_map[j];
			while (1) {
				start = (int)__find_rev_next_bit(dmap,
					(unsigned long)max_blocks,
					(unsigned long)(end + 1));

				if ((unsigned int)start >= max_blocks)
					break;

				end = (int)__find_rev_next_zero_bit(dmap,
					(unsigned long)max_blocks,
					(unsigned long)(start + 1));
				total += (unsigned int)(end - start);
			}
		}

		up_write(&sit_i->sentry_lock);
	}

out:
	seq_printf(seq, "%u\n", total * 4);
	return 0;
}

static int resizf2fs_info_seq_show(struct seq_file *seq, void *offset)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);

	seq_printf(seq, "total_node_count: %u\n"
		"total_valid_node_count: %u\n",
		sbi->total_node_count, sbi->total_valid_node_count);
	return 0;
}

#define F2FS_PROC_FILE_DEF(_name)					\
static int _name##_open_fs(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, _name##_seq_show, PDE_DATA(inode));	\
}									\
									\
static const struct file_operations f2fs_seq_##_name##_fops = {		\
	.open = _name##_open_fs,					\
	.read = seq_read,						\
	.llseek = seq_lseek,						\
	.release = single_release,					\
};

F2FS_PROC_FILE_DEF(victim_bits);
F2FS_PROC_FILE_DEF(segment_info);
F2FS_PROC_FILE_DEF(segment_bits);
F2FS_PROC_FILE_DEF(iostat_info);
F2FS_PROC_FILE_DEF(undiscard_info);
F2FS_PROC_FILE_DEF(resizf2fs_info);
#ifdef CONFIG_F2FS_STAT_FS
/* f2fs big-data statistics */
#define F2FS_BD_PROC_DEF(_name)					\
static int f2fs_##_name##_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, f2fs_##_name##_show, PDE_DATA(inode));	\
}									\
									\
static const struct file_operations f2fs_##_name##_fops = {		\
	.owner = THIS_MODULE,						\
	.open = f2fs_##_name##_open,					\
	.read = seq_read,						\
	.write = f2fs_##_name##_write,					\
	.llseek = seq_lseek,						\
	.release = single_release,					\
};

static int f2fs_bd_base_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);

	/*
	 * each column indicates: blk_cnt fs_blk_cnt free_seg_cnt
	 * reserved_seg_cnt valid_user_blocks
	 */
	seq_printf(seq, "%llu %llu %u %u %u\n",
		le64_to_cpu(sbi->raw_super->block_count),
		le64_to_cpu(sbi->raw_super->block_count) -
		le32_to_cpu(sbi->raw_super->main_blkaddr),
		free_segments(sbi), reserved_segments(sbi),
		valid_user_blocks(sbi));
	return 0;
}

static ssize_t f2fs_bd_base_info_write(struct file *file,
				       const char __user *buf,
				       size_t length, loff_t *ppos)
{
	return length;
}

static int f2fs_bd_discard_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	struct dirty_seglist_info *dirty_i = DIRTY_I(sbi);
	struct sit_info *sit_i = SIT_I(sbi);
	unsigned int segs = le32_to_cpu(sbi->raw_super->segment_count_main);
	unsigned int entries = SIT_VBLOCK_MAP_SIZE / sizeof(unsigned long);
	unsigned int max_blocks = sbi->blocks_per_seg;
	unsigned int total_blks = 0, undiscard_cnt = 0;
	unsigned int i, j;

	if (!f2fs_hw_support_discard(sbi))
		goto out;
	for (i = 0; i < segs; i++) {
		struct seg_entry *se = get_seg_entry(sbi, i);
		/*lint -save -e826*/
		unsigned long *ckpt_map = (unsigned long *)se->ckpt_valid_map;
		unsigned long *discard_map = (unsigned long *)se->discard_map;
		/*lint -restore*/
		unsigned long *dmap = SIT_I(sbi)->tmp_map;
		int start = 0, end = -1;

		down_write(&sit_i->sentry_lock);

		if (se->valid_blocks == max_blocks) {
			up_write(&sit_i->sentry_lock);
			continue;
		}

		if (se->valid_blocks == 0) {
			mutex_lock(&dirty_i->seglist_lock);
			if (test_bit((int)i, dirty_i->dirty_segmap[PRE])) {
				total_blks += 512;
				undiscard_cnt++;
			}
			mutex_unlock(&dirty_i->seglist_lock);
		} else {
			for (j = 0; j < entries; j++)
				dmap[j] = ~ckpt_map[j] & ~discard_map[j];
			while (1) {
				/*lint -save -e571 -e776*/
				start = (int)__find_rev_next_bit(dmap, (unsigned long)max_blocks,
								 (unsigned long)(end + 1));
				/*lint -restore*/
				/*lint -save -e574 -e737*/
				if ((unsigned int)start >= max_blocks)
					break;
				/*lint -restore*/
				/*lint -save -e571 -e776*/
				end = (int)__find_rev_next_zero_bit(dmap, (unsigned long)max_blocks,
								    (unsigned long)(start + 1));
				/*lint -restore*/
				total_blks += (unsigned int)(end - start);
				undiscard_cnt++;
			}
		}

		up_write(&sit_i->sentry_lock);
	}

out:
	/*
	 * each colum indicates: discard_cnt discard_blk_cnt undiscard_cnt
	 * undiscard_blk_cnt discard_time max_discard_time
	 */
	bd_mutex_lock(&sbi->bd_mutex);
	bd->undiscard_cnt = undiscard_cnt;
	bd->undiscard_blk_cnt = total_blks;
	seq_printf(seq, "%u %u %u %u %llu %llu\n", bd->discard_cnt,
		   bd->discard_blk_cnt, bd->undiscard_cnt,
		   bd->undiscard_blk_cnt, bd->discard_time,
		   bd->max_discard_time);
	bd_mutex_unlock(&sbi->bd_mutex);
	return 0;
}

static ssize_t f2fs_bd_discard_info_write(struct file *file,
					  const char __user *buf,
					  size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	char buffer[3] = {0};

	if (!buf || length > 2 || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	bd->discard_cnt = 0;
	bd->discard_blk_cnt = 0;
	bd->undiscard_cnt = 0;
	bd->undiscard_blk_cnt = 0;
	bd->discard_time = 0;
	bd->max_discard_time = 0;
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}

static int f2fs_bd_cp_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	/*
	 * each column indicates: cp_cnt cp_succ_cnt cp_time max_cp_time
	 * max_cp_submit_time max_cp_flush_meta_time max_cp_discard_time
	 */
	bd_mutex_lock(&sbi->bd_mutex);
	bd->cp_cnt = sbi->stat_info->cp_count;
	seq_printf(seq, "%u %u %llu %llu %llu %llu %llu\n", bd->cp_cnt,
		   bd->cp_succ_cnt, bd->cp_time, bd->max_cp_time,
		   bd->max_cp_submit_time, bd->max_cp_flush_meta_time,
		   bd->max_cp_discard_time);
	bd_mutex_unlock(&sbi->bd_mutex);
	return 0;
}

static ssize_t f2fs_bd_cp_info_write(struct file *file,	const char __user *buf,
	size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	char buffer[3] = {0};

	if (!buf || length > 2 || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	bd->cp_cnt = 0;
	bd->cp_succ_cnt = 0;
	bd->cp_time = 0;
	bd->max_cp_time = 0;
	bd->max_cp_submit_time = 0;
	bd->max_cp_flush_meta_time = 0;
	bd->max_cp_discard_time = 0;
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}

static int f2fs_bd_gc_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	/*
	 * each column indicates: fggc_cnt fgg bggc_cnt bggc_fail_cntc_fail_cnt
	 * bggc_data_seg_cnt bggc_data_blk_cnt bggc_node_seg_cnt
	 * bggc_node_blk_cnt
	 * fggc_data_seg_cnt fggc_data_blk_cnt fggc_node_seg_cnt
	 * fggc_node_blk_cnt
	 * node_ssr_cnt data_ssr_cnt node_lfs_cnt data_lfs_cnt data_ipu_cnt
	 * fggc_time
	 */
	bd_mutex_lock(&sbi->bd_mutex);
	seq_printf(seq, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
		"%u %u %llu\n",
		bd->gc_cnt[BG_GC], bd->gc_fail_cnt[BG_GC],
		bd->gc_cnt[FG_GC], bd->gc_fail_cnt[FG_GC],
		bd->gc_data_seg_cnt[BG_GC], bd->gc_data_blk_cnt[BG_GC],
		bd->gc_node_seg_cnt[BG_GC], bd->gc_node_blk_cnt[BG_GC],
		bd->gc_data_seg_cnt[FG_GC], bd->gc_data_blk_cnt[FG_GC],
		bd->gc_node_seg_cnt[FG_GC], bd->gc_node_blk_cnt[FG_GC],
		bd->data_alloc_cnt[SSR], bd->node_alloc_cnt[SSR],
		bd->data_alloc_cnt[LFS], bd->node_alloc_cnt[LFS],
		bd->data_ipu_cnt, bd->fggc_time);
	bd_mutex_unlock(&sbi->bd_mutex);
	return 0;
}

static ssize_t f2fs_bd_gc_info_write(struct file *file,
				     const char __user *buf,
				     size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	int i;
	char buffer[3] = {0};

	if (!buf || length > 2 || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	for (i = BG_GC; i <= FG_GC; i++) {
		bd->gc_cnt[i] = 0;
		bd->gc_fail_cnt[i] = 0;
		bd->gc_data_cnt[i] = 0;
		bd->gc_node_cnt[i] = 0;
		bd->gc_data_seg_cnt[i] = 0;
		bd->gc_data_blk_cnt[i] = 0;
		bd->gc_node_seg_cnt[i] = 0;
		bd->gc_node_blk_cnt[i] = 0;
	}
	bd->fggc_time = 0;
	for (i = LFS; i <= SSR; i++) {
		bd->node_alloc_cnt[i] = 0;
		bd->data_alloc_cnt[i] = 0;
	}
	bd->data_ipu_cnt = 0;
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}

static int f2fs_bd_fsync_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	/*
	 * eacho column indicates: fsync_reg_file_cnt fsync_dir_cnt fsync_time
	 * max_fsync_time fsync_wr_file_time max_fsync_wr_file_time
	 * fsync_cp_time max_fsync_cp_time fsync_sync_node_time
	 * max_fsync_sync_node_time fsync_flush_time max_fsync_flush_time
	 */
	bd_mutex_lock(&sbi->bd_mutex);
	seq_printf(seq, "%u %u %llu %llu %llu %llu %llu %llu %llu "
		"%llu %llu %llu\n",
		bd->fsync_reg_file_cnt, bd->fsync_dir_cnt, bd->fsync_time,
		bd->max_fsync_time, bd->fsync_wr_file_time,
		bd->max_fsync_wr_file_time, bd->fsync_cp_time,
		bd->max_fsync_cp_time, bd->fsync_sync_node_time,
		bd->max_fsync_sync_node_time, bd->fsync_flush_time,
		bd->max_fsync_flush_time);
	bd_mutex_unlock(&sbi->bd_mutex);
	return 0;
}

static ssize_t f2fs_bd_fsync_info_write(struct file *file,
	const char __user *buf,
	size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	char buffer[3] = {0};

	if (!buf || length > 2 || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	bd->fsync_reg_file_cnt = 0;
	bd->fsync_dir_cnt = 0;
	bd->fsync_time = 0;
	bd->max_fsync_time = 0;
	bd->fsync_cp_time = 0;
	bd->max_fsync_cp_time = 0;
	bd->fsync_wr_file_time = 0;
	bd->max_fsync_wr_file_time = 0;
	bd->fsync_sync_node_time = 0;
	bd->max_fsync_sync_node_time = 0;
	bd->fsync_flush_time = 0;
	bd->max_fsync_flush_time = 0;
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}

static int f2fs_bd_hotcold_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	bd_mutex_lock(&sbi->bd_mutex);
	/*
	 * each colum indicates: hot_data_cnt, warm_data_cnt, cold_data_cnt,
	 * hot_node_cnt,
	 * warm_node_cnt, cold_node_cnt, meta_cp_cnt, meta_sit_cnt,
	 * meta_nat_cnt, meta_ssa_cnt,
	 * directio_cnt, gc_cold_data_cnt, rewrite_hot_data_cnt,
	 * rewrite_warm_data_cnt,
	 * gc_segment_hot_data_cnt, gc_segment_warm_data_cnt,
	 * gc_segment_cold_data_cnt,
	 * gc_segment_hot_node_cnt, gc_segment_warm_node_cnt,
	 * gc_segment_cold_node_cnt,
	 * gc_block_hot_data_cnt, gc_block_warm_data_cnt, gc_block_cold_data_cnt,
	 * gc_block_hot_node_cnt, gc_block_warm_node_cnt, gc_block_cold_node_cnt
	 */
	seq_printf(seq, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu "
		   "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
		   bd->hotcold_cnt[HC_HOT_DATA], bd->hotcold_cnt[HC_WARM_DATA],
		   bd->hotcold_cnt[HC_COLD_DATA], bd->hotcold_cnt[HC_HOT_NODE],
		   bd->hotcold_cnt[HC_WARM_NODE], bd->hotcold_cnt[HC_COLD_NODE],
		   bd->hotcold_cnt[HC_META], bd->hotcold_cnt[HC_META_SB],
		   bd->hotcold_cnt[HC_META_CP], bd->hotcold_cnt[HC_META_SIT],
		   bd->hotcold_cnt[HC_META_NAT], bd->hotcold_cnt[HC_META_SSA],
		   bd->hotcold_cnt[HC_DIRECTIO], bd->hotcold_cnt[HC_GC_COLD_DATA],
		   bd->hotcold_cnt[HC_REWRITE_HOT_DATA],
		   bd->hotcold_cnt[HC_REWRITE_WARM_DATA],
		   bd->hotcold_gc_seg_cnt[HC_HOT_DATA],
		   bd->hotcold_gc_seg_cnt[HC_WARM_DATA],
		   bd->hotcold_gc_seg_cnt[HC_COLD_DATA],
		   bd->hotcold_gc_seg_cnt[HC_HOT_NODE],
		   bd->hotcold_gc_seg_cnt[HC_WARM_NODE],
		   bd->hotcold_gc_seg_cnt[HC_COLD_NODE],
		   bd->hotcold_gc_blk_cnt[HC_HOT_DATA],
		   bd->hotcold_gc_blk_cnt[HC_WARM_DATA],
		   bd->hotcold_gc_blk_cnt[HC_COLD_DATA],
		   bd->hotcold_gc_blk_cnt[HC_HOT_NODE],
		   bd->hotcold_gc_blk_cnt[HC_WARM_NODE],
		   bd->hotcold_gc_blk_cnt[HC_COLD_NODE]);
	bd_mutex_unlock(&sbi->bd_mutex);
	return 0;
}

static ssize_t f2fs_bd_hotcold_info_write(struct file *file,
	const char __user *buf,
	size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	char buffer[3] = {0};
	int i;

	if (!buf || length > 2 || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	for (i = 0; i < NR_HOTCOLD_TYPE; i++)
		bd->hotcold_cnt[i] = 0;
	for (i = 0; i < NR_CURSEG; i++) {
		bd->hotcold_gc_seg_cnt[i] = 0;
		bd->hotcold_gc_blk_cnt[i] = 0;
	}
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}

static int f2fs_bd_encrypt_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	bd_mutex_lock(&sbi->bd_mutex);
	seq_printf(seq, "%x\n", bd->encrypt.encrypt_val);
	bd_mutex_unlock(&sbi->bd_mutex);
	return 0;
}

static ssize_t f2fs_bd_encrypt_info_write(struct file *file,
	const char __user *buf,
	size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	char buffer[3] = {0};

	if (!buf || length > 2 || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	bd->encrypt.encrypt_val = 0;
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}

#ifdef CONFIG_F2FS_TURBO_ZONE
static int f2fs_bd_turbo_info_show(struct seq_file *seq, void *p)
{
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);

	bd_mutex_lock(&sbi->bd_mutex);
	/* echo colum indicates:
	 * enable switchable total_segs free_segs
	 * reserved_blks written_valid_blocks
	 * nz_free_segs
	 * tz_bg_gc_count  tz_bg_gc_failed
	 * f2fs_trigger_return_time ioctl_trigger_return_time
	 * ioctl_migrate_blks_nz_tz tz_ioctl_migrate_blks_tz_nz
	 * nz_gc_to_tz_blocks tz_bg_gc_to_nz_blocks
	 * tz_version
	 */

	if (is_tz_existed(sbi) && (sbi->tz_info.enabled ||
		!sbi->tz_info.switchable)) {
		bd->nz_free_segs = FREE_I(sbi)->free_segments -
				(FDEV(F2FS_TURBO_DEV).total_segments -
				sbi->tz_info.total_segs) -
				sbi->tz_info.free_segs;
	} else {
		bd->nz_free_segs = FREE_I(sbi)->free_segments;
	}

	seq_printf(seq, "%u %u %u %u %u %u %u %u %u %lu %lu %lu %lu %lu %lu %u\n",
			sbi->tz_info.enabled ? 1 : 0,
			sbi->tz_info.switchable ? 1 : 0,
			sbi->tz_info.total_segs,
			sbi->tz_info.free_segs,
			sbi->tz_info.reserved_blks,
			sbi->tz_info.written_valid_blocks,
			bd->nz_free_segs,
			bd->tz_bg_gc_cnt,
			bd->tz_bg_gc_fail_cnt,
			bd->tz_f2fs_trigger_return_time,
			bd->tz_ioctl_trigger_return_time,
			bd->tz_ioctl_migrate_blks[NZ_TO_TZ],
			bd->tz_ioctl_migrate_blks[TZ_TO_NZ],
			bd->tz_gc_migrate_blks[NZ_TO_TZ],
			bd->tz_gc_migrate_blks[TZ_TO_NZ],
			f2fs_get_turbo_version(sbi));
	bd_mutex_unlock(&sbi->bd_mutex);

	return 0;
}

static ssize_t f2fs_bd_turbo_info_write(struct file *file,
					 const char __user *buf,
					 size_t length, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct super_block *sb = seq->private;
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct f2fs_bigdata_info *bd = F2FS_BD_STAT(sbi);
	int i;
	char buffer[F2FS_TURBO_BD_RESET_MAX_LEN] = {0};

	if (!buf || length > (F2FS_TURBO_BD_RESET_MAX_LEN - 1) || length <= 0)
		return -EINVAL;

	if (copy_from_user(&buffer, buf, length))
		return -EFAULT;

	if (buffer[0] != '0')
		return -EINVAL;

	bd_mutex_lock(&sbi->bd_mutex);
	bd->tz_f2fs_trigger_return_time = 0;
	bd->tz_ioctl_trigger_return_time = 0;
	bd->tz_bg_gc_cnt = 0;
	bd->tz_bg_gc_fail_cnt = 0;

	for (i = 0; i < NR_TZ_MOVE_DIRECTION; i++) {
		bd->tz_ioctl_migrate_blks[i] = 0;
		bd->tz_gc_migrate_blks[i] = 0;
	}
	bd_mutex_unlock(&sbi->bd_mutex);

	return length;
}
#endif

F2FS_BD_PROC_DEF(bd_base_info);
F2FS_BD_PROC_DEF(bd_discard_info);
F2FS_BD_PROC_DEF(bd_gc_info);
F2FS_BD_PROC_DEF(bd_cp_info);
F2FS_BD_PROC_DEF(bd_fsync_info);
F2FS_BD_PROC_DEF(bd_hotcold_info);
F2FS_BD_PROC_DEF(bd_encrypt_info);
#ifdef CONFIG_F2FS_TURBO_ZONE
F2FS_BD_PROC_DEF(bd_turbo_info);
#endif

static void f2fs_build_bd_stat(struct f2fs_sb_info *sbi)
{
	struct super_block *sb = sbi->sb;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	proc_create_data("bd_base_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_base_info_fops, sb);
	proc_create_data("bd_discard_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_discard_info_fops, sb);
	proc_create_data("bd_cp_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_cp_info_fops, sb);
	proc_create_data("bd_gc_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_gc_info_fops, sb);
	proc_create_data("bd_fsync_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_fsync_info_fops, sb);
	proc_create_data("bd_hotcold_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_hotcold_info_fops, sb);
	proc_create_data("bd_encrypt_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_encrypt_info_fops, sb);
#ifdef CONFIG_F2FS_TURBO_ZONE
	proc_create_data("bd_turbo_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		&f2fs_bd_turbo_info_fops, sb);
#endif
#else
	proc_create_single_data("bd_base_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_base_info_show, sb);
	proc_create_single_data("bd_discard_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_discard_info_show, sb);
	proc_create_single_data("bd_cp_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_cp_info_show, sb);
	proc_create_single_data("bd_gc_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_gc_info_show, sb);
	proc_create_single_data("bd_fsync_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_fsync_info_show, sb);
	proc_create_single_data("bd_hotcold_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_hotcold_info_show, sb);
	proc_create_single_data("bd_encrypt_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_encrypt_info_show, sb);
#ifdef CONFIG_F2FS_TURBO_ZONE
	proc_create_single_data("bd_turbo_info", S_IRUGO | S_IWUGO, sbi->s_proc,
		f2fs_bd_turbo_info_show, sb);
#endif
#endif
}

static void f2fs_unregister_bd_stat(struct f2fs_sb_info *sbi)
{
	remove_proc_entry("bd_base_info", sbi->s_proc);
	remove_proc_entry("bd_discard_info", sbi->s_proc);
	remove_proc_entry("bd_cp_info", sbi->s_proc);
	remove_proc_entry("bd_gc_info", sbi->s_proc);
	remove_proc_entry("bd_fsync_info", sbi->s_proc);
	remove_proc_entry("bd_hotcold_info", sbi->s_proc);
	remove_proc_entry("bd_encrypt_info", sbi->s_proc);
#ifdef CONFIG_F2FS_TURBO_ZONE
	remove_proc_entry("bd_turbo_info", sbi->s_proc);
#endif
}

void f2fs_destroy_bd_info(struct f2fs_sb_info *sbi)
{
	if (sbi->bd_info) {
		kfree(sbi->bd_info);
		sbi->bd_info = NULL;
	}
}
#else /* !CONFIG_F2FS_STAT_FS */
#define f2fs_build_bd_stat
#define f2fs_unregister_bd_stat
#endif

int __init f2fs_init_sysfs(void)
{
	int ret;

	kobject_set_name(&f2fs_kset.kobj, "f2fs");
	f2fs_kset.kobj.parent = fs_kobj;
	ret = kset_register(&f2fs_kset);
	if (ret)
		return ret;

	ret = kobject_init_and_add(&f2fs_feat, &f2fs_feat_ktype,
				   NULL, "features");
	if (ret) {
		kobject_put(&f2fs_feat);
		kset_unregister(&f2fs_kset);
	} else {
		f2fs_proc_root = proc_mkdir("fs/f2fs", NULL);
	}
	return ret;
}

void f2fs_exit_sysfs(void)
{
	kobject_put(&f2fs_feat);
	kset_unregister(&f2fs_kset);
	remove_proc_entry("fs/f2fs", NULL);
	f2fs_proc_root = NULL;
}

int f2fs_register_sysfs(struct f2fs_sb_info *sbi)
{
	struct super_block *sb = sbi->sb;
	int err;

	sbi->s_kobj.kset = &f2fs_kset;
	init_completion(&sbi->s_kobj_unregister);
	err = kobject_init_and_add(&sbi->s_kobj, &f2fs_sb_ktype, NULL,
				"%s", sb->s_id);
	if (err)
		goto put_sb_kobj;

	sbi->s_stat_kobj.kset = &f2fs_kset;
	init_completion(&sbi->s_stat_kobj_unregister);
	err = kobject_init_and_add(&sbi->s_stat_kobj, &f2fs_stat_ktype,
						&sbi->s_kobj, "stat");
	if (err)
		goto put_stat_kobj;

	sbi->s_feature_list_kobj.kset = &f2fs_kset;
	init_completion(&sbi->s_feature_list_kobj_unregister);
	err = kobject_init_and_add(&sbi->s_feature_list_kobj,
					&f2fs_feature_list_ktype,
					&sbi->s_kobj, "feature_list");
	if (err)
		goto put_feature_list_kobj;

	if (f2fs_proc_root)
		sbi->s_proc = proc_mkdir(sb->s_id, f2fs_proc_root);

	if (sbi->s_proc) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		proc_create_data("segment_info", S_IRUGO, sbi->s_proc,
				 &f2fs_seq_segment_info_fops, sb);
		proc_create_data("segment_bits", S_IRUGO, sbi->s_proc,
				 &f2fs_seq_segment_bits_fops, sb);
		f2fs_build_bd_stat(sbi);
		proc_create_data("iostat_info", S_IRUGO, sbi->s_proc,
				&f2fs_seq_iostat_info_fops, sb);
		proc_create_data("victim_bits", S_IRUGO, sbi->s_proc,
				&f2fs_seq_victim_bits_fops, sb);
		proc_create_data("undiscard_info", S_IRUGO, sbi->s_proc,
				&f2fs_seq_undiscard_info_fops, sb);
		proc_create_data("resizf2fs_info", S_IRUGO, sbi->s_proc,
				&f2fs_seq_resizf2fs_info_fops, sb);
#else
		proc_create_single_data("segment_info", S_IRUGO, sbi->s_proc,
				segment_info_seq_show, sb);
		proc_create_single_data("segment_bits", S_IRUGO, sbi->s_proc,
				segment_bits_seq_show, sb);
		f2fs_build_bd_stat(sbi);
		proc_create_single_data("iostat_info", S_IRUGO, sbi->s_proc,
				iostat_info_seq_show, sb);
		proc_create_single_data("victim_bits", S_IRUGO, sbi->s_proc,
				victim_bits_seq_show, sb);
		proc_create_single_data("undiscard_info", S_IRUGO, sbi->s_proc,
				undiscard_info_seq_show, sb);
		proc_create_single_data("resizf2fs_info", S_IRUGO, sbi->s_proc,
				resizf2fs_info_seq_show, sb);
#endif
	}
	return 0;
put_feature_list_kobj:
	kobject_put(&sbi->s_feature_list_kobj);
	wait_for_completion(&sbi->s_feature_list_kobj_unregister);
put_stat_kobj:
	kobject_put(&sbi->s_stat_kobj);
	wait_for_completion(&sbi->s_stat_kobj_unregister);
put_sb_kobj:
	kobject_put(&sbi->s_kobj);
	wait_for_completion(&sbi->s_kobj_unregister);
	return err;
}

void f2fs_unregister_sysfs(struct f2fs_sb_info *sbi)
{
	if (sbi->s_proc) {
		remove_proc_entry("iostat_info", sbi->s_proc);
		remove_proc_entry("segment_info", sbi->s_proc);
		remove_proc_entry("segment_bits", sbi->s_proc);
		remove_proc_entry("victim_bits", sbi->s_proc);
		remove_proc_entry("undiscard_info", sbi->s_proc);
		remove_proc_entry("resizf2fs_info", sbi->s_proc);
		f2fs_unregister_bd_stat(sbi);
		remove_proc_entry(sbi->sb->s_id, f2fs_proc_root);
	}

	kobject_del(&sbi->s_stat_kobj);
	kobject_put(&sbi->s_stat_kobj);
	wait_for_completion(&sbi->s_stat_kobj_unregister);
	kobject_del(&sbi->s_feature_list_kobj);
	kobject_put(&sbi->s_feature_list_kobj);
	wait_for_completion(&sbi->s_feature_list_kobj_unregister);

	kobject_del(&sbi->s_kobj);
	kobject_put(&sbi->s_kobj);
	wait_for_completion(&sbi->s_kobj_unregister);
}
