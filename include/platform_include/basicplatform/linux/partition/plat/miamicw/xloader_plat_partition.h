/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: xloader partition table
 * Create: 2020-03-10
 */

#ifndef _LIBRA_CW_XLOADER_PARTITION_H_
#define _LIBRA_CW_XLOADER_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define FASTBOOT_VRL_INDEX_A    36
#define VECTOR_VRL_INDEX_A      37
#define BL2_VRL_INDEX           50

static const struct partition partition_table_emmc[] =
{
    {PART_XLOADER,                  0,   2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,             3072,       5120,    EMMC_USER_PART}, /* reserved1      5120K    p3 */
    {PART_VRL,                   8704,        512,    EMMC_USER_PART}, /* VRL             512K    p5 */
    {PART_VRL_BACKUP,            9216,        512,    EMMC_USER_PART}, /* VRL backup      512K    p6 */
    {PART_NVME,             18 * 1024,   5 * 1024,    EMMC_USER_PART}, /* nvme              5M    p8 */
    {PART_DDR_PARA,        197 * 1024,   1 * 1024,    EMMC_USER_PART}, /* ddr_para          1M    p18 */
    {PART_DFX,             289 * 1024,  16 * 1024,    EMMC_USER_PART}, /* dfx              16M    p28 */
    {PART_FASTBOOT,        433 * 1024,  12 * 1024,    EMMC_USER_PART}, /* fastboot         12M    p34 */
    {PART_VECTOR,          445 * 1024,   1 * 1024,    EMMC_USER_PART}, /* vector            1M    p35 */
    {PART_BL2,            1080 * 1024,   4 * 1024,    EMMC_USER_PART}, /* bl2               4M    p48 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,      13272 * 1024,      128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p70 */
    {PART_HIBENCH_LOG,      14424 * 1024,       32 * 1024,    EMMC_USER_PART}, /* hibench_log       32M    p73 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST, 14424 * 1024 + 512 * 1024, 192 * 1024, EMMC_USER_PART}, /* fttest    192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST, 668 * 1024 + 13312 * 1024 + 512 * 1024, 48 * 1024, EMMC_USER_PART}, /* ddrtest   48M */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] =
{
    {PART_XLOADER,                 0,     2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,            3072,         5120,    UFS_PART_2}, /* reserved1       5M    p3 */
    {PART_VRL,                   512,          512,    UFS_PART_3}, /* VRL           512K    p1 */
    {PART_VRL_BACKUP,           1024,          512,    UFS_PART_3}, /* VRL backup    512K    p2 */
    {PART_NVME,            10 * 1024,     5 * 1024,    UFS_PART_3}, /* nvme            6M    p4 */
    {PART_DDR_PARA,       189 * 1024,     1 * 1024,    UFS_PART_3}, /* ddr_para        1M    p14 */
    {PART_DFX,            281 * 1024,    16 * 1024,    UFS_PART_3}, /* dfx            16M    p24 */
    {PART_FASTBOOT,       425 * 1024,    12 * 1024,    UFS_PART_3}, /* fastboot       12M    p30 */
    {PART_VECTOR,         437 * 1024,     1 * 1024,    UFS_PART_3}, /* vector          1M    p31 */
    {PART_BL2,           1072 * 1024,     4 * 1024,    UFS_PART_3}, /* bl2             4M    p44 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,     13264 * 1024,       128 * 1024,    UFS_PART_3}, /* hibench_img         128M    p66 */
    {PART_HIBENCH_LOG,     14416 * 1024,        32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG          32M    p69 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST, 14416 * 1024 + 512 * 1024, 192 * 1024, UFS_PART_3}, /* fttest    192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST, 668 * 1024 + 13312 * 1024 + 512 * 1024, 48 * 1024, UFS_PART_3}, /* ddrtest   48M */
#endif
    {"0", 0, 0, 0},
};
#endif
