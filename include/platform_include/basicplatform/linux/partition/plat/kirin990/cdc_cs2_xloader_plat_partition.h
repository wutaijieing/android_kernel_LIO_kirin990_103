#ifndef _TAURUS_CDC_CS2_XLOADER_PLAT_PARTITION_H_
#define _TAURUS_CDC_CS2_XLOADER_PLAT_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define XLOADER3_VRL_INDEX_A    34
#define BL2_VRL_INDEX           36
#define FASTBOOT_VRL_INDEX_A    29
#define VECTOR_VRL_INDEX_A      51
#define XLOADER3_VRL_INDEX_B    38
#define BL2_VRL_INDEX_B         37
#define FASTBOOT_VRL_INDEX_B    30
#define VECTOR_VRL_INDEX_B      52

static const struct partition partition_table_emmc[] = {
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER_A,                0,          2 * 1024,    UFS_PART_0},
    {PART_XLOADER_B,                0,          2 * 1024,    UFS_PART_1},
    {PART_RESERVED1,           7 * 1024,            1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_VRL_A,                  512,             512,    UFS_PART_3}, /* vrl_a            512K    p1 */
    {PART_VRL_B,                512 * 2,             512,    UFS_PART_3}, /* vrl_b            512K    p2 */
    {PART_VRL_BACKUP_A,         512 * 3,             512,    UFS_PART_3}, /* vrl backup_a     512K    p3 */
    {PART_VRL_BACKUP_B,         512 * 4,             512,    UFS_PART_3}, /* vrl backup_b     512K    p4 */
    {PART_BOOT_CTRL,            5 * 512,             512,    UFS_PART_3}, /* boot ctrl          512K    p5 */
    {PART_NVME,               10 * 1024,          5 * 1024,    UFS_PART_3}, /* nvme                 5M    p7 */
    {PART_DDR_PARA,          215 * 1024,          1 * 1024,    UFS_PART_3}, /* DDR_PARA             1M    p27 */
    {PART_LOWPOWER_PARA_A,   216 * 1024,          1 * 1024,    UFS_PART_3}, /* lowpower_para_a      1M    p28 */
    {PART_LOWPOWER_PARA_B,   226 * 1024,          1 * 1024,    UFS_PART_3}, /* lowpower_para_b      1M    p32 */
    {PART_BL2_A,             218 * 1024,          4 * 1024,    UFS_PART_3}, /* bl2_a                4M    p30 */
    {PART_BL2_B,             222 * 1024,          4 * 1024,    UFS_PART_3}, /* bl2_b                4M    p31 */
    {PART_DFX,               327 * 1024,         16 * 1024,    UFS_PART_3}, /* dfx                 16M    p38 */
    {PART_HISEE_IMG_A,       170 * 1024,          4 * 1024,    UFS_PART_3}, /* part_hisee_img_a     4M    p19 */
    {PART_HISEE_IMG_B,       174 * 1024,          4 * 1024,    UFS_PART_3}, /* part_hisee_img_b     4M    p20 */
    {PART_HISEE_FS_A,        465 * 1024,          8 * 1024,    UFS_PART_3}, /* hisee_fs_a           8M    p43 */
    {PART_HISEE_FS_B,        473 * 1024,          8 * 1024,    UFS_PART_3}, /* hisee_fs_b           8M    p44 */
    {PART_FASTBOOT_A,        186 * 1024,         12 * 1024,    UFS_PART_3}, /* fastboot_a          12M    p23 */
    {PART_FASTBOOT_B,        198 * 1024,         12 * 1024,    UFS_PART_3}, /* fastboot_b          12M    p24 */
    {PART_VECTOR_A,          481 * 1024,          4 * 1024,    UFS_PART_3}, /* vector_a             4M    p45 */
    {PART_VECTOR_B,          485 * 1024,          4 * 1024,    UFS_PART_3}, /* vector_b             4M    p46 */
    {PART_HIBENCH_IMG,     23880 * 1024,        128 * 1024,    UFS_PART_3}, /* hibench_img        128M    p110 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_LOG,     25032 * 1024,         32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG         32M    p113 */
#endif

#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,         PART_DDRTEST_START,       48 * 1024,    UFS_PART_3}, /* ddrtest           48M */
#endif
    {"0", 0, 0, 0},
};

#endif
