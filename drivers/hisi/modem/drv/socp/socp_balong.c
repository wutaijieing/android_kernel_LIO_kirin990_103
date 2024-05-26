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

#include "product_config.h"
#include <linux/version.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/vmalloc.h>
#include <osl_thread.h>
#include "socp_balong.h"
#include <linux/clk.h>
#include "bsp_version.h"
#include "bsp_dump.h"
#include "bsp_nvim.h"
/* log2.0 2014-03-19 Begin: */
#include "acore_nv_stru_drv.h"
#include "bsp_print.h"
#include "bsp_slice.h"
#include "bsp_softtimer.h"
#include "socp_ind_delay.h"
#ifdef CONFIG_DEFLATE
#include "deflate.h"
#endif
/* log2.0 2014-03-19 End */

#include <securec.h>


socp_gbl_state_s g_socp_stat = {0};
EXPORT_SYMBOL(g_socp_stat);
extern socp_debug_info_s g_socp_debug_info;
EXPORT_SYMBOL(g_socp_debug_info);

struct socp_enc_dst_log_cfg *g_socp_log_config = NULL;
/* �������ȼ����� */
#define SOCP_ENCSRC_TASK_PRO 79
#define SOCP_ENCDST_TASK_PRO 81
#define SOCP_DECSRC_TASK_PRO 79
#define SOCP_DECDST_TASK_PRO 81
/* SOCP����ַ */
u32 g_socp_reg_base_addr = 0;
/* �жϴ����� */
irqreturn_t socp_app_int_handler(int irq, void *dev_info);

spinlock_t lock;

u32 g_throwout = 0;
u32 g_socp_enable_state[SOCP_MAX_ENCDST_CHN] = {0}; /* socpĿ�Ķ������ϱ�����״̬ */
u32 g_socp_version = 0;
u32 g_socp_debug_trace_cfg = 0;
u32 g_deflate_status = 0;
u32 g_socp_enc_dst_threshold_ovf = 0;
u32 g_socp_enc_dst_isr_trf_int_cnt = 0;
#define SOCP_MAX_ENC_DST_COUNT 100
struct socp_enc_dst_stat_s {
    u32 int_start_slice;
    u32 int_end_slice;
    u32 task_start_slice;
    u32 task_end_slice;
    u32 read_done_start_slice;
    u32 read_done_end_slice;
};
u32 g_enc_dst_stat_cnt;

struct socp_enc_dst_stat_s g_enc_dst_sta[SOCP_MAX_ENC_DST_COUNT];

extern struct socp_enc_dst_log_cfg g_enc_dst_buf_log_cfg;

/* ����Ƿ��ʼ�� */
s32 socp_check_init(void)
{
    if (!g_socp_stat.init_flag) {
        socp_error("The module has not been initialized!\n");
        return BSP_ERR_SOCP_NOT_INIT;
    } else {
        return BSP_OK;
    }
}

/* ����ַ��ʼ������Ч�� */
s32 socp_check_buf_addr(unsigned long start, unsigned long end)
{
    if (start >= end) {
        socp_error("The buff is invalid!\n");
        return BSP_ERR_SOCP_INVALID_PARA;
    } else {
        return BSP_OK;
    }
}

/* ���ͨ������ */
s32 socp_check_chan_type(u32 para, u32 type)
{
    if (type != para) {
        socp_error("The channel type is invalid!\n");
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

/* ���ͨ��ID�Ƿ���Ч */
s32 socp_check_chan_id(u32 para, u32 id)
{
    if (para >= id) {
        socp_error("The channel id is invalid!\n");
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

/* ������Դͨ��ID��Ч�� */
s32 socp_check_encsrc_chan_id(u32 id)
{
    if ((id >= SOCP_CCORE_ENCSRC_CHN_BASE) && (id < (SOCP_CCORE_ENCSRC_CHN_BASE + SOCP_CCORE_ENCSRC_CHN_NUM))) {
        socp_error("The src channel id is invalid!\n");
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

/* ����Ƿ�8�ֽڶ��� */
s32 socp_check_8bytes_align(unsigned long para)
{
    if (0 != (para % 8)) {
        socp_error("The parameter is not 8 bytes aligned!\n");
        return BSP_ERR_SOCP_NOT_8BYTESALIGN;
    } else {
        return BSP_OK;
    }
}

/* ���ͨ�����ȼ���Ч�� */
s32 socp_check_chan_priority(u32 para)
{
    if (para >= SOCP_CHAN_PRIORITY_BUTT) {
        socp_error("The src channele priority[%d] is valid!\n", para);
        return BSP_ERR_SOCP_INVALID_PARA;
    } else {
        return BSP_OK;
    }
}

/* ���socp�������� */
s32 socp_check_data_type(u32 para)
{
    if (para >= SOCP_DATA_TYPE_BUTT) {
        socp_error("channel data type[%d] is invalid!\n", para);
        return BSP_ERR_SOCP_INVALID_PARA;
    } else {
        return BSP_OK;
    }
}

s32 socp_check_encsrc_alloc(u32 id)
{
    if (SOCP_CHN_ALLOCATED != g_socp_stat.enc_src_chan[id].alloc_state) {
        socp_error("encoder src[%d] is not allocated!\n", id);
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

s32 socp_check_encdst_set(u32 id)
{
    if (SOCP_CHN_SET != g_socp_stat.enc_dst_chan[id].set_state) {
        socp_error("encdst channel[%d] set failed!\n", id);
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

s32 socp_check_decsrc_set(u32 id)
{
    if (SOCP_CHN_SET != g_socp_stat.dec_src_chan[id].set_state) {
        socp_error("decsrc channel[%d] set failed!\n", id);
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

s32 socp_check_decdst_alloc(u32 id)
{
    if (SOCP_CHN_ALLOCATED != g_socp_stat.dec_dst_chan[id].alloc_state) {
        socp_error("decdst channel[%d] is alloc failed!\n", id);
        return BSP_ERR_SOCP_INVALID_CHAN;
    } else {
        return BSP_OK;
    }
}

s32 socp_check_data_type_en(u32 param)
{
    if (param >= SOCP_DATA_TYPE_EN_BUTT) {
        socp_error("the data type en is valid, para is %d!\n", param);
        return BSP_ERR_SOCP_INVALID_PARA;
    } else {
        return BSP_OK;
    }
}

s32 socp_check_enc_debug_en(u32 param)
{
    if (param >= SOCP_ENC_DEBUG_EN_BUTT) {
        socp_error("the enc src debug en is invalid, para is %d!\n", param);
        return BSP_ERR_SOCP_INVALID_PARA;
    } else {
        return BSP_OK;
    }
}

void socp_global_reset(void)
{
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 0, 1, 0x1);
    while (SOCP_REG_GETBITS(SOCP_REG_GBLRST, 0, 1) != 0) {
        socp_error("socp is soft resetting\n");
    }
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 1, 1, 0x1);
}

void socp_encsrc_headerr_irq_enable(void)
{
    // enable encode head error of all channels.
    SOCP_REG_WRITE(SOCP_REG_APP_MASK1, 0);
}

void socp_global_ctrl_init(void)
{
    unsigned int i;
    s32 ret = 0;

    /* ��ʼ��ȫ��״̬�ṹ�� */
    /* ����ID��ʼ�� */
    g_socp_stat.u32EncSrcTskID = 0;
    g_socp_stat.dec_dst_task_id = 0;
    g_socp_stat.u32EncDstTskID = 0;
    g_socp_stat.dec_src_task_id = 0;
    /* ��ͷ�����־��ʼ�� */
    g_socp_stat.int_enc_src_header = 0;
    g_socp_stat.u32IntEncSrcRD = 0;
    g_socp_stat.int_dec_dst_trf = 0;
    g_socp_stat.int_dec_dst_ovf = 0;
    g_socp_stat.int_enc_dst_tfr = 0;
    g_socp_stat.int_enc_dst_ovf = 0;
    g_socp_stat.int_dec_src_err = 0;
    g_socp_stat.compress_isr = NULL;

    for (i = 0; i < SOCP_MAX_ENCSRC_CHN; i++) {
        g_socp_stat.enc_src_chan[i].chan_id = i;
        g_socp_stat.enc_src_chan[i].chan_en = SOCP_CHN_DISABLE;
        g_socp_stat.enc_src_chan[i].alloc_state = SOCP_CHN_UNALLOCATED;
        g_socp_stat.enc_src_chan[i].last_rd_size = 0;
        g_socp_stat.enc_src_chan[i].dst_chan_id = 0xff;
        g_socp_stat.enc_src_chan[i].bypass_en = 0;
        g_socp_stat.enc_src_chan[i].priority = SOCP_CHAN_PRIORITY_3;
        g_socp_stat.enc_src_chan[i].data_type = SOCP_DATA_TYPE_BUTT;
        g_socp_stat.enc_src_chan[i].data_type_en = SOCP_DATA_TYPE_EN_BUTT;
        g_socp_stat.enc_src_chan[i].debug_en = SOCP_ENC_DEBUG_EN_BUTT;
        g_socp_stat.enc_src_chan[i].event_cb = BSP_NULL;
        g_socp_stat.enc_src_chan[i].rd_cb = BSP_NULL;
    }

    for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
        g_socp_stat.enc_dst_chan[i].chan_id = i;
        g_socp_stat.enc_dst_chan[i].threshold = 0;
        g_socp_stat.enc_dst_chan[i].set_state = SOCP_CHN_UNSET;
        g_socp_stat.enc_dst_chan[i].event_cb = BSP_NULL;
        g_socp_stat.enc_dst_chan[i].read_cb = BSP_NULL;
        g_socp_stat.enc_dst_chan[i].chan_event = (SOCP_EVENT_ENUM_UIN32)0;
        g_socp_stat.enc_dst_chan[i].struCompress.bcompress = SOCP_NO_COMPRESS;

        ret = memset_s(&g_socp_stat.enc_dst_chan[i].struCompress.ops,
                       sizeof(g_socp_stat.enc_dst_chan[i].struCompress.ops), 0x0,
                       sizeof(g_socp_stat.enc_dst_chan[i].struCompress.ops));
        if (ret) {
            return;
        }
    }

    for (i = 0; i < SOCP_MAX_DECSRC_CHN; i++) {
        g_socp_stat.dec_src_chan[i].chan_id = i;
        g_socp_stat.dec_src_chan[i].chan_en = SOCP_CHN_DISABLE;
        g_socp_stat.dec_src_chan[i].data_type_en = SOCP_DATA_TYPE_EN_BUTT;
        g_socp_stat.dec_src_chan[i].set_state = SOCP_CHN_UNSET;
        g_socp_stat.dec_src_chan[i].event_cb = BSP_NULL;
        g_socp_stat.dec_src_chan[i].rd_cb = BSP_NULL;
    }

    for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
        g_socp_stat.dec_dst_chan[i].chan_id = i;
        g_socp_stat.dec_dst_chan[i].alloc_state = SOCP_CHN_UNALLOCATED;
        g_socp_stat.dec_dst_chan[i].data_type = SOCP_DATA_TYPE_BUTT;
        g_socp_stat.dec_dst_chan[i].event_cb = BSP_NULL;
        g_socp_stat.dec_dst_chan[i].read_cb = BSP_NULL;
    }

    return;
}

s32 socp_clk_enable(void)
{
#ifndef BSP_CONFIG_PHONE_TYPE
    struct clk *cSocp;

    /* ��SOCPʱ�� */
    cSocp = clk_get(NULL, "socp_clk");
    clk_prepare(cSocp);
    if (BSP_OK != clk_enable(cSocp)) {
        socp_error("[init] Socp clk open failed.\n");
        return BSP_ERROR;
    }
#endif

    return BSP_OK;
}

/*
 * �� �� ��: socp_get_idle_buffer
 * ��������: ��ѯ���л�����
 * �������:  ring_buffer       ����ѯ�Ļ���buffer
 *                 p_rw_buff         ����Ļ���buffer
 * �������: ��
 * �� �� ֵ:  ��
 */
void socp_get_idle_buffer(socp_ring_buff_s *ring_buffer, SOCP_BUFFER_RW_STRU *p_rw_buff)
{
#ifdef FEATURE_SOCP_ADDR_64BITS
    if (ring_buffer->write < ring_buffer->read) {
        /* ��ָ�����дָ�룬ֱ�Ӽ��� */
        p_rw_buff->pBuffer = (char *)(uintptr_t)(ring_buffer->Start + (u32)ring_buffer->write);
        p_rw_buff->u32Size = (u32)(ring_buffer->read - ring_buffer->write - 1);
        p_rw_buff->pRbBuffer = (char *)BSP_NULL;
        p_rw_buff->u32RbSize = 0;
    } else {
        /* дָ����ڶ�ָ�룬��Ҫ���ǻؾ� */
        if (ring_buffer->read != 0) {
            p_rw_buff->pBuffer = (char *)((uintptr_t)ring_buffer->Start + (u32)ring_buffer->write);
            p_rw_buff->u32Size = (u32)(ring_buffer->End - (ring_buffer->Start + ring_buffer->write) + 1);
            p_rw_buff->pRbBuffer = (char *)(uintptr_t)ring_buffer->Start;
            p_rw_buff->u32RbSize = ring_buffer->read - 1;
        } else {
            p_rw_buff->pBuffer = (char *)((uintptr_t)ring_buffer->Start + (u32)ring_buffer->write);
            p_rw_buff->u32Size = (u32)(ring_buffer->End - (ring_buffer->Start + ring_buffer->write));
            p_rw_buff->pRbBuffer = (char *)BSP_NULL;
            p_rw_buff->u32RbSize = 0;
        }
    }
#else
    if (ring_buffer->write < ring_buffer->read) {
        /* ��ָ�����дָ�룬ֱ�Ӽ��� */
        p_rw_buff->pBuffer = (char *)((unsigned long)ring_buffer->write);
        p_rw_buff->u32Size = (u32)(ring_buffer->read - ring_buffer->write - 1);
        p_rw_buff->pRbBuffer = (char *)BSP_NULL;
        p_rw_buff->u32RbSize = 0;
    } else {
        /* дָ����ڶ�ָ�룬��Ҫ���ǻؾ� */
        if (ring_buffer->read != (u32)ring_buffer->Start) {
            p_rw_buff->pBuffer = (char *)((unsigned long)ring_buffer->write);
            p_rw_buff->u32Size = (u32)(ring_buffer->End - ring_buffer->write + 1);
            p_rw_buff->pRbBuffer = (char *)ring_buffer->Start;
            p_rw_buff->u32RbSize = (u32)(ring_buffer->read - ring_buffer->Start - 1);
        } else {
            p_rw_buff->pBuffer = (char *)((unsigned long)ring_buffer->write);
            p_rw_buff->u32Size = (u32)(ring_buffer->End - ring_buffer->write);
            p_rw_buff->pRbBuffer = (char *)BSP_NULL;
            p_rw_buff->u32RbSize = 0;
        }
    }
#endif

    return;
}

/*
 * �� �� ��: socp_get_data_buffer
 * ��������: ��ȡ���л�����������
 * �������:  ring_buffer       ����ѯ�Ļ���buffer
 *                 p_rw_buff         ����Ļ���buffer
 * �������: ��
 * �� �� ֵ:  ��
 */
void socp_get_data_buffer(socp_ring_buff_s *ring_buffer, SOCP_BUFFER_RW_STRU *p_rw_buff)
{
#ifdef FEATURE_SOCP_ADDR_64BITS
    if (ring_buffer->read <= ring_buffer->write) {
        /* дָ����ڶ�ָ�룬ֱ�Ӽ��� */
        p_rw_buff->pBuffer = (char *)(uintptr_t)((uintptr_t)ring_buffer->Start + (u32)ring_buffer->read);
        p_rw_buff->u32Size = (u32)(ring_buffer->write - ring_buffer->read);
        p_rw_buff->pRbBuffer = (char *)BSP_NULL;
        p_rw_buff->u32RbSize = 0;
    } else {
        /* ��ָ�����дָ�룬��Ҫ���ǻؾ� */
        p_rw_buff->pBuffer = (char *)((uintptr_t)ring_buffer->Start + (u32)ring_buffer->read);
        p_rw_buff->u32Size =
            (uintptr_t)((uintptr_t)ring_buffer->End - ((unsigned long)ring_buffer->Start + ring_buffer->read) + 1);
        p_rw_buff->pRbBuffer = (char *)(uintptr_t)ring_buffer->Start;
        p_rw_buff->u32RbSize = ring_buffer->write;
    }
#else
    if (ring_buffer->read <= ring_buffer->write) {
        /* дָ����ڶ�ָ�룬ֱ�Ӽ��� */
        p_rw_buff->pBuffer = (char *)((unsigned long)ring_buffer->read);
        p_rw_buff->u32Size = (u32)(ring_buffer->write - ring_buffer->read);
        p_rw_buff->pRbBuffer = (char *)BSP_NULL;
        p_rw_buff->u32RbSize = 0;
    } else {
        /* ��ָ�����дָ�룬��Ҫ���ǻؾ� */
        p_rw_buff->pBuffer = (char *)((unsigned long)ring_buffer->read);
        p_rw_buff->u32Size = (u32)(ring_buffer->End - ring_buffer->read + 1);
        p_rw_buff->pRbBuffer = (char *)ring_buffer->Start;
        p_rw_buff->u32RbSize = (u32)(ring_buffer->write - ring_buffer->Start);
    }
#endif
    return;
}

/*
 * �� �� ��: socp_write_done
 * ��������: ���»�������дָ��
 * �������:  ring_buffer       �����µĻ���buffer
 *                 size          ���µ����ݳ���
 * �������: ��
 * �� �� ֵ:  ��
 */
void socp_write_done(socp_ring_buff_s *ring_buffer, u32 size)
{
    u32 tmp_size;
#ifdef FEATURE_SOCP_ADDR_64BITS
    // ring_buffer->write = (ring_buffer->write+size) % ring_buffer->length;

    tmp_size = (u32)(ring_buffer->End - (ring_buffer->Start + ring_buffer->write) + 1);
    if (tmp_size > size) {
        ring_buffer->write += size;
    } else {
        u32 rb_size = size - tmp_size;
        ring_buffer->write = rb_size;
    }

#else

    tmp_size = (u32)(ring_buffer->End - ring_buffer->write + 1);
    if (tmp_size > size) {
        ring_buffer->write += size;
    } else {
        u32 rb_size = size - tmp_size;
        ring_buffer->write = (u32)(ring_buffer->Start + rb_size);
    }
#endif
    return;
}
/*
 * �� �� ��: socp_read_done
 * ��������: ���»������Ķ�ָ��
 * �������:  ring_buffer       �����µĻ���buffer
 *                 size          ���µ����ݳ���
 * �������: ��
 * �� �� ֵ:  ��
 */
void socp_read_done(socp_ring_buff_s *ring_buffer, u32 size)
{
#ifdef FEATURE_SOCP_ADDR_64BITS
    ring_buffer->read += size;
    if (ring_buffer->read > (u32)(ring_buffer->End - ring_buffer->Start)) {
        ring_buffer->read -= ring_buffer->length;
    }
#else
    ring_buffer->read += size;
    if (ring_buffer->read > ring_buffer->End) {
        ring_buffer->read -= ring_buffer->length;
    }
#endif
}

/*
 * �� �� ��: bsp_socp_clean_encsrc_chan
 * ��������: ��ձ���Դͨ����ͬ��V9 SOCP�ӿ�
 * �������: src_chan_id       ����ͨ����
 * �������: ��
 * �� �� ֵ: BSP_OK
 */
u32 bsp_socp_clean_encsrc_chan(u32 src_chan_id)
{
    u32 reset_flag;
    u32 i;
    u32 chan_id;
    u32 chan_type;
    u32 ret;

    chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    if ((ret = (u32)socp_check_chan_type(chan_type, SOCP_CODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }

    if ((ret = (u32)socp_check_encsrc_chan_id(chan_id)) != BSP_OK) {
        return ret;
    }

    /* ��λͨ�� */
    SOCP_REG_SETBITS(SOCP_REG_ENCRST, chan_id, 1, 1);

    /* �ȴ�ͨ������ */
    for (i = 0; i < SOCP_RESET_TIME; i++) {
        reset_flag = SOCP_REG_GETBITS(SOCP_REG_ENCRST, chan_id, 1);
        if (0 == reset_flag) {
            break;
        }
    }

    if (SOCP_RESET_TIME == i) {
        socp_error("Socp src channel[%d] clean failed!\n", chan_id);
    }

    return BSP_OK;
}

/*
 * �� �� ��: socp_reset_chan_reg_wr_addr
 * ��������: socp��λͨ����ַ�Ͷ�дָ��Ĵ������ڲ�����socp32λ��64λѰַ
 * �������: chan_id: ͨ���ţ�����ͨ�����ͺ�ͨ��ID
 *           type: ͨ�����ͣ����ֱ���ͨ���ͽ���ͨ��
 *           enc_chan: ����ͨ�������ṹ��
 *           dec_chan: ����ͨ�������ṹ��
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_reset_chan_reg_wr_addr(u32 chan_id, u32 Type, socp_enc_src_chan_s *enc_chan, socp_decsrc_chan_s *dec_chan)
{
#ifdef FEATURE_SOCP_ADDR_64BITS       // SOCP 64λѰַ
    if (Type == SOCP_CODER_SRC_CHAN)  // ����ͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFADDR_L(chan_id), (u32)enc_chan->enc_src_buff.Start);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFADDR_H(chan_id), (u32)(((u64)enc_chan->enc_src_buff.Start) >> 32));
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFWPTR(chan_id), 0);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFRPTR(chan_id), 0);
        /* ���¶�дָ�� */
        g_socp_stat.enc_src_chan[chan_id].enc_src_buff.read = 0;
        g_socp_stat.enc_src_chan[chan_id].enc_src_buff.write = 0;

        /* �����������������������RDbuffer����ʼ��ַ�ͳ��� */
        if (SOCP_ENCSRC_CHNMODE_LIST == enc_chan->chan_mode) {
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQADDR_L(chan_id), (u32)enc_chan->rd_buff.Start);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQADDR_H(chan_id), (u32)(((u64)enc_chan->rd_buff.Start) >> 32));
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQRPTR(chan_id), 0);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQWPTR(chan_id), 0);
            /*lint -save -e647*/
            SOCP_REG_SETBITS(SOCP_REG_ENCSRC_RDQCFG(chan_id), 0, 16, enc_chan->rd_buff.length);
            SOCP_REG_SETBITS(SOCP_REG_ENCSRC_RDQCFG(chan_id), 16, 16, 0);
            /*lint -restore +e647*/
            /* ���¶�дָ�� */
            g_socp_stat.enc_src_chan[chan_id].rd_buff.read = 0;
            g_socp_stat.enc_src_chan[chan_id].rd_buff.write = 0;
        }
    } else if (Type == SOCP_DECODER_SRC_CHAN)  // ����ͨ��
    {
        /* ʹ�����ò����������� */
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR_L(chan_id), (u32)dec_chan->dec_src_buff.Start);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR_H(chan_id), (u32)(((u64)dec_chan->dec_src_buff.Start) >> 32));
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(chan_id), 0);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFRPTR(chan_id), 0);
        /* ���¶�Ӧͨ���Ķ�дָ�� */
        g_socp_stat.dec_src_chan[chan_id].dec_src_buff.read = 0;
        g_socp_stat.dec_src_chan[chan_id].dec_src_buff.write = 0;
    }

#else  // SOCP 32λѰַ

    if (Type == SOCP_CODER_SRC_CHAN)  // ����ͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFADDR(chan_id), (u32)enc_chan->enc_src_buff.Start);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFWPTR(chan_id), (u32)enc_chan->enc_src_buff.Start);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFRPTR(chan_id), (u32)enc_chan->enc_src_buff.Start);
        /* ���¶�дָ�� */
        g_socp_stat.enc_src_chan[chan_id].enc_src_buff.read = (u32)(enc_chan->enc_src_buff.Start);
        g_socp_stat.enc_src_chan[chan_id].enc_src_buff.write = (u32)(enc_chan->enc_src_buff.Start);

        /* �����������������������RDbuffer����ʼ��ַ�ͳ��� */
        if (SOCP_ENCSRC_CHNMODE_LIST == enc_chan->chan_mode) {
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQADDR(chan_id), (u32)enc_chan->rd_buff.Start);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQRPTR(chan_id), (u32)enc_chan->rd_buff.Start);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQWPTR(chan_id), (u32)enc_chan->rd_buff.Start);
            /*lint -save -e647*/
            SOCP_REG_SETBITS(SOCP_REG_ENCSRC_RDQCFG(chan_id), 0, 16, enc_chan->rd_buff.length);
            SOCP_REG_SETBITS(SOCP_REG_ENCSRC_RDQCFG(chan_id), 16, 16, 0);
            /*lint -restore +e647*/
            g_socp_stat.enc_src_chan[chan_id].rd_buff.read = (u32)(enc_chan->rd_buff.Start);
            g_socp_stat.enc_src_chan[chan_id].rd_buff.write = (u32)(enc_chan->rd_buff.Start);
        }
    } else if (Type == SOCP_DECODER_SRC_CHAN)  // ����ͨ��
    {
        /* ʹ�����ò����������� */
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR(chan_id), (u32)dec_chan->dec_src_buff.Start);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(chan_id), (u32)dec_chan->dec_src_buff.Start);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFRPTR(chan_id), (u32)dec_chan->dec_src_buff.Start);
        /* ���¶�Ӧͨ���Ķ�дָ�� */
        g_socp_stat.dec_src_chan[chan_id].dec_src_buff.read = (u32)(dec_chan->dec_src_buff.Start);
        g_socp_stat.dec_src_chan[chan_id].dec_src_buff.write = (u32)(dec_chan->dec_src_buff.Start);
    }
