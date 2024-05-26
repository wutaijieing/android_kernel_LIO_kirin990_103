/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description:  common channel driver for multiple service
 * Create:       2021-09-29
 */

#include <securec.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <platform_include/smart/linux/iomcu_ipc.h>

#include "common/common.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ipc.h"
#include "iomcu_log.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "sensorhub_channel_buffer/sensorhub_channel_buffer.h"

#define SENSORHUB_NODE_IO                           0xB3
#define SENSORHUB_NODE_IOCTL_SEND_CMD              _IOW(SENSORHUB_NODE_IO, 0x01, short)
#define SENSORHUB_NODE_IOCTL_READ_DATA             _IOR(SENSORHUB_NODE_IO, 0x02, short)
#define SENSORHUB_NODE_IOCTL_CONFIG_CMD            _IOW(SENSORHUB_NODE_IO, 0x03, short)
#define SENSORHUB_NODE_IOCTL_QUERY_MAX_LEN         _IOR(SENSORHUB_NODE_IO, 0x04, short)
#define SENSORHUB_NODE_IOCTL_SET_MAX_EVENT_NUM     _IOW(SENSORHUB_NODE_IO, 0x05, short)

#define MAX_CHANNEL_SIZE 50
#define RECOVERY_CMD_ID 0xffffffff
#define RECOVERY_FLAG 0

#define MAX_SEND_LEN 1024

struct send_cmd_struct {
	u32 service_id;
	u32 cmd;
	u32 len;
	char *payload;
};

struct send_cmd_struct_32 {
	u32 service_id;
	u32 cmd;
	u32 len;
	compat_uptr_t payload;
};

struct ioctl_read_data {
	u32 service_id;
	u32 command_id;
	u32 buffer_len;
	char *buffer;
};

struct ioctl_read_data_32 {
	u32 service_id;
	u32 command_id;
	u32 buffer_len;
	compat_uptr_t buffer;
};

struct config_max_event_num {
	u32 service_id;
	u32 max_event_num;
};

static int sensorhub_node_read_data(unsigned long arg)
{
	struct ioctl_read_data user_data;
	s32 ret;

	if (copy_from_user(&user_data, (struct ioctl_read_data *)arg, sizeof(struct ioctl_read_data)) != 0) {
		ctxhub_err("%s, copy read data struct failed!\n", __func__);
		return -EINVAL;
	}

	ret = read_msg_from_sensorhub(user_data.service_id, &(user_data.command_id), user_data.buffer, user_data.buffer_len);
	if (ret <= 0) {
		ctxhub_err("%s, read event failed!\n", __func__);
		return -EINVAL;
	}

	if (copy_to_user((struct ioctl_read_data *)arg, &user_data, sizeof(struct ioctl_read_data)) != 0) {
		ctxhub_err("%s, copy to user failed!\n", __func__);
		return -EINVAL;
	}
	ctxhub_info("%s, read service %d, command id 0x%x data success!\n",
				__func__, user_data.service_id, user_data.command_id);
	// we need return read bytes
	return ret;
}

