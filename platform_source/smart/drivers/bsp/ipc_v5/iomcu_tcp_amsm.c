/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Contexthub ipc tcp amsm.
 * Create: 2021-10-14
 */
#include "common/common.h"
#include <linux/completion.h>
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include <platform_include/smart/linux/iomcu_priv.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
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

#define IOMCU_IPC_RX_WAIT_TIMEOUT 2000
#define NS_TO_MS                  1000000

struct iomcu_tcp_amsm_notifier_node {
	int (*notify)(const struct pkt_header *data);
	struct list_head entry;
	uint8_t tag;
	uint8_t cmd;
};

struct iomcu_tcp_amsm_notifier {
	struct list_head head;
	spinlock_t lock;
};

struct tcp_head_amsm {
	uint8_t tag;
	uint8_t cmd;
	uint16_t resv;
	uint32_t resv2;
};

struct iomcu_tcp_amsm_mntn {
	atomic64_t head_max_latency_msecs;
	struct tcp_head_amsm head;
};

static int iomcu_tcp_amsm_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar);
static struct iomcu_tcp_amsm_notifier tcp_amsm_notifier = {LIST_HEAD_INIT(tcp_amsm_notifier.head)};
static struct iomcu_tcp_amsm_mntn tcp_amsm_mntm;
static struct notifier_block tcp_amsm_notify = {
	.notifier_call = iomcu_tcp_amsm_recovery_notifier,
	.priority = -1,
};

static void iomcu_tcp_amsm_latency_info(void)
{
	pr_info("head_max_latency_msecs:tag %x, cmd %x, %llu ms.\n",
		tcp_amsm_mntm.head.tag, tcp_amsm_mntm.head.cmd,
		(u64)atomic64_read(&tcp_amsm_mntm.head_max_latency_msecs));
}

static int iomcu_tcp_amsm_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar)
{
	pr_info("%s +\n", __func__);
	switch (foo) {
	case IOM3_RECOVERY_START:
		iomcu_tcp_amsm_latency_info();
	break;
	default: break;
	}
	pr_info("%s -\n", __func__);
	return 0;
}

static void iomcu_tcp_amsm_notifier_handler(int *pkt_data, uint16_t pke_len)
{
	struct iomcu_tcp_amsm_notifier_node *pos = NULL;
	struct iomcu_tcp_amsm_notifier_node *n = NULL;
	unsigned long flags = 0;
	const struct pkt_header *head = (struct pkt_header *)pkt_data;
	int *info = (int *)head;
	u64 entry_jiffies;
	u64 exit_jiffies;
	u64 diff_msecs;

	pr_info("[%s] [0x%x,0x%x]\n", __func__, info[0], info[1]);
	spin_lock_irqsave(&tcp_amsm_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &tcp_amsm_notifier.head, entry) {
		if (pos->tag != head->tag)
			continue;

		if (!(pos->cmd == head->cmd || pos->cmd == CMD_ALL))
			continue;

		if (pos->notify == NULL)
			continue;

		spin_unlock_irqrestore(&tcp_amsm_notifier.lock, flags);

		entry_jiffies = get_jiffies_64();
		pos->notify((const struct pkt_header *)pkt_data);
		exit_jiffies = get_jiffies_64();

		diff_msecs = jiffies64_to_nsecs(exit_jiffies - entry_jiffies) / NS_TO_MS;
		if (unlikely(diff_msecs > (u64)atomic64_read(&tcp_amsm_mntm.head_max_latency_msecs))) {
			atomic64_set(&tcp_amsm_mntm.head_max_latency_msecs, (s64)diff_msecs);

			tcp_amsm_mntm.head.tag = head->tag;
			tcp_amsm_mntm.head.cmd = head->cmd;
			pr_warn("iomcu head_max_latency_msecs 0x%x 0x%x %llums\n", head->tag, head->cmd, diff_msecs);
		}

		spin_lock_irqsave(&tcp_amsm_notifier.lock, flags);
	}
	spin_unlock_irqrestore(&tcp_amsm_notifier.lock, flags);
}

