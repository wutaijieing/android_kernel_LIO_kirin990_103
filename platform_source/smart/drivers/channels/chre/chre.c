/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: Contexthub chre driver. not enable in USER build.
 * Create: 2017-10-19
 */

#include "chre.h"
#include <linux/version.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <securec.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "common/common.h"
#include "inputhub_api/inputhub_api.h"
#include "shmem/shmem.h"

static struct chre_dev g_chre_dev;

#ifdef CONFIG_DFX_DEBUG_FS
#define chre_log_debug(msg...) pr_err("[D/CHRE]" msg)
#else
#define chre_log_debug(msg...)
#endif
#define chre_log_info(msg...) pr_err("[I/CHRE]" msg)
#define chre_log_err(msg...) pr_err("[E/CHRE]" msg)

#ifdef CONFIG_DFX_DEBUG_FS
#define CHRE_DEBUG_LOADAPP_MAX_SIZE 0x2000
#define CHRE_DEBUG_IMAGE_PATH "/data/test"
#define CHRE_DEBUG_IMAGE_PATH_LEN 28
#define  CHRE_APP2DEV_MESSAGE_LEN 8

/*
 * 发送chre 调试命令给contexthub
 */
int chre_debug_cmd(uint64_t appid, int msg_type)
{
	struct chre_dev *d = &g_chre_dev;
	struct chre_local_message *local_msg = NULL;
	unsigned long flags = 0;
	struct chre_app2dev_message *msg = NULL;
	char bin_path[CHRE_MAX_PACKET_LEN];
	int ret;

	chre_log_info("[%s]AppId = 0x%llx, MsgType = %d;\n", __func__, appid, msg_type);

	msg = (struct chre_app2dev_message *)kzalloc(sizeof(struct chre_app2dev_message), GFP_KERNEL);
	if (msg == NULL) {
		chre_log_err("[%s]kmalloc msg failed\n", __func__);
		return -ENOMEM;
	}
	msg->app_id = appid;
	msg->message_type = msg_type;
	msg->len = snprintf_s(bin_path, sizeof(bin_path), sizeof(bin_path) - 1, "%s%lld.bin", CHRE_DEBUG_IMAGE_PATH, appid);
	if (msg->len < 0) {
		chre_log_err("[%s]snprintf_s fail[%d]\n", __func__, msg->len);
		kfree(msg);
		return -EFAULT;
	}
	msg->len += sizeof(struct chreNanoAppBinaryHeader);
	ret = memcpy_s(&msg->data[0], CHRE_MAX_PACKET_LEN, &msg->app_id, sizeof(msg->app_id)); /*must use &msg->app_id, for it's 64bits variable*/
	if (ret != EOK) {
		chre_log_err("[%s]memset_s fail[%d]\n", __func__, ret);
		kfree(msg);
		return ret;
	}
	ret = memcpy_s(&msg->data[8], CHRE_MAX_PACKET_LEN, &msg->app_id, sizeof(msg->app_id));
	if (ret != EOK) {
		chre_log_err("[%s]memset_s fail[%d]\n", __func__, ret);
		kfree(msg);
		return ret;
	}
	ret = strcpy_s(msg->data + sizeof(struct chreNanoAppBinaryHeader),
			CHRE_MAX_PACKET_LEN - sizeof(struct chreNanoAppBinaryHeader) - 1,
			bin_path);
	if (ret != EOK) {
		chre_log_err("[%s]strcpy_s fail[%d]\n", __func__, ret);
		kfree(msg);
		return ret;
	}

	local_msg = (struct chre_local_message *)kmalloc(sizeof(struct chre_local_message), GFP_KERNEL);
	if (local_msg == NULL) {
		chre_log_err("[%s]kmalloc local_msg failed\n", __func__);
		kfree(msg);
		return -ENOMEM;
	}
	local_msg->data = (char *)msg;
	local_msg->data_len = sizeof(struct chre_app2dev_message);

	spin_lock_irqsave(&d->msg_app2dev_lock, flags);
	list_add_tail(&local_msg->list, &d->msg_app2dev_list);
	spin_unlock_irqrestore(&d->msg_app2dev_lock, flags);

	schedule_work(&d->msg_app2dev_hanlder_work);

	chre_log_info("[%s]ok\n", __func__);

	return 0;/*lint !e429*/
}