static int sensorhub_node_send_cmd(unsigned long arg)
{
	struct send_cmd_struct send = {0};
	char *payload = NULL;
	s32 ret = 0;

	if (copy_from_user(&send, (struct send_cmd_struct *)arg, sizeof(struct send_cmd_struct)) != 0) {
		ctxhub_err("%s, copy send cmd failed\n", __func__);
		return -EFAULT;
	}

	if (send.len > MAX_SEND_LEN) {
		ctxhub_err("%s, send too much %u, not support\n", __func__, send.len);
		return -EFAULT;
	}

	payload = kzalloc(send.len, GFP_KERNEL);
	if (!payload) {
		ctxhub_err("%s, alloc send buffer failed!\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(payload, send.payload, send.len) != 0) {
		ctxhub_err("%s, copy send payload failed!\n", __func__);
		ret = -EINVAL;
		goto free;
	}

	ret = tcp_send_data_by_amf20((u16)send.service_id, (u16)send.cmd, payload, send.len);
	if (ret != 0)
		ctxhub_err("%s, service %u send ipc failed!\n", __func__, send.service_id);

free:
	if (payload != NULL)
		kfree(payload);
	return ret;
}

static int sensorhub_node_packet_process(const uint16_t svc_id, const uint16_t cmd,
										 const void *data, const uint16_t len)
{
	s32 ret;

	if (!data) {
		ctxhub_err("%s, receive null header\n", __func__);
		return -EINVAL;
	}

	ret = store_msg_from_sensorhub(svc_id, cmd, data, len);
	if (ret != 0) {
		ctxhub_err("%s, store event failed!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int sensorhub_node_set_max_event_num(unsigned long arg)
{
	struct config_max_event_num max_event;
	s32 ret;

	if (copy_from_user(&max_event, (struct config_max_event_num *)arg, sizeof(struct config_max_event_num)) != 0) {
		ctxhub_err("%s, read event num failed!\n", __func__);
		return -EINVAL;
	}

	ret = sensorhub_channel_buf_init(max_event.service_id, max_event.max_event_num);
	if (ret != 0) {
		ctxhub_err("%s, set max event num failed!\n", __func__);
		return -EINVAL;
	}

	ret = register_amf20_notifier(max_event.service_id, AMF20_CMD_ALL, sensorhub_node_packet_process);
	if (ret != 0) {
		ctxhub_err("%s, register ipc cb failed!\n", __func__);
		goto channel_buff_release;
	}
	ctxhub_info("%s, set max event num for service %d success!\n", __func__, max_event.service_id);
	return 0;

channel_buff_release:
	(void)sensorhub_channel_buf_release(max_event.service_id);
	return ret;
}

#ifdef CONFIG_COMPAT
static int sensorhub_node_compat_send_cmd(unsigned long arg)
{
	struct send_cmd_struct_32 send32;
	struct send_cmd_struct __user *send;
	s32 ret = 0;

	if (copy_from_user(&send32, compat_ptr(arg), sizeof(struct send_cmd_struct_32)) != 0) {
		ctxhub_err("%s, copy 32 send cmd failed!\n", __func__);
		return -EFAULT;
	}

	send = compat_alloc_user_space(sizeof(struct send_cmd_struct));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 18) || \
	LINUX_VERSION_CODE == KERNEL_VERSION(4, 19, 71))
	if (!access_ok(VERIFY_WRITE, send, sizeof(struct send_cmd_struct))) {
#else
	if (!access_ok(send, sizeof(struct send_cmd_struct))) {
#endif
		ctxhub_err("%s, no permisson to write\n", __func__);
		return -EFAULT;
	}

	ret |= put_user(send32.service_id, &(send->service_id));
	ret |= put_user(send32.cmd, &(send->cmd));
	ret |= put_user(send32.len, &(send->len));
	ret |= put_user(compat_ptr(send32.payload), &(send->payload));
	if (ret < 0 ) {
		ctxhub_err("%s, put user param failed!\n", __func__);
		return ret;
	}

	return sensorhub_node_send_cmd((unsigned long)send);
}

static int sensorhub_node_compat_read_data(unsigned long arg)
{
	s32 ret = 0;
	s32 read_len;

	struct ioctl_read_data_32 data32 = { 0 };
	struct ioctl_read_data __user *data;

	if (copy_from_user(&data32, compat_ptr(arg), sizeof(struct ioctl_read_data_32)) != 0) {
		ctxhub_err("%s, read data copy from user failed!\n", __func__);
		return -EFAULT;
	}

	data = compat_alloc_user_space(sizeof(struct ioctl_read_data));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 18) || \
	LINUX_VERSION_CODE == KERNEL_VERSION(4, 19, 71))
	if (!access_ok(VERIFY_WRITE, data, sizeof(struct ioctl_read_data))) {
#else
	if (!access_ok(data, sizeof(struct ioctl_read_data))) {
#endif
		ctxhub_err("%s, no permission to write!\n", __func__);
		return -EFAULT;
	}

	ret |= put_user(data32.service_id, &(data->service_id));
	ret |= put_user(data32.command_id, &(data->command_id));
	ret |= put_user(data32.buffer_len, &(data->buffer_len));
	ret |= put_user(compat_ptr(data32.buffer), &(data->buffer));
	if (ret < 0) {
		ctxhub_err("%s, put user param failed %d!\n", __func__, ret);
		return ret;
	}

	read_len = sensorhub_node_read_data((unsigned long)data);
	if (read_len > 0) {
		// ret is read data bytes, > 0 means success
		ret |= get_user(data32.service_id, &(data->service_id));
		ret |= get_user(data32.command_id, &(data->command_id));
		if (ret < 0) {
			ctxhub_err("%s, set service id or command id failed!\n", __func__);
			return -EFAULT;
		}

		if (copy_to_user(arg, &data32, sizeof(struct ioctl_read_data_32)) != 0) {
			ctxhub_err("%s, copy read result to user failed!\n", __func__);
			return -EFAULT;
		}
	}
	return read_len;
}

static long sensorhub_node_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	s32 ret = 0;

	if (!file) {
		ctxhub_err("%s, check file param failed!\n", __func__);
		return -EINVAL;
	}

	ctxhub_info("%s: cmd: [0x%x]\n", __func__, cmd);
	switch(cmd) {
	case SENSORHUB_NODE_IOCTL_SEND_CMD:
		ret = sensorhub_node_compat_send_cmd(arg);
		break;
	case SENSORHUB_NODE_IOCTL_READ_DATA:
		ret = sensorhub_node_compat_read_data(arg);
		break;
	case SENSORHUB_NODE_IOCTL_SET_MAX_EVENT_NUM:
		ret = sensorhub_node_set_max_event_num(arg);
		break;
	case SENSORHUB_NODE_IOCTL_CONFIG_CMD:
		break;
	case SENSORHUB_NODE_IOCTL_QUERY_MAX_LEN:
		break;
	default:
		ctxhub_err("%s: unknown cmd : %u\n", __func__, cmd);
		return -EINVAL;
	}
	ctxhub_info("%s, cmd: [0x%x] ret %d\n", __func__, cmd, ret);
	return ret;
}

#endif

static long sensorhub_node_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if (file == NULL) {
		ctxhub_err("%s: invalid param\n", __func__);
		return -EINVAL;
	}

	ctxhub_info("%s: cmd: [0x%x]\n", __func__, cmd);

	switch (cmd) {
	case SENSORHUB_NODE_IOCTL_SEND_CMD:
		ret = sensorhub_node_send_cmd(arg);
		break;
	case SENSORHUB_NODE_IOCTL_READ_DATA:
		ret = sensorhub_node_read_data(arg);
		break;
	case SENSORHUB_NODE_IOCTL_CONFIG_CMD:
		break;
	case SENSORHUB_NODE_IOCTL_QUERY_MAX_LEN:
		break;
	case SENSORHUB_NODE_IOCTL_SET_MAX_EVENT_NUM:
		ret = sensorhub_node_set_max_event_num(arg);
		break;
	default:
		ctxhub_err("%s: unknown cmd : %d\n", __func__, cmd);
		return -EINVAL;
	}

	return ret;
}

