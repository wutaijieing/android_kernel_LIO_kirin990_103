/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __PTABLE_COM_H__
#define __PTABLE_COM_H__

/*
 * Attention
 * Description : Driver for secboot
 * Core        : Acore��Mcore��Fastboot
 * Header File : the following head files need to be modified at the same time
 *             : /acore/kernel/drivers/hisi/modem/drv/include/ptable_com.h
 *             : /mcore/xloader/include/ptable_com.h
 *             : /fastboot/include/ptable_com.h
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bsp_shared_ddr.h>

#ifndef __ASSEMBLY__
#include "bsp_sram.h"

/* ptable */
#define FSZ long unsigned int
#define NAND_OK 0
#define NAND_ERROR 1

/* �������� */
typedef enum {
    /* һ����Ʒ�߾�̬�ļ����ط�ʽ���漰��Ʒ���޸ģ��ݱ�����̬���ط�ʽ */
    IMAGE_SFS_START = 0x0, /* ��̬�ļ� ��ʼ */
    IMAGE_CDROMISOVER = 0x1,
    IMAGE_WEBUIVER = 0x2,
    IMAGE_CFDNV_TABLE = 0x3,
    ZSP_FILE_ADDR = 0x4,  /* [����ֵ],ƹ��Bootrom���룬������BOOTROM_BACKö�٣�
                             Ϊ����֮ǰ�Ĵ�����߶����ZSP�ļ�ͷ��ַ */
    ISO_FILE_ADDR = 0x5,  /* [����ֵ],ƹ��Bootrom���룬������BOOTROM_BACKö�٣�
                             Ϊ����֮ǰ�Ĵ�����߶����ZSP�ļ�ͷ��ַ */
    IMAGE_SFS_TOP = 0x50, /* ��̬�ļ����� */

    /* ������̬�ļ�����ӳ�� */
    IMAGE_DFS_START = 0x64,
    /* 100 */ /* ��̬�ļ���ʼ */

    IMAGE_DYN_WEBUI = 0x65,
    /* 101 */ /* ��Ʒ�߶�̬�ļ�����ӳ�� */
    IMAGE_DYN_FS = 0x66,
    /* 102 */ /* ����̬�ļ�����ӳ�� */

    IMAGE_DFS_TOP = 0xfe, /* ��̬�ļ����� */

    /* �� ��flash����ӳ������ , �����������������Ҫ����������ӳ���־ */
    IMAGE_PART_START = 0xFF, /* ��̬�ļ� ��ʼ */

    IMAGE_PTABLE = 0x100,        /* ������ �����־ */
    IMAGE_M3BOOT = 0x101,        /* M3 BOOT */
    IMAGE_FASTBOOT = 0x102,      /* FASTBOOT���� �� */
    IMAGE_NVBACKLTE = 0x103,     /* NV ���� ������ */
    IMAGE_NVIMG = 0x104,         /* ����NV �� */
    IMAGE_NVDLD = 0x105,         /* ����NV �� */
    IMAGE_NVFACTORY = 0x106,     /* ����NV �� */
    IMAGE_KERNEL = 0x107,        /* Kernel ���� */
    IMAGE_KERNELBK = 0x108,      /* Kernel ���ݷ��� */
    IMAGE_M3IMAGE = 0x109,       /* M3 FIRMWARE ���� */
    IMAGE_LOGO = 0x10C,          /* LOGO ���� */
    IMAGE_OEMINFO = 0x10D,       /* OEMONFO ���� �� */
    IMAGE_SYSTEM = 0x10E,        /* SYSTEM.IMG ���� �� */
    IMAGE_DTS = 0x10F,           /* SYSTEM.IMG ���� �� */
    IMAGE_OM = 0x110,            /* ��ſ�ά�ɲ����� */
    IMAGE_APP = 0x111,           /* APP.IMG ���� �� */
    IMAGE_WEBUI = 0x112,         /* WEBUI ���� �� */
    IMAGE_RESERVE1 = 0x113,      /* Ԥ��1 ���� �� */
    IMAGE_RESERVE2 = 0x114,      /* Ԥ��2 ���� �� */
    IMAGE_RESERVE3 = 0x115,      /* Ԥ��3 ���� �� */
    IMAGE_USERDATA = 0x116,      /* userdata ���� �� */
    IMAGE_ONLINE = 0x117,        /* ONLINE �������� ���� �� */
    IMAGE_CDROMISO = 0x118,      /* CDROMISO ��̨ ������ */
    IMAGE_CACHE = 0x119,         /* CACHE ���� �� */
    IMAGE_RECOVERY_A = 0x11A,    /* RECOVERY���� ��  */
    IMAGE_RECOVERY_B = 0x11B,    /* RECOVERY���� ��  */
    IMAGE_MISC = 0x11C,          /* MISC ���� �� */
    IMAGE_MULTI_CARRIER = 0x11D, /* multicarrier ���� �� */
    IMAGE_HIFI = 0x11e,          /* HIFI ���� �� */
    IMAGE_TEEOS = 0x120,         /* TEEOS ���� �� */
    IMAGE_MODEM_FW = 0x121,      /* modem_fw ���� �� */
    IMAGE_SEC_STORAGE = 0x122,   /* ��ȫ�洢���� */

    IMAGE_PART_TOP = 0x200 /* ���������� ���� */

} image_type_e;

#define PTABLE_IMAGE_TYPE_MASK (0x7)

typedef enum {
    DATA_NORMAL = 0, /* ��ʾ�˷���ӳ��Ϊ��ͨ����������,��ռ��sp */
    DATA_YAFFS = 1   /* ��ʾ�˷���ӳ��Ϊyaffsӳ��,ռ��spare�� */
} flash_data_type_e;

