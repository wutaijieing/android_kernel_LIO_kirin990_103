/*
 * callback.c
 *
 * show sr task stack while watchdog dead
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "pm.h"
#include "pub_def.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/debugfs/debugfs.h"

#include <securec.h>
#include <platform_include/basicplatform/linux/dfx_universal_wdt.h>

#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/sched/debug.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <log/log_usertype.h>

static struct task_struct *sr_task;
static struct timer_list sr_timer;
/* CONFIG_SR_FLOW_TIMEOUT is defined in deconfig file */
static int g_sr_flow_timeout = CONFIG_SR_FLOW_TIMEOUT;
static int g_sr_fail_stack_times = 0;
#define SR_FLOW_TIMEOUT_MAX 3600

static void show_sr_stack(const char *info)
{
	if (sr_task == NULL) {
		lp_crit("not in sr flow");
		return;
	}

	lp_crit("%s the sr task stack is:", info);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	show_stack(sr_task, NULL, KERN_ERR);
#else
	show_stack(sr_task, NULL);
#endif
}

static int sr_timer_start(void)
{
	lp_err("sr_timer_start");
	int timeout = g_sr_flow_timeout;

	if (timeout < 0)
		timeout *= -1;
	else
		timeout *= HZ;

	if (mod_timer(&sr_timer, jiffies + timeout) != 0)
		lp_err("start sr timer failed");

	return 0;
}
static int sr_timer_stop(void)
{
	lp_err("sr_timer_stop");
	g_sr_fail_stack_times = 0;
	(void)del_timer(&sr_timer);

	return 0;
}

#define SR_TIMER_DATA_TYPE struct timer_list *
#define sr_timer_setup(timer, callback, flags) timer_setup(timer, callback, flags)

static void sr_timer_timeout(SR_TIMER_DATA_TYPE timer)
{
	unsigned int user_type;
	no_used(timer);
	if (g_sr_fail_stack_times < 3) {
		show_sr_stack("sr flow time out: ");
		(void)sr_timer_start();
		g_sr_fail_stack_times++;
	} else {
		user_type = get_log_usertype();
		if (user_type == BETA_USER || user_type == OVERSEA_USER)
			panic("sr fail in kernel\n");
	}
}

static int pm_sr_notify(struct notifier_block *nb, unsigned long mode, void *data)
{
	no_used(data);

	if (mode == PM_SUSPEND_PREPARE) {
		sr_task = current;
		(void)sr_timer_start();
		return 0;
	}
	if (mode == PM_POST_SUSPEND) {
		sr_task = NULL;
		(void)sr_timer_stop();
		return 0;
	}

	return 0;
}

static struct notifier_block pm_sr_nb = {
	.notifier_call = pm_sr_notify,
};

static int wdt_sr_notify(struct notifier_block *nb, unsigned long mode, void *data)
{
	no_used(nb);
	no_used(mode);
	no_used(data);

	show_sr_stack("watchdog maybe dead in sr flow: ");

	return 0;
}

static struct notifier_block wdt_sr_nb = {
	.notifier_call = wdt_sr_notify,
};

static struct sr_mntn g_sr_mntn_showtask = {
	.owner = "sr_wdt",
	.enable = true,
	.suspend = sr_timer_stop,
	.resume = sr_timer_start,
};

#ifdef CONFIG_SR_DEBUG
static int timeout_show(struct seq_file *s, void *data)
{
	no_used(data);

	lp_msg(s, "sr timeout is %d seconds", g_sr_flow_timeout);

	return 0;
}
#define INPUT_BUF_LEN 16
static ssize_t timeout_store(struct seq_file *s, const char __user *buffer,
			     size_t count, loff_t *ppos)
{
	int timeout = 0;
	const int correct_columns = 1;

	char input_buf[INPUT_BUF_LEN] = {0};

	if (count >= INPUT_BUF_LEN) {
		lp_err("the input content is too long");
		return -EINVAL;
	}

	if (buffer != NULL && copy_from_user(input_buf, buffer, count) != 0) {
		lp_err("internal error: copy mem fail");
		return -ENOMEM;
	}
	input_buf[count] = '\0';

	if (sscanf_s(input_buf, "%d", &timeout) != correct_columns) {
		lp_err("invalid input");
		return -EINVAL;
	}
	*ppos += count;

	g_sr_flow_timeout = timeout;

	lp_info("change sr timeout to %d success", g_sr_flow_timeout);

	return count;
}
static const struct lowpm_debugdir g_lpwpm_debugfs_srtimeout = {
	.dir = "lowpm_func",
	.files = {
		{"sr_timeout", 0600, timeout_show, timeout_store},
		{},
	},
};
#endif

static __init int init_sr_mntn_wdt(void)
{
	int ret;

	sr_task = NULL;

	if (sr_unsupported())
		return 0;

	wdt_register_notify(&wdt_sr_nb);

	if (register_pm_notifier(&pm_sr_nb) != 0) {
		lp_err("register_pm_notifier failed");
		return -ENODEV;
	}

	ret = register_sr_mntn(&g_sr_mntn_showtask, SR_MNTN_PRIO_C);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

#ifdef CONFIG_SR_DEBUG
	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_srtimeout);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}
#endif

	sr_timer_setup(&sr_timer, sr_timer_timeout, 0);

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_wdt);
