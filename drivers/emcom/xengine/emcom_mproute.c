/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: mproute module implemention
 * Author: lijiqing jiqing.li@huawei.com
 * Create: 2021-10-10
 */

#include "emcom/emcom_mproute.h"
#include "securec.h"
#include <linux/slab.h>
#include <net/inet_hashtables.h>
#include <net/inet6_hashtables.h>
#include <net/tcp.h>
#include <net/udp.h>

#include "../emcom_netlink.h"
#include "../emcom_utils.h"

mproute_policy g_mproute_policy;
int32_t g_default_mark;
spinlock_t g_mproute_lock;
extern struct inet_hashinfo tcp_hashinfo;

#define MPROUTE_FWMARK_CONTROL_MASK 0xFFFF0000
#define MPROUTE_FWMARK_NEW_MASK 0xFFFF
#define SKARR_SZ 200
#define INDEX_INVALID (-1)
#define INVALID_MARK 0
#define IPV4_FMT "%u.%u.%u.%u"
#define ipv4_info(addr) \
	((unsigned char *)&(addr))[0], \
	((unsigned char *)&(addr))[1], \
	((unsigned char *)&(addr))[2], \
	((unsigned char *)&(addr))[3]
#define IPV6_FMT "%x:%x:%x:%x:%x:%x:%x:%x"
#define ipv6_info(addr) \
	ip6_addr_block1(addr), \
	ip6_addr_block2(addr), \
	ip6_addr_block3(addr), \
	ip6_addr_block4(addr), \
	ip6_addr_block5(addr), \
	ip6_addr_block6(addr), \
	ip6_addr_block7(addr), \
	ip6_addr_block8(addr)

#define ip6_addr_block1(ip6_addr) (ntohs((ip6_addr).s6_addr16[0]) & 0xffff)
#define ip6_addr_block2(ip6_addr) (ntohs((ip6_addr).s6_addr16[1]) & 0xffff)
#define ip6_addr_block3(ip6_addr) (ntohs((ip6_addr).s6_addr16[2]) & 0xffff)
#define ip6_addr_block4(ip6_addr) (ntohs((ip6_addr).s6_addr16[3]) & 0xffff)
#define ip6_addr_block5(ip6_addr) (ntohs((ip6_addr).s6_addr16[4]) & 0xffff)
#define ip6_addr_block6(ip6_addr) (ntohs((ip6_addr).s6_addr16[5]) & 0xffff)
#define ip6_addr_block7(ip6_addr) (ntohs((ip6_addr).s6_addr16[6]) & 0xffff)
#define ip6_addr_block8(ip6_addr) (ntohs((ip6_addr).s6_addr16[7]) & 0xffff)

void emcom_mproute_set_default_mark(int32_t mark)
{
	g_default_mark = mark;
}

int32_t emcom_mproute_get_default_mark(void)
{
	return g_default_mark;
}

static void emcom_mproute_policy_init(
	const struct emcom_xengine_mpflow_ai_init_bind_cfg *config,
	struct emcom_xengine_mpflow_ai_info *app)
{
	const struct emcom_xengine_mpflow_ai_init_bind_policy *policy = NULL;
	uint32_t i;
	uint32_t index;

	for (index = 0; index < config->policy_num; index++) {
		policy = &config->scatter_bind[index];

		if ((policy->l4_protocol != IPPROTO_TCP) && (policy->l4_protocol != IPPROTO_UDP)) {
			emcom_loge("invalid protocol: %d", policy->l4_protocol);
			continue;
		}
		if (policy->mode > EMCOM_MPFLOW_IP_BURST_FIX) {
			emcom_loge("invalid mode: %d", policy->mode);
			continue;
		}

		for (i = 0; (i < policy->port_num) && (i < EMCOM_MPFLOW_BIND_PORT_CFG_SIZE); i++) {
			emcom_logi("port[%d, %d]", policy->port_range[i].start_port,
				policy->port_range[i].end_port);

			if (policy->port_range[i].start_port > policy->port_range[i].end_port) {
				emcom_loge("error port");
				continue;
			}
			app->ports[app->port_num].protocol = policy->l4_protocol;
			app->ports[app->port_num].range.start_port = policy->port_range[i].start_port;
			app->ports[app->port_num].range.end_port = policy->port_range[i].end_port;

			app->port_num++;
			if (app->port_num >= EMCOM_MPFLOW_BIND_PORT_SIZE) {
				emcom_loge("too many port: %d", app->port_num);
				return;
			}
		}
	}
}

