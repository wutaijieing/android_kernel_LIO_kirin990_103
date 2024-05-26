/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Contexthub ipc link.
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
#include <linux/delay.h>
#include <securec.h>
#include <securectype.h>
#include "iomcu_link_ipc.h"
#include "iomcu_link_ipc_shm.h"

#define MAX_TRANSID       0xFF
#define align_to_u32(x)   ((((x) + 3) >> 2))
#define MAX_WQ_ACTIVE     0x8

struct iomcu_link_ipc_notifier_work {
	struct work_struct worker;
	enum tcp_ver tcp_version;
	void *tcp_data;
	uint16_t tcp_data_len;
};

struct ipc_link_head {
	uint8_t prio      : 2;     // msg prio.
	uint8_t resp      : 1;     // need resp.
	uint8_t shmem     : 1;     // shmem.
	uint8_t version   : 4;     // version.
	uint8_t transid;           // transition id.
	uint16_t length;           // payload length(tcp_head + data).
};

struct ipc_link_shmem_head {
	struct ipc_link_head link_head;
	uint32_t shmem_offset;
};

struct iomcu_link_buffer_node {
	struct list_head list;
	unsigned long tcp_addr;
	unsigned long link_addr;
	unsigned int context;
	unsigned int shmem_offset;
	unsigned int tcp_len;
	uint8_t shmem;
};

struct iomcu_link_buffer_list {
	struct list_head head;
	unsigned long addr;
};

struct iomcu_link_ipc_priv {
	struct notifier_block iomcu_link_ipc_nb;
	link_ipc_notify_f link_notifier[TCP_MAX];
	atomic_t send_transid;                                  // transid for ap2iomcu
	atomic_t recv_transid;                                  // transid for iomcu2ap
	rproc_id_t ipc_ap_to_iom_mbx;                           // mailbox for ap2iomcu
	rproc_id_t ipc_iom_to_ap_mbx;                           // mailbox for iomcu2ap
	struct list_head buffer_list_head;
	spinlock_t slock;
	struct mutex tranid_mutex;
	struct workqueue_struct *notifier_wq;
};

static struct iomcu_link_ipc_priv g_iomcu_link_ipc_priv = {
	.send_transid = ATOMIC_INIT(0),
	.recv_transid = ATOMIC_INIT(0),
};

static void iomcu_link_ipc_notifier_handler(struct work_struct *work)
{
	struct iomcu_link_ipc_notifier_work *p = container_of(work, struct iomcu_link_ipc_notifier_work, worker);

	if (p->tcp_data == NULL) {
		pr_err("[%s]p->tcp_dat is null\n", __func__);
		kfree(p);
		return;
	}

	pr_info("[%s] [%lx]\n", __func__, (unsigned long)(p->tcp_data));

	uint32_t *temp = (uint32_t *)(p->tcp_data);
	pr_info("[%s] [0x%x] [0x%x]\n", __func__, temp[0], temp[1]);
	pr_info("[%s] version %x, length %x\n", __func__, p->tcp_version, p->tcp_data_len);

	if (g_iomcu_link_ipc_priv.link_notifier[p->tcp_version] != NULL)
		g_iomcu_link_ipc_priv.link_notifier[p->tcp_version](p->tcp_data, p->tcp_data_len);

	kfree(p->tcp_data);
	p->tcp_data = NULL;
	kfree(p);
}

