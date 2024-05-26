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
 * 1 ͷ�ļ�����
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
 * 2 ȫ�ֱ�������
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
 * �� �� ��: scm_malloc_ind_dst_buff
 * ��������: ����Ŀ��ͨ��memory����
 * �������: ��
 * �������: ��
 * �� �� ֵ: void
 * ���ú���:
 * ��������:
 */
u32 scm_malloc_ind_dst_buff(void)
{
    unsigned long phy_addr;

    /* �������Ŀ�Ŀռ� */
    g_scm_ind_coder_dst_cfg.dst_virt_buff = (u8 *)scm_uncache_mem_alloc(g_scm_ind_coder_dst_cfg.buf_len, &phy_addr);

    /* ����ռ���� */
    if (NULL == g_scm_ind_coder_dst_cfg.dst_virt_buff) {
        /* 2M����ʧ������1M */
        g_scm_ind_coder_dst_cfg.buf_len = 1 * 1024 * 1024;

        /* �������Ŀ�Ŀռ� */
        g_scm_ind_coder_dst_cfg.dst_virt_buff = (u8 *)scm_uncache_mem_alloc(g_scm_ind_coder_dst_cfg.buf_len, &phy_addr);
        if (NULL == g_scm_ind_coder_dst_cfg.dst_virt_buff) {
            /* ��¼ͨ����ʼ�����Ϊ�ڴ������쳣 */
            g_scm_ind_coder_dst_cfg.init_state = SCM_CHANNEL_MEM_FAIL;
            return ERR_MSP_FAILURE; /* ���ش��� */
        }
    }

    g_scm_ind_coder_dst_cfg.dst_phy_buff = (u8 *)(uintptr_t)phy_addr;

    return BSP_OK;
}

/*
 * �� �� ��: scm_ind_dst_buff_init
 * ��������: ����Ŀ��ͨ��memory��ʼ��
 * �������: ��
 * �������: ��
 * �� �� ֵ: void
 * ���ú���:
 * ��������:
 */
u32 scm_ind_dst_buff_init(void)
{
    SOCP_ENC_DST_BUF_LOG_CFG_STRU log_cfg;

    memset_s(&log_cfg, sizeof(log_cfg), 0, sizeof(log_cfg));
    if (BSP_OK != bsp_socp_get_sd_logcfg(&log_cfg)) {
        diag_error("No dest channel config from SOCP.\n");

        return scm_malloc_ind_dst_buff();
    }

    /* δ��log buffer�µĴ��� */
    if ((SOCP_DST_CHAN_NOT_CFG == log_cfg.logOnFlag) || ((unsigned long)NULL == log_cfg.ulPhyBufferAddr)) {
        diag_error("No dest channel config from SOCP, flag=%d\n", log_cfg.logOnFlag);

        return scm_malloc_ind_dst_buff();
    }

    /* INDͨ����Ҫ���ӳ�д�룬BUFFER��С50M(Ĭ��ֵ)��ˮ������Ϊˮ������Ϊ75%���ڴ��ڳ�ʼ���Ѿ������ */
    g_scm_ind_coder_dst_cfg.dst_phy_buff = (u8 *)(uintptr_t)(log_cfg.ulPhyBufferAddr);
    g_scm_ind_coder_dst_cfg.buf_len = log_cfg.BufferSize;
    g_scm_ind_coder_dst_cfg.dst_virt_buff = log_cfg.pVirBuffer;

    return BSP_OK;
}

/*
 * �� �� ��: SCM_RlsDestBuf
 * ��������: ����Ŀ��ͨ���������ͷ�
 * �������: ulChanlID Ŀ��ͨ��ID
 *           ulReadSize ���ݴ�С
 * �������: ��
 * �� �� ֵ: ERR_MSP_FAILURE/BSP_OK
 * ���ú���:
 * ��������:
 */