/*
 * 发送chre 调试消息给contexthub
 */
int chre_debug_msg(uint64_t appid, uint32_t evt_type, uint32_t evt_data0)
{
	struct chre_dev *d = &g_chre_dev;
	struct chre_local_message *local_msg = NULL;
	unsigned long flags = 0;
	struct chre_app2dev_message *msg = NULL;
	int ret;

	chre_log_info("[%s]AppId = 0x%llx, EvtType = 0x%x, Data0 = 0x%x\n",
		__func__, appid, evt_type, evt_data0);

	msg = (struct chre_app2dev_message *)kzalloc(sizeof(struct chre_app2dev_message), GFP_KERNEL);
	if (msg == NULL) {
		chre_log_err("[%s]kmalloc msg failed\n", __func__);
		return -ENOMEM;
	}
	msg->app_id = appid;
	msg->message_type = evt_type;
	msg->len = CHRE_APP2DEV_MESSAGE_LEN;
	ret = memcpy_s(&msg->data[0], CHRE_MAX_PACKET_LEN, &evt_type, sizeof(evt_type)); /*must use &msg->app_id, for it's 64bits variable*/
	if (ret != EOK) {
		chre_log_err("[%s]memset_s fail[%d]\n", __func__, ret);
		kfree(msg);
		return ret;
	}
	ret = memcpy_s(&msg->data[4], CHRE_MAX_PACKET_LEN, &evt_data0, sizeof(evt_data0));
	if (ret != EOK) {
		chre_log_err("[%s]memset_s fail[%d]\n", __func__, ret);
		kfree(msg);
		return ret;
	}

	local_msg = (struct chre_local_message *)kmalloc(sizeof(struct chre_local_message), GFP_KERNEL);
	if (local_msg == NULL) {
		chre_log_err("[%s]kmalloc local_msg failed\n", __func__);
		kfree(msg);
		return -ENOMEM;
	}
	local_msg->data = (char *)msg;
	local_msg->data_len = sizeof(struct chre_app2dev_message);

	spin_lock_irqsave(&d->msg_app2dev_lock, flags);
	list_add_tail(&local_msg->list, &d->msg_app2dev_list);
	spin_unlock_irqrestore(&d->msg_app2dev_lock, flags);

	schedule_work(&d->msg_app2dev_hanlder_work);

	chre_log_info("[%s]ok\n", __func__);

	return 0;/*lint !e429*/
}
#endif

/*
 * 发送chre命令给contexthub
 */
static int chre_send_message(char *data, unsigned int len)
{
	struct chre_dev *d = &g_chre_dev;
	struct chre_local_message *local_msg = NULL;
	unsigned long flags = 0;
	int ret = 0;

	if ((data == NULL) || (0 == len)) {
		chre_log_err("[%s]invalid para\n", __func__);
		return -EINVAL;
	}

	local_msg = (struct chre_local_message *)kmalloc(sizeof(struct chre_local_message), GFP_KERNEL);
	if (local_msg == NULL) {
		chre_log_err("[%s]no local_msg memory\n", __func__);
		return -ENOMEM;
	}

	local_msg->data = (char *)kmalloc(len, GFP_KERNEL);
	if (local_msg->data == NULL) {
		chre_log_err("[%s]no data memory\n", __func__);
		kfree(local_msg);
		return -ENOMEM;
	}
	(void)memcpy_s((void *)local_msg->data, len, (void *)data, len);
	local_msg->data_len = len;

	spin_lock_irqsave(&d->msg_dev2app_lock, flags);
	list_add_tail(&local_msg->list, &d->msg_dev2app_list);
	spin_unlock_irqrestore(&d->msg_dev2app_lock, flags);

	up(&d->msg_dev2app_read_wait);

	return ret;/*lint !e429*/
}

/*
 * 加载nanoapp到共享内存并通知contexthub
 */
