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

#ifndef _BSP_SOCP_H
#define _BSP_SOCP_H

#include "osl_common.h"
#include "mdrv_socp_common.h"
#ifdef __KERNEL__
#include "acore_nv_id_drv.h"
#include "acore_nv_stru_drv.h"
#endif
#include "bsp_trace.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************
  �����붨��
**************************************************************************/
#define BSP_ERR_SOCP_BASE            BSP_DEF_ERR(BSP_MODU_SOCP, 0)
#define BSP_ERR_SOCP_NULL            (BSP_ERR_SOCP_BASE + 0x1)
#define BSP_ERR_SOCP_NOT_INIT        (BSP_ERR_SOCP_BASE + 0x2)
#define BSP_ERR_SOCP_MEM_ALLOC       (BSP_ERR_SOCP_BASE + 0x3)
#define BSP_ERR_SOCP_SEM_CREATE      (BSP_ERR_SOCP_BASE + 0x4)
#define BSP_ERR_SOCP_TSK_CREATE      (BSP_ERR_SOCP_BASE + 0x5)
#define BSP_ERR_SOCP_INVALID_CHAN    (BSP_ERR_SOCP_BASE + 0x6)
#define BSP_ERR_SOCP_INVALID_PARA    (BSP_ERR_SOCP_BASE + 0x7)
#define BSP_ERR_SOCP_NO_CHAN         (BSP_ERR_SOCP_BASE + 0x8)
#define BSP_ERR_SOCP_SET_FAIL        (BSP_ERR_SOCP_BASE + 0x9)
#define BSP_ERR_SOCP_TIMEOUT         (BSP_ERR_SOCP_BASE + 0xa)
#define BSP_ERR_SOCP_NOT_8BYTESALIGN (BSP_ERR_SOCP_BASE + 0xb)
#define BSP_ERR_SOCP_CHAN_RUNNING    (BSP_ERR_SOCP_BASE + 0xc)
#define BSP_ERR_SOCP_CHAN_MODE       (BSP_ERR_SOCP_BASE + 0xd)
#define BSP_ERR_SOCP_DEST_CHAN       (BSP_ERR_SOCP_BASE + 0xe)
#define BSP_ERR_SOCP_DECSRC_SET      (BSP_ERR_SOCP_BASE + 0xf)

#define SOCP_MAX_MEM_SIZE            (200 *1024 *1024)
#define SOCP_MIN_MEM_SIZE            (1 *1024 *1024)
#define SOCP_MAX_TIMEOUT             1200     /*MS*/
#define SOCP_MIN_TIMEOUT             10       /*MS*/
#define SOCP_RESERVED_TRUE           1
#define SOCP_RESERVED_FALSE          0

typedef u32 (*socp_compress_isr)     (void);
typedef u32 (*socp_compress_event_cb)(socp_event_cb event_cb);
typedef u32 (*socp_compress_read_cb) (socp_read_cb read_cb);
typedef u32 (*socp_compress_enable)  (u32 chanid);
typedef u32 (*socp_compress_disable) (u32 chanid);
typedef u32 (*socp_compress_set)     (u32 chanid, SOCP_CODER_DEST_CHAN_S* attr);
typedef u32 (*socp_compress_getbuffer) (SOCP_BUFFER_RW_STRU *pRingBuffer);
typedef u32 (*socp_compress_readdone)(u32 u32Size);
typedef u32 (*socp_compress_clear)   (u32 chanid);

typedef struct socp_compress_ops
{
    socp_compress_isr       isr;
    socp_compress_event_cb  register_event_cb;
    socp_compress_read_cb   register_read_cb;
    socp_compress_enable    enable;
    socp_compress_disable   disable;
    socp_compress_set       set;
    socp_compress_getbuffer getbuffer;
    socp_compress_readdone  readdone;
    socp_compress_clear     clear;
}socp_compress_ops_stru;

#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)
typedef enum timeout_module
{
    DECODE_TIMEOUT_INT_TIMEOUT = 0,
    DECODE_TIMEOUT_DEC_INT_TIMEOUT = 1,
    DECODE_TIMEOUT_BUTTON = 2,

} decode_timeout_module_e;

#endif

typedef struct
{
    SOCP_VOTE_TYPE_ENUM_U32 type;
}socp_vote_req_stru;

