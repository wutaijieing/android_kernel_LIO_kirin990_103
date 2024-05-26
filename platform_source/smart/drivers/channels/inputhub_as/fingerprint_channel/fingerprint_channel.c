/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: fingerprint channel implement.
 * Create: 2019/11/25
 */


#include <securec.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <huawei_platform/inputhub/fingerprinthub.h>
#include "iomcu_route.h"
#include "device_interface.h"
#include "common_func.h"
#include "common/common.h"
#include "inputhub_wrapper/inputhub_wrapper.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_ipc.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "iomcu_log.h"

#define CA_TYPE_DEFAULT       (-1)
#define MAX_APP_CONFIG_LENGTH 16

static unsigned int g_fp_ref_cnt;
static bool g_fingerprint_status[FINGERPRINT_TYPE_END];
static wait_queue_head_t g_ipc_wait;
static unsigned int g_fingerprint_ipc_cbge_handle;

static DEFINE_MUTEX(g_fingerprint_ipc_cbge_handle_mutex);
static struct wakeup_source wlock;

struct fingerprint_cmd_map {
	int fhb_ioctl_app_cmd;
	int ca_type;
	int tag;
	enum obj_cmd cmd;
};

static const struct fingerprint_cmd_map g_fingerprint_cmd_map_tab[] = {
	{ FHB_IOCTL_FP_START, CA_TYPE_DEFAULT, TAG_FP, CMD_CMN_OPEN_REQ },
	{ FHB_IOCTL_FP_STOP, CA_TYPE_DEFAULT, TAG_FP, CMD_CMN_CLOSE_REQ },
	{ FHB_IOCTL_FP_DISABLE_SET, CA_TYPE_DEFAULT, TAG_FP,
	FHB_IOCTL_FP_DISABLE_SET_CMD },
};

static struct inputhub_route_table g_fhb_route_table = {
	"fingerprint channel", { NULL, 0 }, { NULL, 0 }, { NULL, 0 },
	__WAIT_QUEUE_HEAD_INITIALIZER(g_fhb_route_table.read_wait)
};

void fingerprint_ipc_cgbe_abort_handle(void)
{
	mutex_lock(&g_fingerprint_ipc_cbge_handle_mutex);
	g_fingerprint_ipc_cbge_handle = 1; /* 1:There is data to read */
	mutex_unlock(&g_fingerprint_ipc_cbge_handle_mutex);
	wake_up_interruptible_sync_poll(&g_ipc_wait, POLLIN);
}

static void update_fingerprint_info(enum obj_cmd cmd, fingerprint_type_t type)
{
	switch (cmd) {
	case CMD_CMN_OPEN_REQ:
		g_fingerprint_status[type] = true;
		ctxhub_err("fingerprint: CMD_CMN_OPEN_REQ in %s, type:%d, %d\n",
			__func__, type, g_fingerprint_status[type]);
		break;
	case CMD_CMN_CLOSE_REQ:
		g_fingerprint_status[type] = false;
		ctxhub_err("fingerprint: CMD_CMN_CLOSE_REQ in %s, type:%d, %d\n",
			__func__, type, g_fingerprint_status[type]);
		break;
	default:
		ctxhub_err("fingerprint: unknown cmd type in %s, type:%d\n",
			__func__, type);
		break;
	}
}

static void fingerprint_report(void)
{
	int ret;
	fingerprint_upload_pkt_t fingerprint_upload;

	(void)memset_s(&fingerprint_upload, sizeof(fingerprint_upload),
			0, sizeof(fingerprint_upload));
	fingerprint_upload.fhd.hd.tag = TAG_FP;
	fingerprint_upload.fhd.hd.cmd = CMD_DATA_REQ;
	fingerprint_upload.data = 0; /* 0 : cancel wait sensorhub msg */

	fingerprint_ipc_cgbe_abort_handle();
	ret = inputhub_route_write(&g_fhb_route_table, (char *)(&fingerprint_upload.data),
			     sizeof(fingerprint_upload.data));
	if (ret <= 0)
		ctxhub_err("%s inputhub_route_write failed\n", __func__);
}

