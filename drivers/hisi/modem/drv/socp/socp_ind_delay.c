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
#include <mdrv.h>
#include "socp_ind_delay.h"
#include <securec.h>

#ifdef BSP_CONFIG_PHONE_TYPE
#include <adrv.h>
#endif

#include "socp_balong.h"

#ifdef CONFIG_DEFLATE
#include "deflate.h"
#endif

#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include "bsp_socp.h"

#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>
#include <nv_stru_drv.h>

#define THIS_MODU mod_socp
struct socp_enc_dst_log_cfg g_enc_dst_buf_log_cfg = {NULL, 0, 0, 10, false, 10, false, 0, 0};
struct socp_enc_dst_log_cfg g_deflate_dst_buf_log_cfg = {NULL, 0, 0, 10, false, 10, false, 0, 0};
u32 g_stDeflateState = 0;
socp_early_cfg_s g_socp_early_cfg = {NULL, 0, 0, 0, 0};
socp_rsv_mem_s g_socp_rsv_mem_info = {NULL, 0, 0, 0, 0, 0 };
DRV_DEFLATE_CFG_STRU g_deflate_nv_ctrl = {0, 0};
extern u32 g_deflate_status;
s32 deflate_set_compress_mode(SOCP_IND_MODE_ENUM mode);
u32 g_bbp_mem_flag;

extern struct platform_device *modem_socp_pdev;

#ifndef BSP_CONFIG_PHONE_TYPE
/*
* 函 数 名  : socp_logbuffer_sizeparse
* 功能描述  : 在代码编译阶段将CMD LINE中的BUFFER大小参数解析出来
* 输入参数  : 无
* 输出参数  : 无
* 返 回 值  : 无
*/
static int __init socp_bbp_mem_enable(char *str)
{
    g_bbp_mem_flag = (u32)simple_strtoul(str, NULL, 0);

    return 0;
}
early_param("modem_socp_enable",socp_bbp_mem_enable);

u32 bsp_socp_ds_mem_enable(void)
{
     return g_bbp_mem_flag;
}
#endif
/*
 * 函 数 名: socp_get_logbuffer_addrparse
 * 功能描述: 获取内存基地址
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
s32 socp_get_rsv_mem_addr(void)
{
    /* 物理地址是32位的实地址并且是8字节对齐的 */
    if ((0 != (g_socp_rsv_mem_info.phy_addr % 8)) ||
        (0 == g_socp_rsv_mem_info.phy_addr)) {
        socp_crit("rsv mem addr invalid, 0x%lx\n", g_socp_rsv_mem_info.phy_addr);
        return -1;
    }

    g_socp_early_cfg.phy_addr = g_socp_rsv_mem_info.phy_addr;

    return 0;
}

/*
 * 函 数 名: socp_get_logbuffer_sizeparse
 * 功能描述: 获取socp buffer大小
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
s32 socp_get_rsv_mem_size(void)
{
    u32 buff_size;

    /* Buffer的大小以Byte为单位，原则上不大于200M，不小于1M */
    buff_size = g_socp_rsv_mem_info.buff_size;

    if ((buff_size > SOCP_MAX_MEM_SIZE) || (buff_size < SOCP_MIN_MEM_SIZE)) {
        socp_crit("cmdline: BuffSize=0x%x\n", buff_size);
        return BSP_ERROR;
    }

    /* 为了保持ulBufferSize的长度8字节对齐,如果长度不是8字节对齐地址也不会 */
    if (0 != (buff_size % 8)) {
        socp_error("BuffSize no 8 byte allignment,BuffSize: 0x%x\n", buff_size);
        return BSP_ERROR;
    }
    g_socp_early_cfg.buff_size = buff_size;
    socp_crit("buff_size 0x%x, adapt buffer_size: 0x%x\n", buff_size, g_socp_early_cfg.buff_size);

    return BSP_OK;
}

/*
 * 函 数 名: socp_get_logbuffer_timeparse
 * 功能描述: 获取socp时间
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_get_threshold_timeout(void)
{
    u32 timeout;

    /* 输入字符串以秒为单位，需要再转换成毫秒，至少为1秒，不大于20分钟 */
    timeout = g_socp_rsv_mem_info.timeout;

    if (SOCP_MAX_TIMEOUT < timeout) {
        timeout = SOCP_MAX_TIMEOUT;
    }

    timeout *= 1000;

    g_socp_early_cfg.timeout = timeout;

    socp_crit("early_cfg: timeout=0x%x\n", g_socp_early_cfg.timeout);

    return;
}