static int chre_loadnanoapp_image(char *bin_path, u64 app_id, size_t bin_path_len)
{
	mm_segment_t fs = 0;
	long read_len = 0;
	chre_req_t *pkt = NULL;
	int ret = 0;
	struct file *fp = NULL;

	pkt = (chre_req_t *)kzalloc((sizeof(chre_req_t) + CHRE_DEBUG_LOADAPP_MAX_SIZE), GFP_KERNEL);
	if (pkt == NULL) {
		chre_log_err("[%s]kmalloc failed\n", __func__);
		return -ENOMEM;
	}

	fs = get_fs(); /*lint !e501*/
	set_fs(KERNEL_DS); /*lint !e501*/
	fp = file_open(bin_path, O_RDONLY, 0);

	if (IS_ERR(fp)) {
		chre_log_err("[%s]file_open %s error(%d)\n", __func__, bin_path, PTR_ERR(fp));
		ret = -EFAULT;
	} else {
		vfs_llseek(fp, 0, SEEK_SET);
		read_len = vfs_read(fp, pkt->data, CHRE_DEBUG_LOADAPP_MAX_SIZE, &(fp->f_pos));
		chre_log_info("[%s]BinPath = %s\n", __func__, bin_path);
		chre_log_info("[%s]Appid = 0x%llx\n", __func__, app_id);
		chre_log_info("[%s]ImgLen = %ld\n", __func__, read_len);
		if ((read_len >= CHRE_DEBUG_LOADAPP_MAX_SIZE)
			|| (read_len < CHRE_NANOAPP_HEADER_SIZE) || read_len < 0) {
			chre_log_err("[%s]vfs_read error %lu\n", __func__, read_len);
			ret = -EFAULT;
		}
		filp_close(fp, NULL);
	}
	set_fs(fs);

	if (ret == 0) {
		pkt->hd.tag = TAG_CHRE;
		pkt->hd.cmd = CMD_CHRE_AP_SEND_TO_MCU;
		pkt->hd.resp = NO_RESP;
		pkt->hd.length = sizeof(chre_req_t) - sizeof(struct pkt_header) + read_len;
		pkt->msg_type = CONTEXT_HUB_LOAD_APP;
		pkt->app_id = app_id;
		ret = shmem_send(TAG_CHRE, (void *)pkt, sizeof(chre_req_t) + read_len);
		if (ret < 0) {
			chre_log_err("[%s]shmem_send error\n", __func__);
		}
	}

	kfree(pkt);
	return ret;
}

/*
 * 发送获取CHRE执行结果的命令
 */
static void chre_resp_result(u32 message_type, u64 app_id, int result)
{
	struct chre_dev2app_message msg;
	int ret = 0;

	msg.message_type = message_type;
	msg.app_id = app_id;
	msg.len = sizeof(int);
	ret = memcpy_s(msg.data, CHRE_MAX_PACKET_LEN, &result, sizeof(int));
	if (ret != EOK) {
		chre_log_err("[%s]memset_s fail[%d]\n", __func__, ret);
		return;
	}

	ret = chre_send_message((char *)&msg, sizeof(struct chre_dev2app_message) - CHRE_MAX_PACKET_LEN + sizeof(int));
	if (ret < 0) {
		chre_log_err("[%s]chre_send_message error\n", __func__);
	}

	return;
}

/*
 * 发送加载nano app的消息给contexthub
 */
