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

#include "diag_service.h"
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <mdrv.h>
#include <securec.h>
#include <bsp_slice.h>
#include <osl_spinlock.h>
#include <osl_sem.h>
#include <osl_list.h>
#include <osl_malloc.h>
#include <osl_sem.h>
#include "msp_service.h"
#include "soc_socp_adapter.h"
#include "diag_system_debug.h"
#include "scm_ind_src.h"
#include "scm_cnf_src.h"


#define THIS_MODU mod_diag

#define DIAG_PKT_TIMEOUT_LEN (32768 * 3) /* 3s */

diag_dfr_info_s g_dfr_cnf = {};
spinlock_t g_ind_src_buf_spin_lock;
spinlock_t g_cnf_src_buf_spin_lock;

diag_srv_proc_cb g_diag_srv_fn = NULL;
diag_dfr_info_s g_dfr_req = {};

typedef struct {
    osl_sem_id sem_id;
    struct list_head list_header;
} diag_srv_list_s;

diag_srv_list_s g_diag_srv_list = {};


diag_srv_ctrl_s g_diag_srv_ctrl = {
    SOCP_CODER_SRC_PS_IND,
    SOCP_CODER_SRC_CNF,
};

typedef struct {
    diag_service_head_s stHead;
    u32 timestamp; /* �����ڵ��ʱ��������ڼ�ⳬʱ */
    u32 ulFrameDataLen;
    u32 ulFrameOffset;
    struct list_head FrameList;
    diag_frame_head_s *pFrame;
} diag_srv_node_s;

void diag_srv_fill_head(diag_srv_head_s *srv_head)
{
    u32 timestamp[2];
    u32 timestamp_len = sizeof(srv_head->frame_header.srv_head.timestamp);
    errno_t ret;

    /* Ĭ�ϲ��ְ������ϱ� */
    srv_head->socp_header.hisi_magic = DIAG_SOCP_HEAD_MAGIC;
    srv_head->socp_header.data_len = 0;

    srv_head->frame_header.srv_head.sid_8b = DIAG_FRAME_MSP_SID_DIAG_SERVICE;
    srv_head->frame_header.srv_head.mdmid_3b = 0;
    srv_head->frame_header.srv_head.rsv_1b = 0;
    srv_head->frame_header.srv_head.ssid_4b = DIAG_FRAME_SSID_APP_CPU;
    srv_head->frame_header.srv_head.sessionid_8b = MSP_SERVICE_SESSION_ID;
    /* Ĭ��Ϊ�����ϱ� */
    srv_head->frame_header.srv_head.mt_2b = DIAG_FRAME_MT_IND;
    srv_head->frame_header.srv_head.index_4b = 0;
    srv_head->frame_header.srv_head.eof_1b = 0;
    srv_head->frame_header.srv_head.ff_1b = 0;
    srv_head->frame_header.srv_head.msg_trans_id = 0;

    (void)mdrv_timer_get_accuracy_timestamp(&timestamp[1], &timestamp[0]);
    ret = memcpy_s(srv_head->frame_header.srv_head.timestamp,
                   sizeof(srv_head->frame_header.srv_head.timestamp), (u8 *)timestamp, timestamp_len);
    if (ret != EOK) {
        diag_error("memory copy error 0x%x\n", ret);
    }
}
/*
 * Function Name: diag_srv_pkt_timeout_clear
 * Description: �鿴�������Ƿ��г�ʱ�Ľڵ㣬�������ɾ��
 */
void diag_srv_pkt_timeout_clear(void)
{
    diag_srv_node_s *p_curr_node = NULL;
    diag_srv_node_s *p_next_node = NULL;
    u32 cur_slice = bsp_get_slice_value();

    osl_sem_down(&g_diag_srv_list.sem_id);

    list_for_each_entry_safe(p_curr_node, p_next_node, &g_diag_srv_list.list_header, FrameList)
    {
        if ((cur_slice > p_curr_node->timestamp) && ((cur_slice - p_curr_node->timestamp) > DIAG_PKT_TIMEOUT_LEN)) {
            if ((NULL != p_curr_node->FrameList.next) && (NULL != p_curr_node->FrameList.prev)) {
                list_del(&(p_curr_node->FrameList));
            }

            diag_crit("diag_srv_pkt_timeout_clear delete node.\n");

            if (NULL != p_curr_node->pFrame) {
                osl_free(p_curr_node->pFrame);
            }

            osl_free(p_curr_node);
        }
    }

    (void)osl_sem_up(&(g_diag_srv_list.sem_id));
}

