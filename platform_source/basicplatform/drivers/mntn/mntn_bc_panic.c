/*
 * mntn_bc_panic.c
 *
 * big core panic debug. (kernel run data recorder.)
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include <asm/pgtable.h>
#include <linux/mm_types.h>
#include <linux/memblock.h>
#include <linux/percpu.h>
#include <linux/printk.h>
#include <platform_include/basicplatform/linux/mntn_dump.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/dfx_bootup_keypoint.h>
#include <mntn_public_interface.h>
#define PR_LOG_TAG MNTN_BC_PANIC_TAG

#define BIGCORE_BIT_MASK 0xF0
/*
 * Function: check if it's in cpu booting stage on big core and set big_cpu_panic flag
 * in mntm dump
 * Notes: big core is inited in smp_init.
 */
static int bc_panic_notify(struct notifier_block *nb, unsigned long event, void *buf)
{
	struct mdump_bc_panic *cb = NULL;
	unsigned int this_cpu = raw_smp_processor_id();

	/* if kernel boot is completed, ignore the panic */
	if (is_bootanim_completed())
		return 0;

	/* check if it's bit core */
	if (!(BIGCORE_BIT_MASK & (1 << this_cpu)))
		return 0;

	/* get reserve memory from mntn dump */
	if (register_mntn_dump
	    (MNTN_DUMP_BC_PANIC, sizeof(*cb), (void **)&cb)) {
		pr_err("%s: fail to get reserve memory\r\n", __func__);
		return 0;
	}

	cb->panic_flag = BIGCORE_PANIC_MAGIC;
	return 0;
}

static struct notifier_block bc_panic_notify_block = {
	.notifier_call = bc_panic_notify,
	.priority = INT_MIN,
};

static int __init bc_panic_init(void)
{
	atomic_notifier_chain_register(&panic_notifier_list,
				       &bc_panic_notify_block);
	return 0;
}

early_initcall(bc_panic_init);

MODULE_LICENSE("GPL");
