/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: mpflow for vlan module implemention
 * Author: wkang.wang@huawei.com
 * Create: 2022-01-25
 */
#include "emcom/mpflow_vlan.h"
#include <asm/uaccess.h>
#include <linux/fdtable.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/version.h>
#include <net/inet_hashtables.h>
#include <net/inet6_hashtables.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/udp.h>
#include "../emcom_netlink.h"
#include "../emcom_utils.h"
#include "emcom/emcom_xengine.h"
#include "securec.h"

#undef HWLOG_TAG
#define HWLOG_TAG emcom_mpflow_vlan
HWLOG_REGIST();

#define SKARR_SZ 200

static mpflow_vlan_all_app_info g_mpvlan_all_app_info;
static spinlock_t g_mpvlan_all_app_info_lock;

static void mpflow_vlan_reset_all_flow(const mpflow_vlan_all_app_info *app);

static inline bool invalid_uid(uid_t uid)
{
	/* if uid less than 10000, it is not an Android apk */
	return (uid < UID_APP);
}

static inline uid_t get_root_uid(uid_t uid)
{
	return uid % ROOT_UID_MASK;
}

void mpflow_vlan_init(void)
{
	errno_t err;
	emcom_logd("mpflow_vlan init");
	spin_lock_init(&g_mpvlan_all_app_info_lock);
	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	err = memset_s(&g_mpvlan_all_app_info, sizeof(mpflow_vlan_all_app_info), 0, sizeof(mpflow_vlan_all_app_info));
	if (err != EOK)
		emcom_loge("mpflow_vlan_init g_mpvlan_all_app_info failed");
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
}

void mpflow_vlan_clear(void)
{
	errno_t err;
	emcom_logd("mpflow_vlan clear");
	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	err = memset_s(&g_mpvlan_all_app_info, sizeof(mpflow_vlan_all_app_info), 0, sizeof(mpflow_vlan_all_app_info));
	if (err != EOK)
		emcom_loge("g_mpvlan_all_app_info memset_s failed");
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
}

void mpflow_vlan_config_start(const char *pdata, uint16_t len)
{
	mpflow_vlan_parse_start_info *startinfo = NULL;
	errno_t err;
	uint32_t i;

	/* input param check */
	if (!pdata || (len <= sizeof(mpflow_vlan_parse_start_info))) {
		emcom_loge("mpflow_vlan start data or length: %hu is error", len);
		return;
	}
	startinfo = (mpflow_vlan_parse_start_info *)pdata;
	if (startinfo->uid_num > MPVLAN_UID_MAX || strlen(startinfo->vlan_iface_name) == 0) {
		emcom_loge("mpflow_vlan uid too many or name err, uid_num: %u", startinfo->uid_num);
		return;
	}
	if (len < startinfo->uid_num * sizeof(uint32_t) + sizeof(mpflow_vlan_parse_start_info)) {
		emcom_loge("mpflow_vlan start uid_num: %u length: %hu is error", startinfo->uid_num, len);
		return;
	}

	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	if (g_mpvlan_all_app_info.is_started) {
		emcom_loge("mpflow_vlan already start");
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return;
	}
	g_mpvlan_all_app_info.is_started = true;
	g_mpvlan_all_app_info.app_num = startinfo->uid_num;
	g_mpvlan_all_app_info.all_bind_mode = MPVLAN_BINDMODE_VLAN;
	err = strcpy_s(g_mpvlan_all_app_info.vlan_iface_name, sizeof(g_mpvlan_all_app_info.vlan_iface_name),
		startinfo->vlan_iface_name);
	if (err != EOK) {
		emcom_loge("strcpy_s failed ret=%d", err);
		(void)memset_s(&g_mpvlan_all_app_info, sizeof(mpflow_vlan_all_app_info), 0, sizeof(mpflow_vlan_all_app_info));
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return;
	}
	for (i = 0; i < MPVLAN_UID_MAX; i++) {
		if (i < startinfo->uid_num) {
			g_mpvlan_all_app_info.all_app[i].uid = get_root_uid(startinfo->uid[i]);
			g_mpvlan_all_app_info.all_app[i].bind_mode = g_mpvlan_all_app_info.all_bind_mode;
		} else {
			g_mpvlan_all_app_info.all_app[i].uid = UID_INVALID_APP;
			g_mpvlan_all_app_info.all_app[i].bind_mode = MPVLAN_BINDMODE_NONE;
		}
	}
	emcom_logi("config uid_num: %u vlan_iface_name: %s", startinfo->uid_num, g_mpvlan_all_app_info.vlan_iface_name);
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
}

