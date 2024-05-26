/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_SMT_H
#define _LINUX_SCHED_SMT_H

#include <linux/static_key.h>

enum smt_mode {
	ST_MODE = 0,
	ST_ISOLATION_MODE = 0,
	ST_HOTPULG_MODE = 0,
	SMT_MODE = 1,
	NR_SMT_MODE,
	NO_SMT_MODE,
};

#ifdef CONFIG_SCHED_SMT
extern struct static_key_false sched_smt_present;

static __always_inline bool sched_smt_active(void)
{
	return static_branch_likely(&sched_smt_present);
}
enum smt_mode get_smt_mode(int cpu, bool is_current);
#else
static inline bool sched_smt_active(void) { return false; }
static inline enum smt_mode get_smt_mode(int cpu, bool is_current) { return ST_MODE; }
#endif

void arch_smt_update(void);

#endif
