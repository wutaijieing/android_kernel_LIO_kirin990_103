/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: uapp source file
 * Create: 2022/04/20
 */

#include <platform_include/see/uapp.h>
#include "uapp_internal.h"
#include <linux/kernel.h>          /* pr_err */
#include <linux/version.h>
#include <linux/arm-smccc.h>       /* arm_smccc_smc */
#include <securec.h>               /* memset_s */

u32 uapp_get_enable_state_tc(void)
{
	uint32_t ret;
	u32 state = 0;

	ret = uapp_get_enable_state(&state);
	if (ret != UAPP_OK) {
		pr_err("[%s]: error, uapp_get_enable_state ret=0x%08X", __func__, ret);
		return ret;
	}

	pr_err("[%s]: enable_state=0x%08X", __func__, state);
	return UAPP_OK;
}