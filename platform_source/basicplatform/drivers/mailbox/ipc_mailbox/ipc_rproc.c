/*
 * ipc_rproc.c
 *
 * ipc rproc communication interfacer
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/arm-smccc.h>
#include <securec.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include "ipc_mailbox.h"
#include "ipc_rproc_inner.h"

/*lint -e580 */
#define PR_LOG_TAG AP_MAILBOX_TAG

#define rproc_pr_err(fmt, args...)   pr_err(fmt "\n", ##args)

#define rproc_pr_debug(fmt, args...) pr_debug(fmt "\n", ##args)

enum call_type_t {
	ASYNC_CALL = 0,
	SYNC_CALL
};

struct ipc_rproc_info {
	struct atomic_notifier_head notifier;
	struct notifier_block nb;
	struct ipc_mbox *mbox;
};

struct ipc_rproc_info_arr {
	struct ipc_rproc_info rproc_arr[IPC_RPROC_MAX_MBX_ID];
	unsigned int rproc_cnt;
};

static struct ipc_rproc_info_arr g_rproc_table;

#ifdef CONFIG_BSP_RESET_CORE_NOTIFY
enum BSP_CORE_TYPE_E {
	BSP_CCORE = 0,
	BSP_HIFI,
	BSP_LPM3,
	BSP_BBE,
	BSP_CDSP,
	BSP_CCORE_TAS,
	BSP_CCORE_WAS,
	BSP_CCORE_CAS,
	BSP_BUTT
};

#define BSP_RESET_NOTIFY_REPLY_OK 0
int bsp_reset_core_notify((enum BSP_CORE_TYPE_E) ecoretype,
	unsigned int cmdtype, unsigned int timeout_ms, unsigned int *retval)
{
	rproc_pr_err("stub func, %s didn't achieved", __func__);
	*retval = 0;
	return BSP_RESET_NOTIFY_REPLY_OK;
}
#endif

static struct ipc_rproc_info *find_rproc(rproc_id_t rproc_id)
{
	struct ipc_rproc_info *rproc = NULL;
	unsigned int i;

	for (i = 0; i < g_rproc_table.rproc_cnt; i++) {
		if (g_rproc_table.rproc_arr[i].mbox &&
			rproc_id == g_rproc_table.rproc_arr[i].mbox->rproc_id) {
			rproc = &g_rproc_table.rproc_arr[i];
			break;
		}
	}
	return rproc;
}

static int get_rproc_channel_size(struct ipc_rproc_info *rproc)
{
	struct ipc_mbox_device *mdev = NULL;

	mdev = rproc->mbox->tx ? rproc->mbox->tx : rproc->mbox->rx;
	if (!mdev)
		return -EINVAL;

	return mdev->ops->get_channel_size(mdev);
}

static int check_channel_size(struct ipc_rproc_info *rproc, int len)
{
	int mail_box_size;

	mail_box_size = get_rproc_channel_size(rproc);
	if (mail_box_size <= 0 || len > mail_box_size) {
		rproc_pr_err("len is invalid, %d:%d", mail_box_size, len);
		return -EINVAL;
	}

	return 0;
}

int ipc_rproc_xfer_async_with_ack(
	rproc_id_t rproc_id, rproc_msg_t *msg, rproc_msg_len_t len,
	void (*ack_handle)(rproc_msg_t *ack_buffer,
		rproc_msg_len_t ack_buffer_len))
{
	struct ipc_rproc_info *rproc = NULL;
	struct ipc_mbox_task *tx_task = NULL;
	struct ipc_mbox *mbox = NULL;
	int ret;
	u64 ts = ktime_get_ns();

	rproc = find_rproc(rproc_id);
	if (!rproc) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	if (check_channel_size(rproc, len))
		return -EINVAL;

	mbox = rproc->mbox;

	tx_task = ipc_mbox_task_alloc(mbox, msg, len, MANUAL_ACK);
	if (!tx_task) {
		rproc_pr_err("alloc task failed");
		return -EINVAL;
	}

	(void)memset_s(tx_task->ts, sizeof(u64) * 11, 0, sizeof(u64) * 11);
	tx_task->rproc_id = rproc_id;
	tx_task->ts[0] = ts;
	tx_task->ts[1] = ktime_get_ns();

	tx_task->ack_handle = ack_handle;
	ret = ipc_mbox_msg_send_async(mbox, tx_task);
	if (ret) {
		/* -12:tx_fifo full */
		rproc_pr_err(
			"%s async send failed, errno: %d", mbox->tx->name, ret);
		ipc_mbox_task_free(&tx_task);
	}

	return ret;
}
EXPORT_SYMBOL(ipc_rproc_xfer_async_with_ack);

