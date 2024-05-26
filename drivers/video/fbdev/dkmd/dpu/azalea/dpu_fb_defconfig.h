/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef DPU_FB_DEFCONFIG_H
#define DPU_FB_DEFCONFIG_H

#ifdef CONFIG_CMDLINE_PARSE
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h> // for runmode_is_factory
#endif

#ifdef CONFIG_HISILICON_PLATFORM_HITEST
#include <platform_include/basicplatform/linux/hitest_slt.h> // for is_running_kernel_slt
#endif


/* decoupling cmdline */
static inline int dpu_runmode_is_factory()
{
	int ret = 0;
#ifdef CONFIG_CMDLINE_PARSE
	/*
	 * #define RUNMODE_FLAG_NORMAL            0
	 * #define RUNMODE_FLAG_FACTORY           1
	 */
	if (runmode_is_factory())
		ret = 1;
#endif
	return ret;
}

/* decoupling hitest slt*/
static inline int dpu_is_running_kernel_slt()
{
	int ret = 0;
#ifdef CONFIG_HISILICON_PLATFORM_HITEST
	/* is slt ,return 1
	 * other ,return 0
	 */
	if (is_running_kernel_slt())
		ret = 1;
#endif
	return ret;
}

#endif /* DPU_FB_DEFCONFIG_H */