static void chre_load_nanoapp(struct chre_app2dev_message *msg)
{
	int ret = 0;
	char file_name[CHRE_MAX_PACKET_LEN];
	struct chre_dev *d = &g_chre_dev;
	struct chre_nanoapp_node *pos = NULL;
	struct chre_nanoapp_node *n = NULL;
	struct chreNanoAppBinaryHeader *header = NULL;
	u64 app_id;
	size_t bin_path_len;

	if ((msg->len > CHRE_MAX_PACKET_LEN) || (msg->len <= sizeof(struct chreNanoAppBinaryHeader))) {
		chre_log_err("[%s]msg len is error : %d\n", __func__, msg->len);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, 0, CHRE_RESP_ERR);
		return;
	}

	header = (struct chreNanoAppBinaryHeader *)msg->data;
	app_id = header->appId;
	list_for_each_entry_safe(pos, n, &d->nanoapp_list, list)
		if (pos->app_id == app_id) {
			chre_log_err("[%s]nanoapp(0x%llx) has been load\n", __func__, app_id);
			chre_resp_result(CONTEXT_HUB_LOAD_APP, app_id, CHRE_RESP_ERR);
			return;
		}

	ret = memcpy_s(file_name, CHRE_MAX_PACKET_LEN - 1,
		msg->data + sizeof(struct chreNanoAppBinaryHeader),
		msg->len - sizeof(struct chreNanoAppBinaryHeader));
	if (ret != EOK) {
		chre_log_err("[%s]memcpy_s fail[%d]\n", __func__, ret);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, app_id, CHRE_RESP_ERR);
		return;
	}

	file_name[msg->len - sizeof(struct chreNanoAppBinaryHeader)] = '\0';

	pos = (struct chre_nanoapp_node *)kmalloc(sizeof(struct chre_nanoapp_node), GFP_KERNEL);
	if (NULL == pos) {
		chre_log_err("[%s]kmalloc nanoapp node fail\n", __func__);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, app_id, CHRE_RESP_ERR);
		return;
	}
	pos->app_id = app_id;
	ret = strcpy_s(pos->bin_path, CHRE_MAX_PACKET_LEN - 1, file_name);
	if (ret != EOK) {
		chre_log_err("[%s]strcpy_s fail[%d]\n", __func__, ret);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, app_id, CHRE_RESP_ERR);
		kfree(pos);
		return;
	}

	mutex_lock(&d->msg_resp_stat_lock);
	d->msg_result = CHRE_WAIT_MSG_STATUS;
	bin_path_len = strlen(pos->bin_path);
	ret = chre_loadnanoapp_image(pos->bin_path, pos->app_id, bin_path_len);
	if (ret < 0) {
		chre_log_err("[%s]load image fail\n", __func__);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, app_id, CHRE_RESP_ERR);
		goto load_err;
	}

	mutex_unlock(&d->msg_resp_stat_lock);
	ret = down_timeout(&d->msg_mcu_resp, msecs_to_jiffies(CHRE_MSG_RESP_TIMEOUT));
	mutex_lock(&d->msg_resp_stat_lock);
	if (ret == -ETIME) {
		chre_log_err("[%s]wait response timeout\n", __func__);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, app_id, CHRE_RESP_ERR);
		goto load_err;
	}

	if (d->msg_result != 0) {
		chre_log_err("[%s]resp result(%d) error\n", __func__, d->msg_result);
		goto load_err;
	}

	d->msg_result = 0;
	mutex_unlock(&d->msg_resp_stat_lock);
	/*load nanoapp success, add it to list*/
	list_add_tail(&pos->list, &d->nanoapp_list);
	return;/*lint !e429*/

load_err:
	d->msg_result = 0;
	mutex_unlock(&d->msg_resp_stat_lock);
	kfree(pos);
	return;
}

/*
 * 发送卸载 nano app命令给contexthub
 */