/*
 * Function Name: diag_srv_create_pkt
 * Description: �յ���һ���ְ�ʱ��������ͽڵ�
 * History:
 *    1.         2015-03-18  Draft Enact
 */
/*lint -save -e429*/
u32 diag_srv_create_pkt(diag_frame_head_s *frame)
{
    struct list_head *me = NULL;
    diag_srv_node_s *tmp_node = NULL;

    /* ��Ϣ���Ȳ��ܴ�����󳤶� */
    if (frame->msg_len + sizeof(diag_frame_head_s) > DIAG_FRAME_SUM_LEN) {
        diag_error("msg len too large, msg_len = 0x%x\n", frame->msg_len);
        return ERR_MSP_INALID_LEN_ERROR;
    }

    /* ����������Ѿ�����ͬtransid�Ľڵ���ֱ���˳� */
    osl_sem_down(&g_diag_srv_list.sem_id);

    list_for_each(me, &g_diag_srv_list.list_header)
    {
        tmp_node = list_entry(me, diag_srv_node_s, FrameList);

        tmp_node->stHead.index_4b = frame->srv_head.index_4b;
        tmp_node->stHead.eof_1b = frame->srv_head.eof_1b;
        tmp_node->stHead.ff_1b = frame->srv_head.ff_1b;
        if (memcmp(&tmp_node->stHead, &frame->srv_head, sizeof(diag_service_head_s)) == 0) {
            osl_sem_up(&g_diag_srv_list.sem_id);
            diag_error("there is a uniform packet in list.\n");
            return ERR_MSP_FAILURE;
        }
    }
    osl_sem_up(&g_diag_srv_list.sem_id);

    /* �����ڵ㣬�����ڴ� */
    tmp_node = (diag_srv_node_s *)osl_malloc(sizeof(diag_srv_node_s));
    if (NULL == tmp_node) {
        diag_error("malloc pTempNode fail\n");
        return ERR_MSP_DIAG_ALLOC_MALLOC_FAIL;
    }

    tmp_node->ulFrameDataLen = frame->msg_len + sizeof(diag_frame_head_s);
    tmp_node->ulFrameOffset = 0;
    tmp_node->pFrame = (diag_frame_head_s *)osl_malloc(tmp_node->ulFrameDataLen);
    if (NULL == tmp_node->pFrame) {
        diag_error("malloc pFrame fail\n");
        osl_free(tmp_node);
        return ERR_MSP_DIAG_ALLOC_MALLOC_FAIL;
    }

    osl_sem_down(&g_diag_srv_list.sem_id);

    memcpy_s(&tmp_node->stHead, (u32)sizeof(tmp_node->stHead), &frame->srv_head, sizeof(frame->srv_head));

    list_add_tail(&tmp_node->FrameList, &g_diag_srv_list.list_header);

    tmp_node->timestamp = mdrv_timer_get_normal_timestamp();

    osl_sem_up(&g_diag_srv_list.sem_id);

    return ERR_MSP_SUCCESS;
}
/*lint -restore*/

/*
 * Function Name: diag_srv_save_pkt
 * Description: �յ��ְ�ʱ�ѷְ����ݿ�����������
 * History:
 *    1.         2015-03-18  Draft Enact
 */