static int iomcu_ipc_link_recv_notifier(struct notifier_block *nb, unsigned long len, void *msg)
{
	struct ipc_link_head *p_link_head;
	struct ipc_link_shmem_head *p_link_head_shmem;
	struct iomcu_link_ipc_notifier_work *notifier_work = NULL;
	void *tcp_data;

	if (len == 0 || msg == NULL) {
		pr_err("[%s]no msg\n", __func__);
		return 0;
	}

	if (atomic_read(&iom3_rec_state) == IOM3_RECOVERY_START) {
		pr_err("iom3 under recovery mode, ignore all recv data\n");
		return 0;
	}

	p_link_head = (struct ipc_link_head *)msg;

	uint32_t *temp = (uint32_t *)p_link_head;
	pr_info("[%s] [0x%x] [0x%x]\n", __func__, temp[0], temp[1]);

	if (p_link_head->version >= TCP_MAX) {
		pr_err("[%s] invalid tcp version %x.\n", __func__, p_link_head->version);
		return 0;
	}

	if (g_iomcu_link_ipc_priv.link_notifier[p_link_head->version] == NULL) {
		pr_err("[%s] tcp notifier not registered %x.\n", __func__, p_link_head->version);
		return 0;
	}

	// check transid
	atomic_inc_return(&g_iomcu_link_ipc_priv.recv_transid);
	atomic_set(&g_iomcu_link_ipc_priv.recv_transid, atomic_read(&g_iomcu_link_ipc_priv.recv_transid) % MAX_TRANSID);

	// transid not match, just print error
	if (p_link_head->transid != atomic_read(&g_iomcu_link_ipc_priv.recv_transid)) {
		pr_warn("[%s] invalid transid %x-%x, maybe some packet missed.\n",
			__func__, p_link_head->transid, atomic_read(&g_iomcu_link_ipc_priv.recv_transid));
		atomic_set(&g_iomcu_link_ipc_priv.recv_transid, p_link_head->transid);
	}

	pr_info("[%s]version[0x%x]len[%u]shmem[0x%x]\n",
		__func__, p_link_head->version, p_link_head->length, p_link_head->shmem);

	// flow control

	if (p_link_head->shmem != 0) {
		p_link_head_shmem = (struct ipc_link_shmem_head *)msg;
		tcp_data = ipc_shm_get_recv_buf(p_link_head_shmem->shmem_offset);
		if (tcp_data == NULL) {
			pr_err("[%s] shmem_get_data error\n", __func__);
			return 0;
		}
	} else {
		if (p_link_head->length + sizeof(struct ipc_link_head) > MAX_SEND_LEN) {
			pr_err("[%s] invalid pkt len %x\n", __func__, p_link_head->length);
			return 0;
		}

		tcp_data = kzalloc(p_link_head->length, GFP_ATOMIC);
		if (tcp_data == NULL) {
			pr_err("[%s] kzalloc notifier error\n", __func__);
			return 0;
		}

		(void)memcpy_s(tcp_data, p_link_head->length, (void *)(p_link_head + 1), p_link_head->length);
	}

	temp = tcp_data;
	pr_info("[%s] tcp [0x%x] [0x%x]\n", __func__, temp[0], temp[1]);

	notifier_work = kzalloc(sizeof(struct iomcu_link_ipc_notifier_work), GFP_ATOMIC);
	if (notifier_work == NULL) {
		pr_err("[%s] alloc notifier_work failed.\n", __func__);
		kfree(tcp_data);
		return 0;
	}

	notifier_work->tcp_data = tcp_data;
	notifier_work->tcp_data_len = p_link_head->length;
	notifier_work->tcp_version = p_link_head->version;

	INIT_WORK(&notifier_work->worker, iomcu_link_ipc_notifier_handler);
	queue_work(g_iomcu_link_ipc_priv.notifier_wq, &notifier_work->worker);

	return 0;
}

static int iomcu_ipc_link_recv_register(void)
{
	int ret;

	pr_info("----%s--->\n", __func__);

	g_iomcu_link_ipc_priv.iomcu_link_ipc_nb.next = NULL;
	g_iomcu_link_ipc_priv.iomcu_link_ipc_nb.notifier_call = iomcu_ipc_link_recv_notifier;

	/* register the rx notify callback */
	ret = RPROC_MONITOR_REGISTER(g_iomcu_link_ipc_priv.ipc_iom_to_ap_mbx, &g_iomcu_link_ipc_priv.iomcu_link_ipc_nb);
	if (ret != 0)
		pr_err("%s:RPROC_MONITOR_REGISTER failed!\n", __func__);

	return ret;
}

static void iomcu_ipc_link_recv_unregister(void)
{
	RPROC_MONITOR_UNREGISTER(g_iomcu_link_ipc_priv.ipc_iom_to_ap_mbx, &g_iomcu_link_ipc_priv.iomcu_link_ipc_nb);
}

static struct iomcu_link_buffer_node *iomcu_link_get_buffer_node(unsigned long tcp_addr)
{
	struct iomcu_link_buffer_node *pos = NULL;

	if (tcp_addr == 0) {
		pr_err("%s: invalid tcp addr.\n", __func__);
		return NULL;
	}

	list_for_each_entry(pos, &g_iomcu_link_ipc_priv.buffer_list_head, list) {
		if (pos->tcp_addr == tcp_addr)
			return pos;
	}

	return NULL;
}