static void chre_unload_nanoapp(struct chre_app2dev_message *msg)
{
	chre_req_t pkt;
	int ret = 0;
	u64 app_id = 0;
	struct chre_nanoapp_node *pos = NULL;
	struct chre_nanoapp_node *n = NULL;
	struct chre_dev *d = &g_chre_dev;
	bool found = 0;

	app_id = ((u64 *)msg->data)[0];
	chre_log_info("[%s]AppId = 0x%llx, MsgLen=%d\n", __func__, app_id, msg->len);

	list_for_each_entry_safe(pos, n, &d->nanoapp_list, list)
		if (pos->app_id == app_id) {
			found = 1;
			break;
		}
	if (found == 0) {
		chre_log_err("[%s]nanoapp(0x%llx) has not been load\n", __func__, app_id);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, 0, CHRE_RESP_ERR);
		return;
	}

	pkt.hd.tag = TAG_CHRE;
	pkt.hd.cmd = CMD_CHRE_AP_SEND_TO_MCU;
	pkt.hd.resp = NO_RESP;
	pkt.hd.length = sizeof(chre_req_t) - sizeof(struct pkt_header);
	pkt.msg_type = CONTEXT_HUB_UNLOAD_APP;
	pkt.app_id = app_id;

	mutex_lock(&d->msg_resp_stat_lock);
	d->msg_result = CHRE_WAIT_MSG_STATUS;
	ret = shmem_send(TAG_CHRE, (void *)&pkt, sizeof(chre_req_t));
	if (ret < 0) {
		chre_log_err("[%s]nanoapp(0x%llx) shmem_send fail\n",
				__func__, app_id);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, 0, CHRE_RESP_ERR);
		goto end;
	}

	mutex_unlock(&d->msg_resp_stat_lock);
	ret = down_timeout(&d->msg_mcu_resp, msecs_to_jiffies(CHRE_MSG_RESP_TIMEOUT));
	mutex_lock(&d->msg_resp_stat_lock);
	if (ret == -ETIME) {
		chre_log_err("[%s]wait response timeout\n", __func__);
		chre_resp_result(CONTEXT_HUB_LOAD_APP, 0, CHRE_RESP_ERR);
		goto end;
	}

	if (d->msg_result != 0) {
		chre_log_err("[%s]resp result(%d) error\n", __func__, d->msg_result);
		goto end;
	}

end:
	d->msg_result = 0;
	mutex_unlock(&d->msg_resp_stat_lock);
	list_del(&pos->list);
	kfree(pos);
	return;
}

/*
 * 发送切换 nano app命令给contexthub
 */
static void chre_switch_nanoapp(struct chre_app2dev_message *msg)
{
	chre_req_t pkt = {0};
	int ret;

	chre_log_info("[%s]MsgType = %d AppId = 0x%llx MsgLen = %d\n",
			__func__, msg->message_type, msg->app_id, msg->len);

	pkt.hd.tag = TAG_CHRE;
	pkt.hd.cmd = CMD_CHRE_AP_SEND_TO_MCU;
	pkt.hd.resp = NO_RESP;
	pkt.hd.length = sizeof(chre_req_t) - sizeof(struct pkt_header);
	pkt.msg_type = (uint8_t)msg->message_type;
	pkt.app_id = msg->app_id;

	ret = shmem_send(TAG_CHRE, (void *)&pkt, sizeof(chre_req_t));
	if (ret < 0) {
		chre_log_err("[%s]shmem_send error\n", __func__);
	}
}

/*
 * 发送nano app私有消息给contexthub
 */
static void chre_private_msg_handle(struct chre_app2dev_message *msg)
{
	chre_req_t *pkt = NULL;
	int send_len;
	int ret;
	int i;

	if (msg->len > CHRE_MAX_PACKET_LEN) {
		chre_log_err("[%s]msg len is error : %d\n", __func__, msg->len);
		return;
	}

	send_len = sizeof(chre_req_t) + msg->len;
	pkt = (chre_req_t *)kzalloc(send_len, GFP_KERNEL);
	if (NULL == pkt) {
		chre_log_err("[%s]kmalloc failed\n", __func__);
		return;
	}

	pkt->hd.tag = TAG_CHRE;
	pkt->hd.cmd = CMD_CHRE_AP_SEND_TO_MCU;
	pkt->hd.resp = NO_RESP;
	pkt->hd.length = send_len - sizeof(struct pkt_header);
	pkt->msg_type = (uint16_t)CONTEXT_HUB_PRIVATE_MSG;
	pkt->app_id = msg->app_id;
	chre_log_debug("[%s][EvtId = %d]\n", __func__, *((u32 *)msg->data));
	for (i = sizeof(u32); i < msg->len; i ++) {
		chre_log_debug("0x%x\n", msg->data[i]);
	}
	(void)memcpy_s(pkt->data, msg->len, msg->data, msg->len);
	ret = shmem_send(TAG_CHRE, (void *)pkt, send_len);
	if (ret < 0) {
		chre_log_err("[%s]shmem_send error\n", __func__);
	}
	kfree(pkt);
}

/*
 * chre消息处理
 */
