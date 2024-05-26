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

/*
 * 1 头文件包含
 */
#include "scm_ind_dst.h"
#include <product_config.h>
#include <mdrv_diag_system.h>
#include <soc_socp_adapter.h>
#include <bsp_socp.h>
#include <bsp_slice.h>
#include <bsp_nvim.h>
#include <diag_system_debug.h>
#include <securec.h>
#include "diag_port_manager.h"
#include "diag_system_debug.h"
#include "OmCommonPpm.h"
#include "scm_common.h"
#include "scm_debug.h"
#include <bsp_cpufreq.h>


/*
 * 2 全局变量定义
 */
scm_coder_dst_cfg_s g_scm_ind_coder_dst_cfg = { SCM_CHANNEL_UNINIT,
                                                   SOCP_CODER_DST_OM_IND,
                                                   SCM_CODER_DST_IND_SIZE,
                                                   SCM_CODER_DST_THRESHOLD,
                                                   SOCP_TIMEOUT_TRF_LONG,
                                                   NULL,
                                                   NULL,
                                                   NULL };

extern mdrv_ppm_chan_debug_info_s g_ppm_chan_info;
extern u32 g_diag_log_level;

/*
 * 函 数 名: scm_malloc_ind_dst_buff
 * 功能描述: 编码目的通道memory申请
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: void
 * 调用函数:
 * 被调函数:
 */
u32 scm_malloc_ind_dst_buff(void)
{
    unsigned long phy_addr;

    /* 申请编码目的空间 */
    g_scm_ind_coder_dst_cfg.dst_virt_buff = (u8 *)scm_uncache_mem_alloc(g_scm_ind_coder_dst_cfg.buf_len, &phy_addr);

    /* 申请空间错误 */
    if (NULL == g_scm_ind_coder_dst_cfg.dst_virt_buff) {
        /* 2M申请失败重试1M */
        g_scm_ind_coder_dst_cfg.buf_len = 1 * 1024 * 1024;

        /* 申请编码目的空间 */
        g_scm_ind_coder_dst_cfg.dst_virt_buff = (u8 *)scm_uncache_mem_alloc(g_scm_ind_coder_dst_cfg.buf_len, &phy_addr);
        if (NULL == g_scm_ind_coder_dst_cfg.dst_virt_buff) {
            /* 记录通道初始化标记为内存申请异常 */
            g_scm_ind_coder_dst_cfg.init_state = SCM_CHANNEL_MEM_FAIL;
            return ERR_MSP_FAILURE; /* 返回错误 */
        }
    }

    g_scm_ind_coder_dst_cfg.dst_phy_buff = (u8 *)(uintptr_t)phy_addr;

    return BSP_OK;
}

/*
 * 函 数 名: scm_ind_dst_buff_init
 * 功能描述: 编码目的通道memory初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: void
 * 调用函数:
 * 被调函数:
 */
u32 scm_ind_dst_buff_init(void)
{
    SOCP_ENC_DST_BUF_LOG_CFG_STRU log_cfg;

    memset_s(&log_cfg, sizeof(log_cfg), 0, sizeof(log_cfg));
    if (BSP_OK != bsp_socp_get_sd_logcfg(&log_cfg)) {
        diag_error("No dest channel config from SOCP.\n");

        return scm_malloc_ind_dst_buff();
    }

    /* 未打开log buffer下的处理 */
    if ((SOCP_DST_CHAN_NOT_CFG == log_cfg.logOnFlag) || ((unsigned long)NULL == log_cfg.ulPhyBufferAddr)) {
        diag_error("No dest channel config from SOCP, flag=%d\n", log_cfg.logOnFlag);

        return scm_malloc_ind_dst_buff();
    }

    /* IND通道需要做延迟写入，BUFFER大小50M(默认值)，水线设置为水线设置为75%，内存在初始化已经申请过 */
    g_scm_ind_coder_dst_cfg.dst_phy_buff = (u8 *)(uintptr_t)(log_cfg.ulPhyBufferAddr);
    g_scm_ind_coder_dst_cfg.buf_len = log_cfg.BufferSize;
    g_scm_ind_coder_dst_cfg.dst_virt_buff = log_cfg.pVirBuffer;

    return BSP_OK;
}

/*
 * 函 数 名: SCM_RlsDestBuf
 * 功能描述: 处理目的通道的数据释放
 * 输入参数: ulChanlID 目的通道ID
 *           ulReadSize 数据大小
 * 输出参数: 无
 * 返 回 值: ERR_MSP_FAILURE/BSP_OK
 * 调用函数:
 * 被调函数:
 */
