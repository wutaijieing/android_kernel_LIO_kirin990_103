/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Contexthub ipc link.
 * Create: 2021-10-14
 */
#ifndef __IOMCU_IPC_LINK_H__
#define __IOMCU_IPC_LINK_H__

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tcp_ver {
	TCP_AMSM = 0,
	TCP_AMF10,
	TCP_AMF20,
	TCP_MAX,
};


typedef int (*link_ipc_notify_f)(const void *data, const uint16_t len);
// link layer.
int iomcu_link_ipc_send(uint8_t level, enum tcp_ver version, void *data, uint16_t length);

int iomcu_link_ipc_recv_register(enum tcp_ver version, link_ipc_notify_f notify);
int iomcu_link_ipc_recv_unregister(enum tcp_ver version, link_ipc_notify_f notify);

void iomcu_link_ipc_put_buffer(unsigned long tcp_addr, bool is_force);
unsigned long iomcu_link_ipc_get_buffer(uint16_t size, uint32_t context);

#ifdef __cplusplus
}
#endif
#endif