static bool emcom_mproute_need_reset(struct sock *sk, const uint16_t mark)
{
	if (sk->sk_mark == mark)
		return false;

	if (unlikely(!refcount_inc_not_zero(&sk->sk_refcnt)))
		return false;

	return true;
}

static void emcom_mproute_path_handover(struct sock *sk, uint16_t mark)
{
	if (!emcom_xengine_mpflow_ai_rehash_sk(sk))
		return;

	if (mark == INVALID_MARK) {
		emcom_loge("invalid mark");
		return;
	}

	emcom_logi("path_handover sk:%pK sport[%u] dev_if[%d] bind[%d] family[%u] mark[%u]",
		sk, sk->sk_num, sk->sk_bound_dev_if,
		sk->is_mp_flow_bind, sk->sk_family, mark);
	sk->sk_mark = (sk->sk_mark & MPROUTE_FWMARK_CONTROL_MASK) |
		(mark & MPROUTE_FWMARK_NEW_MASK);

	sk_dst_reset(sk);
}

static void emcom_mproute_update_app_info(const struct emcom_xengine_mpflow_ai_init_bind_cfg *config,
	const struct emcom_xengine_mpflow_ai_init_bind_policy *burst_policy)
{
	int8_t app_index;
	uint32_t i = 0;
	uint32_t j = 0;
	struct emcom_xengine_mpflow_ai_info *app = NULL;
	struct emcom_xengine_mpflow_ai_info* info = emcom_xengine_mpflow_ai_get_info();

	app_index = emcom_xengine_mpflow_ai_finduid(config->uid);
	if (app_index == INDEX_INVALID) {
		emcom_loge("get app fail, uid: %d", config->uid);
		return;
	}
	app = &info[app_index];
	app->burst_info.burst_port_num = 0;
	app->burst_info.burst_protocol = burst_policy->l4_protocol;
	app->burst_info.burst_mode = burst_policy->mode;

	for (;i < burst_policy->port_num; i++) {
		emcom_logi("burst: port range[%d, %d]", burst_policy->port_range[i].start_port,
			burst_policy->port_range[i].end_port);

		if (burst_policy->port_range[i].start_port > burst_policy->port_range[i].end_port) {
			emcom_loge("burst: invalid port range");
			continue;
		}
		app->burst_info.burst_ports[j].start_port = burst_policy->port_range[i].start_port;
		app->burst_info.burst_ports[j].end_port = burst_policy->port_range[i].end_port;
		j++;
	}
	app->burst_info.burst_port_num = j;
	app->burst_info.launch_path = 0;
	app->burst_info.burst_cnt = 0;
	app->burst_info.jiffies = 0;

	app->port_num = 0;
	app->running_state = MPROUTER_RUNNING;
	emcom_mproute_policy_init(config, app);
}

static void emcom_mproute_init_bind_config(const struct mproute_xengine_head *cfg, uint16_t len)
{
	struct emcom_xengine_mpflow_ai_init_bind_cfg *config;
	const struct emcom_xengine_mpflow_ai_init_bind_policy *burst_policy;
	spinlock_t lock;

	if (len != (sizeof(struct emcom_xengine_mpflow_ai_init_bind_cfg) + sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(struct emcom_xengine_mpflow_ai_init_bind_cfg), len);
		return;
	}

	config = (struct emcom_xengine_mpflow_ai_init_bind_cfg *)((char *)cfg + sizeof(struct mproute_xengine_head));
	if (!config || (cfg->len != sizeof(struct emcom_xengine_mpflow_ai_init_bind_cfg) +
		sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(struct emcom_xengine_mpflow_ai_init_bind_cfg) + sizeof(struct mproute_xengine_head), cfg->len);
		return;
	}

	if (!emcom_xengine_mpflow_ai_start(config->uid))
		return;

	if (config->policy_num > EMCOM_MPFLOW_BIND_PORT_CFG_SIZE) {
		emcom_loge("too many policy");
		return;
	}
	burst_policy = &config->burst_bind;
	emcom_logi("burst: proto[%u] mode[%u] portnum[%u]", burst_policy->l4_protocol, burst_policy->mode,
		burst_policy->port_num);

	if (burst_policy->l4_protocol != 0) {
		if ((burst_policy->l4_protocol != IPPROTO_TCP) && (burst_policy->l4_protocol != IPPROTO_UDP)) {
			emcom_loge("burst: invalid protocol: %d", burst_policy->l4_protocol);
			return;
		}
	}

	if (burst_policy->mode > EMCOM_MPFLOW_IP_BURST_FIX) {
		emcom_loge("burst: invalid mode: %d", burst_policy->mode);
		return;
	}

	lock = emcom_xengine_mpflow_ai_get_lock();
	spin_lock_bh(&lock);
	emcom_mproute_update_app_info(config, burst_policy);
	spin_unlock_bh(&lock);
}