typedef struct
{
    u32 vote_rst;   /* 1:�ɹ�����1:ʧ�� */
}socp_vote_cnf_stru;

struct socp_enc_dst_log_cfg
{
    void*           virt_addr;       /* SOCP����Ŀ��ͨ����������BUFFER����32λϵͳ����4�ֽڣ���64λϵͳ����8�ֽ� */
    unsigned long   phy_addr;        /* SOCP����Ŀ��ͨ����������BUFFER��ַ */
    unsigned int    buff_size;       /* SOCP����Ŀ��ͨ������BUFFER��С */
    unsigned int    over_time;       /* NVE�����õĳ�ʱʱ�� */
    unsigned int    log_on_flag;     /* ��������buffer����������־(SOCP_DST_CHAN_CFG_TYPE_ENUM) */
    unsigned int    cur_time_out;    /* SOCP����Ŀ��ͨ�����ݴ��䳬ʱʱ�� */
    unsigned int    flush_flag;
    unsigned int    mem_log_cfg;
    unsigned int    current_mode;
    unsigned int    cps_mode;
};

typedef struct {
    void*           virt_addr;      /* SOCP����Ŀ��ͨ����������BUFFER����32λϵͳ����4�ֽڣ���64λϵͳ����8�ֽ� */
    unsigned long   phy_addr;       /* SOCP����Ŀ��ͨ����������BUFFER��ַ */
    unsigned int    buff_size;      /* SOCP����Ŀ��ͨ������BUFFER��С */
    unsigned int    buff_useable;    /* Ԥ����kernel buffer�Ƿ���õı�־ */
    unsigned int    timeout;        /* SOCP����Ŀ��ͨ�����ݴ��䳬ʱʱ�� */
    unsigned int    init_flag;      /* Ԥ���ڴ��ȡ������ɱ�־ */
}socp_rsv_mem_s;


#ifdef ENABLE_BUILD_SOCP
/*****************************************************************************
* �� �� ��  : socp_init
*
* ��������  : ģ���ʼ������
*
* �������  : ��
*
* �������  : ��
*
* �� �� ֵ  : ��ʼ���ɹ��ı�ʶ��
*****************************************************************************/
s32 socp_init(void);

/*****************************************************************************
* �� �� ��  : bsp_socp_encdst_dsm_init
*
* ��������  : socp����Ŀ�Ķ��ж�״̬��ʼ��
* �������  : enc_dst_chan_id: ����Ŀ�Ķ�ͨ����
*             b_enable: ��ʼ�����ж�״̬
*
* �������  : ��
*
* �� �� ֵ  : ��
*****************************************************************************/
void bsp_socp_encdst_dsm_init(u32 enc_dst_chan_id, u32 b_enable);

/*****************************************************************************
* �� �� ��  : bsp_socp_data_send_manager
*
* ��������  : socp����Ŀ�Ķ��ϱ�����
* �������  : enc_dst_chan_id: ����Ŀ�Ķ�ͨ����
*             b_enable: �ж�ʹ��
*
* �������  : ��
*
* �� �� ֵ  : ��
*****************************************************************************/
void bsp_socp_data_send_manager(u32 enc_dst_chan_id, u32 b_enable);

