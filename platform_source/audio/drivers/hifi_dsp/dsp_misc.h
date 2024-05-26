/*
 * dsp misc.c
 *
 * head of dsp misc.c
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DSP_MISC_H__
#define __DSP_MISC_H__

#include <linux/list.h>
#include <linux/time64.h>
#include <linux/version.h>

#include "audio_ddr_map.h"

/* mailbox mail_len max */
#define MAIL_LEN_MAX 512

#ifndef OK
#define OK    0
#endif

#define ERROR  (-1)
#define BUSY   (-2)
#define NOMEM  (-3)
#define COPYFAIL (-0xFFF)

#define NVPARAM_COUNT        600         /* HIFI NV size is 600 */
#define NVPARAM_NUMBER       258         /* 256+2, nv_data(256) + nv_id(2) */
#define NVPARAM_START        2           /* head protect_number 0x5a5a5a5a */
#define NVPARAM_TAIL         2           /* tail protect_number 0x5a5a5a5a */
#define NVPARAM_TOTAL_SIZE   ((NVPARAM_NUMBER * NVPARAM_COUNT + NVPARAM_START + NVPARAM_TAIL) * sizeof(unsigned short))

#define DRV_DSP_POWER_ON                0x55AA55AA
#define DRV_DSP_POWER_OFF               0x55FF55FF
#define DRV_DSP_KILLME_VALUE            0xA5A55A5A
#define DRV_DSP_NMI_COMPLETE            0xB5B5B5B5
#define DRV_DSP_NMI_INIT                0xA5A5A5A5
#define DRV_DSP_SOCP_FAMA_HEAD_MAGIC    0x5A5A5A5A
#define DRV_DSP_SOCP_FAMA_REAR_MAGIC    0xA5A5A5A5
#define DRV_DSP_FAMA_ON     0x1
#define DRV_DSP_FAMA_OFF    0x0

#define SIZE_CMD_ID 8

/* notice buffer for reporting data once */
#define REV_MSG_NOTICE_ID_MAX 2

#define ACPU_TO_HIFI_ASYNC_CMD 0xFFFFFFFF

#define BUFFER_NUM 8
#define MAX_NODE_COUNT 10

#define SYSCACHE_QUOTA_SIZE_MAX   0x100000
#define SYSCACHE_QUOTA_SIZE_ALIGN 0x40000
#define CMD_FUNC_NAME_LEN         50
#define RETRY_COUNT               3

enum dsp_platform_type {
	DSP_PLATFORM_ASIC,
	DSP_PLATFORM_FPGA
};

struct rev_msg_buff {
	unsigned char *mail_buff;
	unsigned int mail_buff_len;
	unsigned int cmd_id;         /* 4 byte */
	unsigned char *out_buff_ptr; /* point to behind of cmd_id */
	unsigned int out_buff_len;
};

struct recv_request {
	struct list_head recv_node;
	struct rev_msg_buff rev_msg;
};

struct common_dsp_cmd {
	unsigned short msg_id;
	unsigned short reserve;
	unsigned int   value;
};

struct drv_fama_config {
	unsigned int head_magic;
	unsigned int flag;
	unsigned int rear_magic;
};

enum usbaudio_ioctl_type {
	USBAUDIO_QUERY_INFO = 0,
	USBAUDIO_USB_POWER_RESUME,
	USBAUDIO_NV_ISREADY,
	USBAUDIO_MSG_MAX
};

struct usbaudio_ioctl_input {
	unsigned int msg_type;
	unsigned int input1;
	unsigned int input2;
};

#ifdef EXTERNAL_MODEM
struct hifi_ap_om_data_notify {
	unsigned int chunk_index;
	unsigned int len;
};
#endif

enum syscache_quota_type {
	SYSCACHE_QUOTA_RELEASE = 0,
	SYSCACHE_QUOTA_REQUEST
};

enum syscache_session {
	SYSCACHE_SESSION_AUDIO = 0,
	SYSCACHE_SESSION_VOICE,
	SYSCACHE_SESSION_CNT
};

typedef int (*cmd_proc_func)(uintptr_t arg);

struct dsp_ioctl_cmd {
	unsigned int id;
	cmd_proc_func func;
	char name[CMD_FUNC_NAME_LEN];
};

struct dsp_misc_proc {
	const struct dsp_ioctl_cmd *cmd_table;
	cmd_proc_func sync_msg_proc;
	unsigned int size;
};

struct record_start_dma_stamp_get_cnf {
	unsigned short msg_id;
	unsigned short reserved;
	unsigned int dsp_stamp;
	unsigned int kernel_stamp;
	time64_t kernel_time_s;
	long kernel_time_us;
};

long dsp_msg_process_cmd(unsigned int cmd, uintptr_t data32);
int dsp_send_msg(unsigned int mailcode, const void *data, unsigned int length);
#ifdef ENABLE_AUDIO_KCOV
void dsp_misc_msg_process(void *cmd);
#endif
void dsp_get_log_signal(void);
void dsp_release_log_signal(void);
void dsp_watchdog_send_event(void);
unsigned long try_copy_from_user(void *to, const void __user *from, unsigned long n);
unsigned long try_copy_to_user(void __user *to, const void *from, unsigned long n);
#ifdef EXTERNAL_MODEM
unsigned char *get_dsp_viradr(void);
#endif
enum dsp_platform_type dsp_misc_get_platform_type(void);
void dsp_reset_release_syscache(void);

#endif /* __DSP_MISC_H__ */

