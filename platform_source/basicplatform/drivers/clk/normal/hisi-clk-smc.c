/*
 * hisi-clk-smc.c
 *
 * hisi sec clk smc cmd support
 *
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
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
#include "hisi-clk-smc.h"
#include <linux/arm-smccc.h>
#include <linux/types.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>

#define clk_smc_e(fmt, args ...) pr_err("[CLK_SMC]"fmt, ##args)

#define clk_smc_i(fmt, args ...) pr_info("[CLK_SMC]"fmt, ##args)

#define clk_smc_d(fmt, args ...) pr_debug("[CLK_SMC]"fmt, ##args)

static DEFINE_SPINLOCK(hisi_clk_smc_lock);

/* atf entrance function support */
noinline int atfd_hisi_service_clk_smc(u64 _arg1, u64 _arg2, u64 _arg3, u64 _arg4)
{
	struct arm_smccc_res res;
	unsigned long flags;

	spin_lock_irqsave(&hisi_clk_smc_lock, flags);

	arm_smccc_smc(HISI_CLK_REGISTER_FN_ID, _arg1, _arg2, _arg3, _arg4, 0, 0, 0, &res);

	spin_unlock_irqrestore(&hisi_clk_smc_lock, flags);

	return (int)res.a0;
}