#endif
}

/*
 * �� �� ��: socp_reset_enc_chan
 * ��������: ��λ����ͨ��
 * �������: chan_id       ����ͨ����
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
s32 socp_reset_enc_chan(u32 chan_id)
{
    u32 reset_flag;
    u32 i;
    socp_enc_src_chan_s *p_chan;

    p_chan = &g_socp_stat.enc_src_chan[chan_id];

    /* ��λͨ�� */
    SOCP_REG_SETBITS(SOCP_REG_ENCRST, chan_id, 1, 1);

    /* �ȴ�ͨ������ */
    for (i = 0; i < SOCP_RESET_TIME; i++) {
        reset_flag = SOCP_REG_GETBITS(SOCP_REG_ENCRST, chan_id, 1);
        if (0 == reset_flag) {
            break;
        }

        if (SOCP_RESET_TIME == i) {
            socp_error("socp channel[%d] reset failed!\n", chan_id);
        }
    }

    socp_reset_chan_reg_wr_addr(chan_id, SOCP_CODER_SRC_CHAN, p_chan, NULL);

    /* ������������ */
    /*lint -save -e647*/
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 1, 2, p_chan->chan_mode);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 4, 4, p_chan->dst_chan_id);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 8, 2, p_chan->priority);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 10, 1, p_chan->bypass_en);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 16, 8, p_chan->data_type);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 11, 1, p_chan->data_type_en);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 31, 1, p_chan->debug_en);

    /* ���ͨ��������״̬��ʹ��ͨ�� */
    if (p_chan->chan_en) {
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(chan_id), 0, 1, 1);
    }
    /*lint -restore +e647*/
    return BSP_OK;
}

/*
 * �� �� ��: socp_reset_dec_chan
 * ��������: ��λ����ͨ��
 * �������: chan_id       ����ͨ����
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
s32 socp_reset_dec_chan(u32 chan_id)
{
    u32 reset_flag;
    u32 i;
    socp_decsrc_chan_s *p_chan = NULL;

    if (chan_id >= SOCP_MAX_DECSRC_CHN) {
        return BSP_ERROR;
    }

    p_chan = &g_socp_stat.dec_src_chan[chan_id];

    /* ��λͨ�� */
    SOCP_REG_SETBITS(SOCP_REG_DECRST, chan_id, 1, 1);

    /* �ȴ�ͨ������ */
    for (i = 0; i < SOCP_RESET_TIME; i++) {
        reset_flag = SOCP_REG_GETBITS(SOCP_REG_DECRST, chan_id, 1);
        if (0 == reset_flag) {
            break;
        }
        if ((SOCP_RESET_TIME - 1) == i) {
            socp_error("socp_reset_dec_chan 0x%x failed!\n", chan_id);
        }
    }

    socp_reset_chan_reg_wr_addr(chan_id, SOCP_DECODER_SRC_CHAN, NULL, p_chan);
    /*lint -save -e647*/
    SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(chan_id), 0, 16, p_chan->dec_src_buff.length);
    SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(chan_id), 31, 1, p_chan->data_type_en);
    /*lint -restore +e647*/
    return BSP_OK;
}

/*
 * �� �� ��: socp_soft_free_encdst_chan
 * ��������: ���ͷű���Ŀ��ͨ��
 * �������: enc_dst_chan_id       ����ͨ����
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
s32 socp_soft_free_encdst_chan(u32 enc_dst_chan_id)
{
    u32 chan_id;
    u32 chan_type;
    socp_encdst_chan_s *p_chan = NULL;
    u32 ret;

    chan_id = SOCP_REAL_CHAN_ID(enc_dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(enc_dst_chan_id);

    if ((ret = socp_check_chan_type(chan_type, SOCP_CODER_DEST_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
        return ret;
    }

    p_chan = &g_socp_stat.enc_dst_chan[chan_id];

#ifdef FEATURE_SOCP_ADDR_64BITS
    /* д����ʼ��ַ��Ŀ��buffer��ʼ��ַ�Ĵ��� */
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFADDR_L(chan_id), (u32)p_chan->enc_dst_buff.Start);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFADDR_H(chan_id), (u32)(((u64)p_chan->enc_dst_buff.Start) >> 32));
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(chan_id), 0);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFWPTR(chan_id), 0);

    g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff.write = 0;
    g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff.read = 0;

#else
    /* д����ʼ��ַ��Ŀ��buffer��ʼ��ַ�Ĵ��� */
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFADDR(chan_id), (u32)p_chan->enc_dst_buff.Start);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(chan_id), (u32)p_chan->enc_dst_buff.Start);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFWPTR(chan_id), (u32)p_chan->enc_dst_buff.Start);

    g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff.write = (u32)p_chan->enc_dst_buff.Start;
    g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff.read = (u32)p_chan->enc_dst_buff.Start;
#endif

    g_socp_stat.enc_dst_chan[chan_id].set_state = SOCP_CHN_UNSET;

    return BSP_OK;
}

/*
 * �� �� ��: socp_soft_free_decsrc_chan
 * ��������: ���ͷŽ���Դͨ��
 * �������: dec_src_chan_id       ����ͨ����
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
s32 socp_soft_free_decsrc_chan(u32 dec_src_chan_id)
{
    u32 chan_id;
    u32 chan_type;
    socp_decsrc_chan_s *dec_src_chan = NULL;
    u32 ret;

    chan_id = SOCP_REAL_CHAN_ID(dec_src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dec_src_chan_id);

    if ((ret = socp_check_chan_type(chan_type, SOCP_DECODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
        return ret;
    }

    dec_src_chan = &g_socp_stat.dec_src_chan[chan_id];

#ifdef FEATURE_SOCP_ADDR_64BITS
    /* д����ʼ��ַ��Ŀ��buffer��ʼ��ַ�Ĵ��� */
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(chan_id), 0);
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR_L(chan_id), (u32)dec_src_chan->dec_src_buff.Start);
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR_H(chan_id), (u32)(((u64)dec_src_chan->dec_src_buff.Start) >> 32));
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFRPTR(chan_id), 0);

    g_socp_stat.dec_src_chan[chan_id].dec_src_buff.write = 0;
    g_socp_stat.dec_src_chan[chan_id].dec_src_buff.read = 0;
#else
    /* д����ʼ��ַ��Ŀ��buffer��ʼ��ַ�Ĵ��� */
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(chan_id), (u32)dec_src_chan->dec_src_buff.Start);
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR(chan_id), (u32)dec_src_chan->dec_src_buff.Start);
    SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFRPTR(chan_id), (u32)dec_src_chan->dec_src_buff.Start);

    g_socp_stat.dec_src_chan[chan_id].dec_src_buff.write = (u32)dec_src_chan->dec_src_buff.Start;
    g_socp_stat.dec_src_chan[chan_id].dec_src_buff.read = (u32)dec_src_chan->dec_src_buff.Start;
#endif

    g_socp_stat.dec_src_chan[chan_id].set_state = SOCP_CHN_UNSET;

    return BSP_OK;
}

/* cov_verified_start */
/*
 * �� �� ��: socp_get_enc_rd_size
 * ��������:  ��ȡ����Դͨ��RDbuffer
 * �������: ��
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
u32 socp_get_enc_rd_size(u32 chan_id)
{
    SOCP_BUFFER_RW_STRU buff;
    u32 PAddr;

    SOCP_REG_READ(SOCP_REG_ENCSRC_RDQRPTR(chan_id), PAddr);
    g_socp_stat.enc_src_chan[chan_id].rd_buff.read = PAddr;
    SOCP_REG_READ(SOCP_REG_ENCSRC_RDQWPTR(chan_id), PAddr);
    g_socp_stat.enc_src_chan[chan_id].rd_buff.write = PAddr;

    socp_get_data_buffer(&g_socp_stat.enc_src_chan[chan_id].rd_buff, &buff);
    return (buff.u32Size + buff.u32RbSize);
}

/*
 * �� �� ��: socp_encsrc_rd_handler
 * ��������:  ����Դͨ��RDbuffer�жϴ�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_encsrc_rd_handler(u32 rd_size, u32 i)
{
    u32 chan_id;

    if (rd_size == g_socp_stat.enc_src_chan[i].last_rd_size) {
        if (g_socp_stat.enc_src_chan[i].rd_cb) {
            chan_id = SOCP_CHAN_DEF(SOCP_CODER_SRC_CHAN, i);
            (void)g_socp_stat.enc_src_chan[i].rd_cb(chan_id);

            g_socp_debug_info.socp_debug_encsrc.enc_src_task_rd_cb_cnt[i]++;
        }

        g_socp_stat.enc_src_chan[i].last_rd_size = 0;
    } else {
        g_socp_stat.enc_src_chan[i].last_rd_size = rd_size;
    }

    return;
}
/* cov_verified_stop */

/*
 * �� �� ��: socp_encsrc_task
 * ��������: ģ��������:����Դ�жϣ�˫��
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
int socp_encsrc_task(void *data)
{
    u32 i;
    u32 int_head_state = 0;
    u32 chan_id;
    unsigned long lock_flag;

    do {
        /* ��ʱ���߱��жϣ����������� */
        if (0 != down_interruptible(&g_socp_stat.u32EncSrcSemID)) {
            continue;
        }

        spin_lock_irqsave(&lock, lock_flag);
        int_head_state = g_socp_stat.int_enc_src_header;
        g_socp_stat.int_enc_src_header = 0;
        g_socp_stat.u32IntEncSrcRD = 0;
        spin_unlock_irqrestore(&lock, lock_flag);

        /* ��������ͷ'HISI'������� */
        if (int_head_state) {
            for (i = 0; i < SOCP_MAX_ENCSRC_CHN; i++) {
                /* ���ͨ���Ƿ����� */
                if (SOCP_CHN_ALLOCATED == g_socp_stat.enc_src_chan[i].alloc_state) {
                    if (int_head_state & ((u32)1 << i)) {
                        socp_crit("EncSrcHeaderError chan_id = %d", i);
                        if (g_socp_stat.enc_src_chan[i].event_cb) {
                            g_socp_debug_info.socp_debug_encsrc.enc_src_task_head_cb_ori_cnt[i]++;
                            chan_id = SOCP_CHAN_DEF(SOCP_CODER_SRC_CHAN, i);
                            (void)g_socp_stat.enc_src_chan[i].event_cb(chan_id, SOCP_EVENT_PKT_HEADER_ERROR, 0);
                            g_socp_debug_info.socp_debug_encsrc.enc_src_task_head_cb_cnt[i]++;
                        }
                    }
                }
            }
        }
    } while (1);
}

/*
 * �� �� ��: socp_encdst_task
 * ��������: ģ��������:����Ŀ�ģ�App��
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
int socp_encdst_task(void *data)
{
    u32 i;
    u32 int_trf_state = 0;
    u32 int_ovf_state = 0;
    u32 int_thrh_ovf_state = 0;
    u32 chan_id = 0;
    unsigned long lock_flag;
    u32 read;
    u32 write;

    do {
        /* ��ʱ���߱��жϣ����������� */
        if (0 != down_interruptible(&g_socp_stat.u32EncDstSemID)) {
            continue;
        }

        spin_lock_irqsave(&lock, lock_flag);
        int_trf_state = g_socp_stat.int_enc_dst_tfr;
        g_socp_stat.int_enc_dst_tfr = 0;
        int_ovf_state = g_socp_stat.int_enc_dst_ovf;
        g_socp_stat.int_enc_dst_ovf = 0;
        int_thrh_ovf_state = g_socp_stat.int_enc_dst_thrh_ovf;
        g_socp_stat.int_enc_dst_thrh_ovf = 0;
        spin_unlock_irqrestore(&lock, lock_flag);

        /* ������봫������ж� */
        if (int_trf_state) {
            for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
                /* ���ͨ���Ƿ����� */
                if (SOCP_CHN_SET == g_socp_stat.enc_dst_chan[i].set_state) {
                    if (int_trf_state & ((u32)1 << i)) {
                        if (g_socp_stat.enc_dst_chan[i].read_cb) {
                            /*lint -save -e732*/
                            SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(i), read);
                            SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(i), write);
                            /*lint -restore +e732*/
                            SOCP_DEBUG_TRACE(SOCP_DEBUG_READ_DONE, read, write, 0, 0);
                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_trf_cb_ori_cnt[i]++;
                            chan_id = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
                            if (i == 1) {
                                g_enc_dst_sta[g_enc_dst_stat_cnt].task_start_slice = bsp_get_slice_value();
                            }
                            (void)g_socp_stat.enc_dst_chan[i].read_cb(chan_id);
                            if (i == 1) {
                                g_enc_dst_sta[g_enc_dst_stat_cnt].task_end_slice = bsp_get_slice_value();
                            }

                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_trf_cb_cnt[i]++;
                        }
                    }
                }
            }
        }

        /* �������Ŀ�� buffer ����ж� */
        if (int_ovf_state) {
            for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
                /* ���ͨ���Ƿ����� */
                if (SOCP_CHN_SET == g_socp_stat.enc_dst_chan[i].set_state) {
                    if (int_ovf_state & ((u32)1 << i)) {
                        if (g_socp_stat.enc_dst_chan[i].event_cb) {
                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_ovf_cb_ori_cnt[i]++;
                            chan_id = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
                            (void)g_socp_stat.enc_dst_chan[i].event_cb(chan_id, SOCP_EVENT_OUTBUFFER_OVERFLOW, 0);

                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_ovf_cb_cnt[i]++;
                        }
                        if (g_socp_stat.enc_dst_chan[i].read_cb) {
                            /*lint -save -e732*/
                            SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(i), read);
                            SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(i), write);
                            /*lint -restore +e732*/
                            SOCP_DEBUG_TRACE(SOCP_DEBUG_READ_DONE, read, write, 0, 0);
                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_trf_cb_ori_cnt[i]++;
                            chan_id = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
                            (void)g_socp_stat.enc_dst_chan[i].read_cb(chan_id);

                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_trf_cb_cnt[i]++;
                        }
                    }
                }
            }
        }

        /* �������Ŀ�� buffer ��ֵ����ж� */
        if (int_thrh_ovf_state) {
            for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
                /* ���ͨ���Ƿ����� */
                if (SOCP_CHN_SET == g_socp_stat.enc_dst_chan[i].set_state) {
                    if (int_thrh_ovf_state & ((u32)1 << (i + SOCP_ENC_DST_BUFF_THRESHOLD_OVF_BEGIN))) {
                        if (g_socp_stat.enc_dst_chan[i].event_cb) {
                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_ovf_cb_ori_cnt[i]++;
                            chan_id = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
                            (void)g_socp_stat.enc_dst_chan[i].event_cb(chan_id,
                                                                        SOCP_EVENT_OUTBUFFER_THRESHOLD_OVERFLOW, 0);

                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_ovf_cb_cnt[i]++;
                        }
                        if (g_socp_stat.enc_dst_chan[i].read_cb) {
                            /*lint -save -e732*/
                            SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(i), read);
                            SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(i), write);
                            /*lint -restore +e732*/
                            SOCP_DEBUG_TRACE(SOCP_DEBUG_READ_DONE, read, write, 0, 0);
                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_trf_cb_ori_cnt[i]++;
                            chan_id = SOCP_CHAN_DEF(SOCP_CODER_DEST_CHAN, i);
                            (void)g_socp_stat.enc_dst_chan[i].read_cb(chan_id);

                            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_trf_cb_cnt[i]++;
                        }
                    }
                }
            }
        }

    } while (1);
}

/* cov_verified_start */
/*
 * �� �� ��: socp_decsrc_event_handler
 * ��������: ����Դͨ���¼�������
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_decsrc_event_handler(u32 id, u32 dec_int_state)
{
    u32 chan_id = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN, id);

    if (g_socp_stat.dec_src_chan[id].event_cb) {
        if (dec_int_state & 0x10) {
            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_ori_cnt[id]++;

            (void)g_socp_stat.dec_src_chan[id].event_cb(chan_id, SOCP_EVENT_DECODER_UNDERFLOW, 0);

            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_cnt[id]++;
        }

        if (dec_int_state & 0x100) {
            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_ori_cnt[id]++;

            (void)g_socp_stat.dec_src_chan[id].event_cb(chan_id, SOCP_EVENT_HDLC_HEADER_ERROR, 0);

            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_cnt[id]++;
        }

        if (dec_int_state & 0x1000) {
            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_ori_cnt[id]++;

            (void)g_socp_stat.dec_src_chan[id].event_cb(chan_id, SOCP_EVENT_DATA_TYPE_ERROR, 0);

            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_cnt[id]++;
        }

        if (dec_int_state & 0x10000) {
            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_ori_cnt[id]++;

            (void)g_socp_stat.dec_src_chan[id].event_cb(chan_id, SOCP_EVENT_CRC_ERROR, 0);

            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_cnt[id]++;
        }

        if (dec_int_state & 0x100000) {
            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_ori_cnt[id]++;

            (void)g_socp_stat.dec_src_chan[id].event_cb(chan_id, SOCP_EVENT_PKT_LENGTH_ERROR, 0);

            g_socp_debug_info.socp_debug_dec_src.socp_decsrc_task_err_cb_cnt[id]++;
        }
    }
}

/*
 * �� �� ��: socp_decsrc_handler
 * ��������: ����Դͨ���жϴ�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_decsrc_handler(void)
{
    u32 int_state, dec_int_state;
    u32 chan_id;
    u32 i;

    if (g_socp_stat.int_dec_src_err) {
        int_state = g_socp_stat.int_dec_src_err;
        g_socp_stat.int_dec_src_err = 0;

        for (i = 0; i < SOCP_MAX_DECSRC_CHN; i++) {
            /* ���ͨ���Ƿ����� */

            if (SOCP_CHN_SET == g_socp_stat.dec_src_chan[i].set_state) {
                dec_int_state = int_state >> i;
                chan_id = SOCP_CHAN_DEF(SOCP_DECODER_SRC_CHAN, i);

                if (g_socp_stat.dec_src_chan[i].rd_cb) {
                    if (dec_int_state & 0x1) {
                        (void)g_socp_stat.dec_src_chan[i].rd_cb(chan_id);
                    }
                }

                socp_decsrc_event_handler(i, dec_int_state);
            }
        }
    }
}
/* cov_verified_stop */

