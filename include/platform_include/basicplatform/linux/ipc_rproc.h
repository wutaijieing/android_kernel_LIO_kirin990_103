/*
 * ipc_rproc.h
 *
 * ipc rproc communication interfacer
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __IPC_RPROC_H__
#define __IPC_RPROC_H__

#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/kfifo.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/errno.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define EMDEVCLEAN 1
#define EMDEVDIRTY  2
#define ETIMEOUT  3

/* Mailbox's channel size */
#define MBOX_CHAN_DATA_SIZE     8

typedef u32 rproc_msg_t;
typedef int rproc_msg_len_t;
typedef rproc_msg_t mbox_msg_t;
typedef rproc_msg_len_t mbox_msg_len_t;

typedef enum {
	IPC_LPM3_ACPU_MBX_1,
	IPC_LPM3_ACPU_MBX_2,
	IPC_LPM3_ACPU_MBX_3,
	IPC_HIFI_ACPU_MBX_1,
	IPC_IOM3_ACPU_MBX_1,
	IPC_IVP_ACPU_MBX_1,
	IPC_IVP_ACPU_MBX_2,
	IPC_ISP_ACPU_MBX_1,
	IPC_ISP_ACPU_MBX_2,
	IPC_MODEM_ACPU_MBX_1,
	IPC_MODEM_ACPU_MBX_2,
	IPC_MODEM_ACPU_MBX_3,
	IPC_MODEM_ACPU_MBX_4,
	IPC_ACPU_IOM3_MBX_1,
	IPC_ACPU_IOM3_MBX_2,
	IPC_ACPU_IOM3_MBX_3,
	IPC_ACPU_LPM3_MBX_1,
	IPC_ACPU_LPM3_MBX_2,
	IPC_ACPU_LPM3_MBX_3,
	IPC_ACPU_LPM3_MBX_4,
	IPC_ACPU_LPM3_MBX_5,
	IPC_ACPU_LPM3_MBX_6,
	IPC_ACPU_LPM3_MBX_7,
	IPC_ACPU_LPM3_MBX_8,
	IPC_ACPU_LPM3_MBX_9,
	IPC_ACPU_HIFI_MBX_1,
	IPC_ACPU_HIFI_MBX_2,
	IPC_ACPU_MODEM_MBX_1,
	IPC_ACPU_MODEM_MBX_2,
	IPC_ACPU_MODEM_MBX_3,
	IPC_ACPU_MODEM_MBX_4,
	IPC_ACPU_MODEM_MBX_5,
	IPC_ACPU_IVP_MBX_1,
	IPC_ACPU_IVP_MBX_2,
	IPC_ACPU_IVP_MBX_3,
	IPC_ACPU_IVP_MBX_4,
	IPC_ACPU_ISP_MBX_1,
	IPC_ACPU_ISP_MBX_2,
	IPC_AO_ACPU_IOM3_MBX1,
	IPC_AO_IOM3_ACPU_MBX1,
	IPC_NPU_ACPU_NPU_MBX1,
	IPC_NPU_ACPU_NPU_MBX2,
	IPC_NPU_ACPU_NPU_MBX3,
	IPC_NPU_ACPU_NPU_MBX4,
	IPC_NPU_NPU_ACPU_MBX1,
	IPC_NPU_NPU_ACPU_MBX2,
	IPC_RPROC_MAX_MBX_ID
} rproc_id_t;

#define MAILBOX_MANUACK_TIMEOUT msecs_to_jiffies(300)

/*
 * IPC sync msg send
 * @rproc_id        ipc business channel id, see also @rproc_id_t
 * @msg             the message will be sent
 * @len             the length of @msg
 * @ack_buffer      the buffer will be used to store remote point reply data
 * @ack_buffer_len  the length of @ack_buffer
 */
