/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: the partition table.
 */

#ifndef _SCORPIO_XLOADER_PARTITION_H_
#define _SCORPIO_XLOADER_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define XLOADER3_VRL_INDEX_A    21
#define BL2_VRL_INDEX           32
#define FASTBOOT_VRL_INDEX_A    43
#define VECTOR_VRL_INDEX_A      44

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                0,             2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,              7 * 1024,          1024,    EMMC_USER_PART}, /* reserved1       1024K    p3 */
    {PART_VRL,                    8704,               512,    EMMC_USER_PART}, /* vrl              512K    p5 */
    {PART_VRL_BACKUP,             9 * 1024,           512,    EMMC_USER_PART}, /* vrl backup       512K    p6 */
    {PART_NVME,                   18 * 1024,     5 * 1024,    EMMC_USER_PART}, /* nvme               5M    p8 */
    {PART_DDR_PARA,               235 * 1024,    1 * 1024,    EMMC_USER_PART}, /* DDR_PARA           1M    p17 */
    {PART_LOWPOWER_PARA,          236 * 1024,    1 * 1024,    EMMC_USER_PART}, /* lowpower_para      1M    p18 */
    {PART_DFX,                    343 * 1024,   16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p24 */
    {PART_BL2,                    500 * 1024,    4 * 1024,    EMMC_USER_PART}, /* bl2                4M    p30 */
    {PART_FW_CPU_LPCTRL,       (504 * 1024 + 512),    256,    EMMC_USER_PART}, /* fw_cpu_lpctrl    256K    p32 */
    {PART_FW_GPU_LPCTRL,       (504 * 1024 + 768),    128,    EMMC_USER_PART}, /* fw_gpu_lpctrl    128K    p33 */
    {PART_FW_DDR_LPCTRL,       (504 * 1024 + 896),    128,    EMMC_USER_PART}, /* fw_ddr_lpctrl    128K    p34 */
    {PART_HISEE_IMG,              516 * 1024,    4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p38 */
    {PART_HISEE_FS,               524 * 1024,    8 * 1024,    EMMC_USER_PART}, /* hisee_fs           8M    p40 */
    {PART_FASTBOOT,               532 * 1024,   12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p41 */
    {PART_VECTOR,                 544 * 1024,    4 * 1024,    EMMC_USER_PART}, /* vector             4M    p42 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,           8984 * 1024,  128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p85 */
    {PART_HIBENCH_LOG,          10136 * 1024,   32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG       32M    p88 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,           24 * 1024 * 1024,  192 * 1024,    EMMC_USER_PART}, /* fttest           192M    p66 */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,  788 * 1024 + (0x600000000 >> 10),  48 * 1024,    EMMC_USER_PART}, /* ddrtest  48M */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                0,              2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,              7 * 1024,           1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_VRL,                    512,                 512,    UFS_PART_3}, /* vrl              512K    p1 */
    {PART_VRL_BACKUP,             1024,                512,    UFS_PART_3}, /* vrl backup       512K    p2 */
    {PART_NVME,                   10 * 1024,      5 * 1024,    UFS_PART_3}, /* nvme               5M    p4 */
    {PART_DDR_PARA,               227 * 1024,     1 * 1024,    UFS_PART_3}, /* DDR_PARA           1M    p13 */
    {PART_LOWPOWER_PARA,          228 * 1024,     1 * 1024,    UFS_PART_3}, /* lowpower_para      1M    p14 */
    {PART_DFX,                    335 * 1024,    16 * 1024,    UFS_PART_3}, /* dfx               16M    p20 */
    {PART_BL2,                    492 * 1024,     4 * 1024,    UFS_PART_3}, /* bl2                4M    p26 */
    {PART_FW_CPU_LPCTRL,         (496 * 1024 + 512),   256,    UFS_PART_3}, /* fw_cpu_lpctrl    256K    p28 */
    {PART_FW_GPU_LPCTRL,         (496 * 1024 + 768),   128,    UFS_PART_3}, /* fw_gpu_lpctrl    128K    p29 */
    {PART_FW_DDR_LPCTRL,         (496 * 1024 + 896),   128,    UFS_PART_3}, /* fw_ddr_lpctrl    128K    p30 */
    {PART_HISEE_IMG,              508 * 1024,     4 * 1024,    UFS_PART_3}, /* part_hisee_img     4M    p34 */
    {PART_HISEE_FS,               516 * 1024,     8 * 1024,    UFS_PART_3}, /* hisee_fs           8M    p36 */
    {PART_FASTBOOT,               524 * 1024,    12 * 1024,    UFS_PART_3}, /* fastboot          12M    p37 */
    {PART_VECTOR,                 536 * 1024,     4 * 1024,    UFS_PART_3}, /* vector             4M    p38 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,           8976 * 1024,   128 * 1024,    UFS_PART_3}, /* hibench_img      128M    p81 */
    {PART_HIBENCH_LOG,          10128 * 1024,    32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG       32M    p84 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,          24 * 1024 * 1024,   192 * 1024,    UFS_PART_3}, /* fttest           192M    p62 */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,  788 * 1024 + (0x600000000 >> 10),  48 * 1024,    UFS_PART_3}, /* ddrtest  48M */
#endif
    {"0", 0, 0, 0},
};

#endif