static void chre_handle_msg(char *data)
{
	if (data == NULL) {
		chre_log_err("[%s] data is null\n", __func__);
		return;
	}

	struct chre_app2dev_message *msg = (struct chre_app2dev_message *)data;
	switch (msg->message_type) {
	case CONTEXT_HUB_APPS_ENABLE:
	case CONTEXT_HUB_APPS_DISABLE:
		chre_switch_nanoapp(msg);
		break;
	case CONTEXT_HUB_UNLOAD_APP:
		chre_unload_nanoapp(msg);
		break;
	case CONTEXT_HUB_LOAD_APP:
		chre_load_nanoapp(msg);
		break;
	case CONTEXT_HUB_QUERY_APPS:
		break;
	default:
		chre_private_msg_handle(msg);
		break;
	}

	return;
}

/*
 * chre消息处理workqueue
 */
static void chre_msg_handler_work(struct work_struct *work)
{
	struct chre_dev *d = &g_chre_dev;
	unsigned long flags = 0;
	struct chre_local_message *msg = NULL;

	do {
		spin_lock_irqsave(&d->msg_app2dev_lock, flags);
		if (list_empty(&d->msg_app2dev_list)) {
			spin_unlock_irqrestore(&d->msg_app2dev_lock, flags);
			return;
		}
		msg = list_first_entry(&d->msg_app2dev_list, struct chre_local_message, list);
		list_del(&msg->list);
		spin_unlock_irqrestore(&d->msg_app2dev_lock, flags);
		chre_handle_msg(msg->data);
		kfree(msg->data);
		kfree(msg);
	} while (1);

}

static int chre_mcu_msg_handler(struct chre_dev2app_message *msg)
{
	struct chre_dev *d = &g_chre_dev;
	int ret = 0;

	switch (msg->message_type) {
	case CONTEXT_HUB_LOAD_APP:
	case CONTEXT_HUB_UNLOAD_APP:
		mutex_lock(&d->msg_resp_stat_lock);
		if (CHRE_WAIT_MSG_STATUS == d->msg_result) {
			d->msg_result = ((int *)msg->data)[0];
			up(&d->msg_mcu_resp);
		} else {
			ret = -EINVAL;
		}
		mutex_unlock(&d->msg_resp_stat_lock);
		break;
	default:
		break;
	}

	return ret;
}

/*
 * contexthub消息处理
 */
static int chre_mcu_event_notify(const struct pkt_header *head)
{
	if (head == NULL) {
		chre_log_err("[%s] head is null\n", __func__);
		return -EFAULT;
	}

	chre_req_t *pkt = (chre_req_t *)head;
	struct chre_dev2app_message msg;
	uint16_t data_len;
	int ret;

	msg.message_type = pkt->msg_type;
	msg.app_id = pkt->app_id;
	data_len = pkt->hd.length - (sizeof(chre_req_t) - sizeof(struct pkt_header));

	chre_log_debug("[%s]MsgType = %d, AppId = 0x%llx, MsgLen = %d, Result = %d\n", __func__,
		msg.message_type, msg.app_id, data_len, ((int *)pkt->data)[0]);
	if (((int *)pkt->data)[0] == 0x8001) {
		chre_log_debug("[%s]%s\n", __func__, (char *)(pkt->data + sizeof(int)));
	}

	if (data_len > CHRE_MAX_PACKET_LEN) {
		chre_log_err("[%s]msg len is too long (%d)\n", __func__, data_len);
		return 0;
	}
	msg.len = data_len;
	ret = memcpy_s(msg.data, CHRE_MAX_PACKET_LEN, pkt->data, data_len);
	if (ret != EOK) {
		chre_log_err("[%s]memset_s fail[%d]\n", __func__, ret);
	}

	ret = chre_mcu_msg_handler(&msg);
	if (ret < 0) {
		chre_log_info("[%s]invalid message\n", __func__);
		return 0;
	}

	ret = chre_send_message((char *)&msg, (sizeof(struct chre_dev2app_message) - CHRE_MAX_PACKET_LEN + data_len));
	if (ret < 0) {
		chre_log_err("[%s]chre_send_message failed\n", __func__);
	}

	return 0;
}

