/*
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "clk-hvc.h"
#include <linux/arm-smccc.h>
#include <linux/types.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>

static DEFINE_SPINLOCK(clk_smc_lock);
/* el3 entrance function support */
noinline int uvmm_service_clk_hvc(u64 _arg1, u64 _arg2, u64 _arg3, u64 _arg4)
{
	struct arm_smccc_res res = {0};
	unsigned long flags;

	spin_lock_irqsave(&clk_smc_lock, flags);

	arm_smccc_hvc(CLK_HVC_FN_ID, _arg1, _arg2, _arg3, _arg4, 0, 0, 0, &res);

	spin_unlock_irqrestore(&clk_smc_lock, flags);

	return (int)res.a0;
}