/*
 * �� �� ��: socp_decsrc_task
 * ��������: ģ��������:����Դ��A��
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
int socp_decsrc_task(void *data)
{
    unsigned long lock_flag;

    do {
        /* ��ʱ���߱��жϣ����������� */
        if (0 != down_interruptible(&g_socp_stat.dec_src_sem_id)) {
            continue;
        }
        spin_lock_irqsave(&lock, lock_flag);
        /* �������Դ�ж� */
        socp_decsrc_handler();
        spin_unlock_irqrestore(&lock, lock_flag);
    } while (1);
}

/*
 * �� �� ��: socp_decdst_task
 * ��������: ģ��������:����Ŀ�ģ�˫��
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
int socp_decdst_task(void *data)
{
    u32 i;
    u32 int_trf_state = 0;
    u32 int_ovf_state = 0;
    u32 chan_id;
    unsigned long lock_flag;

    do {
        /* ��ʱ���߱��жϣ����������� */
        if (0 != down_interruptible(&g_socp_stat.dec_dst_sem_id)) {
            continue;
        }

        spin_lock_irqsave(&lock, lock_flag);
        int_trf_state = g_socp_stat.int_dec_dst_trf;
        g_socp_stat.int_dec_dst_trf = 0;
        int_ovf_state = g_socp_stat.int_dec_dst_ovf;
        g_socp_stat.int_dec_dst_ovf = 0;
        spin_unlock_irqrestore(&lock, lock_flag);

        /* ������봫������ж� */
        if (int_trf_state) {
            for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
                /* ���ͨ���Ƿ����� */
                if (SOCP_CHN_ALLOCATED == g_socp_stat.dec_dst_chan[i].alloc_state) {
                    if (int_trf_state & ((u32)1 << i)) {
                        if (g_socp_stat.dec_dst_chan[i].read_cb) {
                            g_socp_debug_info.socp_debug_dec_dst.socp_decdst_task_trf_cb_ori_cnt[i]++;

                            chan_id = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN, i);
                            (void)g_socp_stat.dec_dst_chan[i].read_cb(chan_id);

                            g_socp_debug_info.socp_debug_dec_dst.socp_decdst_task_trf_cb_cnt[i]++;
                        }
                    }
                }
            }
        }

        /* �������Ŀ�� buffer ����ж� */
        if (int_ovf_state) {
            for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
                /* ���ͨ���Ƿ����� */
                if (SOCP_CHN_ALLOCATED == g_socp_stat.dec_dst_chan[i].alloc_state) {
                    if (int_ovf_state & ((u32)1 << i)) {
                        if (g_socp_stat.dec_dst_chan[i].event_cb) {
                            g_socp_debug_info.socp_debug_dec_dst.socp_decdst_task_ovf_cb_ori_cnt[i]++;

                            chan_id = SOCP_CHAN_DEF(SOCP_DECODER_DEST_CHAN, i);
                            (void)g_socp_stat.dec_dst_chan[i].event_cb(chan_id, SOCP_EVENT_OUTBUFFER_OVERFLOW, 0);

                            g_socp_debug_info.socp_debug_dec_dst.socp_decdst_task_ovf_cb_cnt[i]++;
                        }
                    }
                }
            }
        }
    } while (1);
}

/*
 * �� �� ��: socp_create_task
 * ��������: socp���񴴽�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: �����ɹ����ı�ʶ��
 */
s32 socp_create_task(s8 *task_name, unsigned long *task_id, socp_task_entry task_func, u32 task_priority,
                     u32 task_stack_size, void *task_param)
{
    struct task_struct *tsk = NULL;
    struct sched_param param;

    tsk = kthread_run(task_func, task_param, task_name);
    if (IS_ERR(tsk)) {
        socp_error("create kthread failed!\n");
        return BSP_ERROR;
    }

    param.sched_priority = task_priority;
    if (BSP_OK != sched_setscheduler(tsk, SCHED_FIFO, &param)) {
        socp_error("Creat Task %s sched_setscheduler Error\n", task_name);
        return BSP_ERROR;
    }

    *task_id = (uintptr_t)tsk;

    return BSP_OK;
}

/*
 * �� �� ��: socp_init_task
 * ��������: �������������
 * �������: ��
 * �������: ��
 * �� �� ֵ: �����ɹ����ı�ʶ��
 */
s32 socp_init_task(void)
{
    u32 arguments[4] = { 0, 0, 0, 0 };

    /* ����Դͨ������ */
    sema_init(&g_socp_stat.u32EncSrcSemID, 0);
    if (!g_socp_stat.u32EncSrcTskID) {
        if (BSP_OK != socp_create_task("EncSrc", (unsigned long *)&g_socp_stat.u32EncSrcTskID,
                                       (socp_task_entry)socp_encsrc_task, SOCP_ENCSRC_TASK_PRO, 0x1000, arguments)) {
            socp_error("create socp_encsrc_task failed.\n");
            return BSP_ERR_SOCP_TSK_CREATE;
        }
    }

    /* �������ͨ������ */
    sema_init(&g_socp_stat.u32EncDstSemID, 0);
    if (!g_socp_stat.u32EncDstTskID) {
        if (BSP_OK != socp_create_task("EncDst", (unsigned long *)&g_socp_stat.u32EncDstTskID,
                                       (socp_task_entry)socp_encdst_task, SOCP_ENCDST_TASK_PRO, 0x1000, arguments)) {
            socp_error("create socp_encdst_task failed.\n");
            return BSP_ERR_SOCP_TSK_CREATE;
        }
    }

    /* ����Դͨ������ */
    sema_init(&g_socp_stat.dec_src_sem_id, 0);
    if (!g_socp_stat.dec_src_task_id) {
        if (BSP_OK != socp_create_task("DecSrc", (unsigned long *)&g_socp_stat.dec_src_task_id,
                                       (socp_task_entry)socp_decsrc_task, SOCP_DECSRC_TASK_PRO, 0x1000, arguments)) {
            socp_error("create socp_DecSrc_task failed.\n");
            return BSP_ERR_SOCP_TSK_CREATE;
        }
    }

    /* ����Ŀ��ͨ������ */
    sema_init(&g_socp_stat.dec_dst_sem_id, 0);
    if (!g_socp_stat.dec_dst_task_id) {
        if (BSP_OK != socp_create_task("DecDst", (unsigned long *)&g_socp_stat.dec_dst_task_id,
                                       (socp_task_entry)socp_decdst_task, SOCP_DECDST_TASK_PRO, 0x1000, arguments)) {
            socp_error("create socp_DecDst_task failed.\n");
            return BSP_ERR_SOCP_TSK_CREATE;
        }
    }

    return BSP_OK;
}

/*
 * �� �� ��: socp_handler_encsrc
 * ��������: ����Դͨ����������RD�������ϲ���ɣ�����RD�жϿ��Բ�������
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_handler_encsrc(void)
{
    u32 int_flag = 0;
    u32 int_state = 0;
    int is_handle = BSP_FALSE;
    u32 i = 0;

    /* read and clear the interrupt flags */
    SOCP_REG_READ(SOCP_REG_GBL_INTSTAT, int_flag);
    /* �����ͷ���� */
    if (int_flag & SOCP_APP_ENC_FLAGINT_MASK) {
        SOCP_REG_READ(SOCP_REG_APP_INTSTAT1, int_state);
        SOCP_REG_WRITE(SOCP_REG_ENC_RAWINT1, int_state);

        g_socp_stat.int_enc_src_header |= int_state;
        is_handle = BSP_TRUE;

        for (i = 0; i < SOCP_MAX_ENCSRC_CHN; i++) {
            if (int_state & ((u32)1 << i)) {
                /* debugģʽ���ΰ�ͷ�����ж� */
                if (SOCP_REG_GETBITS(SOCP_REG_ENCSRC_BUFCFG1(i), 31, 1)) /*lint !e647*/
                {
                    SOCP_REG_SETBITS(SOCP_REG_APP_MASK1, i, 1, 1);
                }
                g_socp_debug_info.socp_debug_encsrc.enc_src_task_isr_head_int_cnt[i]++;
            }
        }
    }

    /* ���ٴ���RD����жϣ���ʼ��ʱ�������� */

    if (is_handle) {
        up(&g_socp_stat.u32EncSrcSemID);
    }

    return;
}

/*
 * �� �� ��: socp_handler_encdst
 * ��������: ����Ŀ���жϴ�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
/*lint -save -e550*/
void socp_handler_encdst(void)
{
    u32 int_flag = 0;
    u32 int_state = 0;
    int is_handle = BSP_FALSE;
    u32 i;
    u32 mask;
    u32 mask2;
    u32 write;

    unsigned long lock_flag;
    int countFlag = BSP_FALSE;
    u32 ModeState = 0;

    /* ����Ŀ�Ĵ����ж� */
    SOCP_REG_READ(SOCP_REG_GBL_INTSTAT, int_flag);
    if (int_flag & SOCP_APP_ENC_TFRINT_MASK) {
        spin_lock_irqsave(&lock, lock_flag);
        SOCP_REG_READ(SOCP_REG_ENC_INTSTAT0, int_state);
        SOCP_REG_READ(SOCP_REG_ENC_MASK0, mask);
        SOCP_REG_WRITE(SOCP_REG_ENC_MASK0, (int_state | mask));
        /* ��������ж� */
        SOCP_REG_READ(SOCP_REG_ENC_MASK2, mask2);
        SOCP_REG_WRITE(SOCP_REG_ENC_MASK2, ((int_state << 16) | mask2));
        SOCP_REG_WRITE(SOCP_REG_ENC_RAWINT0, int_state);

        g_socp_stat.int_enc_dst_tfr |= int_state;
        spin_unlock_irqrestore(&lock, lock_flag);
        is_handle = BSP_TRUE;

        for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
            if (int_state & ((u32)1 << i)) {
                if (g_throwout == 0x5a5a5a5a) /* ����ѹ����ʹ�� */
                {
                    spin_lock_irqsave(&lock, lock_flag);
                    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(i), write);
                    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(i), write);
                    SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, i, 1, 1);
                    SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, i, 1, 0);
                    /* overflow int */
                    SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, i + 16, 1, 1);
                    SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, i + 16, 1, 0);
                    spin_unlock_irqrestore(&lock, lock_flag);

                    is_handle = BSP_FALSE;
                }
                if (i == 1) {
                    g_enc_dst_sta[g_enc_dst_stat_cnt].int_start_slice = bsp_get_slice_value();
                    g_socp_enc_dst_isr_trf_int_cnt++;
                    countFlag = BSP_TRUE;
                }
                g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_isr_trf_int_cnt[i]++;
            }
        }
    }
    // �����ж�����ֵ�жϹ���һ���Ĵ���
    else if (int_flag & SOCP_APP_ENC_OUTOVFINT_MASK) {
        SOCP_REG_READ(SOCP_REG_ENC_INTSTAT2, int_state);
        // ����Ŀ��buffer��ֵ�жϴ���
        if (0 != (int_state & SOCP_ENC_DST_BUFF_THRESHOLD_OVF_MASK)) {
            spin_lock_irqsave(&lock, lock_flag);
            SOCP_REG_READ(SOCP_REG_ENC_MASK2, mask);
            SOCP_REG_WRITE(SOCP_REG_ENC_MASK2, (int_state | mask));
            SOCP_REG_WRITE(SOCP_REG_ENC_RAWINT2, int_state);
            g_socp_stat.int_enc_dst_thrh_ovf |= int_state;
            spin_unlock_irqrestore(&lock, lock_flag);

            is_handle = BSP_TRUE;

            for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
                if (int_state & ((u32)1 << (i + SOCP_ENC_DST_BUFF_THRESHOLD_OVF_BEGIN))) {
                    g_socp_debug_info.socp_debug_enc_dst.socp_encdst_isr_thrh_ovf_int_cnt[i]++;
                }
            }
        }
        // ����Ŀ��buffer�����ж�
        if (0 != (int_state & SOCP_ENC_DST_BUFF_OVF_MASK)) {
            spin_lock_irqsave(&lock, lock_flag);
            SOCP_REG_READ(SOCP_REG_ENC_MASK2, mask);
            SOCP_REG_WRITE(SOCP_REG_ENC_MASK2, (int_state | mask));
            SOCP_REG_WRITE(SOCP_REG_ENC_RAWINT2, int_state);
            g_socp_stat.int_enc_dst_ovf |= int_state;
            spin_unlock_irqrestore(&lock, lock_flag);

            is_handle = BSP_TRUE;

            for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
                if (int_state & ((u32)1 << i)) {
                    g_socp_debug_info.socp_debug_enc_dst.socp_encdst_task_isr_ovf_int_cnt[i]++;
                }
            }
        }
    } else if (g_socp_stat.compress_isr) {
        g_socp_stat.compress_isr();
        return;
    }

    /* ����Ŀ��bufferģʽ�л���� */
    else if (int_flag & SOCP_CORE0_ENC_MODESWT_MASK) {
        spin_lock_irqsave(&lock, lock_flag);

        SOCP_REG_READ(SOCP_REG_ENC_INTSTAT0, int_state);
        SOCP_REG_READ(SOCP_REG_ENC_MASK0, mask);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, 16, 7, (((int_state | mask) >> 16) & 0x7f));

        /* ��ԭʼ�ж�״̬ */
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, 16, 7, ((int_state >> 16) & 0x7f));

        mask = 0;
        for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
            SOCP_REG_READ(SOCP_REG_ENCDEST_SBCFG(i), ModeState);
            if (ModeState & 0x02) {
                mask |= (1 << i);
            }
        }

        /* ���δ���ѭ��ģʽͨ���Ĵ����жϺ���ֵ����ж� */
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, 0, 7, mask);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, 0, 7, mask);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, 16, 7, mask);

        spin_unlock_irqrestore(&lock, lock_flag);
    } else {
        is_handle = BSP_FALSE;
    }

    if (is_handle) {
        if (countFlag == BSP_TRUE) {
            g_enc_dst_sta[g_enc_dst_stat_cnt].int_end_slice = bsp_get_slice_value();
        }
        up(&g_socp_stat.u32EncDstSemID);
    }

    return;
}
/*lint -restore +e550*/

/*
 * �� �� ��: socp_handler_decsrc
 * ��������: ����Դͨ���жϴ�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_handler_decsrc(void)
{
    u32 int_flag = 0;
    u32 int_state = 0;
    int is_handle = BSP_FALSE;
    u32 i = 0;

    /* ����������� */
    SOCP_REG_READ(SOCP_REG_GBL_INTSTAT, int_flag);
    if (int_flag & SOCP_DEC_INERRINT_MASK) {
        SOCP_REG_READ(SOCP_REG_DEC_INTSTAT1, int_state);
        SOCP_REG_WRITE(SOCP_REG_DEC_RAWINT1, int_state);

        g_socp_stat.int_dec_src_err |= int_state;
        is_handle = BSP_TRUE;

        for (i = 0; i < SOCP_MAX_DECSRC_CHN; i++) {
            if (int_state & 0x1) {
                g_socp_debug_info.socp_debug_dec_src.socp_decsrc_isr_err_int_cnt[i]++;
            }
        }
    }

    if (is_handle) {
        up(&g_socp_stat.dec_src_sem_id);
    }

    return;
}

/*
 * �� �� ��: socp_handler_decdst
 * ��������: ����Ŀ��ͨ���жϴ�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */

void socp_handler_decdst(void)
{
    u32 int_flag = 0;
    u32 int_state = 0;
    int is_handle = BSP_FALSE;
    u32 trf_mask;
    u32 trf_state;
    u32 over_mask;
    u32 over_state;
    u32 trf_mask_reg;
    u32 i = 0;

    trf_mask = SOCP_CORE0_DEC_TFRINT_MASK;
    trf_state = SOCP_REG_DEC_CORE0ISTAT0;
    trf_mask_reg = SOCP_REG_DEC_CORE0MASK0;
    over_mask = SOCP_CORE0_DEC_OUTOVFINT_MASK;
    over_state = SOCP_REG_DEC_CORE0ISTAT2;

    /* ���봫���ж� */
    SOCP_REG_READ(SOCP_REG_GBL_INTSTAT, int_flag);
    if (int_flag & trf_mask) {
        u32 mask;
        SOCP_REG_READ(trf_state, int_state);
        int_state = int_state & 0xffff;
        SOCP_REG_READ(trf_mask_reg, mask);
        SOCP_REG_WRITE(trf_mask_reg, (int_state | mask));
        SOCP_REG_WRITE(SOCP_REG_DEC_RAWINT0, int_state);

        g_socp_stat.int_dec_dst_trf |= int_state;
        is_handle = BSP_TRUE;

        for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
            if (int_state & ((u32)1 << i)) {
                g_socp_debug_info.socp_debug_dec_dst.socp_decdst_isr_trf_int_cnt[i]++;
            }
        }
    }

    /* ����Ŀ��buffer ���� */
    SOCP_REG_READ(SOCP_REG_GBL_INTSTAT, int_flag);
    if (int_flag & over_mask) {
        SOCP_REG_READ(over_state, int_state);
        int_state = int_state & 0xffff;
        SOCP_REG_WRITE(SOCP_REG_DEC_RAWINT2, int_state);

        g_socp_stat.int_dec_dst_ovf |= int_state;
        is_handle = BSP_TRUE;

        for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
            if (int_state & ((u32)1 << i)) {
                g_socp_debug_info.socp_debug_dec_dst.socp_decdst_isr_ovf_int_cnt[i]++;
            }
        }
    }

    if (is_handle) {
        up(&g_socp_stat.dec_dst_sem_id);
    }

    return;
}

/*
 * �� �� ��: socp_app_int_handler
 * ��������: APP ���жϴ�����
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
irqreturn_t socp_app_int_handler(int irq, void *dev_info)
{
    g_socp_debug_info.socp_debug_gbl.socp_app_etr_int_cnt++;

    socp_handler_encsrc();

    socp_handler_encdst();
    socp_handler_decsrc();
    socp_handler_decdst();

    g_socp_debug_info.socp_debug_gbl.socp_app_suc_int_cnt++;

    return 1;
}

struct platform_device *modem_socp_pdev;

static int socp_driver_probe(struct platform_device *pdev)
{
    dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));  //lint !e598 !e648
    modem_socp_pdev = pdev;

    return BSP_OK;
}

static const struct of_device_id socp_dev_of_match[] = {
    {
        .compatible = "hisilicon,socp_balong_app",
        .data = NULL,
    },
    {},
};

static struct platform_driver socp_driver = {
        .driver = {
                   .name = "modem_socp",

                   .owner = THIS_MODULE,
                   .of_match_table = socp_dev_of_match,
        },
        .probe = socp_driver_probe,
};

/*
 * �� �� ��: socp_init
 * ��������: ģ���ʼ������
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��ʼ���ɹ��ı�ʶ��
 */
