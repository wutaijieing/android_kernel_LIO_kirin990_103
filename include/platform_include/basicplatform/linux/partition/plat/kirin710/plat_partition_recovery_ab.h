/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: the partition table.
 */

#ifndef _PLAT_PARTITION_RECOVERY_AB_H_
#define _PLAT_PARTITION_RECOVERY_AB_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"
#include "partition_def.h"

static const struct partition partition_table_emmc[] = {
    {PART_XLOADER_A,        0,         2 * 1024,        EMMC_BOOT_MAJOR_PART},
    {PART_XLOADER_B,        0,         2 * 1024,        EMMC_BOOT_BACKUP_PART},
    {PART_PTABLE,           0,         512,           EMMC_USER_PART}, /* ptable          512K */
    {PART_FRP,              512,       512,           EMMC_USER_PART}, /* frp             512K   p1 */
    {PART_PERSIST,          1024,      2048,          EMMC_USER_PART}, /* persist         2048K  p2 */
    {PART_RESERVED1,        3072,      5120,          EMMC_USER_PART}, /* reserved1       5120K  p3 */
    {PART_RESERVED6,         8 * 1024,        512,        EMMC_USER_PART}, /* reserved6       512K   p4 */
    {PART_VRL_A,                 8704,        512,        EMMC_USER_PART}, /* vrl             512K   p5 */
    {PART_VRL_BACKUP_A,          9216,        512,        EMMC_USER_PART}, /* vrl backup      512K   p6 */
    {PART_MODEM_SECURE,          9728,       8704,        EMMC_USER_PART}, /* modem_secure    8704k  p7 */
    {PART_NVME,             18 * 1024,   5 * 1024,        EMMC_USER_PART}, /* nvme            5M     p8 */
    {PART_CTF,              23 * 1024,   1 * 1024,        EMMC_USER_PART}, /* PART_CTF        1M     p9 */
    {PART_OEMINFO,          24 * 1024,   64 * 1024,       EMMC_USER_PART}, /* oeminfo         64M    p10 */
    {PART_SECURE_STORAGE,   88 * 1024,   32 * 1024,       EMMC_USER_PART}, /* secure storage  32M    p11 */
    {PART_MODEM_OM,         120 * 1024,  32 * 1024,       EMMC_USER_PART}, /* modem om        32M    p12 */
    {PART_MODEMNVM_FACTORY, 152 * 1024,  16 * 1024,       EMMC_USER_PART}, /* modemnvm factory16M    p13 */
    {PART_MODEMNVM_BACKUP,  168 * 1024,  16 * 1024,       EMMC_USER_PART}, /* modemnvm backup 16M    p14 */
    {PART_MODEMNVM_IMG,     184 * 1024,  34 * 1024,       EMMC_USER_PART}, /* modemnvm img    34M    p15 */
    {PART_RESERVED7,        218 * 1024,  2 * 1024,        EMMC_USER_PART}, /* reserved7       2M     p16 */
    {PART_HISEE_ENCOS,      220 * 1024,  4 * 1024,        EMMC_USER_PART}, /* hisee_encos     4M     p17 */
    {PART_VERITYKEY,        224 * 1024,  1 * 1024,        EMMC_USER_PART}, /* veritykey       32M    p18 */
    {PART_DDR_PARA,         225 * 1024,  1 * 1024,        EMMC_USER_PART}, /* ddr_para        1M     p19 */
    {PART_MODEM_DRIVER,     226 * 1024,  20 * 1024,       EMMC_USER_PART}, /* modem_driver    20M    p20 */
    {PART_RAMDISK,             246 * 1024,   2 * 1024,    EMMC_USER_PART}, /* ramdisk           32M    p21 */
    {PART_VBMETA_SYSTEM,       248 * 1024,   1 * 1024,    EMMC_USER_PART}, /* vbmeta_system      1M    p22 */
    {PART_VBMETA_VENDOR,       249 * 1024,   1 * 1024,    EMMC_USER_PART}, /* vbmeta_vendor      1M    p23 */
    {PART_VBMETA_ODM,          250 * 1024,   1 * 1024,    EMMC_USER_PART}, /* vbmeta_odm         1M    p24 */
    {PART_VBMETA_CUST,         251 * 1024,   1 * 1024,    EMMC_USER_PART}, /* vbmeta_cust        1M    p25 */
    {PART_VBMETA_HW_PRODUCT,   252 * 1024,   1 * 1024,    EMMC_USER_PART}, /* vbmeta_hw_product  1M    p26 */
    {PART_SPLASH2,             253 * 1024,  80 * 1024,    EMMC_USER_PART}, /* splash2           80M    p27 */
    {PART_BOOTFAIL_INFO,       333 * 1024,   2 * 1024,    EMMC_USER_PART}, /* bootfail info     2MB    p28 */
    {PART_MISC,                335 * 1024,   2 * 1024,    EMMC_USER_PART}, /* misc              2M     p29 */
    {PART_DFX,                 337 * 1024,  16 * 1024,    EMMC_USER_PART}, /* dfx               16M    p30 */
    {PART_RRECORD,             353 * 1024,  16 * 1024,    EMMC_USER_PART}, /* rrecord           16M    p31 */
    {PART_FW_LPM3_A,           369 * 1024,        256,    EMMC_USER_PART}, /* mcuimage          256K   p32 */
    {PART_KPATCH,              378112,           3840,    EMMC_USER_PART}, /* kpatch            3840KB p33 */
    {PART_HDCP,                373 * 1024,   1 * 1024,    EMMC_USER_PART}, /* PART_HDCP          1M    p34 */
    {PART_HISEE_IMG,           374 * 1024,   4 * 1024,    EMMC_USER_PART}, /* part_hisee_img     4M    p35 */
    {PART_HHEE_A,              378 * 1024,   4 * 1024,    EMMC_USER_PART}, /* part_hhee          4M    p36 */
    {PART_HISEE_FS,            382 * 1024,   8 * 1024,    EMMC_USER_PART}, /* hisee_fs          8M     p37 */
    {PART_FASTBOOT_A,          390 * 1024,  12 * 1024,    EMMC_USER_PART}, /* fastboot          12M    p38 */
    {PART_VECTOR_A,            402 * 1024,   4 * 1024,    EMMC_USER_PART}, /* avs vector        4M     p39 */
    {PART_ISP_BOOT,            406 * 1024,   2 * 1024,    EMMC_USER_PART}, /* isp_boot          2M     p40 */
    {PART_ISP_FIRMWARE,        408 * 1024,  14 * 1024,    EMMC_USER_PART}, /* isp_firmware      14M    p41 */
    {PART_FW_HIFI,             422 * 1024,  12 * 1024,    EMMC_USER_PART}, /* hifi              12M    p42 */
    {PART_TEEOS_A,             434 * 1024,   8 * 1024,    EMMC_USER_PART}, /* teeos             8M     p43 */
    {PART_SENSORHUB_A,         442 * 1024,  16 * 1024,    EMMC_USER_PART}, /* sensorhub         16M    p44 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK,   458 * 1024,  32 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 32M    p45 */
    {PART_ERECOVERY_VENDOR,    490 * 1024,  16 * 1024,    EMMC_USER_PART}, /* erecovery_vendor  16M    p46 */
    {PART_BOOT,                506 * 1024,  40 * 1024,    EMMC_USER_PART}, /* boot              40M    p47 */
    {PART_RECOVERY,            546 * 1024,  64 * 1024,    EMMC_USER_PART}, /* recovery          64M    p48 */
    {PART_ERECOVERY,           610 * 1024,  64 * 1024,    EMMC_USER_PART}, /* erecovery         64M    p49 */
    {PART_METADATA,            674 * 1024,  16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p50 */
    {PART_RESERVED8,           690 * 1024,  66 * 1024,    EMMC_USER_PART}, /* reserved8         66M    p51 */
#else
    {PART_ERECOVERY_RAMDISK,   458 * 1024,  32 * 1024,    EMMC_USER_PART}, /* erecovery_ramdisk 32M    p45 */
    {PART_ERECOVERY_VENDOR,    490 * 1024,  24 * 1024,    EMMC_USER_PART}, /* erecovery_vendor  24M    p46 */
    {PART_BOOT,                514 * 1024,  32 * 1024,    EMMC_USER_PART}, /* boot              32M    p47 */
    {PART_RECOVERY,            546 * 1024,  56 * 1024,    EMMC_USER_PART}, /* recovery          56M    p48 */
    {PART_ERECOVERY,           602 * 1024,  56 * 1024,    EMMC_USER_PART}, /* erecovery         56M    p49 */
    {PART_METADATA,            658 * 1024,  16 * 1024,    EMMC_USER_PART}, /* metadata          16M    p50 */
    {PART_RESERVED8,           674 * 1024,  82 * 1024,    EMMC_USER_PART}, /* reserved8         82M    p51 */
#endif
    {PART_BL2_A,               756 * 1024,   4 * 1024,     EMMC_USER_PART}, /* bl2               4M    p52 */
    {PART_ENG_SYSTEM,          760 * 1024,  12 * 1024,     EMMC_USER_PART}, /* eng_system       12M    p53 */
    {PART_RECOVERY_RAMDISK,    772 * 1024,  32 * 1024,     EMMC_USER_PART}, /* recovery_ramdisk 32M    p54 */
    {PART_RECOVERY_VENDOR,     804 * 1024,  24 * 1024,     EMMC_USER_PART}, /* recovery_vendor  24M    p55 */
    {PART_FW_DTB_A,            828 * 1024,   2 * 1024,     EMMC_USER_PART}, /* fw_dtb            2M    p56 */
    {PART_DTBO_A,              830 * 1024,  24 * 1024,     EMMC_USER_PART}, /* dtoimage         24M    p57 */
    {PART_TRUSTFIRMWARE_A,     854 * 1024,   2 * 1024,     EMMC_USER_PART}, /* trustfirmware     2M    p58 */
    {PART_MODEM_FW,            856 * 1024,  56 * 1024,     EMMC_USER_PART}, /* modem_fw         56M    p59 */
    {PART_ENG_VENDOR,          912 * 1024,  12 * 1024,     EMMC_USER_PART}, /* eng_vendor       12M    p60 */
    {PART_MODEM_PATCH_NV,      924 * 1024,   4 * 1024,     EMMC_USER_PART}, /* modem_patch_nv    4M    p61 */
    {PART_RECOVERY_VBMETA,     928 * 1024,   2 * 1024,     EMMC_USER_PART}, /* recovery_vbmeta   2M    p62 */
    {PART_ERECOVERY_VBMETA,    930 * 1024,   2 * 1024,     EMMC_USER_PART}, /* erecovery_vbmeta  2M    p63 */
    {PART_VBMETA,              932 * 1024,   4 * 1024,     EMMC_USER_PART}, /* PART_VBMETA       4M    p64 */
    {PART_MODEMNVM_UPDATE,     936 * 1024,  16 * 1024,     EMMC_USER_PART}, /* modemnvm_update  16M    p65 */
    {PART_MODEMNVM_CUST,       952 * 1024,  16 * 1024,     EMMC_USER_PART}, /* modemnvm_cust    16M    p66 */
    {PART_PATCH,               968 * 1024,  32 * 1024,     EMMC_USER_PART}, /* patch            32M    p67 */
    {PART_CACHE,              1000 * 1024, 104 * 1024,     EMMC_USER_PART}, /* cache           104M    p68 */
    {PART_VRL_B,              1104 * 1024,        512,     EMMC_USER_PART}, /* vrl             512K    p69 */
    {PART_VRL_BACKUP_B,           1131008,        512,     EMMC_USER_PART}, /* vrl backup      512K    p70 */
    {PART_FW_LPM3_B,          1105 * 1024,        256,     EMMC_USER_PART}, /* mcuimage        256K    p71 */
    {PART_HHEE_B,                   1131776,   4 * 1024,    EMMC_USER_PART}, /* part_hhee        4M    p72 */
    {PART_FASTBOOT_B,               1135872,  12 * 1024,    EMMC_USER_PART}, /* fastboot        12M    p73 */
    {PART_VECTOR_B,                 1148160,   4 * 1024,    EMMC_USER_PART}, /* avs vector       4M    p74 */
    {PART_TEEOS_B,                  1152256,   8 * 1024,    EMMC_USER_PART}, /* teeos            8M    p75 */
    {PART_BL2_B,                    1160448,   4 * 1024,    EMMC_USER_PART}, /* bl2              4M    p76 */
    {PART_FW_DTB_B,                 1164544,   2 * 1024,    EMMC_USER_PART}, /* fw_dtb           2M    p77 */
    {PART_DTBO_B,                   1166592,  24 * 1024,    EMMC_USER_PART}, /* dtoimage        24M    p78 */
    {PART_TRUSTFIRMWARE_B,          1191168,   2 * 1024,    EMMC_USER_PART}, /* trustfirmware    2M    p79 */
    {PART_SENSORHUB_B,              1193216,  16 * 1024,    EMMC_USER_PART}, /* sensorhub       16M    p80 */
    {PART_RECOVERYAB_RESERVED,      1209600,  32 * 1024 + 768,    EMMC_USER_PART}, /* recovery_ab_reserved 110M p81 */
#ifdef CONFIG_FACTORY_MODE
    {PART_PREAS,              1214 * 1024,          136 * 1024,    EMMC_USER_PART}, /* preas          136M    p82 */
    {PART_PREAVS,             1350 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs          32M    p83 */
    {PART_SUPER,              1382 * 1024,         7020 * 1024,    EMMC_USER_PART}, /* super         7020M    p84 */
    {PART_VERSION,            8402 * 1024,          576 * 1024,    EMMC_USER_PART}, /* version        576M    p85 */
    {PART_PRELOAD,            8978 * 1024,          900 * 1024,    EMMC_USER_PART}, /* preload        900M    p86 */
    {PART_HIBENCH_IMG,        9878 * 1024,          128 * 1024,    EMMC_USER_PART}, /* hibench_img    128M    p87 */
    {PART_HIBENCH_DATA,      10006 * 1024,          512 * 1024,    EMMC_USER_PART}, /* hibench_data   512M    p88 */
    {PART_FLASH_AGEING,      10518 * 1024,          512 * 1024,    EMMC_USER_PART}, /* flash_ageing   512M    p89 */
    {PART_HIBENCH_LOG,       11030 * 1024,           32 * 1024,    EMMC_USER_PART}, /* HIBENCH_LOG     32M    p90 */
    {PART_USERDATA,          11062 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata         4G    p91 */
#else
#ifdef CONFIG_USE_EROFS
#ifdef CONFIG_MARKET_INTERNAL
    {PART_PREAS,              1214 * 1024,          312 * 1024,    EMMC_USER_PART}, /* preas           312M    p82 */
    {PART_PREAVS,             1526 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs           32M    p83 */
    {PART_SUPER,              1558 * 1024,         5064 * 1024,    EMMC_USER_PART}, /* super          5064M    p84 */
    {PART_VERSION,            6622 * 1024,           32 * 1024,    EMMC_USER_PART}, /* version          32M    p85 */
    {PART_PRELOAD,            6654 * 1024,          928 * 1024,    EMMC_USER_PART}, /* preload         928M    p86 */
    {PART_USERDATA,           7582 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata        4G      p87 */
#elif defined CONFIG_MARKET_OVERSEA
    {PART_PREAS,              1214 * 1024,         1312 * 1024,    EMMC_USER_PART}, /* preas          1312M    p82 */
    {PART_PREAVS,             2526 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs           32M    p83 */
    {PART_SUPER,              2558 * 1024,         4344 * 1024,    EMMC_USER_PART}, /* super          4344M    p84 */
    {PART_VERSION,            6902 * 1024,           32 * 1024,    EMMC_USER_PART}, /* version          32M    p85 */
    {PART_PRELOAD,            6934 * 1024,          928 * 1024,    EMMC_USER_PART}, /* preload         928M    p86 */
    {PART_USERDATA,           7862 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata        4G      p87 */
#else
    {PART_PREAS,              1214 * 1024,          136 * 1024,    EMMC_USER_PART}, /* preas           136M    p82 */
    {PART_PREAVS,             1350 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs           32M    p83 */
    {PART_SUPER,              1382 * 1024,         7020 * 1024,    EMMC_USER_PART}, /* super          7020M    p84 */
    {PART_VERSION,            8402 * 1024,          576 * 1024,    EMMC_USER_PART}, /* version         576M    p85 */
    {PART_PRELOAD,            8978 * 1024,          900 * 1024,    EMMC_USER_PART}, /* preload         900M    p86 */
    {PART_USERDATA,           9878 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata          4G    p87 */
#endif /* internal */
#else /* not erofs, ext4 */
#ifdef CONFIG_MARKET_INTERNAL
    {PART_PREAS,              1214 * 1024,          136 * 1024,    EMMC_USER_PART}, /* preas           136M    p82 */
    {PART_PREAVS,             1350 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs           32M    p83 */
    {PART_SUPER,              1382 * 1024,         4176 * 1024,    EMMC_USER_PART}, /* super          4176M    p84 */
    {PART_VERSION,            5558 * 1024,           32 * 1024,    EMMC_USER_PART}, /* version          32M    p85 */
    {PART_PRELOAD,            5590 * 1024,          928 * 1024,    EMMC_USER_PART}, /* preload         928M    p86 */
    {PART_USERDATA,           6518 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata        4G      p87 */
#elif defined CONFIG_MARKET_OVERSEA
    {PART_PREAS,              1214 * 1024,         1008 * 1024,    EMMC_USER_PART}, /* preas          1008M    p82 */
    {PART_PREAVS,             2222 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs           32M    p83 */
    {PART_SUPER,              2254 * 1024,         3536 * 1024,    EMMC_USER_PART}, /* super          3536M    p84 */
    {PART_VERSION,            5790 * 1024,           32 * 1024,    EMMC_USER_PART}, /* version          32M    p85 */
    {PART_PRELOAD,            5822 * 1024,          928 * 1024,    EMMC_USER_PART}, /* preload         928M    p86 */
    {PART_USERDATA,           6750 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata        4G      p87 */
#else
    {PART_PREAS,              1214 * 1024,          136 * 1024,    EMMC_USER_PART}, /* preas           136M    p82 */
    {PART_PREAVS,             1350 * 1024,           32 * 1024,    EMMC_USER_PART}, /* preavs           32M    p83 */
    {PART_SUPER,              1382 * 1024,         7020 * 1024,    EMMC_USER_PART}, /* super          7020M    p84 */
    {PART_VERSION,            8402 * 1024,          576 * 1024,    EMMC_USER_PART}, /* version         576M    p85 */
    {PART_PRELOAD,            8978 * 1024,          900 * 1024,    EMMC_USER_PART}, /* preload         900M    p86 */
    {PART_USERDATA,           9878 * 1024, (4UL) * 1024 * 1024,    EMMC_USER_PART}, /* userdata          4G    p87 */
#endif /* internal */
#endif /* erofs, ext4 */
#endif /* factory */
    {"0", 0, 0, 0},
};

static const struct partition partition_table_ufs[] = {
    {"0", 0, 0, 0},
};

#endif
