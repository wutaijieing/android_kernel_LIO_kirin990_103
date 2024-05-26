/*
 * fs/proc/hisi/lmkd_dbg_trigger.c
 *
 * Copyright(C) 2004-2020 Hisilicon Technologies Co., Ltd. All rights reserved.
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
 */

#define pr_fmt(fmt) "hisi_lowmem: " fmt

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/oom.h>
#include <linux/swap.h>
#include <linux/platform_drivers/lowmem_killer.h>

#define CREATE_TRACE_POINTS
#include "lowmem_trace.h"

#define CMA_TUNE 1

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DAEMON
#define SWAP_RECLAIM_PER 5

#define hisi_lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			pr_info(x);			\
	} while (0)


static u32 lowmem_debug_level = 1;
static unsigned long lowmem_deathpending_timeout;
#endif
atomic_t swap_no_space;
static int nzones;

int hisi_lowmem_tune(int *other_free, const int *other_file,
		     struct shrink_control *sc)
{
	if (!(sc->gfp_mask & ___GFP_CMA)) {
		int nr_free_cma;
		nr_free_cma = (int)global_zone_page_state(NR_FREE_CMA_PAGES);
		trace_lowmem_tune(nzones, sc->gfp_mask, *other_free,
				  *other_file, -nr_free_cma, 0);
		*other_free -= nr_free_cma;

		return CMA_TUNE;
	}

	return 0;
}

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DAEMON
static bool hisi_zram_watermark_ok(void)
{
	if (atomic_read(&swap_no_space))
		return false;

	return true;
}

static unsigned long hisi_lowmem_count(struct shrinker *s,
				struct shrink_control *sc)
{
	long scan_count = 0;

	if (!hisi_zram_watermark_ok())
		scan_count = total_swap_pages * SWAP_RECLAIM_PER / 100;

	return scan_count;
}

static unsigned long hisi_lowmem_scan(struct shrinker *s,
				struct shrink_control *sc)
{
	struct task_struct *tsk = NULL;
	struct task_struct *selected = NULL;
	unsigned long rem = 0;
	int tasksize;
	const short min_score_adj = 100;
	int minfree = 0;
	int selected_tasksize = 0;
	short selected_oom_score_adj;
	int other_free =
		global_zone_page_state(NR_FREE_PAGES) - totalreserve_pages;
	int other_file = global_node_page_state(NR_FILE_PAGES) -
				global_node_page_state(NR_SHMEM) -
				global_node_page_state(NR_UNEVICTABLE) -
				total_swapcache_pages();
	int d_state_count = 0;
	static atomic_t atomic_lmk = ATOMIC_INIT(0);

	if (hisi_zram_watermark_ok())
		return SHRINK_STOP;

	if (atomic_inc_return(&atomic_lmk) > 1) {
		atomic_dec(&atomic_lmk);
		return SHRINK_STOP;
	}

	atomic_set(&swap_no_space, 0);

	selected_oom_score_adj = min_score_adj;

	rcu_read_lock();
	for_each_process(tsk) {
		struct task_struct *p;
		short oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}

		/* Bypass D-state process */
		if ((unsigned long)p->state & TASK_UNINTERRUPTIBLE) {
			hisi_lowmem_print(2,
				     "hisi_lowmem_scan filter D state process: %d (%s) state:0x%lx\n",
				     p->pid, p->comm, p->state);
			task_unlock(p);
			d_state_count++;
			continue;
		}

		if (task_lmk_waiting(p)) {
			if (time_before_eq(jiffies,
						lowmem_deathpending_timeout)) {
				task_unlock(p);
				rcu_read_unlock();
				atomic_dec(&atomic_lmk);
				return SHRINK_STOP;
			} else {
				hisi_lowmem_dbg_timeout(tsk, p);
			}
		}

		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
		hisi_lowmem_print(1, "select '%s' (%d), adj %hd, size %d, to kill\n",
			     p->comm, p->pid, oom_score_adj, tasksize);
	}
	if (selected) {
		long cache_size = other_file * (long)(PAGE_SIZE / 1024);
		long cache_limit = minfree * (long)(PAGE_SIZE / 1024);
		long free = other_free * (long)(PAGE_SIZE / 1024);

		task_lock(selected);
		send_sig(SIGKILL, selected, 0);
		if (selected->mm)
			task_set_lmk_waiting(selected);
		task_unlock(selected);
		hisi_lowmem_print(1, "Killing '%s' (%d) (tgid %d), adj %hd,\n"
				 "   to free %ldkB on behalf of '%s' (%d) because\n"
				 "   cache %ldkB is below limit %ldkB for oom_score_adj %hd\n"
				 "   Free memory is %ldkB above reserved (0x%x)\n",
			     selected->comm, selected->pid, selected->tgid,
			     selected_oom_score_adj,
			     selected_tasksize * (long)(PAGE_SIZE / 1024),
			     current->comm, current->pid,
			     cache_size, cache_limit,
			     min_score_adj,
			     free, sc->gfp_mask);
		hisi_lowmem_dbg(selected_oom_score_adj);

		lowmem_deathpending_timeout = jiffies + HZ;
		rem += selected_tasksize;
	} else {
		if (d_state_count) {
			hisi_lowmem_print(1,
				     "No selected (Scanned [%d] D-state processes at %d)\n",
				     d_state_count, (int)min_score_adj);
			/* Set param > 0, so that hisi_lowmem_dbg() won't print too often */
			hisi_lowmem_dbg(min_score_adj);
		}
	}

	hisi_lowmem_print(4, "hisi_lowmem_scan %lu, %x, return %lu\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);

	rcu_read_unlock();
	atomic_dec(&atomic_lmk);

	if (!rem)
		rem = SHRINK_STOP;

	return rem;
}

static struct shrinker hisi_lowmem_shrinker = {
	.scan_objects = hisi_lowmem_scan,
	.count_objects = hisi_lowmem_count,
	.seeks = DEFAULT_SEEKS * 16
};
#endif

static int __init hisi_lowmem_init(void)
{
	struct zone *zone = NULL;

	for_each_populated_zone(zone)
		nzones++;

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_DAEMON
	register_shrinker(&hisi_lowmem_shrinker);
#endif

	return 0;
}
device_initcall(hisi_lowmem_init);