#define PTABLE_VALIDITY_OFFSET (0x3)
#define PTABLE_VALIDITY_MASK (0x1)

typedef enum {
    DATA_VALID = 0,  /* ��ʾ�˷���ӳ������������Ч,����ʹ��(Ĭ��) */
    DATA_INVALID = 1 /* ��ʾ�˷���ӳ�����ݿ��ܲ�����,������ʹ�� */
} flash_data_validity_e;

/*
 *                          |  0 byte
 *  |"pTableHead"            |
 *  *----------------------- | 12 byte (partition head flag string)
 *  | the property of table  |
 *  *----------------------- | 16 byte (partition table property)
 *  | bootrom version        |
 *  *----------------------- | 32 byte (partition bootrom ver string)
 *  | ptable version         |
 *  *----------------------- | 48 byte (partition ptable ver string)
 * ptabe head define 48 bytess
 */
struct st_ptable_head {
    char head_str[12];
    unsigned int property;
    char product_ver_id[16];
    char ver_name[16];
};

#define PTABLE_HEAD_STR "pTableHead"

struct st_part_tbl {
    char name[16];         /* name of this partition, uniquely */
    unsigned int offset;   /* offset in flash of this partition */
    unsigned int loadsize; /* real data size of this partition, must smaller than "capacity" */
    unsigned int capacity; /* total size of this partition */
    unsigned int loadaddr; /* ram address of this this partition */
    unsigned int entry;    /* if the partition is executable,this is entry address of this partition */
    unsigned int image;    /* enum IMAGE_TYPE */
    unsigned int property; /* property of this partition, eg. exe, yaffs, fat, ram image ... */
    unsigned int count;    /* count for partition download or write */
};

#define PTABLE_HEAD_SIZE sizeof(struct st_ptable_head)

/* ptabe TAIL define */
#define PTABLE_END_STR "T" /* "pTableTail" */

/* should move table head space */
#define PTABLE_PARTITION_MAX ((SHM_SIZE_PTABLE - PTABLE_HEAD_SIZE) / sizeof(struct st_part_tbl))

/* β��������ݣ�ʹ�ñ�������ľ�������ռ��DDRPTABLE_RAM_TABLE_SIZE��ʣ��ռ� */
#define PTABLE_TAIL_SIZE (SHM_SIZE_PTABLE - (PTABLE_PARTITION_MAX) * sizeof(struct st_part_tbl) - PTABLE_HEAD_SIZE)
#define PTABLE_TAIL_STR "pTableTail"

extern struct st_ptable_head ptable_head;
extern struct st_part_tbl ptable_product[];
extern unsigned int ptable_get_validity(struct st_part_tbl *ptable);
extern struct st_part_tbl *ptable_get_ram_data(void);

#define PART_TABLE_NAND_OFFSET (FLASH_PTABLE_OFFSET) /* ��������flash�洢�ռ��ƫ��ֵ */

#endif /* __ASSEMBLY__ */

#include "product_config.h"
#define SHM_MEM_PTABLE_ADDR (SHM_BASE_ADDR + SHM_OFFSET_PTABLE)
#define SHM_MEM_PTABLE_SIZE (SHM_SIZE_PTABLE)
/* entry define */
#define PTABLE_FASTBOOT_ENTRY (FASTBOOT_DDR_ENTRY)
#define PTABLE_MODEM_CCORE_ENTRY (MCORE_TEXT_START_ADDR)

#define PTABLE_VER_STR PRODUCT_CFG_PTABLE_VER_NAME
#define PTABLE_VER_ID_STR PRODUCT_CFG_PTABLE_VER_ID

/*
 * FLASH�ռ���� EMMC BOOT
 */
#ifdef CONFIG_EMMC_BOOT

#define PTABLE_M3_BOOT_START (0) /* M3boot��ptable������ʼλ�ü���Сλ�ò��ܵ���!!!!!!! */
#define PTABLE_M3_BOOT_LEN (PRODUCT_CFG_FLASH_M3BOOT_LEN)
#define PTABLE_M3_BOOT_END (PTABLE_M3_BOOT_START + PTABLE_M3_BOOT_LEN)

#define PTABLE_GPT_START (0) /* M3boot��ptable������ʼλ�ü���Сλ�ò��ܵ���!!!!!!! */
#define PTABLE_GPT_LEN (0x20000)
#define PTABLE_GPT_END (PTABLE_GPT_START + PTABLE_GPT_LEN)

/* Fastboot */
#define PTABLE_FASTBOOT_START PTABLE_GPT_END
#define PTABLE_FASTBOOT_LEN (PRODUCT_CFG_FLASH_FASTBOOT_LEN)
#define PTABLE_FASTBOOT_END (PTABLE_FASTBOOT_START + PTABLE_FASTBOOT_LEN)

/* NvDefault */
#define PTABLE_NVFACTORY_START PTABLE_FASTBOOT_END
#define PTABLE_NVFACTORY_LEN (PRODUCT_CFG_FLASH_NV_FACTORY_LEN)
#define PTABLE_NVFACTORY_END (PTABLE_NVFACTORY_START + PTABLE_NVFACTORY_LEN)

/* NV LTE */
#define PTABLE_NV_LTE_START PTABLE_NVFACTORY_END
#define PTABLE_NV_LTE_LEN (PRODUCT_CFG_FLASH_NV_LTE_LEN)
#define PTABLE_NV_LTE_END (PTABLE_NV_LTE_START + PTABLE_NV_LTE_LEN)

/* NvImg */
#define PTABLE_NVIMG_START PTABLE_NV_LTE_END
#define PTABLE_NVIMG_LEN (PRODUCT_CFG_FLASH_NV_IMG_LEN)
#define PTABLE_NVIMG_END (PTABLE_NVIMG_START + PTABLE_NVIMG_LEN)

