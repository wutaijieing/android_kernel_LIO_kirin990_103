/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *  Description: the partition table.
 */

#ifndef _TAURUS_CDC_ACE_CS2_PLAT_PARTITION_H_
#define _TAURUS_CDC_ACE_CS2_PLAT_PARTITION_H_

#include "partition_macro.h"
#include "partition_macro_plus.h"
#include "partition_def.h"

static const struct partition partition_table_emmc[] = {
    {"0", 0, 0, 0},
};

static struct partition partition_table_ufs[] = {
    {PART_XLOADER_A,                        0,      2 * 1024,      UFS_PART_0},
    {PART_XLOADER_B,                        0,      2 * 1024,      UFS_PART_1},
    {PART_PTABLE,                           0,           512,      UFS_PART_2}, /* ptable             512K    p0 */
    {PART_VRL_A,                          512,           512,      UFS_PART_2}, /* vrl_a              512K    p1 */
    {PART_VRL_BACKUP_A,               2 * 512,           512,      UFS_PART_2}, /* vrl backup_a       512K    p2 */
    {PART_RESERVED1,                  3 * 512,           512,      UFS_PART_2}, /* reserved1          512K    p3 */
    {PART_NPU_A,                     2 * 1024,      8 * 1024,      UFS_PART_2}, /* npu_a                8M    p4 */
    {PART_HIEPS_A,                  10 * 1024,      2 * 1024,      UFS_PART_2}, /* hieps_a              2M    p5 */
    {PART_IVP_A,                    12 * 1024,      2 * 1024,      UFS_PART_2}, /* ivp_a                2M    p6 */
    {PART_HDCP_A,                   14 * 1024,      1 * 1024,      UFS_PART_2}, /* PART_HDCP_a          1M    p7 */
    {PART_HISEE_IMG_A,              15 * 1024,      4 * 1024,      UFS_PART_2}, /* part_hisee_img_a     4M    p8 */
    {PART_HHEE_A,                   19 * 1024,      4 * 1024,      UFS_PART_2}, /* hhee_a               4M    p9 */
    {PART_FASTBOOT_A,               23 * 1024,     12 * 1024,      UFS_PART_2}, /* fastboot_a          12M    p10 */
    {PART_LOWPOWER_PARA_A,          35 * 1024,      1 * 1024,      UFS_PART_2}, /* lowpower_para_a      1M    p11 */
    {PART_BL2_A,                    36 * 1024,      4 * 1024,      UFS_PART_2}, /* bl2_a                4M    p12 */
    {PART_TEEOS_A,                  40 * 1024,      8 * 1024,      UFS_PART_2}, /* teeos_a              8M    p13 */
    {PART_FW_LPM3_A,                48 * 1024,      1 * 1024,      UFS_PART_2}, /* fw_lpm3_a            1M    p14 */
    {PART_HISEE_FS_A,               49 * 1024,      8 * 1024,      UFS_PART_2}, /* hisee_fs_a           8M    p15 */
    {PART_VECTOR_A,                 57 * 1024,      4 * 1024,      UFS_PART_2}, /* vector_a             4M    p16 */
    {PART_ISP_BOOT_A,               61 * 1024,      2 * 1024,      UFS_PART_2}, /* isp_boot_a           2M    p17 */
    {PART_ISP_FIRMWARE_A,           63 * 1024,     14 * 1024,      UFS_PART_2}, /* isp_firmware_a      14M    p18 */
    {PART_FW_HIFI_A,                77 * 1024,     12 * 1024,      UFS_PART_2}, /* hifi_a              12M    p19 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK_A,      89 * 1024,     12 * 1024,      UFS_PART_2}, /* erecovery_ramdisk_a 12M    p20 */
    {PART_ERECOVERY_VENDOR_A,      101 * 1024,      8 * 1024,      UFS_PART_2}, /* erecovery_vendor_a   8M    p21 */
    {PART_BOOT_A,                  109 * 1024,     65 * 1024,      UFS_PART_2}, /* boot_a              65M    p22 */
    {PART_RECOVERY_A,              174 * 1024,     85 * 1024,      UFS_PART_2}, /* recovery_a          85M    p23 */
    {PART_ERECOVERY_A,             259 * 1024,     12 * 1024,      UFS_PART_2}, /* erecovery_a         12M    p24 */
    {PART_KPATCH_A,                271 * 1024,     29 * 1024,      UFS_PART_2}, /* kpatch_a            29M    p25 */
#elif defined CONFIG_FACTORY_MODE
    {PART_ERECOVERY_RAMDISK_A,      89 * 1024,     32 * 1024,      UFS_PART_2}, /* erecovery_ramdisk_a 32M    p20 */
    {PART_ERECOVERY_VENDOR_A,      121 * 1024,     24 * 1024,      UFS_PART_2}, /* erecovery_vendor_a  24M    p21 */
    {PART_BOOT_A,                  145 * 1024,     30 * 1024,      UFS_PART_2}, /* boot_a              30M    p22 */
    {PART_RECOVERY_A,              175 * 1024,     41 * 1024,      UFS_PART_2}, /* recovery_a          41M    p23 */
    {PART_ERECOVERY_A,             216 * 1024,     41 * 1024,      UFS_PART_2}, /* erecovery_a         41M    p24 */
    {PART_KPATCH_A,                257 * 1024,     43 * 1024,      UFS_PART_2}, /* kpatch_a            43M    p25 */
#else
    {PART_ERECOVERY_RAMDISK_A,      89 * 1024,     32 * 1024,      UFS_PART_2}, /* erecovery_ramdisk_a 32M    p20 */
    {PART_ERECOVERY_VENDOR_A,      121 * 1024,     16 * 1024,      UFS_PART_2}, /* erecovery_vendor_a  16M    p21 */
    {PART_BOOT_A,                  137 * 1024,     30 * 1024,      UFS_PART_2}, /* boot_a              30M    p22 */
    {PART_RECOVERY_A,              167 * 1024,     45 * 1024,      UFS_PART_2}, /* recovery_a          45M    p23 */
    {PART_ERECOVERY_A,             212 * 1024,     45 * 1024,      UFS_PART_2}, /* erecovery_a         45M    p24 */
    {PART_KPATCH_A,                257 * 1024,     43 * 1024,      UFS_PART_2}, /* kpatch_a            43M    p25 */
#endif
    {PART_ENG_SYSTEM_A,            300 * 1024,     12 * 1024,      UFS_PART_2}, /* eng_system_a        12M    p26 */
    {PART_RAMDISK_A,               312 * 1024,      2 * 1024,      UFS_PART_2}, /* ramdisk_a            2M    p27 */
    {PART_VBMETA_SYSTEM_A,         314 * 1024,      1 * 1024,      UFS_PART_2}, /* vbmeta_system_a      1M    p28 */
    {PART_VBMETA_VENDOR_A,         315 * 1024,      1 * 1024,      UFS_PART_2}, /* vbmeta_vendor_a      1M    p29 */
    {PART_VBMETA_ODM_A,            316 * 1024,      1 * 1024,      UFS_PART_2}, /* vbmeta_odm_a         1M    p30 */
    {PART_VBMETA_CUST_A,           317 * 1024,      1 * 1024,      UFS_PART_2}, /* vbmeta_cus_a         1M    p31 */
    {PART_VBMETA_HW_PRODUCT_A,     318 * 1024,      1 * 1024,      UFS_PART_2}, /* vbmeta_hw_product_a  1M    p32 */
    {PART_RECOVERY_RAMDISK_A,      319 * 1024,     32 * 1024,      UFS_PART_2}, /* recovery_ramdisk_a  32M    p33 */
#ifdef CONFIG_FACTORY_MODE
    {PART_RECOVERY_VENDOR_A,       351 * 1024,     24 * 1024,      UFS_PART_2}, /* recovery_vendor_a   24M    p34 */
    {PART_DTBO_A,                  375 * 1024,     16 * 1024,      UFS_PART_2}, /* dtoimage_a          16M    p35 */
    {PART_ENG_VENDOR_A,            391 * 1024,     16 * 1024,      UFS_PART_2}, /* eng_vendor_a        16M    p36 */
#else
    {PART_RECOVERY_VENDOR_A,       351 * 1024,     16 * 1024,      UFS_PART_2}, /* recovery_vendor_a   16M    p34 */
    {PART_DTBO_A,                  367 * 1024,     20 * 1024,      UFS_PART_2}, /* dtoimage_a          20M    p35 */
    {PART_ENG_VENDOR_A,            387 * 1024,     20 * 1024,      UFS_PART_2}, /* eng_vendor_a        20M    p36 */
#endif
    {PART_SECURITY_DTB_A,          407 * 1024,      2 * 1024,      UFS_PART_2}, /* security_dtb_a       2M    p37 */
    {PART_FW_DTB_A,                409 * 1024,      8 * 1024,      UFS_PART_2}, /* fw_dtb_a             8M    p38 */
    {PART_RDA_DTB_A,               417 * 1024,      8 * 1024,      UFS_PART_2}, /* rda_dtb_a            8M    p39 */
    {PART_MICROVISOR_A,            425 * 1024,     50 * 1024,      UFS_PART_2}, /* microvisor_a        50M    p40 */
    {PART_RDA_RAMDISK_A,           475 * 1024,    200 * 1024,      UFS_PART_2}, /* rda_ramdisk_a      200M    p41 */
    {PART_TRUSTFIRMWARE_A,         675 * 1024,      2 * 1024,      UFS_PART_2}, /* trustfirmware_a      2M    p42 */
    {PART_RECOVERY_VBMETA_A,       677 * 1024,      2 * 1024,      UFS_PART_2}, /* recovery_vbmeta_a    2M    p43 */
    {PART_ERECOVERY_VBMETA_A,      679 * 1024,      2 * 1024,      UFS_PART_2}, /* erecovery_vbmeta_a   2M    p44 */
    {PART_VBMETA_A,                681 * 1024,      4 * 1024,      UFS_PART_2}, /* PART_VBMETA_a        4M    p45 */
    {PART_PATCH_A,                 685 * 1024,     32 * 1024,      UFS_PART_2}, /* patch_a             32M    p46 */
    {PART_PREAS_A,                 717 * 1024,   1280 * 1024,      UFS_PART_2}, /* preas_a           1280M    p47 */
    {PART_PREAVS_A,               1997 * 1024,     32 * 1024,      UFS_PART_2}, /* preavs_a            32M    p48 */
    {PART_VERSION_A,              2029 * 1024,    576 * 1024,      UFS_PART_2}, /* version_a          576M    p49 */
    {PART_PRELOAD_A,              2605 * 1024,   1144 * 1024,      UFS_PART_2}, /* preload_a         1144M    p50 */
    {PART_MONITOR_ISLAND_A,       3749 * 1024,      4 * 1024,      UFS_PART_2}, /* monitor_island_a     4M    p51 */
    {PART_PTABLE_LU3,                       0,           512,      UFS_PART_3}, /* ptable             512K    p0 */
    {PART_VRL_B,                          512,           512,      UFS_PART_3}, /* vrl_b              512K    p1 */
    {PART_VRL_BACKUP_B,               2 * 512,           512,      UFS_PART_3}, /* vrl backup_b       512K    p2 */
    {PART_RESERVED2,                  3 * 512,           512,      UFS_PART_3}, /* reserved2          512K    p3 */
    {PART_NPU_B,                     2 * 1024,      8 * 1024,      UFS_PART_3}, /* npu_b                8M    p4 */
    {PART_HIEPS_B,                  10 * 1024,      2 * 1024,      UFS_PART_3}, /* hieps_b              2M    p5 */
    {PART_IVP_B,                    12 * 1024,      2 * 1024,      UFS_PART_3}, /* ivp_b                2M    p6 */
    {PART_HDCP_B,                   14 * 1024,      1 * 1024,      UFS_PART_3}, /* PART_HDCP_b          1M    p7 */
    {PART_HISEE_IMG_B,              15 * 1024,      4 * 1024,      UFS_PART_3}, /* part_hisee_img_b     4M    p8 */
    {PART_HHEE_B,                   19 * 1024,      4 * 1024,      UFS_PART_3}, /* hhee_b               4M    p9 */
    {PART_FASTBOOT_B,               23 * 1024,     12 * 1024,      UFS_PART_3}, /* fastboot_b          12M    p10 */
    {PART_LOWPOWER_PARA_B,          35 * 1024,      1 * 1024,      UFS_PART_3}, /* lowpower_para_b      1M    p11 */
    {PART_BL2_B,                    36 * 1024,      4 * 1024,      UFS_PART_3}, /* bl2_b                4M    p12 */
    {PART_TEEOS_B,                  40 * 1024,      8 * 1024,      UFS_PART_3}, /* teeos_b              8M    p13 */
    {PART_FW_LPM3_B,                48 * 1024,      1 * 1024,      UFS_PART_3}, /* fw_lpm3_b            1M    p14 */
    {PART_HISEE_FS_B,               49 * 1024,      8 * 1024,      UFS_PART_3}, /* hisee_fs_b           8M    p15 */
    {PART_VECTOR_B,                 57 * 1024,      4 * 1024,      UFS_PART_3}, /* vector_b             4M    p16 */
    {PART_ISP_BOOT_B,               61 * 1024,      2 * 1024,      UFS_PART_3}, /* isp_boot_b           2M    p17 */
    {PART_ISP_FIRMWARE_B,           63 * 1024,     14 * 1024,      UFS_PART_3}, /* isp_firmware_b      14M    p18 */
    {PART_FW_HIFI_B,                77 * 1024,     12 * 1024,      UFS_PART_3}, /* hifi_b              12M    p19 */
#ifdef CONFIG_SANITIZER_ENABLE
    {PART_ERECOVERY_RAMDISK_B,      89 * 1024,     12 * 1024,      UFS_PART_3}, /* erecovery_ramdisk_b 12M    p20 */
    {PART_ERECOVERY_VENDOR_B,      101 * 1024,      8 * 1024,      UFS_PART_3}, /* erecovery_vendor_b   8M    p21 */
    {PART_BOOT_B,                  109 * 1024,     65 * 1024,      UFS_PART_3}, /* boot_b              65M    p22 */
    {PART_RECOVERY_B,              174 * 1024,     85 * 1024,      UFS_PART_3}, /* recovery_b          85M    p23 */
    {PART_ERECOVERY_B,             259 * 1024,     12 * 1024,      UFS_PART_3}, /* erecovery_b         12M    p24 */
    {PART_KPATCH_B,                271 * 1024,     29 * 1024,      UFS_PART_3}, /* kpatch_b            29M    p25 */
#elif defined CONFIG_FACTORY_MODE
    {PART_ERECOVERY_RAMDISK_B,      89 * 1024,     32 * 1024,      UFS_PART_3}, /* erecovery_ramdisk_b 32M    p20 */
    {PART_ERECOVERY_VENDOR_B,      121 * 1024,     24 * 1024,      UFS_PART_3}, /* erecovery_vendor_b  24M    p21 */
    {PART_BOOT_B,                  145 * 1024,     30 * 1024,      UFS_PART_3}, /* boot_b              30M    p22 */
    {PART_RECOVERY_B,              175 * 1024,     41 * 1024,      UFS_PART_3}, /* recovery_b          41M    p23 */
    {PART_ERECOVERY_B,             216 * 1024,     41 * 1024,      UFS_PART_3}, /* erecovery_b         41M    p24 */
    {PART_KPATCH_B,                257 * 1024,     43 * 1024,      UFS_PART_3}, /* kpatch_b            43M    p25 */
#else
    {PART_ERECOVERY_RAMDISK_B,      89 * 1024,     32 * 1024,      UFS_PART_3}, /* erecovery_ramdisk_b 32M    p20 */
    {PART_ERECOVERY_VENDOR_B,      121 * 1024,     16 * 1024,      UFS_PART_3}, /* erecovery_vendor_b  16M    p21 */
    {PART_BOOT_B,                  137 * 1024,     30 * 1024,      UFS_PART_3}, /* boot_b              30M    p22 */
    {PART_RECOVERY_B,              167 * 1024,     45 * 1024,      UFS_PART_3}, /* recovery_b          45M    p23 */
    {PART_ERECOVERY_B,             212 * 1024,     45 * 1024,      UFS_PART_3}, /* erecovery_b         45M    p24 */
    {PART_KPATCH_B,                257 * 1024,     43 * 1024,      UFS_PART_3}, /* kpatch_b            43M    p25 */
#endif
    {PART_ENG_SYSTEM_B,            300 * 1024,     12 * 1024,      UFS_PART_3}, /* eng_system_b        12M    p26 */
    {PART_RAMDISK_B,               312 * 1024,      2 * 1024,      UFS_PART_3}, /* ramdisk_b            2M    p27 */
    {PART_VBMETA_SYSTEM_B,         314 * 1024,      1 * 1024,      UFS_PART_3}, /* vbmeta_system_b      1M    p28 */
    {PART_VBMETA_VENDOR_B,         315 * 1024,      1 * 1024,      UFS_PART_3}, /* vbmeta_vendor_b      1M    p29 */
    {PART_VBMETA_ODM_B,            316 * 1024,      1 * 1024,      UFS_PART_3}, /* vbmeta_odm_b         1M    p30 */
    {PART_VBMETA_CUST_B,           317 * 1024,      1 * 1024,      UFS_PART_3}, /* vbmeta_cus_b         1M    p31 */
    {PART_VBMETA_HW_PRODUCT_B,     318 * 1024,      1 * 1024,      UFS_PART_3}, /* vbmeta_hw_product_b  1M    p32 */
    {PART_RECOVERY_RAMDISK_B,      319 * 1024,     32 * 1024,      UFS_PART_3}, /* recovery_ramdisk_b  32M    p33 */
#ifdef CONFIG_FACTORY_MODE
    {PART_RECOVERY_VENDOR_B,       351 * 1024,     24 * 1024,      UFS_PART_3}, /* recovery_vendor_b   24M    p34 */
    {PART_DTBO_B,                  375 * 1024,     16 * 1024,      UFS_PART_3}, /* dtoimage_b          16M    p35 */
    {PART_ENG_VENDOR_B,            391 * 1024,     16 * 1024,      UFS_PART_3}, /* eng_vendor_b        16M    p36 */
#else
    {PART_RECOVERY_VENDOR_B,       351 * 1024,     16 * 1024,      UFS_PART_3}, /* recovery_vendor_b   16M    p34 */
    {PART_DTBO_B,                  367 * 1024,     20 * 1024,      UFS_PART_3}, /* dtoimage_b          20M    p35 */
    {PART_ENG_VENDOR_B,            387 * 1024,     20 * 1024,      UFS_PART_3}, /* eng_vendor_b        20M    p36 */
#endif
    {PART_SECURITY_DTB_B,          407 * 1024,      2 * 1024,      UFS_PART_3}, /* security_dtb_b       2M    p37 */
    {PART_FW_DTB_B,                409 * 1024,      8 * 1024,      UFS_PART_3}, /* fw_dtb_b             8M    p38 */
    {PART_RDA_DTB_B,               417 * 1024,      8 * 1024,      UFS_PART_3}, /* rda_dtb_b            8M    p39 */
    {PART_MICROVISOR_B,            425 * 1024,     50 * 1024,      UFS_PART_3}, /* microvisor_b        50M    p40 */
    {PART_RDA_RAMDISK_B,           475 * 1024,    200 * 1024,      UFS_PART_3}, /* rda_ramdisk_b      200M    p41 */
    {PART_TRUSTFIRMWARE_B,         675 * 1024,      2 * 1024,      UFS_PART_3}, /* trustfirmware_b      2M    p42 */
    {PART_RECOVERY_VBMETA_B,       677 * 1024,      2 * 1024,      UFS_PART_3}, /* recovery_vbmeta_b    2M    p43 */
    {PART_ERECOVERY_VBMETA_B,      679 * 1024,      2 * 1024,      UFS_PART_3}, /* erecovery_vbmeta_b   2M    p44 */
    {PART_VBMETA_B,                681 * 1024,      4 * 1024,      UFS_PART_3}, /* PART_VBMETA_b        4M    p45 */
    {PART_PATCH_B,                 685 * 1024,     32 * 1024,      UFS_PART_3}, /* patch_b             32M    p46 */
    {PART_PREAS_B,                 717 * 1024,   1280 * 1024,      UFS_PART_3}, /* preas_b           1280M    p47 */
    {PART_PREAVS_B,               1997 * 1024,     32 * 1024,      UFS_PART_3}, /* preavs_b            32M    p48 */
    {PART_VERSION_B,              2029 * 1024,    576 * 1024,      UFS_PART_3}, /* version_b          576M    p49 */
    {PART_PRELOAD_B,              2605 * 1024,   1144 * 1024,      UFS_PART_3}, /* preload_b         1144M    p50 */
    {PART_MONITOR_ISLAND_B,       3749 * 1024,      4 * 1024,      UFS_PART_3}, /* monitor_island_b     4M    p51 */
    {PART_PTABLE_LU4,                       0,           512,      UFS_PART_4}, /* ptable_lu4        512K     p0 */
    {PART_FRP,                            512,           512,      UFS_PART_4}, /* frp                512K    p1 */
    {PART_PERSIST,                   1 * 1024,      6 * 1024,      UFS_PART_4}, /* persist           6144K    p2 */
    {PART_BOOT_CTRL,                 7 * 1024,      1 * 1024,      UFS_PART_4}, /* boot ctrl          512K    p3 */
    {PART_NVME,                      8 * 1024,      5 * 1024,      UFS_PART_4}, /* nvme                 5M    p4 */
    {PART_CTF,                      13 * 1024,      1 * 1024,      UFS_PART_4}, /* PART_CTF             1M    p5 */
    {PART_OEMINFO,                  14 * 1024,     96 * 1024,      UFS_PART_4}, /* oeminfo             96M    p6 */
    {PART_SECURE_STORAGE,          110 * 1024,     32 * 1024,      UFS_PART_4}, /* secure storage      32M    p7 */
    {PART_HISEE_ENCOS,             142 * 1024,      4 * 1024,      UFS_PART_4}, /* hisee_encos          4M    p8 */
    {PART_VERITYKEY,               146 * 1024,      1 * 1024,      UFS_PART_4}, /* veritykey            1M    p9 */
    {PART_DDR_PARA,                147 * 1024,      1 * 1024,      UFS_PART_4}, /* DDR_PARA             1M    p10 */
    {PART_BATT_TP_PARA,            148 * 1024,      1 * 1024,      UFS_PART_4}, /* batt_tp_para         1M    p11 */
    {PART_RESERVED3,               149 * 1024,      1 * 1024,      UFS_PART_4}, /* reserved3            1M    p12 */
    {PART_SPLASH2,                 150 * 1024,     80 * 1024,      UFS_PART_4}, /* splash2             80M    p13 */
    {PART_BOOTFAIL_INFO,           230 * 1024,      2 * 1024,      UFS_PART_4}, /* bootfail info        2M    p14 */
    {PART_MISC,                    232 * 1024,      2 * 1024,      UFS_PART_4}, /* misc                 2M    p15 */
    {PART_DFX,                     234 * 1024,     16 * 1024,      UFS_PART_4}, /* dfx                 16M    p16 */
    {PART_RRECORD,                 250 * 1024,     16 * 1024,      UFS_PART_4}, /* rrecord             16M    p17 */
    {PART_CACHE,                   266 * 1024,    104 * 1024,      UFS_PART_4}, /* cache              104M    p18 */
    {PART_METADATA,                370 * 1024,     16 * 1024,      UFS_PART_4}, /* metadata            16M    p19 */
    {PART_SUPER,                   386 * 1024,  18766 * 1024,      UFS_PART_4}, /* super     2M+9382+9382M    p20 */
#ifdef CONFIG_FACTORY_MODE
    {PART_HIBENCH_IMG,           19152 * 1024,    128 * 1024,      UFS_PART_4}, /* hibench_img        128M    p21 */
    {PART_HIBENCH_DATA,          19280 * 1024,    512 * 1024,      UFS_PART_4}, /* hibench_data       512M    p22 */
    {PART_FLASH_AGEING,          19792 * 1024,    512 * 1024,      UFS_PART_4}, /* FLASH_AGEING       512M    p23 */
    {PART_HIBENCH_LOG,           20304 * 1024,     32 * 1024,      UFS_PART_4}, /* HIBENCH_LOG         32M    p24 */
    {PART_HIBENCH_LPM3,          20336 * 1024,     32 * 1024,      UFS_PART_4}, /* HIBENCH_LPM3        32M    p25 */
    {PART_SECFLASH_AGEING,       20368 * 1024,     32 * 1024,      UFS_PART_4}, /* secflash_ageing     32M    p26 */
    {PART_USERDATA,              20400 * 1024,  (4UL) * 1024 * 1024,    UFS_PART_4}, /* userdata       4G     p27 */
#else
    {PART_USERDATA,              19152 * 1024,  (4UL) * 1024 * 1024,    UFS_PART_4}, /* userdata       4G     p21 */
#endif
    {"0", 0, 0, 0},
};

#endif
