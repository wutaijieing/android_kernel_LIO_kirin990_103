/*
 * Hisilicon IPP Bl31 Driver
 *
 * Copyright (c) 2017 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/arm-smccc.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>

noinline int atfd_hipp_smc(u64 _funcid, u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(_funcid, _arg0, _arg1, _arg2,
				0, 0, 0, 0, &res);

	return (int)res.a0;
}

int atfhipp_smmu_enable(unsigned int mode)
{
	return atfd_hipp_smc(IPP_FID_SMMUENABLE, mode, 0, 0); /*lint !e570 */
}

int atfhipp_smmu_disable(void)
{
	return atfd_hipp_smc(IPP_FID_SMMUDISABLE, 0, 0, 0); /*lint !e570 */
}

int atfhipp_smmu_smrx(unsigned int sid, unsigned int mode)
{
	return atfd_hipp_smc(IPP_FID_SMMUSMRX, sid, mode, 0); /*lint !e570 */
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hisilicon IPP Bl31 Driver");
MODULE_AUTHOR("isp");
