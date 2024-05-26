/*
 * usbaudio_mailbox.c
 *
 * usbaudio mailbox driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "usbaudio_mailbox.h"

#include <linux/usb.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <linux/pm_wakeup.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
#include <uapi/linux/sched/types.h>
#endif
#ifdef CONFIG_DFX_DEBUG_FS
#include <linux/debugfs.h>
#endif

#include "../hifi/usbaudio_setinterface.h"
#include "../hifi/usbaudio_ioctl.h"
#include "audio_log.h"
#ifdef CONFIG_HIFI_MAILBOX
#include "drv_mailbox_cfg.h"
#endif

#ifndef LOG_TAG
#define LOG_TAG "usbaudio"
#endif

struct completion g_probe_msg_complete;
struct completion g_disconnect_msg_complete;
struct completion g_nv_check;
struct wakeup_source *g_rcv_wake_lock;
static atomic_t g_nv_check_ref = ATOMIC_INIT(0);

#define HIFI_USBAUDIO_MSG_TIMEOUT (2 * HZ)
#define HIFI_USBAUDIO_NV_CHECK_TIMEOUT ((1 * HZ) / 20)

#define ERROR    (-1)
#define INVALID  (-4)

struct interface_set_mesg {
	unsigned int dir;
	unsigned int running;
	unsigned int rate;
	unsigned int period;
	struct list_head node;
};

struct usbaudio_msg_proc {
	struct task_struct *kthread;
	struct interface_set_mesg interface_msg_list;
	struct semaphore proc_sema;
	struct wakeup_source *msg_proc_wake_lock;
};

static struct usbaudio_msg_proc g_msg_proc;

static void nv_check_work(struct work_struct *wk)
{
	usbaudio_ctrl_nv_check();
}
static DECLARE_WORK(nv_check_work_queue, nv_check_work);

int usbaudio_mailbox_send_data(const void *pmsg_body,
	unsigned int msg_len, unsigned int msg_priority)
{
	unsigned int ret = 0;

#ifdef CONFIG_HIFI_MAILBOX
	ret = DRV_MAILBOX_SENDMAIL(MAILBOX_MAILCODE_ACPU_TO_HIFI_USBAUDIO, pmsg_body, msg_len);
	if (ret != 0)
		AUDIO_LOGE("usbaudio channel send mail failed, ret: %u", ret);
#endif
	return (int)ret;
}

static void usbaudio_mailbox_msg_add(unsigned int dir,
	unsigned int running, unsigned int rate, unsigned int period)
{
	struct interface_set_mesg *interface_msg = NULL;

	interface_msg = kzalloc(sizeof(*interface_msg), GFP_ATOMIC);
	if (interface_msg) {
		interface_msg->running = running;
		interface_msg->dir = dir;
		interface_msg->rate = rate;
		interface_msg->period = period;
		list_add_tail(&interface_msg->node, &g_msg_proc.interface_msg_list.node);
		up(&g_msg_proc.proc_sema);
	} else {
		AUDIO_LOGE("malloc failed");
	}
}

static void usbaudio_mailbox_recv_proc(const void *usr_para,
	struct mb_queue *mail_handle, unsigned int mail_len)
{
	struct usbaudio_rcv_msg rcv_msg;
	unsigned int ret = 0;
	unsigned int mail_size = mail_len;

	memset(&rcv_msg, 0, sizeof(rcv_msg)); /* unsafe_function_ignore: memset */

#ifdef CONFIG_HIFI_MAILBOX
	ret = DRV_MAILBOX_READMAILDATA(mail_handle, (unsigned char *)&rcv_msg, &mail_size);
	if ((ret != 0) || (mail_size == 0) || (mail_size > sizeof(rcv_msg))) {
		AUDIO_LOGE("mailbox read error, read result: %u, read mail size: %u",
			ret, mail_size);
		return;
	}