static int inputhub_process_fingerprint_report(const struct pkt_header *head)
{
	char *fingerprint_data = NULL;
	const fingerprint_upload_pkt_t *fingerprint_data_upload;
	int app_tag = inputhub_wrapper_get_app_tag(head);
	int ret;

	fingerprint_data_upload = (const fingerprint_upload_pkt_t *)head;
	if (fingerprint_data_upload == NULL) {
		ctxhub_err("%s fingerprint_data_upload is NULL\n", __func__);
		return -EINVAL;
	}

	__pm_wakeup_event(&wlock, jiffies_to_msecs(2 * HZ)); /* 2ms */

	ctxhub_info("fingerprint: %s: tag = %d, data:%d\n", __func__,
		app_tag,
		fingerprint_data_upload->data);
	fingerprint_data = (char *)fingerprint_data_upload +
		sizeof(pkt_common_data_t);

	fingerprint_ipc_cgbe_abort_handle();

	ret = inputhub_route_write(&g_fhb_route_table,
		fingerprint_data,
		sizeof(fingerprint_data_upload->data));
	if (ret <= 0)
		return RET_FAIL;

	return RET_SUCC;
}

static int send_fingerprint_cmd_internal(int tag, enum obj_cmd cmd,
					 fingerprint_type_t type, bool use_lock)
{
	interval_param_t interval_param;
	/* 16 : max cmd array length */
	u8 app_config[MAX_APP_CONFIG_LENGTH] = { 0 };

	(void)memset_s(&interval_param, sizeof(interval_param), 0,
		sizeof(interval_param));
	update_fingerprint_info(cmd, type);
	if (cmd == CMD_CMN_OPEN_REQ) {
		if (!really_do_enable_disable(&g_fp_ref_cnt, true, type))
			return 0;

		app_config[0] = SUB_CMD_FINGERPRINT_OPEN_REQ;
		(void)inputhub_device_enable(tag, true);
		(void)inputhub_wrapper_send_cmd(tag, CMD_CMN_CONFIG_REQ, app_config,
						sizeof(app_config), NULL);
		ctxhub_info("fingerprint:%s: CMD_CMN_OPEN_REQ cmd:%d\n",
			 __func__, cmd);
	} else if (cmd == CMD_CMN_CLOSE_REQ) {
		if (!really_do_enable_disable(&g_fp_ref_cnt, false, type))
			return 0;

		app_config[0] = SUB_CMD_FINGERPRINT_CLOSE_REQ;
		(void)inputhub_wrapper_send_cmd(tag, CMD_CMN_CONFIG_REQ, app_config,
						sizeof(app_config), NULL);
		(void)inputhub_device_enable(tag, false);
		ctxhub_info("fingerprint:%s: CMD_CMN_CLOSE_REQ cmd:%d\n",
			 __func__, cmd);
	} else if (cmd == FHB_IOCTL_FP_DISABLE_SET_CMD) {
		fingerprint_report();
		ctxhub_info("fingerprint:%s: CMD_FINGERPRINT_DISABLE_SET cmd:%d\n",
			 __func__, cmd);
	} else {
		ctxhub_err("fingerprint:%s: unknown cmd\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int send_fingerprint_cmd(unsigned int cmd, unsigned long arg)
{
	uintptr_t arg_tmp = (uintptr_t)arg;
	void __user *argp = (void __user *)arg_tmp;
	int arg_value = 0;
	int i;
	const int len = ARRAY_SIZE(g_fingerprint_cmd_map_tab);

	/* delete flag_for_sensor_test here */

	ctxhub_info("fingerprint: %s enter\n", __func__);
	for (i = 0; i < len; i++) {
		if (cmd == g_fingerprint_cmd_map_tab[i].fhb_ioctl_app_cmd)
			break;
	}

	if (i == len) {
		ctxhub_err("fingerprint:%s unknown cmd %d in parse_ca_cmd\n",
			__func__, cmd);
		return -EFAULT;
	}

	if (copy_from_user(&arg_value, argp, sizeof(arg_value))) {
		ctxhub_err("fingerprint:%s copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	if (!(arg_value >= FINGERPRINT_TYPE_START &&
	      arg_value < FINGERPRINT_TYPE_END)) {
		ctxhub_err("error fingerprint type %d in %s\n",
			arg_value, __func__);
		return -EINVAL;
	}

	return send_fingerprint_cmd_internal(g_fingerprint_cmd_map_tab[i].tag,
			g_fingerprint_cmd_map_tab[i].cmd, arg_value, true);
}

static void enable_fingerprint_when_recovery_iom3(void)
{
	fingerprint_type_t type;

	g_fp_ref_cnt = 0;
	for (type = FINGERPRINT_TYPE_START;
		type < FINGERPRINT_TYPE_END; ++type) {
		if (g_fingerprint_status[type]) {
			ctxhub_info("fingerprint: finger state %d in %s\n", type,
				 __func__);
			send_fingerprint_cmd_internal(TAG_FP, CMD_CMN_OPEN_REQ,
						      type, false);
		}
	}
}

void disable_fingerprint_when_sysreboot(void)
{
	fingerprint_type_t type;

	for (type = FINGERPRINT_TYPE_START;
		type < FINGERPRINT_TYPE_END; ++type) {
		if (g_fingerprint_status[type]) {
			ctxhub_info("fingerprint: finger state %d in %s\n", type,
				 __func__);
			send_fingerprint_cmd_internal(TAG_FP,
						      CMD_CMN_CLOSE_REQ, type, false);
		}
	}
}

/* read /dev/fingerprinthub */
static ssize_t fhb_read(struct file *file, char __user *buf, size_t count,
			loff_t *pos)
{
	ssize_t ret;

	ret = inputhub_route_read(&g_fhb_route_table, buf, count);
	mutex_lock(&g_fingerprint_ipc_cbge_handle_mutex);
	g_fingerprint_ipc_cbge_handle = 0; /* 0:There is not data to read */
	mutex_unlock(&g_fingerprint_ipc_cbge_handle_mutex);
	return ret;
}

/* write to /dev/fingerprinthub, do nothing now */
static ssize_t fhb_write(struct file *file, const char __user *data,
			 size_t len, loff_t *ppos)
{
	int ret;
	fingerprint_req_t fp_pkt;
	struct write_info pkg_ap;

	(void)memset_s(&fp_pkt, sizeof(fp_pkt), 0, sizeof(fp_pkt));
	(void)memset_s(&pkg_ap, sizeof(pkg_ap), 0, sizeof(pkg_ap));

	if (len > sizeof(fp_pkt.buf)) {
		ctxhub_warn("fingerprint:%s len is out of size, len=%lu\n",
			 __func__, len);
		return -EINVAL;
	}
	if (copy_from_user(fp_pkt.buf, data, len)) {
		ctxhub_warn("fingerprint:%s copy_from_user failed\n", __func__);
		return -EFAULT;
	}
	fp_pkt.len = len;
	fp_pkt.sub_cmd = SUB_CMD_FINGERPRINT_CONFIG_SENSOR_DATA_REQ;

	ctxhub_info("fingerprint:%s data=%d, len=%lu\n", __func__,
		 fp_pkt.buf[0], len);

	ret = inputhub_wrapper_send_cmd(TAG_FP, CMD_CMN_CONFIG_REQ, &fp_pkt, sizeof(fp_pkt), NULL);
	if (ret)
		ctxhub_err("%s fail,ret=%d\n", __func__, ret);

	return len;
}

/*
 * Description:    ioctrl function to /dev/fingerprinthub, do open, close ca,
 *                 or set interval and attribute to fingerprinthub
 * Return:         result of ioctrl command, 0 succeeded, -ENOTTY failed
 */
static long fhb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case FHB_IOCTL_FP_START:
		ctxhub_err("fingerprint:%s cmd: FHB_IOCTL_FP_START\n", __func__);
		break;
	case FHB_IOCTL_FP_STOP:
		ctxhub_err("fingerprint:%s cmd: FHB_IOCTL_FP_STOP\n", __func__);
		break;
	case FHB_IOCTL_FP_DISABLE_SET:
		ctxhub_err("fingerprint:%s set cmd : FHB_IOCTL_FP_DISABLE_SET\n",
			__func__);
		break;
	default:
		ctxhub_err("fingerprint:%s unknown cmd : %d\n", __func__, cmd);
		return -ENOTTY;
	}
	return send_fingerprint_cmd(cmd, arg);
}

/*
 * Description:    open to /dev/fingerprinthub
 * Return:         result 0 succeeded
 */
static int fhb_open(struct inode *inode, struct file *file)
{
	ctxhub_info("fingerprint: enter %s\n", __func__);
	return 0;
}

static int fingerprint_recovery_notifier(struct notifier_block *nb,
					 unsigned long foo, void *bar)
{
	/* prevent access the emmc now: */
	ctxhub_info("%s %lu +\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		enable_fingerprint_when_recovery_iom3();
		break;
	default:
		ctxhub_err("%s -unknown state %ld\n", __func__, foo);
		break;
	}
	ctxhub_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block g_fingerprint_recovery_notify = {
	.notifier_call = fingerprint_recovery_notifier,
	.priority = -1,
};

/* fhb_poll CBGE */
static unsigned int fhb_poll(struct file *file, poll_table *wait)
{
	unsigned int mask;

	poll_wait(file, &g_ipc_wait, wait);
	mutex_lock(&g_fingerprint_ipc_cbge_handle_mutex);
	mask = g_fingerprint_ipc_cbge_handle ? POLLIN : 0;
	mutex_unlock(&g_fingerprint_ipc_cbge_handle_mutex);
	return mask;
}

/* file_operations to fingerprinthub */
static const struct file_operations g_fhb_fops = {
	.owner = THIS_MODULE,
	.read = fhb_read,
	.write = fhb_write,
	.unlocked_ioctl = fhb_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = fhb_ioctl,
#endif
	.open = fhb_open,
	.poll = fhb_poll,
};

/* miscdevice to fingerprinthub */
static struct miscdevice fingerprinthub_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fingerprinthub",
	.fops = &g_fhb_fops,
};

/*
 * Description: apply kernel buffer, register fingerprinthub_miscdev
 * Return:      result of function, 0 succeeded, else false
 */
static int __init fingerprinthub_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -1;

	ret = inputhub_route_open(&g_fhb_route_table);
	if (ret != 0) {
		ctxhub_err("cannot open fhb_route_table route err=%d\n", ret);
		return ret;
	}

	ret = inputhub_wrapper_register_event_notifier(TAG_FP, CMD_DATA_REQ,
						       inputhub_process_fingerprint_report);
	if (ret != 0) {
		ctxhub_err("%s: register event notifier failed\n", __func__);
		goto out_close_channel;
	}

	ret = register_iom3_recovery_notifier(&g_fingerprint_recovery_notify);
	if (ret != 0) {
		ctxhub_err("%s, register fingerprint recovery notifer failed!\n", __func__);
		ret = RET_FAIL;
		goto out_unregister_event_notifier;
	}

	init_waitqueue_head(&g_ipc_wait);

	wakeup_source_init(&wlock, "fingerprinthub");

	ret = misc_register(&fingerprinthub_miscdev);
	if (ret != 0) {
		ctxhub_err("%s cannot register miscdev err=%d\n", __func__, ret);
		goto out_wakeup_source_release;
	}

	ctxhub_info("%s ok\n", __func__);

	return 0;

out_wakeup_source_release:
	wakeup_source_trash(&wlock);
out_unregister_event_notifier:
	inputhub_wrapper_unregister_event_notifier(TAG_FP, CMD_DATA_REQ,
						inputhub_process_fingerprint_report);
out_close_channel:
	inputhub_route_close(&g_fhb_route_table);

	return ret;
}

/* release kernel buffer, deregister fingerprinthub_miscdev */
static void __exit fingerprinthub_exit(void)
{
	misc_deregister(&fingerprinthub_miscdev);
	wakeup_source_trash(&wlock);
	inputhub_wrapper_unregister_event_notifier(TAG_FP, CMD_DATA_REQ,
						   inputhub_process_fingerprint_report);
	inputhub_route_close(&g_fhb_route_table);
	ctxhub_info("exit %s\n", __func__);
}

late_initcall_sync(fingerprinthub_init);
module_exit(fingerprinthub_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("FPHub driver");
MODULE_AUTHOR("Smart");