void mpflow_vlan_config_stop(const char *pdata, uint16_t len)
{
	errno_t err;
	mpflow_vlan_all_app_info mpvlan_all_app_info;
	emcom_logd("mpflow_vlan stop");
	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	g_mpvlan_all_app_info.all_bind_mode = MPVLAN_BINDMODE_WIFI;
	mpvlan_all_app_info = g_mpvlan_all_app_info;
	err = memset_s(&g_mpvlan_all_app_info, sizeof(mpflow_vlan_all_app_info), 0, sizeof(mpflow_vlan_all_app_info));
	if (err != EOK)
		emcom_loge("g_mpvlan_all_app_info memset_s failed");
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
	mpflow_vlan_reset_all_flow(&mpvlan_all_app_info);
}

void mpflow_vlan_update_app_info(const char *pdata, uint16_t len)
{
	mpflow_vlan_parse_update_app_info *update_app_info = NULL;
	uint32_t i;

	/* input param check */
	if (!pdata || (len != sizeof(mpflow_vlan_parse_update_app_info))) {
		emcom_loge("mpflow_vlan_update_app_info data or length: %hu is error", len);
		return;
	}
	update_app_info = (mpflow_vlan_parse_update_app_info *)pdata;
	emcom_logd("uid: %u operation: %u", update_app_info->uid, update_app_info->operation);
	if (invalid_uid(update_app_info->uid)) {
		emcom_loge("invalid_uid: %u", update_app_info->uid);
		return;
	}

	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	if (!g_mpvlan_all_app_info.is_started) {
		emcom_loge("mpflow_vlan is not started");
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return;
	}
	if (update_app_info->operation == APP_ADD) {
		uint32_t empty_index = MPVLAN_UID_MAX;
		for (i = 0; i < MPVLAN_UID_MAX; i++) {
			if (g_mpvlan_all_app_info.all_app[i].uid == get_root_uid(update_app_info->uid)) {
				emcom_logi("duplicated uid: %u", update_app_info->uid);
				spin_unlock_bh(&g_mpvlan_all_app_info_lock);
				return;
			}
			if (empty_index == MPVLAN_UID_MAX && g_mpvlan_all_app_info.all_app[i].uid == UID_INVALID_APP)
				empty_index = i;
			if (i > g_mpvlan_all_app_info.app_num)
				break;
		}
		if (empty_index < MPVLAN_UID_MAX) {
			g_mpvlan_all_app_info.all_app[empty_index].uid = get_root_uid(update_app_info->uid);
			g_mpvlan_all_app_info.all_app[empty_index].bind_mode = g_mpvlan_all_app_info.all_bind_mode;
			if (empty_index == g_mpvlan_all_app_info.app_num)
				g_mpvlan_all_app_info.app_num++;
			emcom_logd("add ok. uid: %u num: %u", update_app_info->uid, g_mpvlan_all_app_info.app_num);
		} else {
			emcom_loge("add fail. uid: %u num: %u", update_app_info->uid, g_mpvlan_all_app_info.app_num);
		}
	} else {
		for (i = 0; i < g_mpvlan_all_app_info.app_num; i++) {
			if (g_mpvlan_all_app_info.all_app[i].uid == get_root_uid(update_app_info->uid)) {
				g_mpvlan_all_app_info.all_app[i].uid = UID_INVALID_APP;
				break;
			}
		}
	}
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
}

