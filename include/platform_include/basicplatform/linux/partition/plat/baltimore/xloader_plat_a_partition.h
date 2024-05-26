#ifndef _PISCES_XLOADER_PLAT_PARTITION_H_
#define _PISCES_XLOADER_PLAT_PARTITION_H_

#include "partition_macro.h"


#define XLOADER3_VRL_INDEX_A    20
#define BL2_VRL_INDEX           32
#define FASTBOOT_VRL_INDEX_A    41
#define VECTOR_VRL_INDEX_A      42

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                 0,                   2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,               7 * 1024,                1024,    EMMC_USER_PART}, /* reserved1     1024K    p3 */
    {PART_VRL,                     8704,                     512,    EMMC_USER_PART}, /* vrl            512K    p5 */
    {PART_VRL_BACKUP,              9 * 1024,                 512,    EMMC_USER_PART}, /* vrl backup     512K    p6 */
    {PART_NVME,                    18 * 1024,           5 * 1024,    EMMC_USER_PART}, /* nvme             5M    p8 */
    {PART_DDR_PARA,                235 * 1024,          1 * 1024,    EMMC_USER_PART}, /* DDR_PARA         1M    p17 */
    {PART_LOWPOWER_PARA,           236 * 1024,          1 * 1024,    EMMC_USER_PART}, /* lowpower_para    1M    p18 */
    {PART_DFX,                     343 * 1024,         16 * 1024,    EMMC_USER_PART}, /* dfx             16M    p24 */
    {PART_BL2,                     500 * 1024,          4 * 1024,    EMMC_USER_PART}, /* bl2              4M    p30 */
    {PART_FASTBOOT,                534 * 1024,         12 * 1024,    EMMC_USER_PART}, /* fastboot        12M    p39 */
    {PART_VECTOR,                  546 * 1024,          4 * 1024,    EMMC_USER_PART}, /* vector           4M    p40 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,           14040 * 1024,        128 * 1024,    EMMC_USER_PART}, /* hibench_img    128M    p82 */
    {PART_HIBENCH_LOG,           15192 * 1024,         32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG     32M    p85 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,        192 * 1024,    EMMC_USER_PART}, /* fttest    192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      852  *  1024 + (0x600000000 >> 10),        48 * 1024,    EMMC_USER_PART}, /* ddrtest  48M */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                   0,                 2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,                 7 * 1024,              1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_VRL,                       512,                    512,    UFS_PART_3}, /* vrl              512K    p1 */
    {PART_VRL_BACKUP,                1024,                   512,    UFS_PART_3}, /* vrl backup       512K    p2 */
    {PART_NVME,                      10 * 1024,           5 * 1024,    UFS_PART_3}, /* nvme               5M    p4 */
    {PART_DDR_PARA,                  227 * 1024,          1 * 1024,    UFS_PART_3}, /* DDR_PARA           1M    p13 */
    {PART_LOWPOWER_PARA,             228 * 1024,          1 * 1024,    UFS_PART_3}, /* lowpower_para      1M    p14 */
    {PART_DFX,                       335 * 1024,         16 * 1024,    UFS_PART_3}, /* dfx               16M    p20 */
    {PART_BL2,                       492 * 1024,          4 * 1024,    UFS_PART_3}, /* bl2                4M    p26 */
    {PART_FASTBOOT,                  526 * 1024,         12 * 1024,    UFS_PART_3}, /* fastboot          12M    p35 */
    {PART_VECTOR,                    538 * 1024,          4 * 1024,    UFS_PART_3}, /* vector             4M    p36 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,             14032 * 1024,        128 * 1024,    UFS_PART_3}, /* hibench_img      128M    p78 */
    {PART_HIBENCH_LOG,             15184 * 1024,         32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG       32M    p81 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,        192 * 1024,    UFS_PART_3}, /* fttest      192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      852  *  1024 + (0x600000000 >> 10),   48 * 1024,    UFS_PART_3}, /* ddrtest      48M */
#endif
    {"0", 0, 0, 0},
};

#endif