/* NvDload */
#define PTABLE_NVDLD_START PTABLE_NVIMG_END
#define PTABLE_NVDLD_LEN (PRODUCT_CFG_FLASH_NV_DLD_LEN)
#define PTABLE_NVDLD_END (PTABLE_NVDLD_START + PTABLE_NVDLD_LEN)

/* NvCust */
#define PTABLE_NVCUST_START PTABLE_NVDLD_END
#define PTABLE_NVCUST_LEN (PRODUCT_CFG_FLASH_NV_CUST_LEN)
#define PTABLE_NVCUST_END (PTABLE_NVCUST_START + PTABLE_NVCUST_LEN)

/* Uce */
#define PTABLE_UCE_START PTABLE_NVCUST_END
#define PTABLE_UCE_LEN (PRODUCT_CFG_FLASH_UCE_LEN)
#define PTABLE_UCE_END (PTABLE_UCE_START + PTABLE_UCE_LEN)

/* SecStorage */
#define PTABLE_SEC_STORAGE_START PTABLE_UCE_END
#define PTABLE_SEC_STORAGE_LEN (PRODUCT_CFG_FLASH_SEC_STORAGE_LEN)
#define PTABLE_SEC_STORAGE_END (PTABLE_SEC_STORAGE_START + PTABLE_SEC_STORAGE_LEN)

/* m3image */
#define PTABLE_M3IMAGE_START PTABLE_SEC_STORAGE_END
#define PTABLE_M3IMAGE_LEN (PRODUCT_CFG_FLASH_M3IMAGE_LEN)
#define PTABLE_M3IMAGE_END (PTABLE_M3IMAGE_START + PTABLE_M3IMAGE_LEN)

/* Recovery-A */
#define PTABLE_RECOVERY_A_START PTABLE_M3IMAGE_END
#define PTABLE_RECOVERY_A_LEN (PRODUCT_CFG_FLASH_RECOVERY_LEN)
#define PTABLE_RECOVERY_A_END (PTABLE_RECOVERY_A_START + PTABLE_RECOVERY_A_LEN)

/* Recovery-B */
#define PTABLE_RECOVERY_B_START PTABLE_RECOVERY_A_END
#define PTABLE_RECOVERY_B_LEN (PRODUCT_CFG_FLASH_RECOVERY_LEN)
#define PTABLE_RECOVERY_B_END (PTABLE_RECOVERY_B_START + PTABLE_RECOVERY_B_LEN)

/* misc */
#define PTABLE_MISC_START PTABLE_RECOVERY_B_END
#define PTABLE_MISC_LEN (PRODUCT_CFG_FLASH_MISC_LEN)
#define PTABLE_MISC_END (PTABLE_MISC_START + PTABLE_MISC_LEN)

/* dts */
#define PTABLE_DTS_START PTABLE_MISC_END
#define PTABLE_DTS_LEN (PRODUCT_CFG_FLASH_DTIMG_LEN)
#define PTABLE_DTS_END (PTABLE_DTS_START + PTABLE_DTS_LEN)

/* teeos */
#define PTABLE_TEEOS_START PTABLE_DTS_END
#define PTABLE_TEEOS_LEN (PRODUCT_CFG_FLASH_TEEOS_LEN)
#define PTABLE_TEEOS_END (PTABLE_TEEOS_START + PTABLE_TEEOS_LEN)

/* trsted_fw  */
#define PTABLE_TRUSTED_FW_START (PTABLE_TEEOS_END)
#define PTABLE_TRUSTED_FW_LEN (PRODUCT_CFG_FLASH_TRUSTED_FW_LEN)
#define PTABLE_TRUSTED_FW_END (PTABLE_TRUSTED_FW_START + PTABLE_TRUSTED_FW_LEN)

/* kernel */
#define PTABLE_BOOTIMAGE_START PTABLE_TRUSTED_FW_END
#define PTABLE_BOOTIMAGE_LEN (PRODUCT_CFG_FLASH_BOOTIMG_LEN)
#define PTABLE_BOOTIMAGE_END (PTABLE_BOOTIMAGE_START + PTABLE_BOOTIMAGE_LEN)

/* kernelbak */
#define PTABLE_BOOTIMAGEBAK_START PTABLE_BOOTIMAGE_END
#define PTABLE_BOOTIMAGEBAK_LEN (PRODUCT_CFG_FLASH_BOOTIMGBAK_LEN)
#define PTABLE_BOOTIMAGEBAK_END (PTABLE_BOOTIMAGEBAK_START + PRODUCT_CFG_FLASH_BOOTIMGBAK_LEN)

/* modem_fw  */
#define PTABLE_MODEM_FW_START (PTABLE_BOOTIMAGEBAK_END)
#define PTABLE_MODEM_FW_LEN (PRODUCT_CFG_FLASH_MODEM_FW_LEN)
#define PTABLE_MODEM_FW_END (PTABLE_MODEM_FW_START + PTABLE_MODEM_FW_LEN)

/* FS ONLINE */
#define PTABLE_FS_ONLINE_START PTABLE_MODEM_FW_END
#define PTABLE_FS_ONLINE_LEN (PRODUCT_CFG_FLASH_YAFFS_ONLINE_LEN)
#define PTABLE_FS_ONLINE_END (PTABLE_FS_ONLINE_START + PTABLE_FS_ONLINE_LEN)

/* Hifi */
#define PTABLE_HIFI_START PTABLE_FS_ONLINE_END
#define PTABLE_HIFI_LEN (PRODUCT_CFG_FLASH_HIFI_LEN)
#define PTABLE_HIFI_END (PTABLE_HIFI_START + PTABLE_HIFI_LEN)

