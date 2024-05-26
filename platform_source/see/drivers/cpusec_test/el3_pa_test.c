/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: El3 PA Restriction test file
 * Create : 2021/4/20
 */

#include <asm/cputype.h>
#include <platform_include/see/bl31_smc.h>
#include <linux/arm-smccc.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched/mm.h>

#define PA_RESTRICTION_TEST   0xCA000009

static int smc_to_atf(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
				0, 0, 0, 0, &res);

	return (int)res.a0;
};


void el3_pa_test(void)
{
	smc_to_atf(PA_RESTRICTION_TEST, 0, 0, 0);
}