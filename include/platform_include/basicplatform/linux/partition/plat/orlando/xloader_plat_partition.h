#ifndef _CAPRICORN_XLOADER_PARTITION_H_
#define _CAPRICORN_XLOADER_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define XLOADER3_VRL_INDEX_A    20
#define BL2_VRL_INDEX           22
#define FASTBOOT_VRL_INDEX_A    39
#define VECTOR_VRL_INDEX_A      40

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                           0,     2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED1,                    7 * 1024,       1024,    EMMC_USER_PART}, /* reserved1        1024K  p3 */
    {PART_VRL,                            8704,          512,    EMMC_USER_PART}, /* vrl             512K   p5 */
    {PART_VRL_BACKUP,                   9 * 1024,        512,    EMMC_USER_PART}, /* vrl backup      512K   p6 */
    {PART_NVME,                        18 * 1024,     5 * 1024,    EMMC_USER_PART}, /* nvme             5M     p8 */
    {PART_DDR_PARA,                   223 * 1024,     1 * 1024,    EMMC_USER_PART}, /* DDR_PARA         1M     p17 */
    {PART_LOWPOWER_PARA,              224 * 1024,     1 * 1024,    EMMC_USER_PART}, /* lowpower_para    1M     p18 */
    {PART_BATT_TP_PARA,               225 * 1024,     1 * 1024,    EMMC_USER_PART}, /* batt_tp_para     1M     p19 */
    {PART_BL2,                        226 * 1024,     4 * 1024,    EMMC_USER_PART}, /* bl2              4M     p20 */
    {PART_DFX,                        335 * 1024,    16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p25 */
    {PART_HISEE_IMG,                  490 * 1024,     4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p34 */
    {PART_HISEE_FS,                   498 * 1024,     8 * 1024,    EMMC_USER_PART}, /* hisee_fs           8M    p36 */
    {PART_FASTBOOT,                   506 * 1024,    12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p37 */
    {PART_VECTOR,                     518 * 1024,     4 * 1024,    EMMC_USER_PART}, /* vector             4M    p38 */
    {PART_RESERVED4,                  933 * 1024,     4 * 1024,    EMMC_USER_PART}, /* reserved4A         4M    p59 */
    {PART_HIBENCH_IMG,              11200 * 1024,   128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p78 */
#ifdef FACTORY_VERSION
    {PART_DDRTEST,                  12384 * 1024,    48 * 1024,    EMMC_USER_PART}, /* ddrtest           48M   */
#else
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,          PART_DDRTEST_START,    48 * 1024,    EMMC_USER_PART}, /* ddrtest           48M   */
#endif
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,    192 * 1024,    EMMC_USER_PART}, /* fttest           192M    p66 */
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                           0,     2 * 1024,    UFS_PART_0},
    {PART_RESERVED1,                    7 * 1024,       1024,    UFS_PART_2}, /* reserved1        1024K    p3 */
    {PART_VRL,                             512,        512,    UFS_PART_3}, /* vrl                 512K    p1 */
    {PART_VRL_BACKUP,                     1024,        512,    UFS_PART_3}, /* vrl backup          512K    p2 */
    {PART_NVME,                        10 * 1024,     5 * 1024,    UFS_PART_3}, /* nvme              5M    p4 */
    {PART_DDR_PARA,                   215 * 1024,     1 * 1024,    UFS_PART_3}, /* ddr_para          1M    p13 */
    {PART_LOWPOWER_PARA,              216 * 1024,     1 * 1024,    UFS_PART_3}, /* lowpower_para     1M    p14 */
    {PART_BATT_TP_PARA,               217 * 1024,     1 * 1024,    UFS_PART_3}, /* batt_tp_para      1M    p15 */
    {PART_BL2,                        218 * 1024,     4 * 1024,    UFS_PART_3}, /* bl2               4M    p16 */
    {PART_DFX,                        327 * 1024,    16 * 1024,    UFS_PART_3}, /* dfx              16M    p21 */
    {PART_HISEE_IMG,                  482 * 1024,     4 * 1024,    UFS_PART_3}, /* part_hisee_img    4M    p30 */
    {PART_HISEE_FS,                   490 * 1024,     8 * 1024,    UFS_PART_3}, /* hisee_fs          8M    p32 */
    {PART_FASTBOOT,                   498 * 1024,    12 * 1024,    UFS_PART_3}, /* fastboot         12M    p33 */
    {PART_VECTOR,                     510 * 1024,     4 * 1024,    UFS_PART_3}, /* avs vector        4M    p34 */
    {PART_RESERVED4,                  925 * 1024,     4 * 1024,    UFS_PART_3}, /* reserved4A        4M    p55 */
    {PART_HIBENCH_IMG,              11192 * 1024,   128 * 1024,    UFS_PART_3}, /* hibench_img     128M    p74 */
#ifdef FACTORY_VERSION
    {PART_DDRTEST,                  12376 * 1024,    48 * 1024,    UFS_PART_3}, /* ddrtest           48M   */
#else
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,          PART_DDRTEST_START,    48 * 1024,    UFS_PART_3}, /* ddrtest           48M   */
#endif
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,  192 * 1024,    UFS_PART_3}, /* fttest           192M */
#endif
    {"0", 0, 0, 0},
};

#endif
