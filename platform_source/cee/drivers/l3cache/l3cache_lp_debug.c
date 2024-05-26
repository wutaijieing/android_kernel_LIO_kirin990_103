/*
 * l3cache_lp_debug.c
 *
 * Debug For L3Cache
 *
 * Copyright (c) 2017-2021 Huawei Technologies Co., Ltd.
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
#include <linux/module.h>
#include <linux/platform_drivers/sysreg_funcs_define.h>

#define CLUSTERCFR_EL1				S3_0_C15_C3_0
#define CLUSTERIDR_EL1				S3_0_C15_C3_1
#define CLUSTERREVIDR_EL1			S3_0_C15_C3_2
#define CLUSTERACTLR_EL1			S3_0_C15_C3_3
#define CLUSTERECTLR_EL1			S3_0_C15_C3_4
#define CLUSTERPWRCTLR_EL1			S3_0_C15_C3_5
#define CLUSTERPWRDN_EL1			S3_0_C15_C3_6
#define CLUSTERPWRSTAT_EL1 			S3_0_C15_C3_7
#define CLUSTERTHREADSID_EL1 		S3_0_C15_C4_0
#define CLUSTERACPSID_EL1			S3_0_C15_C4_1
#define CLUSTERSTASHSID_EL1			S3_0_C15_C4_2
#define CLUSTERPARTCR_EL1			S3_0_C15_C4_3
#define CLUSTERBUSQOS_EL1			S3_0_C15_C4_4
#define CLUSTERL3HIT_EL1			S3_0_C15_C4_5
#define CLUSTERL3MISS_EL1			S3_0_C15_C4_6
#define CLUSTERTHREADSIDOVR_EL1		S3_0_C15_C4_7

DEFINE_RENAME_SYSREG_RW_FUNCS(clustercfr_el1, CLUSTERCFR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusteridr_el1, CLUSTERIDR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterrevidr_el1, CLUSTERREVIDR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusteractlr_el1, CLUSTERACTLR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterectlr_el1, CLUSTERECTLR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterpwrctlr_el1, CLUSTERPWRCTLR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterpwrdn_el1, CLUSTERPWRDN_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterpwrstat_el1, CLUSTERPWRSTAT_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterthreadsid_el1, CLUSTERTHREADSID_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusteracpsid_el1, CLUSTERACPSID_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterstashsid_el1, CLUSTERSTASHSID_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterpartcr_el1, CLUSTERPARTCR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterbusqos_el1, CLUSTERBUSQOS_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterl3hit_el1, CLUSTERL3HIT_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterl3miss_el1, CLUSTERL3MISS_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(clusterthreadsidovr_el1, CLUSTERTHREADSIDOVR_EL1)