static int ipc_xfer_sync_to_host(rproc_id_t rproc_id, rproc_msg_t *msg,
	rproc_msg_len_t len, rproc_msg_t *ack_buffer,
	rproc_msg_len_t ack_buffer_len)
{
#ifndef CONFIG_LIBLINUX
	int ret;
	struct arm_smccc_res res;
	rproc_msg_t tx_buffer[MBOX_CHAN_DATA_SIZE] = {0};
	rproc_msg_t _ack_buffer[MBOX_CHAN_DATA_SIZE] = {0};
	unsigned long msg_phy = virt_to_phys(tx_buffer);
	unsigned long ack_phy = virt_to_phys(_ack_buffer);

	if(len > MBOX_CHAN_DATA_SIZE || ack_buffer_len > MBOX_CHAN_DATA_SIZE) {
		rproc_pr_err("%s param error len %d, ack len %d\n", __func__, len, ack_buffer_len);
		return -EINVAL;
	}

	ret = memcpy_s(tx_buffer, sizeof(rproc_msg_t) / sizeof(char) * MBOX_CHAN_DATA_SIZE, \
		msg, sizeof(rproc_msg_t) * len);
	if (ret != EOK) {
		rproc_pr_err("%s: memcpy_s failed", __func__);
		return -EINVAL;
	}

	arm_smccc_hvc(IPC_SEND_SYNC, rproc_id, msg_phy, len, \
		ack_phy, ack_buffer_len, 0, 0, &res);

	if (ack_buffer && ack_buffer_len > 0) {
		ret = memcpy_s(ack_buffer, sizeof(rproc_msg_t) * ack_buffer_len, \
			_ack_buffer, sizeof(rproc_msg_t) * ack_buffer_len);
		if (ret != EOK) {
			rproc_pr_err("%s: memcpy_s ack failed", __func__);
			return -EINVAL;
		}
	}

	return res.a0;
#else
	return 0;
#endif
}

static int ipc_xfer_async_to_host(
	rproc_id_t rproc_id, mbox_msg_t *msg, rproc_msg_len_t len)
{
#ifndef CONFIG_LIBLINUX
	int ret;
	struct arm_smccc_res res;
	mbox_msg_t tx_buffer[MBOX_CHAN_DATA_SIZE] = {0};
	unsigned long msg_phy = virt_to_phys(tx_buffer);

	if(msg == NULL || len > MBOX_CHAN_DATA_SIZE) {
		rproc_pr_err("%s param error len %d\n", __func__, len);
		return -EINVAL;
	}

	ret = memcpy_s(tx_buffer, sizeof(mbox_msg_t) * MBOX_CHAN_DATA_SIZE, \
		msg, sizeof(mbox_msg_t) * len);
	if (ret != EOK) {
		rproc_pr_err("%s: memcpy_s failed", __func__);
		return -EINVAL;
	}

	arm_smccc_hvc(IPC_SEND_ASYNC, rproc_id, msg_phy, len, 0, 0, 0, 0, &res);
	return res.a0;
#else
	return 0;
#endif
}

