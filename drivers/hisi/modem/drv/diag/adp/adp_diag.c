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
#include <scm_ind_src.h>
#include <scm_cnf_src.h>
#include <scm_debug.h>
#include <diag_system_debug.h>
#include <scm_init.h>
#include <scm_common.h>
#include <mdrv.h>
#include <scm_ind_src.h>
#include <scm_cnf_src.h>
#include <scm_ind_dst.h>
#include <scm_debug.h>
#include <diag_system_debug.h>
#include <OmCommonPpm.h>
#include <scm_init.h>
#include <scm_ind_dst.h>
#include <diag_port_manager.h>
#include <OmVcomPpm.h>
#include <scm_common.h>
#include <OmSocketPpm.h>
#include <OmUsbPpm.h>
#include <OmPortSwitch.h>
#include "../report/diag_report.h"
#include "../serivce/diag_service.h"
#include "OmCpPcdevPpm.h"

#if (!defined(DRV_BUILD_SEPARATE)) && defined(CONFIG_DIAG_SYSTEM)

void mdrv_diag_ptr(diag_ptr_id_e type, unsigned int para_mark, unsigned int para0, unsigned int para1)
{
    diag_ptr(type, para_mark, para0, para1);
}

void mdrv_ppm_reg_disconnect_cb(ppm_disconnect_port_cb cb)
{
    ppm_disconnect_cb_reg(cb);
}
unsigned int mdrv_get_thrput_info(diag_thrput_id_e type)
{
    return diag_get_throughput_info(type);
}

void mdrv_scm_reg_ind_coder_dst_send_fuc(void)
{
    scm_reg_ind_coder_dst_send_func();
}
unsigned int mdrv_ppm_log_port_switch(unsigned int phy_port, unsigned int effect)
{
    return ppm_port_switch(phy_port, (bool)effect);
}
unsigned int mdrv_diag_set_log_port(unsigned int phy_port, unsigned int effect)
{
    return ppm_port_switch(phy_port, (bool)effect);
}
unsigned int mdrv_ppm_query_log_port(unsigned int *log_port)
{
    return ppm_query_port(log_port);
}
unsigned int mdrv_diag_get_log_port(unsigned int *log_port)
{
    return ppm_query_port(log_port);
}
void mdrv_ppm_query_usb_info(void *usb_info, unsigned int len)
{
    ppm_query_usb_info(usb_info, len);
}
void mdrv_ppm_clear_usb_time_info(void)
{
    ppm_clear_usb_debug_info();
}
unsigned int mdrv_cpm_com_send(unsigned int logic_port, unsigned char *virt_addr,
                              unsigned char *phy_addr, unsigned int len)
{
    return cpm_com_send(logic_port, virt_addr, phy_addr, len);
}
void mdrv_cpm_logic_rcv_reg(unsigned int logic_port, cpm_rcv_cb rcv_func)
{
    ppm_socket_recv_cb_reg(logic_port, rcv_func);
}

void mdrv_scm_set_power_on_log(void)
{
    scm_set_power_on_log();
}

unsigned int mdrv_diag_shared_mem_write(unsigned int type, unsigned int len, const char *data)
{
    return diag_shared_mem_write(type, len, data);
}
unsigned int mdrv_diag_shared_mem_read(unsigned int type)
{
    return diag_shared_mem_read(type);
}
/*
 * �� �� ��: mdrv_diag_debug_file_header
 * ��������: ΪDIAGά������ļ�ͷ
 * �������: void
 */
unsigned int mdrv_diag_debug_file_header(const void *file)
{
    return diag_debug_file_header(file);
}
/*
 * �� �� ��: mdrv_diag_debug_file_tail
 * ��������: ΪDIAGά������ļ�β
 * �������: void
 */
void mdrv_diag_debug_file_tail(const void *file, char *file_path)
{
    return diag_debug_file_tail(file, file_path);
}

/*
 * �� �� ��: mdrv_diag_report_log
 * ��������:
 * �������:
 */
unsigned int mdrv_diag_report_log(unsigned int module_id, unsigned int pid, char *file_name,
                                  unsigned int line_num, const char *fmt, va_list arg)
{
    return (unsigned int)bsp_diag_report_log(module_id, pid, file_name, line_num, fmt, arg);
}

/*
 * �� �� ��: mdrv_diag_report_trans
 * ��������: �ṹ�������ϱ��ӿ�(�滻ԭ����DIAG_ReportCommand)
 * �������: mdrv_diag_trans_ind_s->module( 31-24:modem_id,23-16:modeid )
 *           mdrv_diag_trans_ind_s->msg_id(͸������ID)
 *           mdrv_diag_trans_ind_s->length(͸����Ϣ�ĳ���)
 *           mdrv_diag_trans_ind_s->data(͸����Ϣ)
 */
