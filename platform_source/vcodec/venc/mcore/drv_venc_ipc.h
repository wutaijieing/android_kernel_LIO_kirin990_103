/*
 * drv_venc_ipc.h
 *
 * This is for Operations Related to ipc.
 *
 * Copyright (c) 2021-2021 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _DRV_VENC_IPC_H_
#define _DRV_VENC_IPC_H_

#include <linux/types.h>
#include "vcodec_type.h"
#include "drv_venc_osal.h"
#include "drv_venc_ioctl.h"
#include "drv_common.h"

enum venc_ipc_cpu_id {
	ACPU_IPC_ID = 0, // AP
	VRCE_IPC_ID = 1  // MCU
};

enum venc_ipc_mailbox_id {
	MBX0 = 0, // MCU --> AP ,fast ipc
	MBX1 = 1, // AP --> MCU ,fast ipc
	MBX2 = 2 // shared mailbox for slow ipc and irq ack
};

struct venc_ipc_ctx {
	int32_t response; // If ipc message success, mcu must reply a positive integer,otherwise returns -1;
	vedu_osal_event_t ipc_event;
};
typedef enum {
	IPC_MSG_START_ENCODE,
	IPC_MSG_CMDLIST_START_ENCODE,
	IPC_MESSAGE_BUTT
} ipc_message_type;

struct ipc_message {
	ipc_message_type id;
	uint32_t data[7];
};

int32_t venc_ipc_init(struct venc_context *ctx);
void venc_ipc_deinit(struct venc_context *ctx);
void venc_ipc_recive_msg(int32_t mbx_id);
int32_t venc_ipc_send_msg(int32_t mbx_id, struct ipc_message *msg);
void venc_ipc_recive_ack(int32_t mbx_id);
void disable_all_level2_int(uint8_t *intr_hub_reg_base);
void clear_all_level2_int(uint8_t *intr_hub_reg_base);
#endif