s32 socp_init(void)
{
    s32 ret;
    u32 irq;

    struct device_node *dev = NULL;

    if (BSP_TRUE == g_socp_stat.init_flag) {
        return BSP_OK;
    }

    socp_crit("[init]start\n");

    spin_lock_init(&lock);

    ret = platform_driver_register(&socp_driver);
    if (ret) {
        socp_error("driver_register fail,ret=0x%x\n", ret);
        return ret;
    }

    /* Add dts begin */
    dev = of_find_compatible_node(NULL, NULL, "hisilicon,socp_balong_app");
    if (NULL == dev) {
        socp_error("[init] Socp dev find failed\n");
        return BSP_ERROR;
    }
    /* Add dts end */

    g_socp_stat.base_addr = (uintptr_t)of_iomap(dev, 0);
    if (0 == g_socp_stat.base_addr) {
        socp_error("[init] base addr is NULL!\n");
        return BSP_ERROR;
    }

    /* bsp_memmap.h���滹û�ж�BBP�Ĵ����ռ���ӳ�� */
    g_socp_stat.armBaseAddr = (unsigned long)BBP_REG_ARM_BASEADDR;
    ret = memset_s(&g_socp_debug_info, sizeof(g_socp_debug_info), 0x0, sizeof(g_socp_debug_info));
    if (ret) {
        socp_error("[init] memset fail,ret=0x%x\n", ret);
        return ret;
    }

    SOCP_REG_READ(SOCP_REG_SOCP_VERSION, g_socp_version);

    socp_global_ctrl_init();

    ret = socp_clk_enable();
    if (ret) {
        return ret;
    }

    socp_global_reset();

    bsp_socp_ind_delay_init();

    /* ������������� */
    ret = socp_init_task();
    if (BSP_OK != ret) {
        socp_error("[init] create task failed(0x%x)\n", ret);
        return (s32)ret;
    }

    /* ���ж� */
    irq = irq_of_parse_and_map(dev, 0);
    ret = request_irq(irq, (irq_handler_t)socp_app_int_handler, 0, "SOCP_APP_IRQ", BSP_NULL);
    if (BSP_OK != ret) {
        socp_error("[init] connect app core int failed(0x%x)\n", ret);
        return BSP_ERROR;
    }

    /* ���ó�ʼ��״̬ */
    g_socp_stat.init_flag = BSP_TRUE;

    socp_encsrc_headerr_irq_enable();

    socp_crit("[init]ok\n");

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_encdst_dsm_init
 * ��������: socp����Ŀ�Ķ��ж�״̬��ʼ��
 * �������: enc_dst_chan_id: ����Ŀ�Ķ�ͨ����
 *           b_enable: ��ʼ�����ж�״̬
 * �������: ��
 * �� �� ֵ: ��
 */
void bsp_socp_encdst_dsm_init(u32 enc_dst_chan_id, u32 b_enable)
{
    u32 real_chan_id;
    u32 chan_type;

    real_chan_id = SOCP_REAL_CHAN_ID(enc_dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(enc_dst_chan_id);

    if (socp_check_chan_type(chan_type, SOCP_CODER_DEST_CHAN) != BSP_OK) {
        return;
    }

    if (SOCP_DEST_DSM_DISABLE == b_enable) {
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id + 16, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id + 16, 1, 1);

        g_socp_enable_state[real_chan_id] = SOCP_DEST_DSM_DISABLE;
    } else if (SOCP_DEST_DSM_ENABLE == b_enable) {
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, real_chan_id, 1, 0);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id, 1, 0);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id + 16, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id + 16, 1, 0);

        g_socp_enable_state[real_chan_id] = SOCP_DEST_DSM_ENABLE;
    } else {
        ;
    }
}

/*
 * �� �� ��: bsp_socp_data_send_continue
 * ��������: socp����Ŀ�Ķ������ϱ�ʹ�ܣ���readdone�е���
 *           ����diag����״̬�ж��Ƿ��ϱ�
 * ע    ��: �ú�������ʱ����Ҫ�����߱�֤ͬ��
 * �������: enc_dst_chan_id: ����Ŀ�Ķ�ͨ����
 *           b_enable: �ж�ʹ��
 * �������: ��
 * �� �� ֵ: ��
 */
void bsp_socp_data_send_continue(u32 real_chan_id)
{
    if (SOCP_DEST_DSM_ENABLE == g_socp_enable_state[real_chan_id]) {
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, real_chan_id, 1, 0);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id, 1, 0);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id + 16, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id + 16, 1, 0);
    }
}
/*
 * �� �� ��: bsp_socp_data_send_manager
 * ��������: socp����Ŀ�Ķ��ϱ�����
 * �������: enc_dst_chan_id: ����Ŀ�Ķ�ͨ����
 *           b_enable: �ж�ʹ��
 * �������: ��
 * �� �� ֵ: ��
 */
void bsp_socp_data_send_manager(u32 enc_dst_chan_id, u32 b_enable)
{
    unsigned long lock_flag;
    u32 real_chan_id;
    u32 chan_type;

    real_chan_id = SOCP_REAL_CHAN_ID(enc_dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(enc_dst_chan_id);

    if (socp_check_chan_type(chan_type, SOCP_CODER_DEST_CHAN) != BSP_OK) {
        return;
    }

    if ((SOCP_DEST_DSM_DISABLE == b_enable) && (g_socp_enable_state[real_chan_id] == SOCP_DEST_DSM_ENABLE)) {
        spin_lock_irqsave(&lock, lock_flag);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id + 16, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id + 16, 1, 1);

        g_socp_enable_state[real_chan_id] = SOCP_DEST_DSM_DISABLE;

        spin_unlock_irqrestore(&lock, lock_flag);
    } else if ((SOCP_DEST_DSM_ENABLE == b_enable) && (g_socp_enable_state[real_chan_id] == SOCP_DEST_DSM_DISABLE)) {
        spin_lock_irqsave(&lock, lock_flag);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, real_chan_id, 1, 0);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id, 1, 0);
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, real_chan_id + 16, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, real_chan_id + 16, 1, 0);

        g_socp_enable_state[real_chan_id] = SOCP_DEST_DSM_ENABLE;

        spin_unlock_irqrestore(&lock, lock_flag);
    } else {
        ;
    }
}

/*
 * �� �� ��: socp_set_reg_wr_addr
 * ��������: ����ͨ������ַ�Ĵ����Ͷ�дָ��Ĵ���������������SOCP32λ��64λѰַ����
 * �������: chan_id: ͨ���ţ�����ͨ�����ͺ�ͨ��ID
 *           attr: ͨ�����ò���
 *           start: ͨ��buffer��ʼ��ַ
 *           end: ͨ��buffer������ַ
 * �������: ��
 * �� �� ֵ: ��
 */
void socp_set_reg_wr_addr(u32 chan_id, const void *attr, unsigned long start, unsigned long end)
{
    u32 real_chan_id = 0;
    u32 chan_type = 0;
    unsigned long rd_start = 0;
    unsigned long rd_end = 0;
    socp_enc_src_chan_s *pEncSrcChan = NULL;
    socp_encdst_chan_s *pEncDstChan = NULL;
    socp_decsrc_chan_s *dec_src_chan = NULL;
    socp_decdst_chan_s *dec_dst_chan = NULL;

    real_chan_id = SOCP_REAL_CHAN_ID(chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(chan_id);

#ifdef FEATURE_SOCP_ADDR_64BITS           // SOCP 64λѰַ
    if (chan_type == SOCP_CODER_SRC_CHAN)  // ����Դͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFADDR_L(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFADDR_H(real_chan_id), (u32)((u64)start >> 32));
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFWPTR(real_chan_id), 0);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFRPTR(real_chan_id), 0);
        if (SOCP_ENCSRC_CHNMODE_LIST == ((SOCP_CODER_SRC_CHAN_S *)attr)->eMode) {
            rd_start = (uintptr_t)((SOCP_CODER_SRC_CHAN_S *)attr)->sCoderSetSrcBuf.pucRDStart;
            rd_end = (uintptr_t)((SOCP_CODER_SRC_CHAN_S *)attr)->sCoderSetSrcBuf.pucRDEnd;
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQADDR_L(real_chan_id), (u32)rd_start);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQADDR_H(real_chan_id), (u32)((u64)rd_start >> 32));
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQRPTR(real_chan_id), 0);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQWPTR(real_chan_id), 0);
        }

        pEncSrcChan = &g_socp_stat.enc_src_chan[real_chan_id];
        pEncSrcChan->chan_mode = ((SOCP_CODER_SRC_CHAN_S *)attr)->eMode;
        pEncSrcChan->priority = ((SOCP_CODER_SRC_CHAN_S *)attr)->ePriority;
        pEncSrcChan->data_type = ((SOCP_CODER_SRC_CHAN_S *)attr)->eDataType;
        pEncSrcChan->data_type_en = ((SOCP_CODER_SRC_CHAN_S *)attr)->eDataTypeEn;
        pEncSrcChan->debug_en = ((SOCP_CODER_SRC_CHAN_S *)attr)->eDebugEn;
        pEncSrcChan->dst_chan_id = ((SOCP_CODER_SRC_CHAN_S *)attr)->u32DestChanID;
        pEncSrcChan->bypass_en = ((SOCP_CODER_SRC_CHAN_S *)attr)->u32BypassEn;
        pEncSrcChan->enc_src_buff.Start = start;
        pEncSrcChan->enc_src_buff.End = end;
        pEncSrcChan->enc_src_buff.write = 0;
        pEncSrcChan->enc_src_buff.read = 0;
        pEncSrcChan->enc_src_buff.length = end - start + 1;  //lint !e834
        pEncSrcChan->enc_src_buff.idle_size = 0;

        if (SOCP_ENCSRC_CHNMODE_LIST == ((SOCP_CODER_SRC_CHAN_S *)attr)->eMode) {
            pEncSrcChan->rd_buff.Start = rd_start;
            pEncSrcChan->rd_buff.End = rd_end;
            pEncSrcChan->rd_buff.write = 0;
            pEncSrcChan->rd_buff.read = 0;
            pEncSrcChan->rd_buff.length = rd_end - rd_start + 1;  //lint !e834
            pEncSrcChan->rd_threshold = ((SOCP_CODER_SRC_CHAN_S *)attr)->sCoderSetSrcBuf.u32RDThreshold;
        }

    } else if (chan_type == SOCP_CODER_DEST_CHAN)  // ����Ŀ��ͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFADDR_L(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFADDR_H(real_chan_id), (u32)((u64)start >> 32));
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(real_chan_id), 0);
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFWPTR(real_chan_id), 0);

        pEncDstChan = &g_socp_stat.enc_dst_chan[real_chan_id];
        pEncDstChan->enc_dst_buff.Start = start;
        pEncDstChan->enc_dst_buff.End = end;
        pEncDstChan->enc_dst_buff.read = 0;
        pEncDstChan->enc_dst_buff.write = 0;
        pEncDstChan->enc_dst_buff.length = end - start + 1;  //lint !e834
        pEncDstChan->buf_thrhold = ((SOCP_CODER_DEST_CHAN_S *)attr)->sCoderSetDstBuf.u32Threshold;
        pEncDstChan->threshold = ((SOCP_CODER_DEST_CHAN_S *)attr)->u32EncDstThrh;
        /* ������ͨ���Ѿ����� */
        pEncDstChan->set_state = SOCP_CHN_SET;
    } else if (chan_type == SOCP_DECODER_SRC_CHAN)  // ����Դͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR_L(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR_H(real_chan_id), (u32)((u64)start >> 32));
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFRPTR(real_chan_id), 0);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(real_chan_id), 0);

        dec_src_chan = &g_socp_stat.dec_src_chan[real_chan_id];
        dec_src_chan->chan_id = real_chan_id;
        dec_src_chan->data_type_en = ((SOCP_DECODER_SRC_CHAN_STRU *)attr)->eDataTypeEn;
        dec_src_chan->dec_src_buff.Start = start;
        dec_src_chan->dec_src_buff.End = end;
        dec_src_chan->dec_src_buff.length = end - start + 1;  //lint !e834
        dec_src_chan->dec_src_buff.read = 0;
        dec_src_chan->dec_src_buff.write = 0;
        dec_src_chan->set_state = SOCP_CHN_SET;
    } else if (chan_type == SOCP_DECODER_DEST_CHAN)  // ����Ŀ��ͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFADDR_L(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFADDR_H(real_chan_id), (u32)((u64)start >> 32));
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFWPTR(real_chan_id), 0);
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFRPTR(real_chan_id), 0);

        dec_dst_chan = &g_socp_stat.dec_dst_chan[real_chan_id];
        dec_dst_chan->data_type = ((SOCP_DECODER_DEST_CHAN_STRU *)attr)->eDataType;
        dec_dst_chan->dec_dst_buff.Start = start;
        dec_dst_chan->dec_dst_buff.End = end;
        dec_dst_chan->dec_dst_buff.length = end - start + 1;  //lint !e834
        dec_dst_chan->dec_dst_buff.read = 0;
        dec_dst_chan->dec_dst_buff.write = 0;
    }

#else  // SOCP 32λѰַ

    if (chan_type == SOCP_CODER_SRC_CHAN)  // ����Դͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFADDR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFWPTR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFRPTR(real_chan_id), (u32)start);
        if (SOCP_ENCSRC_CHNMODE_LIST == ((SOCP_CODER_SRC_CHAN_S *)attr)->eMode) {
            rd_start = (unsigned long)((SOCP_CODER_SRC_CHAN_S *)attr)->sCoderSetSrcBuf.pucRDStart;
            rd_end = (unsigned long)((SOCP_CODER_SRC_CHAN_S *)attr)->sCoderSetSrcBuf.pucRDEnd;
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQADDR(real_chan_id), (u32)rd_start);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQRPTR(real_chan_id), (u32)rd_start);
            SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQWPTR(real_chan_id), (u32)rd_start);
        }
        pEncSrcChan = &g_socp_stat.enc_src_chan[real_chan_id];
        pEncSrcChan->chan_mode = ((SOCP_CODER_SRC_CHAN_S *)attr)->eMode;
        pEncSrcChan->priority = ((SOCP_CODER_SRC_CHAN_S *)attr)->ePriority;
        pEncSrcChan->data_type = ((SOCP_CODER_SRC_CHAN_S *)attr)->eDataType;
        pEncSrcChan->data_type_en = ((SOCP_CODER_SRC_CHAN_S *)attr)->eDataTypeEn;
        pEncSrcChan->debug_en = ((SOCP_CODER_SRC_CHAN_S *)attr)->eDebugEn;
        pEncSrcChan->dst_chan_id = ((SOCP_CODER_SRC_CHAN_S *)attr)->u32DestChanID;
        pEncSrcChan->bypass_en = ((SOCP_CODER_SRC_CHAN_S *)attr)->u32BypassEn;
        pEncSrcChan->enc_src_buff.Start = start;
        pEncSrcChan->enc_src_buff.End = end;
        pEncSrcChan->enc_src_buff.write = start;
        pEncSrcChan->enc_src_buff.read = start;
        pEncSrcChan->enc_src_buff.length = end - start + 1;  //lint !e834
        pEncSrcChan->enc_src_buff.idle_size = 0;

        if (SOCP_ENCSRC_CHNMODE_LIST == ((SOCP_CODER_SRC_CHAN_S *)attr)->eMode) {
            pEncSrcChan->rd_buff.Start = rd_start;
            pEncSrcChan->rd_buff.End = rd_end;
            pEncSrcChan->rd_buff.write = rd_start;
            pEncSrcChan->rd_buff.read = rd_start;
            pEncSrcChan->rd_buff.length = rd_end - rd_start + 1;  //lint !e834
            pEncSrcChan->rd_threshold = ((SOCP_CODER_SRC_CHAN_S *)attr)->sCoderSetSrcBuf.u32RDThreshold;
        }
    } else if (chan_type == SOCP_CODER_DEST_CHAN)  // ����Ŀ��ͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFADDR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFWPTR(real_chan_id), (u32)start);

        pEncDstChan = &g_socp_stat.enc_dst_chan[real_chan_id];
        pEncDstChan->enc_dst_buff.Start = start;
        pEncDstChan->enc_dst_buff.End = end;
        pEncDstChan->enc_dst_buff.read = (u32)start;
        pEncDstChan->enc_dst_buff.write = (u32)start;
        pEncDstChan->enc_dst_buff.length = end - start + 1;  //lint !e834
        pEncDstChan->buf_thrhold = ((SOCP_CODER_DEST_CHAN_S *)attr)->sCoderSetDstBuf.u32Threshold;
        pEncDstChan->threshold = ((SOCP_CODER_DEST_CHAN_S *)attr)->u32EncDstThrh;

        /* ������ͨ���Ѿ����� */
        pEncDstChan->set_state = SOCP_CHN_SET;
    } else if (chan_type == SOCP_DECODER_SRC_CHAN)  // ����Դͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFADDR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFRPTR(real_chan_id), (u32)start);

        dec_src_chan = &g_socp_stat.dec_src_chan[real_chan_id];
        dec_src_chan->chan_id = real_chan_id;
        dec_src_chan->data_type_en = ((SOCP_DECODER_SRC_CHAN_STRU *)attr)->eDataTypeEn;
        dec_src_chan->dec_src_buff.Start = start;
        dec_src_chan->dec_src_buff.End = end;
        dec_src_chan->dec_src_buff.length = end - start + 1;  //lint !e834
        dec_src_chan->dec_src_buff.read = (u32)start;
        dec_src_chan->dec_src_buff.write = (u32)start;
        dec_src_chan->set_state = SOCP_CHN_SET;
    } else if (chan_type == SOCP_DECODER_DEST_CHAN)  // ����Ŀ��ͨ��
    {
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFRPTR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFADDR(real_chan_id), (u32)start);
        SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFWPTR(real_chan_id), (u32)start);

        dec_dst_chan = &g_socp_stat.dec_dst_chan[real_chan_id];
        dec_dst_chan->data_type = ((SOCP_DECODER_DEST_CHAN_STRU *)attr)->eDataType;
        dec_dst_chan->dec_dst_buff.Start = start;
        dec_dst_chan->dec_dst_buff.End = end;
        dec_dst_chan->dec_dst_buff.length = end - start + 1;  //lint !e834
        dec_dst_chan->dec_dst_buff.read = (u32)start;
        dec_dst_chan->dec_dst_buff.write = (u32)start;
    }
#endif
}

/*
 * �� �� ��: bsp_socp_coder_set_src_chan
 * ��������: ����Դͨ�����ú���
 * �������: src_attr     ����Դͨ�����ò���
 *           ulSrcChanID  ����Դͨ��ID
 * �������: ��
 * �� �� ֵ: ���뼰���óɹ����ı�ʶ��
 */
s32 bsp_socp_coder_set_src_chan(u32 src_chan_id, SOCP_CODER_SRC_CHAN_S *src_attr)
{
    unsigned long start = 0;
    unsigned long end = 0;
    unsigned long base_addr_rdstart = 0;
    unsigned long base_addr_rdend = 0;
    u32 buf_length = 0;
    u32 rd_buf_length = 0;
    u32 i;
    u32 real_chan_id;
    u32 src_chan_type;
    u32 dst_chan_id;
    u32 dst_chan_type;
    u32 reset_flag;
    s32 ret;

    g_socp_debug_info.socp_debug_gbl.alloc_enc_src_cnt++;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == src_attr) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }

    if ((ret = socp_check_chan_priority(src_attr->ePriority)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_data_type(src_attr->eDataType)) != BSP_OK) {
        return ret;
    }

    if ((ret = socp_check_data_type_en(src_attr->eDataTypeEn)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_enc_debug_en(src_attr->eDebugEn)) != BSP_OK) {
        return ret;
    }
    real_chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    src_chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    if ((ret = socp_check_chan_type(src_chan_type, SOCP_CODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }

    if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
        return ret;
    }
    dst_chan_id = SOCP_REAL_CHAN_ID(src_attr->u32DestChanID);
    dst_chan_type = SOCP_REAL_CHAN_TYPE(src_attr->u32DestChanID);
    if ((ret = socp_check_chan_type(dst_chan_type, SOCP_CODER_DEST_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(dst_chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
        return ret;
    }

    if ((SOCP_ENCSRC_CHNMODE_CTSPACKET != src_attr->eMode) && (SOCP_ENCSRC_CHNMODE_LIST != src_attr->eMode)) {
        socp_error("chnnel mode(%d) is invalid\n", src_attr->eMode);
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    /* ʹ�����ò����������� */
    /* �ж���ʼ��ַ�Ƿ�8�ֽڶ��� */
    start = (uintptr_t)src_attr->sCoderSetSrcBuf.pucInputStart;
    end = (uintptr_t)src_attr->sCoderSetSrcBuf.pucInputEnd;

    if (0 == start) {
        socp_error("start addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_8bytes_align(start)) != BSP_OK) {
        return ret;
    }
    if (0 == end) {
        socp_error("end addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_buf_addr(start, end)) != BSP_OK) {
        return ret;
    }
    buf_length = (u32)(end - start + 1);

    if ((ret = socp_check_8bytes_align(buf_length)) != BSP_OK) {
        return ret;
    }

    if (g_socp_version < SOCP_201_VERSION) {
        if (buf_length > SOCP_ENC_SRC_BUFLGTH_MAX) {
            socp_error("buffer length is too large\n");
            return BSP_ERR_SOCP_INVALID_PARA;
        }
    }

    /* �����������������������RDbuffer����ʼ��ַ�ͳ��� */
    if (SOCP_ENCSRC_CHNMODE_LIST == src_attr->eMode) {
        /* �ж�RDBuffer����ʼ��ַ�Ƿ�8�ֽڶ��� */
        base_addr_rdstart = (uintptr_t)src_attr->sCoderSetSrcBuf.pucRDStart;
        base_addr_rdend = (uintptr_t)src_attr->sCoderSetSrcBuf.pucRDEnd;

        if (0 == base_addr_rdstart) {
            socp_error("RD start addr is NULL\n");
            return BSP_ERR_SOCP_NULL;
        }
        if ((ret = socp_check_8bytes_align(base_addr_rdstart)) != BSP_OK) {
            return ret;
        }
        if (0 == base_addr_rdend) {
            socp_error("RD end addr is NULL\n");
            return BSP_ERR_SOCP_NULL;
        }
        if ((ret = socp_check_buf_addr(base_addr_rdstart, base_addr_rdend)) != BSP_OK) {
            return ret;
        }
        rd_buf_length = (u32)(base_addr_rdend - base_addr_rdstart + 1);

        if ((ret = socp_check_8bytes_align(rd_buf_length)) != BSP_OK) {
            return ret;
        }
        if (rd_buf_length > SOCP_ENC_SRC_RDLGTH_MAX) {
            socp_error("RD buffer length is too large\n");
            return BSP_ERR_SOCP_INVALID_PARA;
        }
    }

    /* ��λͨ�� */
    SOCP_REG_SETBITS(SOCP_REG_ENCRST, real_chan_id, 1, 1);

    /* �ȴ�ͨ������ */
    for (i = 0; i < SOCP_RESET_TIME; i++) {
        reset_flag = SOCP_REG_GETBITS(SOCP_REG_ENCRST, real_chan_id, 1);
        if (0 == reset_flag) {
            break;
        }
    }

    /* ���ñ���Դͨ������ */
    /*lint -save -e647*/
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 1, 2, src_attr->eMode);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 4, 4, src_attr->u32DestChanID);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 8, 2, src_attr->ePriority);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 10, 1, src_attr->u32BypassEn);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 11, 1, src_attr->eDataTypeEn);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 31, 1, src_attr->eDebugEn);
    SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 16, 8, src_attr->eDataType);
    /*lint -restore +e647*/

    /*lint -save -e845*/
    /*lint -save -e647*/
    if (g_socp_version >= SOCP_201_VERSION) {
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFCFG0(real_chan_id), buf_length);
    } else {
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG0(real_chan_id), 0, 27, buf_length);
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG0(real_chan_id), 27, 5, 0);
    }
    if (SOCP_ENCSRC_CHNMODE_LIST == src_attr->eMode) {
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_RDQCFG(real_chan_id), 0, 16, rd_buf_length);
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_RDQCFG(real_chan_id), 16, 16, 0);
    }
    /*lint -restore +e647*/
    /*lint -restore +e845*/

    socp_set_reg_wr_addr(real_chan_id, (void *)src_attr, start, end);

    /* ���ͨ��״̬ */
    g_socp_stat.enc_src_chan[real_chan_id].alloc_state = SOCP_CHN_ALLOCATED;
    g_socp_debug_info.socp_debug_gbl.alloc_enc_src_suc_cnt++;
    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_decoder_set_dest_chan
 * ��������: ����Ŀ��ͨ�����뼰���ú���
 * �������: attr           ����Ŀ��ͨ�����ò���
 *           pDestChanID     ��ʼ������Ŀ��ͨ��ID������ͨ��Դ��Ŀ��ID�ж�Ӧ��ϵ
 *                           SrcID = DestChanID%4
 * �������:
 * �� �� ֵ: ���뼰���óɹ����ı�ʶ��
 */
