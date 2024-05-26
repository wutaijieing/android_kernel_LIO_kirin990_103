/*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
* Description: This module is a map utils for FI
* Author: songqiubin songqiubin@huawei.com
* Create: 2019-09-18
*/

#include "fi_utils.h"
#include <linux/stddef.h>
#include <linux/string.h>
#include "fi.h"

struct sk_buff *fi_get_netlink_skb(int type, int len, char **data)
{
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *pskb_out = NULL;

	pskb_out = nlmsg_new(len, GFP_ATOMIC);
	if (pskb_out == NULL) {
		fi_loge("Out of memry");
		return NULL;
	}
	nlh = nlmsg_put(pskb_out, 0, 0, type, len, 0);
	if (nlh == NULL) {
		kfree_skb(pskb_out);
		return NULL;
	}
	*data = nlmsg_data(nlh);
	return pskb_out;
}

void fi_update_netlink_type(struct nlmsghdr *nlh, uint16_t service)
{
	if (nlh == NULL) {
		fi_loge("nlh is null pointer");
		return;
	}

	if (service == FI_MPROUTE_SERVICE)
		nlh->nlmsg_type |= FI_NL_MSG_TYPE_MASK;
}

void fi_enqueue_netlink_skb(struct sk_buff *pskb)
{
	if (skb_queue_len(&g_fi_ctx.skb_queue) > FI_NL_SKB_QUEUE_MAXLEN) {
		fi_logi("skb_queue len too many, discard the skb");
		kfree_skb(pskb);
		return;
	}
	NETLINK_CB(pskb).dst_group = 0; /* For unicast */ /*lint !e545*/
	skb_queue_tail(&g_fi_ctx.skb_queue, pskb);
	up(&g_fi_ctx.netlink_sync_sema);
}

int fi_netlink_thread(void *data)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	uint32_t netlink_pid = 0;

	while (1) {
		if (kthread_should_stop())
			break;
		/* netlink thread will block at this semaphone when no data coming. */
		down(&g_fi_ctx.netlink_sync_sema);
		fi_logd("fi_netlink_thread get sema success!");
		do {
			skb = skb_dequeue(&g_fi_ctx.skb_queue);
			if (skb != NULL) {
				nlh = nlmsg_hdr(skb);
				if (g_fi_ctx.mproute_pid &&
					(((nlh->nlmsg_type & FI_NL_MSG_TYPE_MASK) >> FI_NL_MSG_TYPE_MASK_LEN) == FI_MPROUTE_SERVICE))
					netlink_pid = g_fi_ctx.mproute_pid;
				else if (g_fi_ctx.nl_pid)
					netlink_pid = g_fi_ctx.nl_pid;

				if (netlink_pid) {
					nlh->nlmsg_type &= ~FI_NL_MSG_TYPE_MASK;
					netlink_unicast(g_fi_ctx.nlfd, skb, netlink_pid, MSG_DONTWAIT);
					netlink_pid = 0;
				} else {
					kfree_skb(skb);
				}
			}
		} while (!skb_queue_empty(&g_fi_ctx.skb_queue));
	}
	return 0;
}

void fi_empty_netlink_skb_queue(void)
{
	struct sk_buff *skb = NULL;
	while (!skb_queue_empty(&g_fi_ctx.skb_queue)) {
		skb = skb_dequeue(&g_fi_ctx.skb_queue);
		if (skb != NULL)
			kfree_skb(skb);
	}
}

void fi_send_mproute_msg2daemon(const void *data, int len)
{
	char *nl_data = NULL;
	struct sk_buff * pskb = NULL;
	struct nlmsghdr *nlh = NULL;

	pskb = fi_get_netlink_skb(FI_MPROUTE_KD_RPT, len, &nl_data);
	if (pskb == NULL) {
		fi_loge("failed to malloc memory for data");
		return;
	}

	nlh = nlmsg_hdr(pskb);
	// update netlink type for mproute
	fi_update_netlink_type(nlh, FI_MPROUTE_SERVICE);

	if (data && (len > 0)) {
		if (memcpy_s((void *)nlmsg_data(nlh), len, data, len) != EOK) {
			fi_loge("failed to memcpy data to netlink msg");
			kfree_skb(pskb);
			return;
		}
	}

	fi_enqueue_netlink_skb(pskb);
	fi_logd("finish to send a message to native");
	return;
}