/*
 * 设备节点读消息处理
 */
static ssize_t chre_dev_read(struct file *file, char __user *buf, size_t count,
				loff_t *pos)
{
	struct chre_dev *d = &g_chre_dev;
	struct chre_local_message *msg = NULL;
	unsigned long flags = 0;
	ssize_t read_len;
	int ret;

get_message:
	ret = down_interruptible(&d->msg_dev2app_read_wait);
	if (ret != 0) {
		chre_log_err("[%s]read_wait down by signal\n", __func__);
		return 0;
	}
	chre_log_debug("[%s]get one msg\n", __func__);

	spin_lock_irqsave(&d->msg_dev2app_lock, flags);
	if (list_empty(&d->msg_dev2app_list)) {
		spin_unlock_irqrestore(&d->msg_dev2app_lock, flags);
		chre_log_err("[%s]no message\n", __func__);
		goto get_message;
	}
	msg = list_first_entry(&d->msg_dev2app_list, struct chre_local_message, list);
	list_del(&msg->list);
	spin_unlock_irqrestore(&d->msg_dev2app_lock, flags);

	ret = 0;
	if (msg->data == NULL) {
		chre_log_err("[%s]msg is null\n", __func__);
		ret = -EINVAL;
	}
	if ((msg->data_len == 0) || (msg->data_len > count)) {
		chre_log_err("[%s]msg len invalid\n", __func__);
		ret = -EINVAL;
	}
	if (ret == 0) {
		if (copy_to_user(buf, (const void *)msg->data, (size_t)msg->data_len)) {
			chre_log_err("[%s]copy to user failed\n", __func__);
			ret = -EFAULT;
		}
	}
	read_len = (ssize_t)msg->data_len;
	if (msg->data != NULL)
		kfree(msg->data);
	kfree(msg);

	if (ret != 0) {
		chre_log_err("[%s]invalid message\n", __func__);
		goto get_message;
	}

	return read_len;
}

/*
 * 设备节点写消息处理
 */
static ssize_t chre_dev_write(struct file *file, const char __user *data,
						size_t len, loff_t *ppos)
{
	struct chre_dev *d = &g_chre_dev;
	char *msg = NULL;
	struct chre_local_message *local_msg;
	unsigned long flags = 0;

	chre_log_debug("chre_dev_write msg_len = %ld\n", len);

	if ((len < (sizeof(struct chre_app2dev_message) - CHRE_MAX_PACKET_LEN)) || (len > sizeof(struct chre_app2dev_message))) {
		chre_log_err("[%s]invalid message len=%ld\n", __func__, len);
		return -EINVAL;
	}

	msg = (char *)kmalloc(sizeof(struct chre_app2dev_message), GFP_KERNEL);
	if (msg == NULL) {
		chre_log_err("[%s]kmalloc msg failed\n", __func__);
		return -ENOMEM;
	}
	if (copy_from_user((void *)msg, data, len)) {
		chre_log_err("[%s]copy from user failed\n", __func__);
		kfree(msg);
		return -EFAULT;
	}

	local_msg = (struct chre_local_message *)kmalloc(sizeof(struct chre_local_message), GFP_KERNEL);
	if (local_msg == NULL) {
		chre_log_err("[%s]kmalloc local_msg failed\n", __func__);
		kfree(msg);
		return -ENOMEM;
	}
	local_msg->data = msg;
	local_msg->data_len = len;
	spin_lock_irqsave(&d->msg_app2dev_lock, flags);
	list_add_tail(&local_msg->list, &d->msg_app2dev_list);
	spin_unlock_irqrestore(&d->msg_app2dev_lock, flags);

	schedule_work(&d->msg_app2dev_hanlder_work);

	return len;/*lint !e429*/
}

/*
 * contexthub复位后业务恢复
 */
