/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: the partition table.
 */

#ifndef _LIBRA_XLOADER_PLAT_PARTITION_H_
#define _LIBRA_XLOADER_PLAT_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define FASTBOOT_VRL_INDEX_A    40
#define FASTBOOT_VRL_INDEX_B    75
#define VECTOR_VRL_INDEX_A      41
#define VECTOR_VRL_INDEX_B      76
#define BL2_VRL_INDEX           54
#define BL2_VRL_INDEX_B         78

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER_A,        0,         2 * 1024,        EMMC_BOOT_MAJOR_PART},
    {PART_XLOADER_B,        0,         2 * 1024,        EMMC_BOOT_BACKUP_PART},
    {PART_RESERVED1,        3072,      5120,          EMMC_USER_PART}, /* reserved1       5120K  p3 */
    {PART_VRL_A,                 8704,        512,        EMMC_USER_PART}, /* vrl             512K   p5 */
    {PART_VRL_BACKUP_A,          9216,        512,        EMMC_USER_PART}, /* vrl backup      512K   p6 */
    {PART_NVME,             18 * 1024,   5 * 1024,        EMMC_USER_PART}, /* nvme            5M     p8 */
    {PART_DDR_PARA,         225 * 1024,  1 * 1024,        EMMC_USER_PART}, /* DDR_PARA        1M     p19 */
    {PART_DFX,                 337 * 1024,  16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p30 */
    {PART_HISEE_IMG,           374 * 1024,   4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p35 */
    {PART_HISEE_FS,            382 * 1024,   8 * 1024,    EMMC_USER_PART}, /* hisee_fs          8M     p37 */
    {PART_FASTBOOT_A,          390 * 1024,  12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p38 */
    {PART_VECTOR_A,            402 * 1024,   4 * 1024,    EMMC_USER_PART}, /* avs vector        4M     p39 */
    {PART_ISP_BOOT,            406 * 1024,   2 * 1024,    EMMC_USER_PART}, /* isp_boot          2M     p40 */
    {PART_BL2_A,               756 * 1024,   4 * 1024,     EMMC_USER_PART}, /* bl2               4M    p52 */
    {PART_VRL_B,              1104 * 1024,        512,     EMMC_USER_PART}, /* vrl             512K    p69 */
    {PART_VRL_BACKUP_B,           1131008,        512,     EMMC_USER_PART}, /* vrl backup      512K    p70 */
    {PART_FASTBOOT_B,               1135872,  12 * 1024,    EMMC_USER_PART}, /* fastboot        12M    p73 */
    {PART_VECTOR_B,                 1148160,   4 * 1024,    EMMC_USER_PART}, /* avs vector       4M    p74 */
    {PART_BL2_B,                    1160448,   4 * 1024,    EMMC_USER_PART}, /* bl2              4M    p76 */
    {PART_RECOVERYAB_RESERVED,      1209600,  32 * 1024 + 768,    EMMC_USER_PART}, /* recovery_ab_reserved 110M p81 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,        9878 * 1024,          128 * 1024,    EMMC_USER_PART}, /* hibench_img    128M    p87 */
    {PART_HIBENCH_LOG,       11030 * 1024,           32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG     32M    p90 */
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
    {"0", 0, 0, 0},
};

#endif
