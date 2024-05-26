#ifndef _TAURUS_A_PLUS_B_PLAT_PARTITION_H
#define _TAURUS_A_PLUS_B_PLAT_PARTITION_H

#include "partition_macro.h"
#include "partition_macro_plus.h"
#include "partition_def.h"

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER,                   0,             2 * 1024,    EMMC_BOOT_MAJOR_PART},
    {PART_RESERVED0,                 0,             2 * 1024,    EMMC_BOOT_BACKUP_PART},
    {PART_PTABLE,                    0,                  512,    EMMC_USER_PART}, /* ptable           512K    */
    {PART_FRP,                       512,                512,    EMMC_USER_PART}, /* frp              512K    p1 */
    {PART_PERSIST,                   1024,          6 * 1024,    EMMC_USER_PART}, /* persist            6M    p2 */
    {PART_RESERVED1,                 7 * 1024,          1024,    EMMC_USER_PART}, /* reserved1       1024K    p3 */
    {PART_RESERVED6,                 8 * 1024,           512,    EMMC_USER_PART}, /* reserved6        512K    p4 */
    {PART_VRL,                       8704,               512,    EMMC_USER_PART}, /* vrl              512K    p5 */
    {PART_VRL_BACKUP,                9 * 1024,           512,    EMMC_USER_PART}, /* vrl backup       512K    p6 */
    {PART_MODEM_SECURE,              9728,              8704,    EMMC_USER_PART}, /* modem_secure    8704K    p7 */
    {PART_NVME,                      18 * 1024,       5 * 1024,    EMMC_USER_PART}, /* nvme               5M    p8 */
    {PART_CTF,                       23 * 1024,       1 * 1024,    EMMC_USER_PART}, /* ctf                1M    p9 */
    {PART_OEMINFO,                   24 * 1024,      96 * 1024,    EMMC_USER_PART}, /* oeminfo           96M    p10 */
    {PART_SECURE_STORAGE,            120 * 1024,     32 * 1024,    EMMC_USER_PART}, /* secure storage    32M    p11 */
    {PART_MODEMNVM_FACTORY,          152 * 1024,     32 * 1024,    EMMC_USER_PART}, /* modemnvm factory  32M    p12 */
    {PART_MODEMNVM_BACKUP,           184 * 1024,     32 * 1024,    EMMC_USER_PART}, /* modemnvm backup   32M    p13 */
    {PART_MODEMNVM_IMG,              216 * 1024,     64 * 1024,    EMMC_USER_PART}, /* modemnvm img      64M    p14 */
    {PART_HISEE_ENCOS,               280 * 1024,      4 * 1024,    EMMC_USER_PART}, /* hisee_encos        4M    p15 */
    {PART_VERITYKEY,                 284 * 1024,      1 * 1024,    EMMC_USER_PART}, /* veritykey          1M    p16 */
    {PART_DDR_PARA,                  285 * 1024,      1 * 1024,    EMMC_USER_PART}, /* DDR_PARA           1M    p17 */
    {PART_LOWPOWER_PARA,             286 * 1024,      1 * 1024,    EMMC_USER_PART}, /* lowpower_para      1M    p18 */
    {PART_BATT_TP_PARA,              287 * 1024,      1 * 1024,    EMMC_USER_PART}, /* batt_tp_para       1M    p19 */
    {PART_BL2,                       288 * 1024,      4 * 1024,    EMMC_USER_PART}, /* bl2                4M    p20 */
    {PART_RESERVED2,                 292 * 1024,     21 * 1024,    EMMC_USER_PART}, /* reserved2         21M    p21 */
    {PART_SPLASH2,                   313 * 1024,     80 * 1024,    EMMC_USER_PART}, /* splash2           80M    p22 */
    {PART_BOOTFAIL_INFO,             393 * 1024,      2 * 1024,    EMMC_USER_PART}, /* bootfail info      2M    p23 */
    {PART_MISC,                      395 * 1024,      2 * 1024,    EMMC_USER_PART}, /* misc               2M    p24 */
    {PART_DFX,                       397 * 1024,     16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p25 */
    {PART_RRECORD,                   413 * 1024,     16 * 1024,    EMMC_USER_PART}, /* rrecord           16M    p26 */
    {PART_CACHE,                     429 * 1024,    104 * 1024,    EMMC_USER_PART}, /* cache            104M    p27 */
    {PART_FW_LPM3,                   533 * 1024,      1 * 1024,    EMMC_USER_PART}, /* fw_lpm3            1M    p28 */
    {PART_RESERVED3,                 534 * 1024,      5 * 1024,    EMMC_USER_PART}, /* reserved3A         5M    p29 */
    {PART_NPU,                       539 * 1024,      8 * 1024,    EMMC_USER_PART}, /* npu                8M    p30 */
    {PART_HIEPS,                     547 * 1024,      2 * 1024,    EMMC_USER_PART}, /* hieps              2M    p31 */
    {PART_IVP,                       549 * 1024,      2 * 1024,    EMMC_USER_PART}, /* ivp                2M    p32 */
    {PART_HDCP,                      551 * 1024,      1 * 1024,    EMMC_USER_PART}, /* PART_HDCP          1M    p33 */
    {PART_HISEE_IMG,                 552 * 1024,      4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p34 */
    {PART_HHEE,                      556 * 1024,      4 * 1024,    EMMC_USER_PART}, /* hhee               4M    p35 */
    {PART_HISEE_FS,                  560 * 1024,      8 * 1024,    EMMC_USER_PART}, /* hisee_fs           8M    p36 */
    {PART_FASTBOOT,                  568 * 1024,     12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p37 */
    {PART_VECTOR,                    580 * 1024,      4 * 1024,    EMMC_USER_PART}, /* vector             4M    p38 */
    {PART_ISP_BOOT,                  584 * 1024,      2 * 1024,    EMMC_USER_PART}, /* isp_boot           2M    p39 */
    {PART_ISP_FIRMWARE,              586 * 1024,     14 * 1024,    EMMC_USER_PART}, /* isp_firmware      14M    p40 */
    {PART_FW_HIFI,                   600 * 1024,     12 * 1024,    EMMC_USER_PART}, /* hifi              12M    p41 */
    {PART_TEEOS,                     612 * 1024,      8 * 1024,    EMMC_USER_PART}, /* teeos              8M    p42 */
    {PART_SENSORHUB,                 620 * 1024,     16 * 1024,    EMMC_USER_PART}, /* sensorhub         16M    p43 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK,         636 * 1024,     12 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 12M    p44 */
    {PART_ERECOVERY_VENDOR,          648 * 1024,      8 * 1024,    EMMC_USER_PART}, /* erecovery_vendor   8M    p45 */
    {PART_BOOT,                      656 * 1024,     65 * 1024,    EMMC_USER_PART}, /* boot              65M    p46 */
    {PART_RECOVERY,                  721 * 1024,     85 * 1024,    EMMC_USER_PART}, /* recovery          85M    p47 */
    {PART_ERECOVERY,                 806 * 1024,     12 * 1024,    EMMC_USER_PART}, /* erecovery         12M    p48 */
    {PART_METADATA,                  818 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p49 */
    {PART_KPATCH,                    834 * 1024,     29 * 1024,    EMMC_USER_PART}, /* reserved          29M    p50 */
#elif defined CONFIG_FACTORY_MODE
    {PART_ERECOVERY_RAMDISK,         636 * 1024,     32 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 32M    p44 */
    {PART_ERECOVERY_VENDOR,          668 * 1024,     24 * 1024,    EMMC_USER_PART}, /* erecovery_vendor  24M    p45 */
    {PART_BOOT,                      692 * 1024,     30 * 1024,    EMMC_USER_PART}, /* boot              30M    p46 */
    {PART_RECOVERY,                  722 * 1024,     41 * 1024,    EMMC_USER_PART}, /* recovery          41M    p47 */
    {PART_ERECOVERY,                 763 * 1024,     41 * 1024,    EMMC_USER_PART}, /* erecovery         41M    p48 */
    {PART_METADATA,                  804 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p49 */
    {PART_KPATCH,                    820 * 1024,     43 * 1024,    EMMC_USER_PART}, /* kpatch            43M    p50 */
#else
    {PART_ERECOVERY_RAMDISK,         636 * 1024,     32 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 32M    p44 */
    {PART_ERECOVERY_VENDOR,          668 * 1024,     16 * 1024,    EMMC_USER_PART}, /* erecovery_vendor  16M    p45 */
    {PART_BOOT,                      684 * 1024,     30 * 1024,    EMMC_USER_PART}, /* boot              30M    p46 */
    {PART_RECOVERY,                  714 * 1024,     45 * 1024,    EMMC_USER_PART}, /* recovery          45M    p47 */
    {PART_ERECOVERY,                 759 * 1024,     45 * 1024,    EMMC_USER_PART}, /* erecovery         45M    p48 */
    {PART_METADATA,                  804 * 1024,     16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p49 */
    {PART_KPATCH,                    820 * 1024,     43 * 1024,    EMMC_USER_PART}, /* kpatch            43M    p50 */
#endif
    {PART_ENG_SYSTEM,                863 * 1024,     12 * 1024,    EMMC_USER_PART}, /* eng_system        12M    p51 */
    {PART_RAMDISK,                   875 * 1024,      2 * 1024,    EMMC_USER_PART}, /* ramdisk            2M    p52 */
    {PART_VBMETA_SYSTEM,             877 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_system      1M    p53 */
    {PART_VBMETA_VENDOR,             878 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_vendor      1M    p54 */
    {PART_VBMETA_ODM,                879 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_odm         1M    p55 */
    {PART_VBMETA_CUST,               880 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_cust        1M    p56 */
    {PART_VBMETA_HW_PRODUCT,         881 * 1024,      1 * 1024,    EMMC_USER_PART}, /* vbmeta_hw_product  1M    p57 */
    {PART_RECOVERY_RAMDISK,          882 * 1024,     32 * 1024,    EMMC_USER_PART}, /* recovery_ramdisk  32M    p58 */
#ifdef CONFIG_FACTORY_MODE
    {PART_RECOVERY_VENDOR,           914 * 1024,     24 * 1024,    EMMC_USER_PART}, /* recovery_vendor   24M    p59 */
    {PART_SECURITY_DTB,              938 * 1024,      2 * 1024,    EMMC_USER_PART}, /* security_dtb       2M    p60 */
    {PART_DTBO,                      940 * 1024,     16 * 1024,    EMMC_USER_PART}, /* dtoimage          16M    p61 */
    {PART_TRUSTFIRMWARE,             956 * 1024,      2 * 1024,    EMMC_USER_PART}, /* trustfirmware      2M    p62 */
    {PART_MODEM_FW,                  958 * 1024,    174 * 1024,    EMMC_USER_PART}, /* modem_fw         174M    p63 */
    {PART_ENG_VENDOR,               1132 * 1024,     16 * 1024,    EMMC_USER_PART}, /* eng_vendor        16M    p64 */
#else
    {PART_RECOVERY_VENDOR,           914 * 1024,     16 * 1024,    EMMC_USER_PART}, /* recovery_vendor   16M    p59 */
    {PART_SECURITY_DTB,              930 * 1024,      2 * 1024,    EMMC_USER_PART}, /* dtimage            2M    p60 */
    {PART_DTBO,                      932 * 1024,     20 * 1024,    EMMC_USER_PART}, /* dtoimage          20M    p61 */
    {PART_TRUSTFIRMWARE,             952 * 1024,      2 * 1024,    EMMC_USER_PART}, /* trustfirmware      2M    p62 */
    {PART_MODEM_FW,                  954 * 1024,    174 * 1024,    EMMC_USER_PART}, /* modem_fw         174M    p63 */
    {PART_ENG_VENDOR,               1128 * 1024,     20 * 1024,    EMMC_USER_PART}, /* eng_vendor        20M    p64 */
#endif
    {PART_MODEM_PATCH_NV,           1148 * 1024,      4 * 1024,    EMMC_USER_PART}, /* modem_patch_nv     4M    p65 */
    {PART_MODEM_DRIVER,             1152 * 1024,     20 * 1024,    EMMC_USER_PART}, /* modem_driver      20M    p66 */
    {PART_RECOVERY_VBMETA,          1172 * 1024,      2 * 1024,    EMMC_USER_PART}, /* recovery_vbmeta    2M    p67 */
    {PART_ERECOVERY_VBMETA,         1174 * 1024,      2 * 1024,    EMMC_USER_PART}, /* erecovery_vbmeta   2M    p68 */
    {PART_VBMETA,                   1176 * 1024,      4 * 1024,    EMMC_USER_PART}, /* PART_VBMETA        4M    p69 */
    {PART_MODEMNVM_UPDATE,          1180 * 1024,     20 * 1024,    EMMC_USER_PART}, /* modemnvm_update   20M    p70 */
    {PART_MODEMNVM_CUST,            1200 * 1024,     16 * 1024,    EMMC_USER_PART}, /* modemnvm_cust     16M    p71 */
    {PART_PATCH,                    1216 * 1024,    128 * 1024,    EMMC_USER_PART}, /* patch            128M    p72 */
#ifdef CONFIG_FACTORY_MODE
    {PART_PREAS,                    1344 * 1024,    368 * 1024,    EMMC_USER_PART}, /* preas            368M    p73 */
    {PART_PREAVS,                   1712 * 1024,     32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p74 */
    {PART_SUPER,                    1744 * 1024,   9168 * 1024,    EMMC_USER_PART}, /* super           9168M    p75 */
    {PART_VERSION,                 10912 * 1024,    576 * 1024,    EMMC_USER_PART}, /* version          576M    p76 */
    {PART_PRELOAD,                 11488 * 1024,   1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p77 */
    {PART_HIBENCH_IMG,             12632 * 1024,    128 * 1024,    EMMC_USER_PART}, /* hibench_img      128M    p78 */
    {PART_HIBENCH_DATA,            12760 * 1024,    512 * 1024,    EMMC_USER_PART}, /* hibench_data     512M    p79 */
    {PART_FLASH_AGEING,            13272 * 1024,    512 * 1024,    EMMC_USER_PART}, /* FLASH_AGEING     512M    p80 */
    {PART_HIBENCH_LOG,             13784 * 1024,     32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG       32M    p81 */
    {PART_HIBENCH_LPM3,            13816 * 1024,     32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LPM3      32M    p82 */
    {PART_SECFLASH_AGEING,         13848 * 1024,     32 * 1024,    EMMC_USER_PART}, /* secflash_ageing   32M    p83 */
    {PART_USERDATA,                13880 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata    4G    p84 */
#elif defined CONFIG_MARKET_INTERNAL
#ifdef CONFIG_USE_EROFS
    {PART_PREAS,                1344 * 1024,        296 * 1024,    EMMC_USER_PART}, /* preas            296M    p73 */
    {PART_PREAVS,               1640 * 1024,         32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p74 */
    {PART_SUPER,                1672 * 1024,       6792 * 1024,    EMMC_USER_PART}, /* super           6792M    p75 */
    {PART_VERSION,              8464 * 1024,        576 * 1024,    EMMC_USER_PART}, /* version          576M    p76 */
    {PART_PRELOAD,              9040 * 1024,       1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p77 */
    {PART_USERDATA,            10184 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata    4G    p78 */
#else
    {PART_PREAS,                1344 * 1024,        368 * 1024,    EMMC_USER_PART}, /* preas            368M    p73 */
    {PART_PREAVS,               1712 * 1024,         32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p74 */
    {PART_SUPER,                1744 * 1024,       9168 * 1024,    EMMC_USER_PART}, /* super           9168M    p75 */
    {PART_VERSION,             10912 * 1024,        576 * 1024,    EMMC_USER_PART}, /* version          576M    p76 */
    {PART_PRELOAD,             11488 * 1024,       1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p77 */
    {PART_USERDATA,            12632 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata    4G    p78 */
#endif
#else
#ifdef CONFIG_USE_EROFS
    {PART_PREAS,                1344 * 1024,       1024 * 1024,    EMMC_USER_PART}, /* preas           1024M    p73 */
    {PART_PREAVS,               2368 * 1024,         32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p74 */
    {PART_SUPER,                2400 * 1024,       6768 * 1024,    EMMC_USER_PART}, /* super           6768M    p75 */
    {PART_VERSION,              9168 * 1024,        576 * 1024,    EMMC_USER_PART}, /* version          576M    p76 */
    {PART_PRELOAD,              9744 * 1024,       1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p77 */
    {PART_USERDATA,            10888 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata    4G    p78 */
#else
    {PART_PREAS,                1344 * 1024,       1280 * 1024,    EMMC_USER_PART}, /* preas           1280M    p73 */
    {PART_PREAVS,               2624 * 1024,         32 * 1024,    EMMC_USER_PART}, /* preavs            32M    p74 */
    {PART_SUPER,                2656 * 1024,       9384 * 1024,    EMMC_USER_PART}, /* super           9384M    p75 */
    {PART_VERSION,             12040 * 1024,        576 * 1024,    EMMC_USER_PART}, /* version          576M    p76 */
    {PART_PRELOAD,             12616 * 1024,       1144 * 1024,    EMMC_USER_PART}, /* preload         1144M    p77 */
    {PART_USERDATA,            13760 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata    4G    p78 */
#endif
#endif
    {"0", 0, 0, 0},                                        /* total 11848M */
};

static struct partition partition_table_ufs[] = {
    {PART_XLOADER,                   0,                 2 * 1024,    UFS_PART_0},
    {PART_RESERVED0,                 0,                 2 * 1024,    UFS_PART_1},
    {PART_PTABLE,                    0,                      512,    UFS_PART_2}, /* ptable           512K    */
    {PART_FRP,                       512,                    512,    UFS_PART_2}, /* frp              512K    p1 */
    {PART_PERSIST,                   1 * 1024,          6 * 1024,    UFS_PART_2}, /* persist         6144K    p2 */
    {PART_RESERVED1,                 7 * 1024,              1024,    UFS_PART_2}, /* reserved1       1024K    p3 */
    {PART_PTABLE_LU3,                0,                      512,    UFS_PART_3}, /* ptable_lu3       512K    p0 */
    {PART_VRL,                       512,                    512,    UFS_PART_3}, /* vrl              512K    p1 */
    {PART_VRL_BACKUP,                1024,                   512,    UFS_PART_3}, /* vrl backup       512K    p2 */
    {PART_MODEM_SECURE,              1536,                  8704,    UFS_PART_3}, /* modem_secure    8704K    p3 */
    {PART_NVME,                      10 * 1024,      5 * 1024,    UFS_PART_3}, /* nvme               5M    p4 */
    {PART_CTF,                       15 * 1024,      1 * 1024,    UFS_PART_3}, /* PART_CTF           1M    p5 */
    {PART_OEMINFO,                   16 * 1024,     96 * 1024,    UFS_PART_3}, /* oeminfo           96M    p6 */
    {PART_SECURE_STORAGE,            112 * 1024,    32 * 1024,    UFS_PART_3}, /* secure storage    32M    p7 */
    {PART_MODEMNVM_FACTORY,          144 * 1024,    32 * 1024,    UFS_PART_3}, /* modemnvm factory  16M    p8 */
    {PART_MODEMNVM_BACKUP,           176 * 1024,    32 * 1024,    UFS_PART_3}, /* modemnvm backup   16M    p9 */
    {PART_MODEMNVM_IMG,              208 * 1024,    64 * 1024,    UFS_PART_3}, /* modemnvm img      34M    p10 */
    {PART_HISEE_ENCOS,               272 * 1024,     4 * 1024,    UFS_PART_3}, /* hisee_encos        4M    p11 */
    {PART_VERITYKEY,                 276 * 1024,     1 * 1024,    UFS_PART_3}, /* reserved2          1M    p12 */
    {PART_DDR_PARA,                  277 * 1024,     1 * 1024,    UFS_PART_3}, /* DDR_PARA           1M    p13 */
    {PART_LOWPOWER_PARA,             278 * 1024,     1 * 1024,    UFS_PART_3}, /* lowpower_para      1M    p14 */
    {PART_BATT_TP_PARA,              279 * 1024,     1 * 1024,    UFS_PART_3}, /* batt_tp_para       1M    p15 */
    {PART_BL2,                       280 * 1024,     4 * 1024,    UFS_PART_3}, /* bl2                4M    p16 */
    {PART_RESERVED2,                 284 * 1024,    21 * 1024,    UFS_PART_3}, /* reserved2         21M    p17 */
    {PART_SPLASH2,                   305 * 1024,    80 * 1024,    UFS_PART_3}, /* splash2           80M    p18 */
    {PART_BOOTFAIL_INFO,             385 * 1024,     2 * 1024,    UFS_PART_3}, /* bootfail info      2M    p19 */
    {PART_MISC,                      387 * 1024,     2 * 1024,    UFS_PART_3}, /* misc               2M    p20 */
    {PART_DFX,                       389 * 1024,    16 * 1024,    UFS_PART_3}, /* dfx               16M    p21 */
    {PART_RRECORD,                   405 * 1024,    16 * 1024,    UFS_PART_3}, /* rrecord           16M    p22 */
    {PART_CACHE,                     421 * 1024,   104 * 1024,    UFS_PART_3}, /* cache            104M    p23 */
    {PART_FW_LPM3,                   525 * 1024,     1 * 1024,    UFS_PART_3}, /* fw_lpm3            1M    p24 */
    {PART_RESERVED3,                 526 * 1024,     5 * 1024,    UFS_PART_3}, /* reserved3A         5M    p25 */
    {PART_NPU,                       531 * 1024,     8 * 1024,    UFS_PART_3}, /* npu                8M    p26 */
    {PART_HIEPS,                     539 * 1024,     2 * 1024,    UFS_PART_3}, /* hieps              2M    p27 */
    {PART_IVP,                       541 * 1024,     2 * 1024,    UFS_PART_3}, /* ivp                2M    p28 */
    {PART_HDCP,                      543 * 1024,     1 * 1024,    UFS_PART_3}, /* PART_HDCP          1M    p29 */
    {PART_HISEE_IMG,                 544 * 1024,     4 * 1024,    UFS_PART_3}, /* part_hisee_img     4M    p30 */
    {PART_HHEE,                      548 * 1024,     4 * 1024,    UFS_PART_3}, /* hhee               4M    p31 */
    {PART_HISEE_FS,                  552 * 1024,     8 * 1024,    UFS_PART_3}, /* hisee_fs           8M    p32 */
    {PART_FASTBOOT,                  560 * 1024,    12 * 1024,    UFS_PART_3}, /* fastboot          12M    p33 */
    {PART_VECTOR,                    572 * 1024,     4 * 1024,    UFS_PART_3}, /* vector             4M    p34 */
    {PART_ISP_BOOT,                  576 * 1024,     2 * 1024,    UFS_PART_3}, /* isp_boot           2M    p35 */
    {PART_ISP_FIRMWARE,              578 * 1024,    14 * 1024,    UFS_PART_3}, /* isp_firmware      14M    p36 */
    {PART_FW_HIFI,                   592 * 1024,    12 * 1024,    UFS_PART_3}, /* hifi              12M    p37 */
    {PART_TEEOS,                     604 * 1024,     8 * 1024,    UFS_PART_3}, /* teeos              8M    p38 */
    {PART_SENSORHUB,                 612 * 1024,    16 * 1024,    UFS_PART_3}, /* sensorhub         16M    p39 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK,         628 * 1024,    12 * 1024,    UFS_PART_3}, /* erecovery_ramdisk 12M    p40 */
    {PART_ERECOVERY_VENDOR,          640 * 1024,     8 * 1024,    UFS_PART_3}, /* erecovery_vendor   8M    p41 */
    {PART_BOOT,                      648 * 1024,    65 * 1024,    UFS_PART_3}, /* boot              65M    p42 */
    {PART_RECOVERY,                  713 * 1024,    85 * 1024,    UFS_PART_3}, /* recovery          85M    p43 */
    {PART_ERECOVERY,                 798 * 1024,    12 * 1024,    UFS_PART_3}, /* erecovery         12M    p44 */
    {PART_METADATA,                  810 * 1024,    16 * 1024,    UFS_PART_3}, /* metadata          16M    p45 */
    {PART_KPATCH,                    826 * 1024,    29 * 1024,    UFS_PART_3}, /* reserved          29M    p46 */
#elif defined CONFIG_FACTORY_MODE
    {PART_ERECOVERY_RAMDISK,         628 * 1024,    32 * 1024,    UFS_PART_3}, /* erecovery_ramdisk 32M    p40 */
    {PART_ERECOVERY_VENDOR,          660 * 1024,    24 * 1024,    UFS_PART_3}, /* erecovery_vendor  24M    p41 */
    {PART_BOOT,                      684 * 1024,    30 * 1024,    UFS_PART_3}, /* boot              30M    p42 */
    {PART_RECOVERY,                  714 * 1024,    41 * 1024,    UFS_PART_3}, /* recovery          41M    p43 */
    {PART_ERECOVERY,                 755 * 1024,    41 * 1024,    UFS_PART_3}, /* erecovery         41M    p44 */
    {PART_METADATA,                  796 * 1024,    16 * 1024,    UFS_PART_3}, /* metadata          16M    p45 */
    {PART_KPATCH,                    812 * 1024,    43 * 1024,    UFS_PART_3}, /* kpatch            43M    p46 */
#else
    {PART_ERECOVERY_RAMDISK,         628 * 1024,    32 * 1024,    UFS_PART_3}, /* erecovery_ramdisk 32M    p40 */
    {PART_ERECOVERY_VENDOR,          660 * 1024,    16 * 1024,    UFS_PART_3}, /* erecovery_vendor  16M    p41 */
    {PART_BOOT,                      676 * 1024,    30 * 1024,    UFS_PART_3}, /* boot              30M    p42 */
    {PART_RECOVERY,                  706 * 1024,    45 * 1024,    UFS_PART_3}, /* recovery          45M    p43 */
    {PART_ERECOVERY,                 751 * 1024,    45 * 1024,    UFS_PART_3}, /* erecovery         45M    p44 */
    {PART_METADATA,                  796 * 1024,    16 * 1024,    UFS_PART_3}, /* metadata          16M    p45 */
    {PART_KPATCH,                    812 * 1024,    43 * 1024,    UFS_PART_3}, /* kpatch            43M    p46 */
#endif
    {PART_ENG_SYSTEM,                855 * 1024,    12 * 1024,    UFS_PART_3}, /* eng_system        12M    p47 */
    {PART_RAMDISK,                   867 * 1024,     2 * 1024,    UFS_PART_3}, /* ramdisk           32M    p48 */
    {PART_VBMETA_SYSTEM,             869 * 1024,     1 * 1024,    UFS_PART_3}, /* vbmeta_system      1M    p49 */
    {PART_VBMETA_VENDOR,             870 * 1024,     1 * 1024,    UFS_PART_3}, /* vbmeta_vendor      1M    p50 */
    {PART_VBMETA_ODM,                871 * 1024,     1 * 1024,    UFS_PART_3}, /* vbmeta_odm         1M    p51 */
    {PART_VBMETA_CUST,               872 * 1024,     1 * 1024,    UFS_PART_3}, /* vbmeta_cust        1M    p52 */
    {PART_VBMETA_HW_PRODUCT,         873 * 1024,     1 * 1024,    UFS_PART_3}, /* vbmeta_hw_product  1M    p53 */
    {PART_RECOVERY_RAMDISK,          874 * 1024,    32 * 1024,    UFS_PART_3}, /* recovery_ramdisk  32M    p54 */
#ifdef CONFIG_FACTORY_MODE
    {PART_RECOVERY_VENDOR,           906 * 1024,    24 * 1024,    UFS_PART_3}, /* recovery_vendor   24M    p55 */
    {PART_SECURITY_DTB,              930 * 1024,     2 * 1024,    UFS_PART_3}, /* security_dtb       2M    p56 */
    {PART_DTBO,                      932 * 1024,    16 * 1024,    UFS_PART_3}, /* dtoimage          16M    p57 */
    {PART_TRUSTFIRMWARE,             948 * 1024,     2 * 1024,    UFS_PART_3}, /* trustfirmware      2M    p58 */
    {PART_MODEM_FW,                  950 * 1024,   174 * 1024,    UFS_PART_3}, /* modem_fw         174M    p59 */
    {PART_ENG_VENDOR,               1124 * 1024,    16 * 1024,    UFS_PART_3}, /* eng_vendor        16M    p60 */
#else
    {PART_RECOVERY_VENDOR,           906 * 1024,    16 * 1024,    UFS_PART_3}, /* recovery_vendor   16M    p55 */
    {PART_SECURITY_DTB,              922 * 1024,     2 * 1024,    UFS_PART_3}, /* security_dtb       2M    p56 */
    {PART_DTBO,                      924 * 1024,    20 * 1024,    UFS_PART_3}, /* dtoimage          20M    p57 */
    {PART_TRUSTFIRMWARE,             944 * 1024,     2 * 1024,    UFS_PART_3}, /* trustfirmware      2M    p58 */
    {PART_MODEM_FW,                  946 * 1024,   174 * 1024,    UFS_PART_3}, /* modem_fw         174M    p59 */
    {PART_ENG_VENDOR,               1120 * 1024,    20 * 1024,    UFS_PART_3}, /* eng_vendor        20M    p60 */
#endif
    {PART_MODEM_PATCH_NV,           1140 * 1024,     4 * 1024,    UFS_PART_3}, /* modem_patch_nv     4M    p61 */
    {PART_MODEM_DRIVER,             1144 * 1024,    20 * 1024,    UFS_PART_3}, /* modem_driver      20M    p62 */
    {PART_RECOVERY_VBMETA,          1164 * 1024,     2 * 1024,    UFS_PART_3}, /* recovery_vbmeta    2M    p63 */
    {PART_ERECOVERY_VBMETA,         1166 * 1024,     2 * 1024,    UFS_PART_3}, /* erecovery_vbmeta   2M    p64 */
    {PART_VBMETA,                   1168 * 1024,     4 * 1024,    UFS_PART_3}, /* PART_VBMETA        4M    p65 */
    {PART_MODEMNVM_UPDATE,          1172 * 1024,    20 * 1024,    UFS_PART_3}, /* modemnvm_update   16M    p66 */
    {PART_MODEMNVM_CUST,            1192 * 1024,    16 * 1024,    UFS_PART_3}, /* modemnvm_cust     16M    p67 */
    {PART_PATCH,                    1208 * 1024,   128 * 1024,    UFS_PART_3}, /* patch            128M    p68 */
#ifdef CONFIG_FACTORY_MODE
    {PART_PREAS,                    1336 * 1024,   368 * 1024,    UFS_PART_3}, /* preas            368M    p69 */
    {PART_PREAVS,                   1704 * 1024,    32 * 1024,    UFS_PART_3}, /* preavs            32M    p70 */
    {PART_SUPER,                    1736 * 1024,  9168 * 1024,    UFS_PART_3}, /* super           9168M    p71 */
    {PART_VERSION,                 10904 * 1024,   576 * 1024,    UFS_PART_3}, /* version          576M    p72 */
    {PART_PRELOAD,                 11480 * 1024,  1144 * 1024,    UFS_PART_3}, /* preload         1144M    p73 */
    {PART_HIBENCH_IMG,             12624 * 1024,   128 * 1024,    UFS_PART_3}, /* hibench_img      128M    p74 */
    {PART_HIBENCH_DATA,            12752 * 1024,   512 * 1024,    UFS_PART_3}, /* hibench_data     512M    p75 */
    {PART_FLASH_AGEING,            13264 * 1024,   512 * 1024,    UFS_PART_3}, /* FLASH_AGEING     512M    p76 */
    {PART_HIBENCH_LOG,             13776 * 1024,    32 * 1024,    UFS_PART_3}, /* HIBENCH_LOG       32M    p77 */
    {PART_HIBENCH_LPM3,            13808 * 1024,    32 * 1024,    UFS_PART_3}, /* HIBENCH_LPM3      32M    p78 */
    {PART_SECFLASH_AGEING,         13840 * 1024,    32 * 1024,    UFS_PART_3}, /* secflash_ageing   32M    p79 */
    {PART_USERDATA,                13872 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata    4G    p80 */
#elif defined CONFIG_MARKET_INTERNAL
#ifdef CONFIG_USE_EROFS
    {PART_PREAS,            1336 * 1024,        296 * 1024,    UFS_PART_3}, /* preas            296M    p69 */
    {PART_PREAVS,           1632 * 1024,         32 * 1024,    UFS_PART_3}, /* preavs            32M    p70 */
    {PART_SUPER,            1664 * 1024,       6792 * 1024,    UFS_PART_3}, /* super           6792M    p71 */
    {PART_VERSION,          8456 * 1024,        576 * 1024,    UFS_PART_3}, /* version          576M    p72 */
    {PART_PRELOAD,          9032 * 1024,       1144 * 1024,    UFS_PART_3}, /* preload         1144M    p73 */
    {PART_USERDATA,        10176 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata    4G    p74 */
#else
    {PART_PREAS,            1336 * 1024,        368 * 1024,    UFS_PART_3}, /* preas            368M    p69 */
    {PART_PREAVS,           1704 * 1024,         32 * 1024,    UFS_PART_3}, /* preavs            32M    p70 */
    {PART_SUPER,            1736 * 1024,       9168 * 1024,    UFS_PART_3}, /* super           9168M    p71 */
    {PART_VERSION,         10904 * 1024,        576 * 1024,    UFS_PART_3}, /* version          576M    p72 */
    {PART_PRELOAD,         11480 * 1024,       1144 * 1024,    UFS_PART_3}, /* preload         1144M    p73 */
    {PART_USERDATA,        12624 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata    4G    p74 */
#endif
#else
#ifdef CONFIG_USE_EROFS
    {PART_PREAS,            1336 * 1024,       1024 * 1024,    UFS_PART_3}, /* preas           1024M    p69 */
    {PART_PREAVS,           2360 * 1024,         32 * 1024,    UFS_PART_3}, /* preavs            32M    p70 */
    {PART_SUPER,            2392 * 1024,       6768 * 1024,    UFS_PART_3}, /* super           6768M    p71 */
    {PART_VERSION,          9160 * 1024,        576 * 1024,    UFS_PART_3}, /* version          576M    p72 */
    {PART_PRELOAD,          9736 * 1024,       1144 * 1024,    UFS_PART_3}, /* preload         1144M    p73 */
    {PART_USERDATA,        10880 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata    4G    p74 */
#else
    {PART_PREAS,            1336 * 1024,       1280 * 1024,    UFS_PART_3}, /* preas           1280M    p69 */
    {PART_PREAVS,           2616 * 1024,         32 * 1024,    UFS_PART_3}, /* preavs            32M    p70 */
    {PART_SUPER,            2648 * 1024,       9384 * 1024,    UFS_PART_3}, /* super           9384M    p71 */
    {PART_VERSION,         12032 * 1024,        576 * 1024,    UFS_PART_3}, /* version          576M    p72 */
    {PART_PRELOAD,         12608 * 1024,       1144 * 1024,    UFS_PART_3}, /* preload         1144M    p73 */
    {PART_USERDATA,        13752 * 1024, (4UL) * 1024 * 1024,    UFS_PART_3}, /* userdata    4G    p74 */
#endif
#endif
    {PART_PTABLE_LU4,                0,              512,   UFS_PART_4}, /* ptable_lu4        512K    p0 */
    {PART_RESERVED12,              512,             1536,   UFS_PART_4}, /* reserved12       1536K    p1 */
    {PART_USERDATA2,              2048,  (4UL) * 1024 * 1024,   UFS_PART_4}, /* userdata2           4G    p2 */
    {"0", 0, 0, 0},
};

#endif
