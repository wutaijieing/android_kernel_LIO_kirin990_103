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

#ifndef __OM_CP_PCDEV_PPM_H__
#define __OM_CP_PCDEV_PPM_H__

#include <mdrv.h>
#include <mdrv_pcdev.h>
#include <mdrv_diag_system.h>
#include <osl_types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#pragma pack(push)
#pragma pack(4)

/*
 * 2 宏定义
 */
#define PPM_PCDEV_SOCP_CNF_BUFFER_SIZE (8 * 1024)
#define PPM_PCDEV_SOCP_CNF_BUFFER_NUM (8)

#define PPM_PCDEV_SOCP_IND_BUFFER_SIZE (2 * 1024)
#define PPM_PCDEV_SOCP_IND_BUFFER_NUM (2)

#define PPM_PCDEV_DUMP_PACKET_NUM (100)

/*
 * 3 枚举定义
 */
typedef enum {
    PPM_PCDEV_TYPE_IND,
    PPM_PCDEV_TYPE_CFG,
    PPM_PCDEV_TYPE_BUTT,
} ppm_pcdev_type_e;

/*
 * 4 结构体定义
 */
typedef struct {
    u32 write_num1;
    u32 write_num2;
    u32 write_max_time;
    u32 write_err_num;
    u32 write_err_len;
    u32 write_err_ret;
    u32 write_err_slice;
    u32 write_invalid_chan_num;
    u32 send_data_len;

    u32 write_cb_num;

    u32 open_num;
    u32 open_slice;
    u32 open_ok_num;
    u32 open_ok_slice;
    u32 open_ok_end_num;
    u32 open_ok_end_slice;

    u32 out_num;
    u32 out_slice;

    u32 pcdev_in_num;
    u32 pcdev_in_slice;

    u32 state_err_num;
    u32 state_err_slice;

    u32 read_cb_num; /* 收到AP发送数据的次数 */

    u32 handle_err_num;
    u32 get_buff_err_num;

    u32 pcdev_rcv_pkt_num;
    u32 pcdev_rcv_pkt_len;
    u32 pcdev_rcv_err_num;
    u32 read_buff_free_err_num;
} ppm_pcdev_mntn_s;

typedef struct {
    u32 slice;
    u32 pkt_len;
    u32 total_pkt;
    u32 total_len;
} ppm_pcdev_dump_s;

typedef struct PPM_PCDEV_CTRL_S {
    u32 ready_state;
    ppm_pcdev_mntn_s pcdev_mntn[PPM_PCDEV_TYPE_BUTT];
    int port_udi_handle[PPM_PCDEV_TYPE_BUTT];
    ppm_pcdev_dump_s *dump_area;
} ppm_pcdev_ctrl_s;

typedef struct {
    u32 msg_id;   /* 消息ID */
    u32 length;     /* 消息长度 */
    u32 phy_port; /* 结果码 */
} ap_cp_port_msg_s;

/*
 * 4 函数声明
 */
u32  ppm_pcdev_cfg_send_data(u8 *virt_addr, u8 *phy_addr, u32 len);
void ppm_pcdev_cfg_status_cb(void *context);
void ppm_pcdev_cfg_write_data_cb(u8 *virt_addr, u8 *phy_addr, s32 len);
s32  ppm_pcdev_cfg_read_data_cb(void);
void ppm_pcdev_cfg_port_open(void);
void ppm_pcdev_ind_status_cb(void *context);
void ppm_pcdev_ind_write_data_cb(u8 *virt_addr, u8 *phy_addr, s32 len);
void ppm_pcdev_ind_port_open(void);
u32  ppm_pcdev_ind_send_data(u8 *virt_addr, u8 *phy_addr, u32 len);
u32  ppm_pcdev_port_init(void);
u32  ppm_pcdev_port_send(ppm_pcdev_type_e handle, u8 *virt_addr, u8 *phy_addr, u32 len);
u32  ppm_pcdev_debug_info_show(ppm_pcdev_type_e type);
void ppm_pcdev_ready(void);
void ppm_pcdev_event_reg(void);
u32  ppm_pcdev_send_icc_msg(u32 msg_id, u32 len, void *context);
void ppm_pcdev_conn_state_notify(unsigned int state);
u32  ppm_pcdev_diag_status(u32 *status);
u32  ppm_pcdev_port_sync(void *icc_msg);
void ppm_pcdev_dump_record(u32 length);

/*
 * 5 全局变量声明
 */

/*
 * 6 OTHERS定义
 */

#pragma pack(pop)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of OmCommonPpm.h */
