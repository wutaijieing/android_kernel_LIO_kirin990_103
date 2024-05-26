/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SWAP_CGROUP_H
#define __LINUX_SWAP_CGROUP_H

#include <linux/swap.h>

#ifdef CONFIG_MEMCG_SWAP

extern unsigned short swap_cgroup_cmpxchg(swp_entry_t ent,
					unsigned short old, unsigned short new);
extern unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id,
					 unsigned int nr_ents);
extern unsigned short lookup_swap_cgroup_id(swp_entry_t ent);
extern int swap_cgroup_swapon(int type, unsigned long max_pages);
extern void swap_cgroup_swapoff(int type);

#else

static inline
unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id,
				  unsigned int nr_ents)
{
	return 0;
}

static inline
unsigned short lookup_swap_cgroup_id(swp_entry_t ent)
{
	return 0;
}

static inline int
swap_cgroup_swapon(int type, unsigned long max_pages)
{
	return 0;
}

static inline void swap_cgroup_swapoff(int type)
{
	return;
}

#endif /* CONFIG_MEMCG_SWAP */

#ifdef CONFIG_HYPERHOLD
static struct mem_cgroup *get_memcg_from_swap_entry(swp_entry_t entry)
{
	struct mem_cgroup *mcg = NULL;
	unsigned short id;

	id = lookup_swap_cgroup_id(entry);
	rcu_read_lock();
	mcg = mem_cgroup_from_id(id);
	if (mcg)
		css_get(&mcg->css);
	rcu_read_unlock();

	return mcg;
}

static void swap_entry_memcg_put(struct mem_cgroup *mcg)
{
	if (mcg)
		css_put(&mcg->css);
}
#else
static inline struct mem_cgroup *get_memcg_from_swap_entry(swp_entry_t entry)
{
	return NULL;
}
static inline void swap_entry_memcg_put(struct mem_cgroup *mcg) {}
#endif

#endif /* __LINUX_SWAP_CGROUP_H */