u32 scm_rls_ind_dst_buff(u32 read_size)
{
    u32 data_len;
    SOCP_BUFFER_RW_STRU buffer;
    SOCP_CODER_DST_ENUM_U32 chan_id;

    chan_id = g_scm_ind_coder_dst_cfg.chan_id;

    if (0 == read_size) /* 释放通道所有数据 */
    {
        if (BSP_OK != bsp_socp_get_read_buff(chan_id, &buffer)) {
            diag_error("Get Read Buffer is Error\n");
            return ERR_MSP_FAILURE;
        }

        data_len = buffer.u32Size + buffer.u32RbSize;

        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMCB, data_len);
    } else {
        data_len = read_size;
    }

    if (BSP_OK != bsp_socp_read_data_done(chan_id, data_len)) {
        diag_error("Read Data Done is Error,DataLen=0x%x\n", data_len);

        return ERR_MSP_FAILURE;
    }
    OM_ACPU_DEBUG_CHANNEL_TRACE(chan_id, NULL, data_len, OM_ACPU_READ_DONE, OM_ACPU_DATA);
    return BSP_OK;
}

/*
 * 函 数 名: SCM_CoderDstChannelInit
 * 功能描述: 将ACPU的编码目的通道的配置重置
 * 输入参数: 无
 * 输出参数: 无
 * 返 回 值: ERR_MSP_FAILURE/BSP_OK
 * 调用函数:
 * 被调函数:
 */
u32 scm_ind_dst_channel_init(void)
{
    SOCP_CODER_DEST_CHAN_S chan_info;

    /* 扩大编码目的通道1阈值门限 */
    chan_info.u32EncDstThrh = 2 * SCM_CODER_DST_GTHRESHOLD;

    chan_info.sCoderSetDstBuf.pucOutputStart = g_scm_ind_coder_dst_cfg.dst_phy_buff;

    chan_info.sCoderSetDstBuf.pucOutputEnd = (g_scm_ind_coder_dst_cfg.dst_phy_buff + g_scm_ind_coder_dst_cfg.buf_len) - 1;

    chan_info.sCoderSetDstBuf.u32Threshold = g_scm_ind_coder_dst_cfg.threshold;

    chan_info.u32EncDstTimeoutMode = g_scm_ind_coder_dst_cfg.timeout_mode;

    if (BSP_OK != mdrv_socp_coder_set_dest_chan_attr(g_scm_ind_coder_dst_cfg.chan_id, &chan_info)) {
        g_scm_ind_coder_dst_cfg.init_state = SCM_CHANNEL_CFG_FAIL; /* 记录通道初始化配置错误 */

        return ERR_MSP_FAILURE; /* 返回失败 */
    }

    g_scm_ind_coder_dst_cfg.init_state = SCM_CHANNEL_INIT_SUCC; /* 记录通道初始化配置错误 */

    (void)bsp_socp_register_read_cb(g_scm_ind_coder_dst_cfg.chan_id, (socp_read_cb)scm_ind_dst_read_cb);

    (void)bsp_socp_register_event_cb(g_scm_ind_coder_dst_cfg.chan_id, (socp_event_cb)diag_system_debug_event_cb);

    (void)socp_dst_channel_enable(g_scm_ind_coder_dst_cfg.chan_id);

    return BSP_OK;
}

/*
 * 函 数 名: SCM_SocpSendDataToUDI
 * 功能描述: 用于把数据从SOCP通道的缓冲中发送到指定的端口
 * 输入参数: enChanID:  目的通道号
 *           pucVirData:SOCP通道传递的数据虚拟地址
 *           pucPHYData:SOCP通道传递的数据物理地址
 *           data_len: SOCP通道的数据长度
 * 输出参数: 无
 * 返 回 值: void
 */
void scm_send_ind_data_to_udi(u8 *virt_addr, u8 *phy_addr, u32 len)
{
    u32 send_result;
    u32 ret = ERR_MSP_FAILURE;
    unsigned int phy_port;
    u32 send_len;
    bool send_succ_flag = false;
    bool usb_send_flag = false;
    mdrv_ppm_port_debug_info_s *debug_info = NULL;
    unsigned int logic_port;
    SOCP_CODER_DST_ENUM_U32 chan_id;

    chan_id = g_scm_ind_coder_dst_cfg.chan_id;
    debug_info = &g_ppm_chan_info.ind_debug_info;
    logic_port = CPM_OM_IND_COMM;

    /* 参数检查 */
    if ((0 == len) || (NULL == virt_addr)) {
        debug_info->usb_send_cb_abnormal_num++;
        return;
    }

    ppm_get_send_data_len(chan_id, len, &send_len, &phy_port);

    OM_ACPU_DEBUG_CHANNEL_TRACE(chan_id, virt_addr, send_len, OM_ACPU_SEND_USB_IND, OM_ACPU_DATA);

    diag_system_debug_rev_socp_data(send_len);
    diag_throughput_save(EN_DIAG_THRPUT_DATA_CHN_PHY, send_len);
    send_result = cpm_com_send(logic_port, virt_addr, phy_addr, send_len);
    if (CPM_SEND_ERR == send_result) /* 当前通道已经发送失败，调用SOCP通道无数据搬运 */
    {
        debug_info->usb_send_err_num++;
        debug_info->usb_send_err_len += send_len;
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMWR, send_len);
    } else if (CPM_SEND_FUNC_NULL == send_result) /* 当前通道异常，扔掉所有数据 */
    {
        debug_info->ppm_discard_num++;
        debug_info->ppm_discard_len += len;
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMWR, send_len);
    } else if (CPM_SEND_PARA_ERR == send_result) /* 发送数据获取实地址异常 */
    {
        debug_info->ppm_get_virt_err++;
        debug_info->ppm_get_virt_send_len += len;
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMWR, send_len);
    } else if (CPM_SEND_AYNC == send_result)  // 增加cpm错误码
    {
        send_succ_flag = true;
        usb_send_flag = true;
        ret = BSP_OK;
    } else if (CPM_SEND_OK == send_result) {
        send_succ_flag = true;
    } else {
        debug_info->ppm_discard_num++;
        debug_info->ppm_discard_len += len;
    }

    if (usb_send_flag != true) {
        ret = scm_rls_ind_dst_buff(send_len);

        if (BSP_OK != ret) {
            debug_info->socp_readdone_err_num++;
            debug_info->socp_readdone_err_len += send_len;

            diag_error("SCM_RlsDestBuf return Error 0x%x\n", (s32)ret);
        }
    }

    if ((BSP_OK == ret) && (true == send_succ_flag)) {
        debug_info->usb_send_num++;
        debug_info->usb_send_real_len += send_len;
    }

    return;
}