/*****************************************************************************
 �� �� ��  : bsp_socp_coder_set_src_chan
 ��������  : �˽ӿ����SOCP������Դͨ���ķ��䣬���ݱ�����Դͨ����������ͨ�����ԣ�����Ŀ��ͨ�������غ���ִ�н����
 �������  : src_attr:������Դͨ�������ṹ��ָ�롣
             pSrcChanID:���뵽��Դͨ��ID��
 �������  : �ޡ�
 �� �� ֵ  : SOCP_OK:����Դͨ������ɹ���
             SOCP_ERROR:����Դͨ������ʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_coder_set_src_chan(SOCP_CODER_SRC_ENUM_U32 src_chan_id, SOCP_CODER_SRC_CHAN_S *src_attr);

/*****************************************************************************
 �� �� ��  : bsp_socp_coder_set_dest_chan_attr
 ��������  : �˽ӿ����ĳһ����Ŀ��ͨ�������ã����غ���ִ�еĽ����
 �������  : dst_chan_id:SOCP��������Ŀ��ͨ��ID��
             dst_attr:SOCP������Ŀ��ͨ�������ṹ��ָ�롣
 �������  : �ޡ�
 �� �� ֵ  : SOCP_OK:����Ŀ��ͨ�����óɹ���
             SOCP_ERROR:����Ŀ��ͨ������ʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_coder_set_dest_chan_attr(u32 dst_chan_id, SOCP_CODER_DEST_CHAN_S *dst_attr);

/*****************************************************************************
 �� �� ��      : bsp_socp_decoder_set_dest_chan
 ��������  :�˽ӿ����SOCP������Ŀ��ͨ���ķ��䣬
                ���ݽ���Ŀ��ͨ����������ͨ�����ԣ�
                ������Դͨ�������غ���ִ�н����
 �������  : attr:������Ŀ��ͨ�������ṹ��ָ��
                         pDestChanID:���뵽��Ŀ��ͨ��ID
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:����Ŀ��ͨ������ɹ���
                             SOCP_ERROR:����Ŀ��ͨ������ʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_decoder_set_dest_chan(SOCP_DECODER_DST_ENUM_U32 dst_chan_id, SOCP_DECODER_DEST_CHAN_STRU *attr);

/*****************************************************************************
 �� �� ��      : bsp_socp_decoder_set_src_chan_attr
 ��������  :�˽ӿ����ĳһ����Դͨ�������ã����غ���ִ�еĽ����
 �������  : src_chan_id:������Դͨ��ID
                         input_attr:������Դͨ�������ṹ��ָ��
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:����Դͨ�����óɹ�
                             SOCP_ERROR:����Դͨ������ʧ��
*****************************************************************************/
s32 bsp_socp_decoder_set_src_chan_attr ( u32 src_chan_id,SOCP_DECODER_SRC_CHAN_STRU *input_attr);

/*****************************************************************************
 �� �� ��      : bsp_socp_decoder_get_err_cnt
 ��������  :�˽ӿڸ�������ͨ���������쳣����ļ���ֵ��
 �������  : chan_id:������ͨ��ID
                         err_cnt:�������쳣�����ṹ��ָ��
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:�����쳣�����ɹ�
                             SOCP_ERROR:�����쳣����ʧ��
*****************************************************************************/
s32 bsp_socp_decoder_get_err_cnt (u32 chan_id, SOCP_DECODER_ERROR_CNT_STRU *err_cnt);

/*****************************************************************************
 �� �� ��  : bsp_socp_free_channel
 ��������  : �˽ӿڸ���ͨ��ID�ͷŷ���ı����ͨ����
 �������  : chan_id:ͨ��ID��
 �������  : �ޡ�
 �� �� ֵ  : SOCP_OK:ͨ���ͷųɹ���
             SOCP_ERROR:ͨ���ͷ�ʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_free_channel(u32 chan_id);

/*****************************************************************************
* �� �� ��  : socp_clean_encsrc_chan
*
* ��������  : ��ձ���Դͨ����ͬ��V9 SOCP�ӿ�
*
* �������  : src_chan_id       ����ͨ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_OK
*****************************************************************************/
u32 bsp_socp_clean_encsrc_chan(SOCP_CODER_SRC_ENUM_U32 src_chan_id);

/*****************************************************************************
 �� �� ��  : bsp_socp_register_event_cb
 ��������  : �˽ӿ�Ϊ����ͨ��ע���¼��ص�������
 �������  : chan_id:ͨ��ID��
             event_cb:�¼��ص��������ο�socp_event_cb��������
 �������  : �ޡ�
 �� �� ֵ  : SOCP_OK:ע���¼��ص������ɹ���
             SOCP_ERROR:ע���¼��ص�����ʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_register_event_cb(u32 chan_id, socp_event_cb event_cb);

/*****************************************************************************
 �� �� ��  : bsp_socp_start
 ��������  : �˽ӿ�����Դͨ��������������߽��롣
 �������  : src_chan_id:Դͨ��ID��
 �������  : �ޡ�
 �� �� ֵ  : SOCP_OK:�������������ɹ���
             SOCP_ERROR:������������ʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_start(u32 src_chan_id);

/*****************************************************************************
 �� �� ��  : bsp_socp_stop
 ��������  : �˽ӿ�����Դͨ����ֹͣ������߽��롣
 �������  : src_chan_id:Դͨ��ID��
 �������  : �ޡ�
 �� �� ֵ  : SOCP_OK:��������ֹͣ�ɹ���
             SOCP_ERROR:��������ֹͣʧ�ܡ�
*****************************************************************************/
s32 bsp_socp_stop(u32 src_chan_id);