int emcom_mproute_getmark(int8_t index, uid_t uid, struct sockaddr *uaddr, uint8_t proto)
{
	struct emcom_xengine_mpflow_ai_info* info = emcom_xengine_mpflow_ai_get_info();
	struct emcom_xengine_mpflow_ai_info *app = &info[index];
	__be32 daddr[EMCOM_MPFLOW_AI_CLAT_IPV6] = {0};
	uint16_t dport = 0;
	uint8_t port_index;
	struct emcom_xengine_mpflow_ai_burst_port *burst_info = &info[index].burst_info;

	/* apply all flow reset first */
	if (app->rst_all_flow) {
		emcom_logi("all_flow_mark %u", app->all_flow_mark);
		return app->all_flow_mark;
	}

	if (g_mproute_policy.mark_count == 0) {
		emcom_loge("mark_count zero");
		return INVALID_MARK;
	}
	/* apply reset mode second */
	if (time_after(jiffies, app->rst_jiffies + app->rst_duration)) {
		app->rst_mark = INVALID_MARK;
	} else {
		emcom_logi("rst_mark %u", app->rst_mark);
		return app->rst_mark;
	}

	if (!emcom_xengine_mpflow_ai_get_addr_port(uaddr, daddr, &dport)) {
		emcom_loge("no port");
		return INVALID_MARK;
	}

	port_index = emcom_xengine_mpflow_ai_get_port_index_in_range(index, dport, proto);
	if (port_index >= EMCOM_MPFLOW_BIND_PORT_SIZE) {
		emcom_loge("no port match");
		return INVALID_MARK;
	}

	if (burst_info->burst_mode != EMCOM_MPFLOW_IP_BURST_FIX) {
		emcom_logd("not download app");
		return g_mproute_policy.best_mark;  // not download app, new flow bind to best mark
	}

	return emcom_mproute_bind_burst(index, dport, proto, burst_info);
}

int emcom_mproute_bind_burst(int8_t index, uint16_t dport, uint8_t proto,
	struct emcom_xengine_mpflow_ai_burst_port *burst_info)
{
	uint16_t qos_path = g_mproute_policy.best_mark;
	uint8_t launch_cnt;
	int burst_path;

	if (g_mproute_policy.mark_count < LEAST_BURST_PATH_NUM)
		return qos_path;

	if (emcom_xengine_mpflow_ai_in_busrt_range(index, dport, proto)) {
		if (time_after(jiffies, (burst_info->jiffies + EMCOM_MPFLOW_FLOW_BIND_BURST_TIME)) ||
			(burst_info->jiffies == 0)) {
			burst_path = g_mproute_policy.mark[0];
			burst_info->burst_cnt = 0;
			burst_info->jiffies = jiffies;
		} else {
			burst_info->burst_cnt++;
			launch_cnt = burst_info->burst_cnt % g_mproute_policy.mark_count;
			if (launch_cnt >= EMCOM_MPROUTER_MAX_MARK) {
				emcom_loge("launch_cnt %u", launch_cnt);
				return INVALID_MARK;
			}
			emcom_logd("launch_cnt %u", launch_cnt);
			if (g_mproute_policy.mark_count > launch_cnt) {
				burst_path = g_mproute_policy.mark[launch_cnt];
			} else {
				emcom_loge("launch_cnt %u markCount %u", launch_cnt, g_mproute_policy.mark_count);
				return INVALID_MARK;
			}
		}
		emcom_logi("burst_path %d, burst_info->burst_cnt %u markCount %u", burst_path, burst_info->burst_cnt,
			g_mproute_policy.mark_count);

		return burst_path;
	}
	emcom_logi("qos_path %u", qos_path);