s32 bsp_socp_decoder_set_dest_chan(SOCP_DECODER_DST_ENUM_U32 dst_chan_id, SOCP_DECODER_DEST_CHAN_STRU *attr)
{
    unsigned long start;
    unsigned long end;
    u32 buf_thrhold;
    u32 buf_length;
    u32 chan_id;
    u32 src_chan_id;
    u32 chan_type;
    // socp_decdst_chan_s *p_chan;
    s32 ret;

    g_socp_debug_info.socp_debug_gbl.socp_alloc_dec_dst_cnt++;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == attr) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_data_type(attr->eDataType)) != BSP_OK) {
        return ret;
    }
    chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);
    if ((ret = socp_check_chan_type(chan_type, SOCP_DECODER_DEST_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECDST_CHN)) != BSP_OK) {
        return ret;
    }

    src_chan_id = SOCP_REAL_CHAN_ID(attr->u32SrcChanID);
    chan_type = SOCP_REAL_CHAN_TYPE(attr->u32SrcChanID);
    if ((ret = socp_check_chan_type(chan_type, SOCP_DECODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(src_chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
        return ret;
    }

    /* �ж�ͨ��ID��Ӧ��ϵ */
    if (src_chan_id != chan_id % 4) {
        socp_error("dest ID(%d) is not matching src ID(%d)!\n", chan_id, src_chan_id);
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    /* �жϸ����ĵ�ַ�ͳ����Ƿ�Ϊ���ֽڱ��� */
    start = (uintptr_t)attr->sDecoderDstSetBuf.pucOutputStart;
    end = (uintptr_t)attr->sDecoderDstSetBuf.pucOutputEnd;
    buf_thrhold = attr->sDecoderDstSetBuf.u32Threshold;

    if (0 == start) {
        socp_error("start addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_8bytes_align(start)) != BSP_OK) {
        return ret;
    }
    if (0 == end) {
        socp_error("end addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_buf_addr(start, end)) != BSP_OK) {
        return ret;
    }
    buf_length = (u32)(end - start + 1);

    if (0 == buf_thrhold) {
        socp_error("the buf_thrhold is 0\n");
        return BSP_ERR_SOCP_NULL;
    }
    if (buf_thrhold > SOCP_DEC_DST_TH_MAX) {
        socp_error("buffer thrh is too large!\n");
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    if ((ret = socp_check_8bytes_align(buf_length)) != BSP_OK) {
        return ret;
    }
    if (buf_length > SOCP_DEC_DST_BUFLGTH_MAX) {
        socp_error("buffer length is too large!\n");
        return BSP_ERR_SOCP_INVALID_PARA;
    }
    /*lint -save -e647*/
    SOCP_REG_SETBITS(SOCP_REG_DECDEST_BUFCFG(chan_id), 24, 8, attr->eDataType);
    SOCP_REG_SETBITS(SOCP_REG_DECDEST_BUFCFG(chan_id), 0, 16, buf_length);
    SOCP_REG_SETBITS(SOCP_REG_DECDEST_BUFCFG(chan_id), 16, 8, buf_thrhold);

    socp_set_reg_wr_addr(dst_chan_id, (void *)attr, start, end);

    /* �����жϣ��ٴ��ж� */
    SOCP_REG_SETBITS(SOCP_REG_DEC_RAWINT0, chan_id, 1, 1);
    SOCP_REG_SETBITS(SOCP_REG_DEC_CORE0MASK0, chan_id, 1, 0);
    SOCP_REG_SETBITS(SOCP_REG_DEC_RAWINT2, chan_id, 1, 1);
    SOCP_REG_SETBITS(SOCP_REG_DEC_CORE0MASK2, chan_id, 1, 0);
    /*lint -restore +e647*/

    /* ���ͨ������״̬ */
    g_socp_stat.dec_dst_chan[chan_id].alloc_state = SOCP_CHN_ALLOCATED;
    g_socp_debug_info.socp_debug_gbl.socp_alloc_dec_dst_suc_cnt++;

    return BSP_OK;
}

static s32 socp_encdst_param_check(SOCP_CODER_DEST_CHAN_S *dst_attr)
{
    unsigned long start;
    unsigned long end;
    u32 buf_thrhold;
    u32 buf_length;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == dst_attr) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }

    start = (uintptr_t)dst_attr->sCoderSetDstBuf.pucOutputStart;
    end = (uintptr_t)dst_attr->sCoderSetDstBuf.pucOutputEnd;
    buf_thrhold = dst_attr->sCoderSetDstBuf.u32Threshold;

    if (0 == start) {
        socp_error("start addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_8bytes_align(start)) != BSP_OK) {
        return ret;
    }
    if (0 == end) {
        socp_error("end addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_buf_addr(start, end)) != BSP_OK) {
        return ret;
    }
    buf_length = (u32)(end - start + 1);

    if (0 == buf_thrhold) {
        socp_error("the buf_thrhold is 0\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_8bytes_align(buf_length)) != BSP_OK) {
        return ret;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_coder_set_dest_chan_attr
 * ��������: ����Ŀ��ͨ�����ú���
 * �������: dst_chan_id      ����Ŀ��ͨ��ID
 *           dst_attr          ����Ŀ��ͨ�����ò���
 * �������: ��
 * �� �� ֵ: ���óɹ����ı�ʶ��
 */
s32 bsp_socp_coder_set_dest_chan_attr(u32 dst_chan_id, SOCP_CODER_DEST_CHAN_S *dst_attr)
{
    unsigned long start;
    unsigned long end;
    u32 buf_thrhold;
    u32 buf_length;
    u32 chan_id;
    u32 chan_type;
    u32 threshold;
    u32 u32TimeoutMode;
    s32 ret;

    g_socp_debug_info.socp_debug_gbl.socp_set_enc_dst_cnt++;

    ret = socp_encdst_param_check(dst_attr);
    if (ret) {
        return ret;
    }

    chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);
    if ((ret = socp_check_chan_type(chan_type, SOCP_CODER_DEST_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
        return ret;
    }

    start = (uintptr_t)dst_attr->sCoderSetDstBuf.pucOutputStart;
    end = (uintptr_t)dst_attr->sCoderSetDstBuf.pucOutputEnd;
    buf_thrhold = dst_attr->sCoderSetDstBuf.u32Threshold;
    threshold = dst_attr->u32EncDstThrh;
    u32TimeoutMode = dst_attr->u32EncDstTimeoutMode;
    buf_length = (u32)(end - start + 1);

    /* ����������������ٴ�����,ͨ����λ֮��ֻ����һ�� */
    /* ʹ�����ò����������� */
    /*lint -save -e647*/
    if (SOCP_CHN_SET == g_socp_stat.enc_dst_chan[chan_id].set_state) {
        socp_error("channel 0x%x can't be set twice!\n", chan_id);
        return BSP_ERR_SOCP_SET_FAIL;
    }
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFCFG(chan_id), buf_length);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFTHRESHOLD(chan_id), (buf_thrhold * 1024));

    SOCP_REG_SETBITS(SOCP_REG_ENCDEST_BUFTHRH(chan_id), 0, 31, threshold);

    if (g_socp_version < SOCP_206_VERSION) {
        u32TimeoutMode = SOCP_TIMEOUT_TRF;
    }

    if (SOCP_TIMEOUT_TRF_LONG == u32TimeoutMode) {
        SOCP_REG_SETBITS(SOCP_REG_GBLRST, 4, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 3, 1, 1);
        bsp_socp_set_timeout(SOCP_TIMEOUT_TRF_LONG, SOCP_TIMEOUT_TRF_LONG_MIN); /* �����ϱ�:����ʱ��ֵ=10ms */
    } else if ((SOCP_TIMEOUT_TRF_SHORT == u32TimeoutMode)) {
        SOCP_REG_SETBITS(SOCP_REG_GBLRST, 4, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 3, 1, 0);
        bsp_socp_set_timeout(SOCP_TIMEOUT_TRF_SHORT, SOCP_TIMEOUT_TRF_SHORT_VAL); /* �̳�ʱ��ֵ=10ms */
    } else {
        SOCP_REG_SETBITS(SOCP_REG_GBLRST, 4, 1, 0);
    }

    socp_set_reg_wr_addr(dst_chan_id, (void *)dst_attr, start, end);

    bsp_socp_encdst_dsm_init(dst_chan_id, SOCP_DEST_DSM_DISABLE);

    /*lint -restore +e647*/

    g_socp_debug_info.socp_debug_gbl.socp_set_enc_dst_suc_cnt++;

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_decoder_set_src_chan_attr
 * ��������: ����Դͨ�����ú���
 * �������: src_chan_id    ����Դͨ��ID
 *           input_attr      ����Դͨ�����ò���
 * �������:
 * �� �� ֵ: ���óɹ����ı�ʶ��
 */
s32 bsp_socp_decoder_set_src_chan_attr(u32 src_chan_id, SOCP_DECODER_SRC_CHAN_STRU *input_attr)
{
    unsigned long start;
    unsigned long end;
    u32 buf_length = 0;
    u32 chan_id;
    u32 chan_type;
    u32 i;
    u32 reset_flag;
    // socp_decsrc_chan_s *dec_src_chan;
    s32 ret;

    g_socp_debug_info.socp_debug_gbl.socp_set_dec_src_cnt++;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == input_attr) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);
    if ((ret = socp_check_chan_type(chan_type, SOCP_DECODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_data_type_en(input_attr->eDataTypeEn)) != BSP_OK) {
        return ret;
    }

    start = (uintptr_t)input_attr->sDecoderSetSrcBuf.pucInputStart;
    end = (uintptr_t)input_attr->sDecoderSetSrcBuf.pucInputEnd;

    if (0 == start) {
        socp_error("start addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_8bytes_align(start)) != BSP_OK) {
        return ret;
    }
    if (0 == end) {
        socp_error("end addr is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    if ((ret = socp_check_buf_addr(start, end)) != BSP_OK) {
        return ret;
    }
    buf_length = (u32)(end - start + 1);

    if ((ret = socp_check_8bytes_align(buf_length)) != BSP_OK) {
        return ret;
    }
    if (buf_length > SOCP_DEC_SRC_BUFLGTH_MAX) {
        socp_error("buffer length is too large!\n");
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    if (SOCP_CHN_SET == g_socp_stat.dec_src_chan[chan_id].set_state) {
        socp_error("channel 0x%x has been configed!\n", chan_id);
        return BSP_ERR_SOCP_DECSRC_SET;
    }

    /* ���ȸ�λͨ�� */
    SOCP_REG_SETBITS(SOCP_REG_DECRST, chan_id, 1, 1);

    /* �ȴ�ͨ����λ״̬���� */
    for (i = 0; i < SOCP_RESET_TIME; i++) {
        reset_flag = SOCP_REG_GETBITS(SOCP_REG_DECRST, chan_id, 1);
        if (0 == reset_flag) {
            break;
        }
    }
    /*lint -save -e647*/
    SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(chan_id), 0, 16, buf_length);
    SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(chan_id), 31, 1, input_attr->eDataTypeEn);
    /*lint -restore +e647*/

    socp_set_reg_wr_addr(src_chan_id, (void *)input_attr, start, end);

    g_socp_debug_info.socp_debug_gbl.socp_set_dec_src_suc_cnt++;

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_decoder_get_err_cnt
 * ��������: ����ͨ���л�ȡ�����������
 * �������: chan_id       ����ͨ��ID
 * �������: err_cnt         ����ͨ����������ṹ��ָ��
 * �� �� ֵ: ��ȡ�ɹ����ı�ʶ��
 */
s32 bsp_socp_decoder_get_err_cnt(u32 dst_chan_id, SOCP_DECODER_ERROR_CNT_STRU *err_cnt)
{
    u32 chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �ж�ͨ��ID�Ƿ���Ч */
    chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);
    if ((ret = socp_check_chan_type(chan_type, SOCP_DECODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_decsrc_set(chan_id)) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == err_cnt) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }

    /* �ж�ͨ���Ƿ�򿪣�������Ϊdebugģʽ */
    if (g_socp_stat.dec_src_chan[chan_id].chan_en) {
        /*lint -save -e647*/
        err_cnt->u32PktlengthCnt = SOCP_REG_GETBITS(SOCP_REG_DEC_BUFSTAT0(chan_id), 16, 16);
        err_cnt->u32CrcCnt = SOCP_REG_GETBITS(SOCP_REG_DEC_BUFSTAT0(chan_id), 0, 16);
        err_cnt->u32DataTypeCnt = SOCP_REG_GETBITS(SOCP_REG_DEC_BUFSTAT1(chan_id), 16, 16);
        err_cnt->u32HdlcHeaderCnt = SOCP_REG_GETBITS(SOCP_REG_DEC_BUFSTAT1(chan_id), 0, 16);
        /*lint -restore +e647*/
    } else {
        socp_crit("debug mode is closed!\n");
        return BSP_ERR_SOCP_SET_FAIL;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_set_timeout
 * ��������: ��ʱ��ֵ���ú���
 * �������:   time_out_en          ���ö���ѡ��ʹ��
 *             time_out        ��ʱ��ֵ
 * �������:
 * �� �� ֵ: ���óɹ����ı�ʶ��
 */
s32 bsp_socp_set_timeout(SOCP_TIMEOUT_EN_ENUM_UIN32 time_out_en, u32 time_out)
{
    u32 u32newtime;
    u32 temp;

#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)
    decode_timeout_module_e decode_timeout_module = DECODE_TIMEOUT_INT_TIMEOUT;
#endif

    u32newtime = ((g_socp_version == SOCP_203_VERSION) || (g_socp_version == SOCP_204_VERSION)) ? SOCP_CLK_RATIO(time_out)
                                                                                            : time_out;

    switch (time_out_en) {
        case SOCP_TIMEOUT_BUFOVF_DISABLE: {
            SOCP_REG_SETBITS(SOCP_REG_BUFTIMEOUT, 31, 1, 0);
            break;
        }
        case SOCP_TIMEOUT_BUFOVF_ENABLE: {
            SOCP_REG_SETBITS(SOCP_REG_BUFTIMEOUT, 31, 1, 1);
            /* ���ӻ���ķ��� */
            SOCP_REG_SETBITS(SOCP_REG_BUFTIMEOUT, 0, 16, u32newtime);
            break;
        }

        case SOCP_TIMEOUT_TRF: {
            if (SOCP_REG_GETBITS(SOCP_REG_GBLRST, 4, 1)) {
                SOCP_REG_SETBITS(SOCP_REG_GBLRST, 4, 1, 0);
            }

#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)

            decode_timeout_module = (decode_timeout_module_e)SOCP_REG_GETBITS(SOCP_REG_GBLRST, 1, 1);
            if (decode_timeout_module == DECODE_TIMEOUT_INT_TIMEOUT) {
                if (u32newtime > 0xFF) {
                    socp_error("the value is too large!\n");
                    return BSP_ERR_SOCP_INVALID_PARA;
                }

                SOCP_REG_WRITE(SOCP_REG_INTTIMEOUT, u32newtime);
            } else {
                SOCP_REG_WRITE(SOCP_REG_INTTIMEOUT, u32newtime);
            }

#else
            SOCP_REG_WRITE(SOCP_REG_INTTIMEOUT, u32newtime);
#endif
            break;
        }

        case SOCP_TIMEOUT_TRF_LONG:
            /* fall through */
        case SOCP_TIMEOUT_TRF_SHORT: {
            if (0 == SOCP_REG_GETBITS(SOCP_REG_GBLRST, 4, 1)) {
                SOCP_REG_SETBITS(SOCP_REG_GBLRST, 4, 1, 1);
            }

            if (g_socp_version >= SOCP_206_VERSION) {
                if (SOCP_TIMEOUT_TRF_LONG == time_out_en) {
                    if (u32newtime > 0xffffff) {
                        socp_error("the value is too large!\n");
                        return BSP_ERR_SOCP_INVALID_PARA;
                    }
                    SOCP_REG_READ(SOCP_REG_INTTIMEOUT, temp);
                    u32newtime = (u32newtime << 8) | (temp & 0xff);
                    SOCP_REG_WRITE(SOCP_REG_INTTIMEOUT, u32newtime);
                } else {
                    if (u32newtime > 0xff) {
                        socp_error("the value is too large!\n");
                        return BSP_ERR_SOCP_INVALID_PARA;
                    }
                    SOCP_REG_READ(SOCP_REG_INTTIMEOUT, temp);
                    u32newtime = (temp & 0xffffff00) | u32newtime;
                    SOCP_REG_WRITE(SOCP_REG_INTTIMEOUT, u32newtime);
                }
            } else {
                socp_error("This version does not support the function.\n");
            }

            break;
        }

#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)
        case SOCP_TIMEOUT_DECODE_TRF: {
            decode_timeout_module = (decode_timeout_module_e)SOCP_REG_GETBITS(SOCP_REG_GBLRST, 1, 1);
            if (decode_timeout_module == DECODE_TIMEOUT_INT_TIMEOUT) {
                return BSP_ERR_SOCP_INVALID_PARA;
            }

            SOCP_REG_WRITE(SOCP_REG_DEC_INT_TIMEOUT, u32newtime);
            break;
        }
#endif

        default: {
            socp_error("invalid timeout choice type!\n");
            return BSP_ERR_SOCP_SET_FAIL;
        }
    }

    return BSP_OK;
}

/*
 * �� �� ��  : bsp_socp_set_dec_pkt_lgth
 * ��������  : ��ʱ��ֵ���ú���
 * �������  :   pkt_length          ������������ò����ṹ��
 * �������  :
 * �� �� ֵ  : ���óɹ����ı�ʶ��
 */
/* cov_verified_start */
s32 bsp_socp_set_dec_pkt_lgth(SOCP_DEC_PKTLGTH_STRU *pkt_length)
{
    u32 pkt_max_len;
    u32 pkt_min_len;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }
    /* �жϲ�����Ч�� */
    if (NULL == pkt_length) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    pkt_max_len = pkt_length->u32PktMax;
    pkt_min_len = pkt_length->u32PktMin;

    if (0 == pkt_max_len) {
        socp_error("the pkt_max_len is 0.\n");
        return BSP_ERR_SOCP_NULL;
    }
    if (pkt_max_len * 1024 > SOCP_DEC_MAXPKT_MAX) {
        socp_error("pkt_max_len 0x%x is too large!\n", pkt_max_len);
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    if (pkt_min_len > SOCP_DEC_MINPKT_MAX) {
        socp_error("pkt_min_len 0x%x is too large!\n", pkt_min_len);
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    /* ���ý���ͨ·���������üĴ��� */
    SOCP_REG_SETBITS(SOCP_REG_DEC_PKTLEN, 0, 12, pkt_max_len);
    SOCP_REG_SETBITS(SOCP_REG_DEC_PKTLEN, 12, 5, pkt_min_len);

    return BSP_OK;
}
/* cov_verified_stop */

/*
 * �� �� ��: bsp_socp_set_debug
 * ��������: ���ý���Դͨ��debugģʽ����
 * �������: dec_chan_id  ����Դͨ��ID
 *           debug_en    debugģʽʹ�ܱ�ʶ
 * �������:
 * �� �� ֵ: ���óɹ����ı�ʶ��
 */
s32 bsp_socp_set_debug(u32 dec_chan_id, u32 debug_en)
{
    u32 chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �ж�ͨ��ID�Ƿ���Ч */
    chan_id = SOCP_REAL_CHAN_ID(dec_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dec_chan_id);
    if ((ret = socp_check_chan_type(chan_type, SOCP_DECODER_SRC_CHAN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_decsrc_set(chan_id)) != BSP_OK) {
        return ret;
    }

    /* �жϸ�ͨ����ģʽ��û�д򿪵Ļ����������� */
    if (g_socp_stat.dec_src_chan[chan_id].chan_en) {
        socp_error("decoder channel is open, can't set debug bit\n");
        return BSP_ERR_SOCP_SET_FAIL;
    } else {
        /*lint -save -e647*/
        SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(chan_id), 29, 1, debug_en);
        /*lint -restore +e647*/
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_free_channel
 * ��������: ͨ���ͷź���
 * �������: chan_id       �����ͨ��ָ��
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
s32 bsp_socp_free_channel(u32 chan_id)
{
    u32 real_chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }
    /* �ж�ͨ��ID�Ƿ���Ч */
    real_chan_id = SOCP_REAL_CHAN_ID(chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(chan_id);

    if (SOCP_CODER_SRC_CHAN == chan_type) {
        socp_enc_src_chan_s *p_chan = NULL;

        if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(real_chan_id)) != BSP_OK) {
            return ret;
        }

        p_chan = &g_socp_stat.enc_src_chan[real_chan_id];
        if (SOCP_CHN_ENABLE == p_chan->chan_en) {
            socp_error("chan 0x%x is running!\n", chan_id);
            return BSP_ERR_SOCP_CHAN_RUNNING;
        }

        (void)socp_reset_enc_chan(real_chan_id);

        p_chan->alloc_state = SOCP_CHN_UNALLOCATED;

        g_socp_debug_info.socp_debug_encsrc.free_enc_src_cnt[real_chan_id]++;
    } else if (SOCP_DECODER_DEST_CHAN == chan_type) {
        if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECDST_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decdst_alloc(real_chan_id)) != BSP_OK) {
            return ret;
        }

        /* ��������������Ч */
        /*lint -save -e647*/
        SOCP_REG_SETBITS(SOCP_REG_DECDEST_BUFCFG(real_chan_id), 24, 8, 0xff);
        /*lint -restore +e647*/

        g_socp_stat.dec_dst_chan[real_chan_id].alloc_state = SOCP_CHN_UNALLOCATED;

        g_socp_debug_info.socp_debug_dec_dst.socp_free_decdst_cnt[real_chan_id]++;
    } else {
        socp_error("invalid chan type 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_chan_soft_reset
 * ��������: ͨ����λ����
 * �������: chan_id       �����ͨ��ID
 * �������: ��
 * �� �� ֵ: �ͷųɹ����ı�ʶ��
 */
s32 bsp_socp_chan_soft_reset(u32 chan_id)
{
    u32 real_chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �ж�ͨ��ID�Ƿ���Ч */
    real_chan_id = SOCP_REAL_CHAN_ID(chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(chan_id);
    /* ����Դͨ����λ��ֻ����̬���估LTE DSP/BBPͨ�� */
    /* ����ͨ���ĸ�λ���������漰����Ĵ����������ӽӿ� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(real_chan_id)) != BSP_OK) {
            return ret;
        }
        (void)socp_reset_enc_chan(chan_id);
        g_socp_debug_info.socp_debug_encsrc.soft_reset_enc_src_cnt[real_chan_id]++;
    } else if (SOCP_DECODER_SRC_CHAN == chan_type) {
        if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decsrc_set(real_chan_id)) != BSP_OK) {
            return ret;
        }

        (void)socp_reset_dec_chan(real_chan_id);

        g_socp_debug_info.socp_debug_dec_src.socp_soft_reset_decsrc_cnt[real_chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

s32 socp_dst_channel_enable(u32 dst_chan_id)
{
    s32 ret;
    u32 real_chan_id;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }
    real_chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);

    if (g_socp_version >= SOCP_206_VERSION) {
        SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(real_chan_id), 0, 1, 1);  // ��������Ŀ��ͨ��
    }
    return BSP_OK;
}
s32 socp_dst_channel_disable(u32 dst_chan_id)
{
    s32 ret;
    u32 real_chan_id;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    real_chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    if (g_socp_version >= SOCP_206_VERSION) {
        SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(real_chan_id), 0, 1, 0);  // ֹͣ����Ŀ��ͨ��
    }

    return BSP_OK;
}
s32 bsp_socp_dst_trans_id_disable(u32 dst_chan_id)
{
    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_start
 * ��������: ������߽�����������
 * �������: src_chan_id      ͨ��ID
 * �������:
 * �� �� ֵ: �����ɹ����ı�ʶ��
 */
s32 bsp_socp_start(u32 src_chan_id)
{
    u32 real_chan_id;
    u32 chan_type;
    u32 logic = (u32)1;
    u32 dst_chan_id;
    u32 i;
    u32 int_id_mask = 0;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }
    /* �ж�ͨ��ID�Ƿ���Ч */
    real_chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        if (real_chan_id < SOCP_MAX_ENCSRC_CHN) {
            if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
                return ret;
            }
            if ((ret = socp_check_encsrc_alloc(real_chan_id)) != BSP_OK) {
                return ret;
            }
        } else {
            socp_error("enc src 0x%x is invalid!\n", src_chan_id);
            return BSP_ERR_SOCP_INVALID_CHAN;
        }
        /*lint -save -e647*/
        dst_chan_id = SOCP_REG_GETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 4, 4);

        if (SOCP_CHN_SET != g_socp_stat.enc_dst_chan[dst_chan_id].set_state) {
            socp_error("enc dst chan is invalid!\n");
            return BSP_ERR_SOCP_DEST_CHAN;
        }

        /* �����жϣ��ٴ��ж� */
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT1, real_chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_APP_MASK1, real_chan_id, 1, 0);

        if (SOCP_ENCSRC_CHNMODE_LIST == g_socp_stat.enc_src_chan[real_chan_id].chan_mode) {
            SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT3, real_chan_id, 1, 1);
        }

        /* ���ô�״̬ */
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 0, 1, 1);
        if (real_chan_id < SOCP_MAX_ENCSRC_CHN) {
            g_socp_stat.enc_src_chan[real_chan_id].chan_en = SOCP_CHN_ENABLE;
            g_socp_debug_info.socp_debug_encsrc.start_enc_src_cnt[real_chan_id]++;
        }
    } else if (SOCP_DECODER_SRC_CHAN == chan_type) {
        if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decsrc_set(real_chan_id)) != BSP_OK) {
            return ret;
        }

        /* ��rd����ж� */
        if (SOCP_DECSRC_CHNMODE_LIST == g_socp_stat.dec_src_chan[real_chan_id].chan_mode) {
            SOCP_REG_SETBITS(SOCP_REG_DEC_RAWINT1, real_chan_id, 1, 1);
            SOCP_REG_SETBITS(SOCP_REG_DEC_MASK1, real_chan_id, 1, 0);
        }

        for (i = 1; i < SOCP_DEC_SRCINT_NUM; i++) {
            int_id_mask = SOCP_REG_GETBITS(SOCP_REG_DEC_RAWINT1, i * 4, 4);
            int_id_mask |= 1 << real_chan_id;
            SOCP_REG_SETBITS(SOCP_REG_DEC_RAWINT1, i * 4, 4, int_id_mask);

            int_id_mask = SOCP_REG_GETBITS(SOCP_REG_DEC_MASK1, i * 4, 4);
            int_id_mask &= ~(logic << real_chan_id);
            SOCP_REG_SETBITS(SOCP_REG_DEC_MASK1, i * 4, 4, int_id_mask);
        }

        /* ���ô�״̬ */
        SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(real_chan_id), 30, 1, 1);
        g_socp_stat.dec_src_chan[real_chan_id].chan_en = SOCP_CHN_ENABLE;

        g_socp_debug_info.socp_debug_dec_src.socp_soft_start_decsrc_cnt[real_chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }
    /*lint -restore +e647*/
    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_stop
 * ��������: ������߽���ֹͣ����
 * �������: src_chan_id      ͨ��ID
 * �������:
 * �� �� ֵ: ֹͣ�ɹ����ı�ʶ��
 */

s32 bsp_socp_stop(u32 src_chan_id)
{
    u32 real_chan_id;
    u32 chan_type;
    u32 int_id_mask = 0;
    u32 i;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �ж�ͨ��ID�Ƿ���Ч */
    real_chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        if (real_chan_id < SOCP_MAX_ENCSRC_CHN) {
            if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
                return ret;
            }
            if ((ret = socp_check_encsrc_alloc(real_chan_id)) != BSP_OK) {
                return ret;
            }
        } else {
            socp_error("enc src 0x%x is invalid!\n", src_chan_id);
            return BSP_ERR_SOCP_INVALID_CHAN;
        }
        /*lint -save -e647*/
        SOCP_REG_SETBITS(SOCP_REG_APP_MASK1, real_chan_id, 1, 1);
        if (SOCP_ENCSRC_CHNMODE_LIST == g_socp_stat.enc_src_chan[real_chan_id].chan_mode) {
            SOCP_REG_SETBITS(SOCP_REG_APP_MASK3, real_chan_id, 1, 1);
        }

        /* ����ͨ���ر�״̬ */
        SOCP_REG_SETBITS(SOCP_REG_ENCSRC_BUFCFG1(real_chan_id), 0, 1, 0);
        if (real_chan_id < SOCP_MAX_ENCSRC_CHN) {
            g_socp_stat.enc_src_chan[real_chan_id].chan_en = SOCP_CHN_DISABLE;
            g_socp_debug_info.socp_debug_encsrc.stop_enc_src_cnt[real_chan_id]++;
        }
    } else if (SOCP_DECODER_SRC_CHAN == chan_type) {
        if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decsrc_set(real_chan_id)) != BSP_OK) {
            return ret;
        }

        /* �ر��ж� */
        if (SOCP_DECSRC_CHNMODE_LIST == g_socp_stat.dec_src_chan[real_chan_id].chan_mode) {
            SOCP_REG_SETBITS(SOCP_REG_DEC_CORE0MASK0, real_chan_id, 1, 1);
        }

        for (i = 1; i < SOCP_DEC_SRCINT_NUM; i++) {
            int_id_mask = SOCP_REG_GETBITS(SOCP_REG_DEC_MASK1, i * 4, 4);
            int_id_mask |= 1 << real_chan_id;
            SOCP_REG_SETBITS(SOCP_REG_DEC_CORE0MASK0, i * 4, 4, int_id_mask);
        }

        /* ����ͨ���ر�״̬ */
        SOCP_REG_SETBITS(SOCP_REG_DECSRC_BUFCFG0(real_chan_id), 30, 1, 0);
        g_socp_stat.dec_src_chan[real_chan_id].chan_en = SOCP_CHN_DISABLE;

        g_socp_debug_info.socp_debug_dec_src.socp_stop_start_decsrc_cnt[real_chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }
    /*lint -restore +e647*/
    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_register_event_cb
 * ��������: �쳣�¼��ص�����ע�ắ��
 * �������: chan_id      ͨ��ID
 *           event_cb        �쳣�¼��Ļص�����
 * �������:
 * �� �� ֵ: ע��ɹ����ı�ʶ��
 */
s32 bsp_socp_register_event_cb(u32 chan_id, socp_event_cb event_cb)
{
    u32 real_chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* ��ȡͨ�����ͺ�ʵ�ʵ�ͨ��ID */
    real_chan_id = SOCP_REAL_CHAN_ID(chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(chan_id);

    switch (chan_type) {
        case SOCP_CODER_SRC_CHAN: /* ����Դͨ�� */
        {
            if (real_chan_id < SOCP_MAX_ENCSRC_CHN) {
                if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
                    return ret;
                }
                if ((ret = socp_check_encsrc_alloc(real_chan_id)) != BSP_OK) {
                    return ret;
                }
                g_socp_stat.enc_src_chan[real_chan_id].event_cb = event_cb;

                g_socp_debug_info.socp_debug_encsrc.reg_event_enc_src_cnt[real_chan_id]++;
            }
            break;
        }
        case SOCP_CODER_DEST_CHAN: /* ����Ŀ��ͨ�� */
        {
            if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
                return ret;
            }
            if ((ret = socp_check_encdst_set(real_chan_id)) != BSP_OK) {
                return ret;
            }

            g_socp_stat.enc_dst_chan[real_chan_id].event_cb = event_cb;

            g_socp_debug_info.socp_debug_enc_dst.socp_reg_event_encdst_cnt[real_chan_id]++;
            break;
        }
        case SOCP_DECODER_SRC_CHAN: /* ����Դͨ�� */
        {
            if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
                return ret;
            }
            if ((ret = socp_check_decsrc_set(real_chan_id)) != BSP_OK) {
                return ret;
            }

            g_socp_stat.dec_src_chan[real_chan_id].event_cb = event_cb;

            g_socp_debug_info.socp_debug_dec_src.socp_reg_event_decsrc_cnt[real_chan_id]++;
            break;
        }
        case SOCP_DECODER_DEST_CHAN: /* ����Ŀ��ͨ�� */
        {
            if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECDST_CHN)) != BSP_OK) {
                return ret;
            }
            if ((ret = socp_check_decdst_alloc(real_chan_id)) != BSP_OK) {
                return ret;
            }

            g_socp_stat.dec_dst_chan[real_chan_id].event_cb = event_cb;

            g_socp_debug_info.socp_debug_dec_dst.socp_reg_event_decdst_cnt[real_chan_id]++;
            break;
        }
        default: {
            socp_error("invalid chan type: 0x%x!\n", chan_type);
            return BSP_ERR_SOCP_INVALID_CHAN;
        }
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_get_write_buff
 * ��������: �ϲ��ȡд����buffer����
 * �������: src_chan_id    Դͨ��ID
 * �������: p_rw_buff           ��ȡ��buffer
 * �� �� ֵ: ��ȡ�ɹ����ı�ʶ��
 */
s32 bsp_socp_get_write_buff(u32 src_chan_id, SOCP_BUFFER_RW_STRU *p_rw_buff)
{
    u32 chan_id;
    u32 chan_type;
    u32 phy_addr;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == p_rw_buff) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }
    /* ���ʵ��ͨ��id */
    chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        g_socp_debug_info.socp_debug_encsrc.get_wbuf_enter_enc_src_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_encsrc_chan_id(chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(chan_id)) != BSP_OK) {
            return ret;
        }

        /* ���ݶ�дָ���ȡbuffer */
        SOCP_REG_READ(SOCP_REG_ENCSRC_BUFRPTR(chan_id), phy_addr);
        g_socp_stat.enc_src_chan[chan_id].enc_src_buff.read = phy_addr;
        SOCP_REG_READ(SOCP_REG_ENCSRC_BUFWPTR(chan_id), phy_addr);
        g_socp_stat.enc_src_chan[chan_id].enc_src_buff.write = phy_addr;
        socp_get_idle_buffer(&g_socp_stat.enc_src_chan[chan_id].enc_src_buff, p_rw_buff);
        g_socp_debug_info.socp_debug_encsrc.get_wbuf_suc_enc_src_cnt[chan_id]++;
    } else if (SOCP_DECODER_SRC_CHAN == chan_type) /* ����ͨ�� */
    {
        g_socp_debug_info.socp_debug_dec_src.socp_get_wbuf_decsrc_etr_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decsrc_set(chan_id)) != BSP_OK) {
            return ret;
        }
        SOCP_REG_READ(SOCP_REG_DECSRC_BUFRPTR(chan_id), phy_addr);
        g_socp_stat.dec_src_chan[chan_id].dec_src_buff.read = phy_addr;
        SOCP_REG_READ(SOCP_REG_DECSRC_BUFWPTR(chan_id), phy_addr);
        g_socp_stat.dec_src_chan[chan_id].dec_src_buff.write = phy_addr;
        socp_get_idle_buffer(&g_socp_stat.dec_src_chan[chan_id].dec_src_buff, p_rw_buff);
        g_socp_debug_info.socp_debug_dec_src.socp_get_wbuf_decsrc_suc_cnt[chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_write_done
 * ��������: д������ɺ���
 * �������: src_chan_id    Դͨ��ID
 *           write_size      д�����ݵĳ���
 * �������:
 * �� �� ֵ: ����������ı�ʶ��
 */
s32 bsp_socp_write_done(u32 src_chan_id, u32 write_size)
{
    u32 chan_id;
    u32 chan_type;
    u32 write_pad;
    SOCP_BUFFER_RW_STRU rw_buff;
    u32 phy_addr;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (0 == write_size) {
        socp_error("the write_size is 0\n");
        return BSP_ERR_SOCP_NULL;
    }

    /* ���ʵ��ͨ��id */
    chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        socp_enc_src_chan_s *p_chan = NULL;

        g_socp_debug_info.socp_debug_encsrc.write_done_enter_enc_src_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_encsrc_chan_id(chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(chan_id)) != BSP_OK) {
            return ret;
        }

        p_chan = &g_socp_stat.enc_src_chan[chan_id];
        write_pad = write_size % 8;
        if (0 != write_pad) {
            write_size += (8 - write_pad);
        }

        SOCP_REG_READ(SOCP_REG_ENCSRC_BUFWPTR(chan_id), phy_addr);
        p_chan->enc_src_buff.write = phy_addr;
        SOCP_REG_READ(SOCP_REG_ENCSRC_BUFRPTR(chan_id), phy_addr);
        p_chan->enc_src_buff.read = phy_addr;

        socp_get_idle_buffer(&p_chan->enc_src_buff, &rw_buff);

        if (rw_buff.u32Size + rw_buff.u32RbSize < write_size) {
            socp_error("rw_buff is not enough, write_size=0x%x\n", write_size);
            g_socp_debug_info.socp_debug_encsrc.write_done_fail_enc_src_cnt[chan_id]++;
            return BSP_ERR_SOCP_INVALID_PARA;
        }

        /* ���ö�дָ�� */
        socp_write_done(&p_chan->enc_src_buff, write_size);

        /* д��дָ�뵽дָ��Ĵ��� */
        phy_addr = p_chan->enc_src_buff.write;
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_BUFWPTR(chan_id), phy_addr);
        g_socp_debug_info.socp_debug_encsrc.write_done_suc_enc_src_cnt[chan_id]++;
    } else if (SOCP_DECODER_SRC_CHAN == chan_type) /* ����ͨ�� */
    {
        socp_decsrc_chan_s *p_chan = NULL;

        g_socp_debug_info.socp_debug_dec_src.socp_write_done_decsrc_etr_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECSRC_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decsrc_set(chan_id)) != BSP_OK) {
            return ret;
        }
        p_chan = &g_socp_stat.dec_src_chan[chan_id];
        SOCP_REG_READ(SOCP_REG_DECSRC_BUFWPTR(chan_id), phy_addr);
        p_chan->dec_src_buff.write = phy_addr;
        SOCP_REG_READ(SOCP_REG_DECSRC_BUFRPTR(chan_id), phy_addr);
        p_chan->dec_src_buff.read = phy_addr;
        socp_get_idle_buffer(&p_chan->dec_src_buff, &rw_buff);

        if (rw_buff.u32Size + rw_buff.u32RbSize < write_size) {
            socp_error("rw_buff is not enough, write_size=0x%x\n", write_size);
            g_socp_debug_info.socp_debug_dec_src.socp_write_done_decsrc_fail_cnt[chan_id]++;

            return BSP_ERR_SOCP_INVALID_PARA;
        }

        /* ���ö�дָ�� */
        socp_write_done(&p_chan->dec_src_buff, write_size);

        /* д��дָ�뵽дָ��Ĵ��� */
        phy_addr = p_chan->dec_src_buff.write;
        SOCP_REG_WRITE(SOCP_REG_DECSRC_BUFWPTR(chan_id), phy_addr);
        g_socp_debug_info.socp_debug_dec_src.socp_write_done_decsrc_suc_cnt[chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_register_rd_cb
 * ��������: RDbuffer�ص�����ע�ắ��
 * �������: src_chan_id    Դͨ��ID
 *           rd_cb            RDbuffer��ɻص�����
 * �������:
 * �� �� ֵ: ע��ɹ����ı�ʶ��
 */
s32 bsp_socp_register_rd_cb(u32 src_chan_id, socp_rd_cb rd_cb)
{
    u32 real_chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* ��ȡͨ�����ͺ�ʵ�ʵ�ͨ��ID */
    real_chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        if ((ret = socp_check_encsrc_chan_id(real_chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(real_chan_id)) != BSP_OK) {
            return ret;
        }

        if (SOCP_ENCSRC_CHNMODE_LIST == g_socp_stat.enc_src_chan[real_chan_id].chan_mode) {
            /* ���ö�Ӧͨ���Ļص����� */
            g_socp_stat.enc_src_chan[real_chan_id].rd_cb = rd_cb;
        } else {
            socp_error("invalid chan mode!\n");
            return BSP_ERR_SOCP_CHAN_MODE;
        }

        g_socp_debug_info.socp_debug_encsrc.reg_rd_cb_enc_src_cnt[real_chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

/*
 * �� �� ��  : bsp_socp_get_rd_buffer
 * ��������  : ��ȡRDbuffer����
 * �������  : src_chan_id    Դͨ��ID
 * �������  : p_rw_buff           ��ȡ��RDbuffer
 * �� �� ֵ  : ��ȡ�ɹ����ı�ʶ��
 */
/* cov_verified_start */
s32 bsp_socp_get_rd_buffer(u32 src_chan_id, SOCP_BUFFER_RW_STRU *p_rw_buff)
{
    u32 chan_id;
    u32 chan_type;
    u32 phy_addr;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == p_rw_buff) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }

    /* ���ʵ��ͨ��id */
    chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        g_socp_debug_info.socp_debug_encsrc.get_rd_buff_enc_src_enter_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_encsrc_chan_id(chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(chan_id)) != BSP_OK) {
            return ret;
        }

        /* ���ݶ�дָ���ȡbuffer */
        SOCP_REG_READ(SOCP_REG_ENCSRC_RDQRPTR(chan_id), phy_addr);
        g_socp_stat.enc_src_chan[chan_id].rd_buff.read = phy_addr;
        SOCP_REG_READ(SOCP_REG_ENCSRC_RDQWPTR(chan_id), phy_addr);
        g_socp_stat.enc_src_chan[chan_id].rd_buff.write = phy_addr;
        socp_get_data_buffer(&g_socp_stat.enc_src_chan[chan_id].rd_buff, p_rw_buff);
        g_socp_debug_info.socp_debug_encsrc.get_rd_buff_suc_enc_src_cnt[chan_id]++;
    } else {
        socp_error("invalid chan type(0x%x)\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_read_rd_done
 * ��������: ��ȡRDbuffer������ɺ���
 * �������: src_chan_id   Դͨ��ID
 *           rd_size      ��ȡ��RDbuffer���ݳ���
 * �������:
 * �� �� ֵ: ��ȡ�ɹ����ı�ʶ��
 */
s32 bsp_socp_read_rd_done(u32 src_chan_id, u32 rd_size)
{
    u32 chan_id;
    u32 chan_type;
    SOCP_BUFFER_RW_STRU rw_buff;
    u32 phy_addr;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (0 == rd_size) {
        socp_error("the rd_size is 0\n");
        return BSP_ERR_SOCP_NULL;
    }

    /* ���ʵ��ͨ��id */
    chan_id = SOCP_REAL_CHAN_ID(src_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(src_chan_id);

    /* ����ͨ�� */
    if (SOCP_CODER_SRC_CHAN == chan_type) {
        socp_enc_src_chan_s *p_chan = NULL;

        g_socp_debug_info.socp_debug_encsrc.read_rd_done_enter_enc_src_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_encsrc_chan_id(chan_id)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encsrc_alloc(chan_id)) != BSP_OK) {
            return ret;
        }

        p_chan = &g_socp_stat.enc_src_chan[chan_id];
        g_socp_stat.enc_src_chan[chan_id].last_rd_size = 0;

        /* ���ö�дָ�� */
        SOCP_REG_READ(SOCP_REG_ENCSRC_RDQWPTR(chan_id), phy_addr);
        p_chan->rd_buff.write = phy_addr;
        SOCP_REG_READ(SOCP_REG_ENCSRC_RDQRPTR(chan_id), phy_addr);
        p_chan->rd_buff.read = phy_addr;
        socp_get_data_buffer(&p_chan->rd_buff, &rw_buff);

        if (rw_buff.u32Size + rw_buff.u32RbSize < rd_size) {
            socp_error("rw_buff is not enough, rd_size=0x%x\n", rd_size);
            g_socp_debug_info.socp_debug_encsrc.read_rd_done_fail_enc_src_cnt[chan_id]++;

            return BSP_ERR_SOCP_INVALID_PARA;
        }

        socp_read_done(&p_chan->rd_buff, rd_size);

        /* д���ָ�뵽��ָ��Ĵ��� */
        phy_addr = p_chan->rd_buff.read;
        SOCP_REG_WRITE(SOCP_REG_ENCSRC_RDQRPTR(chan_id), phy_addr);
        g_socp_debug_info.socp_debug_encsrc.read_rd_suc_enc_src_done_cnt[chan_id]++;
    } else {
        socp_error("invalid chan type(0x%x)", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}
/* cov_verified_stop */

// ����Ŀ��ͨ��ר��
/*
 * �� �� ��: bsp_socp_register_read_cb
 * ��������: �����ݻص�����ע�ắ��
 * �������: dst_chan_id  Ŀ��ͨ����ID
 *           read_cb         �����ݻص�����
 * �������:
 * �� �� ֵ: ע��ɹ����ı�ʶ��
 */
s32 bsp_socp_register_read_cb(u32 dst_chan_id, socp_read_cb read_cb)
{
    u32 real_chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* ��ȡͨ�����ͺ�ʵ�ʵ�ͨ��ID */
    real_chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);

    if (SOCP_DECODER_DEST_CHAN == chan_type) /* ����ͨ�� */
    {
        if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_DECDST_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decdst_alloc(real_chan_id)) != BSP_OK) {
            return ret;
        }

        g_socp_stat.dec_dst_chan[real_chan_id].read_cb = read_cb;

        g_socp_debug_info.socp_debug_dec_dst.socp_reg_readcb_decdst_cnt[real_chan_id]++;
    } else if (SOCP_CODER_DEST_CHAN == chan_type) /* ����ͨ�� */
    {
        if ((ret = socp_check_chan_id(real_chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encdst_set(real_chan_id)) != BSP_OK) {
            return ret;
        }

        /* ���ö�Ӧͨ���Ļص����� */
        g_socp_stat.enc_dst_chan[real_chan_id].read_cb = read_cb;

        g_socp_debug_info.socp_debug_enc_dst.socp_reg_readcb_encdst_cnt[real_chan_id]++;
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_get_read_buff
 * ��������: ��ȡ������buffer����
 * �������: dst_chan_id  Ŀ��ͨ��buffer
 * �������: p_buffer        ��ȡ�Ķ�����buffer
 * �� �� ֵ: ��ȡ�ɹ����ı�ʶ��
 */
s32 bsp_socp_get_read_buff(u32 dst_chan_id, SOCP_BUFFER_RW_STRU *p_buffer)
{
    u32 chan_id;
    u32 chan_type;
    u32 phy_addr;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* �жϲ�����Ч�� */
    if (NULL == p_buffer) {
        socp_error("the parameter is NULL\n");
        return BSP_ERR_SOCP_NULL;
    }

    /* ���ʵ��ͨ��id */
    chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);
    p_buffer->u32Size = 0;
    p_buffer->u32RbSize = 0;

    if (SOCP_DECODER_DEST_CHAN == chan_type) /* ����ͨ�� */
    {
        g_socp_debug_info.socp_debug_dec_dst.socp_get_readbuf_decdst_etr_cnt[chan_id]++;

        /* ����ͨ��id */
        if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECDST_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_decdst_alloc(chan_id)) != BSP_OK) {
            return ret;
        }
        /* ���ݶ�дָ���ȡbuffer */
        SOCP_REG_READ(SOCP_REG_DECDEST_BUFRPTR(chan_id), phy_addr);
        g_socp_stat.dec_dst_chan[chan_id].dec_dst_buff.read = phy_addr;
        SOCP_REG_READ(SOCP_REG_DECDEST_BUFWPTR(chan_id), phy_addr);
        g_socp_stat.dec_dst_chan[chan_id].dec_dst_buff.write = phy_addr;
        socp_get_data_buffer(&g_socp_stat.dec_dst_chan[chan_id].dec_dst_buff, p_buffer);
        g_socp_debug_info.socp_debug_dec_dst.socp_get_readbuf_decdst_suc_cnt[chan_id]++;
    } else if (SOCP_CODER_DEST_CHAN == chan_type) {
        g_socp_debug_info.socp_debug_enc_dst.socp_get_read_buff_encdst_etr_cnt[chan_id]++;
        /* deflateʹ�ܻ�ȡdeflate buffer */
        if ((SOCP_COMPRESS == g_socp_stat.enc_dst_chan[chan_id].struCompress.bcompress) &&
            (g_socp_stat.enc_dst_chan[chan_id].struCompress.ops.getbuffer)) {
            return g_socp_stat.enc_dst_chan[chan_id].struCompress.ops.getbuffer(p_buffer);
        }

        /* ����ͨ��id */
        if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
            return ret;
        }
        if ((ret = socp_check_encdst_set(chan_id)) != BSP_OK) {
            return ret;
        }

        /* ���ݶ�дָ���ȡbuffer */
        SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(chan_id), phy_addr);
        g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff.read = phy_addr;
        SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(chan_id), phy_addr);
        g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff.write = phy_addr;
        socp_get_data_buffer(&g_socp_stat.enc_dst_chan[chan_id].enc_dst_buff, p_buffer);
        g_socp_debug_info.socp_debug_enc_dst.socp_get_read_buff_encdst_suc_cnt[chan_id]++;

    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return BSP_OK;
}