u32 scm_rls_ind_dst_buff(u32 read_size)
{
    u32 data_len;
    SOCP_BUFFER_RW_STRU buffer;
    SOCP_CODER_DST_ENUM_U32 chan_id;

    chan_id = g_scm_ind_coder_dst_cfg.chan_id;

    if (0 == read_size) /* �ͷ�ͨ���������� */
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
 * �� �� ��: SCM_CoderDstChannelInit
 * ��������: ��ACPU�ı���Ŀ��ͨ������������
 * �������: ��
 * �������: ��
 * �� �� ֵ: ERR_MSP_FAILURE/BSP_OK
 * ���ú���:
 * ��������:
 */
u32 scm_ind_dst_channel_init(void)
{
    SOCP_CODER_DEST_CHAN_S chan_info;

    /* �������Ŀ��ͨ��1��ֵ���� */
    chan_info.u32EncDstThrh = 2 * SCM_CODER_DST_GTHRESHOLD;

    chan_info.sCoderSetDstBuf.pucOutputStart = g_scm_ind_coder_dst_cfg.dst_phy_buff;

    chan_info.sCoderSetDstBuf.pucOutputEnd = (g_scm_ind_coder_dst_cfg.dst_phy_buff + g_scm_ind_coder_dst_cfg.buf_len) - 1;

    chan_info.sCoderSetDstBuf.u32Threshold = g_scm_ind_coder_dst_cfg.threshold;

    chan_info.u32EncDstTimeoutMode = g_scm_ind_coder_dst_cfg.timeout_mode;

    if (BSP_OK != mdrv_socp_coder_set_dest_chan_attr(g_scm_ind_coder_dst_cfg.chan_id, &chan_info)) {
        g_scm_ind_coder_dst_cfg.init_state = SCM_CHANNEL_CFG_FAIL; /* ��¼ͨ����ʼ�����ô��� */

        return ERR_MSP_FAILURE; /* ����ʧ�� */
    }

    g_scm_ind_coder_dst_cfg.init_state = SCM_CHANNEL_INIT_SUCC; /* ��¼ͨ����ʼ�����ô��� */

    (void)bsp_socp_register_read_cb(g_scm_ind_coder_dst_cfg.chan_id, (socp_read_cb)scm_ind_dst_read_cb);

    (void)bsp_socp_register_event_cb(g_scm_ind_coder_dst_cfg.chan_id, (socp_event_cb)diag_system_debug_event_cb);

    (void)socp_dst_channel_enable(g_scm_ind_coder_dst_cfg.chan_id);

    return BSP_OK;
}

/*
 * �� �� ��: SCM_SocpSendDataToUDI
 * ��������: ���ڰ����ݴ�SOCPͨ���Ļ����з��͵�ָ���Ķ˿�
 * �������: enChanID:  Ŀ��ͨ����
 *           pucVirData:SOCPͨ�����ݵ����������ַ
 *           pucPHYData:SOCPͨ�����ݵ����������ַ
 *           data_len: SOCPͨ�������ݳ���
 * �������: ��
 * �� �� ֵ: void
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

    /* ������� */
    if ((0 == len) || (NULL == virt_addr)) {
        debug_info->usb_send_cb_abnormal_num++;
        return;
    }

    ppm_get_send_data_len(chan_id, len, &send_len, &phy_port);

    OM_ACPU_DEBUG_CHANNEL_TRACE(chan_id, virt_addr, send_len, OM_ACPU_SEND_USB_IND, OM_ACPU_DATA);

    diag_system_debug_rev_socp_data(send_len);
    diag_throughput_save(EN_DIAG_THRPUT_DATA_CHN_PHY, send_len);
    send_result = cpm_com_send(logic_port, virt_addr, phy_addr, send_len);
    if (CPM_SEND_ERR == send_result) /* ��ǰͨ���Ѿ�����ʧ�ܣ�����SOCPͨ�������ݰ��� */
    {
        debug_info->usb_send_err_num++;
        debug_info->usb_send_err_len += send_len;
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMWR, send_len);
    } else if (CPM_SEND_FUNC_NULL == send_result) /* ��ǰͨ���쳣���ӵ��������� */
    {
        debug_info->ppm_discard_num++;
        debug_info->ppm_discard_len += len;
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMWR, send_len);
    } else if (CPM_SEND_PARA_ERR == send_result) /* �������ݻ�ȡʵ��ַ�쳣 */
    {
        debug_info->ppm_get_virt_err++;
        debug_info->ppm_get_virt_send_len += len;
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_CPMWR, send_len);
    } else if (CPM_SEND_AYNC == send_result)  // ����cpm������
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
 * �� �� ��: SCM_CoderDestReadCB
 * ��������: �������Ŀ��ͨ��������
 * �������: ulDstChID Ŀ��ͨ��ID
 * �������: ��
 * �� �� ֵ: ��
 * ���ú���:
 * ��������:
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
        diag_error("Get Read Buffer is Error\n"); /* ��¼Log */
        return ERR_MSP_INVALID_PARAMETER;
    }

    /* ����log���ܣ�INDͨ���ϱ�����Ϊ�գ�ʹlog�����ڱ��� */
    if (NULL == g_scm_ind_coder_dst_cfg.func) {
        diag_crit("ind dst channel is null, delay log is open\n");
        return ERR_MSP_SUCCESS;
    }

    if ((0 == (buffer.u32Size + buffer.u32RbSize)) || (NULL == buffer.pBuffer)) {
        bsp_socp_read_data_done(dst_chan, buffer.u32Size + buffer.u32RbSize); /* ������� */
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_BRANCH, buffer.u32Size + buffer.u32RbSize);
        diag_error("Get RD error\n");
        return ERR_MSP_SUCCESS;
    }

    if (0 == buffer.u32Size) {
        return ERR_MSP_SUCCESS;
    }

    /* �������� */
    virt_addr = scm_uncache_mem_phy_to_virt((u8 *)buffer.pBuffer, g_scm_ind_coder_dst_cfg.dst_phy_buff,
                                       g_scm_ind_coder_dst_cfg.dst_virt_buff, g_scm_ind_coder_dst_cfg.buf_len);
    if (virt_addr == NULL) {
        bsp_socp_read_data_done(dst_chan, buffer.u32Size + buffer.u32RbSize); /* ������� */
        diag_system_debug_ind_dst_lost(EN_DIAG_DST_LOST_BRANCH, buffer.u32Size + buffer.u32RbSize);
        diag_error("stBuffer.pBuffe==NULL\n");
        return ERR_MSP_MALLOC_FAILUE;
    }
    in_slice = bsp_get_slice_value();
    g_scm_ind_coder_dst_cfg.func((u8 *)virt_addr, (u8 *)buffer.pBuffer, (u32)buffer.u32Size);
    out_slice = bsp_get_slice_value();
    /* ��¼�ص�������ִ��ʱ�� */
    if (g_diag_log_level) {
        diag_crit("g_scm_cnf_coder_dst_cfg.func Proc time 0x%x\n", (out_slice - in_slice));
    }

    return ERR_MSP_SUCCESS;
}

u32 scm_ind_get_dst_buff_size(void)
{
    return g_scm_ind_coder_dst_cfg.buf_len;
}
