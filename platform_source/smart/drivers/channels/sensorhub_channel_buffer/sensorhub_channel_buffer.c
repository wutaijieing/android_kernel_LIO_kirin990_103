/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub channel buffer manager.
 * Create: 2021-09-30
 */
#include "sensorhub_channel_buffer.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <securec.h>

#define MAX_RECEIVE_LEN 2048

#define ch_buf_log_info(msg...) pr_info("[I/CH_BUF]" msg)
#define ch_buf_log_err(msg...) pr_err("[E/CH_BUF]" msg)
#define ch_buf_log_warn(msg...) pr_warn("[W/CH_BUF]" msg)

// no multi thread, no need lock
static LIST_HEAD(g_msg_buf_node_list);

struct sensorhub_channel_buf_node {
	struct list_head list;
	u32 service_id;
	struct kfifo read_kfifo;
	u32 max_read_kfifo_len; // read_kfifo array len;
	struct mutex read_mutex;         // mutex for read operation
	wait_queue_head_t read_wait;     // signal to notify data read/write
};

static struct sensorhub_channel_buf_node *get_sensorhub_channel_buf_node(u32 service_id)
{
	struct sensorhub_channel_buf_node *cur = NULL;

	list_for_each_entry(cur, &g_msg_buf_node_list, list) {
		if (cur->service_id == service_id)
			return cur;
	}
	return NULL;
}

int get_active_channel(u32 *channel_array, u32 array_size)
{
	struct sensorhub_channel_buf_node *cur = NULL;
	u32 channel_size = 0;

	list_for_each_entry(cur, &g_msg_buf_node_list, list) {
		if(channel_size < array_size)
			channel_array[channel_size] = cur->service_id;
		channel_size++;
	}
	return (int)channel_size;
}