diag_frame_head_s *diag_srv_save_pkt(diag_frame_head_s *frame, u32 len)
{
    struct list_head *me = NULL;
    diag_srv_node_s *tmp_node = NULL;
    u32 ops_len = 0;
    u32 offset = 0;
    u32 local_len = 0;
    errno_t ret = 0;

    osl_sem_down(&g_diag_srv_list.sem_id);

    list_for_each(me, &g_diag_srv_list.list_header)
    {
        tmp_node = list_entry(me, diag_srv_node_s, FrameList);

        /* �˴�ע��stService��4G ��5G������ */
        tmp_node->stHead.index_4b = frame->srv_head.index_4b;
        tmp_node->stHead.eof_1b = frame->srv_head.eof_1b;
        tmp_node->stHead.ff_1b = frame->srv_head.ff_1b;
        if (0 == memcmp(&tmp_node->stHead, &frame->srv_head, sizeof(diag_service_head_s))) {
            if (0 == frame->srv_head.index_4b) /* ��0֡ */
            {
                /* ��0֡��Ҫ����header, cmd_id, meglen and data */
                ret = memcpy_s(tmp_node->pFrame, tmp_node->ulFrameDataLen, frame, len);
                if (ret != EOK) {
                     diag_error("memory copy fail %x\n", ret);
                }
                tmp_node->ulFrameOffset = len;
                diag_save_ptr(&g_dfr_req, (u8 *)frame, len);
            } else if (frame->srv_head.eof_1b) /* ���1֡ */
            {
                /* �����һ֡�⣬�Ѵ洢�����ݳ��� */
                ops_len = tmp_node->ulFrameOffset - sizeof(diag_frame_head_s);
                local_len = len - sizeof(diag_service_head_s);

                if ((NULL == tmp_node->pFrame) || (tmp_node->ulFrameOffset + local_len > DIAG_FRAME_SUM_LEN) ||
                    (tmp_node->pFrame->msg_len != ops_len + local_len) || (tmp_node->ulFrameDataLen < tmp_node->ulFrameOffset)) {
                    diag_error("rev data len error, length:0x%x ulLocalLen:0x%x\n", ops_len, local_len);
                    osl_sem_up(&g_diag_srv_list.sem_id);
                    return NULL;
                }

                /* δ��������ݳ��� */
                ops_len = tmp_node->pFrame->msg_len - ops_len;

                /* ��ǰ��������ƫ�� */
                offset = tmp_node->ulFrameOffset;

                /* ���һֻ֡��Ҫ����ʣ��data */
                ret = memcpy_s(((u8 *)tmp_node->pFrame) + offset,
                               tmp_node->ulFrameDataLen - tmp_node->ulFrameOffset,
                               ((u8 *)frame) + sizeof(diag_service_head_s), ops_len);
                if (ret != EOK) {
                     diag_error("memory copy fail %x\n", ret);
                }
                tmp_node->ulFrameOffset += ops_len;
                diag_save_ptr(&g_dfr_req, (u8 *)frame, len);
            } else {
                /* ��ǰ��������ƫ�� */
                offset = tmp_node->ulFrameOffset;
                local_len = len - sizeof(diag_service_head_s);

                if ((NULL == tmp_node->pFrame) || (offset + local_len > DIAG_FRAME_SUM_LEN) ||
                    (tmp_node->pFrame->msg_len < (offset - sizeof(diag_frame_head_s) + local_len)) ||
                    (tmp_node->ulFrameDataLen < offset)) {
                    diag_error("msg len error, uloffset:0x%x ulLocallen:0x%x\n", offset, local_len);
                    osl_sem_up(&g_diag_srv_list.sem_id);
                    return NULL;
                }

                /* �м��֡������cmdid�ͳ��ȣ�ֻ��Ҫ����data */
                ret = memcpy_s(((u8 *)tmp_node->pFrame) + offset, (u32)(tmp_node->ulFrameDataLen - offset),
                               ((u8 *)frame) + sizeof(diag_service_head_s), local_len);
                if (ret != EOK) {
                     diag_error("memory copy fail %x\n", ret);
                }
                tmp_node->ulFrameOffset += local_len;
                diag_save_ptr(&g_dfr_req, (u8 *)frame, len);
            }

            osl_sem_up(&g_diag_srv_list.sem_id);
            return tmp_node->pFrame;
        }
    }

    osl_sem_up(&g_diag_srv_list.sem_id);

    return (diag_frame_head_s *)NULL;
}

/*
 * Function Name: diag_srv_destroy_pkt
 * Description: ɾ������ͽڵ�
 */
void diag_srv_destroy_pkt(diag_frame_head_s *frame)
{
    struct list_head *me = NULL;
    diag_srv_node_s *tmp_node = NULL;

    osl_sem_down(&g_diag_srv_list.sem_id);

    list_for_each(me, &g_diag_srv_list.list_header)
    {
        tmp_node = list_entry(me, diag_srv_node_s, FrameList);

        if (NULL == tmp_node) {
            osl_sem_up(&g_diag_srv_list.sem_id);
            return;
        }

        /* �˴�ע��stService��4G ��5G������ */
        tmp_node->stHead.index_4b = frame->srv_head.index_4b;
        tmp_node->stHead.eof_1b = frame->srv_head.eof_1b;
        tmp_node->stHead.ff_1b = frame->srv_head.ff_1b;
        if (0 == memcmp(&tmp_node->stHead, &frame->srv_head, sizeof(diag_service_head_s))) {
            /* ɾ���ڵ� */
            if ((NULL != tmp_node->FrameList.next) && (NULL != tmp_node->FrameList.prev)) {
                list_del(&tmp_node->FrameList);
            }

            if (NULL != tmp_node->pFrame) {
                osl_free(tmp_node->pFrame);
            }

            osl_free(tmp_node);
            break;
        }
    }

    osl_sem_up(&g_diag_srv_list.sem_id);

    return;
}