/*****************************************************************************
 �� �� ��      : bsp_socp_set_timeout
 ��������  :�˽ӿ����ó�ʱ��ֵ��
 �������  : time_out:��ʱ��ֵ

 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:���ó�ʱʱ����ֵ�ɹ���
                             SOCP_ERROR:���ó�ʱʱ����ֵʧ��
*****************************************************************************/
s32 bsp_socp_set_timeout (SOCP_TIMEOUT_EN_ENUM_UIN32 time_out_en, u32 time_out);

/*****************************************************************************
 �� �� ��   : bsp_socp_set_dec_pkt_lgth
 ��������  :���ý�������ȼ���ֵ
 �������  : pkt_length:��������ȼ�ֵ

 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:���óɹ���
                    ����ֵ:����ʧ��
*****************************************************************************/
s32 bsp_socp_set_dec_pkt_lgth(SOCP_DEC_PKTLGTH_STRU *pkt_length);

/*****************************************************************************
 �� �� ��   : bsp_socp_set_debug
 ��������  :���ý���Դͨ����debugģʽ
 �������  : chan_id:ͨ��ID
                  debug_en: debug��ʶ
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:���óɹ���
                     ����ֵ:����ʧ��
*****************************************************************************/
s32 bsp_socp_set_debug(u32 dst_chan_id, u32 debug_en);

/*****************************************************************************
 �� �� ��      : bsp_socp_get_write_buff
 ��������  :�˽ӿ����ڻ�ȡд����buffer��
 �������  : src_chan_id:Դͨ��ID
                  p_rw_buff:           :д����buffer

 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:��ȡд����buffer�ɹ���
                             SOCP_ERROR:��ȡд����bufferʧ��
*****************************************************************************/
s32 bsp_socp_get_write_buff( u32 src_chan_id, SOCP_BUFFER_RW_STRU *p_rw_buff);

/*****************************************************************************
 �� �� ��      : bsp_socp_write_done
 ��������  :�ýӿ��������ݵ�д�������ṩд�����ݵĳ��ȡ�
 �������  : src_chan_id:Դͨ��ID
                  write_size:   ��д�����ݵĳ���
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:д�����ݳɹ���
                             SOCP_ERROR:д������ʧ��
*****************************************************************************/
s32 bsp_socp_write_done(u32 src_chan_id, u32 write_size);


/*****************************************************************************
 �� �� ��      : bsp_socp_register_rd_cb
 ��������  :�ýӿ�����ע���RD�������ж�ȡ���ݵĻص�������
 �������  : src_chan_id:Դͨ��ID
                  rd_cb:  �¼��ص�����
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:ע��RD���λ����������ݻص������ɹ���
                             SOCP_ERROR:ע��RD���λ����������ݻص�����ʧ��
*****************************************************************************/
s32 bsp_socp_register_rd_cb(u32 src_chan_id, socp_rd_cb rd_cb);

/*****************************************************************************
 �� �� ��      : bsp_socp_get_rd_buffer
 ��������  :�ô˽ӿ����ڻ�ȡRD buffer������ָ�롣
 �������  : src_chan_id:Դͨ��ID
                  p_rw_buff:  RD buffer
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:��ȡRD���λ������ɹ�
                             SOCP_ERROR:��ȡRD���λ�����ʧ��
*****************************************************************************/
s32 bsp_socp_get_rd_buffer( u32 src_chan_id,SOCP_BUFFER_RW_STRU *p_rw_buff);

/*****************************************************************************
 �� �� ��      : bsp_socp_read_rd_done
 ��������  :�˽ӿ������ϲ�֪ͨSOCP��������RD buffer��ʵ�ʶ�ȡ�����ݡ�
 �������  : src_chan_id:Դͨ��ID
                  rd_size:  ��RD buffer��ʵ�ʶ�ȡ�����ݳ���
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:��ȡRDbuffer�е����ݳɹ�
                             SOCP_ERROR:��ȡRDbuffer�е�����ʧ��
*****************************************************************************/
s32 bsp_socp_read_rd_done(u32 src_chan_id, u32 rd_size);

