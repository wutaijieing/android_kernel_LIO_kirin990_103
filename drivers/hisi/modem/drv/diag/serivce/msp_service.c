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

#include "msp_service.h"
#include <product_config.h>
#include <mdrv.h>
#include <securec.h>
#include <osl_malloc.h>
#include <bsp_diag.h>
#include "diag_service.h"
#include "diag_system_debug.h"


#define THIS_MODU mod_diag

msp_service_table_s g_msp_service[DIAG_FRAME_MSP_SID_BUTT] = {
    {DIAG_FRAME_MSP_SID_DEFAULT,           NULL},
    {DIAG_FRAME_MSP_SID_AT_SERVICE,        NULL},
    {DIAG_FRAME_MSP_SID_DIAG_SERVICE,      NULL},
    {DIAG_FRAME_MSP_SID_DATA_SERVICE,      NULL},
    {DIAG_FRAME_MSP_SID_NV_SERVICE,        NULL},
    {DIAG_FRAME_MSP_SID_USIM_SERVICE,      NULL},
    {DIAG_FRAME_MSP_SID_DM_SERVICE,        NULL},
    {DIAG_FRAME_MSP_SID_CBT_SERVICE,       NULL}
};

void msp_srv_proc(SOCP_DECODER_DST_ENUM_U32 chan, u8 *buff, u32 buff_size, u8 *rb_buff, u32 rb_size)
{
    u32 total_len = 0;
    u8 *data = NULL;
    u32 sid = 0;
    diag_service_head_s *srv_header = NULL;

    if (buff == NULL) {
        diag_ptr(EN_DIAG_PTR_MSP_SERVICE_ERR1, 0, 0, 0);
        return;
    }

    diag_ptr(EN_DIAG_PTR_MSP_SERVICE_1, 1, 0, 0);

    /* ��μ�� */
    total_len = buff_size + rb_size;
    if (!total_len) {
        diag_ptr(EN_DIAG_PTR_MSP_SERVICE_ERR2, 1, 0, 0);
        return;
    }

    data = osl_malloc(total_len);
    if (data == NULL) {
        diag_ptr(EN_DIAG_PTR_MSP_SERVICE_ERR3, 1, 0, 0);
        return;
    }

    memcpy_s(data, total_len, buff, buff_size);

    /* �ؾ�ָ�����Ϊ�� */
    if ((NULL != rb_buff) && (0 != rb_size)) {
        memcpy_s(data + buff_size, total_len - buff_size, rb_buff, rb_size);
    }

    diag_ptr(EN_DIAG_PTR_MSP_SERVICE_2, 1, 0, 0);

    /* ��Ϣ���ݴ�С����Ҫ����serviceͷ���� */
    if (total_len < sizeof(diag_service_head_s)) {
        osl_free(data);
        diag_error("msg len is smaller than service header, msg_len:0x%x\n", total_len);
        return;
    }

    sid = SERVICE_HEAD_SID(data);
    srv_header = (diag_service_head_s *)data;

    if (sid < DIAG_FRAME_MSP_SID_BUTT) {
        if (g_msp_service[sid].fnService) {
            g_msp_service[sid].fnService((void *)srv_header, total_len);
        } else {
            diag_error("sid:0x%x is error\n", sid);
        }
    } else {
        diag_error("sid:0x%x is too large\n", sid);
    }

    osl_free(data);

    return;
}

extern u32 scm_reg_decode_dst_proc(SOCP_DECODER_DST_ENUM_U32 chan_id, scm_decode_dst_cb func);
void msp_srv_init(void)
{
    u32 ret = ERR_MSP_SUCCESS;
    /* register to SCM */
    ret = scm_reg_decode_dst_proc(SOCP_DECODER_DST_LOM, (scm_decode_dst_cb)msp_srv_proc);
    if(ret != ERR_MSP_SUCCESS) {
    }
    return;
}

void msp_srv_proc_func_reg(diag_frame_sid_type_e type, msp_service_func srv_fn)
{
    if (type >= DIAG_FRAME_MSP_SID_BUTT) {
        return;
    }

    g_msp_service[type].fnService = srv_fn;
}