u32 diag_srv_get_data(diag_frame_head_s *rcv_head, u32 len, diag_frame_head_s **new_head)
{
    diag_frame_head_s *tmp_frame = NULL;

    if (rcv_head->srv_head.ff_1b) {
        /* ÿ���зְ�ʱ����Ƿ��г�ʱ�Ľڵ� */
        diag_srv_pkt_timeout_clear();

        /* index4b��Զ�������16, ����Ϣ���֡����������16,���index�����ܴ���16 */
        if (0 == rcv_head->srv_head.index_4b) {
            diag_srv_create_pkt(rcv_head);
            (void)diag_srv_save_pkt(rcv_head, len);
            return ERR_MSP_SUCCESS;
        } else if (rcv_head->srv_head.eof_1b) {
            tmp_frame = diag_srv_save_pkt(rcv_head, len);
            if (tmp_frame == NULL) {
                return ERR_MSP_FAILURE;
            }
            /* 5G�зְ��Ľڵ�һ�����ߵ�5G��ʽ,4G�·ְ�һ�����ߵ�4G�ĸ�ʽ */
            len = tmp_frame->msg_len + sizeof(diag_frame_head_s);

            *new_head = tmp_frame;
            return SERVICE_MSG_PROC;
        } else {
            (void)diag_srv_save_pkt(rcv_head, len);
            return ERR_MSP_SUCCESS;
        }
    } else {
        if (len < rcv_head->msg_len + sizeof(diag_frame_head_s)) {
            diag_error("rev tools data len error, rev:0x%x except:0x%x\n", len,
                       rcv_head->msg_len + (u32)sizeof(diag_frame_head_s));
            return ERR_MSP_INVALID_PARAMETER;
        }
        diag_save_ptr(&g_dfr_req, (u8 *)rcv_head, (sizeof(diag_frame_head_s) + rcv_head->msg_len));

        *new_head = rcv_head;

        return SERVICE_MSG_PROC;
    }
}

/*
 * Function Name: diag_srv_proc
 * Description: DIAG service ������
 */
u32 diag_srv_proc(void *data, u32 len)
{
    u32 ret = BSP_ERROR;
    diag_frame_head_s *rcv_head = NULL;
    diag_frame_head_s *new_head = NULL;

    if ((NULL == data) || (len < sizeof(diag_service_head_s))) {
        diag_error("para error:data_len=0x%x\n", len);
        return BSP_ERROR;
    }

    rcv_head = (diag_frame_head_s *)data;

    diag_ptr(EN_DIAG_PTR_SERVICE_IN, 1, rcv_head->cmd_id, 0);

    /* ֻ����DIAG���� */
    if (DIAG_FRAME_MSP_SID_DIAG_SERVICE == SERVICE_HEAD_SID(data)) {
        diag_ptr(EN_DIAG_PTR_SERVICE_1, 1, rcv_head->cmd_id, 0);

        /* ��ʼ����������˯�� */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
        __pm_stay_awake(g_diag_srv_ctrl.stWakelock);
#else
        __pm_stay_awake(&g_diag_srv_ctrl.stWakelock);
#endif

        ret = diag_srv_get_data(rcv_head, len, &new_head);
        if (ret != SERVICE_MSG_PROC) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
            __pm_relax(g_diag_srv_ctrl.stWakelock);
#else
            __pm_relax(&g_diag_srv_ctrl.stWakelock);
#endif
            return ret;
        }
        
        if (g_diag_srv_fn && (new_head != NULL)) {
            /* ��¼�����N��cmdid */
            diag_lnr(EN_DIAG_DRV_LNR_CMDID, rcv_head->cmd_id, bsp_get_slice_value());          
            ret = g_diag_srv_fn((void *)new_head);

        } else {
            ret = ERR_MSP_NO_INITILIZATION;
            diag_ptr(EN_DIAG_PTR_DIAG_SERVICE_ERR1, 1, rcv_head->cmd_id, 0);
        }

        if (rcv_head->srv_head.ff_1b) {
            diag_srv_destroy_pkt(rcv_head);
        }

        /* �������������˯�� */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
        __pm_relax(g_diag_srv_ctrl.stWakelock);
#else
        __pm_relax(&g_diag_srv_ctrl.stWakelock);
#endif
    } else {
        diag_ptr(EN_DIAG_PTR_DIAG_SERVICE_ERR2, 1, rcv_head->cmd_id, 0);
        ret = ERR_MSP_INVALID_PARAMETER;
    }

    return ret;
}

/*
 * Function Name: diag_srv_proc_func_reg
 * Description: DIAG service����ע��ӿ�
 * History:
 *    1.         2014-11-18  Draft Enact
 */
void diag_srv_proc_func_reg(diag_srv_proc_cb srv_fn)
{
    g_diag_srv_fn = srv_fn;
}

