/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: bl32 init and other fucntions going to thee.
 * Create: 2020/5/16
 */

#include <asm/cputype.h>
#include <platform_include/see/bl31_smc.h>
#include <linux/arm-smccc.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched/mm.h>

static inline int smc_to_atf(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
				0, 0, 0, 0, &res);

	return (int)res.a0;
};

/* eng use ecall test */
static void thee_call(void *cpu __attribute__((__unused__)))
{
	smc_to_atf(THEE_REQ_VM2, 0, 0, 0);
}

void thee_test(void)
{
	int cpu;
	cpumask_t mask;

	cpumask_clear(&mask);

	preempt_disable();

	for_each_online_cpu (cpu) /*lint !e713 !e574*/
		cpumask_set_cpu(cpu, &mask);

	on_each_cpu_mask(&mask, thee_call, NULL, 1);

	preempt_enable();
}

static int __init theed_init(void)
{
	return 0;
}

MODULE_LICENSE("GPL");
arch_initcall(theed_init);
