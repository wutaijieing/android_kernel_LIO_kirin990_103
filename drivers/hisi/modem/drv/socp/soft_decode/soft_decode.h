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

#ifndef _SCM_SOFTDECODE_H_
#define _SCM_SOFTDECODE_H_

#include <osl_types.h>
#include <osl_spinlock.h>
#include <osl_sem.h>
#include <bsp_print.h>
#include "hdlc.h"
#include "ring_buffer.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*
 * 2 宏定义
 */
#define SCM_DATA_RCV_BUFFER_SIZE (36 * 1024) /* SCM数据接收buffer大小 */
#define SCM_DATA_RCV_PKT_SIZE (36 * 1024)      /* SCM数据接收PKT大小 */

#define soft_decode_crit(fmt, ...) printk(KERN_ERR "[%s]:" fmt, BSP_MOD(THIS_MODU), ##__VA_ARGS__)
#define soft_decode_error(fmt, ...) \
    printk(KERN_ERR "[%s]:<%s %d>" fmt, BSP_MOD(THIS_MODU), __FUNCTION__, __LINE__, ##__VA_ARGS__)

/*
 * 3 枚举定义
 */

/*
 * 4 结构体定义
 */

/*
 * 结构名: SCM_DATE_RCV_CTRL_STRU
 * 结构说明: SCM数据接收的控制结构
 */
typedef struct {
    osl_sem_id sem_id;
    om_ring_id rng_om_rbuff_id;
    char *p_buffer;
} scm_data_rcv_ctrl_s;

/*
 * 结构名: scm_mantennace_info_s
 * 结构说明: SCM软解码模块可维可测信息记录结构
 */
typedef struct {
    u32 data_len; /* 接收或发送数据长度 */
    u32 num;     /* 接收或发送数据次数 */
} scm_mantennace_info_s;

typedef struct {
    u32 sem_create_err;
    u32 sem_give_err;
    u32 ringbuff_create_err;
    u32 task_id_err;
    u32 buff_no_enough;
    u32 rbuff_flush;
    u32 rbuff_put_err;
} scm_soft_decode_rb_info_s;

/*
 * 结构名: OM_ACPU_PC_UE_SUC_STRU
 * 结构说明: OM模块正常接收可维可测信息记录结构
 */
typedef struct {
    scm_soft_decode_rb_info_s rb_info;
    scm_mantennace_info_s put_info;
    scm_mantennace_info_s get_info;
    scm_mantennace_info_s hdlc_decap_data;
    u32 frame_decap_data;
    u32 hdlc_init_err;
    u32 data_type_err;
    u32 cpm_reg_logic_rcv_suc;
} scm_soft_decode_info_s;

extern scm_soft_decode_info_s g_scm_soft_decode_info;
/*
 * 6 函数定义
 */

u32 SCM_SoftDecodeCfgDataRcv(u8 *p_buffer, u32 len);
u32 SCM_SoftDecodeDataRcv(u8 *p_buffer, u32 len);
int SCM_SoftDecodeCfgRcvTaskInit(void);
int SCM_SoftDecodeCfgRcvSelfTask(void *para);
void scm_soft_decode_init(void);
u32 SCM_SoftDecodeAcpuRcvData(om_hdlc_s *p_hdlc_ctrl, u8 *p_data, u32 len);
u32 SCM_SoftDecodeCfgHdlcInit(om_hdlc_s *p_hdlc);
void scm_soft_decode_info_show(void);
u32 diag_fill_soft_header(u32 p_file);
void diag_fill_soft_tail(u32 p_file, s8 *file_path);
void diag_save_soft_decode_info(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
