#ifndef _TAURUS_A_PLUS_B_XLOADER_PLAT_PARTITION_H
#define _TAURUS_A_PLUS_B_XLOADER_PLAT_PARTITION_H

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define XLOADER3_VRL_INDEX_A    20
#define BL2_VRL_INDEX           22
#define FASTBOOT_VRL_INDEX_A    39
#define VECTOR_VRL_INDEX_A      40

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                   0,             2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,                 7 * 1024,          1024,    EMMC_USER_PART}, /* reserved1       1024K    p3 */
    {PART_VRL,                       8704,             512,    EMMC_USER_PART}, /* vrl              512K    p5 */
    {PART_VRL_BACKUP,                9 * 1024,           512,    EMMC_USER_PART}, /* vrl backup       512K    p6 */
    {PART_NVME,                      18 * 1024,       5 * 1024,    EMMC_USER_PART}, /* nvme               5M    p8 */
    {PART_DDR_PARA,                  285 * 1024,      1 * 1024,    EMMC_USER_PART}, /* DDR_PARA           1M    p17 */
    {PART_LOWPOWER_PARA,             286 * 1024,      1 * 1024,    EMMC_USER_PART}, /* lowpower_para      1M    p18 */
    {PART_BL2,                       288 * 1024,      4 * 1024,    EMMC_USER_PART}, /* bl2                4M    p20 */
    {PART_DFX,                       397 * 1024,     16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p25 */
    {PART_HISEE_IMG,                 552 * 1024,      4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p34 */
    {PART_HISEE_FS,                  560 * 1024,      8 * 1024,    EMMC_USER_PART}, /* hisee_fs           8M    p36 */
    {PART_FASTBOOT,                  568 * 1024,     12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p37 */
    {PART_VECTOR,                    580 * 1024,      4 * 1024,    EMMC_USER_PART}, /* vector             4M    p38 */
    {PART_HIBENCH_IMG,             12632 * 1024,    128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p78 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_LOG,             13784 * 1024,     32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG       32M    p81 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,    192 * 1024,    EMMC_USER_PART}, /* fttest      192M    p66 */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,         PART_DDRTEST_START,    48 * 1024,    EMMC_USER_PART}, /* ddrtest       48M    p67 */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                   0,                 2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,                 7 * 1024,              1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_VRL,                       512,                  512,    UFS_PART_3}, /* vrl              512K    p1 */
    {PART_VRL_BACKUP,                1024,                 512,    UFS_PART_3}, /* vrl backup       512K    p2 */
    {PART_NVME,                      10 * 1024,           5 * 1024,    UFS_PART_3}, /* nvme               5M    p4 */
    {PART_DDR_PARA,                  277 * 1024,          1 * 1024,    UFS_PART_3}, /* DDR_PARA           1M    p13 */
    {PART_LOWPOWER_PARA,             278 * 1024,          1 * 1024,    UFS_PART_3}, /* lowpower_para      1M    p14 */
    {PART_BL2,                       280 * 1024,          4 * 1024,    UFS_PART_3}, /* bl2                4M    p16 */
    {PART_DFX,                       389 * 1024,         16 * 1024,    UFS_PART_3}, /* dfx               16M    p21 */
    {PART_HISEE_IMG,                 544 * 1024,          4 * 1024,    UFS_PART_3}, /* part_hisee_img     4M    p30 */
    {PART_HISEE_FS,                  552 * 1024,          8 * 1024,    UFS_PART_3}, /* hisee_fs           8M    p32 */
    {PART_FASTBOOT,                  560 * 1024,         12 * 1024,    UFS_PART_3}, /* fastboot          12M    p33 */
    {PART_VECTOR,                    572 * 1024,          4 * 1024,    UFS_PART_3}, /* vector             4M    p34 */
    {PART_HIBENCH_IMG,             12624 * 1024,        128 * 1024,    UFS_PART_3}, /* hibench_img      128M    p74 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_LOG,             13776 * 1024,         32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG       32M    p77 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,        192 * 1024,    UFS_PART_3}, /* fttest     192M    p62 */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,      PART_DDRTEST_START,         48 * 1024,    UFS_PART_3}, /* ddrtest       48M    p67 */
#endif
    {"0", 0, 0, 0},
};

#endif
