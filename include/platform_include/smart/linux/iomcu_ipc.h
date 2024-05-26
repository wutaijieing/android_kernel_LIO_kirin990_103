/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Contexthub share memory driver
 * Create: 2019-10-01
 */
#ifndef __IOMCU_IPC_H__
#define __IOMCU_IPC_H__

#include <linux/types.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOM3_ST_NORMAL 0
#define IOM3_ST_RECOVERY 1
#define IOM3_ST_REPEAT 2
#define MAX_SEND_LEN 32
#define IPC_BUF_USR_SIZE   (MAX_SEND_LEN - sizeof(struct pkt_header))
#define AMF20_CMD_ALL  0xFFFF

enum iomcu_trans_type {
	IOMCU_TRANS_IPC_TYPE = 0,
	IOMCU_TRANS_SHMEM_TYPE = 1,
	IOMCU_TRANS_IPC64_TYPE = 2,
	IOMCU_TRANS_END_TYPE,
};

struct write_info {
	int tag;
	int cmd;
	const void *wr_buf; /* maybe NULL */
	int wr_len; /* maybe zero */
	int app_tag;
};

struct read_info {
	int errno;
	int data_length;
	char data[MAX_PKT_LENGTH];
};

extern int inputhub_recv_msg_app_handler(const struct pkt_header *head, bool is_notifier);
void inputhub_pm_callback(struct pkt_header *head);
extern int iomcu_route_recv_mcu_data(const char *buf, unsigned int length);
extern int write_customize_cmd(const struct write_info *wr, struct read_info *rd, bool is_lock);
extern int register_mcu_event_notifier(int tag, int cmd, int (*notify)(const struct pkt_header *head));
extern int unregister_mcu_event_notifier(int tag, int cmd, int (*notify)(const struct pkt_header *head));
extern void inputhub_update_info(const void *buf, int ret, bool is_in_recovery);
extern void init_locks(void);
extern void init_mcu_notifier_list(void);
int iomcu_ipc_init(void);
void iomcu_ipc_exit(void);
int iomcu_write_ipc_msg(struct pkt_header *pkt, unsigned int length, struct read_info *rd, uint8_t waiter_cmd);
int iomcu_ipc_recv_register(void);
int iomcu_ipc_recv_notifier(struct notifier_block *nb, unsigned long len, void *msg);

#ifdef CONFIG_IPC_V5
typedef int (*tcp_amf20_notify_f)(const uint16_t svc_id, const uint16_t cmd, const void *data, const uint16_t len);
int tcp_send_data_by_amf20(const uint16_t svc_id, const uint16_t cmd, uintptr_t *data, uint16_t data_len);
int register_amf20_notifier(uint16_t svc_id, uint16_t cmd, tcp_amf20_notify_f tcp_notify);
int unregister_amf20_notifier(uint16_t svc_id, uint16_t cmd, tcp_amf20_notify_f tcp_notify);
#endif

#ifdef __cplusplus
}
#endif
#endif