/*
 * 函 数 名: socp_get_cmdline_param
 * 功能描述: 获取cmdline参数
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void socp_get_cmdline_info(void)
{
    s32 ret;

    socp_get_threshold_timeout();

    ret = socp_get_rsv_mem_addr();
    if(ret) {
        g_socp_early_cfg.buff_useable = BSP_FALSE;
        return;
    }

    ret = socp_get_rsv_mem_size();
    if(ret) {
        g_socp_early_cfg.buff_useable = BSP_FALSE;
        return;
    }

    g_socp_early_cfg.buff_useable = BSP_TRUE;
    return;
}



void *socp_logbuffer_memremap(unsigned long phys_addr, size_t size)
{
    unsigned long i;
    u8 *vaddr = NULL;
    unsigned long npages = ((unsigned long)PAGE_ALIGN((phys_addr & (PAGE_SIZE - 1)) + size) >> PAGE_SHIFT);
    unsigned long offset = phys_addr & (PAGE_SIZE - 1);
    struct page **pages = NULL;

    pages = vmalloc(sizeof(struct page *) * npages);
    if (NULL == pages) {
        return NULL;
    }

    pages[0] = phys_to_page(phys_addr);

    for (i = 0; i < npages - 1; i++) {
        pages[i + 1] = pages[i] + 1;
    }

    vaddr = (u8 *)vmap(pages, (unsigned int)npages, (unsigned long)VM_MAP, pgprot_writecombine(PAGE_KERNEL));

    if (NULL != vaddr)

    {
        vaddr += offset;
    }

    vfree(pages);

    return (void *)vaddr;
}

/*
 * 函 数 名: socp_rsv_mem_mmap
 * 功能描述: 物理地址转换成虚拟内存
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
s32 socp_rsv_mem_mmap(void)
{
    /* step1: if logcfg is on, mmap kernel reserved memory */
    if (BSP_TRUE == g_socp_early_cfg.buff_useable) {
        g_socp_rsv_mem_info.virt_addr = socp_logbuffer_memremap(g_socp_rsv_mem_info.phy_addr,
                                                                (size_t)g_socp_rsv_mem_info.buff_size);
        if (NULL == g_socp_rsv_mem_info.virt_addr) {
            socp_error("remap socp buffer fail\n");
            g_socp_rsv_mem_info.buff_useable = BSP_FALSE;
            return BSP_ERROR;
        }

        socp_crit("kernel reserved buffer mmap success\n");
    } else {
        socp_crit("do not use kernel rsv mem\n");
    }

    return BSP_OK;
}

/* log2.0 2014-03-19 Begin: */
/*
 * 函 数 名: bsp_socp_get_log_cfg
 * 功能描述: 获取LOG2.0 SOCP水线、超时配置信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: SOCP_ENC_DST_BUF_LOG_CFG_STRU指针
 */
struct socp_enc_dst_log_cfg *bsp_socp_get_log_cfg(void)
{
    return &g_enc_dst_buf_log_cfg;
}
/*
 * 函 数 名: bsp_socp_get_log_ind_mode
 * 功能描述: 上报模式接口
 * 输入参数: 模式参数
 * 输出参数: 无
 * 返 回 值: BSP_S32 BSP_OK:成功 BSP_ERROR:失败
 */
s32 bsp_socp_get_log_ind_mode(u32 *log_ind_mode)
{
    *log_ind_mode = g_enc_dst_buf_log_cfg.current_mode;
    return BSP_OK;
}

u32 bsp_socp_get_sd_logcfg(SOCP_ENC_DST_BUF_LOG_CFG_STRU *cfg)
{
    struct socp_enc_dst_log_cfg *log_cfg = NULL;
    u32 deflate_enable;
    if (NULL == cfg)
        return BSP_ERR_SOCP_INVALID_PARA;

    log_cfg = bsp_socp_get_log_cfg();

    cfg->logOnFlag = log_cfg->log_on_flag;
    cfg->BufferSize = log_cfg->buff_size;

    deflate_enable = bsp_socp_compress_status();
    if (1 == deflate_enable) {
        cfg->overTime = g_deflate_dst_buf_log_cfg.over_time;
    } else {
        cfg->overTime = log_cfg->over_time;
    }

    cfg->pVirBuffer = log_cfg->virt_addr;
    cfg->ulCurTimeout = log_cfg->cur_time_out;
    cfg->ulPhyBufferAddr = log_cfg->phy_addr;

    return BSP_OK;
}