int store_msg_from_sensorhub(u32 service_id, u32 command_id, u8 *data, u32 data_len)
{
	int ret;
	u8 *buf_alloc_addr = NULL;
	struct sensorhub_channel_buf_node *buf_node = get_sensorhub_channel_buf_node(service_id);

	if (buf_node == NULL) {
		ch_buf_log_err("cannot find buf_node by the service id[%u]...\n", service_id);
		return -EFAULT;
	}

	if (data_len > MAX_RECEIVE_LEN) {
		ch_buf_log_err("%s, service %u data len too big %u...\n", __func__, service_id, data_len);
		return -EFAULT;
	}

	ch_buf_log_info("[%s] store kfifo_len : =[%d], service_id = [%u], cmd id 0x%x, \n",
			__func__, (int)kfifo_len(&buf_node->read_kfifo), service_id, command_id);
	mutex_lock(&(buf_node->read_mutex));
	if (kfifo_avail(&(buf_node->read_kfifo)) < sizeof(u8 *)) {
		ch_buf_log_err("%s: read_kfifo len not available\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}

	buf_alloc_addr = kzalloc(data_len + sizeof(struct amf20_sensorhub_msg), GFP_ATOMIC);
	if (buf_alloc_addr == NULL) {
		ch_buf_log_err("Failed to alloc memory to save upload resp...\n");
		ret = -EFAULT;
		goto ret_err;
	}

	((struct amf20_sensorhub_msg*)buf_alloc_addr)->service_id = service_id;
	((struct amf20_sensorhub_msg*)buf_alloc_addr)->command_id = command_id;
	((struct amf20_sensorhub_msg*)buf_alloc_addr)->data_len = data_len;
	ret = memcpy_s(buf_alloc_addr + sizeof(struct amf20_sensorhub_msg), data_len, data, data_len);
	if (ret != EOK) {
		ch_buf_log_err("%s failed to copy msg_store\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}
	ret = kfifo_in(&(buf_node->read_kfifo), (u8 *)&buf_alloc_addr,
		       sizeof(u8 *));
	if (ret != sizeof(u8 *)) {
		// ret always 0 or sizeof(u8 *), other value is impossible
		ch_buf_log_err("%s kfifo in failed\n", __func__);
		ret = -EFAULT;
		goto ret_err;
	}
	mutex_unlock(&(buf_node->read_mutex));
	wake_up_interruptible(&buf_node->read_wait);
	return 0;

ret_err:
	if(buf_alloc_addr != NULL)
		kfree(buf_alloc_addr);
	mutex_unlock(&(buf_node->read_mutex));
	return ret;
}

static bool data_ready(struct sensorhub_channel_buf_node *buf_node)
{
	ch_buf_log_info("[%s] kfifo_len : =[%d], \n",
			__func__, (int)kfifo_len(&buf_node->read_kfifo));
	if (kfifo_len(&buf_node->read_kfifo) < sizeof(u8 *))
		return false;

	return true;
}

int read_msg_from_sensorhub(u32 service_id, u32 *command_id, u8 *user_buf, u32 buf_len)
{
	struct sensorhub_channel_buf_node *buf_node = get_sensorhub_channel_buf_node(service_id);
	struct amf20_sensorhub_msg *msg_read = NULL;
	u32 msg_payload_len;
	int ret;

	if (buf_node == NULL) {
		ch_buf_log_err("cannot find buf_node by the service id[%u]...\n", service_id);
		return -EFAULT;
	}

	ch_buf_log_info("[%s] enter :service_id =[%u]\n", __func__, service_id);
	/* woke up by signal */
	ret = wait_event_interruptible(buf_node->read_wait, data_ready(buf_node));
	if (ret != 0) {
		ch_buf_log_err("wait interruptible for read[channel:%u] ret:%d", service_id, ret);
		return -EAGAIN;
	}

	mutex_lock(&buf_node->read_mutex);
	ret = kfifo_out(&buf_node->read_kfifo, (u8 *)&msg_read,
			sizeof(u8 *));
	if (ret != sizeof(u8 *)) {
		ch_buf_log_err("%s: kfifo out failed.\n", __func__);
		ret = -EFAULT;
		goto free_buf;
	}

	if (msg_read == NULL) {
		ch_buf_log_err("%s: kfifo out msg read = NULL.\n", __func__);
		ret = -EFAULT;
		goto free_buf;
	}

	if (buf_len < msg_read->data_len) {
		ch_buf_log_err("%s: user len %u < copy len %u.\n", __func__, buf_len, msg_read->data_len);
		ret = -EFAULT;
		goto free_buf;
	}

	msg_payload_len = msg_read->data_len;
	*command_id = msg_read->command_id;
	ret = copy_to_user(user_buf, msg_read->buffer, msg_payload_len);
	if (ret != 0) {
		ch_buf_log_err("%s failed to copy msg_payload_len to user\n", __func__);
		ret = -EFAULT;
		goto free_buf;
	}

	ch_buf_log_info("[%s] :service_id =[%u], command_id = [%u], len %u\n",
					__func__, service_id, msg_read->command_id, msg_payload_len);
	ret = msg_payload_len;

free_buf:
	mutex_unlock(&buf_node->read_mutex);
	if (msg_read != NULL)
		kfree(msg_read);

	return ret;
}

static void free_fifo_buf(struct sensorhub_channel_buf_node *buf_node)
{
	int ret;
	u8 *msg;

	mutex_lock(&buf_node->read_mutex);
	while (kfifo_len(&buf_node->read_kfifo) >= sizeof(u8 *)) {
		ret = kfifo_out(&buf_node->read_kfifo, (u8 *)&msg, sizeof(u8 *));
		if (ret != sizeof(u8 *)) {
			ch_buf_log_err("%s: kfifo out failed.\n", __func__);
			continue;
		}
		if (msg != NULL)
			kfree(msg);
	}
	mutex_unlock(&buf_node->read_mutex);
	return;
}

int sensorhub_channel_buf_init(u32 service_id, u32 channle_buf_len)
{
	struct sensorhub_channel_buf_node *buf_node = get_sensorhub_channel_buf_node(service_id);

	if (buf_node != NULL) {
		ch_buf_log_warn("%s: service_id [%u] channel has been initialized \n", __func__, service_id);
		return -EFAULT;
	}

	buf_node = kzalloc(sizeof(struct sensorhub_channel_buf_node), GFP_ATOMIC);
	if (buf_node == NULL) {
		ch_buf_log_err("[%s] : Failed to alloc memory for sensorhub_channel_buf_node\n", __func__);
		return -EFAULT;
	}

	list_add(&buf_node->list, &g_msg_buf_node_list);
	buf_node->service_id = service_id;

	mutex_init(&buf_node->read_mutex);
	init_waitqueue_head(&buf_node->read_wait);
	buf_node->max_read_kfifo_len = channle_buf_len;
	kfifo_alloc(&buf_node->read_kfifo, sizeof(u8 *) * buf_node->max_read_kfifo_len, GFP_KERNEL);
	ch_buf_log_info("[%s] :service_id =[%u], channle_buf_len =[%d], kfifo_avail = [%d]\n",
			__func__, service_id, channle_buf_len, (int)kfifo_avail(&buf_node->read_kfifo));
	return 0;
}

int sensorhub_channel_buf_release(u32 service_id)
{
	struct sensorhub_channel_buf_node *buf_node = get_sensorhub_channel_buf_node(service_id);
	if (buf_node == NULL) {
		ch_buf_log_err("%s: no suchs serivce id %u in channle buffer\n", __func__, service_id);
		return -EFAULT;
	}

	list_del(&buf_node->list);
	free_fifo_buf(buf_node);
	kfree(buf_node);
	ch_buf_log_info("%s: serivce id %u release\n", __func__, service_id);
	return 0;
}