#ifdef CONFIG_AB_APCP_NEW_INTERFACE
#define RPROC_SYNC_SEND(rproc_id, msg, len, ack_buffer, ack_buffer_len) ({\
	ipc_rproc_xfer_sync(rproc_id, msg, len, ack_buffer, ack_buffer_len);\
})
#else
#define RPROC_SYNC_SEND(rproc_id, msg, len, ack_buffer, ack_buffer_len) ({\
	hisi_rproc_xfer_sync(rproc_id, msg, len, ack_buffer, ack_buffer_len);\
})
#endif

/*
 * IPC async msg send
 * @rproc_id        ipc business channel id, see also @rproc_id_t
 * @msg             the message will be sent
 * @len             the length of @msg
 */
#define RPROC_ASYNC_SEND(rproc_id, msg, len) ({\
	ipc_rproc_xfer_async(rproc_id, msg, len);\
})

/*
 * Regist a monitor be used to monit the received msg on the mailbox channel
 * @rproc_id        ipc business channel id, see also @rproc_id_t
 * @nb              notifier block
 */
#define RPROC_MONITOR_REGISTER(rproc_id, nb) ({\
	ipc_rproc_rx_register(rproc_id, nb); \
})

/*
 * Unregist a monitor be used to monit the received msg on the mailbox channel
 * @rproc_id        ipc business channel id, see also @rproc_id_t
 * @nb              notifier block
 */
#define RPROC_MONITOR_UNREGISTER(rproc_id, nb) ({\
	ipc_rproc_rx_unregister(rproc_id, nb); \
})

#define RPROC_GET_IPC_VERSION(rproc_id) ({\
	ipc_rproc_get_ipc_version(rproc_id); \
})

/*
 * Get the mailbox max data register capability
 * @rproc_id        ipc business channel id, see also @rproc_id_t
 */
#define RPROC_GET_MBOX_CHAN_DATA_SIZE(rproc_id) ({\
	ipc_rproc_get_channel_size(rproc_id); \
})

/* Get the business channel id maped physical channel id
 * @rproc_id        ipc business channel id, see also @rproc_id_t
 */
#define RPROC_GET_MAILBOX_ID(rproc_id) ({\
	ipc_rproc_get_mailbox_id(rproc_id);\
})

#define RPROC_PUT(rproc_id)         ipc_rproc_put(rproc_id)
#define RPROC_FLUSH_TX(rproc_id)    ipc_rproc_flush_tx(rproc_id)
#define RPROC_IS_SUSPEND(rproc_id)  ipc_rproc_is_suspend(rproc_id)

int ipc_rproc_xfer_sync(rproc_id_t rproc_id,
	rproc_msg_t *msg,
	rproc_msg_len_t len,
	rproc_msg_t *ack_buffer,
	rproc_msg_len_t ack_buffer_len);

#ifndef CONFIG_AB_APCP_NEW_INTERFACE
int hisi_rproc_xfer_sync(rproc_id_t rproc_id,
	rproc_msg_t *msg,
	rproc_msg_len_t len,
	rproc_msg_t *ack_buffer,
	rproc_msg_len_t ack_buffer_len);
#endif

int ipc_rproc_xfer_async(rproc_id_t rproc_id,
	rproc_msg_t *msg,
	rproc_msg_len_t len);

int ipc_rproc_get_channel_size(rproc_id_t rproc_id);
int ipc_rproc_get_ipc_version(rproc_id_t rproc_id);
int ipc_rproc_get_mailbox_id(rproc_id_t rproc_id);
int ipc_rproc_xfer_async_with_ack(
	rproc_id_t rproc_id, rproc_msg_t *msg, rproc_msg_len_t len,
	void (*ack_handle)(rproc_msg_t *ack_buffer,
		rproc_msg_len_t ack_buffer_len));

int ipc_rproc_rx_register(rproc_id_t rproc_id, struct notifier_block *nb);
int ipc_rproc_rx_unregister(rproc_id_t rproc_id, struct notifier_block *nb);
int ipc_rproc_put(rproc_id_t rproc_id);
int ipc_rproc_flush_tx(rproc_id_t rproc_id);
int ipc_rproc_is_suspend(rproc_id_t rproc_id);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* __IPC_RPROC_H__ */