static int iomc_tcp_amsm_recv_handler(const void *buf, const uint16_t len)
{
	const struct tcp_head_amsm *head = (const struct tcp_head_amsm *)buf;
	struct pkt_header *pkt_head = NULL;
	struct iomcu_tcp_amsm_notifier_node *pos = NULL;
	struct iomcu_tcp_amsm_notifier_node *n = NULL;
	unsigned long flags = 0;
	uint32_t pkt_len;
	bool is_notifier = false;

	if (head == NULL || len < sizeof(struct tcp_head_amsm)) {
		pr_err("[%s] para err len %x.\n", __func__, len);
		return 0;
	}

	pr_info("[%s]tag[0x%x]cmd[0x%x]len[%u]tran[0x%x]\n", __func__, head->tag, head->cmd, len);

	spin_lock_irqsave(&tcp_amsm_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &tcp_amsm_notifier.head, entry) {
		if (pos->tag != head->tag)
			continue;

		if ((pos->cmd != head->cmd) && (pos->cmd != CMD_ALL))
			continue;

		is_notifier = true;
		break;
	}
	spin_unlock_irqrestore(&tcp_amsm_notifier.lock, flags);

	if (!is_notifier) {
		pr_err("[%s] unregistered cmd %x, tag %x.\n", __func__, head->tag, head->cmd);
		return 0;
	}

	pkt_len = len + sizeof(struct pkt_header) - sizeof(struct tcp_head_amsm);
	pkt_head = (struct pkt_header *)kzalloc(pkt_len, GFP_ATOMIC);
	if (pkt_head == NULL) {
		pr_err("[%s] kzalloc notifier error\n", __func__);
		return 0;
	}

	// set pkt_header to adpater legacy app.
	pkt_head->cmd = head->cmd;
	pkt_head->tag = head->tag;
	pkt_head->length = len - sizeof(struct tcp_head_amsm);

	// copy app-payload.
	if (len > sizeof(struct tcp_head_amsm))
		(void)memcpy_s((uint8_t *)(&pkt_head[1]),
			       len - sizeof(struct tcp_head_amsm),
			       (void *)&head[1], len - sizeof(struct tcp_head_amsm));

	// maybe pm register to ipc?
	inputhub_pm_callback((void *)pkt_head);

	iomcu_tcp_amsm_notifier_handler((void *)pkt_head, pkt_len);

	kfree((void *)pkt_head);
	return 0;
}

static int write_customize_cmd_para_check(const struct write_info *wr)
{
	if (wr == NULL) {
		pr_err("NULL pointer in %s\n", __func__);
		return -EINVAL;
	}

	if (wr->tag < TAG_BEGIN || wr->tag >= TAG_END || wr->wr_len < 0) {
		pr_err("[%s]tag[0x%x]orLen[0x%x]err\n", __func__, wr->tag, wr->wr_len);
		return -EINVAL;
	}

	if (wr->wr_len != 0 && wr->wr_buf == NULL) {
		pr_err("tag = %d, wr_len %d  error in %s\n", wr->tag, wr->wr_len, __func__);
		return -EINVAL;
	}

	if (wr->wr_len > ipc_shm_get_capacity()) {
		pr_err("[%s] wr_len %d  Max Len %d error.\n", __func__, wr->wr_len, ipc_shm_get_capacity());
		return -EINVAL;
	}

	return 0;
}

int write_customize_cmd(const struct write_info *wr, struct read_info *rd, bool is_lock)
{
	int ret;
	unsigned int send_length;
	struct tcp_head_amsm *tcp_head;
	unsigned long tcp_addr;
	unsigned long flags = 0;

	if (get_contexthub_dts_status() != 0) {
		pr_err("[%s]do not send ipc as sensorhub is disabled\n", __func__);
		return -1;
	}

	ret = write_customize_cmd_para_check(wr);
	if (ret != 0)
		return ret;
	ret = iomcu_check_dump_status();
	if (ret != 0)
		return ret;

	pr_info("[%s] tag[0x%x] cmd[0x%x] len[%d]\n", __func__, wr->tag, wr->cmd, wr->wr_len);

	send_length = wr->wr_len + sizeof(struct tcp_head_amsm);
	tcp_addr = iomcu_link_ipc_get_buffer(send_length, (wr->cmd << 8) | wr->tag);
	if (tcp_addr == 0) {
		pr_err("[%s] get link buffer failed.\n", __func__);
		return -EINVAL;
	}

	tcp_head = (struct tcp_head_amsm*)tcp_addr;
	tcp_head->cmd = wr->cmd;
	tcp_head->tag = wr->tag;

	(void)memcpy_s((void *)(tcp_head + 1), wr->wr_len, wr->wr_buf, wr->wr_len);

	ret = iomcu_link_ipc_send(1, TCP_AMSM, (void *)tcp_addr, send_length);
	if (ret != 0) {
		pr_err("[%s] send tag[0x%x]cmd[0x%x] failed.\n", __func__, tcp_head->cmd, tcp_head->cmd);
		(void)iomcu_link_ipc_put_buffer(tcp_addr, true);
		return ret;
	}

	return 0;
}