/* OEM info */
#define PTABLE_OEMINFO_START PTABLE_HIFI_END
#define PTABLE_OEMINFO_LEN (PRODUCT_CFG_FLASH_OEMINFO_LEN)
#define PTABLE_OEMINFO_END (PTABLE_OEMINFO_START + PTABLE_OEMINFO_LEN)

/* Reserve1 */
#define PTABLE_RESERVE1_START PTABLE_OEMINFO_END
#define PTABLE_RESERVE1_LEN (PRODUCT_CFG_FLASH_RESERVE1_LEN)
#define PTABLE_RESERVE1_END (PTABLE_RESERVE1_START + PTABLE_RESERVE1_LEN)

/* Reserve2 */
#define PTABLE_RESERVE2_START PTABLE_RESERVE1_END
#define PTABLE_RESERVE2_LEN (PRODUCT_CFG_FLASH_RESERVE2_LEN)
#define PTABLE_RESERVE2_END (PTABLE_RESERVE2_START + PRODUCT_CFG_FLASH_RESERVE2_LEN)

/* Reserve3 */
#define PTABLE_RESERVE3_START PTABLE_RESERVE2_END
#define PTABLE_RESERVE3_LEN (PRODUCT_CFG_FLASH_RESERVE3_LEN)
#define PTABLE_RESERVE3_END (PTABLE_RESERVE3_START + PRODUCT_CFG_FLASH_RESERVE3_LEN)

/* FS APP */
#define PTABLE_FS_APP_START PTABLE_RESERVE3_END
#define PTABLE_FS_APP_LEN (PRODUCT_CFG_FLASH_YAFFS_APP_LEN)
#define PTABLE_FS_APP_END (PTABLE_FS_APP_START + PTABLE_FS_APP_LEN)

/* FS WEBUI */
#define PTABLE_FS_WEBUI_START PTABLE_FS_APP_END
#define PTABLE_FS_WEBUI_LEN (PRODUCT_CFG_FLASH_YAFFS_WEBUI_LEN)
#define PTABLE_FS_WEBUI_END (PTABLE_FS_WEBUI_START + PTABLE_FS_WEBUI_LEN)

/* system */
#define PTABLE_FS_SYSIMG_START PTABLE_FS_WEBUI_END
#define PTABLE_FS_SYSIMG_LEN (PRODUCT_CFG_FLASH_SYSIMG_LEN)
#define PTABLE_FS_SYSIMG_END (PTABLE_FS_SYSIMG_START + PTABLE_FS_SYSIMG_LEN)

/* USER DATA */
#define PTABLE_FS_USERDATA_START PTABLE_FS_SYSIMG_END
#define PTABLE_FS_USERDATA_LEN (PRODUCT_CFG_FLASH_YAFFS_USERDATA_LEN)
#define PTABLE_FS_USERDATA_END (PTABLE_FS_USERDATA_START + PTABLE_FS_USERDATA_LEN)

/* logo */
#define PTABLE_LOGO_START PTABLE_FS_USERDATA_END
#define PTABLE_LOGO_LEN (PRODUCT_CFG_FLASH_LOGO_LEN)
#define PTABLE_LOGO_END (PTABLE_LOGO_START + PRODUCT_CFG_FLASH_LOGO_LEN)

/* cache */
#define PTABLE_CACHE_START PTABLE_LOGO_END
#define PTABLE_CACHE_LEN (PRODUCT_CFG_FLASH_CACHE_LEN)
#define PTABLE_CACHE_END (PTABLE_CACHE_START + PTABLE_CACHE_LEN)

/* hac */
#define PTABLE_HACLD_START (PTABLE_CACHE_END)
#define PTABLE_HACLD_LEN (PRODUCT_CFG_FLASH_HAC_IMG_LEN)
#define PTABLE_HACLD_END (PTABLE_HACLD_START + PTABLE_HACLD_LEN)

/* ISO  */
#define PTABLE_ISO_START (PTABLE_HACLD_END)
#define PTABLE_ISO_LEN (PRODUCT_CFG_FLASH_ISO_LEN)
#define PTABLE_ISO_END (PTABLE_ISO_START + PTABLE_ISO_LEN)

/*
 * FLASH�ռ���� UBIFS BOOT
 */
#elif defined CONFIG_UBIFS_BOOT

#define PTABLE_GPT_START (0) /* M3boot��ptable������ʼλ�ü���Сλ�ò��ܵ���!!!!!!! */
#define PTABLE_GPT_LEN (0)
#define PTABLE_GPT_END (PTABLE_GPT_START + PTABLE_GPT_LEN)

/* M3boot */
#define PTABLE_M3_BOOT_START (0) /* M3boot��ptable������ʼλ�ü���Сλ�ò��ܵ���!!!!!!! */
#define PTABLE_M3_BOOT_LEN (PRODUCT_CFG_FLASH_M3BOOT_LEN)
#define PTABLE_M3_BOOT_END (PTABLE_M3_BOOT_START + PTABLE_M3_BOOT_LEN)

/* Fastboot */
#define PTABLE_FASTBOOT_START PTABLE_M3_BOOT_END
#define PTABLE_FASTBOOT_LEN (PRODUCT_CFG_FLASH_FASTBOOT_LEN)
#define PTABLE_FASTBOOT_END (PTABLE_FASTBOOT_START + PTABLE_FASTBOOT_LEN)

/* NvDefault */
#define PTABLE_NVFACTORY_START PTABLE_FASTBOOT_END
#define PTABLE_NVFACTORY_LEN (PRODUCT_CFG_FLASH_NV_FACTORY_LEN)
#define PTABLE_NVFACTORY_END (PTABLE_NVFACTORY_START + PTABLE_NVFACTORY_LEN)