unsigned int mdrv_diag_report_trans(diag_trans_ind_s *trans_msg)
{
    return diag_report_trans(trans_msg);
}
/*
 * �� �� ��: mdrv_diag_event_report
 * ��������: �¼��ϱ��ӿ�
 * �������: mdrv_diag_event_ind_s->module( 31-24:modem_id,23-16:modeid,15-12:level,11-0:pid )
 *           mdrv_diag_event_ind_s->event_id(event ID)
 *           mdrv_diag_event_ind_s->length(event�ĳ���)
 *           mdrv_diag_event_ind_s->data(event��Ϣ)
 */
unsigned int mdrv_diag_report_event(diag_event_ind_s *event_msg)
{
    return diag_report_event(event_msg);
}
unsigned int mdrv_diag_report_air(diag_air_ind_s *air_msg)
{
    return diag_report_air(air_msg);
}

/*
 * �� �� ��: mdrv_diag_report_trace
 * ��������: �����Ϣ�ϱ��ӿ�
 * �������: msg_data(��׼��VOS��Ϣ�壬Դģ�顢Ŀ��ģ����Ϣ����Ϣ���л�ȡ)
 */
unsigned int mdrv_diag_report_trace(void *trace_msg, unsigned int modem_id)
{
    return diag_report_trace(trace_msg, modem_id);
}

/*
 * �� �� ��: mdrv_diag_reset
 * ��������: ��λdiag�������
 * �������: void
 */
void mdrv_diag_report_reset(void)
{
    return diag_report_reset();
}
/*
 * �� �� ��: mdrv_diag_reset_mntn_info
 * ��������: ��λdiagά��ͳ����Ϣ
 * �������: void
 */
void mdrv_diag_reset_mntn_info(diag_mntn_e  type)
{
    if (DIAGLOG_SRC_MNTN == type) {
        return diag_reset_src_mntn_info();
    } else if (DIAGLOG_DST_MNTN == type) {
        return diag_reset_dst_mntn_info();
    }
}
/*
 * �� �� ��: mdrv_diag_get_mntn_info
 * ��������: ��ȡά��ͳ����Ϣ
 * �������: void
 */
void* mdrv_diag_get_mntn_info(diag_mntn_e  type)
{
    if (DIAGLOG_SRC_MNTN == type) {
        return (void *)diag_get_src_mntn_info();
    } else {
        return diag_get_dst_mntn_info();
    }
}
/*
 * �� �� ��: mdrv_diag_report_msg_trans
 * ��������: ��ȡά��ͳ����Ϣ
 * �������: void
 */
unsigned int mdrv_diag_report_msg_trans(diag_trans_ind_s *trans_msg, unsigned int cmd_id)
{
    return diag_report_msg_trans(trans_msg, cmd_id);
}
/*
 * �� �� ��: mdrv_diag_report_msg_trans
 * ��������: ��ȡά��ͳ����Ϣ
 * �������: void
 */
unsigned int mdrv_diag_report_cnf(diag_cnf_info_s *diag_info, void *cnf_msg, unsigned int len)
{
    return diag_report_cnf(diag_info, cnf_msg, len);
}
/*
 * �� �� ��: diag_report_reset_msg
 * ��������:
 * �������: �ϱ�������λ��Ϣ(CNFͨ��TRNANS��Ϣ)
 */
unsigned int mdrv_diag_report_reset_msg(diag_trans_ind_s *trans_msg)
{
    return diag_report_reset_msg(trans_msg);
}

void mdrv_diag_service_proc_reg(diag_srv_proc_cb srv_fn)
{
    return diag_srv_proc_func_reg(srv_fn);
}

extern u32 scm_reg_decode_dst_proc(SOCP_DECODER_DST_ENUM_U32 enChanlID, scm_decode_dst_cb func);
unsigned int mdrv_scm_reg_decode_dst_proc(SOCP_DECODER_DST_ENUM_U32 chan_id, scm_decode_dst_cb func)
{
    return scm_reg_decode_dst_proc(chan_id, func);
}
void mdrv_ppm_pcdev_ready(void)
{
}

unsigned int mdrv_ppm_pedev_diag_statue(unsigned int *status)
{
    return 0;
}

void mdrv_ppm_notify_conn_state(unsigned int state)
{
    return;
}
unsigned int mdrv_diag_get_usb_type(void)
{
    return diag_get_usb_type();
}

#else
void mdrv_diag_ptr(diag_ptr_id_e type, unsigned int para_mark, unsigned int para0, unsigned int para1)
{
    return;
}
void mdrv_ppm_reg_disconnect_cb(ppm_disconnect_port_cb cb)
{
    return;
}
unsigned int mdrv_get_thrput_info(diag_thrput_id_e type)
{
    return 0;
}

