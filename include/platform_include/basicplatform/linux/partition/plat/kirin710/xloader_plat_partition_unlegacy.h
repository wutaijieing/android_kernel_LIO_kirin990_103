/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: the partition table.
 */

#ifndef _LIBRA_XLOADER_PLAT_PARTITION_H_
#define _LIBRA_XLOADER_PLAT_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define FASTBOOT_VRL_INDEX_A    40
#define VECTOR_VRL_INDEX_A      41
#define BL2_VRL_INDEX           54

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,          0,         2 * 1024,        EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,        3072,      5120,          EMMC_USER_PART}, /* reserved1       5120K  p3 */
    {PART_VRL,              8704,      512,           EMMC_USER_PART}, /* vrl             512K   p5 */
    {PART_VRL_BACKUP,       9216,      512,           EMMC_USER_PART}, /* vrl backup      512K   p6 */
    {PART_NVME,             18 * 1024,   5 * 1024,        EMMC_USER_PART}, /* nvme            5M     p8 */
    {PART_DDR_PARA,         225 * 1024,  1 * 1024,        EMMC_USER_PART}, /* DDR_PARA        1M     p19 */
    {PART_DFX,                 337 * 1024,  16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p30 */
    {PART_HISEE_IMG,           374 * 1024,   4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p35 */
    {PART_HISEE_FS,            382 * 1024,   8 * 1024,    EMMC_USER_PART}, /* hisee_fs          8M     p37 */
    {PART_FASTBOOT,            390 * 1024,  12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p38 */
    {PART_VECTOR,              402 * 1024,   4 * 1024,    EMMC_USER_PART}, /* avs vector        4M     p39 */
    {PART_ISP_BOOT,            406 * 1024,   2 * 1024,    EMMC_USER_PART}, /* isp_boot          2M     p40 */
    {PART_BL2,                 756 * 1024,   4 * 1024,     EMMC_USER_PART}, /* bl2               4M    p52 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,        9768 * 1024,    128 * 1024,    EMMC_USER_PART}, /* hibench_img   128M    p75 */
    {PART_HIBENCH_LOG,       10920 * 1024,     32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG    32M    p78 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,        24 * 1024 * 1024,     192 * 1024,    EMMC_USER_PART}, /* fttest      192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      PART_DDRTEST_START,     48 * 1024,    EMMC_USER_PART}, /* ddrtest      48M */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,          0,         2 * 1024,        UFS_PART_0},
    {PART_RESERVED1,        3072,      5120,          UFS_PART_2}, /* reserved1       5120K  p3 */
    {PART_VRL,              512,       512,           UFS_PART_3}, /* vrl             512K   p1 */
    {PART_VRL_BACKUP,       1024,      512,           UFS_PART_3}, /* vrl backup      512K   p2 */
    {PART_NVME,             10 * 1024,   5 * 1024,        UFS_PART_3}, /* nvme            5M     p4 */
    {PART_DDR_PARA,         217 * 1024,  1 * 1024,        UFS_PART_3}, /* DDR_PARA        1M     p15 */
    {PART_DFX,              329 * 1024,  16 * 1024,       UFS_PART_3}, /* dfx             16M    p26 */
    {PART_HISEE_IMG,        366 * 1024,  4 * 1024,        UFS_PART_3}, /* part_hisee_img   4M    p31 */
    {PART_HISEE_FS,         374 * 1024,  8 * 1024,        UFS_PART_3}, /* hisee_fs        8M     p33 */
    {PART_FASTBOOT,         382 * 1024,  12 * 1024,       UFS_PART_3}, /* fastboot        12M    p34 */
    {PART_VECTOR,           394 * 1024,  4 * 1024,        UFS_PART_3}, /* avs vector      4M     p35 */
    {PART_BL2,                 748 * 1024,   4 * 1024,    UFS_PART_3}, /* bl2              4M    p48 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,        9760 * 1024,   128 * 1024,    UFS_PART_3}, /* hibench_img     128M    p71 */
    {PART_HIBENCH_LOG,       10912 * 1024,    32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG      32M    p74 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,         24 * 1024 * 1024,    192 * 1024,    UFS_PART_3}, /* fttest         192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      PART_DDRTEST_START,   48 * 1024,    UFS_PART_3}, /* ddrtest      48M */
#endif
    {"0", 0, 0, 0},
};

#endif