static void chre_recovery_handler(void)
{
	struct chre_dev *d = &g_chre_dev;
	struct chre_nanoapp_node *pos;
	struct chre_nanoapp_node *n;
	struct chre_dev2app_message msg;
	int ret;

	chre_log_info("[%s] ++", __func__);

	/* reload nanoapps */
	list_for_each_entry_safe(pos, n, &d->nanoapp_list, list) {
		ret = chre_loadnanoapp_image(pos->bin_path, pos->app_id, strlen(pos->bin_path));
		if (ret < 0) {
			chre_log_err("[%s]error BinPath = %s AppId = 0x%llx\n",
				__func__, pos->bin_path, pos->app_id);
		}
	}

	/* send reboot message to hal */
	msg.message_type = CONTEXT_HUB_OS_REBOOT;
	msg.len = 0;
	ret = chre_send_message((char *)&msg, (sizeof(struct chre_dev2app_message) - CHRE_MAX_PACKET_LEN));
	if (ret < 0) {
		chre_log_err("[%s]chre_send_message failed\n", __func__);
	}

	chre_log_info("[%s]--", __func__);
}

/*
 * contexthub复位通知
 */
static int chre_recover_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	switch (action) {
		case IOM3_RECOVERY_IDLE:
		case IOM3_RECOVERY_START:
		case IOM3_RECOVERY_MINISYS:
		case IOM3_RECOVERY_3RD_DOING:
		case IOM3_RECOVERY_FAILED:
			break;
		case IOM3_RECOVERY_DOING:
			chre_recovery_handler();
			break;
		default:
			break;
	}
	return 0;
}

static long chre_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int chre_dev_open(struct inode *inode, struct file *file)
{
	chre_log_info("chre device opened\n");
	return 0;
}

static int chre_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct notifier_block chre_recover_notify = {
	.notifier_call = chre_recover_notifier,
	.priority = -1,
};

static const struct file_operations chre_dev_fops = {
	.owner      =   THIS_MODULE,
	.llseek     =   no_llseek,
	.read       =   chre_dev_read,
	.write      =   chre_dev_write,
	.unlocked_ioctl =  chre_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =  chre_dev_ioctl,
#endif
	.open       =   chre_dev_open,
	.release    =   chre_dev_release,
};

static struct miscdevice nanohub_dev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "nanohub_comms",
	.fops =     &chre_dev_fops,
};

/*
 * contexthub运行时环境的初始化
 */
int __init chre_init(void)
{
	int ret = 0;
	int ret1 = 0;
	struct chre_dev *d = &g_chre_dev;

	ret = get_contexthub_dts_status();
	if (ret != 0) {
		chre_log_err("[%s]contexthub is not enabled\n", __func__);
		return ret;
	}

	ret = register_mcu_event_notifier(TAG_CHRE, CMD_CHRE_MCU_SEND_TO_AP, chre_mcu_event_notify);
	ret1 = register_mcu_event_notifier(TAG_CHRE, CMD_CHRE_AP_SEND_TO_MCU_RESP, chre_mcu_event_notify);
	if (ret < 0 || ret1 < 0) {
		chre_log_err("[%s]register_mcu_event_notifier fail\n", __func__);
		return ret;
	}

	ret = register_iom3_recovery_notifier(&chre_recover_notify);
	if (ret < 0) {
		chre_log_err("[%s]register_iom3_recovery_notifier fail\n", __func__);
		return ret;
	}

	ret = misc_register(&nanohub_dev);
	if (ret != 0) {
		chre_log_err("[%s]nanohub_comms register error\n", __func__);
		return ret;
	}

	sema_init(&d->msg_dev2app_read_wait, 0);
	sema_init(&d->msg_mcu_resp, 0);
	spin_lock_init(&d->msg_app2dev_lock);
	INIT_LIST_HEAD(&d->msg_app2dev_list);
	spin_lock_init(&d->msg_dev2app_lock);
	INIT_LIST_HEAD(&d->msg_dev2app_list);
	INIT_WORK(&d->msg_app2dev_hanlder_work, chre_msg_handler_work);
	INIT_LIST_HEAD(&d->nanoapp_list);
	mutex_init(&d->msg_resp_stat_lock);

	chre_log_info("chre init ok\n");

	return ret;
}

late_initcall_sync(chre_init);
MODULE_ALIAS("platform:contexthub" MODULE_NAME);
MODULE_LICENSE("GPL v2");