	return qos_path;
}

static void emcom_mproute_tcp_flow_reset(const struct reset_mproute_policy_info *reset,
	bool *need_reset_device)
{
	struct sock *sk = NULL;
	if (reset->flow.l3proto == ETH_P_IP) {
		sk = inet_lookup_established(&init_net, &tcp_hashinfo,
			reset->flow.ipv4_dip, htons(reset->flow.dst_port),
			reset->flow.ipv4_sip, htons(reset->flow.src_port),
			reset->flow.sk_dev_itf);
	} else {
		sk = __inet6_lookup_established(&init_net, &tcp_hashinfo,
			&reset->flow.ipv6_dip, htons(reset->flow.dst_port),
			&reset->flow.ipv6_sip, reset->flow.src_port,
			reset->flow.sk_dev_itf, 0);
	}
	if (!sk) {
		emcom_loge("tcp reset flow not found");
		return;
	}

	emcom_logi("reset sk:%pK sport[%u], state[%u] bind[%d] act[%u] family[%u] protocol[%u]",
		sk, sk->sk_num, sk->sk_state, sk->is_mp_flow_bind, reset->policy.act, sk->sk_family, sk->sk_protocol);
	if (reset->policy.act == SK_ERROR) {
		emcom_xengine_mpflow_reset(tcp_sk(sk));
		*need_reset_device = true;
		emcom_logi("tcp reset completed");
	} else {
		emcom_loge("tcp reset action not support, action: %u", reset->policy.act);
	}
	sock_put(sk);
}

static void emcom_mproute_udp_flow_reset(const struct reset_mproute_policy_info *reset,
	bool *need_reset_device)
{
	struct sock *sk = NULL;
	if (reset->flow.l3proto == ETH_P_IP) {
		sk = udp4_lib_lookup(&init_net,
			reset->flow.ipv4_dip, htons(reset->flow.dst_port),
			reset->flow.ipv4_sip, htons(reset->flow.src_port),
			reset->flow.sk_dev_itf);
	} else {
		sk = udp6_lib_lookup(&init_net,
			&reset->flow.ipv6_dip, htons(reset->flow.dst_port),
			&reset->flow.ipv6_sip, htons(reset->flow.src_port),
			reset->flow.sk_dev_itf);
	}

	if (sk) {
		emcom_logi("reset sk:%pK sport[%u], state[%u] bind[%d] act[%u] family[%u] protocol[%u]",
			sk, sk->sk_num, sk->sk_state, sk->is_mp_flow_bind, reset->policy.act, sk->sk_family, sk->sk_protocol);
		if (sk->sk_protocol == IPPROTO_UDP) {
			if (reset->policy.act == SK_ERROR) {
				emcom_xengine_mpflow_ai_udp_reset(sk);
				*need_reset_device = true;
			} else if (reset->policy.act == INTF_CHANGE) {
				emcom_mproute_path_handover(sk, reset->policy.rst_mark);
			} else {
				emcom_loge("udp reset action not support, action: %u", reset->policy.act);
			}
			emcom_logi("udp reset completed");
		}
		sock_put(sk);
		return;
	}
	emcom_loge("udp flow not found");
}

static void emcom_mproute_update_reset_info(struct emcom_xengine_mpflow_ai_info *app,
	const struct reset_mproute_policy_info *reset_policy)
{
	spinlock_t lock = emcom_xengine_mpflow_ai_get_lock();

	spin_lock_bh(&lock);
	app->rst_jiffies = jiffies;
	app->rst_mark = reset_policy->policy.rst_mark;
	app->rst_duration = msecs_to_jiffies(reset_policy->policy.const_perid);
	app->rst_devif = reset_policy->flow.sk_dev_itf;
	spin_unlock_bh(&lock);
	if (reset_policy->flow.l3proto == ETH_P_IP) {
		emcom_logi("[MPFlow_KERNEL]Reset Completed. SrcIP["IPV4_FMT"] SrcPort[%u] "
			"DstIP["IPV4_FMT"] DstPort[%u] l4proto[%u] intf[%u]",
			ipv4_info(reset_policy->flow.ipv4_sip), reset_policy->flow.src_port,
			ipv4_info(reset_policy->flow.ipv4_dip), reset_policy->flow.dst_port,
			reset_policy->flow.l4proto, reset_policy->flow.sk_dev_itf);
	} else {
		emcom_logi("[MPFlow_KERNEL]Reset Completed. SrcIP["IPV6_FMT"] SrcPort[%u] "
			"DstIP["IPV6_FMT"] DstPort[%u] l4proto[%u] intf[%u]",
			ipv6_info(reset_policy->flow.ipv6_sip), reset_policy->flow.src_port,
			ipv6_info(reset_policy->flow.ipv6_dip), reset_policy->flow.dst_port,
			reset_policy->flow.l4proto, reset_policy->flow.sk_dev_itf);
	}
}