unsigned int mdrv_socp_get_log_cfg(socp_encdst_buf_log_cfg_s *cfg)
{
    unsigned int ret;
    SOCP_ENC_DST_BUF_LOG_CFG_STRU get_cfg = {0};

    if (cfg == NULL) {
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    ret = bsp_socp_get_sd_logcfg(&get_cfg);
    if (ret == BSP_OK) {
        cfg->vir_buffer = get_cfg.pVirBuffer;
        cfg->phy_buffer_addr = get_cfg.ulPhyBufferAddr;
        cfg->buffer_size = get_cfg.BufferSize;
        cfg->over_time = get_cfg.overTime;
        cfg->log_on_flag = get_cfg.logOnFlag;
        cfg->cur_time_out = get_cfg.ulCurTimeout;
    }

    return ret;
}

/*
 * 函 数 名: socp_logbuffer_dmalloc
 * 功能描述: 动态申请内存
 * 输入参数: 设备节点
 * 输出参数: 无
 * 返 回 值: 设置成功与否标志
 */
s32 socp_logbuffer_dmalloc(struct device_node *dev)
{
    dma_addr_t address = 0;
    u8 *p_buff = NULL;
    int ret;
    u32 dst_chan[SOCP_DST_CHAN_CFG_BUTT] = {0};
    u32 size;

    ret = of_property_read_u32_array(dev, "dst_chan_cfg", dst_chan, (size_t)SOCP_DST_CHAN_CFG_BUTT);
    if (ret) {
        socp_crit("dts don't config dmalloc logbuffer size, use default size 1M!\n");
        size = 1 * 1024 * 1024;
    } else {
        socp_crit("of_property_read_u32_array get size 0x%x!\n", dst_chan[SOCP_DST_CHAN_CFG_SIZE]);
        size = dst_chan[SOCP_DST_CHAN_CFG_SIZE];
    }
#ifdef CONFIG_ARM64
    p_buff = (u8 *)dma_alloc_coherent(&modem_socp_pdev->dev, (size_t)size, &address, GFP_KERNEL);
#else
    p_buff = (u8 *)dma_alloc_coherent(NULL, size, &address, GFP_KERNEL);
#endif

    if (BSP_NULL == p_buff) {
        socp_error("logbuffer_dmalloc buffer failed\n");
        return BSP_ERROR;
    }

    socp_crit("socp_logbuffer_dmalloc success\n");

    g_enc_dst_buf_log_cfg.phy_addr = (unsigned long)address;
    g_enc_dst_buf_log_cfg.virt_addr = p_buff;
    g_enc_dst_buf_log_cfg.cur_time_out = 10; /* 使用默认值 */
    g_enc_dst_buf_log_cfg.buff_size = size;
    g_enc_dst_buf_log_cfg.log_on_flag = SOCP_DST_CHAN_DTS;

#ifdef CONFIG_DEFLATE
    g_deflate_dst_buf_log_cfg.phy_addr = (unsigned long)address;
    g_deflate_dst_buf_log_cfg.virt_addr = p_buff;
    g_deflate_dst_buf_log_cfg.cur_time_out = 10; /* 使用默认值 */
    g_deflate_dst_buf_log_cfg.buff_size = size;
    g_deflate_dst_buf_log_cfg.log_on_flag = SOCP_DST_CHAN_DTS;
#endif

    return BSP_OK;
}

/*
 * 函 数 名: bsp_socp_timeout_init
 * 功能描述: 模块初始化目的buffer上报超时配置的函数
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 初始化成功的标识码
 */
s32 bsp_socp_logbuffer_init(struct device_node *dev)
{
    s32 ret;

    /* step1: if mdmlog rsv mem is useable, use kernel reserved buffer */
    if (BSP_TRUE == g_socp_early_cfg.buff_useable) {
        g_enc_dst_buf_log_cfg.virt_addr = g_socp_rsv_mem_info.virt_addr;
        g_enc_dst_buf_log_cfg.phy_addr = g_socp_rsv_mem_info.phy_addr;
        g_enc_dst_buf_log_cfg.buff_size = g_socp_rsv_mem_info.buff_size;
        g_enc_dst_buf_log_cfg.over_time = g_socp_rsv_mem_info.timeout;
        g_enc_dst_buf_log_cfg.log_on_flag = SOCP_DST_CHAN_DELAY;
#ifdef CONFIG_DEFLATE
        g_deflate_dst_buf_log_cfg.virt_addr = g_socp_rsv_mem_info.virt_addr;
        g_deflate_dst_buf_log_cfg.phy_addr = g_socp_rsv_mem_info.phy_addr;
        g_deflate_dst_buf_log_cfg.buff_size = g_socp_rsv_mem_info.buff_size;
        g_deflate_dst_buf_log_cfg.over_time = g_socp_rsv_mem_info.timeout;
        g_deflate_dst_buf_log_cfg.log_on_flag = SOCP_DST_CHAN_DELAY;
#endif
    }
    /* step2: if mdmlog rsv mem is not useable, update logOnFlag state */
    else {
        g_enc_dst_buf_log_cfg.log_on_flag = SOCP_DST_CHAN_NOT_CFG;
#ifdef CONFIG_DEFLATE
        g_deflate_dst_buf_log_cfg.log_on_flag = SOCP_DST_CHAN_NOT_CFG;
#endif
    }

    /* step3: if mdmlog rsv mem is not useable, alloc dma mem for dst channel */
    if (SOCP_DST_CHAN_NOT_CFG == g_enc_dst_buf_log_cfg.log_on_flag) {
        ret = socp_logbuffer_dmalloc(dev);
        if (ret) {
            socp_error("of_property_read_u32_array failed!\n");
            return BSP_ERROR;
        }
    }

    return BSP_OK;
}

void bsp_socp_set_mode_direct(void)
{
    u32 ret;

    if (SOCP_IND_MODE_DIRECT == g_enc_dst_buf_log_cfg.current_mode) {
        socp_crit("the ind mode direct is already config!\n");
        return;
    }

    (void)bsp_socp_set_timeout(SOCP_TIMEOUT_TRF_LONG, SOCP_TIMEOUT_TRF_LONG_MIN);
    g_enc_dst_buf_log_cfg.cur_time_out = SOCP_TIMEOUT_TRF_LONG_MIN;
    bsp_socp_set_enc_dst_threshold((bool)FALSE, SOCP_CODER_DST_OM_IND);
    ret = bsp_socp_encdst_set_cycle(SOCP_CODER_DST_OM_IND, SOCP_IND_MODE_DIRECT);
    if (BSP_OK != ret) {
        socp_error("direct mode config failed!\n");
    } else {
        g_enc_dst_buf_log_cfg.current_mode = SOCP_IND_MODE_DIRECT;
        socp_crit("direct mode config sucess!\n");
    }
}

void bsp_socp_set_mode_delay(void)
{
    u32 ret;

    if (SOCP_IND_MODE_DELAY == g_enc_dst_buf_log_cfg.current_mode) {
        socp_crit("socp:the ind mode delay is already config!\n");
        return;
    }

    /* if logbuffer is not configed, can't enable delay mode */
    if (g_enc_dst_buf_log_cfg.log_on_flag == SOCP_DST_CHAN_DELAY) {
        (void)bsp_socp_set_timeout(SOCP_TIMEOUT_TRF_LONG, SOCP_TIMEOUT_TRF_LONG_MAX);
        g_enc_dst_buf_log_cfg.cur_time_out = SOCP_TIMEOUT_TRF_LONG_MAX;
        bsp_socp_set_enc_dst_threshold((bool)TRUE, SOCP_CODER_DST_OM_IND);
        ret = bsp_socp_encdst_set_cycle(SOCP_CODER_DST_OM_IND, SOCP_IND_MODE_DELAY);
        if (BSP_OK != ret) {
            socp_error("delay mode config failed!\n");
        } else {
            g_enc_dst_buf_log_cfg.current_mode = SOCP_IND_MODE_DELAY;
            socp_crit("delay mode config sucess!\n");
        }
    } else {
        socp_crit("delay mode can't config:mem can't be setted!\n");
    }
}

void bsp_socp_set_mode_cycle(void)
{
    u32 ret;

    if (SOCP_IND_MODE_CYCLE == g_enc_dst_buf_log_cfg.current_mode) {
        socp_crit("the ind mode cycle is already config!\n");
        return;
    }

    /* if logbuffer is not configed, can't enable cycle mode */
    /* 如果dst方式malloc 10M内存,也支持配置循环模式，支持商用10Mlog抓取 */
    if (((g_enc_dst_buf_log_cfg.log_on_flag == SOCP_DST_CHAN_DELAY) && (BSP_TRUE == g_socp_rsv_mem_info.buff_useable)) ||
        (g_enc_dst_buf_log_cfg.log_on_flag == SOCP_DST_CHAN_DTS)) {
        ret = bsp_socp_encdst_set_cycle(SOCP_CODER_DST_OM_IND, SOCP_IND_MODE_CYCLE);
        if (BSP_OK != ret) {
            socp_error("cycle mode config failed!\n");
        } else {
            g_enc_dst_buf_log_cfg.current_mode = SOCP_IND_MODE_CYCLE;
            socp_crit("the ind cycle mode config sucess!\n");
        }
    } else {
        socp_crit("ind delay can't config:mem can't be setted!\n");
    }
}

/*
 * 函 数 名: bsp_socp_set_ind_mode
 * 功能描述: 上报模式接口
 * 输入参数: 模式参数
 * 输出参数: 无
 * 返 回 值: BSP_S32 BSP_OK:成功 BSP_ERROR:失败
 */
s32 bsp_socp_set_ind_mode(SOCP_IND_MODE_ENUM mode)
{
    switch (mode) {
        case SOCP_IND_MODE_DIRECT: {
            bsp_socp_set_mode_direct();
            break;
        }
        case SOCP_IND_MODE_DELAY: {
            bsp_socp_set_mode_delay();
            break;
        }
        case SOCP_IND_MODE_CYCLE: {
            bsp_socp_set_mode_cycle();
            break;
        }
        default: {
            socp_error("set invalid mode: %d!\n", g_enc_dst_buf_log_cfg.current_mode);
            return BSP_ERROR;
        }
    }

    return BSP_OK;
}

/*
 * 函 数 名: bsp_socp_compress_status
 * 功能描述: 查询当前压缩模式是否开启
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */

s32 bsp_socp_compress_status(void)
{
    return g_deflate_status;
}
/*
 * 函 数 名: bsp_socp_get_cfg_ind_mode
 * 功能描述: 查询获取当前上报模式
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */

s32 bsp_socp_get_cfg_ind_mode(u32 *cfg_ind_mode)
{
    u32 deflate_tate;
    deflate_tate = bsp_socp_compress_status();
    if (SOCP_COMPRESS == deflate_tate) {
        bsp_deflate_get_log_ind_mode(cfg_ind_mode);
        socp_crit("deflate channel is open!\n");
        return BSP_OK;
    } else {
        bsp_socp_get_log_ind_mode(cfg_ind_mode);
        socp_crit("socp channel is open!\n");
        return BSP_OK;
    }
}
/*
 * 函 数 名: bsp_socp_set_cfg_ind_mode
 * 功能描述: 设置当前上报模式
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */

s32 bsp_socp_set_cfg_ind_mode(SOCP_IND_MODE_ENUM mode)
{
    u32 deflate_tate;
    
    deflate_tate = bsp_socp_compress_status();

    if (SOCP_NO_COMPRESS == deflate_tate) {
        bsp_socp_set_ind_mode(mode);
        socp_crit("socp channel is open!\n");
        return BSP_OK;
    } else {
        bsp_deflate_set_ind_mode(mode);
        socp_crit("deflate channel is open!\n");
    }
    return BSP_OK;
}
/*
 * 函 数 名: bsp_socp_get_cps_ind_mode
 * 功能描述: 获取当前压缩模式
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
s32 bsp_socp_get_cps_ind_mode(u32 *cps_ind_mode)
{
    u32 deflate_tate;
    deflate_tate = bsp_socp_compress_status();
    if (SOCP_COMPRESS == deflate_tate) {
        *cps_ind_mode = DEFLATE_IND_COMPRESS;
        socp_crit("deflate channel is open!\n");
        return BSP_OK;
    } else {
        *cps_ind_mode = DEFLATE_IND_NO_COMPRESS;
        socp_crit("socp channel is open!\n");
        return BSP_OK;
    }
}
/*
 * 函 数 名: bsp_socp_set_cps_ind_mode
 * 功能描述: 设置当前压缩模式
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
s32 bsp_socp_set_cps_ind_mode(DEFLATE_IND_COMPRESSS_ENUM mode)
{
#ifdef CONFIG_DEFLATE
    u32 deflate_tate;
    if (0 == g_deflate_nv_ctrl.deflate_enable) {
        socp_crit("deflate nv not open!\n");
        return BSP_ERROR;
    }
    deflate_tate = bsp_socp_compress_status();
    if (DEFLATE_IND_NO_COMPRESS == mode) {
        if (SOCP_NO_COMPRESS == deflate_tate) {
            socp_crit("deflate:compress disable is already config!\n");
            return BSP_OK;
        } else {
            bsp_socp_compress_disable(SOCP_CODER_DST_OM_IND);
            socp_crit("deflate:compress disable is config!\n");
            return BSP_OK;
        }

    } else if (DEFLATE_IND_COMPRESS == mode) {
        if (SOCP_COMPRESS == deflate_tate) {
            socp_crit("deflate:compress enable is already config!\n");
            return BSP_OK;

        } else {
            bsp_socp_compress_enable(SOCP_CODER_DST_OM_IND);
            socp_crit("deflate:compress enbale is config!\n");
            return BSP_OK;
        }
    } else {
        socp_error("deflate invalid mode: %d!\n", mode);
        return BSP_ERROR;
    }

    return BSP_OK;
#else
    return BSP_ERROR;
#endif
}

void bsp_socp_get_rsv_mem_info(void)
{
#ifdef BSP_CONFIG_PHONE_TYPE
    socp_attr_s *socp_mem_attr = NULL;

    if(g_socp_rsv_mem_info.init_flag != BSP_TRUE) {
        socp_mem_attr = bsp_socp_get_mem_info();
        if (socp_mem_attr != NULL) {
            g_socp_rsv_mem_info.virt_addr = NULL;
            g_socp_rsv_mem_info.phy_addr = socp_mem_attr->socp_mem_info.base_addr;
            g_socp_rsv_mem_info.buff_size = socp_mem_attr->socp_mem_info.buffer_size;
            g_socp_rsv_mem_info.buff_useable = socp_mem_attr->socp_mem_info.rsv_mem_usable;
            g_socp_rsv_mem_info.timeout = socp_mem_attr->socp_mem_info.threshold_timeout;

            g_socp_rsv_mem_info.init_flag = BSP_TRUE;
        }
    }else {
        g_socp_rsv_mem_info.virt_addr = NULL;
        g_socp_rsv_mem_info.timeout = SOCP_TIMEOUT_TRF_SHORT_VAL;
    }
#else
        g_socp_rsv_mem_info.virt_addr = NULL;
        g_socp_rsv_mem_info.timeout = SOCP_TIMEOUT_TRF_SHORT_VAL;
#endif

    return;
}

/*
 * 函 数 名: socp_ind_delay_init
 * 功能描述: 模块初始化函数
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 初始化成功的标识码
 */
s32 bsp_socp_ind_delay_init(void)
{
    s32 ret;
    struct device_node *dev = NULL;

    dev = of_find_compatible_node(NULL, NULL, "hisilicon,socp_balong_app");
    if (NULL == dev) {
        socp_error("Socp dev find failed\n");
        return BSP_ERROR;
    }

    /* get rsv mem info from kernel */
    bsp_socp_get_rsv_mem_info();

    /* 获取cmdline参数 */
    socp_get_cmdline_info();

    /* 对rsv mem进行map */
    (void)socp_rsv_mem_mmap();

    ret = bsp_socp_logbuffer_init(dev);
    if (ret) {
        socp_error("socp dst logbuffer init faield!\n");
        return ret;
    }

    socp_crit("socp_ind_delay init sucess!\n");

    return BSP_OK;
}

/*
 * 函 数 名: bsp_deflate_get_log_ind_mode
 * 功能描述: 上报模式接口
 * 输入参数: 模式参数
 * 输出参数: 无
 * 返 回 值: BSP_S32 BSP_OK:成功 BSP_ERROR:失败
 */

s32 bsp_deflate_get_log_ind_mode(u32 *cfg_ind_mode)
{
    *cfg_ind_mode = g_deflate_dst_buf_log_cfg.current_mode;

    return BSP_OK;
}

/*
 * 函 数 名: bsp_deflate_get_compress_mode
 * 功能描述: 上报模式接口
 * 输入参数: 模式参数
 * 输出参数: 无
 * 返 回 值: BSP_S32 BSP_OK:成功 BSP_ERROR:失败
 */

s32 bsp_deflate_get_compress_mode(u32 *cps_ind_mode)
{
    *cps_ind_mode = g_deflate_dst_buf_log_cfg.cps_mode;

    return BSP_OK;
}

/*
 * 函 数 名: bsp_deflate_set_ind_mode
 * 功能描述: 上报模式接口
 * 输入参数: 模式参数
 * 输出参数: 无
 * 返 回 值: BSP_S32 BSP_OK:成功 BSP_ERROR:失败
 */
s32 bsp_deflate_set_ind_mode(SOCP_IND_MODE_ENUM mode)
{
#ifdef CONFIG_DEFLATE
    u32 ret;
    switch (mode) {
        case SOCP_IND_MODE_DIRECT: {
            if (SOCP_IND_MODE_DIRECT == g_deflate_dst_buf_log_cfg.current_mode) {
                socp_crit("deflate direct mode is already config %d!\n", g_deflate_dst_buf_log_cfg.current_mode);
                break;
            }
            (void)deflate_set_time(SOCP_IND_MODE_DIRECT);
            g_deflate_dst_buf_log_cfg.cur_time_out = DEFLATE_TIMEOUT_INDIRECT;
            deflate_set_dst_threshold((bool)FALSE);
            deflate_set_cycle_mode(SOCP_IND_MODE_DIRECT);
            g_deflate_dst_buf_log_cfg.current_mode = mode;
            socp_crit("deflate direct mode config sucess %d!\n", g_deflate_dst_buf_log_cfg.current_mode);

            break;
        }
        case SOCP_IND_MODE_DELAY: {
            if (SOCP_IND_MODE_DELAY == g_deflate_dst_buf_log_cfg.current_mode) {
                socp_crit("deflate delay mode is already config %d!\n", g_deflate_dst_buf_log_cfg.current_mode);
                break;
            }

            /* if logbuffer is not configed, can't enable delay mode */
            if (g_deflate_dst_buf_log_cfg.log_on_flag == DEFLATE_DST_CHAN_DELAY) {
                ret = deflate_set_time(SOCP_IND_MODE_DELAY);
                if (0 != ret) {
                    socp_error("deflate set time error !\n");
                    break;
                }
                g_deflate_dst_buf_log_cfg.cur_time_out = DEFLATE_TIMEOUT_DEFLATY;
                ;
                deflate_set_dst_threshold((bool)TRUE);
                deflate_set_cycle_mode(SOCP_IND_MODE_DELAY);
                g_deflate_dst_buf_log_cfg.current_mode = mode;
                socp_crit("deflate delay mode config sucess %d!\n", g_deflate_dst_buf_log_cfg.current_mode);

            } else {
                socp_crit("deflate delay can't config:mem can't be setted!\n");
                return DEFLATE_ERR_MEM_NOT_ENOUGH;
            }
            break;
        }
        case SOCP_IND_MODE_CYCLE: {
            if (SOCP_IND_MODE_CYCLE == g_deflate_dst_buf_log_cfg.current_mode) {
                socp_crit("deflate cycle mode is already config %d!\n", g_deflate_dst_buf_log_cfg.current_mode);
                break;
            }

            /* if logbuffer is not configed, can't enable cycle mode */
            if (((g_enc_dst_buf_log_cfg.log_on_flag == DEFLATE_DST_CHAN_DELAY) &&
                 (BSP_TRUE == g_socp_rsv_mem_info.buff_useable)) ||
                (g_enc_dst_buf_log_cfg.log_on_flag == SOCP_DST_CHAN_DTS)) {

                deflate_set_cycle_mode(SOCP_IND_MODE_CYCLE);

                g_deflate_dst_buf_log_cfg.current_mode = mode;
                socp_crit("deflate cycle cycle mode config sucess %d!\n", g_deflate_dst_buf_log_cfg.current_mode);
            } else {
                socp_crit("deflate:ind delay can't config:mem can't be setted!\n");
                return DEFLATE_ERR_MEM_NOT_ENOUGH;
            }
            break;
        }

        default: {
            socp_error("deflate set invalid mode: %d!\n", g_deflate_dst_buf_log_cfg.current_mode);
            return BSP_ERROR;
        }
    }
#endif
    return BSP_OK;
}

/*
 * 函 数 名: bsp_socp_read_cur_mode
 * 功能描述: 循环模式查询接口
 * 输入参数: 通道ID
 * 输出参数: 无
 * 返 回 值: 初始化成功的标识码
 */
u32 bsp_socp_read_cur_mode(u32 dst_chan_id)
{
    u32 mode_state;
    u32 chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);

    /*lint -save -e647*/
    mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1);
    /*lint -restore +e647*/
    return mode_state;
}