int ipc_rproc_xfer_async(
	rproc_id_t rproc_id, rproc_msg_t *msg, rproc_msg_len_t len)
{
	struct ipc_rproc_info *rproc = NULL;
	struct ipc_mbox_task *tx_task = NULL;
	struct ipc_mbox *mbox = NULL;
	enum mbox_ack_type_t ack_type = AUTO_ACK;
	int ret;
	u64 ts = ktime_get_ns();

	rproc = find_rproc(rproc_id);
	if (!rproc) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	if (check_channel_size(rproc, len))
		return -EINVAL;

	mbox = rproc->mbox;

	/* trap to host */
	if (ipc_mbox_vm_flag(mbox) != 0) {
		ret = ipc_xfer_async_to_host(rproc_id, msg, len);
		if (ret != 0)
			rproc_pr_err("%s trap to host fail, %d", __func__, mbox->rproc_id);

		return ret;
	}

	tx_task = ipc_mbox_task_alloc(mbox, msg, len, ack_type);
	if (!tx_task) {
		rproc_pr_err("alloc task failed");
		return -EINVAL;
	}

	(void)memset_s(tx_task->ts, sizeof(u64) * 11, 0, sizeof(u64) * 11);
	tx_task->rproc_id = rproc_id;
	tx_task->ts[0] = ts;
	tx_task->ts[1] = ktime_get_ns();

	ret = ipc_mbox_msg_send_async(mbox, tx_task);
	if (ret) {
		/* -12:tx_fifo full */
		rproc_pr_err(
			"%s async send failed, errno: %d", mbox->tx->name, ret);
		ipc_mbox_task_free(&tx_task);
	}

	return ret;
}
EXPORT_SYMBOL(ipc_rproc_xfer_async);

int ipc_rproc_xfer_sync(rproc_id_t rproc_id, rproc_msg_t *msg,
	rproc_msg_len_t len, rproc_msg_t *ack_buffer,
	rproc_msg_len_t ack_buffer_len)
{
	struct ipc_rproc_info *rproc = NULL;
	struct ipc_mbox *mbox = NULL;
	enum mbox_ack_type_t ack_type = MANUAL_ACK;
	int ret;

	rproc = find_rproc(rproc_id);
	if (!rproc) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	if (check_channel_size(rproc, len))
		return -EINVAL;

	mbox = rproc->mbox;

	/* trap to host */
	if (ipc_mbox_vm_flag(mbox) != 0) {
		ret = ipc_xfer_sync_to_host(mbox->rproc_id,
				msg, len, ack_buffer,ack_buffer_len);
		if (ret)
			rproc_pr_err("%s trap to host fail, %d", __func__, mbox->rproc_id);

		return ret;
	}

	ret = ipc_mbox_msg_send_sync(
		mbox, msg, len, ack_type, ack_buffer, ack_buffer_len);
	if (ret)
		rproc_pr_err("fail to sync send, id %d", rproc_id);

	return ret;
}
EXPORT_SYMBOL(ipc_rproc_xfer_sync);

#ifndef CONFIG_AB_APCP_NEW_INTERFACE
int hisi_rproc_xfer_sync(rproc_id_t rproc_id, rproc_msg_t *msg,
	rproc_msg_len_t len, rproc_msg_t *ack_buffer,
	rproc_msg_len_t ack_buffer_len)
{
	return ipc_rproc_xfer_sync(rproc_id, msg, len, ack_buffer,
		ack_buffer_len);
}
EXPORT_SYMBOL(hisi_rproc_xfer_sync);
#endif

static int ipc_rproc_rx_notifier(
	struct notifier_block *nb, unsigned long len, void *msg)
{
	struct ipc_rproc_info *rproc =
		container_of(nb, struct ipc_rproc_info, nb);

	atomic_notifier_call_chain(&rproc->notifier, len, msg);
	return 0;
}