/* NV LTE */
#define PTABLE_NV_LTE_START PTABLE_NVFACTORY_END
#define PTABLE_NV_LTE_LEN (PRODUCT_CFG_FLASH_NV_LTE_LEN)
#define PTABLE_NV_LTE_END (PTABLE_NV_LTE_START + PTABLE_NV_LTE_LEN)

/* NvDload */
#define PTABLE_NVDLD_START PTABLE_NV_LTE_END
#define PTABLE_NVDLD_LEN (PRODUCT_CFG_FLASH_NV_DLD_LEN)
#define PTABLE_NVDLD_END (PTABLE_NVDLD_START + PTABLE_NVDLD_LEN)

/* NvCust */
#define PTABLE_NVCUST_START PTABLE_NVDLD_END
#define PTABLE_NVCUST_LEN (PRODUCT_CFG_FLASH_NV_CUST_LEN)
#define PTABLE_NVCUST_END (PTABLE_NVCUST_START + PTABLE_NVCUST_LEN)

/* nandmonitor */
#define PTABLE_NANDMONITOR_START PTABLE_NVCUST_END
#ifdef CONFIG_FMC_MNTN_ENABLE
#define PTABLE_NANDMONITOR_LEN 0x100000
#else
#define PTABLE_NANDMONITOR_LEN 0
#endif
#define PTABLE_NANDMONITOR_END (PTABLE_NANDMONITOR_START + PTABLE_NANDMONITOR_LEN)

/* Uce */
#define PTABLE_UCE_START PTABLE_NANDMONITOR_END
#define PTABLE_UCE_LEN (PRODUCT_CFG_FLASH_UCE_LEN)
#define PTABLE_UCE_END (PTABLE_UCE_START + PTABLE_UCE_LEN)

/* m3image */
#define PTABLE_M3IMAGE_START PTABLE_UCE_END
#define PTABLE_M3IMAGE_LEN (PRODUCT_CFG_FLASH_M3IMAGE_LEN)
#define PTABLE_M3IMAGE_END (PTABLE_M3IMAGE_START + PTABLE_M3IMAGE_LEN)

/* Recovery-A */
#define PTABLE_RECOVERY_A_START PTABLE_M3IMAGE_END
#define PTABLE_RECOVERY_A_LEN (PRODUCT_CFG_FLASH_RECOVERY_LEN)
#define PTABLE_RECOVERY_A_END (PTABLE_RECOVERY_A_START + PTABLE_RECOVERY_A_LEN)

/* Recovery-B */
#define PTABLE_RECOVERY_B_START PTABLE_RECOVERY_A_END
#define PTABLE_RECOVERY_B_LEN (PRODUCT_CFG_FLASH_RECOVERY_LEN)
#define PTABLE_RECOVERY_B_END (PTABLE_RECOVERY_B_START + PTABLE_RECOVERY_B_LEN)

/* misc */
#define PTABLE_MISC_START PTABLE_RECOVERY_B_END
#define PTABLE_MISC_LEN (PRODUCT_CFG_FLASH_MISC_LEN)
#define PTABLE_MISC_END (PTABLE_MISC_START + PTABLE_MISC_LEN)

/* dts */
#define PTABLE_DTS_START PTABLE_MISC_END
#define PTABLE_DTS_LEN (PRODUCT_CFG_FLASH_DTIMG_LEN)
#define PTABLE_DTS_END (PTABLE_DTS_START + PTABLE_DTS_LEN)

/* teeos */
#define PTABLE_TEEOS_START PTABLE_DTS_END
#define PTABLE_TEEOS_LEN (PRODUCT_CFG_FLASH_TEEOS_LEN)
#define PTABLE_TEEOS_END (PTABLE_TEEOS_START + PTABLE_TEEOS_LEN)

/* trsted_fw  */
#define PTABLE_TRUSTED_FW_START (PTABLE_TEEOS_END)
#define PTABLE_TRUSTED_FW_LEN (PRODUCT_CFG_FLASH_TRUSTED_FW_LEN)
#define PTABLE_TRUSTED_FW_END (PTABLE_TRUSTED_FW_START + PTABLE_TRUSTED_FW_LEN)

/* kernel */
#define PTABLE_BOOTIMAGE_START PTABLE_TRUSTED_FW_END
#define PTABLE_BOOTIMAGE_LEN (PRODUCT_CFG_FLASH_BOOTIMG_LEN)
#define PTABLE_BOOTIMAGE_END (PTABLE_BOOTIMAGE_START + PTABLE_BOOTIMAGE_LEN)

/* kernelbak */
#define PTABLE_BOOTIMAGEBAK_START PTABLE_BOOTIMAGE_END
#define PTABLE_BOOTIMAGEBAK_LEN (PRODUCT_CFG_FLASH_BOOTIMGBAK_LEN)
#define PTABLE_BOOTIMAGEBAK_END (PTABLE_BOOTIMAGEBAK_START + PRODUCT_CFG_FLASH_BOOTIMGBAK_LEN)

/* modem_fw  */
#define PTABLE_MODEM_FW_START (PTABLE_BOOTIMAGEBAK_END)
#define PTABLE_MODEM_FW_LEN (PRODUCT_CFG_FLASH_MODEM_FW_LEN)
#define PTABLE_MODEM_FW_END (PTABLE_MODEM_FW_START + PTABLE_MODEM_FW_LEN)

/* Hifi */
#define PTABLE_HIFI_START PTABLE_MODEM_FW_END
#define PTABLE_HIFI_LEN (PRODUCT_CFG_FLASH_HIFI_LEN)
#define PTABLE_HIFI_END (PTABLE_HIFI_START + PTABLE_HIFI_LEN)