/* sensorhub_node_open: do nothing now */
static int sensorhub_node_open(struct inode *inode, struct file *file)
{
	ctxhub_info("%s ok\n", __func__);
	return 0;
}

/* sensorhub_node_release: do nothing now */
static int sensorhub_node_release(struct inode *inode, struct file *file)
{
	ctxhub_info("%s ok\n", __func__);
	return 0;
}

static ssize_t sensorhub_node_read(struct file *file, char __user *buf, size_t count,
			loff_t *pos)
{
	return 0;
}

static const struct file_operations g_sensorhub_node_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = sensorhub_node_read,
	.unlocked_ioctl = sensorhub_node_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sensorhub_node_compat_ioctl,
#endif
	.open = sensorhub_node_open,
	.release = sensorhub_node_release,
};

static struct miscdevice g_sensorhub_node_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sensorhub_node",
	.fops = &g_sensorhub_node_fops,
};

static void store_recovery_event_to_all_channel(void)
{
	int num;
	int arr[MAX_CHANNEL_SIZE];
	int i;
	int ret;
	char buf = RECOVERY_FLAG;

	num = get_active_channel(arr, MAX_CHANNEL_SIZE);
	if (num > MAX_CHANNEL_SIZE)
		ctxhub_warn("num %d exceed max channel size %d\n", num, MAX_CHANNEL_SIZE);

	num = num > MAX_CHANNEL_SIZE ? MAX_CHANNEL_SIZE : num;
	for (i = 0; i < num; i++) {
		ret = store_msg_from_sensorhub(arr[i], RECOVERY_CMD_ID, &buf, sizeof(char));
		if (ret < 0)
			ctxhub_err("%s, store recovery event to channel %d failed!\n", __func__, arr[i]);
	}
}

static int recovery_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	int ret = 0;

	switch (action) {
	case IOM3_RECOVERY_3RD_DOING:
		pr_info("[%s]: sensorhub recovery is doing.\n", __func__);
		break;
	case IOM3_RECOVERY_IDLE:
		pr_info("[%s]: sensorhub recovery is done.\n", __func__);
		store_recovery_event_to_all_channel();
		break;
	default:
		pr_err("[%s]: register_iom3_recovery_notifier err.\n", __func__);
		break;
	}
	return ret;
}

static struct notifier_block recovery_notify = {
	.notifier_call = recovery_notifier,
	.priority = -1,
};

static int __init sensorhub_node_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;

	ret = misc_register(&g_sensorhub_node_dev);
	if (ret != 0) {
		ctxhub_err("%s, register sensorhub node dev failed!\n", __func__);
		return ret;
	}

	(void)register_iom3_recovery_notifier(&recovery_notify);

	ctxhub_info("%s ok\n", __func__);
	return 0;
}

static void __exit sensorhub_node_exit(void)
{
	misc_deregister(&g_sensorhub_node_dev);
}

late_initcall_sync(sensorhub_node_init);
module_exit(sensorhub_node_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SensorHub common channel driver");
MODULE_AUTHOR("Smart");
