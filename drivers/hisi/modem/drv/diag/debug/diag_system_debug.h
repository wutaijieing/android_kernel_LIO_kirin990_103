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

#ifndef __DIAG_SYSTEM_DEBUG_H__
#define __DIAG_SYSTEM_DEBUG_H__

/*
 * 1 Include Headfile
 */
#include <linux/version.h>
#include <product_config.h>
#include <mdrv.h>
#include <mdrv_diag_system.h>
#include <osl_types.h>
#include <osl_sem.h>
#include <bsp_slice.h>
#include <bsp_diag.h>
#include <bsp_print.h>
#include <bsp_socp.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#pragma pack(push)
#pragma pack(4)
#endif

#undef THIS_MODU
#define THIS_MODU mod_diag

#define DIAG_DEBUG_START (0xaaaa5555)
#define DIAG_DEBUG_END (0x5555aaaa)

/* debug�汾 V1.00 */
#define DIAG_DEBUG_VERSION (0x00010000)

/* debug��ʾ��Ϣ�ĳ��� */
#define DIAG_DEBUG_INFO_LEN (32)

/* �������ݲɼ���ʱ���� */
#define DIAG_DEBUG_DEALAY (5000)

/* ���ӵ����ݽṹ���洢��Ϣǰ�ȴ洢��Ϣ���ȣ���8λ��0xa5����24λ�����ݳ��� */
#define DIAG_DEBUG_SIZE_FLAG (0xa5000000)