int register_mcu_event_notifier(int tag, int cmd, int (*notify)(const struct pkt_header *head))
{
	struct iomcu_tcp_amsm_notifier_node *pnode = NULL;
	int ret = 0;
	unsigned long flags = 0;

	if ((!(tag >= TAG_BEGIN && tag < TAG_END)) || notify == NULL)
		return -EINVAL;

	spin_lock_irqsave(&tcp_amsm_notifier.lock, flags);
	list_for_each_entry(pnode, &tcp_amsm_notifier.head, entry) {
		if ((tag == pnode->tag) && (cmd == pnode->cmd) && (notify == pnode->notify)) {
				pr_warn("[%s] tag[0x%x]cmd[0x%x]notify[%pK] already registed\n!", __func__, tag, cmd, notify);
				goto out;
			}
	}

	pnode = kzalloc(sizeof(struct iomcu_tcp_amsm_notifier_node), GFP_ATOMIC);
	if (pnode == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	pnode->tag = tag;
	pnode->cmd = cmd;
	pnode->notify = notify;

	list_add(&pnode->entry, &tcp_amsm_notifier.head);
out:
	spin_unlock_irqrestore(&tcp_amsm_notifier.lock, flags);
	return ret;
}

int unregister_mcu_event_notifier(int tag, int cmd, int (*notify)(const struct pkt_header *head))
{
	struct iomcu_tcp_amsm_notifier_node *pos = NULL;
	struct iomcu_tcp_amsm_notifier_node *n = NULL;
	unsigned long flags = 0;

	if ((!(tag >= TAG_BEGIN && tag < TAG_END)) || notify == NULL)
		return -EINVAL;

	spin_lock_irqsave(&tcp_amsm_notifier.lock, flags);
	list_for_each_entry_safe(pos, n, &tcp_amsm_notifier.head, entry) {
		if ((tag == pos->tag) && (cmd == pos->cmd) && (notify == pos->notify)) {
			list_del(&pos->entry);
			kfree(pos);
			break;
		}
	}
	spin_unlock_irqrestore(&tcp_amsm_notifier.lock, flags);
	return 0;
}

static int iomcu_tcp_amsm_init(void)
{
	int ret;

	(void)memset_s(&tcp_amsm_mntm, sizeof(struct iomcu_tcp_amsm_mntn), 0, sizeof(struct iomcu_tcp_amsm_mntn));

	INIT_LIST_HEAD(&tcp_amsm_notifier.head);
	spin_lock_init(&tcp_amsm_notifier.lock);

	ret = iomcu_link_ipc_recv_register(TCP_AMSM, iomc_tcp_amsm_recv_handler);
	if (ret != 0)
		pr_err("[%s] iomcu_link_ipc_recv_register TCP_AMSM faild\n", __func__);

	ret = register_iom3_recovery_notifier(&tcp_amsm_notify);
	if (ret != 0)
		pr_err("[%s] register_iom3_recovery_notifier faild\n", __func__);

	return 0;
}

static void __exit iomcu_tcp_amsm_exit(void)
{
	iomcu_link_ipc_recv_unregister(TCP_AMSM, iomc_tcp_amsm_recv_handler);

	unregister_iom3_recovery_notifier(&tcp_amsm_notify);
}

device_initcall_sync(iomcu_tcp_amsm_init);
module_exit(iomcu_tcp_amsm_exit);

MODULE_DESCRIPTION("iomcu tcp layer - amsm");
MODULE_LICENSE("GPL");