void diag_srv_init(void)
{
    u32 ret;

    spin_lock_init(&g_ind_src_buf_spin_lock);
    spin_lock_init(&g_cnf_src_buf_spin_lock);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    g_diag_srv_ctrl.stWakelock = wakeup_source_register(NULL, "diag_srv_lock");
    if (g_diag_srv_ctrl.stWakelock == NULL) {
        diag_error("diag srv wakeup lock register err\n");
    }
#else
    wakeup_source_init(&g_diag_srv_ctrl.stWakelock, "diag_srv_lock");
#endif

    /* �����ڵ㱣���ź��� */
    osl_sem_init(1, &g_diag_srv_list.sem_id);

    /* ��ʼ���������� */
    INIT_LIST_HEAD(&g_diag_srv_list.list_header);

    msp_srv_proc_func_reg(DIAG_FRAME_MSP_SID_DIAG_SERVICE, diag_srv_proc);

    ret = diag_ctreate_dfr("DFRReqA", DIAG_DFR_BUFFER_MAX, &g_dfr_req);
    if (ret) {
        diag_error("DFR_Req failed.\n");
    }

    ret = diag_ctreate_dfr("DFRCnfA", DIAG_DFR_BUFFER_MAX, &g_dfr_cnf);

    if (ret) {
        diag_error("DFR_Cnf failed.\n");
    }
}

/*
 * �� �� ��:diag_srv_write_data
 * ��������: д�����ݣ�����дָ��
 * �������: id����Ϣ���ݡ�����
 * �������: ��
 * �� �� ֵ: �Ƿ�ɹ���־
 */
void diag_srv_write_data(SOCP_BUFFER_RW_STRU *rw_buffer, const void *payload, u32 len)
{
    u32 size;
    u32 rb_size;
    errno_t ret = 0;

    if (0 == len) {
        return;
    }

    if (rw_buffer->u32Size > len) {
        if (rw_buffer->pBuffer != NULL) {
            ret = memcpy_s(((u8 *)rw_buffer->pBuffer), rw_buffer->u32Size, payload, len);
            if (ret != EOK) {
                diag_error("memory copy fail %x\n", ret);
            }
            rw_buffer->pBuffer = rw_buffer->pBuffer + len;
            rw_buffer->u32Size = rw_buffer->u32Size - len;
        }
    } else {
        size = rw_buffer->u32Size;
        ret = memcpy_s(((u8 *)rw_buffer->pBuffer), rw_buffer->u32Size, payload, size);
        if (ret != EOK) {
            diag_error("memory copy fail %x\n", ret);
        }

        rb_size = len - rw_buffer->u32Size;
        if (rb_size && rw_buffer->pRbBuffer != NULL) {
            ret = memcpy_s(rw_buffer->pRbBuffer, rw_buffer->u32RbSize, ((u8 *)payload + size), rb_size);
            if (ret != EOK) {
                diag_error("memory copy fail %x\n", ret);
            }
        }

        rw_buffer->pBuffer = rw_buffer->pRbBuffer + rb_size;
        rw_buffer->u32Size = rw_buffer->u32RbSize - rb_size;
        rw_buffer->pRbBuffer = NULL;
        rw_buffer->u32RbSize = 0;
    }

    return;
}

/*
 * Function Name: diag_SrvcPackFirst
 * Description: ���ְ�ʱ�ķ�װ����ְ�ʱ����һ���ķ�װ
 */
