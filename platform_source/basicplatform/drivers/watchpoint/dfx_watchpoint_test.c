/*
 * dfx_watchpoint_test.c
 *
 * watchpoint&breakpoint test case
 *
 * Copyright (c) 2010-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <platform_include/basicplatform/linux/util.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <asm/debug-monitors.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <securec.h>

static atomic_t g_watchpt_test_exclusive;
struct perf_event * __percpu *sample_hbp;

void arch_kgdb_breakpoint(void)
{
	asm volatile("brk %0" : : "I" (1));
}

/*
 * watchpoint test:cover ldrx strx instruction
 */
static void watchpt_test_atomic_add(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile(
		"prfm pstl1strm, %2\n"
		"ldxr %w0, %2\n"
		"add %w0, %w0, %w3\n"
		"stxr %w1, %w0, %2\n"
		: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
		: "Ir" (i)
	);
}

void watchpt_test_exclusive(void)
{
	unsigned long flags;

	local_irq_save(flags);
	watchpt_test_atomic_add(1, &g_watchpt_test_exclusive);
	local_irq_restore(flags);
}

void breakpoint_test_stub(void)
{
	pr_err("%s is excuted\n", __func__);
}

static void sample_hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	dump_stack();
	pr_err("%s: Dump stack\n", __func__);
}

int test_call_breakpoint_set(u64 addr_ins)
{
	int ret;
	struct perf_event_attr attr;

	hw_breakpoint_init(&attr);
	attr.bp_addr = addr_ins;
	attr.bp_len = HW_BREAKPOINT_LEN_4;
	attr.bp_type = HW_BREAKPOINT_X;

	sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
	if (IS_ERR((void __force *)sample_hbp)) {
		ret = PTR_ERR((void __force *)sample_hbp);
		goto fail;
	}

	pr_err("%s: HW Breakpoint for write installed\n", __func__);
	return 0;

fail:
	pr_err("%s: Breakpoint registration failed\n", __func__);
	return ret;
}

void test_call_breakpoint_release(void)
{
	unregister_wide_hw_breakpoint(sample_hbp);
	pr_err("%s: HW Breakpoint for write uninstalled\n", __func__);
}