void mpflow_vlan_network_status_update(const char *pdata, uint16_t len)
{
	mpflow_vlan_parse_network_status_info *net_satus = NULL;
	mpflow_vlan_all_app_info mpvlan_all_app_info;
	uint32_t i;

	/* input param check */
	if (!pdata || (len != sizeof(mpflow_vlan_parse_network_status_info))) {
		emcom_loge("mpflow_vlan_network_status_update data or length: %hu is error", len);
		return;
	}
	net_satus = (mpflow_vlan_parse_network_status_info *)pdata;
	emcom_logd("wifi_status: %u", net_satus->wifi_status);
	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	if (!g_mpvlan_all_app_info.is_started) {
		emcom_loge("mpflow_vlan is not started");
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return;
	}
	if (net_satus->wifi_status) {
		g_mpvlan_all_app_info.all_bind_mode |= MPVLAN_BINDMODE_WIFI;
		for (i = 0; i < g_mpvlan_all_app_info.app_num; i++)
			g_mpvlan_all_app_info.all_app[i].bind_mode = MPVLAN_BINDMODE_WIFI; // reset to wifi
	} else {
		g_mpvlan_all_app_info.all_bind_mode &= ~MPVLAN_BINDMODE_WIFI;
		for (i = 0; i < g_mpvlan_all_app_info.app_num; i++)
			g_mpvlan_all_app_info.all_app[i].bind_mode = MPVLAN_BINDMODE_VLAN; // reset to VLAN
	}
	mpvlan_all_app_info = g_mpvlan_all_app_info;
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
	mpflow_vlan_reset_all_flow(&mpvlan_all_app_info);
}

static void mpflow_vlan_reset_all_tcp_flow(uint32_t uid, const char*dst_ifname)
{
	struct sock *sk = NULL;
	uint32_t bucket = 0;
	struct sock *sk_arr[SKARR_SZ];
	int accum = 0;
	int idx = 0;

	for (; bucket <= tcp_hashinfo.ehash_mask; bucket++) {
		struct inet_ehash_bucket *head = &tcp_hashinfo.ehash[bucket];
		spinlock_t *lock = inet_ehash_lockp(&tcp_hashinfo, bucket);
		struct hlist_nulls_node *node = NULL;

		if (hlist_nulls_empty(&head->chain))
			continue;

		spin_lock_bh(lock);
		sk_nulls_for_each(sk, node, &head->chain) {
			if (sk->sk_state == TCP_ESTABLISHED || sk->sk_state == TCP_SYN_SENT) {
				uid_t sock_uid = get_root_uid(sock_i_uid(sk).val);
				char ifname[IFNAMSIZ] = {0};
				if (uid != sock_uid || emcom_xengine_mpflow_ai_check_loop_sk(sk))
					continue;
				if (!sk->is_mp_flow_bind && sk->sk_bound_dev_if)
					continue;
				if (dst_ifname) {
					(void)netdev_get_name(sock_net(sk), ifname, sk->sk_bound_dev_if);
					if (strncmp(ifname, dst_ifname, strlen(dst_ifname)) == 0)
						continue;
				}
				if (unlikely(!refcount_inc_not_zero(&sk->sk_refcnt)))
					continue;
				sk_arr[accum] = sk;
				accum++;
			}

			if (accum >= SKARR_SZ)
				break;
		}
		spin_unlock_bh(lock);
		if (accum >= SKARR_SZ)
			break;
	}
	emcom_logi("reset tcp flow, uid: %u dst_ifname: %s flow_num: %d", uid, dst_ifname, accum);

	for (; idx < accum; idx++) {
		emcom_xengine_mpflow_reset(tcp_sk(sk_arr[idx]));
		sock_gen_put(sk_arr[idx]);
	}
}

