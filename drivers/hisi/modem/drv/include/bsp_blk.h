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

#ifndef __BSP_BLK_H__
#define __BSP_BLK_H__

/*
 * Attention
 * Description : Driver for blk
 * Core          : Acore、Mcore、Fastboot
 * Header File : the following head files need to be modified at the same time
 *                 : /acore/kernel/drivers/hisi/modem/drv/include/bsp_blk.h
 *                 : /mcore/xloader/include/bsp_blk.h
 *                 : /fastboot/include/bsp_blk.h
 */
/* 
 * fmc自适应约束，注意事项:
 * 1.block0不支持擦除后直接读取，请写入后再读取;
 * 2.block0只支持整块写入，不支持部分写入。
 */
#include<linux/types.h>
#include <linux/io.h>
#include <product_config.h>

#ifndef BSP_CONFIG_PHONE_TYPE
#include <hi_bootmode.h>
#include <soc_onchiprom.h>
#include <bsp_sysctrl.h>
#endif

#ifdef __cplusplus /* __cplusplus */
extern "C"
{
#endif /* __cplusplus */

#ifndef BSP_CONFIG_PHONE_TYPE
static inline unsigned int bsp_boot_mode_get(void)
{
    return readl(bsp_sysctrl_addr_get((void *)SC_AO_BASE_ADDR)+BOOT_MODE_OFFSET) & SC_BOOTMODE_BITMASK;
}
#else
static inline unsigned int bsp_boot_mode_get(void)
{
    return 0xffffffff;
}
#endif


#ifndef BOOT_MODE_FMC_PAD
#define BOOT_MODE_FMC_PAD     0xffffffff
#endif
#ifndef BOOT_MODE_AP_PCIE
#define BOOT_MODE_AP_PCIE     0xfffffffe
#endif
#ifndef BOOT_MODE_AP_UART
#define BOOT_MODE_AP_UART     0xfffffffd
#endif
#ifndef BOOT_MODE_EMMC0
#define BOOT_MODE_EMMC0       0xfffffffc
#endif

#define BLOCK_BALONG_NOT_SUPPORT 0
#define BLOCK_BALONG_IS_SUPPORT 1


struct block_ops {
    int (*read)(const char *partition_name, loff_t partition_offset, void *data_addr, size_t data_len);
    int (*write)(const char *partition_name, loff_t partition_offset, const void *data_addr, size_t data_len);
    int (*erase)(const char *partition_name);
    int (*isbad)(const char *partition_name, loff_t partition_offset);
    int (*get_part_size)(const char *part_name, u32 *size);
    int (*find_part)(const char *part_name, char *blk_path, unsigned int blk_path_len);
    int (*get_cdromiso_blkname)(char *blk_path, int len);
};
int bsp_blk_register_ops(struct block_ops* ops);
int bsp_blk_read(const char *part_name,loff_t part_offset, void *data_buf, size_t data_len);
int bsp_blk_write(const char *part_name,loff_t part_offset, const void *data_buf, size_t data_len);
int bsp_blk_isbad(const char *part_name,loff_t partition_offset);
int bsp_blk_erase(const char *part_name);
int bsp_blk_size(const char *part_name,u32 *size);
int bsp_blk_get_cdromiso_blkname(char *blk_path, int len);
int flash_find_ptn_s(const char *part_name, char *blk_path, unsigned int blk_path_len);

static inline unsigned int bsp_blk_is_support(void)
{
#ifdef BSP_CONFIG_EMU_NANDC_MEM
    return BLOCK_BALONG_NOT_SUPPORT;
#endif
    return BLOCK_BALONG_IS_SUPPORT;
}

#ifdef __cplusplus /* __cplusplus */
}
#endif /* __cplusplus */

#endif