/*****************************************************************************
 �� �� ��      : bsp_socp_register_read_cb
 ��������  :�ýӿ�����ע������ݵĻص�������
 �������  : dst_chan_id:Ŀ��ͨ��ID
                  read_cb: �¼��ص�����
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:ע������ݻص������ɹ�
                             SOCP_ERROR:ע������ݻص�����ʧ��
*****************************************************************************/
s32 bsp_socp_register_read_cb( u32 dst_chan_id, socp_read_cb read_cb);

/*****************************************************************************
 �� �� ��      : bsp_socp_register_read_cb
 ��������  :�ô˽ӿ����ڻ�ȡ�����ݻ�����ָ�롣
 �������  : dst_chan_id:Ŀ��ͨ��ID
                  read_cb: ������buffer
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:��ȡ�����ݻ������ɹ���
                             SOCP_ERROR:��ȡ�����ݻ������ɹ���
*****************************************************************************/
s32 bsp_socp_get_read_buff(u32 dst_chan_id,SOCP_BUFFER_RW_STRU *pBuffer);

/*****************************************************************************
 �� �� ��      : bsp_socp_read_data_done
 ��������  :�ýӿ������ϲ����SOCP��������Ŀ��ͨ���ж��ߵ�ʵ�����ݡ�
 �������  : dst_chan_id:Ŀ��ͨ��ID
                  read_size: �Ѷ������ݵĳ���
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:�����ݳɹ���
                             SOCP_ERROR:������ʧ��
*****************************************************************************/
s32 bsp_socp_read_data_done(u32 dst_chan_id,u32 read_size);

/*****************************************************************************
 �� �� ��      : bsp_socp_set_bbp_enable
 ��������  :ʹ�ܻ�ֹͣBBPͨ����
 �������  : b_enable:ͨ��ID
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:���óɹ���
                   ����ֵ:����ʧ��
*****************************************************************************/
s32 bsp_socp_set_bbp_enable(int b_enable);

/*****************************************************************************
 �� �� ��      : bsp_socp_set_bbp_ds_mode
 ��������  :����BBP DSͨ�������������ģʽ��
 �������  : ds_mode:DSͨ���������ʱ����ģʽ����
 �������  : �ޡ�
 �� �� ֵ      : SOCP_OK:���óɹ���
                   ����ֵ:����ʧ��
*****************************************************************************/
s32 bsp_socp_set_bbp_ds_mode(SOCP_BBP_DS_MODE_ENUM_UIN32 ds_mode);

/*****************************************************************************
* �� �� ��  : bsp_socp_get_state
*
* ��������  : ��ȡSOCP״̬
*
* �� �� ֵ  : SOCP_IDLE    ����
*             SOCP_BUSY    æµ
*****************************************************************************/
SOCP_STATE_ENUM_UINT32 bsp_socp_get_state(void);


/*****************************************************************************
* �� �� ��  : bsp_socp_vote
* ��������  : SOCPͶƱ�ӿڣ�����ͶƱ�������SOCP�Ƿ�˯�ߣ��ýӿ�ֻ��A���ṩ
* �������  : id --- ͶƱ���ID��type --- ͶƱ����
* �������  : ��
* �� �� ֵ  : BSP_S32 0 --- ͶƱ�ɹ���0xFFFFFFFF --- ͶƱʧ��
*****************************************************************************/
BSP_S32 bsp_socp_vote(SOCP_VOTE_ID_ENUM_U32 id, SOCP_VOTE_TYPE_ENUM_U32 type);

/*****************************************************************************
* �� �� ��  : bsp_socp_vote_to_mcore
* ��������  : SOCPͶƱ�ӿڣ��ýӿ�ֻ��C���ṩ������LDSP�״μ��ص�SOCP�ϵ�����
* �������  : type --- ͶƱ����
* �������  : ��
* �� �� ֵ  : BSP_S32 0 --- ͶƱ�ɹ���0xFFFFFFFF --- ͶƱʧ��
*****************************************************************************/
BSP_S32 bsp_socp_vote_to_mcore(SOCP_VOTE_TYPE_ENUM_U32 type);

