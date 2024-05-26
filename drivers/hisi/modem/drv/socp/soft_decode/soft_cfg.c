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
#include <mdrv.h>
#include <bsp_print.h>
#include "soft_decode.h"
#include "hdlc.h"
#include "ring_buffer.h"
#include "soft_cfg.h"


#define THIS_MODU mod_soft_dec

/* 解码目的通道回调函数 */
scm_decode_dst_cb g_scm_decode_cb_func[SOCP_DECODER_DST_CB_BUTT] = {NULL, NULL, NULL};



u32 scm_reg_decode_dst_proc(SOCP_DECODER_DST_ENUM_U32 chan_id, scm_decode_dst_cb func)
{
    u32 ulOffset;

    if (chan_id >= SOCP_DECODER_DST_BUTT) {
        return ERR_MSP_FAILURE;
    }

    if (NULL == func) {
        return ERR_MSP_FAILURE;
    }

    if (SOCP_DECODER_DST_LOM == chan_id) {
        ulOffset = SOCP_DECODER_DST_CB_TL_OM;
    } else if (SOCP_DECODER_DST_GUOM == chan_id) {
        ulOffset = SOCP_DECODER_DST_CB_GU_OM;
    } else if (SOCP_DECODER_CBT == chan_id) {
        ulOffset = SOCP_DECODER_DST_CB_GU_CBT;
    } else {
        return ERR_MSP_FAILURE;
    }

    g_scm_decode_cb_func[ulOffset] = func;

    return BSP_OK;
}


void scm_rcv_data_dispatch(om_hdlc_s *p_hdlc_ctrl, u8 data_type)
{
    /* TL数据 */
    if (SCM_DATA_TYPE_TL == data_type) {
        if (NULL != g_scm_decode_cb_func[SOCP_DECODER_DST_CB_TL_OM]) {

            /* TL不需要DATATYPE字段，回调时删除 */
            g_scm_decode_cb_func[SOCP_DECODER_DST_CB_TL_OM](SOCP_DECODER_DST_LOM,
                                                             p_hdlc_ctrl->p_decap_buff +
                                                                 sizeof(SOCP_DATA_TYPE_ENUM_UIN8),
                                                             p_hdlc_ctrl->info_len - sizeof(SOCP_DATA_TYPE_ENUM_UIN8),
                                                             0, 0);
        }

        return;
    }

    return;
}