/* OEM info */
#define PTABLE_OEMINFO_START PTABLE_HIFI_END
#define PTABLE_OEMINFO_LEN (PRODUCT_CFG_FLASH_OEMINFO_LEN)
#define PTABLE_OEMINFO_END (PTABLE_OEMINFO_START + PTABLE_OEMINFO_LEN)

/* Reserve1 */
#define PTABLE_RESERVE1_START PTABLE_OEMINFO_END
#define PTABLE_RESERVE1_LEN (PRODUCT_CFG_FLASH_RESERVE1_LEN)
#define PTABLE_RESERVE1_END (PTABLE_RESERVE1_START + PTABLE_RESERVE1_LEN)

/* Reserve2 */
#define PTABLE_RESERVE2_START PTABLE_RESERVE1_END
#define PTABLE_RESERVE2_LEN (PRODUCT_CFG_FLASH_RESERVE2_LEN)
#define PTABLE_RESERVE2_END (PTABLE_RESERVE2_START + PRODUCT_CFG_FLASH_RESERVE2_LEN)

/* Reserve3 */
#define PTABLE_RESERVE3_START PTABLE_RESERVE2_END
#define PTABLE_RESERVE3_LEN (PRODUCT_CFG_FLASH_RESERVE3_LEN)
#define PTABLE_RESERVE3_END (PTABLE_RESERVE3_START + PRODUCT_CFG_FLASH_RESERVE3_LEN)

/* system */
#define PTABLE_FS_SYSIMG_START PTABLE_RESERVE3_END
#define PTABLE_FS_SYSIMG_LEN (PRODUCT_CFG_FLASH_SYSIMG_LEN)
#define PTABLE_FS_SYSIMG_END (PTABLE_FS_SYSIMG_START + PTABLE_FS_SYSIMG_LEN)

/* UBI_USE */
#define PTABLE_UBIUSE_START PTABLE_FS_SYSIMG_END
#define PTABLE_UBIUSE_LEN (PRODUCT_CFG_FLASH_UBIUSE_LEN)
#define PTABLE_UBIUSE_END (PTABLE_UBIUSE_START + PTABLE_UBIUSE_LEN)

/* hac */
#define PTABLE_HACLD_START (PTABLE_UBIUSE_END)
#define PTABLE_HACLD_LEN (PRODUCT_CFG_FLASH_HAC_IMG_LEN)
#define PTABLE_HACLD_END (PTABLE_HACLD_START + PTABLE_HACLD_LEN)

/* ISO  */
#define PTABLE_ISO_START (PTABLE_HACLD_END)
#define PTABLE_ISO_LEN (PRODUCT_CFG_FLASH_ISO_LEN)
#define PTABLE_ISO_END (PTABLE_ISO_START + PTABLE_ISO_LEN)

/*
 * FLASH�ռ���� FMC BOOT
 */
#else
#define PTABLE_GPT_START (0)
#define PTABLE_GPT_LEN (0)
#define PTABLE_GPT_END (PTABLE_GPT_START + PTABLE_GPT_LEN)
/* M3boot */
#define PTABLE_M3_BOOT_START (0)
#define PTABLE_M3_BOOT_LEN (PRODUCT_CFG_FLASH_M3BOOT_LEN)
#define PTABLE_M3_BOOT_END (PTABLE_M3_BOOT_START + PTABLE_M3_BOOT_LEN)

/* Fastboot */
#define PTABLE_FASTBOOT_START PTABLE_M3_BOOT_END
#define PTABLE_FASTBOOT_LEN (PRODUCT_CFG_FLASH_FASTBOOT_LEN)
#define PTABLE_FASTBOOT_END (PTABLE_FASTBOOT_START + PTABLE_FASTBOOT_LEN)

/* NvDefault */
#define PTABLE_NVFACTORY_START PTABLE_FASTBOOT_END
#define PTABLE_NVFACTORY_LEN (PRODUCT_CFG_FLASH_NV_FACTORY_LEN)
#define PTABLE_NVFACTORY_END (PTABLE_NVFACTORY_START + PTABLE_NVFACTORY_LEN)

/* NV LTE */
#define PTABLE_NV_LTE_START PTABLE_NVFACTORY_END
#define PTABLE_NV_LTE_LEN (PRODUCT_CFG_FLASH_NV_LTE_LEN)
#define PTABLE_NV_LTE_END (PTABLE_NV_LTE_START + PTABLE_NV_LTE_LEN)

/* NvImg */
#define PTABLE_NVIMG_START PTABLE_NV_LTE_END
#define PTABLE_NVIMG_LEN (PRODUCT_CFG_FLASH_NV_IMG_LEN)
#define PTABLE_NVIMG_END (PTABLE_NVIMG_START + PTABLE_NVIMG_LEN)

/* NvDload */
#define PTABLE_NVDLD_START PTABLE_NVIMG_END
#define PTABLE_NVDLD_LEN (PRODUCT_CFG_FLASH_NV_DLD_LEN)
#define PTABLE_NVDLD_END (PTABLE_NVDLD_START + PTABLE_NVDLD_LEN)

/* NvCust */
#define PTABLE_NVCUST_START PTABLE_NVDLD_END
#define PTABLE_NVCUST_LEN (PRODUCT_CFG_FLASH_NV_CUST_LEN)
#define PTABLE_NVCUST_END (PTABLE_NVCUST_START + PTABLE_NVCUST_LEN)

/* nandmonitor */
#define PTABLE_NANDMONITOR_START PTABLE_NVCUST_END
#ifdef CONFIG_FMC_MNTN_ENABLE
#define PTABLE_NANDMONITOR_LEN 0x100000
#else
#define PTABLE_NANDMONITOR_LEN 0
#endif
#define PTABLE_NANDMONITOR_END (PTABLE_NANDMONITOR_START + PTABLE_NANDMONITOR_LEN)