u32 diag_srv_ind_send(diag_msg_report_head_s *msg)
{
    SOCP_BUFFER_RW_STRU socp_buf = { NULL, NULL, 0, 0 };
    diag_socp_head_s *socp_header;
    u32 tmp_len = 0;
    u32 ret = ERR_MSP_FAILURE;
    unsigned long lock;

    tmp_len = ALIGN_DDR_WITH_8BYTE(msg->header_size + msg->data_size);

    socp_header = &(((diag_srv_head_s *)(msg->header))->socp_header);
    socp_header->data_len = msg->header_size + msg->data_size - sizeof(diag_socp_head_s);

    spin_lock_irqsave(&g_ind_src_buf_spin_lock, lock);

    if (mdrv_socp_get_write_buff(g_diag_srv_ctrl.ind_chan_id, &socp_buf)) {
        /* no print because it is a noraml way */
        spin_unlock_irqrestore(&g_ind_src_buf_spin_lock, lock);
        return ERR_MSP_GET_WRITE_BUFF_FAIL; /* ����ʧ�� */
    }

    diag_buffer_overflow_record(socp_buf.u32Size + socp_buf.u32RbSize);

    /* �����ַת�� */
    if ((socp_buf.u32Size + socp_buf.u32RbSize) >= (tmp_len)) {
        socp_buf.pBuffer = (char *)scm_ind_src_phy_to_virt((u8 *)socp_buf.pBuffer);
        socp_buf.pRbBuffer = (char *)scm_ind_src_phy_to_virt((u8 *)socp_buf.pRbBuffer);
    } else {
        spin_unlock_irqrestore(&g_ind_src_buf_spin_lock, lock);
        diag_debug_ind_src_lost(tmp_len);
        return ERR_MSP_NOT_FREEE_SPACE;
    }

    if (!(socp_buf.pBuffer) && (!socp_buf.pRbBuffer)) {
        spin_unlock_irqrestore(&g_ind_src_buf_spin_lock, lock);
        diag_error("get virt fail\n");
        return ERR_MSP_GET_VIRT_ADDR_FAIL;
    }

    diag_srv_write_data(&socp_buf, msg->header, msg->header_size);
    diag_srv_write_data(&socp_buf, msg->data, msg->data_size);

    ret = (u32)mdrv_socp_write_done(g_diag_srv_ctrl.ind_chan_id, (tmp_len));
    spin_unlock_irqrestore(&g_ind_src_buf_spin_lock, lock);
    if (ret) {
        diag_error("write done fail,ret:0x%x\n", ret);
        return ERR_MSP_WRITE_DONE_FAIL;
    }

    return ERR_MSP_SUCCESS;
}

/*
 * Function Name: diag_srv_pack_ind_data
 * Description: DIAG service�����ϱ����ݽӿ�
 * History:
 *    1.         2014-11-18  Draft Enact
 *    2.         2015-03-14  �����ְ��������
 *                    ��֡�ṹ���ƣ��ְ����������Լ��:
 *                    A. ��һ����ulCmdId��ulMsgLen�������ֱ�Ӹ�����
 *                    B. �����һ���⣬����ÿ�������뱣֤����󳤶����
 *                    C. transid��timestamp��Ϊ����һ��ְ��ı�־
 */
u32 diag_srv_pack_ind_data(diag_msg_report_head_s *msg)
{
    diag_msg_report_head_s report_info;
    diag_frame_head_s *frame_header = NULL;
    u32 ret = ERR_MSP_FAILURE;
    s32 data_len = 0; /* �����ܳ��� */
    u32 cur_len = 0; /* ��ǰ�ѷ�������ݳ��� */
    u32 pack_index = 0;
    u32 frame_header_size = 0;
    u32 send_len = 0;

    frame_header_size = msg->header_size - sizeof(diag_socp_head_s);

    /* ��Ҫ�������ݵ��ܳ��� */
    data_len = (s32)(frame_header_size + msg->data_size);
    if (data_len > (s32)(DIAG_FRAME_SUM_LEN - 15 * sizeof(diag_frame_head_s))) {
        diag_error("diag report length is %d.\n", data_len);
        return ERR_MSP_FAILURE;
    }

    diag_package_record(data_len);

    frame_header = &(((diag_srv_head_s *)(msg->header))->frame_header);

    cur_len = (u32)data_len;
    if (data_len > DIAG_FRAME_MAX_LEN) {
        cur_len = DIAG_FRAME_MAX_LEN;
        frame_header->srv_head.ff_1b = 1;
    } else {
        frame_header->srv_head.ff_1b = 0;
    }

    report_info.header = msg->header;
    report_info.header_size = msg->header_size;
    report_info.data = msg->data;
    report_info.data_size = cur_len - frame_header_size;
    send_len += report_info.data_size;

    /* ���ڷְ�ʱ��һ������cmdid��Ҫ��䣬������û�У����Ե�һ���������� */
    ret = diag_srv_ind_send(&report_info);
    if (ret) {
        return ret;
    }

    /* ��Ҫ�ְ� */
    if (frame_header->srv_head.ff_1b) {
        /* ʣ���û�з��͵����ݵĳ��� */
        data_len = data_len - (s32)cur_len;

        while (data_len > 0) {
            frame_header->srv_head.index_4b = ++pack_index;
            if ((data_len + sizeof(diag_service_head_s)) > DIAG_FRAME_MAX_LEN) {
                cur_len = DIAG_FRAME_MAX_LEN - sizeof(diag_service_head_s);
            } else {
                cur_len = (u32)data_len;
                frame_header->srv_head.eof_1b = 1; /* ��¼�ְ�������� */
            }
            report_info.header = msg->header;
            report_info.header_size = sizeof(diag_socp_head_s) + sizeof(diag_service_head_s);
            report_info.data = (u8 *)(msg->data) + send_len;
            report_info.data_size = cur_len;
            send_len += report_info.data_size;

            ret = diag_srv_ind_send(&report_info);
            if (ret) {
                return ERR_MSP_FAILURE;
            }

            data_len -= (s32)cur_len;
        }
    }

    return ERR_MSP_SUCCESS;
}

