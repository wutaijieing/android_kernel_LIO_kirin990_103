/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: hyperhold header file
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author:	He Biao <hebiao6@huawei.com>
 *		Wang Cheng Ke <wangchengke2@huawei.com>
 *		Wang Fa <fa.wang@huawei.com>
 *
 * Create: 2020-4-16
 *
 */

#ifndef HYPERHOLD_H
#define HYPERHOLD_H

#ifdef CONFIG_HP_CORE
extern void hyperhold_init(struct zram *zram);
extern void hyperhold_track(struct zram *zram,
			u32 index, struct mem_cgroup *memcg);
#ifdef CONFIG_CRYPTO_DELTA
extern void hyperhold_handle_recompressed_index(struct zram *zram, u32 index,
			u32 size_diff);
#endif
extern void hyperhold_untrack(struct zram *zram, u32 index);
extern unsigned long hyperhold_reclaim_in(unsigned long size);
extern int hyperhold_batch_out(struct mem_cgroup *mcg,
				unsigned long size, bool preload);
extern int hyperhold_fault_decomp(struct zram *zram, u32 index,
				struct page *page);
extern int hyperhold_fault_out(struct zram *zram, u32 index);
extern bool hyperhold_delete(struct zram *zram, u32 index);
extern void hyperhold_mem_cgroup_remove(struct mem_cgroup *memcg);
extern unsigned long zram_zsmalloc(struct zs_pool *zs_pool,
					size_t size, gfp_t gfp);
#ifdef CONFIG_DFX_DEBUG_FS
extern ssize_t hyperhold_ft_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len);
extern ssize_t hyperhold_ft_show(struct device *dev,
	struct device_attribute *attr, char *buf);
extern ssize_t hyperhold_discard_show(struct device *dev,
	struct device_attribute *attr, char *buf);
#endif
extern ssize_t hyperhold_cache_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len);
extern ssize_t hyperhold_cache_show(struct device *dev,
	struct device_attribute *attr, char *buf);
extern ssize_t hyperhold_report_show(struct device *dev,
	struct device_attribute *attr, char *buf);
extern ssize_t hyperhold_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len);
extern ssize_t hyperhold_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf);
extern void hyperhold_force_reclaim(struct mem_cgroup *mcg);
extern struct task_struct *get_task_from_proc(struct inode *inode);

extern void hyperhold_idle_inc(struct zram *zram, u32 index);
extern void hyperhold_idle_dec(struct zram *zram, u32 index);
extern void hyperhold_idle_reclaim(void);
extern void hyperhold_get_page_stat(u64 *orig, u64 *compr);

#else
static inline void hyperhold_init(struct zram *zram) {}
static inline void hyperhold_track(struct zram *zram, u32 index,
					struct mem_cgroup *mem_cgroup) {}
#ifdef CONFIG_CRYPTO_DELTA
static inline void hyperhold_handle_recompressed_index(struct zram *zram,
					u32 index, u32 size_diff) {}
#endif
static inline void hyperhold_untrack(struct zram *zram, u32 index) {}
static inline unsigned long hyperhold_reclaim_in(unsigned long size)
{
	return 0;
}
static inline int hyperhold_batch_out(struct mem_cgroup *mcg,
				unsigned long size, bool preload)
{
	return 0;
}
static inline int hyperhold_fault_decomp(struct zram *zram, u32 index,
				struct page *page)
{
	return -ENOENT;
}
static inline int hyperhold_fault_out(struct zram *zram, u32 index)
{
	return 0;
}
static inline bool hyperhold_delete(struct zram *zram, u32 index)
{
	return true;
}
static inline void hyperhold_mem_cgroup_remove(struct mem_cgroup *memcg) {}
static inline ssize_t hyperhold_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	return len;
}
static inline ssize_t hyperhold_cache_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	return len;
}
static inline ssize_t hyperhold_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}
static inline ssize_t hyperhold_cache_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}
static inline void hyperhold_force_reclaim(struct mem_cgroup *mcg) {}
#ifdef CONFIG_DFX_DEBUG_FS
static inline  ssize_t hyperhold_ft_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len)
{
	return len;
}

static inline ssize_t hyperhold_ft_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}

static inline ssize_t hyperhold_discard_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}
#endif

static inline ssize_t hyperhold_report_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}

static inline void hyperhold_idle_inc(struct zram *zram, u32 index) {}
static inline void hyperhold_idle_dec(struct zram *zram, u32 index) {}
static inline void hyperhold_idle_reclaim(void) {}
static inline void hyperhold_get_page_stat(u64 *orig, u64 *compr) {}
#endif


#endif