/*****************************************************************************
* �� �� ��  : bsp_socp_get_log_cfg
* ��������  : ��ȡlog����
* �������  :
* �������  :
* �� �� ֵ  :
*****************************************************************************/
struct socp_enc_dst_log_cfg * bsp_socp_get_log_cfg(void);
/*****************************************************************************
* �� �� ��  : bsp_socp_get_sd_logcfg
* ��������  : ��ȡ����
* �������  :
* �������  :
* �� �� ֵ  :
*****************************************************************************/
u32 bsp_socp_get_sd_logcfg(SOCP_ENC_DST_BUF_LOG_CFG_STRU* cfg);
/*****************************************************************************
* �� �� ��  : socp_set_clk_autodiv_enable
* ��������  : ����clk�ӿ�clk_disable_unprepare��bypass��0�������Զ���Ƶ
* �������  : ��
* �������  : ��
* �� �� ֵ  : ��
* ע    ��  :
              clk_prepare_enable �ӿ��� clk_disable_unprepare �ӿڱ���ɶ�ʹ��
              clk���Զ���ƵĬ�ϴ��ڴ�״̬������
              �����Ƚ��� clk_prepare_enable ���ܽ��� clk_disable_unprepare ����
*****************************************************************************/
void bsp_socp_set_clk_autodiv_enable(void);

/*****************************************************************************
* �� �� ��  : socp_set_clk_autodiv_disable
* ��������  : ����clk�ӿ�clk_prepare_enable��bypass��1�������Զ���Ƶ
* �������  : ��
* �������  : ��
* �� �� ֵ  : ��
* ע    ��  :
              clk_prepare_enable �ӿ��� clk_disable_unprepare �ӿڱ���ɶ�ʹ��
              clk���Զ���ƵĬ�ϴ��ڴ�״̬������
              �����Ƚ��� clk_prepare_enable ���ܽ��� clk_disable_unprepare ����
*****************************************************************************/
void bsp_socp_set_clk_autodiv_disable(void);


#if (FEATURE_SOCP_DECODE_INT_TIMEOUT == FEATURE_ON)
/*****************************************************************************
* �� �� ��  : bsp_socp_set_decode_timeout_register
* ��������  :ѡ����볬ʱ�Ĵ���
* �������  : ��
* �������  : ��
* �� �� ֵ  :
*****************************************************************************/
s32 bsp_socp_set_decode_timeout_register(decode_timeout_module_e module);
#endif
/*****************************************************************************
* �� �� ��  : bsp_socp_set_enc_dst_threshold
* ��������  :
* �������  :
* �������  :
* �� �� ֵ  :
*****************************************************************************/
void bsp_socp_set_enc_dst_threshold(bool mode,u32 dst_chan_id);

/*****************************************************************************
* �� �� ��  : bsp_socp_set_ind_mode
*
* ��������  : �����ϱ�ģʽ�ӿ�
*
* �������  : ģʽ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/
s32 bsp_socp_set_ind_mode(SOCP_IND_MODE_ENUM mode);

/*****************************************************************************
* �� �� ��  : bsp_socp_get_log_ind_mode
*
* ��������  : ��ȡ��ǰ�ϱ�ģʽ
*
* �������  : ģʽ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/
s32  bsp_socp_get_log_ind_mode(u32 *log_ind_mode);
/*****************************************************************************
* �� �� ��  : bsp_report_ind_mode_ajust
*
* ��������  : �ϱ�ģʽ����
*
* �������  : ģʽ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/
s32 bsp_report_ind_mode_ajust(SOCP_IND_MODE_ENUM mode);

/*****************************************************************************
* �� �� ��  :  bsp_socp_encsrc_chan_open
*
* ��������  : ��SOCP����Դͨ������
*
* �������  : ��
*
* �������  : ��
*
* �� �� ֵ  : void
*****************************************************************************/
void bsp_socp_encsrc_chan_open(u32 src_chan_id);

/*****************************************************************************
* �� �� ��  : bsp_socp_encsrc_chan_close
*
* ��������  : �ر�socp����Դͨ�����أ����ҵȴ�socp���к��˳�
              �ýӿ����ڷ�ֹsocp�����µ�����
*
* �������  : ��
*
* �������  : ��
*
* �� �� ֵ  : void
*****************************************************************************/
void bsp_socp_encsrc_chan_close(u32 src_chan_id);

/*****************************************************************************
* �� �� ��  : bsp_socp_dump_save
*
* ��������  : �쳣ǰ�洢socp�Ĵ���
*****************************************************************************/
void bsp_socp_dump_save(void);