s32 socp_decode_read_data_done(u32 chan_id, u32 read_size)
{
    u32 trf_mask_reg = 0;
    s32 ret;
    u32 phy_addr;
    SOCP_BUFFER_RW_STRU rw_buff;
    socp_decdst_chan_s *p_chan = NULL;
    unsigned long lock_flag;

    g_socp_debug_info.socp_debug_dec_dst.socp_read_done_decdst_etr_cnt[chan_id]++;

    /* ����ͨ��id */
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_DECDST_CHN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_decdst_alloc(chan_id)) != BSP_OK) {
        return ret;
    }

    trf_mask_reg = SOCP_REG_DEC_CORE0MASK0;
    p_chan = &g_socp_stat.dec_dst_chan[chan_id];
    SOCP_REG_READ(SOCP_REG_DECDEST_BUFWPTR(chan_id), phy_addr);
    p_chan->dec_dst_buff.write = phy_addr;
    SOCP_REG_READ(SOCP_REG_DECDEST_BUFRPTR(chan_id), phy_addr);
    p_chan->dec_dst_buff.read = phy_addr;
    socp_get_data_buffer(&p_chan->dec_dst_buff, &rw_buff);

    if (rw_buff.u32Size + rw_buff.u32RbSize < read_size) {
        socp_error("rw_buff is not enough, read_size=0x%x!\n", read_size);

        spin_lock_irqsave(&lock, lock_flag);
        SOCP_REG_SETBITS(SOCP_REG_DEC_RAWINT0, chan_id, 1, 1);
        SOCP_REG_SETBITS(trf_mask_reg, chan_id, 1, 0);
        spin_unlock_irqrestore(&lock, lock_flag);
        g_socp_debug_info.socp_debug_dec_dst.socp_read_done_decdst_fail_cnt[chan_id]++;

        return BSP_ERR_SOCP_INVALID_PARA;
    }

    /* ���ö�дָ�� */
    socp_read_done(&p_chan->dec_dst_buff, read_size);

    /* д��дָ�뵽дָ��Ĵ��� */
    phy_addr = p_chan->dec_dst_buff.read;
    SOCP_REG_WRITE(SOCP_REG_DECDEST_BUFRPTR(chan_id), phy_addr);

    spin_lock_irqsave(&lock, lock_flag);
    SOCP_REG_SETBITS(SOCP_REG_DEC_RAWINT0, chan_id, 1, 1);
    SOCP_REG_SETBITS(trf_mask_reg, chan_id, 1, 0);
    spin_unlock_irqrestore(&lock, lock_flag);

    if (0 == read_size) {
        g_socp_debug_info.socp_debug_dec_dst.socp_read_done_zero_decdst_cnt[chan_id]++;
    } else {
        g_socp_debug_info.socp_debug_dec_dst.socp_read_done_vld_decdst_cnt[chan_id]++;
    }

    g_socp_debug_info.socp_debug_dec_dst.socp_read_done_decdst_suc_cnt[chan_id]++;

    return BSP_OK;
}

