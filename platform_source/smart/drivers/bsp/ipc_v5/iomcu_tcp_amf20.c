/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Contexthub ipc tcp amf20.
 * Create: 2021-10-14
 */
#include "common/common.h"
#include <linux/completion.h>
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_priv.h>
#include <linux/platform_drivers/ipc_rproc.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <securec.h>
#include <securectype.h>
#include "iomcu_link_ipc.h"
#include "iomcu_link_ipc_shm.h"

struct iomcu_tcp_amf20_notifier_node {
	tcp_amf20_notify_f notify;
	struct list_head entry;
	uint16_t svc_id;
	uint16_t cmd;
};

struct iomcu_tcp_amf20_notifier {
	struct list_head head;
	spinlock_t lock;
};

struct tcp_head_amf20 {
	uint16_t svc_id;
	uint16_t cmd;
};

struct iomcu_tcp_amf20_mntn {
	atomic64_t head_max_latency_msecs;
	struct tcp_head_amf20 head;
};

#define NS_TO_MS 1000000

static int iomcu_tcp_amf20_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar);

static struct iomcu_tcp_amf20_notifier tcp_amf20_notifier = {LIST_HEAD_INIT(tcp_amf20_notifier.head)};
static struct iomcu_tcp_amf20_mntn tcp_amf20_mntm;
static struct notifier_block tcp_amf20_notify = {
	.notifier_call = iomcu_tcp_amf20_recovery_notifier,
	.priority = -1,
};

static void iomcu_tcp_amf20_latency_info(void)
{
	pr_info("head_max_latency_msecs:svc_id %x, cmd %x, %llu ms.\n",
		tcp_amf20_mntm.head.svc_id, tcp_amf20_mntm.head.cmd,
		(u64)atomic64_read(&tcp_amf20_mntm.head_max_latency_msecs));
}

static int iomcu_tcp_amf20_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar)
{
	pr_info("%s +\n", __func__);
	switch (foo) {
	case IOM3_RECOVERY_START:
		iomcu_tcp_amf20_latency_info();
		break;
	default:
		break;
	}
	pr_info("%s -\n", __func__);
	return 0;
}

static int iomc_tcp_amf20_recv_handler(const void *tcp_data, const uint16_t tcp_data_len)
{
	const struct tcp_head_amf20 *head = (const struct tcp_head_amf20 *)tcp_data;
	struct iomcu_tcp_amf20_notifier_node *pos = NULL;
	struct iomcu_tcp_amf20_notifier_node *n = NULL;
	unsigned long flags = 0;
	bool is_notifier = false;
	u64 entry_jiffies;
	u64 exit_jiffies;
	u64 diff_msecs;

	if (head == NULL || tcp_data_len < sizeof(struct tcp_head_amf20)) {
		pr_err("[%s] para err len %x.\n", __func__, tcp_data_len);
		return 0;
	}

	pr_info("[%s] svc_id[0x%x] cmd[0x%x] len[%u]\n", __func__, head->svc_id, head->cmd, tcp_data_len);

	spin_lock_irqsave(&tcp_amf20_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &tcp_amf20_notifier.head, entry) {
		if (pos->svc_id != head->svc_id)
			continue;

		if ((pos->cmd != head->cmd) && (pos->cmd != AMF20_CMD_ALL))
			continue;

		if (pos->notify == NULL)
			continue;

		spin_unlock_irqrestore(&tcp_amf20_notifier.lock, flags);

		// to notify user data.
		entry_jiffies = get_jiffies_64();
		pos->notify(head->svc_id, head->cmd, (uintptr_t)&head[1], tcp_data_len - sizeof(struct tcp_head_amf20));
		exit_jiffies = get_jiffies_64();

		diff_msecs = jiffies64_to_nsecs(exit_jiffies - entry_jiffies) / NS_TO_MS;
		if (unlikely(diff_msecs > (u64)atomic64_read(&tcp_amf20_mntm.head_max_latency_msecs))) {
			atomic64_set(&tcp_amf20_mntm.head_max_latency_msecs, (s64)diff_msecs);
			(void)memcpy_s((void *)&tcp_amf20_mntm.head, sizeof(tcp_amf20_mntm.head), head, sizeof(struct tcp_head_amf20));
			pr_warn("[%s] [svc_id]0x%x [cmd]0x%x [cost]%llums\n", __func__, head->svc_id, head->cmd, diff_msecs);
		}

		is_notifier = true;

		spin_lock_irqsave(&tcp_amf20_notifier.lock, flags);
	}
	spin_unlock_irqrestore(&tcp_amf20_notifier.lock, flags);

	if (!is_notifier)
		pr_err("[%s] unregistered svc_id %x, cmd %x.\n", __func__, head->svc_id, head->cmd);

	return 0;
}