/* Uce */
#define PTABLE_UCE_START PTABLE_NANDMONITOR_END
#define PTABLE_UCE_LEN (PRODUCT_CFG_FLASH_UCE_LEN)
#define PTABLE_UCE_END (PTABLE_UCE_START + PTABLE_UCE_LEN)

/* SecStorage */
#define PTABLE_SEC_STORAGE_START PTABLE_UCE_END
#define PTABLE_SEC_STORAGE_LEN (PRODUCT_CFG_FLASH_SEC_STORAGE_LEN)
#define PTABLE_SEC_STORAGE_END (PTABLE_SEC_STORAGE_START + PTABLE_SEC_STORAGE_LEN)

/* m3image */
#define PTABLE_M3IMAGE_START PTABLE_SEC_STORAGE_END
#define PTABLE_M3IMAGE_LEN (PRODUCT_CFG_FLASH_M3IMAGE_LEN)
#define PTABLE_M3IMAGE_END (PTABLE_M3IMAGE_START + PTABLE_M3IMAGE_LEN)

/* Recovery-A */
#define PTABLE_RECOVERY_A_START PTABLE_M3IMAGE_END
#define PTABLE_RECOVERY_A_LEN (PRODUCT_CFG_FLASH_RECOVERY_LEN)
#define PTABLE_RECOVERY_A_END (PTABLE_RECOVERY_A_START + PTABLE_RECOVERY_A_LEN)

/* Recovery-B */
#define PTABLE_RECOVERY_B_START PTABLE_RECOVERY_A_END
#define PTABLE_RECOVERY_B_LEN (PRODUCT_CFG_FLASH_RECOVERY_LEN)
#define PTABLE_RECOVERY_B_END (PTABLE_RECOVERY_B_START + PTABLE_RECOVERY_B_LEN)

/* misc */
#define PTABLE_MISC_START PTABLE_RECOVERY_B_END
#define PTABLE_MISC_LEN (PRODUCT_CFG_FLASH_MISC_LEN)
#define PTABLE_MISC_END (PTABLE_MISC_START + PTABLE_MISC_LEN)

/* dts */
#define PTABLE_DTS_START PTABLE_MISC_END
#define PTABLE_DTS_LEN (PRODUCT_CFG_FLASH_DTIMG_LEN)
#define PTABLE_DTS_END (PTABLE_DTS_START + PTABLE_DTS_LEN)

/* teeos */
#define PTABLE_TEEOS_START PTABLE_DTS_END
#define PTABLE_TEEOS_LEN (PRODUCT_CFG_FLASH_TEEOS_LEN)
#define PTABLE_TEEOS_END (PTABLE_TEEOS_START + PTABLE_TEEOS_LEN)

/* trsted_fw  */
#define PTABLE_TRUSTED_FW_START (PTABLE_TEEOS_END)
#define PTABLE_TRUSTED_FW_LEN (PRODUCT_CFG_FLASH_TRUSTED_FW_LEN)
#define PTABLE_TRUSTED_FW_END (PTABLE_TRUSTED_FW_START + PTABLE_TRUSTED_FW_LEN)

/* kernel */
#define PTABLE_BOOTIMAGE_START PTABLE_TRUSTED_FW_END
#define PTABLE_BOOTIMAGE_LEN (PRODUCT_CFG_FLASH_BOOTIMG_LEN)
#define PTABLE_BOOTIMAGE_END (PTABLE_BOOTIMAGE_START + PTABLE_BOOTIMAGE_LEN)

/* kernelbak */
#define PTABLE_BOOTIMAGEBAK_START PTABLE_BOOTIMAGE_END
#define PTABLE_BOOTIMAGEBAK_LEN (PRODUCT_CFG_FLASH_BOOTIMGBAK_LEN)
#define PTABLE_BOOTIMAGEBAK_END (PTABLE_BOOTIMAGEBAK_START + PRODUCT_CFG_FLASH_BOOTIMGBAK_LEN)

/* modem_fw  */
#define PTABLE_MODEM_FW_START (PTABLE_BOOTIMAGEBAK_END)
#define PTABLE_MODEM_FW_LEN (PRODUCT_CFG_FLASH_MODEM_FW_LEN)
#define PTABLE_MODEM_FW_END (PTABLE_MODEM_FW_START + PTABLE_MODEM_FW_LEN)

/* FS ONLINE */
#define PTABLE_FS_ONLINE_START PTABLE_MODEM_FW_END
#define PTABLE_FS_ONLINE_LEN (PRODUCT_CFG_FLASH_YAFFS_ONLINE_LEN)
#define PTABLE_FS_ONLINE_END (PTABLE_FS_ONLINE_START + PTABLE_FS_ONLINE_LEN)

/* Hifi */
#define PTABLE_HIFI_START PTABLE_FS_ONLINE_END
#define PTABLE_HIFI_LEN (PRODUCT_CFG_FLASH_HIFI_LEN)
#define PTABLE_HIFI_END (PTABLE_HIFI_START + PTABLE_HIFI_LEN)

/* OEM info */
#define PTABLE_OEMINFO_START PTABLE_HIFI_END
#define PTABLE_OEMINFO_LEN (PRODUCT_CFG_FLASH_OEMINFO_LEN)
#define PTABLE_OEMINFO_END (PTABLE_OEMINFO_START + PTABLE_OEMINFO_LEN)

/* Reserve1 */
#define PTABLE_RESERVE1_START PTABLE_OEMINFO_END
#define PTABLE_RESERVE1_LEN (PRODUCT_CFG_FLASH_RESERVE1_LEN)
#define PTABLE_RESERVE1_END (PTABLE_RESERVE1_START + PTABLE_RESERVE1_LEN)

