#ifndef _VIRGO_XLOADER_PARTITION_H_
#define _VIRGO_XLOADER_PARTITION_H_

#include "partition_macro.h"

#define XLOADER3_VRL_INDEX_A    19
#define BL2_VRL_INDEX           31
#define FASTBOOT_VRL_INDEX_A    37
#define VECTOR_VRL_INDEX_A      38

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                   0,            2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,                 7 * 1024,         1024,    EMMC_USER_PART}, /* reserved1       1024K    p3 */
    {PART_VRL,                       8704,            512,    EMMC_USER_PART}, /* vrl              512K    p5 */
    {PART_VRL_BACKUP,                9 * 1024,          512,    EMMC_USER_PART}, /* vrl backup       512K    p6 */
    {PART_NVME,                      18 * 1024,      5 * 1024,    EMMC_USER_PART}, /* nvme               5M    p8 */
    {PART_DDR_PARA,                  231 * 1024,     1 * 1024,    EMMC_USER_PART}, /* DDR_PARA           1M    p16 */
    {PART_LOWPOWER_PARA,             232 * 1024,     1 * 1024,    EMMC_USER_PART}, /* lowpower_para      1M    p17 */
    {PART_DFX,                       343 * 1024,    16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p23 */
    {PART_BL2,                       502 * 1024,     4 * 1024,    EMMC_USER_PART}, /* bl2                4M    p29 */
    {PART_FASTBOOT,                  522 * 1024,    12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p35 */
    {PART_VECTOR,                    534 * 1024,     4 * 1024,    EMMC_USER_PART}, /* vector             4M    p36 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,             12488 * 1024,   128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p77 */
    {PART_HIBENCH_LOG,             13640 * 1024,    32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG       32M    p80 */
#endif
#ifdef CONFIG_SOCBENCH_VERSION
    {PART_FTTEST,                24 * 1024 * 1024,   192 * 1024,    EMMC_USER_PART}, /* fttest           192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      788  *  1024 + (0x600000000 >> 10),        48 * 1024,    EMMC_USER_PART}, /* ddrtest   48M */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                   0,            2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,                 7 * 1024,         1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_VRL,                       512,             512,    UFS_PART_3}, /* vrl              512K    p1 */
    {PART_VRL_BACKUP,                1024,            512,    UFS_PART_3}, /* vrl backup       512K    p2 */
    {PART_NVME,                      10 * 1024,      5 * 1024,    UFS_PART_3}, /* nvme               5M    p4 */
    {PART_DDR_PARA,                  223 * 1024,     1 * 1024,    UFS_PART_3}, /* DDR_PARA           1M    p12 */
    {PART_LOWPOWER_PARA,             224 * 1024,     1 * 1024,    UFS_PART_3}, /* lowpower_para      1M    p13 */
    {PART_DFX,                       335 * 1024,    16 * 1024,    UFS_PART_3}, /* dfx               16M    p19 */
    {PART_BL2,                       494 * 1024,     4 * 1024,    UFS_PART_3}, /* bl2                4M    p25 */
    {PART_FASTBOOT,                  514 * 1024,    12 * 1024,    UFS_PART_3}, /* fastboot          12M    p31 */
    {PART_VECTOR,                    526 * 1024,     4 * 1024,    UFS_PART_3}, /* vector             4M    p32 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_IMG,             12480 * 1024,   128 * 1024,    UFS_PART_3}, /* hibench_img      128M    p73 */
    {PART_HIBENCH_LOG,             13632 * 1024,    32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG       32M    p76 */
#endif
#ifdef CONFIG_SOCBENCH_VERSION
    {PART_FTTEST,                24 * 1024 * 1024,   192 * 1024,    UFS_PART_3}, /* fttest    192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      788  *  1024 + (0x600000000 >> 10),         48 * 1024,    UFS_PART_3}, /* ddrtest    48M */
#endif
    {"0", 0, 0, 0},
};

#endif