static void emcom_mproute_reset_single_flow(const struct mproute_xengine_head *cfg, uint16_t len)
{
	bool need_reset_device = false;
	int8_t app_index;
	struct emcom_xengine_mpflow_ai_info *app = NULL;
	struct emcom_xengine_mpflow_ai_info* info = NULL;
	struct reset_mproute_policy_info *reset_policy;
	spinlock_t lock;

	if (len != (sizeof(struct reset_mproute_policy_info) + sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(struct reset_mproute_policy_info), len);
		return;
	}

	reset_policy = (struct reset_mproute_policy_info *)((char *)cfg + sizeof(struct mproute_xengine_head));
	if (!reset_policy || (cfg->len != sizeof(struct reset_mproute_policy_info) +
		sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(struct reset_mproute_policy_info) + sizeof(struct mproute_xengine_head), cfg->len);
		return;
	}

	emcom_logi("uid:%u, act:%u, const_perid:%u, mark:%d",
	reset_policy->uid, reset_policy->policy.act, reset_policy->policy.const_perid, reset_policy->policy.rst_mark);

	lock = emcom_xengine_mpflow_ai_get_lock();
	info = emcom_xengine_mpflow_ai_get_info();
	spin_lock_bh(&lock);
	app_index = emcom_xengine_mpflow_ai_finduid(reset_policy->uid);
	if (app_index == INDEX_INVALID) {
		spin_unlock_bh(&lock);
		return;
	}
	app = &info[app_index];
	spin_unlock_bh(&lock);

	switch (reset_policy->flow.l4proto) {
	case IPPROTO_TCP:
		emcom_mproute_tcp_flow_reset(reset_policy, &need_reset_device);
		break;
	case IPPROTO_UDP:
		emcom_mproute_udp_flow_reset(reset_policy, &need_reset_device);
		break;
	default:
		emcom_logi("mpflow ai reset unsupport protocol: %d.\n", reset_policy->flow.l4proto);
		break;
	}

	if (need_reset_device)
		emcom_mproute_update_reset_info(app, reset_policy);
}

static void emcom_mproute_close_tcp_flow(const uint32_t uid, const uint16_t mark)
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
				uid_t sock_uid = sock_i_uid(sk).val;
				if (uid != sock_uid)
					continue;
				if (emcom_xengine_mpflow_ai_check_loop_sk(sk))
					continue;

				if (!emcom_mproute_need_reset(sk, mark)) {
					continue;
				}
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
	emcom_logi("reset tcp flow, uid: %u flow_num: %d", uid, accum);

	for (; idx < accum; idx++) {
		emcom_xengine_mpflow_reset(tcp_sk(sk_arr[idx]));
		sock_gen_put(sk_arr[idx]);
	}
}

static void emcom_mproute_close_udp_flow(const uint32_t uid, const uint16_t mark,
	const reset_act act)
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
				uid_t sock_uid = sock_i_uid(sk).val;
				if (uid != sock_uid) {
					continue;
				}
				if (emcom_xengine_mpflow_ai_check_loop_sk(sk))
					continue;

				if (!emcom_mproute_need_reset(sk, mark)) {
					continue;
				}
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
	emcom_logi("reset udp flow, uid: %u action: %u flow_num: %d",
		uid, act, accum);

	for (; idx < accum; idx++) {
		if (act == SK_ERROR) {
			emcom_xengine_mpflow_ai_udp_reset(sk_arr[idx]);
		} else if (act == INTF_CHANGE) {
			emcom_mproute_path_handover(sk_arr[idx], mark);
		} else {
			emcom_loge("close udp flow not support, action: %u", act);
		}
		sock_gen_put(sk_arr[idx]);
	}
}