int ipc_rproc_rx_register(rproc_id_t rproc_id, struct notifier_block *nb)
{
	struct ipc_rproc_info *rproc = NULL;

	if (!nb) {
		rproc_pr_err("invalid notifier block");
		return -EINVAL;
	}

	rproc = find_rproc(rproc_id);
	if (!rproc) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}
	atomic_notifier_chain_register(&rproc->notifier, nb);
	return 0;
}
EXPORT_SYMBOL(ipc_rproc_rx_register);

int ipc_rproc_rx_unregister(rproc_id_t rproc_id, struct notifier_block *nb)
{
	struct ipc_rproc_info *rproc = NULL;

	if (!nb) {
		rproc_pr_err("invalid notifier block");
		return -EINVAL;
	}

	rproc = find_rproc(rproc_id);
	if (!rproc) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return  -EINVAL;
	}
	atomic_notifier_chain_unregister(&rproc->notifier, nb);
	return 0;
}
EXPORT_SYMBOL(ipc_rproc_rx_unregister);

/*
 * Release the ipc channel's structure, it's usually called by
 * module_exit function, but the module_exit function should never be used.
 * @rproc_id_t
 * @return   0:succeed, other:failed
 */
int ipc_rproc_put(rproc_id_t rproc_id)
{
	struct ipc_rproc_info *rproc = NULL;
	unsigned int i;

	for (i = 0; i < g_rproc_table.rproc_cnt; i++) {
		rproc = &g_rproc_table.rproc_arr[i];
		if (rproc->mbox && rproc_id == rproc->mbox->rproc_id) {
			ipc_mbox_put(&rproc->mbox);
			break;
		}
	}

	if (unlikely(g_rproc_table.rproc_cnt == i)) {
		if (!rproc)
			rproc_pr_err("[%s]rproc pointer is null!\n", __func__);
		else
			rproc_pr_err("\nrelease the ipc channel %d's failed\n",
				rproc_id);

		return -ENODEV;
	}
	return 0;
}

/*
 * Flush the tx_work queue.
 * @rproc_id_t
 * @return   0:succeed, other:failed
 */
int ipc_rproc_flush_tx(rproc_id_t rproc_id)
{
	struct ipc_rproc_info *rproc = NULL;
	unsigned int i;

	for (i = 0; i < g_rproc_table.rproc_cnt; i++) {
		rproc = &g_rproc_table.rproc_arr[i];
		if (!rproc->mbox)
			continue;
		if (rproc->mbox->tx && rproc_id == rproc->mbox->rproc_id) {
			ipc_mbox_empty_task(rproc->mbox->tx);
			break;
		}
	}

	if (unlikely(g_rproc_table.rproc_cnt == i))
		return -ENODEV;

	return 0;
}

/*
 * Judge the mailbox is suspend.
 * @rproc_id_t
 * @return   0:succeed, other:failed
 */
int ipc_rproc_is_suspend(rproc_id_t rproc_id)
{
	struct ipc_rproc_info *rproc = NULL;
	struct ipc_mbox_device *mdev = NULL;
	unsigned long flags = 0;
	int ret = 0;

	rproc = find_rproc(rproc_id);
	if (!rproc || !rproc->mbox || !rproc->mbox->tx) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	mdev = rproc->mbox->tx;
	spin_lock_irqsave(&mdev->status_lock, flags);
	if ((MDEV_DEACTIVATED & mdev->status))
		ret = -ENODEV;
	spin_unlock_irqrestore(&mdev->status_lock, flags);
	return ret;
}
EXPORT_SYMBOL(ipc_rproc_is_suspend);

