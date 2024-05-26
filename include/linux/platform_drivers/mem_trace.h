/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: This file is tracking mem trace.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Create: 2019-5-26
 */

#ifndef _MEM_TRACE_H
#define _MEM_TRACE_H

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/page-flags.h>
#include <linux/printk.h>

#define SLUB_NAME_LEN  64
#define MM_ALLOC_DEBUG_MAX_LATENCY_US	(200 * 1000) /* 200ms */

enum {
	START_TRACK,
	ION_TRACK = START_TRACK,
	SLUB_TRACK,
	LSLUB_TRACK,
	VMALLOC_TRACK,
	CMA_TRACK,
	ZSPAGE_TRACK,
	BUDDY_TRACK,
	SKB_TRACK,
	NR_TRACK,
};

enum {
	SLUB_ALLOC,
	SLUB_FREE,
	NR_SLUB_ID,
};

/*
 * Enum for memory allocation statistics
 */
enum {
	COST_TOTAL,
	COST_FREELIST,
	COST_RECLAIM,
	COST_COMPACT,
	COST_OOM,
	COST_MAX
};

struct mm_slub_detail_info {
	char name[SLUB_NAME_LEN];
	unsigned long active_objs;
	unsigned long num_objs;
	unsigned long active_slabs;
	unsigned long num_slabs;
	unsigned long size; /* total size */
	unsigned int objects_per_slab;
	unsigned int objsize;
};

struct mm_ion_detail_info {
	pid_t pid;
	size_t size;
};

struct mm_vmalloc_detail_info {
	int type;
	size_t size;
};

struct mm_stack_info {
	unsigned long caller;
	atomic_t ref;
};

/*
 * Structure for saving memory allocation latency
 */
struct time_context {
	signed long long stime;
	signed long long etime;
	signed long long cost;
};

#ifdef CONFIG_H_MM_PAGE_TRACE
size_t hisi_get_mem_total(int type);
size_t hisi_get_mem_detail(int type, void *buf, size_t len);
int hisi_page_trace_on(int type, char *name);
int hisi_page_trace_off(int type, char *name);
int hisi_page_trace_open(int type, int subtype);
int hisi_page_trace_close(int type, int subtype);
size_t hisi_page_trace_read(int type,
				struct mm_stack_info *info,
				size_t len, int subtype);
size_t hisi_get_ion_by_pid(pid_t pid);
void set_buddy_track(struct page *page,
				unsigned int order, unsigned long caller);
void set_lslub_track(struct page *page,
				unsigned int order, unsigned long caller);
void alloc_node_func_map(struct pglist_data *pgdat);
void set_alloc_track(unsigned long caller);
void set_free_track(unsigned long caller);
int buddy_track_map(int nid);
int buddy_track_unmap(void);
void hisi_vmalloc_detail_show(void);
void hisi_mem_stats_show(void);
#else
static inline size_t hisi_get_mem_total(int type)
{
	return 0;
}

static inline size_t hisi_get_mem_detail(int type, void *buf, size_t len)
{
	return 0;
}

static inline size_t hisi_get_ion_by_pid(pid_t pid)
{
	return 0;
}

static inline int hisi_page_trace_on(int type, char *name)
{
	return 0;
}

static inline int hisi_page_trace_off(int type, char *name)
{
	return 0;
}

static inline int hisi_page_trace_open(int type, int subtype)
{
	return 0;
}

static inline int hisi_page_trace_close(int type, int subtype)
{
	return 0;
}

static inline size_t hisi_page_trace_read(int type,
				struct mm_stack_info *info,
				size_t len, int subtype)
{
	return 0;
}
static inline void set_buddy_track(struct page *page,
				unsigned int order, unsigned long caller) { }
static inline void set_lslub_track(struct page *page,
				unsigned int order, unsigned long caller) { }
static inline void alloc_node_func_map(struct pglist_data *pgdat) { }
static inline void set_alloc_track(unsigned long caller) { }
static inline void set_free_track(unsigned long caller) { }
static inline int buddy_track_map(int nid)
{
	return 0;
}
static inline int buddy_track_unmap(void)
{
	return 0;
}
static inline void hisi_vmalloc_detail_show(void) { }
static inline void hisi_mem_stats_show(void) { }
#endif

#if defined(CONFIG_H_MM_PAGE_TRACE) && defined(CONFIG_DFX_DEBUG_FS)
extern unsigned int record_threshold;
static inline void mm_record_time_start(gfp_t gfp_mask, gfp_t gfp_need,
	struct time_context *time)
{
	if (!time)
		return;

	if (gfp_mask & gfp_need)
		time->stime = ktime_get();
}
static inline void mm_record_time_end(gfp_t gfp_mask, gfp_t gfp_need,
	struct time_context *time)
{
	if (!time)
		return;

	if (gfp_mask & gfp_need) {
		time->etime = ktime_get();
		if (time->etime > time->stime)
			time->cost += ktime_us_delta(time->etime, time->stime);
	}
}

static inline void mm_record_time_print(gfp_t gfp_mask,
	gfp_t gfp_need, unsigned int order, struct time_context *time, int length __attribute__((unused)))
{
	if (!time)
		return;

	if ((gfp_mask & gfp_need) == 0)
		return;

	if (time[COST_TOTAL].cost > record_threshold)
		printk(KERN_INFO "alloc cost: order-%u total-%lld freelist-%lld reclaim-%lld compact-%lld oom-%lld\n",
			order, time[COST_TOTAL].cost, time[COST_FREELIST].cost,
			time[COST_RECLAIM].cost, time[COST_COMPACT].cost,
			time[COST_OOM].cost);
}
#else

static inline void mm_record_time_start(gfp_t gfp_mask __attribute__((unused)),
	gfp_t gfp_need __attribute__((unused)),
	struct time_context *time __attribute__((unused)))
{ }
static inline void mm_record_time_end(gfp_t gfp_mask __attribute__((unused)),
	gfp_t gfp_need __attribute__((unused)),
	struct time_context *time __attribute__((unused)))
{ }
static inline void mm_record_time_print(gfp_t gfp_mask __attribute__((unused)),
	gfp_t gfp_need __attribute__((unused)),
	unsigned int order __attribute__((unused)),
	struct time_context *time __attribute__((unused)),
	int length __attribute__((unused)))
{ }
#endif /* CONFIG_H_MM_PAGE_TRACE && CONFIG_DFX_DEBUG_FS */

#endif
