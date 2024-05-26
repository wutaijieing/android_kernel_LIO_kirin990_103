/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: mpflow for vlan module implemention
 * Author: wkang.wang@huawei.com
 * Create: 2022-01-25
 */
#ifndef __MPFLOW_VLAN_H__
#define __MPFLOW_VLAN_H__

#include <net/sock.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/netdevice.h>

#define MPVLAN_UID_MAX 50
#define ROOT_UID_MASK 100000

typedef enum {
	NET_TYPE_INVALID = -1,
	NET_TYPE_WIFI,
	NET_TYPE_VLAN,
	NET_TYPE_MAX,
} net_type;

typedef enum {
	MPVLAN_BINDMODE_NONE = 0,
	MPVLAN_BINDMODE_WIFI = 1 << NET_TYPE_WIFI,
	MPVLAN_BINDMODE_VLAN = 1 << NET_TYPE_VLAN,
} enum_mpvlan_bindmode;

typedef enum {
	APP_DEL = 0,
	APP_ADD = 1,
} enum_update_app_op;

typedef struct {
	uid_t uid; /* The uid of Application */
	uint32_t bind_mode;
} mpflow_vlan_app_info;

typedef struct {
	bool is_started;
	uint32_t app_num;
	uint32_t all_bind_mode;
	char vlan_iface_name[IFNAMSIZ];
	mpflow_vlan_app_info all_app[MPVLAN_UID_MAX];
} mpflow_vlan_all_app_info;

typedef struct {
	char vlan_iface_name[IFNAMSIZ];
	uint32_t uid_num;
	uint32_t uid[0];
} mpflow_vlan_parse_start_info;

typedef struct {
	uint32_t uid;
	uint32_t operation;
} mpflow_vlan_parse_update_app_info;

typedef struct {
	uint32_t wifi_status;
} mpflow_vlan_parse_network_status_info;

void mpflow_vlan_init(void);
void mpflow_vlan_clear(void);
void mpflow_vlan_config_start(const char *pdata, uint16_t len);
void mpflow_vlan_config_stop(const char *pdata, uint16_t len);
void mpflow_vlan_update_app_info(const char *pdata, uint16_t len);
void mpflow_vlan_network_status_update(const char *pdata, uint16_t len);
bool mpflow_vlan_bind2device(struct sock *sk, struct sockaddr *uaddr, uid_t uid);

#endif
