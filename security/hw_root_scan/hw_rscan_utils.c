/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: the hw_rscan_utils.c - get current run mode, eng or user
 * Create: 2016-06-18
 */

#include "./include/hw_rscan_utils.h"

int get_ro_secure(void)
{
#ifdef CONFIG_HW_ROOT_SCAN_ENG_DEBUG
	return RO_SECURE;
#else
	return RO_NORMAL;
#endif
}

