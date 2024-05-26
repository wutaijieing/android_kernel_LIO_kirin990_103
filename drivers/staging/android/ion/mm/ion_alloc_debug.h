/* Copyright (c) Hisilicon Technologies Co., Ltd. 2001-2021. All rights reserved.
 * FileName: ion_alloc_debug.h
 * Description: This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;
 * either version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef _ION_ALLOC_DEBUG_H
#define _ION_ALLOC_DEBUG_H

#include <linux/ktime.h>

enum ion_alloc_runtime_stage {
	ION_ALLOC_START,
	ION_ALLOC_HALF,
	ION_ALLOC_END,
};

enum ion_alloc_print_flag {
	ION_ALLOC_SIZE_GT_200MB,
	ION_ALLOC_TIME_GT_100MS,
	ION_ALLOC_TIME_GT_500MS,
};

struct ion_debug_info {
	s64 _timedelta;
	s64 _htimedelta;
	long msleep_count_s;
	long msleep_count_e;
	unsigned long kernel_stack;
	unsigned long free_cma;
	unsigned long free_page;
	unsigned long nr_inactive_file;
	unsigned long nr_isolated_anon;
	unsigned long nr_isolated_file;
#ifdef CONFIG_SCHED_INFO
	unsigned long long run_time;
	unsigned long long run_delay;
#endif
	ktime_t _stime, _etime, _htime;
};

void ion_debug_get_meminfo(struct ion_debug_info *ion_dbg_info);
void ion_debug_get_timeinfo(struct ion_debug_info *ion_dbg_info, unsigned int runtime_stage);
void _ion_debug_print_info(struct ion_debug_info *ion_dbg_info, unsigned int print_flag);
void ion_debug_print_info(struct ion_debug_info *ion_dbg_info, size_t len,
			  unsigned int heap_id_mask, unsigned int flags);

#endif