#endif
	__pm_wakeup_event(g_rcv_wake_lock, 1000);
	switch (rcv_msg.msg_type) {
	case USBAUDIO_CHN_MSG_PROBE_RCV:
		AUDIO_LOGI("receive message: probe succ");
		complete(&g_probe_msg_complete);
		break;
	case USBAUDIO_CHN_MSG_DISCONNECT_RCV:
		AUDIO_LOGI("receive message: disconnect succ");
		complete(&g_disconnect_msg_complete);
		break;
	case USBAUDIO_CHN_MSG_NV_CHECK_RCV:
		AUDIO_LOGI("receive message: nv check succ");
		atomic_inc(&g_nv_check_ref);
		schedule_work(&nv_check_work_queue);
		complete(&g_nv_check);
		break;
	case USBAUDIO_CHN_MSG_PIPEOUTINTERFACE_ON_RCV:
		AUDIO_LOGI("receive message: pipout on");
		usbaudio_mailbox_msg_add(DOWNLINK_STREAM, START_STREAM,
			rcv_msg.rate, rcv_msg.period);
		break;
	case USBAUDIO_CHN_MSG_PIPEOUTINTERFACE_OFF_RCV:
		AUDIO_LOGI("receive message: pipout off");
		usbaudio_mailbox_msg_add(DOWNLINK_STREAM, STOP_STREAM,
			rcv_msg.rate, rcv_msg.period);
		break;
	case USBAUDIO_CHN_MSG_PIPEININTERFACE_ON_RCV:
		AUDIO_LOGI("receive message: pipein on");
		usbaudio_mailbox_msg_add(UPLINK_STREAM, START_STREAM,
			rcv_msg.rate, rcv_msg.period);
		break;
	case USBAUDIO_CHN_MSG_PIPEININTERFACE_OFF_RCV:
		AUDIO_LOGI("receive message: pipein off");
		usbaudio_mailbox_msg_add(UPLINK_STREAM, STOP_STREAM,
			rcv_msg.rate, rcv_msg.period);
		break;
	default:
		AUDIO_LOGE("msg type: %u", rcv_msg.msg_type);
		break;
	}
}

static int usbaudio_mailbox_isr_register(mb_msg_cb receive_func)
{
	int ret = 0;

	if (!receive_func) {
		AUDIO_LOGE("receive func is null");
		return ERROR;
	}

#ifdef CONFIG_HIFI_MAILBOX
	ret = DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_USBAUDIO,
		receive_func, NULL);
	if (ret != 0) {
		AUDIO_LOGE("register receive func error, ret: %d, mailcode: 0x%x",
			ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_USBAUDIO);
		return ERROR;
	}
#endif
	return ret;
}

static int usbaudio_mailbox_isr_unregister(void)
{
	int ret = 0;

#ifdef CONFIG_HIFI_MAILBOX
	ret = DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_USBAUDIO,
		NULL, NULL);
	if (ret != 0) {
		AUDIO_LOGE("unregister receive func error, ret: %d, mailcode: 0x%x",
			ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_USBAUDIO);
		return ERROR;
	}
#endif
	return ret;
}

int usbaudio_setinterface_complete_msg(int dir, int val, int retval, unsigned int rate)
{
	int ret;
	struct usbaudio_setinterface_complete_msg complete_msg;

	memset(&complete_msg, 0, sizeof(complete_msg));

	complete_msg.dir = dir;
	complete_msg.val = val;
	complete_msg.rate = rate;
	complete_msg.ret = retval;
	complete_msg.msg_type = USBAUDIO_CHN_MSG_SETINTERFACE_COMPLETE;

	ret = usbaudio_mailbox_send_data(&complete_msg, sizeof(complete_msg), 0);

	return ret;
}

int usbaudio_nv_is_ready(void)
{
	int ret = 1;

	if (atomic_read(&g_nv_check_ref) >= 1)
		ret = 0;

	return ret;
}

void usbaudio_set_nv_ready(void)
{
	AUDIO_LOGI("nv check set");

	atomic_set(&g_nv_check_ref, 0);
	atomic_inc(&g_nv_check_ref);
}