#define diag_crit(fmt, ...) printk(KERN_ERR "[%s]:" fmt, BSP_MOD(THIS_MODU), ##__VA_ARGS__)
#define diag_error(fmt, ...) \
    printk(KERN_ERR "[%s]:<%s %d>" fmt, BSP_MOD(THIS_MODU), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define diag_debug(fmt, ...)

/* OverFlow (Data Flow Record �������ͳ��)****************************************** */
#define DIAG_SRC_50_OVERFLOW_THR (SCM_CODER_SRC_IND_BUFFER_SIZE >> 1) /* 50%������ֵ */
#define DIAG_SRC_80_OVERFLOW_THR (SCM_CODER_SRC_IND_BUFFER_SIZE >> 3) /* 87.5%������ֵ */

/* DFR (Data Flow Record������¼��)****************************************** */

#define DIAG_DFR_NAME_MAX (8)
#define DIAG_DFR_BUFFER_MAX (8 * 1024)
#define DIAG_DFR_MAGIC_NUM (0x44494147) /* DIAG */

#define DIAG_DFR_START_NUM (0xabcd4321)

#define DFR_ALIGN_WITH_4BYTE(len) (((len) + 3) & (~3))

typedef struct {
    u32 magic_num;               /* ħ���� */
    osl_sem_id sem_id;             /* �洢�ռ���ʵĻ����ź��� */
    char name[DIAG_DFR_NAME_MAX]; /* �洢���������� */
    u32 cur_info;                    /* �洢�ռ�ĵ�ǰ��ַ */
    u32 dfr_len;                    /* �洢�ռ���ܳ��� */
    u8 *data;                    /* �洢�ռ���׵�ַ */
}diag_dfr_info_s;

typedef struct {
    u32 start;
    u32 timestamp;
}diag_dfr_header_s;

/* PTR ********************************************************************** */
/*
 * ��������ÿ�����������10-20�����ܸ��������������̣�
 * 1000�����ܻ���50-100���������Ĵ�������
 */
#define DIAG_PTR_NUMBER (2000)

typedef struct {
    u16 step;                            /* �¼�ö�� */
    u16 para_mark;                          /* �Ƿ�Я������, 1Я������ 0��Я������ */
    u32 timestamp;                            /* ��ǰʱ�� */
    u32 para[2];                           /* Я���Ĳ�������֧��0������2������֧������ */ /*lint !e43*/
} diag_ptr_element_s;

typedef struct {
    u32 cur_info;
    diag_ptr_element_s ptr_element[DIAG_PTR_NUMBER];
} diag_ptr_info_s;

/* ������ͳ�� ********************************************************************** */
#define DIAG_THRPUT_DEBUG_NUM (100) /* ����100���� */

#define DIAG_THRPUT_DEBUG_LEN (5 * 32768) /* ÿ��ͳ�����������ټ��5������ */

typedef struct {
    unsigned int timestamp;      /* ��ǰʱ���ʱ��� */
    unsigned int throughput; /* ������(Bytes/s) */
}diag_thrput_node_s;

typedef struct {
    unsigned int timestamp; /* ���һ��ͳ�Ƶ�ʱ��� */
    unsigned int total_len; /* ��ǰ���ۼƵ��ֽ��� */
    unsigned int max_thrput;   /* �����ʷ�ֵ */
    unsigned int cur_ptr;   /* ��ǰָ�� */
    diag_thrput_node_s node[DIAG_THRPUT_DEBUG_NUM];
}diag_thrput_info_s;

extern diag_mntn_dst_info_s g_ind_dst_mntn_info;
extern u32 g_diag_throughput[EN_DIAG_THRPUT_MAX];
extern u32 g_send_usb_start_slice;
extern u32 g_send_pedev_start_slice;
/* *****************LNR******************************* */
#define DIAG_DRV_LNR_NUMBER (100)

typedef enum {
    EN_DIAG_DRV_LNR_CMDID = 0, /* ���N�����յ���������� */
    EN_DIAG_DRV_LNR_LOG_LOST, /* ���N��log��ʧ�ļ�¼ */
    EN_DIAG_DRV_LNR_PS_TRANS, /* ���N��PS͸������ļ�¼ */
    EN_DIAG_DRV_LNR_CCORE_MSG, /* ���N��C�˴�A���յ�����ϢID�ļ�¼ */
    EN_DIAG_DRV_LNR_MESSAGE_RPT, /* ���N��ͨ��messageģ��report�ϱ���cmdid�ļ�¼ */
    EN_DIAG_DRV_LNR_TRANS_IND, /* ���N��͸���ϱ��ļ�¼ */
    EN_DIAG_DRV_LNR_INFO_MAX
}diag_lnr_id_e;

typedef enum {
    NOT_SUPPORT_QUERY = 0,         /* DIAG��֧��USB���� */
    TYPE_USB_SPEED_HIGH = 1,       /* USB 2.0 */
    TYPE_USB_SPEED_SUPER = 2,      /* USB 3.0 */
    TYPE_USB_SPEED_SUPER_PLUS = 3, /* USB 3.1 */
}diag_usb_type_e;

typedef struct {
    unsigned int cur_info;
    unsigned int param1[DIAG_DRV_LNR_NUMBER];
    unsigned int param2[DIAG_DRV_LNR_NUMBER];
}diag_lnr_info_tbl_s;

#ifndef DIAG_SYSTEM_A_PLUS_B_AP
static inline void diag_system_debug_ind_dst_lost(u32 type, u32 len)
{
    g_ind_dst_mntn_info.delta_lost_times++;
    g_ind_dst_mntn_info.delta_lost_len += len;
    g_ind_dst_mntn_info.cur_fail_num[type]++;
    g_ind_dst_mntn_info.cur_fail_len[type] += len;
}

static inline void diag_system_debug_rev_socp_data(u32 len)
{
    g_ind_dst_mntn_info.delta_socp_len += len;
}

static inline void diag_system_debug_usb_len(u32 len)
{
    g_ind_dst_mntn_info.delta_usb_len += len;
}
static inline void diag_system_debug_usb_ok_len(u32 len)
{
    g_ind_dst_mntn_info.delta_usb_ok_len += len;
}

static inline void diag_system_debug_usb_free_len(u32 len)
{
    g_ind_dst_mntn_info.delta_usb_free_len += len;
}
static inline void diag_system_debug_usb_fail_len(u32 len)
{
    g_ind_dst_mntn_info.delta_usb_fail_len += len;
}

static inline void diag_system_debug_pcdev_len(u32 len)
{
    g_ind_dst_mntn_info.delat_pcdev_len += len;
}
static inline void diag_system_debug_pcdev_ok_len(u32 len)
{
    g_ind_dst_mntn_info.delat_pcdev_ok_len += len;
}

static inline void diag_system_debug_pcdev_free_len(u32 len)
{
    g_ind_dst_mntn_info.delat_pcdev_free_len += len;
}
static inline void diag_system_debug_pcdev_fail_len(u32 len)
{
    g_ind_dst_mntn_info.delat_pcdev_fail_len += len;
}

static inline void diag_system_debug_vcom_len(u32 len)
{
    g_ind_dst_mntn_info.delat_vcom_len += len;
}
static inline void diag_system_debug_vcom_fail_len(u32 len)
{
    g_ind_dst_mntn_info.delat_vcom_fail_len += len;
}
static inline void diag_system_debug_vcom_sucess_len(u32 len)
{
    g_ind_dst_mntn_info.delat_vcom_succ_len += len;
}
static inline void diag_system_debug_socket_len(u32 len)
{
    g_ind_dst_mntn_info.delat_socket_len += len;
}
static inline void diag_system_debug_socket_sucess_len(u32 len)
{
    g_ind_dst_mntn_info.delat_socket_succ_len += len;
}
static inline void diag_system_debug_socket_fail_len(u32 len)
{
    g_ind_dst_mntn_info.delat_socket_fail_len += len;
}

static inline void diag_system_debug_send_data_end(void)
{
    u32 curent_slice;
    u32 start_slice;
    u32 delta;

    curent_slice = bsp_get_slice_value();
    start_slice = bsp_get_socp_ind_dst_int_slice();
    delta = get_timer_slice_delta(start_slice, curent_slice);

    g_ind_dst_mntn_info.delat_socp_int_to_port_time += delta;
}
static inline void diag_system_debug_send_usb_end(void)
{
    u32 curent_slice;
    u32 start_slice;
    u32 delta;

    curent_slice = bsp_get_slice_value();
    start_slice = g_send_usb_start_slice;

    delta = get_timer_slice_delta(start_slice, curent_slice);
    g_ind_dst_mntn_info.delta_usb_send_time += delta;
}
static inline void diag_system_debug_send_usb_start(void)
{
    g_send_usb_start_slice = bsp_get_slice_value();
}
static inline void diag_system_debug_send_pcdev_end(void)
{
    u32 curent_slice;
    u32 start_slice;
    u32 delta;

    curent_slice = bsp_get_slice_value();
    start_slice = g_send_pedev_start_slice;

    delta = get_timer_slice_delta(start_slice, curent_slice);
    g_ind_dst_mntn_info.delta_pcdev_send_time += delta;
}
static inline void diag_system_debug_send_pcdev_start(void)
{
    g_send_pedev_start_slice = bsp_get_slice_value();
}

static inline void diag_throughput_save(diag_thrput_id_e chan, u32 bytes)
{
    g_diag_throughput[chan] += bytes;
}

#else
static inline void diag_system_debug_ind_dst_lost(u32 type, u32 len)
{
    return;
}

static inline void diag_system_debug_rev_socp_data(u32 len)
{
    return;
}

static inline void diag_system_debug_usb_len(u32 len)
{
    return;
}
static inline void diag_system_debug_usb_ok_len(u32 len)
{
    return;
}

static inline void diag_system_debug_usb_free_len(u32 len)
{
    return;
}
static inline void diag_system_debug_usb_fail_len(u32 len)
{
    return;
}

static inline void diag_system_debug_vcom_len(u32 len)
{
    return;
}
static inline void diag_system_debug_vcom_fail_len(u32 len)
{
    return;
}
static inline void diag_system_debug_vcom_sucess_len(u32 len)
{
    return;
}
static inline void diag_system_debug_socket_len(u32 len)
{
    return;
}
static inline void diag_system_debug_socket_sucess_len(u32 len)
{
    return;
}
static inline void diag_system_debug_socket_fail_len(u32 len)
{
    return;
}

static inline void diag_system_debug_send_data_end(void)
{
    return;
}
static inline void diag_system_debug_send_usb_end(void)
{
    return;
}
static inline void diag_system_debug_send_usb_start(void)
{
    return;
}
static inline void diag_throughput_save(diag_thrput_id_e chan, u32 bytes)
{
    return;
}
#endif
/* ��������****************************************** */
void diag_debug_ptr(void);
void diag_debug_show_ptr(u32 num);
void diag_ptr(diag_ptr_id_e type, u32 para_mark, u32 para0, u32 para1);
void diag_throughput_save(diag_thrput_id_e chan, u32 bytes);
u32  diag_get_throughput_info(diag_thrput_id_e type);
s32  diag_system_debug_event_cb(unsigned int chan_id, SOCP_EVENT_ENUM_UIN32 event, unsigned int param);
diag_mntn_dst_info_s * diag_get_dst_mntn_info(void);
void diag_reset_dst_mntn_info(void);
u32  diag_debug_log_level(u32 level);
u32  diag_ctreate_dfr(const char *name, u32 len, diag_dfr_info_s *dfr);
void diag_save_ptr(diag_dfr_info_s *dfr, u8 *data, u32 len);
void diag_get_dfr(diag_dfr_info_s *dfr);
void diag_lnr(diag_lnr_id_e type, u32 param1, u32 param2);
void diag_show_lnr(diag_lnr_id_e type, u32 n);
u32  diag_debug_file_header(const void *file);
void diag_debug_file_tail(const void *file, char *path);
void diag_reset_src_mntn_info(void);
diag_debug_info_s *diag_get_src_mntn_info(void);
void diag_buffer_overflow_record(u32 pack_len);
void diag_debug_ind_src_lost(u32 len);
void diag_package_record(u32 pack_len);
u32  diag_get_usb_type(void);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#pragma pack(pop)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of diag_Debug.h */