/*
 * Function Name: diag_srv_cnf_send
 * Description: diag_service��cnf���ݷ���
 */
u32 diag_srv_cnf_send(diag_msg_report_head_s *msg)
{
    SOCP_BUFFER_RW_STRU socp_buf = { NULL, NULL, 0, 0 };
    diag_socp_head_s *socp_header;
    u32 tmp_len = 0;
    u32 ret = ERR_MSP_FAILURE;
    unsigned long lock;

    tmp_len = ALIGN_DDR_WITH_8BYTE((msg->header_size + msg->data_size));

    socp_header = &(((diag_srv_head_s *)(msg->header))->socp_header);
    socp_header->data_len = msg->header_size + msg->data_size - sizeof(diag_socp_head_s);

    spin_lock_irqsave(&g_cnf_src_buf_spin_lock, lock);

    if (mdrv_socp_get_write_buff(g_diag_srv_ctrl.cnf_chan_id, &socp_buf)) {
        spin_unlock_irqrestore(&g_cnf_src_buf_spin_lock, lock);
        return ERR_MSP_GET_WRITE_BUFF_FAIL; /* ����ʧ�� */
    }
    /* �����ַת�� */
    if ((socp_buf.u32Size + socp_buf.u32RbSize) >= (tmp_len)) {
        socp_buf.pBuffer = (char *)scm_cnf_src_phy_to_virt((u8 *)socp_buf.pBuffer);
        socp_buf.pRbBuffer = (char *)scm_cnf_src_phy_to_virt((u8 *)socp_buf.pRbBuffer);
    } else {
        spin_unlock_irqrestore(&g_cnf_src_buf_spin_lock, lock);
        return ERR_MSP_NOT_FREEE_SPACE;
    }

    if (!(socp_buf.pBuffer) && (!socp_buf.pRbBuffer)) {
        spin_unlock_irqrestore(&g_cnf_src_buf_spin_lock, lock);
        diag_error("get virt fail\n");
        return ERR_MSP_GET_VIRT_ADDR_FAIL;
    }

    diag_srv_write_data(&socp_buf, msg->header, msg->header_size);
    diag_srv_write_data(&socp_buf, msg->data, msg->data_size);

    ret = (u32)mdrv_socp_write_done(g_diag_srv_ctrl.cnf_chan_id, (tmp_len));
    spin_unlock_irqrestore(&g_cnf_src_buf_spin_lock, lock);
    if (ret) {
        diag_error("write done fail,ret:0x%x\n", ret);
        return ERR_MSP_WRITE_DONE_FAIL;
    }

    if (msg->data) {
        diag_save_ptr(&g_dfr_cnf, (u8 *)msg->header,
                     (u32)(msg->header_size - sizeof(diag_socp_head_s)));
    }

    return ERR_MSP_SUCCESS;
}

/*
 * Function Name: diag_srv_pack_cnf_data
 * Description: DIAG service�����ϱ����ݽӿ�
 * History:
 *    1.         2014-11-18  Draft Enact
 *    2.         2015-03-14  �����ְ��������
 *                    ��֡�ṹ���ƣ��ְ����������Լ��:
 *                    A. ��һ����ulCmdId��ulMsgLen�������ֱ�Ӹ�����
 *                    B. �����һ���⣬����ÿ�������뱣֤����󳤶����
 *                    C. transid��timestamp��Ϊ����һ��ְ��ı�־
 */
