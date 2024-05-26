/*
 * drv_tele_mntn_platform.h
 *
 * mntn platform header
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#ifndef __DRV_TELE_MNTN_PLATFORM_H__
#define __DRV_TELE_MNTN_PLATFORM_H__

#include <drv_tele_mntn_common.h>
#include <global_ddr_map.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/bitops.h>
/* writel(/arch/arm/include/asm/io.h) */
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/hwspinlock.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <soc_sctrl_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_rtctimerwdtv100_interface.h>
#include <m3_rdr_ddr_map.h>

#define TELE_MNTN_AREA_ADDR	RESERVED_LPMX_CORE_PHYMEM_BASE
#define TELE_MNTN_AREA_SIZE	0x80000
#define TELE_MNTN_CUR_CPUID	0 /* IPC_CORE_ACPU */
#define TELE_MNTN_DATA_SECTION
#define TELE_MNTN_CHECK_DDR_SELF_REFRESH	0

#endif