/*****************************************************************************
* �� �� ��  : bsp_socp_check_state
*
* ��������  : �ж�SOCPͨ���ر��Ժ��״̬���ṩ��TCM�µ�������ʹ��
*
* �������  : Դͨ����
*
* �������  : ��
*
* �� �� ֵ  : SOCP״̬
*****************************************************************************/
s32 bsp_socp_check_state(u32 src_chan_id);

/*****************************************************************************
 �� �� ��  : bsp_socp_update_bbp_ptr
 ��������  : �ô˽ӿ����ڶ�дָ����Ե�ַ��Ϊ��Ե�ַ
 �������  : ulSrcChanId:Դͨ��ID

 �������  : �ޡ�
 �� �� ֵ  : ��
*****************************************************************************/
void bsp_socp_update_bbp_ptr(u32 src_chan_id);


u32 bsp_get_socp_ind_dst_int_slice(void);
/*****************************************************************************
 �� �� ��      : bsp_clear_socp_buff
 ��������  : �ô˽ӿ��������SOCPԴbuff
 �������  : src_chan_id:ͨ��id
 �������  : �ޡ�
 �� �� ֵ     : ��
*****************************************************************************/
s32 bsp_clear_socp_buff(u32 src_chan_id);

/*****************************************************************************
* �� �� ��  : bsp_socp_soft_free_encdst_chan
*
* ��������  : ���ͷű���Ŀ��ͨ��
*
* �������  : enc_dst_chan_id       ����ͨ����
*
* �������  : ��
*
* �� �� ֵ  : �ͷųɹ����ı�ʶ��
*****************************************************************************/
s32 bsp_socp_soft_free_encdst_chan(u32 enc_dst_chan_id);

/*****************************************************************************
* �� �� ��  : bsp_SocpEncDstQueryIntInfo
*
* ��������  : �ṩ��diag_debug��ѯsocp����ͨ��Ŀ�Ķ��ж���Ϣ
*
* �������  : ��
* �������  :
*
* �� �� ֵ  : ��
*****************************************************************************/
void bsp_SocpEncDstQueryIntInfo(u32 *trf_info, u32 *thrh_ovf_info);

/*****************************************************************************
* �� �� ��  : bsp_clear_socp_encdst_int_info
*
* ��������  : ���socpĿ�Ķ�����ͳ��ֵ
*
* �������  : ��
* �������  :
*
* �� �� ֵ  : ��
*****************************************************************************/
void bsp_clear_socp_encdst_int_info(void);

s32 socp_dst_channel_enable(u32 dst_chan_id);

s32 socp_dst_channel_disable(u32 dst_chan_id);
s32 bsp_socp_dst_trans_id_disable(u32 dst_chan_id);


#ifdef __KERNEL__
/*****************************************************************************
 �� �� ��      : bsp_socp_set_rate_ctrl
 ��������  : �ô˽ӿ���������SOCP��������
 �������  : p_rate_ctrl:��������
 �������  : �ޡ�
 �� �� ֵ     : ��
*****************************************************************************/
s32 bsp_socp_set_rate_ctrl(DRV_DIAG_RATE_STRU *p_rate_ctrl);
#endif

#else

static inline void bsp_socp_encsrc_chan_open(u32 src_chan_id)
{
    return;
}

static inline void bsp_socp_encsrc_chan_close(u32 src_chan_id)
{
    return;
}

static inline s32 bsp_socp_check_state(u32 src_chan_id)
{
	return 0;
}

static inline void bsp_socp_dump_save(void)
{
    return;
}

static inline s32 bsp_socp_get_write_buff( u32 src_chan_id, SOCP_BUFFER_RW_STRU *p_rw_buff)
{
    return 0;
}

static inline s32 bsp_socp_write_done(u32 src_chan_id, u32 write_size)
{
    return 0;
}
static inline s32 bsp_socp_coder_set_dest_chan_attr(u32 dst_chan_id, SOCP_CODER_DEST_CHAN_S *dst_attr)
{
    return 0;
}
static inline s32 bsp_socp_coder_set_src_chan(SOCP_CODER_SRC_ENUM_U32 src_chan_id, SOCP_CODER_SRC_CHAN_S *src_attr)
{
    return 0;
}