static void mpflow_vlan_reset_all_udp_flow(uint32_t uid, const char*dst_ifname)
{
	struct sock *sk = NULL;
	uint32_t bucket = 0;
	struct sock *sk_arr[SKARR_SZ];
	int accum = 0;
	int idx = 0;

	for (; bucket <= udp_table.mask; bucket++) {
		struct udp_hslot *hslot = &udp_table.hash[bucket];

		if (hlist_empty(&hslot->head))
			continue;

		spin_lock_bh(&hslot->lock);
		sk_for_each(sk, &hslot->head) {
			if (sk->sk_state != TCP_SYN_RECV && sk->sk_state != TCP_TIME_WAIT &&
				sk->sk_state != TCP_NEW_SYN_RECV) {
				uid_t sock_uid = get_root_uid(sock_i_uid(sk).val);
				char ifname[IFNAMSIZ] = {0};
				if (uid != sock_uid || emcom_xengine_mpflow_ai_check_loop_sk(sk))
					continue;
				if (dst_ifname) {
					(void)netdev_get_name(sock_net(sk), ifname, sk->sk_bound_dev_if);
					if (strncmp(ifname, dst_ifname, strlen(dst_ifname)) == 0)
						continue;
				}
				if (unlikely(!refcount_inc_not_zero(&sk->sk_refcnt)))
					continue;
				sk_arr[accum] = sk;
				accum++;
			}
			if (accum >= SKARR_SZ)
				break;
		}
		spin_unlock_bh(&hslot->lock);
		if (accum >= SKARR_SZ)
			break;
	}
	emcom_logi("reset udp flow, uid: %u dst_ifname: %s flow_num: %d", uid, dst_ifname, accum);

	for (; idx < accum; idx++) {
		emcom_xengine_mpflow_ai_udp_reset(sk_arr[idx]);
		sock_gen_put(sk_arr[idx]);
	}
}

static void mpflow_vlan_reset_all_flow(const mpflow_vlan_all_app_info *app)
{
	int i;
	if (app->all_bind_mode & MPVLAN_BINDMODE_WIFI) {
		for (i = 0; i < app->app_num; i++) {
			if (!invalid_uid(app->all_app[i].uid)) {
				mpflow_vlan_reset_all_tcp_flow(app->all_app[i].uid, NULL);
				mpflow_vlan_reset_all_udp_flow(app->all_app[i].uid, NULL);
			}
		}
	} else {
		for (i = 0; i < app->app_num; i++) {
			if (!invalid_uid(app->all_app[i].uid)) {
				mpflow_vlan_reset_all_tcp_flow(app->all_app[i].uid, app->vlan_iface_name);
				mpflow_vlan_reset_all_udp_flow(app->all_app[i].uid, app->vlan_iface_name);
			}
		}
	}
}

static uint32_t mpflow_vlan_finduid(uid_t uid)
{
	uint32_t i;
	for (i = 0; i < g_mpvlan_all_app_info.app_num; i++) {
		if (g_mpvlan_all_app_info.all_app[i].uid == get_root_uid(uid))
			return i;
	}
	return INDEX_INVALID;
}

static int mpflow_vlan_get_bind_device(uint32_t index)
{
	if (g_mpvlan_all_app_info.all_app[index].bind_mode & MPVLAN_BINDMODE_WIFI)
		return MPVLAN_BINDMODE_NONE;
	else
		return MPVLAN_BINDMODE_VLAN;
}

static int mpflow_vlan_get_bind_iface_name(char *ifname, unsigned int len, int bind_device)
{
	if (bind_device == MPVLAN_BINDMODE_VLAN) {
		return memcpy_s(ifname, len, g_mpvlan_all_app_info.vlan_iface_name,
			(strlen(g_mpvlan_all_app_info.vlan_iface_name) + 1));
	}
	return EOK;
}