int iomcu_link_ipc_send(uint8_t prio, enum tcp_ver version, void *data, uint16_t length)
{
	struct ipc_link_head *link_head;
	struct ipc_link_shmem_head shmem_head;
	struct iomcu_link_buffer_node *node;
	int state = get_iom3_power_state();
	int iom3_state = get_iom3_state();
	unsigned long flags = 0;
	int ret = 0;

	if (data == NULL) {
		pr_err("%s: data NULL!\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&g_iomcu_link_ipc_priv.slock, flags);

	node = iomcu_link_get_buffer_node((unsigned long)data);
	if (node == NULL) {
		pr_err("%s: tcp addr not match.\n", __func__);
		spin_unlock_irqrestore(&g_iomcu_link_ipc_priv.slock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&g_iomcu_link_ipc_priv.slock, flags);

	if (length > node->tcp_len) {
		pr_err("%s: too much than alloc %x, %x.\n", __func__, length, node->tcp_len);
		return -EINVAL;
	}

	if (node->shmem != 0)
		link_head = &shmem_head.link_head;
	else
		link_head = (struct ipc_link_head *)node->link_addr;

	mutex_lock(&g_iomcu_link_ipc_priv.tranid_mutex);
	link_head->prio = prio;
	link_head->resp = 0;
	link_head->version = version;
	link_head->transid = (uint8_t)atomic_inc_return(&g_iomcu_link_ipc_priv.send_transid);
	link_head->length = length;
	link_head->shmem = node->shmem;

	uint32_t *temp = (uint32_t *)link_head;
	pr_info("[%s] [0x%x] [0x%x]\n", __func__, temp[0], temp[1]);
	pr_info("[%s] lvl %d resp %d ver %d tranid %d shmem %d len %d, offset %x\n",
		__func__, link_head->prio, link_head->resp, link_head->version,
		link_head->transid, link_head->shmem,
		link_head->length, node->shmem_offset);

	if ((length + sizeof(struct ipc_link_head) <= MAX_SEND_LEN)) {
		ret = RPROC_SYNC_SEND(g_iomcu_link_ipc_priv.ipc_ap_to_iom_mbx,
				      (mbox_msg_t *)link_head,
				      align_to_u32(length + sizeof(struct ipc_link_head)),
				      NULL, 0);
	} else {
		shmem_head.shmem_offset = (uint32_t)node->shmem_offset;
		ret = RPROC_SYNC_SEND(g_iomcu_link_ipc_priv.ipc_ap_to_iom_mbx,
				      (mbox_msg_t *)link_head,
				      align_to_u32(sizeof(struct ipc_link_shmem_head)),
				      NULL, 0);
	}

	if (ret != 0) {
		pr_err("ap ipc err[%d], iom3_rec_state[%d]g_iom3_state[%d]iom3_power_state[%d]\n",
			ret, atomic_read(&iom3_rec_state), iom3_state, state);
		(uint8_t)atomic_dec_return(&g_iomcu_link_ipc_priv.send_transid);
		ret = -EINVAL;
	}

	mutex_unlock(&g_iomcu_link_ipc_priv.tranid_mutex);

	iomcu_link_ipc_put_buffer((unsigned long)data, false);
	return ret;
}

int iomcu_link_ipc_recv_register(enum tcp_ver version, link_ipc_notify_f notify)
{
	if (version >= TCP_MAX) {
		pr_err("%s:invalid version %x\n", __func__, (uint32_t)version);
		return -EINVAL;
	}

	pr_info("%s:iomcu_link_ipc_recv_register %x\n", __func__, (uint32_t)version);

	g_iomcu_link_ipc_priv.link_notifier[version] = notify;

	return 0;
}

int iomcu_link_ipc_recv_unregister(enum tcp_ver version, link_ipc_notify_f notify)
{
	if (version >= TCP_MAX) {
		pr_err("%s:invalid version %x\n", __func__, (uint32_t)version);
		return -EINVAL;
	}

	g_iomcu_link_ipc_priv.link_notifier[version] = NULL;

	return 0;
}

unsigned long iomcu_link_ipc_get_buffer(uint16_t size, uint32_t context)
{
	uint32_t pkt_len = size + sizeof(struct ipc_link_head);
	struct iomcu_link_buffer_node *buffer_node;
	unsigned long flags = 0;

	buffer_node = kzalloc(sizeof(struct iomcu_link_buffer_node), GFP_ATOMIC);
	if (buffer_node == NULL) {
		pr_err("%s: alloc buffer node failed %x\n", __func__, size);
		return 0;
	}

	buffer_node->context = context;
	if (pkt_len > MAX_SEND_LEN) {
		buffer_node->tcp_addr = ipc_shm_alloc(size, context, &buffer_node->shmem_offset);
		buffer_node->link_addr = 0;
		buffer_node->shmem = 1;

		if (buffer_node->tcp_addr == 0) {
			pr_err("%s: alloc tcp buffer failed %x.\n", __func__, size);
			kfree(buffer_node);
			return 0;
		}
	} else {
		buffer_node->link_addr = kzalloc(pkt_len, GFP_ATOMIC);
		buffer_node->tcp_addr = buffer_node->link_addr + sizeof(struct ipc_link_head);
		buffer_node->shmem = 0;

		if (buffer_node->link_addr == 0) {
			pr_err("%s: alloc link buffer failed %x.\n", __func__, pkt_len);
			kfree(buffer_node);
			return 0;
		}
	}
	buffer_node->tcp_len = size;

	spin_lock_irqsave(&g_iomcu_link_ipc_priv.slock, flags);
	list_add(&buffer_node->list, &g_iomcu_link_ipc_priv.buffer_list_head);
	spin_unlock_irqrestore(&g_iomcu_link_ipc_priv.slock, flags);

	return buffer_node->tcp_addr;
}

// is_force
//	TRUE : free node and shm
//	FALSE: only free node, shm will be by sensorhub(set flag).
void iomcu_link_ipc_put_buffer(unsigned long tcp_addr, bool is_force)
{
	unsigned long flags = 0;
	struct iomcu_link_buffer_node *node = NULL;

	if (tcp_addr == 0) {
		pr_err("%s: invalid tcp addr.\n", __func__);
		return;
	}

	spin_lock_irqsave(&g_iomcu_link_ipc_priv.slock, flags);
	node = iomcu_link_get_buffer_node(tcp_addr);
	if (node == NULL) {
		pr_err("%s: tcp addr not match.\n", __func__);
		spin_unlock_irqrestore(&g_iomcu_link_ipc_priv.slock, flags);
		return;
	}

	if (node->shmem != 0) {
		if (is_force)
			ipc_shm_free(node->tcp_addr);
	} else {
		if (node->link_addr != 0)
			kfree(node->link_addr);
	}

	list_del(&node->list);
	kfree(node);

	spin_unlock_irqrestore(&g_iomcu_link_ipc_priv.slock, flags);
}

static int iomcu_link_ipc_init(void)
{
	int ret;

	(void)memset_s(&g_iomcu_link_ipc_priv, sizeof(g_iomcu_link_ipc_priv), 0, sizeof(g_iomcu_link_ipc_priv));

	g_iomcu_link_ipc_priv.ipc_ap_to_iom_mbx = IPC_AO_ACPU_IOM3_MBX1;
	g_iomcu_link_ipc_priv.ipc_iom_to_ap_mbx = IPC_AO_IOM3_ACPU_MBX1;

	ret = iomcu_ipc_link_recv_register();
	if (ret != 0)
		pr_err("ipc iomcu_ipc_link_recv_register faild\n");

	INIT_LIST_HEAD(&g_iomcu_link_ipc_priv.buffer_list_head);
	spin_lock_init(&g_iomcu_link_ipc_priv.slock);
	mutex_init(&g_iomcu_link_ipc_priv.tranid_mutex);

	g_iomcu_link_ipc_priv.notifier_wq = create_workqueue("iomcu_link_tcp");
	workqueue_set_max_active(g_iomcu_link_ipc_priv.notifier_wq, MAX_WQ_ACTIVE);

	atomic_set(&g_iomcu_link_ipc_priv.send_transid, 0);
	atomic_set(&g_iomcu_link_ipc_priv.recv_transid, 0);

	pr_info("[%s] done...\n", __func__);

	return 0;
}

static void __exit iomcu_link_ipc_exit(void)
{
	iomcu_ipc_link_recv_unregister();

	pr_info("[%s] done...\n", __func__);
}

device_initcall(iomcu_link_ipc_init);
module_exit(iomcu_link_ipc_exit);

MODULE_DESCRIPTION("iomcu ipc-link layer");
MODULE_LICENSE("GPL");