void mdrv_scm_reg_ind_coder_dst_send_fuc(void)
{
    return;
}
unsigned int mdrv_ppm_log_port_switch(unsigned int phy_port, unsigned int effect)
{
    return 0;
}
unsigned int mdrv_ppm_query_log_port(unsigned int *log_port)
{
    return 0;
}
unsigned int mdrv_cpm_com_send(unsigned int logic_port, unsigned char *virt_addr,
                              unsigned char *phy_addr, unsigned int len)
{
    return 0;
}
void mdrv_cpm_logic_rcv_reg(unsigned int logic_port, cpm_rcv_cb rcv_func)
{
    return;
}
unsigned int mdrv_scm_reg_decode_dst_proc(SOCP_DECODER_DST_ENUM_U32 enChanlID, scm_decode_dst_cb func)
{
    return 0;
}
void mdrv_diag_reset_dst_mntn_info(void)
{
    return;
}

unsigned int mdrv_diag_shared_mem_write(unsigned int type, unsigned int len, const char *data)
{
    return 0;
}

unsigned int mdrv_diag_shared_mem_read(unsigned int type)
{
    return 0;
}
unsigned int mdrv_diag_debug_file_header(const void *file)
{
    return 0;
}
void mdrv_diag_debug_file_tail(const void *file, char *file_path)
{
    return;
}

/*
 * �� �� ��: mdrv_diag_report_log
 * ��������:
 * �������:
 */
unsigned int mdrv_diag_report_log(unsigned int module_id, unsigned int pid, char *file_name,
                                  unsigned int line_num, const char *fmt, va_list arg)
{
    return 0;
}

/*
 * �� �� ��: mdrv_diag_report_trans
 * ��������: �ṹ�������ϱ��ӿ�(�滻ԭ����DIAG_ReportCommand)
 * �������: diag_trans_ind_s->module( 31-24:modem_id,23-16:modeid )
 *           diag_trans_ind_s->msg_id(͸������ID)
 *           diag_trans_ind_s->length(͸����Ϣ�ĳ���)
 *           diag_trans_ind_s->data(͸����Ϣ)
 */
unsigned int mdrv_diag_report_trans(diag_trans_ind_s *trans_msg)
{
    return 0;
}
/*
 * �� �� ��: mdrv_diag_event_report
 * ��������: �¼��ϱ��ӿ�
 * �������: diag_event_ind_s->module( 31-24:modem_id,23-16:modeid,15-12:level,11-0:pid )
 *           diag_event_ind_s->event_id(event ID)
 *           diag_event_ind_s->length(event�ĳ���)
 *           diag_event_ind_s->data(event��Ϣ)
 */
unsigned int mdrv_diag_report_event(diag_event_ind_s *event_msg)
{
    return 0;
}
unsigned int mdrv_diag_report_air(diag_air_ind_s *air_msg)
{
    return 0;
}
/*
 * �� �� ��: mdrv_diag_trace_report
 * ��������: �����Ϣ�ϱ��ӿ�
 * �������: msg_data(��׼��VOS��Ϣ�壬Դģ�顢Ŀ��ģ����Ϣ����Ϣ���л�ȡ)
 */
unsigned int mdrv_diag_report_trace(void *trace_msg, unsigned int modem_id)
{
    return 0;
}
/*
 * �� �� ��: mdrv_diag_reset
 * ��������: ��λdiag�������
 * �������: void
 */
void mdrv_diag_report_reset(void)
{
    return;
}
/*
 * �� �� ��: mdrv_diag_reset_mntn_info
 * ��������: ��λdiagά��ͳ����Ϣ
 * �������: mdrv_diag_mntn_e  type
 */
void mdrv_diag_reset_mntn_info(diag_mntn_e  type)
{
    return;
}
/*
 * �� �� ��: mdrv_diag_get_mntn_info
 * ��������: ��ȡά��ͳ����Ϣ
 * �������: mdrv_diag_mntn_e  type
 */
void* mdrv_diag_get_mntn_info(diag_mntn_e  type)
{
    return NULL;
}
/*
 * �� �� ��: mdrv_diag_report_msg_trans
 * ��������: ��ȡά��ͳ����Ϣ
 * �������: void
 */
unsigned int mdrv_diag_report_msg_trans(diag_trans_ind_s *trans_id, unsigned int cmd_id)
{
    return 0;
}
/*
 * �� �� ��: mdrv_diag_report_cnf
 * ��������: ��ȡά��ͳ����Ϣ
 * �������: void
 */
unsigned int mdrv_diag_report_cnf(diag_cnf_info_s *diag_info, void *cnf_msg, unsigned int len)
{
    return 0;
}

void mdrv_diag_service_proc_reg(diag_srv_proc_cb srv_fn)
{
    return;
}

void mdrv_ppm_query_usb_info(void *usb_info, unsigned int len)
{
    return;
}
void mdrv_ppm_clear_usb_time_info(void)
{
    return;
}

void mdrv_ppm_pcdev_ready(void)
{
    return;
}
unsigned int mdrv_ppm_pedev_diag_statue(unsigned int *status)
{
    return 0;
}
void mdrv_ppm_notify_conn_state(unsigned int state)
{
    return;
}
unsigned int mdrv_diag_get_usb_type(void)
{
    return 0;
}

#endif