int tcp_send_data_by_amf20(const uint16_t svc_id, const uint16_t cmd, uintptr_t *data, uint16_t data_len)
{
	int ret;
	unsigned int send_length;
	struct tcp_head_amf20 *tcp_head;
	unsigned long tcp_addr;

	if (get_contexthub_dts_status() != 0) {
		pr_err("[%s]do not send ipc as sensorhub is disabled\n", __func__);
		return -1;
	}

	if (data == NULL) {
		pr_err("[%s] data NULL or data_len to large...\n", __func__);
		return -1;
	}

	if (data_len > ipc_shm_get_capacity()) {
		pr_err("[%s] data_len %d  Max Len %d error.\n", __func__, data_len, ipc_shm_get_capacity());
		return -EINVAL;
	}

	ret = iomcu_check_dump_status();
	if (ret != 0)
		return ret;

	pr_info("[%s]svc_id[0x%x]cmd[0x%x]len[%d]\n", __func__, svc_id, cmd, data_len);

	send_length = data_len + sizeof(struct tcp_head_amf20);
	tcp_addr = iomcu_link_ipc_get_buffer(send_length, (svc_id << 16) | cmd);
	if (tcp_addr == 0) {
		pr_err("[%s] get link buffer failed.\n", __func__);
		return -EINVAL;
	}

	tcp_head = (struct tcp_head_amf20*)tcp_addr;
	tcp_head->svc_id = svc_id;
	tcp_head->cmd = cmd;

	(void)memcpy_s((void *)(tcp_head + 1), data_len, data, data_len);

	ret = iomcu_link_ipc_send(1, TCP_AMF20, (void *)tcp_addr, send_length);
	if (ret != 0) {
		pr_err("[%s] send svc_id[0x%x]cmd[0x%x] failed.\n", __func__, tcp_head->svc_id, tcp_head->cmd);
		(void)iomcu_link_ipc_put_buffer(tcp_addr, true);
		return ret;
	}

	return 0;
}

int register_amf20_notifier(uint16_t svc_id, uint16_t cmd, tcp_amf20_notify_f notify)
{
	struct iomcu_tcp_amf20_notifier_node *pnode = NULL;
	int ret = 0;
	unsigned long flags = 0;

	if (notify == NULL)
		return -EINVAL;

	spin_lock_irqsave(&tcp_amf20_notifier.lock, flags);
	list_for_each_entry(pnode, &tcp_amf20_notifier.head, entry) {
		if ((svc_id == pnode->svc_id) && (cmd == pnode->cmd) && (notify == pnode->notify)) {
				pr_warn("[%s] svc_id[0x%x] cmd[0x%x] notify[%pK] already registed\n!", __func__, svc_id, cmd, notify);
				goto out;
			}
	}

	pnode = kzalloc(sizeof(struct iomcu_tcp_amf20_notifier_node), GFP_ATOMIC);
	if (pnode == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	pnode->svc_id = svc_id;
	pnode->cmd = cmd;
	pnode->notify = notify;

	list_add(&pnode->entry, &tcp_amf20_notifier.head);
out:
	spin_unlock_irqrestore(&tcp_amf20_notifier.lock, flags);
	return ret;
}

int unregister_amf20_notifier(uint16_t svc_id, uint16_t cmd, tcp_amf20_notify_f notify)
{
	struct iomcu_tcp_amf20_notifier_node *pos = NULL;
	struct iomcu_tcp_amf20_notifier_node *n = NULL;
	unsigned long flags = 0;

	if (notify == NULL)
		return -EINVAL;

	spin_lock_irqsave(&tcp_amf20_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &tcp_amf20_notifier.head, entry) {
		if ((svc_id == pos->svc_id) && (cmd == pos->cmd) && (notify == pos->notify)) {
			list_del(&pos->entry);
			kfree(pos);
			break;
		}
	}
	spin_unlock_irqrestore(&tcp_amf20_notifier.lock, flags);
	return 0;
}

static int iomcu_tcp_amf20_init(void)
{
	int ret;

	(void)memset_s(&tcp_amf20_mntm, sizeof(struct iomcu_tcp_amf20_mntn), 0, sizeof(struct iomcu_tcp_amf20_mntn));

	INIT_LIST_HEAD(&tcp_amf20_notifier.head);
	spin_lock_init(&tcp_amf20_notifier.lock);

	ret = iomcu_link_ipc_recv_register(TCP_AMF20, iomc_tcp_amf20_recv_handler);
	if (ret != 0)
		pr_err("[%s] iomcu_link_ipc_recv_register TCP_AMF20 faild\n", __func__);

	ret = register_iom3_recovery_notifier(&tcp_amf20_notify);
	if (ret != 0)
		pr_err("[%s] register_iom3_recovery_notifier faild\n", __func__);

	pr_info("[%s] done...\n", __func__);

	return 0;
}

static void __exit iomcu_tcp_amf20_exit(void)
{
	iomcu_link_ipc_recv_unregister(TCP_AMF20, iomc_tcp_amf20_recv_handler);

	unregister_iom3_recovery_notifier(&tcp_amf20_notify);

	pr_info("[%s] done...\n", __func__);

	return;
}

device_initcall_sync(iomcu_tcp_amf20_init);
module_exit(iomcu_tcp_amf20_exit);

MODULE_DESCRIPTION("iomcu tcp layer - amf20");
MODULE_LICENSE("GPL");