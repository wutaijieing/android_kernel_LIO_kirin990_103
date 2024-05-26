/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *  Description: the partition table.
 */

#ifndef _SAGITTARIUS_PARTITION_H_
#define _SAGITTARIUS_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"
#include "partition_def.h"

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                        0,      2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED0,                      0,      2 * 1024,    EMMC_BOOT_BACKUP_PART},
    {PART_PTABLE,                         0,           512,    EMMC_USER_PART}, /* ptable           512K     */
    {PART_FRP,                          512,           512,    EMMC_USER_PART}, /* frp              512K    p1 */
    {PART_PERSIST,                     1024,      6 * 1024,    EMMC_USER_PART}, /* persist            6M    p2 */
    {PART_RESERVED1,               7 * 1024,          1024,    EMMC_USER_PART}, /* reserved1       1024K    p3 */
    {PART_RESERVED6,               8 * 1024,           512,    EMMC_USER_PART}, /* reserved6        512K    p4 */
    {PART_VRL,                         8704,           512,    EMMC_USER_PART}, /* vrl              512K    p5 */
    {PART_VRL_BACKUP,              9 * 1024,           512,    EMMC_USER_PART}, /* vrl backup       512K    p6 */
    {PART_MODEM_SECURE,                9728,          8704,    EMMC_USER_PART}, /* modem_secure    8704K    p7 */
    {PART_NVME,                   18 * 1024,      5 * 1024,    EMMC_USER_PART}, /* nvme               5M    p8 */
    {PART_CTF,                    23 * 1024,      1 * 1024,    EMMC_USER_PART}, /* ctf                1M    p9 */
    {PART_OEMINFO,                24 * 1024,     96 * 1024,    EMMC_USER_PART}, /* oeminfo           96M    p10 */
    {PART_SECURE_STORAGE,        120 * 1024,     32 * 1024,    EMMC_USER_PART}, /* secure storage    32M    p11 */
    {PART_MODEMNVM_FACTORY,      152 * 1024,     16 * 1024,    EMMC_USER_PART}, /* modemnvm factory  16M    p12 */
    {PART_MODEMNVM_BACKUP,       168 * 1024,     16 * 1024,    EMMC_USER_PART}, /* modemnvm backup   16M    p13 */
    {PART_MODEMNVM_IMG,          184 * 1024,     46 * 1024,    EMMC_USER_PART}, /* modemnvm img      46M    p14 */
    {PART_VERITYKEY,             230 * 1024,      1 * 1024,    EMMC_USER_PART}, /* veritykey          1M    p15 */
    {PART_DDR_PARA,              231 * 1024,      1 * 1024,    EMMC_USER_PART}, /* DDR_PARA           1M    p16 */
    {PART_LOWPOWER_PARA,         232 * 1024,      1 * 1024,    EMMC_USER_PART}, /* lowpower_para      1M    p17 */
    {PART_BATT_TP_PARA,          233 * 1024,      1 * 1024,    EMMC_USER_PART}, /* batt_tp_para       1M    p18 */
    {PART_BL2,                   234 * 1024,      4 * 1024,    EMMC_USER_PART}, /* bl2                4M    p19 */
    {PART_RESERVED2,             238 * 1024,     25 * 1024,    EMMC_USER_PART}, /* reserved2         25M    p20 */
    {PART_SPLASH2,               263 * 1024,     80 * 1024,    EMMC_USER_PART}, /* splash2           80M    p21 */
    {PART_BOOTFAIL_INFO,         343 * 1024,      2 * 1024,    EMMC_USER_PART}, /* bootfail info      2M    p22 */
    {PART_MISC,                  345 * 1024,      2 * 1024,    EMMC_USER_PART}, /* misc               2M    p23 */
    {PART_DFX,                   347 * 1024,     16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p24 */
    {PART_RRECORD,               363 * 1024,     16 * 1024,    EMMC_USER_PART}, /* rrecord           16M    p25 */
    {PART_CACHE,                 379 * 1024,    104 * 1024,    EMMC_USER_PART}, /* cache            104M    p26 */
    {PART_FW_LPM3,               483 * 1024,      1 * 1024,    EMMC_USER_PART}, /* fw_lpm3            1M    p27 */
    {PART_RESERVED3,             484 * 1024,      4 * 1024,    EMMC_USER_PART}, /* reserved3A         4M    p28 */
    {PART_TOC,                   488 * 1024,      1 * 1024,    EMMC_USER_PART}, /* toc                1M    p29 */
    {PART_NPU,                   489 * 1024,      8 * 1024,    EMMC_USER_PART}, /* npu                8M    p30 */
    {PART_IVP,                   497 * 1024,      2 * 1024,    EMMC_USER_PART}, /* ivp                2M    p31 */
    {PART_HDCP,                  499 * 1024,      1 * 1024,    EMMC_USER_PART}, /* PART_HDCP          1M    p32 */
    {PART_HHEE,                  500 * 1024,      4 * 1024,    EMMC_USER_PART}, /* hhee               4M    p33 */
    {PART_FASTBOOT,              504 * 1024,     12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p34 */
    {PART_VECTOR,                516 * 1024,      4 * 1024,    EMMC_USER_PART}, /* vector             4M    p35 */
    {PART_ISP_BOOT,              520 * 1024,      2 * 1024,    EMMC_USER_PART}, /* isp_boot           2M    p36 */
    {PART_ISP_FIRMWARE,          522 * 1024,     14 * 1024,    EMMC_USER_PART}, /* isp_firmware      14M    p37 */
    {PART_FW_HIFI,               536 * 1024,     12 * 1024,    EMMC_USER_PART}, /* hifi              12M    p38 */
    {PART_TEEOS,                 548 * 1024,      8 * 1024,    EMMC_USER_PART}, /* teeos              8M    p39 */
    {PART_SENSORHUB,             556 * 1024,     16 * 1024,    EMMC_USER_PART}, /* sensorhub         16M    p40 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK,     572 * 1024,     12 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 12M    p41 */
    {PART_ERECOVERY_VENDOR,      584 * 1024,      8 * 1024,    EMMC_USER_PART}, /* erecovery_vendor   8M    p42 */
    {PART_BOOT,                  592 * 1024,     70 * 1024,    EMMC_USER_PART}, /* boot              65M    p43 */
    {PART_RECOVERY,              662 * 1024,     85 * 1024,    EMMC_USER_PART}, /* recovery          85M    p44 */
    {PART_ERECOVERY,             747 * 1024,     12 * 1024,    EMMC_USER_PART}, /* erecovery         12M    p45 */
    {PART_RESERVED,              759 * 1024,     72 * 1024,    EMMC_USER_PART}, /* reserved          77M    p46 */
#else
    {PART_ERECOVERY_RAMDISK,     572 * 1024,     32 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 32M    p41 */
    {PART_ERECOVERY_VENDOR,      604 * 1024,     16 * 1024,    EMMC_USER_PART}, /* erecovery_vendor  16M    p42 */
    {PART_BOOT,                  620 * 1024,     30 * 1024,    EMMC_USER_PART}, /* boot              25M    p43 */
    {PART_RECOVERY,              650 * 1024,     45 * 1024,    EMMC_USER_PART}, /* recovery          45M    p44 */
    {PART_ERECOVERY,             695 * 1024,     45 * 1024,    EMMC_USER_PART}, /* erecovery         45M    p45 */
    {PART_RESERVED,              740 * 1024,     91 * 1024,    EMMC_USER_PART}, /* reserved          96M    p46 */
#endif
    {PART_UEFIBIOS,              831 * 1024,     32 * 1024,    EMMC_USER_PART}, /* uefibios          32M    p47 */
    {PART_RECOVERY_RAMDISK,      863 * 1024,     32 * 1024,    EMMC_USER_PART}, /* recovery_ramdisk  32M    p48 */
    {PART_RECOVERY_VENDOR,       895 * 1024,     16 * 1024,    EMMC_USER_PART}, /* recovery_vendor   16M    p49 */
    {PART_ENG_SYSTEM,            911 * 1024,     12 * 1024,    EMMC_USER_PART}, /* eng_system        12M    p50 */
    {PART_ENG_VENDOR,            923 * 1024,     20 * 1024,    EMMC_USER_PART}, /* eng_vendor        20M    p51 */
    {PART_FW_DTB,                943 * 1024,      2 * 1024,    EMMC_USER_PART}, /* fw_dtb             2M    p52 */
    {PART_DTBO,                  945 * 1024,     20 * 1024,    EMMC_USER_PART}, /* dtoimage          20M    p53 */
    {PART_TRUSTFIRMWARE,         965 * 1024,      2 * 1024,    EMMC_USER_PART}, /* trustfirmware      2M    p54 */
    {PART_MODEM_FW,              967 * 1024,    124 * 1024,    EMMC_USER_PART}, /* modem_fw         124M    p55 */
    {PART_MODEM_VENDOR,         1091 * 1024,     10 * 1024,    EMMC_USER_PART}, /* modem_vendor      10M    p56 */
    {PART_MODEM_PATCH_NV,       1101 * 1024,     16 * 1024,    EMMC_USER_PART}, /* modem_patch_nv    16M    p57 */
    {PART_MODEM_DRIVER,         1117 * 1024,     20 * 1024,    EMMC_USER_PART}, /* modem_driver      20M    p58 */
    {PART_MODEMNVM_UPDATE,      1137 * 1024,     16 * 1024,    EMMC_USER_PART}, /* modemnvm_update   16M    p59 */
    {PART_MODEMNVM_CUST,        1153 * 1024,     16 * 1024,    EMMC_USER_PART}, /* modemnvm_cust     16M    p60 */
    {PART_RAMDISK,              1169 * 1024,      2 * 1024,    EMMC_USER_PART}, /* ramdisk           32M    p61 */
    {PART_VBMETA_SYSTEM,        1171 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_system      1M    p62 */
    {PART_VBMETA_VENDOR,        1172 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_vendor      1M    p63 */
    {PART_VBMETA_ODM,           1173 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_odm         1M    p64 */
    {PART_VBMETA_CUST,          1174 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_cust        1M    p65 */
    {PART_VBMETA_HW_PRODUCT,    1175 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_hw_product  1M    p66 */
    {PART_RECOVERY_VBMETA,      1176 * 1024,      2 * 1024,    EMMC_USER_PART}, /* recovery_vbmeta    2M    p67 */
    {PART_ERECOVERY_VBMETA,     1178 * 1024,      2 * 1024,    EMMC_USER_PART}, /* erecovery_vbmeta   2M    p68 */
    {PART_VBMETA,               1180 * 1024,      4 * 1024,    EMMC_USER_PART}, /* vbmeta             4M    p69 */
    {PART_PATCH,                1184 * 1024,     32 * 1024,    EMMC_USER_PART}, /* patch             32M    p70 */
#ifdef CONFIG_FACTORY_MODE
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,   1280 * 1024,    EMMC_USER_PART}, /* preas           1280M    p72 */
    {PART_PREAVS,               2512 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                2544 * 1024,   8240 * 1024,    EMMC_USER_PART}, /* super           8240M    p74 */
    {PART_VERSION,             10784 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,             11360 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_HIBENCH_IMG,         12504 * 1024,    128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p77 */
    {PART_HIBENCH_DATA,        12632 * 1024,    512 * 1024,    EMMC_USER_PART}, /* hibench_data     512M    p78 */
    {PART_FLASH_AGEING,        13144 * 1024,    512 * 1024,    EMMC_USER_PART}, /* FLASH_AGEING     512M    p79 */
    {PART_HIBENCH_LOG,         13656 * 1024,     32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG       32M    p80 */
    {PART_HIBENCH_LPM3,        13688 * 1024,     32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LPM3      32M    p81 */
    {PART_USERDATA,            13720 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p82 */
#else
#ifdef CONFIG_USE_EROFS
#ifdef CONFIG_MARKET_INTERNAL
    {PART_SUPER,                1216 * 1024,   7408 * 1024,    EMMC_USER_PART}, /* super           7408M    p71 */
    {PART_VERSION,              8624 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p72 */
    {PART_PRELOAD,              9200 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p73 */
    {PART_USERDATA,            10344 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p74 */
#elif defined CONFIG_MARKET_OVERSEA
    {PART_SUPER,                1216 * 1024,   8232 * 1024,    EMMC_USER_PART}, /* super           8232M    p71 */
    {PART_VERSION,              9448 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p72 */
    {PART_PRELOAD,             10024 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p73 */
    {PART_USERDATA,            11168 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p74 */
#elif defined CONFIG_MARKET_FULL_INTERNAL
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,    296 * 1024,    EMMC_USER_PART}, /* preas            296M    p72 */
    {PART_PREAVS,               1528 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                1560 * 1024,   6458 * 1024,    EMMC_USER_PART}, /* super           6378M    p74 */
    {PART_VERSION,              8018 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,              8594 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_USERDATA,             9738 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p77 */
#elif defined CONFIG_MARKET_FULL_OVERSEA
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,   1024 * 1024,    EMMC_USER_PART}, /* preas            976M    p72 */
    {PART_PREAVS,               2256 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                2288 * 1024,   6434 * 1024,    EMMC_USER_PART}, /* super           6354M    p74 */
    {PART_VERSION,              8722 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,              9298 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_USERDATA,            10442 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p77 */
#else
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,   1280 * 1024,    EMMC_USER_PART}, /* preas           1280M    p72 */
    {PART_PREAVS,               2512 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                2544 * 1024,   8240 * 1024,    EMMC_USER_PART}, /* super           8240M    p74 */
    {PART_VERSION,             10784 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,             11360 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_USERDATA,            12504 * 1024,  (4UL) * 1024 * 1024,   EMMC_USER_PART}, /* userdata     4G    p77 */
#endif
#else /* do not use erofs */
#ifdef CONFIG_MARKET_INTERNAL
    {PART_SUPER,                1216 * 1024,   9568 * 1024,    EMMC_USER_PART}, /* super           9568M    p71 */
    {PART_VERSION,             10784 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p72 */
    {PART_PRELOAD,             11360 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p73 */
    {PART_USERDATA,            12504 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p74 */
#elif defined CONFIG_MARKET_OVERSEA
    {PART_SUPER,                1216 * 1024,  10696 * 1024,    EMMC_USER_PART}, /* super          10696M    p71 */
    {PART_VERSION,             11912 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p72 */
    {PART_PRELOAD,             12488 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p73 */
    {PART_USERDATA,            13632 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p74 */
#elif defined CONFIG_MARKET_FULL_INTERNAL
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,    368 * 1024,    EMMC_USER_PART}, /* preas            368M    p72 */
    {PART_PREAVS,               1600 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                1632 * 1024,   8418 * 1024,    EMMC_USER_PART}, /* super           8418M    p74 */
    {PART_VERSION,             10050 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,             10626 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_USERDATA,            11770 * 1024,  (4UL) * 1024 * 1024,   EMMC_USER_PART}, /* userdata     4G    p77 */
#elif defined CONFIG_MARKET_FULL_OVERSEA
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,   1280 * 1024,    EMMC_USER_PART}, /* preas           1280M    p72 */
    {PART_PREAVS,               2512 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                2544 * 1024,   8634 * 1024,    EMMC_USER_PART}, /* super           8634M    p74 */
    {PART_VERSION,             11178 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,             11754 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_USERDATA,            12898 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p77 */
#else
    {PART_METADATA,             1216 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p71 */
    {PART_PREAS,                1232 * 1024,    296 * 1024,    EMMC_USER_PART}, /* preas            296M    p72 */
    {PART_PREAVS,               1528 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p73 */
    {PART_SUPER,                1560 * 1024,   5458 * 1024,    EMMC_USER_PART}, /* super           5378M    p74 */
    {PART_VERSION,              7018 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p75 */
    {PART_PRELOAD,              7594 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p76 */
    {PART_USERDATA,             8738 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata     4G    p77 */
#endif
#endif
#endif
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {PART_XLOADER,                        0,      2 * 1024,    UFS_PART_0},
    {PART_RESERVED0,                      0,      2 * 1024,    UFS_PART_1},
    {PART_PTABLE,                         0,           512,    UFS_PART_2}, /* ptable           512K     */
    {PART_FRP,                          512,           512,    UFS_PART_2}, /* frp              512K    p1 */
    {PART_PERSIST,                 1 * 1024,      6 * 1024,    UFS_PART_2}, /* persist         6144K    p2 */
    {PART_RESERVED1,               7 * 1024,          1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_PTABLE_LU3,                     0,           512,    UFS_PART_3}, /* ptable_lu3       512K    p0 */
    {PART_VRL,                          512,           512,    UFS_PART_3}, /* vrl              512K    p1 */
    {PART_VRL_BACKUP,                  1024,           512,    UFS_PART_3}, /* vrl backup       512K    p2 */
    {PART_MODEM_SECURE,                1536,          8704,    UFS_PART_3}, /* modem_secure    8704K    p3 */
    {PART_NVME,                   10 * 1024,      5 * 1024,    UFS_PART_3}, /* nvme               5M    p4 */
    {PART_CTF,                    15 * 1024,      1 * 1024,    UFS_PART_3}, /* PART_CTF           1M    p5 */
    {PART_OEMINFO,                16 * 1024,     96 * 1024,    UFS_PART_3}, /* oeminfo           96M    p6 */
    {PART_SECURE_STORAGE,        112 * 1024,     32 * 1024,    UFS_PART_3}, /* secure storage    32M    p7 */
    {PART_MODEMNVM_FACTORY,      144 * 1024,     16 * 1024,    UFS_PART_3}, /* modemnvm factory  16M    p8 */
    {PART_MODEMNVM_BACKUP,       160 * 1024,     16 * 1024,    UFS_PART_3}, /* modemnvm backup   16M    p9 */
    {PART_MODEMNVM_IMG,          176 * 1024,     46 * 1024,    UFS_PART_3}, /* modemnvm img      46M    p10 */
    {PART_VERITYKEY,             222 * 1024,      1 * 1024,    UFS_PART_3}, /* veritykey          1M    p11 */
    {PART_DDR_PARA,              223 * 1024,      1 * 1024,    UFS_PART_3}, /* DDR_PARA           1M    p12 */
    {PART_LOWPOWER_PARA,         224 * 1024,      1 * 1024,    UFS_PART_3}, /* lowpower_para      1M    p13 */
    {PART_BATT_TP_PARA,          225 * 1024,      1 * 1024,    UFS_PART_3}, /* batt_tp_para       1M    p14 */
    {PART_BL2,                   226 * 1024,      4 * 1024,    UFS_PART_3}, /* bl2                4M    p15 */
    {PART_RESERVED2,             230 * 1024,     25 * 1024,    UFS_PART_3}, /* reserved2         25M    p16 */
    {PART_SPLASH2,               255 * 1024,     80 * 1024,    UFS_PART_3}, /* splash2           80M    p17 */
    {PART_BOOTFAIL_INFO,         335 * 1024,      2 * 1024,    UFS_PART_3}, /* bootfail info      2M    p18 */
    {PART_MISC,                  337 * 1024,      2 * 1024,    UFS_PART_3}, /* misc               2M    p19 */
    {PART_DFX,                   339 * 1024,     16 * 1024,    UFS_PART_3}, /* dfx               16M    p20 */
    {PART_RRECORD,               355 * 1024,     16 * 1024,    UFS_PART_3}, /* rrecord           16M    p21 */
    {PART_CACHE,                 371 * 1024,    104 * 1024,    UFS_PART_3}, /* cache            104M    p22 */
    {PART_FW_LPM3,               475 * 1024,      1 * 1024,    UFS_PART_3}, /* fw_lpm3            1M    p23 */
    {PART_RESERVED3,             476 * 1024,      4 * 1024,    UFS_PART_3}, /* reserved3A         4M    p24 */
    {PART_TOC,                   480 * 1024,      1 * 1024,    UFS_PART_3}, /* toc                1M    p25 */
    {PART_NPU,                   481 * 1024,      8 * 1024,    UFS_PART_3}, /* npu                8M    p26 */
    {PART_IVP,                   489 * 1024,      2 * 1024,    UFS_PART_3}, /* ivp                2M    p27 */
    {PART_HDCP,                  491 * 1024,      1 * 1024,    UFS_PART_3}, /* PART_HDCP          1M    p28 */
    {PART_HHEE,                  492 * 1024,      4 * 1024,    UFS_PART_3}, /* hhee               4M    p29 */
    {PART_FASTBOOT,              496 * 1024,     12 * 1024,    UFS_PART_3}, /* fastboot          12M    p30 */
    {PART_VECTOR,                508 * 1024,      4 * 1024,    UFS_PART_3}, /* vector             4M    p31 */
    {PART_ISP_BOOT,              512 * 1024,      2 * 1024,    UFS_PART_3}, /* isp_boot           2M    p32 */
    {PART_ISP_FIRMWARE,          514 * 1024,     14 * 1024,    UFS_PART_3}, /* isp_firmware      14M    p33 */
    {PART_FW_HIFI,               528 * 1024,     12 * 1024,    UFS_PART_3}, /* hifi              12M    p34 */
    {PART_TEEOS,                 540 * 1024,      8 * 1024,    UFS_PART_3}, /* teeos              8M    p35 */
    {PART_SENSORHUB,             548 * 1024,     16 * 1024,    UFS_PART_3}, /* sensorhub         16M    p36 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK,     564 * 1024,     12 * 1024,    UFS_PART_3}, /* erecovery_ramdisk 12M    p37 */
    {PART_ERECOVERY_VENDOR,      576 * 1024,      8 * 1024,    UFS_PART_3}, /* erecovery_vendor   8M    p38 */
    {PART_BOOT,                  584 * 1024,     70 * 1024,    UFS_PART_3}, /* boot              65M    p39 */
    {PART_RECOVERY,              654 * 1024,     85 * 1024,    UFS_PART_3}, /* recovery          85M    p40 */
    {PART_ERECOVERY,             739 * 1024,     12 * 1024,    UFS_PART_3}, /* erecovery         12M    p41 */
    {PART_RESERVED,              751 * 1024,     72 * 1024,    UFS_PART_3}, /* reserved          77M    p42 */
#else
    {PART_ERECOVERY_RAMDISK,     564 * 1024,     32 * 1024,    UFS_PART_3}, /* erecovery_ramdisk 32M    p37 */
    {PART_ERECOVERY_VENDOR,      596 * 1024,     16 * 1024,    UFS_PART_3}, /* erecovery_vendor  16M    p38 */
    {PART_BOOT,                  612 * 1024,     30 * 1024,    UFS_PART_3}, /* boot              25M    p39 */
    {PART_RECOVERY,              642 * 1024,     45 * 1024,    UFS_PART_3}, /* recovery          45M    p40 */
    {PART_ERECOVERY,             687 * 1024,     45 * 1024,    UFS_PART_3}, /* erecovery         45M    p41 */
    {PART_RESERVED,              732 * 1024,     91 * 1024,    UFS_PART_3}, /* reserved          96M    p42 */
#endif
    {PART_UEFIBIOS,              823 * 1024,     32 * 1024,    UFS_PART_3}, /* uefibios          32M    p43 */
    {PART_RECOVERY_RAMDISK,      855 * 1024,     32 * 1024,    UFS_PART_3}, /* recovery_ramdisk  32M    p44 */
    {PART_RECOVERY_VENDOR,       887 * 1024,     16 * 1024,    UFS_PART_3}, /* recovery_vendor   16M    p45 */
    {PART_ENG_SYSTEM,            903 * 1024,     12 * 1024,    UFS_PART_3}, /* eng_system        12M    p46 */
    {PART_ENG_VENDOR,            915 * 1024,     20 * 1024,    UFS_PART_3}, /* eng_vendor        20M    p47 */
    {PART_FW_DTB,                935 * 1024,      2 * 1024,    UFS_PART_3}, /* fw_dtb             2M    p48 */
    {PART_DTBO,                  937 * 1024,     20 * 1024,    UFS_PART_3}, /* dtoimage          20M    p49 */
    {PART_TRUSTFIRMWARE,         957 * 1024,      2 * 1024,    UFS_PART_3}, /* trustfirmware      2M    p50 */
    {PART_MODEM_FW,              959 * 1024,    124 * 1024,    UFS_PART_3}, /* modem_fw         124M    p51 */
    {PART_MODEM_VENDOR,         1083 * 1024,     10 * 1024,    UFS_PART_3}, /* modem_vendor      10M    p52 */
    {PART_MODEM_PATCH_NV,       1093 * 1024,     16 * 1024,    UFS_PART_3}, /* modem_patch_nv    16M    p53 */
    {PART_MODEM_DRIVER,         1109 * 1024,     20 * 1024,    UFS_PART_3}, /* modem_driver      20M    p54 */
    {PART_MODEMNVM_UPDATE,      1129 * 1024,     16 * 1024,    UFS_PART_3}, /* modemnvm_update   16M    p55 */
    {PART_MODEMNVM_CUST,        1145 * 1024,     16 * 1024,    UFS_PART_3}, /* modemnvm_cust     16M    p56 */
    {PART_RAMDISK,              1161 * 1024,      2 * 1024,    UFS_PART_3}, /* ramdisk           32M    p57 */
    {PART_VBMETA_SYSTEM,        1163 * 1024,      1 * 1024,    UFS_PART_3}, /* vbmeta_system      1M    p58 */
    {PART_VBMETA_VENDOR,        1164 * 1024,      1 * 1024,    UFS_PART_3}, /* vbmeta_vendor      1M    p59 */
    {PART_VBMETA_ODM,           1165 * 1024,      1 * 1024,    UFS_PART_3}, /* vbmeta_odm         1M    p60 */
    {PART_VBMETA_CUST,          1166 * 1024,      1 * 1024,    UFS_PART_3}, /* vbmeta_cust        1M    p61 */
    {PART_VBMETA_HW_PRODUCT,    1167 * 1024,      1 * 1024,    UFS_PART_3}, /* vbmeta_hw_product  1M    p62 */
    {PART_RECOVERY_VBMETA,      1168 * 1024,      2 * 1024,    UFS_PART_3}, /* recovery_vbmeta    2M    p63 */
    {PART_ERECOVERY_VBMETA,     1170 * 1024,      2 * 1024,    UFS_PART_3}, /* erecovery_vbmeta   2M    p64 */
    {PART_VBMETA,               1172 * 1024,      4 * 1024,    UFS_PART_3}, /* vbmeta             4M    p65 */
    {PART_PATCH,                1176 * 1024,     32 * 1024,    UFS_PART_3}, /* patch             32M    p66 */
#ifdef CONFIG_FACTORY_MODE
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,   1280 * 1024,    UFS_PART_3}, /* preas            928M    p68 */
    {PART_PREAVS,               2504 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                2536 * 1024,   8240 * 1024,    UFS_PART_3}, /* super           9568M    p70 */
    {PART_VERSION,             10776 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,             11352 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_HIBENCH_IMG,         12496 * 1024,    128 * 1024,    UFS_PART_3}, /* hibench_img      128M    p73 */
    {PART_HIBENCH_DATA,        12624 * 1024,    512 * 1024,    UFS_PART_3}, /* hibench_data     512M    p74 */
    {PART_FLASH_AGEING,        13136 * 1024,    512 * 1024,    UFS_PART_3}, /* FLASH_AGEING     512M    p75 */
    {PART_HIBENCH_LOG,         13648 * 1024,     32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG       32M    p76 */
    {PART_HIBENCH_LPM3,        13680 * 1024,     32 * 1024,    UFS_PART_3}, /* HIBENCH_LPM3      32M    p77 */
    {PART_USERDATA,            13712 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p78 */
#else
#ifdef CONFIG_USE_EROFS
#ifdef CONFIG_MARKET_INTERNAL
    {PART_SUPER,                1208 * 1024,   7408 * 1024,    UFS_PART_3}, /* super           7408M    p67 */
    {PART_VERSION,              8616 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p68 */
    {PART_PRELOAD,              9192 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p69 */
    {PART_USERDATA,            10336 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p70 */
#elif defined CONFIG_MARKET_OVERSEA
    {PART_SUPER,                1208 * 1024,   8232 * 1024,    UFS_PART_3}, /* super           8232M    p67 */
    {PART_VERSION,              9440 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p68 */
    {PART_PRELOAD,             10016 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p69 */
    {PART_USERDATA,            11160 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p70 */
#elif defined CONFIG_MARKET_FULL_INTERNAL
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,    296 * 1024,    UFS_PART_3}, /* preas            296M    p68 */
    {PART_PREAVS,               1520 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                1552 * 1024,   6458 * 1024,    UFS_PART_3}, /* super           6458M    p70 */
    {PART_VERSION,              8010 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,              8586 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_USERDATA,             9730 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p73 */
#elif defined CONFIG_MARKET_FULL_OVERSEA
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,   1024 * 1024,    UFS_PART_3}, /* preas           1024M    p68 */
    {PART_PREAVS,               2248 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                2280 * 1024,   6434 * 1024,    UFS_PART_3}, /* super           6434M    p70 */
    {PART_VERSION,              8714 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,              9290 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_USERDATA,            10434 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p73 */
#else
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,   1280 * 1024,    UFS_PART_3}, /* preas            928M    p68 */
    {PART_PREAVS,               2504 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                2536 * 1024,   8240 * 1024,    UFS_PART_3}, /* super           8240M    p70 */
    {PART_VERSION,             10776 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,             11352 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_USERDATA,            12496 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p73 */
#endif
#else /* do not use erofs */
#ifdef CONFIG_MARKET_INTERNAL
    {PART_SUPER,                1208 * 1024,   9568 * 1024,    UFS_PART_3}, /* super           9568M    p67 */
    {PART_VERSION,             10776 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p68 */
    {PART_PRELOAD,             11352 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p69 */
    {PART_USERDATA,            12496 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p70 */
#elif defined CONFIG_MARKET_OVERSEA
    {PART_SUPER,                1208 * 1024,  10696 * 1024,    UFS_PART_3}, /* super          10696M    p67 */
    {PART_VERSION,             11904 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p68 */
    {PART_PRELOAD,             12480 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p69 */
    {PART_USERDATA,            13624 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p70 */
#elif defined CONFIG_MARKET_FULL_INTERNAL
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,    368 * 1024,    UFS_PART_3}, /* preas            368M    p68 */
    {PART_PREAVS,               1592 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                1624 * 1024,   8418 * 1024,    UFS_PART_3}, /* super           8418M    p70 */
    {PART_VERSION,             10042 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,             10618 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_USERDATA,            11762 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p73 */
#elif defined CONFIG_MARKET_FULL_OVERSEA
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,   1280 * 1024,    UFS_PART_3}, /* preas           1280M    p68 */
    {PART_PREAVS,               2504 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                2536 * 1024,   8634 * 1024,    UFS_PART_3}, /* super           8634M    p70 */
    {PART_VERSION,             11170 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,             11746 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_USERDATA,            12890 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p73 */
#else
    {PART_METADATA,             1208 * 1024,     16 * 1024,    UFS_PART_3}, /* metadata          16M    p67 */
    {PART_PREAS,                1224 * 1024,    296 * 1024,    UFS_PART_3}, /* preas            296M    p68 */
    {PART_PREAVS,               1520 * 1024,     32 * 1024,    UFS_PART_3}, /* preavs            32M    p69 */
    {PART_SUPER,                1552 * 1024,   5458 * 1024,    UFS_PART_3}, /* super           5458M    p70 */
    {PART_VERSION,              7010 * 1024,    576 * 1024,    UFS_PART_3}, /* version          576M    p71 */
    {PART_PRELOAD,              7586 * 1024,   1144 * 1024,    UFS_PART_3}, /* preload         1144M    p72 */
    {PART_USERDATA,             8730 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata     4G    p73 */
#endif
#endif
#endif
    {PART_PTABLE_LU4,                   0,         512,    UFS_PART_4}, /* ptable_lu4        512K   p0 */
    {PART_RESERVED12,                 512,        1536,    UFS_PART_4}, /* reserved12       1536K   p1 */
    {PART_USERDATA2,                 2048, (4UL) * 1024 * 1024,    UFS_PART_4}, /* userdata2     4G   p2 */
    {"0", 0, 0, 0},
};

#endif