void emcom_mproute_close_all_flow(const struct mproute_xengine_head *cfg, uint16_t len)
{
	uint32_t uid;
	int8_t app_index;
	reset_act act;
	spinlock_t lock;
	struct emcom_xengine_mpflow_ai_info *app = NULL;
	struct transfer_mproute_flow_info *reset_all_flow_policy;
	struct emcom_xengine_mpflow_ai_info* info;

	if (len != (sizeof(struct transfer_mproute_flow_info) + sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(struct transfer_mproute_flow_info) + sizeof(struct mproute_xengine_head), len);
		return;
	}

	lock = emcom_xengine_mpflow_ai_get_lock();
	info = emcom_xengine_mpflow_ai_get_info();
	reset_all_flow_policy = (struct transfer_mproute_flow_info *)((char *)cfg + sizeof(struct mproute_xengine_head));
	if (!reset_all_flow_policy || (cfg->len != sizeof(struct transfer_mproute_flow_info) +
		sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(struct reset_flow_policy_info) + sizeof(struct mproute_xengine_head), cfg->len);
		return;
	}

	uid = reset_all_flow_policy->uid;
	act = reset_all_flow_policy->policy.act;
	emcom_logi("reset all flow:uid[%u] rst_mark[%d] act[%d] time_ms[%u]",
		uid, reset_all_flow_policy->policy.rst_mark, act, reset_all_flow_policy->policy.const_perid);

	spin_lock_bh(&lock);
	app_index = emcom_xengine_mpflow_ai_finduid(uid);
	if (app_index == INDEX_INVALID) {
		spin_unlock_bh(&lock);
		return;
	}
	app = &info[app_index];
	if (!reset_all_flow_policy->policy.const_perid) {
		app->rst_all_flow = false;
		app->all_flow_duration = 0;
		emcom_logi("clear reset all flow");
		spin_unlock_bh(&lock);
		return;
	}
	app->all_flow_mark = reset_all_flow_policy->policy.rst_mark;
	app->rst_all_flow = true;
	app->all_flow_jiffies = jiffies;
	app->all_flow_duration = msecs_to_jiffies(reset_all_flow_policy->policy.const_perid);
	spin_unlock_bh(&lock);

	emcom_mproute_close_tcp_flow(uid, reset_all_flow_policy->policy.rst_mark);
	emcom_mproute_close_udp_flow(uid, reset_all_flow_policy->policy.rst_mark, act);
}

