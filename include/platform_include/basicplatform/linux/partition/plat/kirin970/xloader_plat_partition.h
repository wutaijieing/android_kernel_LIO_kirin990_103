/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: the partition table.
 */
#ifndef _ARIES_XLOADER_PARTITION_H_
#define _ARIES_XLOADER_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define FASTBOOT_VRL_INDEX_A    34
#define VECTOR_VRL_INDEX_A      35

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                      0,       2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,                 3072,           5120,    EMMC_USER_PART}, /* reserved1         5120K  p3 */
    {PART_VRL,                       8704,            512,    EMMC_USER_PART}, /* vrl               512K   p5 */
    {PART_VRL_BACKUP,                9216,            512,    EMMC_USER_PART}, /* vrl backup        512K   p6 */
    {PART_NVME,                 18 * 1024,       5 * 1024,    EMMC_USER_PART}, /* nvme              5M     p8 */
    {PART_DDR_PARA,            225 * 1024,       1 * 1024,    EMMC_USER_PART}, /* DDR_PARA          1M     p19 */
    {PART_DFX,                 337 * 1024,      16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p24 */
    {PART_HISEE_IMG,           374 * 1024,       4 * 1024,    EMMC_USER_PART}, /* hisee_img         4M     p29 */
    {PART_HISEE_FS,            382 * 1024,       8 * 1024,    EMMC_USER_PART}, /* hisee_fs          8M     p31 */
    {PART_FASTBOOT,            390 * 1024,      12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p32 */
    {PART_VECTOR,              402 * 1024,       4 * 1024,    EMMC_USER_PART}, /* avs vector        4M     p33 */
    {PART_PATCH,              2264 * 1024,      96 * 1024,    EMMC_USER_PART}, /* patch            96M    p70 */
    {PART_RESERVED6,          7896 * 1024,     128 * 1024,    EMMC_USER_PART}, /* reserved6       128M    p72 */
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,          24 * 1024 * 1024,     192 * 1024,    EMMC_USER_PART},/* fttest           192M*/
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                     0,        2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,                3072,            5120,    UFS_PART_2}, /* reserved1        5120K  p3 */
    {PART_VRL,                       512,             512,    UFS_PART_3}, /* vrl              512K   p1 */
    {PART_VRL_BACKUP,               1024,             512,    UFS_PART_3}, /* vrl backup       512K   p2 */
    {PART_NVME,                10 * 1024,        5 * 1024,    UFS_PART_3}, /* nvme             5M     p4 */
    {PART_DDR_PARA,           217 * 1024,        1 * 1024,    UFS_PART_3}, /* DDR_PARA         1M     p15 */
    {PART_DFX,                329 * 1024,       16 * 1024,    UFS_PART_3}, /* dfx              16M    p20 */
    {PART_HISEE_IMG,          366 * 1024,        4 * 1024,    UFS_PART_3}, /* hisee_img        4M     p25 */
    {PART_HISEE_FS,           374 * 1024,        8 * 1024,    UFS_PART_3}, /* hisee_fs         8M     p27 */
    {PART_FASTBOOT,           382 * 1024,       12 * 1024,    UFS_PART_3}, /* fastboot         12M    p28 */
    {PART_VECTOR,             394 * 1024,        4 * 1024,    UFS_PART_3}, /* avs vector       4M     p29 */
    {PART_PATCH,              2256 * 1024,      96 * 1024,    UFS_PART_3}, /* patch            96M    p66 */
    {PART_RESERVED6,          7888 * 1024,     128 * 1024,    UFS_PART_3}, /* reserved6       128M    p68 */
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,          24 * 1024 * 1024,     192 * 1024,    UFS_PART_3},/* fttest           192M*/
#endif
    {"0", 0, 0, 0},
};

#endif
