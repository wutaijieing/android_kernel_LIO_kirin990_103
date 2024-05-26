/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *  Description: the xloader partition table.
 */

#ifndef _TAURUS_CDC_ACE_CS2_XLOADER_PLAT_PARTITION_H_
#define _TAURUS_CDC_ACE_CS2_XLOADER_PLAT_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"

#define XLOADER3_VRL_INDEX_A    13
#define BL2_VRL_INDEX           14
#define FASTBOOT_VRL_INDEX_A    12
#define VECTOR_VRL_INDEX_A      18
#define XLOADER3_VRL_INDEX_B    65
#define BL2_VRL_INDEX_B         66
#define FASTBOOT_VRL_INDEX_B    64
#define VECTOR_VRL_INDEX_B      70

static const struct partition partition_table_emmc[] = {
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER_A,                        0,      2 * 1024,      UFS_PART_0},
    {PART_XLOADER_B,                        0,      2 * 1024,      UFS_PART_1},
    {PART_RESERVED1,                  3 * 512,           512,      UFS_PART_2}, /* reserved1          512K    p3 */
    {PART_VRL_A,                          512,           512,      UFS_PART_2}, /* vrl_a              512K    p1 */
    {PART_VRL_B,                          512,           512,      UFS_PART_3}, /* vrl_b              512K    p1 */
    {PART_VRL_BACKUP_A,               2 * 512,           512,      UFS_PART_2}, /* vrl backup_a       512K    p2 */
    {PART_VRL_BACKUP_B,               2 * 512,           512,      UFS_PART_3}, /* vrl backup_b       512K    p2 */
    {PART_BOOT_CTRL,                 7 * 1024,      1 * 1024,      UFS_PART_4}, /* boot ctrl          512K    p3 */
    {PART_NVME,                      8 * 1024,      5 * 1024,      UFS_PART_4}, /* nvme                 5M    p4 */
    {PART_DDR_PARA,                147 * 1024,      1 * 1024,      UFS_PART_4}, /* DDR_PARA             1M    p10 */
    {PART_LOWPOWER_PARA_A,          35 * 1024,      1 * 1024,      UFS_PART_2}, /* lowpower_para_a      1M    p11 */
    {PART_LOWPOWER_PARA_B,          35 * 1024,      1 * 1024,      UFS_PART_3}, /* lowpower_para_b      1M    p11 */
    {PART_BL2_A,                    36 * 1024,      4 * 1024,      UFS_PART_2}, /* bl2_a                4M    p12 */
    {PART_BL2_B,                    36 * 1024,      4 * 1024,      UFS_PART_3}, /* bl2_b                4M    p12 */
    {PART_DFX,                     234 * 1024,     16 * 1024,      UFS_PART_4}, /* dfx                 16M    p16 */
    {PART_HISEE_IMG_A,              15 * 1024,      4 * 1024,      UFS_PART_2}, /* part_hisee_img_a     4M    p8 */
    {PART_HISEE_IMG_B,              15 * 1024,      4 * 1024,      UFS_PART_3}, /* part_hisee_img_b     4M    p8 */
    {PART_HISEE_FS_A,               49 * 1024,      8 * 1024,      UFS_PART_2}, /* hisee_fs_a           8M    p15 */
    {PART_HISEE_FS_B,               49 * 1024,      8 * 1024,      UFS_PART_3}, /* hisee_fs_b           8M    p15 */
    {PART_FASTBOOT_A,               23 * 1024,     12 * 1024,      UFS_PART_2}, /* fastboot_a          12M    p10 */
    {PART_FASTBOOT_B,               23 * 1024,     12 * 1024,      UFS_PART_3}, /* fastboot_b          12M    p10 */
    {PART_VECTOR_A,                 57 * 1024,      4 * 1024,      UFS_PART_2}, /* vector_a             4M    p16 */
    {PART_VECTOR_B,                 57 * 1024,      4 * 1024,      UFS_PART_3}, /* vector_b             4M    p16 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,           19152 * 1024,    128 * 1024,      UFS_PART_4}, /* hibench_img        128M    p20 */
    {PART_HIBENCH_LOG,           20304 * 1024,     32 * 1024,      UFS_PART_4}, /* HIBENCH_LOG         32M    p23 */
#endif

    {"0", 0, 0, 0},
};

#endif