void emcom_mproute_stop(const struct mproute_xengine_head *cfg, uint16_t len)
{
	int8_t index;
	bool mpflow_ai_uids_empty = false;
	spinlock_t lock;
	int uid;
	errno_t err;

	err = memset_s(&g_mproute_policy, sizeof(g_mproute_policy), 0, sizeof(g_mproute_policy));
	if (err != EOK)
		emcom_logd("emcom_mproute_stop g_mproute_policy failed");

	emcom_mproute_set_default_mark(0);

	if (len != (sizeof(int) + sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u", sizeof(int), len);
		return;
	}

	uid = *((int *)((char *)cfg + sizeof(struct mproute_xengine_head)));
	lock = emcom_xengine_mpflow_ai_get_lock();
	emcom_logd("mproute stop uid: %u", uid);

	emcom_mproute_close_tcp_flow(uid, emcom_mproute_get_default_mark());
	emcom_mproute_close_udp_flow(uid, emcom_mproute_get_default_mark(), INTF_CHANGE);

	spin_lock_bh(&lock);
	index = emcom_xengine_mpflow_ai_finduid(uid);
	if (index != INDEX_INVALID)
		emcom_xengine_mpflow_ai_app_clear(index, uid);

	emcom_xengine_mpflow_delete(uid, EMCOM_MPROUTE_VER_V1);
	mpflow_ai_uids_empty = emcom_xengine_mpflow_ai_uid_empty();
	spin_unlock_bh(&lock);

	if (mpflow_ai_uids_empty)
		emcom_xengine_mpflow_unregister_nf_hook(EMCOM_MPFLOW_VER_V2);
}

mproute_policy *emcom_mproute_get_policy(void)
{
	return &g_mproute_policy;
}

static void emcom_mproute_update_policy(const struct mproute_xengine_head *cfg, uint16_t len)
{
	mproute_policy *policy_cfg;

	if (len != (sizeof(mproute_policy) + sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u",
			sizeof(mproute_policy) + sizeof(struct mproute_xengine_head), len);
		return;
	}
	policy_cfg = (mproute_policy *)((char *)cfg + sizeof(struct mproute_xengine_head));
	spin_lock_bh(&g_mproute_lock);
	memcpy_s(&g_mproute_policy, sizeof(mproute_policy), policy_cfg, sizeof(mproute_policy));
	spin_unlock_bh(&g_mproute_lock);

	emcom_logd("g_mproute_policy markCount:%d, bestMark:%d, mark0:%d, mark1:%d, mark2:%d, mark3:%d",
		g_mproute_policy.mark_count, g_mproute_policy.best_mark, g_mproute_policy.mark[0], g_mproute_policy.mark[1],
		g_mproute_policy.mark[2], g_mproute_policy.mark[3]);
}

static void emcom_mproute_clear_policy(const struct mproute_xengine_head *cfg, uint16_t len)
{
	int uid;

	if (len != (sizeof(int) + sizeof(struct mproute_xengine_head))) {
		emcom_loge("input length error expect: %zu, real: %u", sizeof(int), len);
		return;
	}

	uid = *((int *)((char *)cfg + sizeof(struct mproute_xengine_head)));
	emcom_logd("g_mproute_policy clear uid:%d", uid);
	spin_lock_bh(&g_mproute_lock);
	g_mproute_policy.best_mark = INVALID_MARK;
	g_mproute_policy.mark_count = 0;
	memset_s(g_mproute_policy.mark, sizeof(int32_t) * EMCOM_MPROUTER_MAX_MARK, 0,
		sizeof(int32_t) * EMCOM_MPROUTER_MAX_MARK);
	spin_unlock_bh(&g_mproute_lock);
}

static void emcom_mproute_update_network_info(const struct mproute_xengine_head *cfg, uint16_t len)
{
	emcom_xengine_update_network_info((char *)cfg + sizeof(struct mproute_xengine_head),
		len - sizeof(struct mproute_xengine_head));
}

void emcom_mproute_init(void)
{
	errno_t err;

	emcom_logd("mproute init");
	spin_lock_init(&g_mproute_lock);
	err = memset_s(&g_mproute_policy, sizeof(g_mproute_policy), 0, sizeof(g_mproute_policy));
	if (err != EOK)
		emcom_logd("emcom_mproute_init g_mproute_policy failed");

	emcom_mproute_set_default_mark(0);
}

void mproute_evt_process(const struct mproute_xengine_head *cfg, uint16_t len)
{
	switch (cfg->type) {
	case MPROUTE_UPDATE_POLICY:
		emcom_mproute_update_policy(cfg, len);
		break;
	case MPROUTE_CLEAR_POLICY:
		emcom_mproute_clear_policy(cfg, len);
		break;
	case MPROUTE_INIT_BIND_CONFIG:
		emcom_mproute_init_bind_config(cfg, len);
		break;
	case MPROUTE_RESET_FLOW:
		emcom_mproute_reset_single_flow(cfg, len);
		break;
	case MPROUTE_RESET_ALL_FLOW:
		emcom_mproute_close_all_flow(cfg, len);
		break;
	case MPROUTE_NOTIFY_NETWORK_INFO:
		emcom_mproute_update_network_info(cfg, len);
		break;
	case MPROUTE_STOP:
		emcom_mproute_stop(cfg, len);
		break;
	default:
		emcom_loge("receive a not FI message, type:%d", cfg->type);
		break;
	}
	return;
}

void emcom_mproute_evt_proc(int32_t event, const uint8_t *data, uint16_t len)
{
	const struct mproute_xengine_head *cfg;
	if (data == NULL || len < sizeof(struct mproute_xengine_head)) {
		emcom_loge("invalid data");
		return;
	}

	cfg = (struct mproute_xengine_head *)data;
	switch (event) {
#ifdef CONFIG_HW_NSTACK_FI
	case NETLINK_EMCOM_DK_MPROUTE_XENGINE:
		if (len != cfg->len) {
			emcom_loge("invalid data");
			return;
		}
		emcom_logi("emcom mproute receive msg type is %d.\n", event);
		mproute_evt_process(cfg, len);
	break;
#endif

	default:
		emcom_logi("emcom mproute unsupport packet, the type is %d.\n", event);
		break;
	}
}
EXPORT_SYMBOL(emcom_mproute_evt_proc);