u32 diag_srv_pack_cnf_data(diag_msg_report_head_s *msg)
{
    diag_msg_report_head_s report_info;
    diag_frame_head_s *frame_header;
    u32 ret = ERR_MSP_FAILURE;
    s32 data_len = 0; /* �����ܳ��� */
    u32 cur_len = 0; /* ��ǰ�ѷ�������ݳ��� */
    u32 pack_index = 0;
    u32 frame_header_size = 0;
    u32 send_len = 0;

    frame_header_size = msg->header_size - sizeof(diag_socp_head_s);
    frame_header = &(((diag_srv_head_s *)(msg->header))->frame_header);

    diag_ptr(EN_DIAG_PTR_SERVICE_PACKETDATA, 1, frame_header->cmd_id, 0);

    /* ��Ҫ�������ݵ��ܳ��� */
    data_len = (s32)(frame_header_size + msg->data_size);
    if (data_len > (s32)(DIAG_FRAME_SUM_LEN - 15 * sizeof(diag_frame_head_s))) {
        diag_error("diag report length is %d.\n", data_len);
        return ERR_MSP_FAILURE;
    }

    /* ��������ͷ */
    frame_header = &(((diag_srv_head_s *)(msg->header))->frame_header);
    frame_header->srv_head.mt_2b = DIAG_FRAME_MT_CNF;

    cur_len = (u32)data_len;
    if (data_len > DIAG_FRAME_MAX_LEN) {
        cur_len = DIAG_FRAME_MAX_LEN;
        frame_header->srv_head.ff_1b = 1;
    } else {
        frame_header->srv_head.ff_1b = 0;
    }

    report_info.header = msg->header;
    report_info.header_size = msg->header_size;
    report_info.data = msg->data;
    report_info.data_size = cur_len - frame_header_size;
    send_len += report_info.data_size;

    /* ���ڷְ�ʱ��һ������cmdid��Ҫ��䣬������û�У����Ե�һ���������� */
    ret = diag_srv_cnf_send(&report_info);
    if (ret) {
        return ERR_MSP_FAILURE;
    }

    /* ��Ҫ�ְ� */
    if (frame_header->srv_head.ff_1b) {
        /* ʣ���û�з��͵����ݵĳ��� */
        data_len = data_len - (s32)cur_len;

        while (data_len > 0) {
            frame_header->srv_head.index_4b = ++pack_index;
            if ((data_len + sizeof(diag_service_head_s)) > DIAG_FRAME_MAX_LEN) {
                cur_len = DIAG_FRAME_MAX_LEN - sizeof(diag_service_head_s);
            } else {
                cur_len = (u32)data_len;
                frame_header->srv_head.eof_1b = 1; /* ��¼�ְ�������� */
            }
            report_info.header = msg->header;
            report_info.header_size = sizeof(diag_socp_head_s) + sizeof(diag_service_head_s);
            report_info.data = (u8 *)(msg->data) + send_len;
            report_info.data_size = cur_len;
            send_len += report_info.data_size;

            ret = diag_srv_cnf_send(&report_info);
            if (ret) {
                return ERR_MSP_FAILURE;
            }

            data_len -= (s32)cur_len;
        }
    }

    return ERR_MSP_SUCCESS;
}

u32 diag_srv_pack_reset_data(diag_msg_report_head_s *msg)
{
    diag_msg_report_head_s report_info;
    diag_frame_head_s *frame_header;
    u32 ret = ERR_MSP_FAILURE;
    s32 data_len = 0; /* �����ܳ��� */
    u32 cur_len = 0; /* ��ǰ�ѷ�������ݳ��� */

    frame_header = &(((diag_srv_head_s *)(msg->header))->frame_header);

    /* ��Ҫ�������ݵ��ܳ��� */
    data_len = (s32)(msg->header_size + msg->data_size);
    if (data_len - sizeof(diag_socp_head_s) > (s32)(DIAG_FRAME_SUM_LEN - 15 * sizeof(diag_frame_head_s))) {
        diag_error("report length error(0x%x).\n", data_len);
        return ERR_MSP_FAILURE;
    }

    cur_len = (u32)data_len;
    if (data_len > DIAG_FRAME_MAX_LEN) {
        return ERR_MSP_INALID_LEN_ERROR;
    }

    /* ��������ͷ */
    frame_header = &(((diag_srv_head_s *)(msg->header))->frame_header);
    frame_header->srv_head.mt_2b = DIAG_FRAME_MT_IND;
    frame_header->srv_head.ff_1b = 0;
    frame_header->msg_len = (u32)data_len - sizeof(diag_srv_head_s);

    report_info.header = msg->header;
    report_info.header_size = msg->header_size;
    report_info.data = msg->data;
    report_info.data_size = cur_len - msg->header_size;

    /* ���ڷְ�ʱ��һ������cmdid��Ҫ��䣬������û�У����Ե�һ���������� */
    ret = diag_srv_cnf_send(&report_info);
    if (ret) {
        return ERR_MSP_FAILURE;
    }

    return ERR_MSP_SUCCESS;
}

void diag_debug_dfr(void)
{
    diag_error("%d, %s, %d.\n", g_dfr_req.magic_num, g_dfr_req.name, g_dfr_req.dfr_len);
    diag_get_dfr(&g_dfr_req);
    diag_error("%d, %s, %d.\n", g_dfr_cnf.magic_num, g_dfr_cnf.name, g_dfr_cnf.dfr_len);
    diag_get_dfr(&g_dfr_cnf);
}