static void ipc_add_rproc_info(struct ipc_mbox_device *mdev)
{
	struct ipc_rproc_info *rproc = NULL;
	int rproc_id;

	if (g_rproc_table.rproc_cnt >= ARRAY_SIZE(g_rproc_table.rproc_arr)) {
		rproc_pr_debug("rproc table is overload");
		return;
	}

	rproc_id = mdev->ops->get_rproc_id(mdev);
	rproc = &g_rproc_table.rproc_arr[g_rproc_table.rproc_cnt];
	if (rproc->mbox) {
		rproc_pr_debug("rproc-%d has init", rproc_id);
		return;
	}

	ATOMIC_INIT_NOTIFIER_HEAD(&rproc->notifier);

	rproc->nb.next = NULL;
	rproc->nb.notifier_call = ipc_rproc_rx_notifier;
	/*
	 * rproc_id as mdev_index to get the right mailbox-dev
	 */
	rproc->mbox = ipc_mbox_get(rproc_id, &rproc->nb);
	if (!rproc->mbox) {
		rproc_pr_debug("%s rproc[%d] mbox is not exist",
			__func__, rproc_id);
		return;
	}

	g_rproc_table.rproc_cnt++;
	rproc_pr_debug("%s get mbox rproc[%d]", __func__, rproc_id);
}

int ipc_rproc_get_ipc_version(rproc_id_t rproc_id)
{
	struct ipc_rproc_info *rproc = NULL;
	struct ipc_mbox_device *mdev = NULL;

	rproc = find_rproc(rproc_id);
	if (!rproc || !rproc->mbox) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	mdev = rproc->mbox->tx ? rproc->mbox->tx : rproc->mbox->rx;
	if (!mdev)
		return -EINVAL;

	return mdev->ops->get_ipc_version(mdev);
}
EXPORT_SYMBOL(ipc_rproc_get_ipc_version);

int ipc_rproc_get_channel_size(rproc_id_t rproc_id)
{
	struct ipc_rproc_info *rproc = NULL;

	rproc = find_rproc(rproc_id);
	if (!rproc || !rproc->mbox) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	return get_rproc_channel_size(rproc);
}
EXPORT_SYMBOL(ipc_rproc_get_channel_size);

int ipc_rproc_get_mailbox_id(rproc_id_t rproc_id)
{
	struct ipc_mbox_device *mdev = NULL;
	struct ipc_rproc_info *rproc = NULL;

	rproc = find_rproc(rproc_id);
	if (!rproc || !rproc->mbox) {
		rproc_pr_err("%s:invalid rproc xfer:%u", __func__, rproc_id);
		return -EINVAL;
	}

	mdev = rproc->mbox->tx ? rproc->mbox->tx : rproc->mbox->rx;
	if (!mdev)
		return -EINVAL;

	return mdev->ops->get_channel_id(mdev);
}
EXPORT_SYMBOL(ipc_rproc_get_mailbox_id);

#ifdef CONFIG_NPU_PCIE
struct ipc_mbox_device *ipc_rproc_get_mdev(rproc_id_t rproc_id, int mailbox_type)
{
	struct ipc_rproc_info *rproc = NULL;
	struct ipc_mbox_device *mdev = NULL;
	rproc = find_rproc(rproc_id);
	if(mailbox_type == TX_MAIL) {
		mdev = rproc->mbox->tx;
	} else if(mailbox_type == RX_MAIL) {
		mdev = rproc->mbox->rx;
	} else {
		rproc_pr_err("%s: mailbox_type:%d error", __func__, mailbox_type);
	}
	return mdev;
}
EXPORT_SYMBOL(ipc_rproc_get_mdev);
#endif

int ipc_rproc_init(struct ipc_mbox_device **mdev_list)
{
	struct ipc_mbox_device *mdev = NULL;
	unsigned int i;

	for (i = 0; (mdev = mdev_list[i]); i++)
		ipc_add_rproc_info(mdev);

	return 0;
}
EXPORT_SYMBOL(ipc_rproc_init);

static void __exit ipc_rproc_exit(void)
{
	struct ipc_rproc_info *rproc = NULL;
	unsigned int i;

	for (i = 0; i < g_rproc_table.rproc_cnt; i++) {
		rproc = &g_rproc_table.rproc_arr[i];
		if (rproc->mbox)
			ipc_mbox_put(&rproc->mbox);
	}
	g_rproc_table.rproc_cnt = 0;
}

module_exit(ipc_rproc_exit);
/*lint +e580 */
