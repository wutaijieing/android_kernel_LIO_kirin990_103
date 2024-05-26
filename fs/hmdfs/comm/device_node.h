/* SPDX-License-Identifier: GPL-2.0 */
/*
 * device_node.h
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
 * Description: send and recv msg for hmdfs
 * Author: liuxuesong3@huawei.com
 * Create: 2020-04-14
 *
 */

#ifndef HMDFS_DEVICE_NODE_H
#define HMDFS_DEVICE_NODE_H

#include "hmdfs.h"
#include "transport.h"

enum CTRL_NODE_CMD {
	CMD_INIT = 0,
	CMD_UPDATE_SOCKET,
	CMD_OFF_LINE,
	CMD_SET_ACCOUNT,
	CMD_OFF_LINE_ALL,
	CMD_UPDATE_CAPABILITY,
	CMD_GET_P2P_SESSION_FAIL,
	CMD_DELETE_CONNECTION,
	CMD_UPDATE_DEVSL,
	CMD_CNT,
};

enum {
	CAPABILITY_P2P = 0,
	CAPABILITY_CNT,
};

struct update_capability_param {
	int32_t cmd;
	uint64_t capability;
	uint8_t cid[HMDFS_CID_SIZE];
} __packed;

struct get_p2p_session_fail_param {
	int32_t cmd;
	uint8_t cid[HMDFS_CID_SIZE];
} __packed;

struct init_param {
	int32_t cmd;
	uint64_t local_iid;
	uint8_t current_account[HMDFS_ACCOUNT_HASH_MAX_LEN];
} __packed;

enum {
	LINK_TYPE_WLAN = 0,
	LINK_TYPE_P2P,
	LINK_TYPE_LOOPBACK,
};

struct update_socket_param {
	int32_t cmd;
	int32_t newfd;
	uint64_t local_iid;
	uint8_t status;
	uint8_t protocol;
	uint16_t udp_port;
	uint8_t version;
	uint8_t device_type;
	uint8_t masterkey[HMDFS_KEY_SIZE];
	uint8_t cid[HMDFS_CID_SIZE];
	int32_t link_type;  /* WIFI or P2P or LOOPBACK */
	int32_t binded_fd;
} __packed;

struct update_devsl_param {
    int32_t cmd;
    uint32_t devsl;
    uint8_t cid[HMDFS_CID_SIZE];
} __attribute__((packed));

struct offline_param {
	int32_t cmd;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct offline_all_param {
	int32_t cmd;
} __packed;

struct set_account_param {
	int32_t cmd;
	uint8_t current_account[HMDFS_ACCOUNT_HASH_MAX_LEN];
} __packed;

struct delete_connection_param {
	int32_t cmd;
	int32_t fd;
	uint8_t cid[HMDFS_CID_SIZE];
} __packed;

enum NOTIFY {
	NOTIFY_HS_DONE = 0,
	NOTIFY_OFFLINE,
	NOTIFY_OFFLINE_IID,
	NOTIFY_GET_SESSION,
	NOTIFY_REGET_SESSION,
	NOTIFY_DISCONNECT, /* one connection disconnected */
	NOTIFY_D2DP_REMOTE,
	NOTIFY_D2DP_FAILED,
	NOTIFY_CNT,
};

struct connection_id {
	struct in_addr host_ip;
	uint16_t host_port;
	struct in_addr peer_ip;
	uint16_t peer_port;
} __packed;

enum {
	NOTIFY_FLAG_P2P = 0,
	NOTIFY_FLAG_CNT,
};

struct notify_hs_done_param {
	int32_t notify;
	uint64_t remote_iid;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_offline_param {
	int32_t notify;
	uint64_t remote_iid;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_offline_iid_param {
	int32_t notify;
	uint64_t remote_iid;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_reget_session_param {
	int32_t notify;
	int32_t fd;
	uint64_t remote_iid;
	int32_t flag;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_disconnect_param {
	int32_t notify;
	int32_t fd;
	uint64_t remote_iid;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_get_session_param {
	int32_t notify;
	uint64_t remote_iid;
	int32_t flag;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_d2dp_remote_param {
	int32_t notify;
	int32_t fd;
	uint16_t remote_udp_port;
	uint8_t remote_version;
	uint8_t remote_device_type;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct notify_d2dp_failed_param {
	int32_t notify;
	int32_t fd;
	uint8_t remote_cid[HMDFS_CID_SIZE];
} __packed;

struct sbi_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, struct sbi_attribute *attr,
			char *buf);
	ssize_t (*store)(struct kobject *kobj, struct sbi_attribute *attr,
			 const char *buf, size_t len);
};

struct peer_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, struct peer_attribute *attr,
			char *buf);
	ssize_t (*store)(struct kobject *kobj, struct peer_attribute *attr,
			 const char *buf, size_t len);
};

struct sbi_cmd_attribute {
	struct attribute attr;
	int command;
};

void notify_hs_done(struct hmdfs_peer *node);
void notify_offline(struct hmdfs_peer *node);
void notify_offline_iid(struct hmdfs_peer *node);
void notify_reget_session(struct connection *conn);
void notify_get_connection(struct hmdfs_peer *peer, int flag);
void notify_disconnect(struct connection *conn);
#ifdef CONFIG_HMDFS_D2DP_TRANSPORT
void notify_d2dp_remote(struct connection *conn);
void notify_d2dp_failed(struct connection *conn);
#endif
int hmdfs_register_sysfs(struct hmdfs_sb_info *sbi, const char *name,
			 int namelen);
void hmdfs_unregister_sysfs(struct hmdfs_sb_info *sbi);
void hmdfs_release_sysfs(struct hmdfs_sb_info *sbi);
int hmdfs_register_peer_sysfs(struct hmdfs_sb_info *sbi,
			      struct hmdfs_peer *peer);
void hmdfs_release_peer_sysfs(struct hmdfs_peer *peer);
int hmdfs_sysfs_init(void);
void hmdfs_sysfs_exit(void);

static inline void hmdfs_disconnect_node_marked(struct hmdfs_peer *conn)
{
	hmdfs_start_process_offline(conn);
	hmdfs_disconnect_node(conn);
	hmdfs_stop_process_offline(conn);
}

static inline struct sbi_attribute *to_sbi_attr(struct attribute *x)
{
	return container_of(x, struct sbi_attribute, attr);
}

static inline struct hmdfs_sb_info *to_sbi(struct kobject *x)
{
	return container_of(x, struct hmdfs_sb_info, kobj);
}

static inline struct peer_attribute *to_peer_attr(struct attribute *x)
{
	return container_of(x, struct peer_attribute, attr);
}

static inline struct hmdfs_peer *to_peer(struct kobject *x)
{
	return container_of(x, struct hmdfs_peer, kobj);
}
#endif