u32 bsp_socp_mode(u32 chan_id)
{
    u32 mode_state;

    /*lint -save -e647*/
    if (g_socp_version < SOCP_206_VERSION) {
        mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1);
    } else {
        mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 2, 1);
    }
    /*lint -restore +e647*/

    return mode_state;
}

u32 bsp_socp_mode_ex(u32 chan_id)
{
    u32 mode_state = 0;
    /*lint -save -e647*/
    if (g_socp_version < SOCP_206_VERSION) {
        mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 0, 2);
    } else {
        mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 2);
    }
    /*lint -restore +e647*/
    return mode_state;
}

s32 socp_encode_read_data_done(u32 chan_id, u32 read_size)
{
    s32 ret;
    u32 phy_addr;
    u32 phy_addr2;
    u32 phy_addr3;
    SOCP_BUFFER_RW_STRU rw_buff;
    unsigned long lock_flag;
    u32 curmodestate;
    socp_encdst_chan_s *p_chan = NULL;

    g_socp_debug_info.socp_debug_enc_dst.socp_read_done_encdst_etr_cnt[chan_id]++;

    /* ����ͨ��id */
    if ((ret = socp_check_chan_id(chan_id, SOCP_MAX_ENCDST_CHN)) != BSP_OK) {
        return ret;
    }
    if ((ret = socp_check_encdst_set(chan_id)) != BSP_OK) {
        return ret;
    }
    if (chan_id == 1) {
        g_enc_dst_sta[g_enc_dst_stat_cnt].read_done_start_slice = bsp_get_slice_value();
    }
    /* �ж�deflateʹ�ܣ�deflate readdone */
    if ((SOCP_COMPRESS == g_socp_stat.enc_dst_chan[chan_id].struCompress.bcompress) &&
        (g_socp_stat.enc_dst_chan[chan_id].struCompress.ops.readdone)) {
        return g_socp_stat.enc_dst_chan[chan_id].struCompress.ops.readdone(read_size);
    }

    if (0 == read_size) {
        g_socp_debug_info.socp_debug_enc_dst.socp_read_done_zero_encdst_cnt[chan_id]++;
    } else {
        g_socp_debug_info.socp_debug_enc_dst.socp_read_done_vld_encdst_cnt[chan_id]++;
    }

    spin_lock_irqsave(&lock, lock_flag);

    curmodestate = bsp_socp_mode_ex(chan_id);
    if (0 != curmodestate) /* ctrl & state ��������ģʽ */
    {
        spin_unlock_irqrestore(&lock, lock_flag);
        socp_error("no block mode, curmodestate=0x%x\n", curmodestate);
        return BSP_OK;
    }

    p_chan = &g_socp_stat.enc_dst_chan[chan_id];
    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(chan_id), phy_addr);
    p_chan->enc_dst_buff.write = phy_addr;
    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(chan_id), phy_addr);
    p_chan->enc_dst_buff.read = phy_addr;
    socp_get_data_buffer(&p_chan->enc_dst_buff, &rw_buff);

    if (rw_buff.u32Size + rw_buff.u32RbSize < read_size) {
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT0, chan_id, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK0, chan_id, 1, 0);
        /* overflow int */
        SOCP_REG_SETBITS(SOCP_REG_ENC_RAWINT2, chan_id + 16, 1, 1);
        SOCP_REG_SETBITS(SOCP_REG_ENC_MASK2, chan_id + 16, 1, 0);

        spin_unlock_irqrestore(&lock, lock_flag);
        g_socp_debug_info.socp_debug_enc_dst.socp_read_done_encdst_fail_cnt[chan_id]++;

        socp_error("rw_buff is not enough, read_size=0x%x!\n", read_size);
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    /* ���ö�дָ�� */
    socp_read_done(&p_chan->enc_dst_buff, read_size);

    /* д���ָ�뵽��ָ��Ĵ��� */
    phy_addr2 = p_chan->enc_dst_buff.read;
    /*lint -save -e732*/
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(chan_id), phy_addr2);
    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(chan_id), phy_addr3);
    /*lint -restore +e732*/
    SOCP_DEBUG_TRACE(SOCP_DEBUG_READ_DONE, p_chan->enc_dst_buff.write, phy_addr, phy_addr2, phy_addr3);

    bsp_socp_data_send_continue(chan_id);

    spin_unlock_irqrestore(&lock, lock_flag);
    g_socp_debug_info.socp_debug_enc_dst.socp_read_done_encdst_suc_cnt[chan_id]++;

    if (chan_id == 1) {
        g_enc_dst_sta[g_enc_dst_stat_cnt].read_done_end_slice = bsp_get_slice_value();
        g_enc_dst_stat_cnt = (g_enc_dst_stat_cnt + 1) % SOCP_MAX_ENC_DST_COUNT;
    }

    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_read_data_done
 * ��������: ��������ɺ���
 * �������: dst_chan_id    Ŀ��ͨ��ID
 *           read_size      ��ȡ���ݴ�С
 * �������: ��
 * �� �� ֵ: �����ݳɹ����ı�ʶ��
 */
s32 bsp_socp_read_data_done(u32 dst_chan_id, u32 read_size)
{
    u32 chan_id;
    u32 chan_type;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }

    /* ͳ��socpĿ�Ķ���������������ж��ڽӿ��ڲ�ʵ�� */
    socp_encdst_overflow_info();

    /* ���ʵ��ͨ��id */
    chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);

    if (SOCP_DECODER_DEST_CHAN == chan_type) /* ����ͨ�� */
    {
        ret = socp_decode_read_data_done(chan_id, read_size);
    } else if (SOCP_CODER_DEST_CHAN == chan_type) /* ����ͨ�� */
    {
        ret = socp_encode_read_data_done(chan_id, read_size);
    } else {
        socp_error("invalid chan type: 0x%x!\n", chan_type);
        return BSP_ERR_SOCP_INVALID_CHAN;
    }

    return ret;
}

/*
 * �� �� ��  : bsp_socp_set_bbp_enable
 * ��������  : ʹ��/��ֹBPP LOG������
 * �������  : b_enable       ʹ�ܱ�ʶ
 * �������  : ��
 * �� �� ֵ  : �����ݳɹ����ı�ʶ��
 */
/* cov_verified_start */
s32 bsp_socp_set_bbp_enable(int b_enable)
{
    if (b_enable) {
        BBP_REG_SETBITS(BBP_REG_CH_EN, 0, 1, 1);
    } else {
        BBP_REG_SETBITS(BBP_REG_CH_EN, 0, 1, 0);
    }
    return BSP_OK;
}

/*
 * �� �� ��: bsp_socp_set_bbp_ds_mode
 * ��������: ����BPP����ģʽ
 * �������: eDsMode    ����ģʽ
 * �������: ��
 * �� �� ֵ:
 */
s32 bsp_socp_set_bbp_ds_mode(SOCP_BBP_DS_MODE_ENUM_UIN32 eDsMode)
{
    if (eDsMode >= SOCP_BBP_DS_MODE_BUTT) {
        socp_error("invalid DS mode!\n");
        return BSP_ERR_SOCP_INVALID_PARA;
    }

    BBP_REG_SETBITS(BBP_REG_DS_CFG, 31, 1, eDsMode);
    return BSP_OK;
}
/* cov_verified_stop */

void bsp_socp_set_enc_dst_threshold(bool mode, u32 dst_chan_id)
{
    u32 buf_len;
    u32 thrh;

    dst_chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);

    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFCFG(dst_chan_id), buf_len);
    if (mode == true) /* trueΪ��Ҫ����ʱ�ϱ��ĳ��� */
    {
        thrh = (buf_len >> 2) * 3;
    } else {
        thrh = 0x1000;
    }
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFTHRESHOLD(dst_chan_id), thrh);

    socp_crit("set encdst thrh success! 0x%x\n", thrh);

    return;
}

#ifdef CONFIG_DEFLATE
/*
 * �� �� ��: bsp_socp_compress_enable
 * ��������: deflate ѹ��ʹ��
 * �������: ѹ��Ŀ��ͨ��ID
 * �������: ��
 * �� �� ֵ:ѹ���ɹ�����־
 */
s32 bsp_socp_compress_enable(u32 dst_chan_id)
{
    socp_encdst_chan_s *p_chan = NULL;
    u32 real_chan_id;
    u32 socp_idle_state;
#ifndef FEATURE_SOCP_ADDR_64BITS
    u32 start;
#endif
    u32 chan_type;
    u32 cnt = 500;
    SOCP_CODER_DEST_CHAN_S attr;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }
    real_chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);
    p_chan = &g_socp_stat.enc_dst_chan[real_chan_id];

    if (SOCP_COMPRESS == p_chan->struCompress.bcompress) {
        socp_error("socp is already compress enabled\n");
        return BSP_ERROR;
    }
    if ((NULL == p_chan->struCompress.ops.enable) || NULL == p_chan->struCompress.ops.set ||
        NULL == p_chan->struCompress.ops.register_read_cb || NULL == p_chan->struCompress.ops.readdone ||
        NULL == p_chan->struCompress.ops.getbuffer) {
        socp_error("socp_compress_enable invalid!\n");
        return BSP_ERROR;
    }
    /* ͣSOCP */
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 16, 1, 1);
    /*lint -save -e732*/
    SOCP_REG_READ(SOCP_REG_ENCSTAT, socp_idle_state);

    while (socp_idle_state && cnt) {
        SOCP_REG_READ(SOCP_REG_ENCSTAT, socp_idle_state);
        msleep(1);
        cnt--;
    }
    /*lint -restore +e732*/

    /* ��дָ�����ã���ǰ���ݶ��� */
#ifdef FEATURE_SOCP_ADDR_64BITS
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(real_chan_id), 0);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFWPTR(real_chan_id), 0);
    p_chan->enc_dst_buff.read = 0;
#else
    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFADDR(real_chan_id), start);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFRPTR(real_chan_id), start);
    SOCP_REG_WRITE(SOCP_REG_ENCDEST_BUFWPTR(real_chan_id), start);
    p_chan->enc_dst_buff.read = start;