/*
 * 函 数 名: bsp_socp_logbuffer_cfgshow
 * 功能描述: 打印延时写入的配置信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void bsp_socp_logbuffer_cfgshow(void)
{
    socp_crit("socp_logbuffer_cfgshow: over_time           %d\n", g_enc_dst_buf_log_cfg.over_time);
    socp_crit("socp_logbuffer_cfgshow: Current Time Out   %d\n", g_enc_dst_buf_log_cfg.cur_time_out);
    socp_crit("socp_logbuffer_cfgshow: buff_size         %d\n", g_enc_dst_buf_log_cfg.buff_size);
    socp_crit("socp_logbuffer_cfgshow: log_on_flag          %d\n", g_enc_dst_buf_log_cfg.log_on_flag);
    socp_crit("socp_logbuffer_cfgshow: PhyBufferAddr      0x%lx\n", g_enc_dst_buf_log_cfg.phy_addr);
    socp_crit("socp_logbuffer_cfgshow: virtBufferAddr     0x%lx\n", (uintptr_t)g_enc_dst_buf_log_cfg.virt_addr);
    socp_crit("socp_logbuffer_cfgshow: currentmode        %d\n", g_enc_dst_buf_log_cfg.current_mode);

    return;
}
EXPORT_SYMBOL(bsp_socp_logbuffer_cfgshow);

/*
 * 函 数 名: bsp_deflate_logbuffer_cfgshow
 * 功能描述: 打印延时写入的配置信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void bsp_deflate_logbuffer_cfgshow(void)
{
    socp_crit("deflate_logbuffer_cfgshow: over_time           %d\n", g_deflate_dst_buf_log_cfg.over_time);
    socp_crit("deflate_logbuffer_cfgshow: Current Time Out   %d\n", g_deflate_dst_buf_log_cfg.cur_time_out);
    socp_crit("deflate_logbuffer_cfgshow: buff_size         %d\n", g_deflate_dst_buf_log_cfg.buff_size);
    socp_crit("deflate_logbuffer_cfgshow: log_on_flag          %d\n", g_deflate_dst_buf_log_cfg.log_on_flag);
    socp_crit("deflate_logbuffer_cfgshow: PhyBufferAddr      0x%lx\n", g_deflate_dst_buf_log_cfg.phy_addr);
    socp_crit("deflate_logbuffer_cfgshow: currentmode        %d\n", g_deflate_dst_buf_log_cfg.current_mode);

    return;
}
EXPORT_SYMBOL(bsp_deflate_logbuffer_cfgshow);

/*
 * 函 数 名: socp_logbuffer_early_cfgshow
 * 功能描述: cmdline方式申请内存信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void bsp_socp_logbuffer_early_cfgshow(void)
{
    socp_crit("socp_logbuffer_early_cfgshow: buff_size         %d\n", g_socp_early_cfg.buff_size);
    socp_crit("socp_logbuffer_early_cfgshow: BufUsable          %d\n", g_socp_early_cfg.buff_useable);
    socp_crit("socp_logbuffer_early_cfgshow: PhyBufferAddr      0x%lx\n", g_socp_early_cfg.phy_addr);
    socp_crit("socp_logbuffer_early_cfgshow: time               %d\n", g_socp_early_cfg.timeout);

    return;
}
EXPORT_SYMBOL(bsp_socp_logbuffer_early_cfgshow);

/*
 * 函 数 名: socp_logbuffer_memreseve_cfgshow
 * 功能描述: kernel预留内存配置信息
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: 无
 */
void bsp_socp_logbuffer_memreserve_cfgshow(void)
{
    socp_crit("socp_logbuffer_memreserve_cfgshow: BufferSize         %d\n", g_socp_rsv_mem_info.buff_size);
    socp_crit("socp_logbuffer_memreserve_cfgshow: BufUsable          %d\n", g_socp_rsv_mem_info.buff_useable);
    socp_crit("socp_logbuffer_memreserve_cfgshow: PhyBufferAddr      0x%lx\n", g_socp_rsv_mem_info.phy_addr);
    socp_crit("socp_logbuffer_memreserve_cfgshow: time               %d\n", g_socp_rsv_mem_info.timeout);

    return;
}
EXPORT_SYMBOL(bsp_socp_logbuffer_memreserve_cfgshow);

EXPORT_SYMBOL(bsp_socp_get_log_cfg);

