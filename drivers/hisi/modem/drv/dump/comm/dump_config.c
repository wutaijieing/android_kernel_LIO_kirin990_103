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

#include <linux/of.h>
#include <linux/delay.h>
#include <asm/string.h>
#include "osl_types.h"
#include "bsp_version.h"
#include "bsp_sram.h"
#include "bsp_dump_mem.h"
#include "bsp_coresight.h"
#include "mdrv_om.h"
#include "nv_stru_drv.h"
#include "bsp_nvim.h"
#include "bsp_dump.h"
#include "dump_config.h"
#include "bsp_slice.h"
#include "dump_debug.h"

#undef THIS_MODU
#define THIS_MODU mod_dump

NV_DUMP_STRU g_dump_cfg;
dump_product_type_e g_product_type = DUMP_PRODUCT_BUTT;
enum EDITION_KIND g_edition = EDITION_MAX;
dump_access_mdmddr_type_e g_dump_access_ddr_type = DUMP_ACCESS_MDD_DDR_SEC_BUTT;
DUMP_FILE_CFG_STRU g_dump_file_cfg;

/*
 * 功能描述: 拷贝寄存器函数
 */
bool dump_is_ab_version(void)
{
    return false;
}

/*
 * 功能描述: 拷贝寄存器函数
 */
void dump_copy_register(u32 *dst, const u32 *src, u32 len)
{
    while (len-- > 0) {
        *dst++ = *src++;
    }
}

/*
 * 功能描述: 获取当前的版本类型
 */
dump_access_mdmddr_type_e dump_get_access_mdmddr_type(void)
{
    struct device_node *node = NULL;

    if (g_dump_access_ddr_type != DUMP_ACCESS_MDD_DDR_SEC_BUTT) {
        return g_dump_access_ddr_type;
    }

    node = of_find_compatible_node(NULL, NULL, "hisilicon,ap_sec_access_mdmddr");
    if (NULL == node) {
        dump_error("fail to read ap_sec_access_mdmddr mntn type from dts!\n");
        return DUMP_ACCESS_MDD_DDR_SEC_BUTT;
    }

    if (of_property_read_u32(node, "ap_sec_access_mdmddr", &g_dump_access_ddr_type)) {
        dump_error("fail to read read ap_sec_access_mdmddr !\n");
        return DUMP_ACCESS_MDD_DDR_SEC_BUTT;
    }
    return g_dump_access_ddr_type;
}

/*
 * 功能描述: 初始化产品形态
 */
dump_product_type_e dump_get_product_type(void)
{
    struct device_node *node = NULL;
    const char *product_type = NULL;

    if (DUMP_PRODUCT_BUTT != g_product_type) {
        return g_product_type;
    }
    node = of_find_compatible_node(NULL, NULL, "hisilicon,smntn_type");
    if (NULL == node) {
        dump_error("fail to read dts node mntn type !\n");
        return DUMP_MBB;
    }

    if (of_property_read_string(node, "product_type", &product_type)) {
        dump_error("fail to read product_type !\n");
        return DUMP_MBB;
    }

    if (0 == strncmp(product_type, "MBB", strlen("MBB"))) {
        g_product_type = DUMP_MBB;
    } else if (0 == strncmp(product_type, "PHONE", strlen("PHONE"))) {
        g_product_type = DUMP_PHONE;
    } else {
        dump_fetal("find product type error use mbb default\n");
        g_product_type = DUMP_MBB;
    }
    return g_product_type;
}

/*
 * 功能描述: 获取当前特性
 */
NV_DUMP_STRU *dump_get_feature_cfg(void)
{
    s32 ret = BSP_OK;

    ret = bsp_nvm_read(NVID_DUMP, (u8 *)&g_dump_cfg, sizeof(NV_DUMP_STRU));
    if (BSP_OK != ret) {
        /* 使用默认值 */
        g_dump_cfg.dump_cfg.Bits.dump_switch = 0x1;
        g_dump_cfg.dump_cfg.Bits.ARMexc = 0x1;
        g_dump_cfg.dump_cfg.Bits.stackFlow = 0x1;
        g_dump_cfg.dump_cfg.Bits.taskSwitch = 0x1;
        g_dump_cfg.dump_cfg.Bits.intSwitch = 0x1;
        g_dump_cfg.dump_cfg.Bits.intLock = 0x1;
        g_dump_cfg.dump_cfg.Bits.appRegSave1 = 0x0;
        g_dump_cfg.dump_cfg.Bits.appRegSave2 = 0x0;
        g_dump_cfg.dump_cfg.Bits.appRegSave3 = 0x0;
        g_dump_cfg.dump_cfg.Bits.commRegSave1 = 0x0;
        g_dump_cfg.dump_cfg.Bits.commRegSave2 = 0x0;
        g_dump_cfg.dump_cfg.Bits.commRegSave3 = 0x0;
        g_dump_cfg.dump_cfg.Bits.sysErrReboot = 0x1;
        g_dump_cfg.dump_cfg.Bits.reset_log = 0x1;
        g_dump_cfg.dump_cfg.Bits.fetal_err = 0x1;
        g_dump_cfg.dump_cfg.Bits.dumpTextClip = 0x0;
        dump_error("fail to read dump nv,nv id = 0x%x\n", NVID_DUMP);
    }

    return &g_dump_cfg;
}

/*
 * 功能描述: 获取当前的版本类型
 */
enum EDITION_KIND dump_get_edition_type(void)
{
    if (g_edition == EDITION_MAX) {
        g_edition = (enum EDITION_KIND)bbox_check_edition();
    }

    return g_edition;
}
/*
 * 功能描述: 判断文件的保存
 */

int dump_file_cfg_init(void)
{
    if (BSP_OK != bsp_nvm_read(NV_ID_DRV_DUMP_FILE, (u8 *)&g_dump_file_cfg, sizeof(DUMP_FILE_CFG_STRU))) {
        g_dump_file_cfg.file_cnt = 2;
        g_dump_file_cfg.file_list.file_bits.mdm_dump = 0x1;
        g_dump_file_cfg.file_list.file_bits.mdm_share = 0x1;
        g_dump_file_cfg.file_list.file_bits.mdm_ddr = 0x1;
        g_dump_file_cfg.file_list.file_bits.mdm_etb = 0x1;
        g_dump_file_cfg.file_list.file_bits.lphy_tcm = 0x1;
        g_dump_file_cfg.file_list.file_bits.lpm3_tcm = 0x1;
        g_dump_file_cfg.file_list.file_bits.ap_etb = 0x1;
        g_dump_file_cfg.file_list.file_bits.reset_log = 0x1;

        dump_error("fail to init dump feature\n");

        return BSP_ERROR;
    }
    return BSP_OK;
}

/*
 * 功能描述: 查询文件保存
 */

DUMP_FILE_CFG_STRU *dump_get_file_cfg(void)
{
    return &g_dump_file_cfg;
}

bool bsp_dump_is_secdump(void)
{
    NV_DUMP_STRU *cfg = dump_get_feature_cfg();
    if (cfg->dump_cfg.Bits.secDump) {
        return true;
    }
    return false;
}
dump_product_type_e bsp_dump_get_product_type(void)
{
    return dump_get_product_type();
}
dump_access_mdmddr_type_e bsp_dump_get_access_mdmddr_type(void)
{
    return dump_get_access_mdmddr_type();
}