static inline s32 bsp_socp_start(u32 src_chan_id)
{
    return 0;
}
static inline u32 bsp_get_socp_ind_dst_int_slice(void)
{
    return 0;
}
static inline s32 bsp_socp_set_ind_mode(SOCP_IND_MODE_ENUM mode)
{
	return 0;
}
static inline s32 bsp_socp_get_read_buff(u32 dst_chan_id,SOCP_BUFFER_RW_STRU *p_buffer)
{
	return 0;
}
static inline u32 bsp_socp_get_sd_logcfg(SOCP_ENC_DST_BUF_LOG_CFG_STRU* cfg)
{
	return 0;
}
static inline s32 bsp_socp_read_data_done(u32 dst_chan_id,u32 read_size)
{
	return 0;
}
static inline s32 bsp_socp_register_read_cb( u32 dst_chan_id, socp_read_cb read_cb)
{
	return 0;
}
static inline s32 bsp_socp_register_event_cb(u32 chan_id, socp_event_cb event_cb)
{
	return 0;
}
static inline void bsp_socp_encdst_dsm_init(u32 enc_dst_chan_id, u32 b_enable)
{

}

static inline s32 bsp_clear_socp_buff(u32 src_chan_id)
{
    return 0;
}
static inline s32 bsp_socp_soft_free_encdst_chan(u32 enc_dst_chan_id)
{
    return 0;
}
static inline void socp_m3_init(void)
{
    return;
}

static inline void bsp_SocpEncDstQueryIntInfo(u32 *trf_info, u32 *thrh_ovf_info)
{
    return;
}

static inline void bsp_clear_socp_encdst_int_info(void)
{
    return;
}

static inline s32 socp_dst_channel_enable(u32 dst_chan_id)
{
    return 0;
}

static inline s32 socp_dst_channel_disable(u32 dst_chan_id)
{
    return 0;
}

static inline s32 bsp_socp_set_rate_ctrl(DRV_DIAG_RATE_STRU *p_rate_ctrl)
{
    return 0;
}
static inline s32 bsp_socp_dst_trans_id_disable(u32 dst_chan_id)
{
    return 0;

}

#endif

/*****************************************************************************
* �� �� ��  : bsp_socp_register_compress
*
* ��������  : ѹ���ӿں���ע�ᵽsocp��
*
* �������  : ѹ���ӿڲ���
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/

s32 bsp_socp_register_compress(socp_compress_ops_stru *ops);

/*****************************************************************************
* �� �� ��  : bsp_deflate_get_log_ind_mode
*
* ��������  : ��ȡ��ǰѹ���ϱ�ģʽ�ӿ�
*
* �������  : �ϱ�ģʽ
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/
s32  bsp_deflate_get_log_ind_mode(u32 *log_ind_mode);
/*****************************************************************************
* �� �� ��  : bsp_deflate_cfg_ind_mode
*
* ��������  : ѹ���ϱ�ģʽ�ӿ�
*
* �������  : �ϱ�ģʽ
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/

s32 bsp_deflate_set_ind_mode(SOCP_IND_MODE_ENUM mode);
/*****************************************************************************
* �� �� ��  : bsp_socp_compress_disable
*
* ��������  : ѹ��ʹ�ܽӿ�
*
* �������  : Ŀ��ͨ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/
s32 bsp_socp_compress_disable(u32 dst_chan_id);
/*****************************************************************************
* �� �� ��  : bsp_socp_compress_enable
*
* ��������  : ѹ��ʹ�ܽӿ�
*
* �������  : Ŀ��ͨ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_S32 BSP_OK:�ɹ� BSP_ERROR:ʧ��
*****************************************************************************/
s32 bsp_socp_compress_enable(u32 dst_chan_id);

s32 bsp_socp_set_cfg_ind_mode(SOCP_IND_MODE_ENUM mode);
s32 bsp_socp_get_cfg_ind_mode(u32 *Cfg_ind_mode);
s32 bsp_socp_set_cps_ind_mode(DEFLATE_IND_COMPRESSS_ENUM mode);
s32 bsp_socp_get_cps_ind_mode(u32 *cps_ind_mode);
s32 bsp_socp_compress_status(void);

#ifdef __cplusplus
}
#endif

#endif /* end of _BSP_SOCP_H*/