int usbaudio_nv_check(void)
{
	int ret;
	unsigned long retval;
	struct usbaudio_nv_check_msg nv_check_msg;

	if (atomic_read(&g_nv_check_ref) >= 1)
		return 0;

	memset(&nv_check_msg, 0, sizeof(nv_check_msg));

	AUDIO_LOGI("nv check msg");
	init_completion(&g_nv_check);

	nv_check_msg.msg_type = (unsigned short)USBAUDIO_CHN_MSG_NV_CHECK;

	ret = usbaudio_mailbox_send_data(&nv_check_msg, sizeof(nv_check_msg), 0);
	retval = wait_for_completion_timeout(&g_nv_check, HIFI_USBAUDIO_NV_CHECK_TIMEOUT);
	if (retval == 0) {
		AUDIO_LOGE("usbaudio nv check timeout");
		return -ETIME;
	} else {
		AUDIO_LOGE("usbaudio nv check intime");
		ret = INVALID;
	}

	return ret;
}

int usbaudio_probe_msg(struct usbaudio_pcms *pcms)
{
	int ret;
	int i;
	unsigned long retval;
	struct usbaudio_probe_msg probe_msg;

	memset(&probe_msg, 0, sizeof(probe_msg)); /* unsafe_function_ignore: memset */
	AUDIO_LOGI("msg len %lu", sizeof(probe_msg));
	init_completion(&g_probe_msg_complete);

	probe_msg.msg_type = (unsigned short)USBAUDIO_CHN_MSG_PROBE;
	for (i = 0; i < USBAUDIO_PCM_NUM; i++) {
		memcpy((void *)(&probe_msg.pcms.fmts[i]),
			(void *)(&pcms->fmts[i]), sizeof(pcms->fmts[i]));
		memcpy((void *)(&probe_msg.pcms.ifdesc[i]),
			(void *)(&pcms->ifdesc[i]), sizeof(pcms->ifdesc[i]));
		memcpy((void *)(&probe_msg.pcms.epdesc[i]),
			(void *)(&pcms->epdesc[i]), sizeof(pcms->epdesc[i]));
	}
	probe_msg.pcms.customsized = pcms->customsized;
	ret = usbaudio_mailbox_send_data(&probe_msg, sizeof(struct usbaudio_probe_msg), 0);

	retval = wait_for_completion_timeout(&g_probe_msg_complete, HIFI_USBAUDIO_MSG_TIMEOUT);
	if (retval == 0) {
		AUDIO_LOGE("usbaudio probe msg send timeout");
		return -ETIME;
	}

	return ret;
}

int usbaudio_disconnect_msg(unsigned int dsp_reset_flag)
{
	int ret;
	unsigned long retval;
	struct usbaudio_disconnect_msg disconnect_msg;

	memset(&disconnect_msg, 0, sizeof(disconnect_msg));

	AUDIO_LOGI("reset flag %u, len %lu", dsp_reset_flag, sizeof(disconnect_msg));
	init_completion(&g_disconnect_msg_complete);
	disconnect_msg.msg_type = (unsigned short)USBAUDIO_CHN_MSG_DISCONNECT;
	disconnect_msg.dsp_reset_flag = dsp_reset_flag;
	ret = usbaudio_mailbox_send_data(&disconnect_msg, sizeof(disconnect_msg), 0);

	retval = wait_for_completion_timeout(&g_disconnect_msg_complete,
		HIFI_USBAUDIO_MSG_TIMEOUT);
	if (retval == 0) {
		AUDIO_LOGE("send timeout");
		return -ETIME;
	}

	return ret;
}

static int interface_msg_proc_thread(void *p)
{
	int ret;
	struct interface_set_mesg *set_mesg = NULL;

	while (!kthread_should_stop()) {
		ret = down_interruptible(&g_msg_proc.proc_sema);
		if (ret == -ETIME)
			AUDIO_LOGE("proc sema down interruptible err, ret: %d", ret);

		__pm_stay_awake(g_msg_proc.msg_proc_wake_lock);
		if (list_empty(&g_msg_proc.interface_msg_list.node)) {
			AUDIO_LOGE("interface msg list is empty");
			__pm_relax(g_msg_proc.msg_proc_wake_lock);
			continue;
		}

		set_mesg = list_entry(g_msg_proc.interface_msg_list.node.next,
			struct interface_set_mesg, node);
		if (set_mesg) {
			AUDIO_LOGI("[0: out 1: in]%u [0: start 1: stop]%u rate: %u period: %u",
				set_mesg->dir, set_mesg->running,
				set_mesg->rate, set_mesg->period);
			if (set_mesg->dir == 0)
				usbaudio_ctrl_set_pipeout_interface(set_mesg->running,
					set_mesg->rate);
			else
				usbaudio_ctrl_set_pipein_interface(set_mesg->running,
					set_mesg->rate, set_mesg->period);

			list_del(&set_mesg->node);
			kfree(set_mesg);
		} else {
			AUDIO_LOGE("set mesg is null");
		}

		__pm_relax(g_msg_proc.msg_proc_wake_lock);
	}

	return 0;
}