void scm_reg_ind_coder_dst_send_func(void)
{
    diag_crit("SCM_RegCoderDestIndChan.\n");

    g_scm_ind_coder_dst_cfg.func = (scm_coder_dst_read_cb)scm_send_ind_data_to_udi;
}

void scm_set_power_on_log(void)
{
    shm_power_on_log_info_s poweron_log;
    SOCP_ENC_DST_BUF_LOG_CFG_STRU log_cfg;

    memset_s(&log_cfg, sizeof(log_cfg), 0, sizeof(log_cfg));
    if (BSP_OK == bsp_socp_get_sd_logcfg(&log_cfg)) {
        poweron_log.power_on_log_a = (log_cfg.BufferSize < 50 * 1024 * 1024) ? 0 : 1;
        diag_shared_mem_write(POWER_ON_LOG_A, sizeof(poweron_log.power_on_log_a), &(poweron_log.power_on_log_a));
    }
    return;
}

/*
 * 函 数 名: SCM_CoderDestReadCB
 * 功能描述: 处理编码目的通道的数据
 * 输入参数: ulDstChID 目的通道ID
 * 输出参数: 无
 * 返 回 值: 无
 * 调用函数:
 * 被调函数:
 */
int scm_ind_dst_read_cb(unsigned int u32ChanID)
{
    SOCP_BUFFER_RW_STRU buffer;
    u32 in_slice;
    u32 out_slice;
    unsigned long *virt_addr = NULL;
    u32 dst_chan;

    dst_chan = g_scm_ind_coder_dst_cfg.chan_id;

    if (BSP_OK != bsp_socp_get_read_buff(dst_chan, &buffer)) {
        diag_error("Get Read Buffer is Error\n"); /* 记录Log */
        return ERR_MSP_INVALID_PARAMETER;
    }

    /* 开机log功能，IND通道上报函数为空，使log缓存在本地 */
    if (NULL == g_scm_ind_coder_dst_cfg.func) {
        diag_crit("ind dst channel is null, delay log is open\n");
        return ERR_MSP_SUCCESS;
    }

    if ((0 == (buffer.u32Size + buffer.u32RbSize)) || (NULL == buffer.pBuffer)) {
        bsp_socp_read_data_done(dst_chan, buffer.u32Size + buffer.u32RbSize); /* 清空数据 */
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_BRANCH, buffer.u32Size + buffer.u32RbSize);
        diag_error("Get RD error\n");
        return ERR_MSP_SUCCESS;
    }

    if (0 == buffer.u32Size) {
        return ERR_MSP_SUCCESS;
    }

    /* 发送数据 */
    virt_addr = scm_uncache_mem_phy_to_virt((u8 *)buffer.pBuffer, g_scm_ind_coder_dst_cfg.dst_phy_buff,
                                       g_scm_ind_coder_dst_cfg.dst_virt_buff, g_scm_ind_coder_dst_cfg.buf_len);
    if (virt_addr == NULL) {
        bsp_socp_read_data_done(dst_chan, buffer.u32Size + buffer.u32RbSize); /* 清空数据 */
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_BRANCH, buffer.u32Size + buffer.u32RbSize);
        diag_error("stBuffer.pBuffe==NULL\n");
        return ERR_MSP_MALLOC_FAILUE;
    }
    in_slice = bsp_get_slice_value();
    g_scm_ind_coder_dst_cfg.func((u8 *)virt_addr, (u8 *)buffer.pBuffer, (u32)buffer.u32Size);
    out_slice = bsp_get_slice_value();
    /* 记录回调函数的执行时间 */
    if (g_diag_log_level) {
        diag_crit("g_scm_cnf_coder_dst_cfg.func Proc time 0x%x\n", (out_slice - in_slice));
    }

    return ERR_MSP_SUCCESS;
}

u32 scm_ind_get_dst_buff_size(void)
{
    return g_scm_ind_coder_dst_cfg.buf_len;
}
