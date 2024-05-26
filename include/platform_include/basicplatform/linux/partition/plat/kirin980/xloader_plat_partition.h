#ifndef _CANCER_XLOADER_PARTITION_H_
#define _CANCER_XLOADER_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"


#define XLOADER3_VRL_INDEX_A    20
#define FASTBOOT_VRL_INDEX_A    36
#define VECTOR_VRL_INDEX_A      37

static const struct partition partition_table_emmc[] = {
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,          0,             2 * 1024,        UFS_PART_0},
    {PART_RESERVED1,        7 * 1024,          1024,        UFS_PART_2}, /* reserved1        1M     p3 */
    {PART_VRL,              512,                512,        UFS_PART_3}, /* VRL              512K   p1 */
    {PART_VRL_BACKUP,       1024,               512,        UFS_PART_3}, /* VRL backup       512K   p2 */
    {PART_NVME,             10 * 1024,       5 * 1024,        UFS_PART_3}, /* nvme             5M     p4 */
    {PART_DDR_PARA,         215 * 1024,      1 * 1024,        UFS_PART_3}, /* DDR_PARA         1M     p13 */
    {PART_LOWPOWER_PARA,    216 * 1024,      1 * 1024,        UFS_PART_3}, /* lowpower_para    1M     p14 */
    {PART_BATT_TP_PARA,     217 * 1024,      1 * 1024,        UFS_PART_3}, /* batt_tp_para     1M     p15 */
    {PART_DFX,              327 * 1024,     16 * 1024,        UFS_PART_3}, /* dfx              16M    p20 */
    {PART_HISEE_IMG,        474 * 1024,      4 * 1024,        UFS_PART_3}, /* part_hisee_img_a 4M     p27 */
    {PART_HISEE_FS,         482 * 1024,      8 * 1024,        UFS_PART_3}, /* hisee_fs         8M     p29 */
    {PART_FASTBOOT,         490 * 1024,     12 * 1024,        UFS_PART_3}, /* fastboot         12M    p30 */
    {PART_VECTOR,           502 * 1024,      4 * 1024,        UFS_PART_3}, /* avs vector       4M     p31 */
    {PART_HIBENCH_IMG,      11464 * 1024,  128 * 1024,        UFS_PART_3}, /* hibench_img      128M   p65 */
#ifdef FACTORY_VERSION
    {PART_HIBENCH_LOG,      12616 * 1024,  32 * 1024,         UFS_PART_3}, /* HIBENCH_LOG      32M    p68 */
#endif
#ifdef CONFIG_HISI_DEBUG_FS
    {PART_FTTEST,                24 * 1024 * 1024,        192 * 1024,    UFS_PART_3}, /* fttest   192M */
#endif
#ifdef XLOADER_DDR_TEST
    {PART_DDRTEST,         PART_DDRTEST_START,         48 * 1024,    UFS_PART_3}, /* ddrtest  48M */
#endif
    {"0", 0, 0, 0},
};

#endif