static int mpflow_vlan_rehash(char *ifname, unsigned int len, struct sock *sk, uid_t uid, uint16_t dport)
{
	struct net_device *dev = NULL;
	struct inet_sock *inet = NULL;

	if (strlen(ifname) == 0) {
		/* 1.wzry sgame stuck one min when shutdown LTE, because UDP not create a new sock when receive a reset,
		the old sock sk_bound_dev_if is LTE, so the UDP sock will continue use LTE to send packet
		2.as a wifi hotpoint, can not bind wlan, need reset sk_bound_dev_if to 0 */
		emcom_logd("MPVLAN_BINDMODE_NONE uid: %u, sk: %pK, protocol: %u, DstPort: %u, dev_if: %u", uid, sk,
			sk->sk_protocol, dport, sk->sk_bound_dev_if);
		if (sk->sk_bound_dev_if && sk->sk_protocol == IPPROTO_UDP) {
			/* Fix the bug for weixin app bind wifi ip, then reset to lte, cannot find the sk */
			if (!emcom_xengine_mpflow_ai_rehash_sk(sk))
				return MPFLOW_ERROR;
			sk->sk_bound_dev_if = 0;
			sk_dst_reset(sk);
			inet = inet_sk(sk);
			if (inet->inet_saddr)
				inet->inet_saddr = 0;
		} else {
			sk->sk_bound_dev_if = 0;
		}
		return MPFLOW_ERROR;
	}

	/* Fix the bug for weixin app bind wifi ip, then reset to lte, cannot find the sk */
	if (!emcom_xengine_mpflow_ai_rehash_sk(sk))
		return MPFLOW_ERROR;

	rcu_read_lock();
	dev = dev_get_by_name_rcu(sock_net(sk), ifname);
	if (!dev || (emcom_xengine_mpflow_getinetaddr(dev) == false)) {
		rcu_read_unlock();
		emcom_logd("device not ready uid: %u, sk: %pK, dev: %pK, name: %s",
				uid, sk, dev, (dev == NULL) ? "null" : dev->name);
		return MPFLOW_ERROR;
	}
	rcu_read_unlock();

	sk->sk_bound_dev_if = dev->ifindex;
	if (sk->sk_protocol == IPPROTO_UDP) {
		sk_dst_reset(sk);
		inet = inet_sk(sk);
		if (inet->inet_saddr)
			inet->inet_saddr = 0;
	}

	emcom_logi("bind success uid: %u, ifname: %s, len: %d, ifindex: %d, srcPort: %u, protocol: %u",
			uid, ifname, len, sk->sk_bound_dev_if, sk->sk_num, sk->sk_protocol);

	return MPFLOW_OK;
}

bool mpflow_vlan_bind2device(struct sock *sk, struct sockaddr *uaddr, uid_t uid)
{
	char ifname[IFNAMSIZ] = {0};
	errno_t err = EOK;
	uint16_t dport = 0;
	int bind_device;
	uint32_t index;
	bool isvalidaddr = false;
	struct sockaddr_in *usin = NULL;

	spin_lock_bh(&g_mpvlan_all_app_info_lock);
	index = mpflow_vlan_finduid(uid);
	if (index == INDEX_INVALID) {
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return false;
	}

	emcom_xengine_mpflow_ai_get_port(uaddr, &dport);  // not check return value
	isvalidaddr = emcom_xengine_check_ip_addrss(uaddr) && (!emcom_xengine_check_ip_is_private(uaddr));
	if (isvalidaddr == false) {
		emcom_logd("invalid addr. uid: %u", uid);
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return true;
	}

	bind_device = mpflow_vlan_get_bind_device(index);
	err = mpflow_vlan_get_bind_iface_name(ifname, IFNAMSIZ, bind_device);
	if (err != EOK) {
		emcom_loge("mpflow vlan bind memcpy ifname failed");
		spin_unlock_bh(&g_mpvlan_all_app_info_lock);
		return true;
	}
	spin_unlock_bh(&g_mpvlan_all_app_info_lock);
	sk->is_mp_flow_bind = 1;
	if (mpflow_vlan_rehash(ifname, IFNAMSIZ, sk, uid, dport) != MPFLOW_OK)
		return true;

	usin = (struct sockaddr_in *)uaddr;
	if (usin->sin_family == AF_INET) {
		emcom_logi("[MPFlow_vlan_KERNEL] Bind Completed. SrcIP: *.*, SrcPort: %u, DstIP:"IPV4_FMT", DstPort: %u ",
			sk->sk_num, ipv4_info(usin->sin_addr.s_addr), dport);
	} else if (usin->sin_family == AF_INET6) {
		struct sockaddr_in6 *usin6 = (struct sockaddr_in6 *)uaddr;
		emcom_logi("[MPFlow_vlan_KERNEL] Bind Completed. SrcIP: *.*, SrcPort: %u, DstIP:"IPV6_FMT", DstPort: %u ",
			sk->sk_num, ipv6_info(usin6->sin6_addr), dport);
	}
	return true;
}


MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("xengine module driver");