static void usbaudio_wakeup_source_deinit(void)
{
	wakeup_source_unregister(g_msg_proc.msg_proc_wake_lock);
	g_msg_proc.msg_proc_wake_lock = NULL;
	wakeup_source_unregister(g_rcv_wake_lock);
	g_rcv_wake_lock = NULL;
}

static int usbaudio_wakeup_source_init(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	g_msg_proc.msg_proc_wake_lock = wakeup_source_register(NULL, "usbaudio_msg_proc");
#else
	g_msg_proc.msg_proc_wake_lock = wakeup_source_register("usbaudio_msg_proc");
#endif
	if (g_msg_proc.msg_proc_wake_lock == NULL)
		goto wakeup_source_init_err;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	g_rcv_wake_lock = wakeup_source_register(NULL, "usbaudio_rcv_msg");
#else
	g_rcv_wake_lock = wakeup_source_register("usbaudio_rcv_msg");
#endif
	if (g_rcv_wake_lock == NULL)
		goto wakeup_source_init_err;

	return 0;

wakeup_source_init_err:
	usbaudio_wakeup_source_deinit();
	return -ENOMEM;
}

int usbaudio_mailbox_init(void)
{
	struct sched_param param;
	int ret = 0;

	AUDIO_LOGI("in");

	atomic_set(&g_nv_check_ref, 0);

	ret = usbaudio_mailbox_isr_register(usbaudio_mailbox_recv_proc);
	if (ret != 0) {
		AUDIO_LOGE("usbaudio mailbox receive isr registe failed: %d", ret);
		return -EIO;
	}

	INIT_LIST_HEAD(&g_msg_proc.interface_msg_list.node);

	sema_init(&g_msg_proc.proc_sema, 0);

	ret = usbaudio_wakeup_source_init();
	if (ret) {
		AUDIO_LOGE("request wakeup source failed");
		goto usbaudio_wakeup_source_init_err;
	}

	g_msg_proc.kthread = kthread_create(interface_msg_proc_thread,
		0, "interface_msg_proc_thread");
	if (IS_ERR(g_msg_proc.kthread)) {
		AUDIO_LOGE("create interface msg proc thread fail");
		ret = -ENOMEM;
		goto kthread_create_err;
	}

	/* set high prio */
	memset(&param, 0, sizeof(param));
	param.sched_priority = MAX_RT_PRIO - 20;
	ret = sched_setscheduler(g_msg_proc.kthread, SCHED_RR, &param);
	if (ret != 0) {
		AUDIO_LOGE("set thread schedule priorty failed: %d", param.sched_priority);
		goto set_thread_schedule_err;
	}

	wake_up_process(g_msg_proc.kthread);
	return 0;

set_thread_schedule_err:
	kthread_stop(g_msg_proc.kthread);
	g_msg_proc.kthread = NULL;

kthread_create_err:
	usbaudio_wakeup_source_deinit();

usbaudio_wakeup_source_init_err:
	usbaudio_mailbox_isr_unregister();

	return ret;
}

void usbaudio_mailbox_deinit(void)
{
	struct interface_set_mesg *set_mesg = NULL;
	struct interface_set_mesg *n = NULL;

	AUDIO_LOGI("in");

	if (g_msg_proc.kthread) {
		up(&g_msg_proc.proc_sema);
		kthread_stop(g_msg_proc.kthread);
		g_msg_proc.kthread = NULL;
	}

	list_for_each_entry_safe(set_mesg, n, &g_msg_proc.interface_msg_list.node, node) {
		list_del(&set_mesg->node);
		kfree(set_mesg);
	}

	usbaudio_wakeup_source_deinit();
	usbaudio_mailbox_isr_unregister();
}