#endif

    attr.u32EncDstThrh = p_chan->threshold;
    attr.sCoderSetDstBuf.pucOutputStart = (u8 *)(uintptr_t)(p_chan->enc_dst_buff.Start);
    attr.sCoderSetDstBuf.pucOutputEnd = (u8 *)(uintptr_t)(p_chan->enc_dst_buff.End);
    attr.sCoderSetDstBuf.u32Threshold = p_chan->buf_thrhold;

    p_chan->struCompress.ops.set(dst_chan_id, &attr);
    p_chan->struCompress.ops.register_read_cb(p_chan->read_cb);
    p_chan->struCompress.ops.register_event_cb(p_chan->event_cb);
    /* ѹ��ʹ�� */
    p_chan->struCompress.ops.enable(dst_chan_id);

    /* ���ѹ��ͨ��ԭʼ�жϣ������ж�״̬ */
    bsp_socp_data_send_manager(dst_chan_id, 0);
    /* ʹ��SOCP */
    /*lint -save -e845*/
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 16, 1, 0);
    /*lint -restore +e845*/
    p_chan->struCompress.bcompress = SOCP_COMPRESS;
    g_deflate_status = SOCP_COMPRESS;
    return BSP_OK;
}
/*
 * �� �� ��: bsp_socp_compress_disable
 * ��������: deflate ѹ��ֹͣ
 * �������: ѹ��Ŀ��ͨ��ID
 * �������: ��
 * �� �� ֵ:ѹ��ֹͣ�ɹ�����־
 */
s32 bsp_socp_compress_disable(u32 dst_chan_id)
{
    socp_encdst_chan_s *p_chan = NULL;
    u32 real_chan_id;
    u32 socp_idle_state;
    u32 chan_type;
    u32 cnt = 500;
    s32 ret;

    /* �ж��Ƿ��Ѿ���ʼ�� */
    if ((ret = socp_check_init()) != BSP_OK) {
        return ret;
    }
    /* �������Ƿ���Ч */
    real_chan_id = SOCP_REAL_CHAN_ID(dst_chan_id);
    chan_type = SOCP_REAL_CHAN_TYPE(dst_chan_id);
    p_chan = &g_socp_stat.enc_dst_chan[real_chan_id];
    if (SOCP_NO_COMPRESS == p_chan->struCompress.bcompress) {
        socp_error("socp is already compress_disabled\n");
        return BSP_ERROR;
    }
    if (NULL == p_chan->struCompress.ops.disable || NULL == p_chan->struCompress.ops.clear) {
        socp_error("socp_compress_disable invalid!\n");
        return BSP_ERROR;
    }

    /*lint -save -e732*/
    /* ͣSOCP */
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 16, 1, 1);
    SOCP_REG_READ(SOCP_REG_ENCSTAT, socp_idle_state);

    while (socp_idle_state && cnt) {
        SOCP_REG_READ(SOCP_REG_ENCSTAT, socp_idle_state);
        msleep(1);
        cnt--;
    }

    p_chan->struCompress.ops.disable(dst_chan_id);
    p_chan->struCompress.ops.clear(dst_chan_id);

    bsp_socp_data_send_manager(dst_chan_id, 1);

    /* ʹ��SOCP */
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 16, 1, 0);
    /*lint -restore +e845*/
    p_chan->struCompress.bcompress = SOCP_NO_COMPRESS;
    g_deflate_status = SOCP_NO_COMPRESS;
    return BSP_OK;
}
/*
 * �� �� ��: bsp_socp_register_compress
 * ��������: ѹ������ע��
 * �������: ע�ắ���ṹ��
 * �������: ��
 * �� �� ֵ:ע��ɹ�����־
 */
s32 bsp_socp_register_compress(socp_compress_ops_stru *ops)
{
    int i;
    g_socp_stat.compress_isr = ops->isr;

    for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
        g_socp_stat.enc_dst_chan[i].struCompress.ops.register_event_cb = ops->register_event_cb;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.register_read_cb = ops->register_read_cb;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.enable = ops->enable;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.disable = ops->disable;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.set = ops->set;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.getbuffer = ops->getbuffer;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.readdone = ops->readdone;
        g_socp_stat.enc_dst_chan[i].struCompress.ops.clear = ops->clear;
    }
    return BSP_OK;
}

#endif
/*
 * �� �� ��: bsp_socp_mode_change_chip_bugfix
 * ��������: ����޸�оƬ��ģʽ�л���ʱ����Ҫ����һ�������ݵ�bug
 * �������: ͨ����
 * �������: ��
 * �� �� ֵ: ��
 */
void bsp_socp_mode_change_chip_bugfix(u32 chanid)
{
    s32 ret;
    u32 len;
    u32 bufdepth;
    u32 write_value;
    u32 read_value;
    u32 chan_id = SOCP_REAL_CHAN_ID(chanid);

    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(chan_id), write_value);
    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(chan_id), read_value);
    SOCP_REG_READ(SOCP_REG_ENCDEST_BUFCFG(chan_id), bufdepth);

    if (write_value >= read_value) {
        len = write_value - read_value;
    } else {
        len = write_value + bufdepth - read_value;
    }

    if (bufdepth - len < 64 * 1024) {
        ret = bsp_socp_read_data_done(chanid, len < 64 * 1024 ? len : 64 * 1024);
        if (ret != BSP_OK) {
            socp_error("socp_update read ptr fail(0x%x)\n", ret);
        }
    }
}

/*
 * �� �� ��: bsp_socp_encdst_set_cycle
 * ��������: SOCPѭ��ģʽ����
 * �������: ͨ���š�ģʽ
 * �������: ��
 * �� �� ֵ: ��
 */
/*lint -save -e647*/
s32 bsp_socp_encdst_set_cycle(u32 chanid, u32 cycle)
{
    u32 mode_state;
    unsigned long lock_flag;
    u32 chan_id = SOCP_REAL_CHAN_ID(chanid);
    u32 waittime = 10000;

    /* �ر��Զ�ʱ���ſأ������ϱ�ģʽ���ò���Ч */
    SOCP_REG_SETBITS(SOCP_REG_CLKCTRL, 0, 1, 0);

    if (g_socp_version < SOCP_206_VERSION) {
        mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1);
    } else {
        mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 2, 1);
    }

    if ((0 == cycle || 1 == cycle) && mode_state) {
        u32 i;

        spin_lock_irqsave(&lock, lock_flag);
        if (g_socp_version < SOCP_206_VERSION) {
            SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 0, 1, 0);
        } else {
            SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1, 0);
        }
        spin_unlock_irqrestore(&lock, lock_flag);

        for (i = 0; i < waittime; i++) {
            if (g_socp_version < SOCP_206_VERSION) {
                mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1);
            } else {
                mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 2, 1);
            }
            if (0 == mode_state) {
                break;
            }
        }

        if (waittime == i) {
            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_mode_change_fail_cnt[chan_id]++;
            socp_error("set encdst cycle off timeout!\n");
            /* �ر��Զ�ʱ���ſأ������ϱ�ģʽ���ò���Ч */
            SOCP_REG_SETBITS(SOCP_REG_CLKCTRL, 0, 1, 1);
            return BSP_ERROR;
        }

        /* Drop data of socp dst buffer */
        bsp_socp_mode_change_chip_bugfix(chanid);

        bsp_socp_data_send_manager(chanid, SOCP_DEST_DSM_ENABLE);

    } else if ((2 == cycle) && (!mode_state)) {
        u32 i;

        bsp_socp_data_send_manager(chanid, SOCP_DEST_DSM_DISABLE);

        spin_lock_irqsave(&lock, lock_flag);
        if (g_socp_version < SOCP_206_VERSION) {
            SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 0, 1, 1);
        } else {
            SOCP_REG_SETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1, 1);
        }
        spin_unlock_irqrestore(&lock, lock_flag);

        for (i = 0; i < waittime; i++) {
            if (g_socp_version < SOCP_206_VERSION) {
                mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 1, 1);
            } else {
                mode_state = SOCP_REG_GETBITS(SOCP_REG_ENCDEST_SBCFG(chan_id), 2, 1);
            }
            if (1 == mode_state) {
                break;
            }
        }

        if (waittime == i) {
            g_socp_debug_info.socp_debug_enc_dst.socp_encdst_mode_change_fail_cnt[chan_id]++;
            socp_error("set encdst cycle on timeout!\n");
            SOCP_REG_SETBITS(SOCP_REG_CLKCTRL, 0, 1, 1);
            return BSP_ERROR;
        }
    }

    /* �ر��Զ�ʱ���ſأ������ϱ�ģʽ���ò���Ч */
    SOCP_REG_SETBITS(SOCP_REG_CLKCTRL, 0, 1, 1);
    return BSP_OK;
}
/*lint -restore +e647*/

/*
 * �� �� ��: socp_enc_dst_stat
 * ��������: ��ȡsocp��ӡ��Ϣ
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 */
void show_socp_enc_dst_stat(void)
{
    int i;
    int count = (g_enc_dst_stat_cnt + 1) % SOCP_MAX_ENC_DST_COUNT;

    for (i = 0; i < SOCP_MAX_ENC_DST_COUNT; i++) {
        socp_crit("\r stat %d count:\r\n", i);
        socp_crit("Int   Start  : 0x%x,  End  : 0x%x ,Slice :0x%x\n", g_enc_dst_sta[count].int_start_slice,
                  g_enc_dst_sta[count].int_end_slice,
                  g_enc_dst_sta[count].int_end_slice - g_enc_dst_sta[count].int_start_slice);
        socp_crit("Task  Start  : 0x%x,  End  : 0x%x ,Slice :0x%x\n", g_enc_dst_sta[count].task_start_slice,
                  g_enc_dst_sta[count].task_end_slice,
                  g_enc_dst_sta[count].task_end_slice - g_enc_dst_sta[count].task_start_slice);
        socp_crit("Read  Start  : 0x%x,  End  : 0x%x ,Slice :0x%x\n", g_enc_dst_sta[count].read_done_start_slice,
                  g_enc_dst_sta[count].read_done_end_slice,
                  g_enc_dst_sta[count].read_done_end_slice - g_enc_dst_sta[count].read_done_start_slice);
        socp_crit("Int  ==> Task 0x%x\n",
                  g_enc_dst_sta[count].task_start_slice - g_enc_dst_sta[count].int_start_slice);
        socp_crit("Task ==> Read 0x%x\n",
                  g_enc_dst_sta[count].read_done_start_slice - g_enc_dst_sta[count].task_end_slice);
        count = (count + 1) % SOCP_MAX_ENC_DST_COUNT;
    }
}

/*
 * �� �� ��: socp_encdst_overflow_info
 * ��������: ͳ��socpĿ�Ķ���ֵ����
 * �������: ��
 * �������:
 * �� �� ֵ: ��
 */
void socp_encdst_overflow_info(void)
{
    u32 int_state = 0;

    SOCP_REG_READ(SOCP_REG_ENC_RAWINT2, int_state);
    if ((int_state & 0xf0000) == 0x20000) {
        g_socp_enc_dst_threshold_ovf++;
    }
}

/*
 * �� �� ��: bsp_SocpEncDstQueryIntInfo
 * ��������: �ṩ��diag_debug��ѯsocp����ͨ��Ŀ�Ķ��ж���Ϣ
 * �������: ��
 * �������:
 * �� �� ֵ: ��
 */
void bsp_SocpEncDstQueryIntInfo(u32 *trf_info, u32 *thrh_ovf_info)
{
    *trf_info = g_socp_enc_dst_isr_trf_int_cnt;
    *thrh_ovf_info = g_socp_enc_dst_threshold_ovf;
}

/*
 * �� �� ��: bsp_clear_socp_encdst_int_info
 * ��������: ���socpĿ�Ķ�����ͳ��ֵ
 * �������: ��
 * �������:
 * �� �� ֵ: ��
 */
void bsp_clear_socp_encdst_int_info(void)
{
    g_socp_enc_dst_isr_trf_int_cnt = 0;
    g_socp_enc_dst_threshold_ovf = 0;
}
#define MALLOC_MAX_SIZE 0x100000
#define MALLOC_MAX_INDEX 8 /* page_size Ϊ4K */
#define SOCP_PAGE_SIZE 0x1000

// __inline
s32 socp_get_index(u32 size, u32 *index)
{
    u32 i = 0;
    if (size > MALLOC_MAX_SIZE) {
        return BSP_ERROR;
    }
    for (i = 0; i <= MALLOC_MAX_INDEX; i++) {
        if (size <= (u32)(SOCP_PAGE_SIZE * (1 << i))) {
            *index = i;
            break;
        }
    }
    return BSP_OK;
}

void *socp_malloc(u32 size)
{
    u8 *item = NULL;
    u32 index = 0;

    if (BSP_OK != socp_get_index(size, &index)) {
        return BSP_NULL;
    }

    index = 4;
    /* �����ڴ� */
    item = (u8 *)(uintptr_t)__get_free_pages(GFP_KERNEL, index);
    if (NULL == item) {
        socp_error("malloc failed\n");
        return BSP_NULL;
    }

    return (void *)item;
}

s32 socp_free(void *mem)
{
    u8 *item;

    item = mem;

    free_pages((uintptr_t)item, 4);
    return BSP_OK;
}

/* �͹������ begin */
/* �͹��Ĳ����ݲ��޸� */
/* cov_verified_start */
void bsp_socp_drx_restore_reg_app_only(void)
{
    u32 i = 0;

    for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
        SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(i), g_socp_stat.enc_dst_chan[i].enc_dst_buff.read);
        SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(i), g_socp_stat.enc_dst_chan[i].enc_dst_buff.write);
    }

    for (i = 0; i < SOCP_MAX_DECDST_CHN; i++) {
        SOCP_REG_READ(SOCP_REG_DECDEST_BUFRPTR(i), g_socp_stat.dec_dst_chan[i].dec_dst_buff.read);
        SOCP_REG_READ(SOCP_REG_DECDEST_BUFWPTR(i), g_socp_stat.dec_dst_chan[i].dec_dst_buff.write);
    }

    for (i = 0; i < SOCP_MAX_ENCSRC_CHN; i++) {
        SOCP_REG_READ(SOCP_REG_ENCSRC_BUFRPTR(i), g_socp_stat.enc_src_chan[i].enc_src_buff.read);
        SOCP_REG_READ(SOCP_REG_ENCSRC_BUFWPTR(i), g_socp_stat.enc_src_chan[i].enc_src_buff.write);

        SOCP_REG_READ(SOCP_REG_ENCSRC_RDQRPTR(i), g_socp_stat.enc_src_chan[i].rd_buff.read);
        SOCP_REG_READ(SOCP_REG_ENCSRC_RDQWPTR(i), g_socp_stat.enc_src_chan[i].rd_buff.write);
    }

    for (i = 0; i < SOCP_MAX_DECSRC_CHN; i++) {
        SOCP_REG_READ(SOCP_REG_DECSRC_BUFRPTR(i), g_socp_stat.dec_src_chan[i].dec_src_buff.read);
        SOCP_REG_READ(SOCP_REG_DECSRC_BUFWPTR(i), g_socp_stat.dec_src_chan[i].dec_src_buff.write);
    }
}
/* cov_verified_stop */

/*
 * �� �� ��: bsp_socp_get_state
 * ��������: ��ȡSOCP״̬
 * �� �� ֵ: SOCP_IDLE    ����
 *           SOCP_BUSY    æµ
 */
SOCP_STATE_ENUM_UINT32 bsp_socp_get_state(void)
{
    u32 enc_chan_state;
    u32 dec_chan_state;

    SOCP_REG_READ(SOCP_REG_ENCSTAT, enc_chan_state);
    SOCP_REG_READ(SOCP_REG_DECSTAT, dec_chan_state);
    if (enc_chan_state != 0 || dec_chan_state != 0) {
        return SOCP_BUSY;
    }

    return SOCP_IDLE;
}

/*
 * �� �� ��: socp_is_encdst_chan_empty
 * ��������: SOCP����Ŀ��ͨ���Ƿ�������
 * �������: ��
 * �������: ��
 * �� �� ֵ: u32 0:������ ��0:��Ӧͨ����λ
 */
u32 socp_is_encdst_chan_empty(void)
{
    u32 chanSet = 0;
    u32 i;
    u32 u32ReadPtr;
    u32 u32WritePtr;

    /* �ж�Ŀ��ͨ����дָ���Ƿ���� */
    for (i = 0; i < SOCP_MAX_ENCDST_CHN; i++) {
        SOCP_REG_READ(SOCP_REG_ENCDEST_BUFWPTR(i), u32WritePtr);
        SOCP_REG_READ(SOCP_REG_ENCDEST_BUFRPTR(i), u32ReadPtr);
        if (u32WritePtr != u32ReadPtr) {
            chanSet = chanSet | (1 << i);
        }
    }

    return chanSet;
}
#ifndef CONFIG_HISI_BALONG_MODEM_MODULE
module_init(socp_init);
#endif


/*
 * �� �� ��: socp_set_clk_autodiv_enable
 * ��������: ����clk�ӿ�clk_disable_unprepare��bypass��0�������Զ���Ƶ
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 * ע    ��:
 *             clk_prepare_enable �ӿ��� clk_disable_unprepare �ӿڱ���ɶ�ʹ��
 *             clk���Զ���ƵĬ�ϴ��ڴ�״̬������
 *             �����Ƚ��� clk_prepare_enable ���ܽ��� clk_disable_unprepare ����
 */
void bsp_socp_set_clk_autodiv_enable(void)
{
}

/*
 * �� �� ��: socp_set_clk_autodiv_disable
 * ��������: ����clk�ӿ�clk_prepare_enable��bypass��1�������Զ���Ƶ
 * �������: ��
 * �������: ��
 * �� �� ֵ: ��
 * ע    ��:
 *             clk_prepare_enable �ӿ��� clk_disable_unprepare �ӿڱ���ɶ�ʹ��
 *             clk���Զ���ƵĬ�ϴ��ڴ�״̬������
 *             �����Ƚ��� clk_prepare_enable ���ܽ��� clk_disable_unprepare ����
 */
void bsp_socp_set_clk_autodiv_disable(void)
{
}

/*
 * �� �� ��  : bsp_socp_set_decode_timeout_register
 * ��������  :������жϳ�ʱ���üĴ���ѡ��
 * �� �� ֵ  :  ��
 */
#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)
s32 bsp_socp_set_decode_timeout_register(decode_timeout_module_e module)
{
    if (module > DECODE_TIMEOUT_DEC_INT_TIMEOUT) {
        return BSP_ERR_SOCP_INVALID_PARA;
    }
    SOCP_REG_SETBITS(SOCP_REG_GBLRST, 1, 1, module);

    return BSP_OK;
}
EXPORT_SYMBOL(bsp_socp_set_decode_timeout_register);
#endif

u32 bsp_get_socp_ind_dst_int_slice(void)
{
    return g_enc_dst_sta[g_enc_dst_stat_cnt].int_start_slice;
}

s32 bsp_clear_socp_buff(u32 src_chan_id)
{
    return BSP_OK;
}

EXPORT_SYMBOL(socp_reset_dec_chan);
EXPORT_SYMBOL(socp_soft_free_encdst_chan);
EXPORT_SYMBOL(socp_soft_free_decsrc_chan);
EXPORT_SYMBOL(bsp_socp_clean_encsrc_chan);
EXPORT_SYMBOL(socp_init);
EXPORT_SYMBOL(bsp_socp_encdst_dsm_init);
EXPORT_SYMBOL(bsp_socp_data_send_manager);
EXPORT_SYMBOL(bsp_socp_coder_set_src_chan);
EXPORT_SYMBOL(bsp_socp_decoder_set_dest_chan);
EXPORT_SYMBOL(bsp_socp_coder_set_dest_chan_attr);
EXPORT_SYMBOL(bsp_socp_decoder_get_err_cnt);
EXPORT_SYMBOL(bsp_socp_decoder_set_src_chan_attr);
EXPORT_SYMBOL(bsp_socp_set_timeout);
EXPORT_SYMBOL(bsp_socp_set_dec_pkt_lgth);
EXPORT_SYMBOL(bsp_socp_set_debug);
EXPORT_SYMBOL(bsp_socp_free_channel);
EXPORT_SYMBOL(bsp_socp_chan_soft_reset);
EXPORT_SYMBOL(bsp_socp_start);
EXPORT_SYMBOL(bsp_socp_stop);
EXPORT_SYMBOL(bsp_socp_register_event_cb);
EXPORT_SYMBOL(bsp_socp_get_write_buff);
EXPORT_SYMBOL(bsp_socp_write_done);
EXPORT_SYMBOL(bsp_socp_register_rd_cb);
EXPORT_SYMBOL(bsp_socp_get_rd_buffer);
EXPORT_SYMBOL(bsp_socp_read_rd_done);
EXPORT_SYMBOL(bsp_socp_register_read_cb);
EXPORT_SYMBOL(bsp_socp_get_read_buff);
EXPORT_SYMBOL(bsp_socp_read_data_done);
EXPORT_SYMBOL(bsp_socp_set_bbp_enable);
EXPORT_SYMBOL(bsp_socp_set_bbp_ds_mode);
EXPORT_SYMBOL(socp_get_index);
EXPORT_SYMBOL(socp_malloc);
EXPORT_SYMBOL(socp_free);
EXPORT_SYMBOL(bsp_socp_drx_restore_reg_app_only);
EXPORT_SYMBOL(bsp_socp_get_state);
EXPORT_SYMBOL(socp_is_encdst_chan_empty);
EXPORT_SYMBOL(socp_dst_channel_enable);
EXPORT_SYMBOL(socp_dst_channel_disable);

