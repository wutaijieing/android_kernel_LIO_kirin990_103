/* Copyright (c) Hisilicon Technologies Co., Ltd. 2001-2021. All rights reserved.
 * FileName: ion_alloc_debug.c
 * Description: This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;
 * either version 2 of the License,
 * or (at your option) any later version.
 */

#include "ion_alloc_debug.h"
#include <linux/atomic.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/vmstat.h>
#ifdef CONFIG_SCHED_INFO
#include <linux/sched/cputime.h>
#endif
#include <linux/platform_drivers/mm_ion.h>

#define MM_ION_ALLOC_MAX_LATENCY_US	(500 * 1000) /* 500ms */

extern atomic64_t shrink_msleep_count;

void ion_debug_get_meminfo(struct ion_debug_info *ion_dbg_info)
{
	ion_dbg_info->kernel_stack = global_zone_page_state(NR_KERNEL_STACK_KB);
	ion_dbg_info->free_cma = global_zone_page_state(NR_FREE_CMA_PAGES);
	ion_dbg_info->free_page = global_zone_page_state(NR_FREE_PAGES);
	ion_dbg_info->nr_inactive_file = global_node_page_state(NR_INACTIVE_FILE);
	ion_dbg_info->nr_isolated_anon = global_node_page_state(NR_ISOLATED_ANON);
	ion_dbg_info->nr_isolated_file = global_node_page_state(NR_ISOLATED_FILE);
}

void ion_debug_get_timeinfo(struct ion_debug_info *ion_dbg_info, unsigned int runtime_stage)
{
	switch (runtime_stage) {
	case ION_ALLOC_START:
		ion_dbg_info->msleep_count_s = atomic64_read(&shrink_msleep_count);
		ion_dbg_info->_stime = ktime_get();
		break;
	case ION_ALLOC_HALF:
		ion_dbg_info->_htime = ktime_get();
		ion_dbg_info->_htimedelta = ktime_us_delta(ion_dbg_info->_htime, ion_dbg_info->_stime);
#ifdef CONFIG_SCHED_INFO
		ion_dbg_info->run_time = task_sched_runtime(current);
		ion_dbg_info->run_delay = current->sched_info.run_delay;
#endif
		break;
	case ION_ALLOC_END:
		ion_dbg_info->_etime = ktime_get();
		ion_dbg_info->_timedelta = ktime_us_delta(ion_dbg_info->_etime, ion_dbg_info->_stime);
#ifdef CONFIG_SCHED_INFO
		ion_dbg_info->run_time = task_sched_runtime(current) - ion_dbg_info->run_time;
		ion_dbg_info->run_delay = current->sched_info.run_delay - ion_dbg_info->run_delay;
#endif
		ion_dbg_info->msleep_count_e = atomic64_read(&shrink_msleep_count);
		break;
	default:
		pr_err("%s: Invalid runtime_stage %u\n", __func__, runtime_stage);
	}
}

void _ion_debug_print_info(struct ion_debug_info *ion_dbg_info, unsigned int print_flag)
{
	switch (print_flag) {
	case ION_ALLOC_SIZE_GT_200MB:
		pr_err("ion_alloc_size > 200MB: inactive_file:%lu, isolated_file:%lu, isolated_anon:%lu,"
			"free pages:%lu, kernel_stack:%luKB, free_cma:%lu\n",
			ion_dbg_info->nr_inactive_file, ion_dbg_info->nr_isolated_file,
			ion_dbg_info->nr_isolated_anon, ion_dbg_info->free_page,
			ion_dbg_info->kernel_stack, ion_dbg_info->free_cma);
		break;
	case ION_ALLOC_TIME_GT_100MS:
#ifdef CONFIG_SCHED_INFO
		pr_err("ion_alloc_time > 100MS: PID:%d, process_name:%s,"
			"free_pages:%lu, htime_cost:%lld, time_cost:%lld,"
			"runtime:%llu, run_delay:%llu\n",
			current->pid, current->comm, ion_dbg_info->free_page,
			ion_dbg_info->_htimedelta, ion_dbg_info->_timedelta,
			ion_dbg_info->run_time / 1000, ion_dbg_info->run_delay / 1000); /* 1000:ns To us */
#endif
		break;
	case ION_ALLOC_TIME_GT_500MS:
		pr_err("ion_alloc_time > 500MS: inactive_file:%lu, isolated_file:%lu,"
			"isolated_anon:%lu, free pages:%lu, kernel_stack:%luKB,"
			"free_cma:%lu, msleep_count_s:%ld, msleep_count_e:%ld,"
			"htime cost=%lld, time cost=%lld\n",
			ion_dbg_info->nr_inactive_file, ion_dbg_info->nr_isolated_file,
			ion_dbg_info->nr_isolated_anon, ion_dbg_info->free_page,
			ion_dbg_info->kernel_stack, ion_dbg_info->free_cma,
			ion_dbg_info->msleep_count_s, ion_dbg_info->msleep_count_e,
			ion_dbg_info->_htimedelta, ion_dbg_info->_timedelta);
		break;
	default:
		pr_err("%s: Invalid print_flag %u\n", __func__, print_flag);
	}
}

void ion_debug_print_info(struct ion_debug_info *ion_dbg_info, size_t len,
			  unsigned int heap_id_mask, unsigned int flags)
{
	if (ion_dbg_info->_timedelta > MM_ION_ALLOC_MAX_LATENCY_US) {
		pr_err("%s: ion_alloc timeout, alloc size=%zu, heap_id_mask=0x%x, flags=0x%x,\n",
			__func__, len, heap_id_mask, flags);
		_ion_debug_print_info(ion_dbg_info, ION_ALLOC_TIME_GT_500MS);
		mm_ion_proecss_info();
		show_mem(0, NULL);
	}

#ifdef CONFIG_SCHED_INFO
	if (ion_dbg_info->_timedelta > 100000) { /* 100000 : means 100ms */
		pr_err("%s: alloc size=%zu, heap_id_mask=0x%x, flags=0x%x,\n",
			__func__, len, heap_id_mask, flags);
		_ion_debug_print_info(ion_dbg_info, ION_ALLOC_TIME_GT_100MS);
	}
#endif
}