/* Reserve2 */
#define PTABLE_RESERVE2_START PTABLE_RESERVE1_END
#define PTABLE_RESERVE2_LEN (PRODUCT_CFG_FLASH_RESERVE2_LEN)
#define PTABLE_RESERVE2_END (PTABLE_RESERVE2_START + PRODUCT_CFG_FLASH_RESERVE2_LEN)

/* Reserve3 */
#define PTABLE_RESERVE3_START PTABLE_RESERVE2_END
#define PTABLE_RESERVE3_LEN (PRODUCT_CFG_FLASH_RESERVE3_LEN)
#define PTABLE_RESERVE3_END (PTABLE_RESERVE3_START + PRODUCT_CFG_FLASH_RESERVE3_LEN)

/* FS APP */
#define PTABLE_FS_APP_START PTABLE_RESERVE3_END
#define PTABLE_FS_APP_LEN (PRODUCT_CFG_FLASH_YAFFS_APP_LEN)
#define PTABLE_FS_APP_END (PTABLE_FS_APP_START + PTABLE_FS_APP_LEN)

/* FS WEBUI */
#define PTABLE_FS_WEBUI_START PTABLE_FS_APP_END
#define PTABLE_FS_WEBUI_LEN (PRODUCT_CFG_FLASH_YAFFS_WEBUI_LEN)
#define PTABLE_FS_WEBUI_END (PTABLE_FS_WEBUI_START + PTABLE_FS_WEBUI_LEN)

/* system */
#define PTABLE_FS_SYSIMG_START PTABLE_FS_WEBUI_END
#define PTABLE_FS_SYSIMG_LEN (PRODUCT_CFG_FLASH_SYSIMG_LEN)
#define PTABLE_FS_SYSIMG_END (PTABLE_FS_SYSIMG_START + PTABLE_FS_SYSIMG_LEN)

/* USER DATA */
#define PTABLE_FS_USERDATA_START PTABLE_FS_SYSIMG_END
#define PTABLE_FS_USERDATA_LEN (PRODUCT_CFG_FLASH_YAFFS_USERDATA_LEN)
#define PTABLE_FS_USERDATA_END (PTABLE_FS_USERDATA_START + PTABLE_FS_USERDATA_LEN)

/* logo */
#define PTABLE_LOGO_START PTABLE_FS_USERDATA_END
#define PTABLE_LOGO_LEN (PRODUCT_CFG_FLASH_LOGO_LEN)
#define PTABLE_LOGO_END (PTABLE_LOGO_START + PRODUCT_CFG_FLASH_LOGO_LEN)

/* cache */
#define PTABLE_CACHE_START PTABLE_LOGO_END
#define PTABLE_CACHE_LEN (PRODUCT_CFG_FLASH_CACHE_LEN)
#define PTABLE_CACHE_END (PTABLE_CACHE_START + PTABLE_CACHE_LEN)

/* hac */
#define PTABLE_HACLD_START (PTABLE_CACHE_END)
#define PTABLE_HACLD_LEN (PRODUCT_CFG_FLASH_HAC_IMG_LEN)
#define PTABLE_HACLD_END (PTABLE_HACLD_START + PTABLE_HACLD_LEN)

/* ISO  */
#define PTABLE_ISO_START (PTABLE_HACLD_END)
#define PTABLE_ISO_LEN (PRODUCT_CFG_FLASH_ISO_LEN)
#define PTABLE_ISO_END (PTABLE_ISO_START + PTABLE_ISO_LEN)

#endif

/* partition name define, must be less than 16 characters */
#define PTABLE_GPT_NM "ptable"
#define PTABLE_M3BOOT_NM "m3boot"
#define PTABLE_FASTBOOT_NM "fastboot"
#define PTABLE_NVBACK_NM "nvbacklte"
#define PTABLE_NVIMG_NM "nvimg"
#define PTABLE_NVDLOAD_NM "nvdload"
#define PTABLE_NVCUST_NM "nvcust"
#define PTABLE_NANDMONITOR_NM "nandmonitor"
#define PTABLE_UCE_NM "uce"
#define PTABLE_HACLOAD_NM "hacload"
#define PTABLE_SEC_STORAGE_NM "sec_storage"
#define PTABLE_NVDEFAULT_NM "nvdefault"
#define PTABLE_OEMINFO_NM "oeminfo"
#define PTABLE_USERDATA_NM "userdata"
#define PTABLE_ONLINE_NM "online"
#define PTABLE_KERNEL_NM "kernel"
#define PTABLE_KERNELBK_NM "kernelbk"
#define PTABLE_LOGO_NM "logo"
#define PTABLE_M3IMG_NM "m3image"
#define PTABLE_HIFI_NM "hifi"
#define PTABLE_MISC_NM "misc"
#define PTABLE_RESERVE1_NM "reserve1"
#define PTABLE_RESERVE2_NM "reserve2"
#define PTABLE_RESERVE3_NM "reserve3"
#define PTABLE_OM_NM "om"
#define PTABLE_APP_NM "app"
#define PTABLE_WEBUI_NM "webui"
#define PTABLE_SYSTEM_NM "system"
#define PTABLE_DTS_NM "dts"
#define PTABLE_CDROMISO_NM "cdromiso"
#define PTABLE_CACHE_NM "cache"
#define PTABLE_RECOVERYA_NM "recovery-a"
#define PTABLE_RECOVERYB_NM "recovery-b"
#define PTABLE_TEEOS_NM "teeos"
#define PTABLE_MODEM_FW_NM "modem_fw"
#define PTABLE_TRUSTED_FW_NM "trusted_fw"
#define PTABLE_UBIUSE_NM "ubi_use"
#define PTABLE_DEBUG_CERT "debug_cert"
#ifdef __cplusplus
}
#endif

#endif
