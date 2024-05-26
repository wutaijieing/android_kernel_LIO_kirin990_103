/*
 * usb_vendor.c
 *
 * Chip usb notifier
 *
 * Copyright (c) 2015-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "usb_vendor.h"

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/delay.h>
#include <linux/fcntl.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>

/* usb adapter for charger */
struct usb_notifer_ctx {
	int usb_status;
	int usb_old_status;
	unsigned int stat_usb_insert;
	unsigned int stat_usb_insert_proc;
	unsigned int stat_usb_insert_proc_end;
	unsigned int stat_usb_enum_done;
	unsigned int stat_usb_enum_done_proc;
	unsigned int stat_usb_enum_done_proc_end;
	unsigned int stat_usb_remove;
	unsigned int stat_usb_remove_proc;
	unsigned int stat_usb_remove_proc_end;
	unsigned int stat_usb_disable;
	unsigned int stat_usb_disable_proc;
	unsigned int stat_usb_disable_proc_end;
	unsigned int stat_usb_no_need_notify;
	unsigned int stat_wait_cdev_created;

	struct workqueue_struct *usb_notify_wq;
	struct delayed_work usb_notify_wk;
};

static BLOCKING_NOTIFIER_HEAD(usb_modem_notifier_list);
static struct usb_notifer_ctx g_usb_notifier;

/* usb enum done management */

#define USB_FD_DEVICE_MAX		32

struct usb_enum_stat {
	const char *fd_name;                  /* function drvier file name */
	unsigned int usb_intf_id;           /* usb interface id */
	unsigned int is_enum;               /* whether the dev is enum */
};

static struct usb_enum_stat g_usb_devices_enum_stat[USB_FD_DEVICE_MAX];
static unsigned int g_usb_dev_enum_done_num;
static unsigned int g_usb_dev_setup_num;

/* usb enum done management implement */
void bsp_usb_remove_setup_dev_fdname(void)
{
	if (g_usb_dev_setup_num > 0)
		g_usb_dev_setup_num--;

	if (g_usb_dev_setup_num == 0) {
		g_usb_dev_enum_done_num = 0;
		memset(g_usb_devices_enum_stat, 0,
			sizeof(g_usb_devices_enum_stat));
	}
}

void bsp_usb_add_setup_dev_fdname(unsigned int intf_id, const char *fd_name)
{
	if (g_usb_dev_setup_num >= USB_FD_DEVICE_MAX) {
		pr_err("%s error, setup_num:%u, USB_FD_DEVICE_MAX:%d\n",
			__func__, g_usb_dev_setup_num, USB_FD_DEVICE_MAX);
		return;
	}
	g_usb_devices_enum_stat[g_usb_dev_setup_num].usb_intf_id = intf_id;
	g_usb_devices_enum_stat[g_usb_dev_setup_num].fd_name = fd_name;
	g_usb_dev_setup_num++;
}

int bsp_usb_is_all_enum(void)
{
	return (g_usb_dev_setup_num != 0
		&& g_usb_dev_setup_num == g_usb_dev_enum_done_num);
}

int bsp_usb_is_all_disable(void)
{
	return (g_usb_dev_setup_num != 0
		&& g_usb_dev_enum_done_num == 0);
}

void bsp_usb_set_enum_stat(unsigned int intf_id, int enum_stat)
{
	unsigned int i;
	struct usb_enum_stat *find_dev = NULL;

	if (enum_stat) {
		/* if all dev is already enum, do nothing */
		if (bsp_usb_is_all_enum())
			return;

		for (i = 0; i < g_usb_dev_setup_num; i++) {
			if (g_usb_devices_enum_stat[i].usb_intf_id == intf_id) {
				find_dev = &g_usb_devices_enum_stat[i];
				break;
			}
		}
		if (find_dev != NULL) {
			/* if the dev is already enum, do nothing */
			if (find_dev->is_enum)
				return;
			find_dev->is_enum = (unsigned int)enum_stat;
			g_usb_dev_enum_done_num++;

			/* after change stat, if all dev enum done, notify callback */
			if (bsp_usb_is_all_enum())
				bsp_usb_status_change(USB_MODEM_ENUM_DONE);
		}
	} else {
		if (bsp_usb_is_all_disable())
			return;

		for (i = 0; i < g_usb_dev_setup_num; i++) {
			if (g_usb_devices_enum_stat[i].usb_intf_id == intf_id) {
				find_dev = &g_usb_devices_enum_stat[i];
				break;
			}
		}
		if (find_dev != NULL) {
			/* if the dev is already disable, do nothing */
			if (!find_dev->is_enum)
				return;
			find_dev->is_enum = 0U;

			/*
			 * g_usb_dev_enum_done_num is always > 0,
			 * we protect it in check bsp_usb_is_all_disable
			 */
			if (g_usb_dev_enum_done_num > 0)
				g_usb_dev_enum_done_num--;

			/*
			 * if the version is not support pmu detect
			 * and all the device is disable, we assume that
			 * the usb is remove, so notify disable callback,
			 * tell the other modules
			 * else, we use the pmu remove detect.
			 */
			if (bsp_usb_is_all_disable())
				bsp_usb_status_change(USB_MODEM_DEVICE_DISABLE);
		}
	}
}

void bsp_usb_enum_info_dump(void)
{
	unsigned int i;

	pr_info("modem usb is enum done:%d\n", bsp_usb_is_all_enum());
	pr_info("setup_num:%u, enum_done_num:%u\n", g_usb_dev_setup_num,
					g_usb_dev_enum_done_num);
	pr_info("device enum info details:\n\n");
	for (i = 0; i < g_usb_dev_setup_num; i++) {
		pr_info("enum dev:%u\n", i);
		pr_info("usb_intf_id:%u\n",
			g_usb_devices_enum_stat[i].usb_intf_id);
		pr_info("is_enum:%u\n",
			g_usb_devices_enum_stat[i].is_enum);
		if (g_usb_devices_enum_stat[i].fd_name)
			pr_info("fd_name:%s\n\n",
				g_usb_devices_enum_stat[i].fd_name);
	}
}

/* usb charger adapter implement */
void bsp_usb_status_change(int status)
{
	if (status == USB_MODEM_ENUM_DONE)
		g_usb_notifier.stat_usb_enum_done++;
	else if (status == USB_MODEM_DEVICE_INSERT)
		g_usb_notifier.stat_usb_insert++;
	else if (status == USB_MODEM_DEVICE_REMOVE)
		g_usb_notifier.stat_usb_remove++;
	else if (status == USB_MODEM_DEVICE_DISABLE)
		g_usb_notifier.stat_usb_disable++;
	else
		pr_err("%s: error status:%d\n", __func__, status);

	g_usb_notifier.usb_status = status;

	/* usb_notify_handler */
	if (g_usb_notifier.usb_notify_wq)
		queue_delayed_work(g_usb_notifier.usb_notify_wq,
			&g_usb_notifier.usb_notify_wk, 0);
}

static void bsp_usb_insert_process(void)
{
	g_usb_notifier.stat_usb_insert_proc++;

	blocking_notifier_call_chain(&usb_modem_notifier_list,
		USB_MODEM_DEVICE_INSERT, (void *)&g_usb_notifier);
	blocking_notifier_call_chain(&usb_modem_notifier_list,
		USB_MODEM_CHARGER_IDEN, (void *)&g_usb_notifier);

	g_usb_notifier.stat_usb_insert_proc_end++;
}

static void bsp_usb_enum_done_process(void)
{
	pr_info("modem usb enum done\n");
	g_usb_notifier.stat_usb_enum_done_proc++;

	/* notify kernel notifier */
	blocking_notifier_call_chain(&usb_modem_notifier_list,
		USB_MODEM_ENUM_DONE, (void *)&g_usb_notifier);

	g_usb_notifier.stat_usb_enum_done_proc_end++;
}

static void bsp_usb_remove_device_process(void)
{
	pr_info("modem usb remove\n");
	g_usb_notifier.stat_usb_remove_proc++;

	/*
	 * notify kernel notifier,
	 * we must call notifier list before disable callback,
	 * there are something need to do before user
	 */
	blocking_notifier_call_chain(&usb_modem_notifier_list,
		USB_MODEM_DEVICE_REMOVE, (void *)&g_usb_notifier);

	g_usb_notifier.stat_usb_remove_proc_end++;
}

static void bsp_usb_disable_device_process(void)
{
	pr_info("modem usb disable\n");
	g_usb_notifier.stat_usb_disable_proc++;

	/* notify kernel notifier */
	blocking_notifier_call_chain(&usb_modem_notifier_list,
		USB_MODEM_DEVICE_DISABLE, (void *)&g_usb_notifier);

	g_usb_notifier.stat_usb_disable_proc_end++;
}

/*
 * usb_register_notify - register a notifier callback
 * whenever a usb change happens
 * @nb: pointer to the notifier block for the callback events.
 *
 * These changes are either USB devices or busses being added or removed.
 */
void bsp_usb_register_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_register(&usb_modem_notifier_list, nb);
}

/*
 * usb_unregister_notify - unregister a notifier callback
 * @nb: pointer to the notifier block for the callback events.
 *
 * usb_register_notify() must have been previously called for this function
 * to work properly.
 */
void bsp_usb_unregister_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&usb_modem_notifier_list, nb);
}

void bsp_usb_notifier_dump(void)
{
	pr_info("stat_usb_insert:            %u\n",
		g_usb_notifier.stat_usb_insert);
	pr_info("stat_usb_insert_proc:       %u\n",
		g_usb_notifier.stat_usb_insert_proc);
	pr_info("stat_usb_insert_proc_end:   %u\n",
		g_usb_notifier.stat_usb_insert_proc_end);
	pr_info("stat_usb_enum_done:         %u\n",
		g_usb_notifier.stat_usb_enum_done);
	pr_info("stat_usb_enum_done_proc:    %u\n",
		g_usb_notifier.stat_usb_enum_done_proc);
	pr_info("stat_usb_enum_done_proc_end:%u\n",
		g_usb_notifier.stat_usb_enum_done_proc_end);
	pr_info("stat_usb_remove:            %u\n",
		g_usb_notifier.stat_usb_remove);
	pr_info("stat_usb_remove_proc:       %u\n",
		g_usb_notifier.stat_usb_remove_proc);
	pr_info("stat_usb_remove_proc_end:   %u\n",
		g_usb_notifier.stat_usb_remove_proc_end);
	pr_info("stat_usb_disable:           %u\n",
		g_usb_notifier.stat_usb_disable);
	pr_info("stat_usb_disable_proc:      %u\n",
		g_usb_notifier.stat_usb_disable_proc);
	pr_info("stat_usb_disable_proc_end:  %uu\n",
		g_usb_notifier.stat_usb_disable_proc_end);
	pr_info("usb_status:                 %d\n",
		g_usb_notifier.usb_status);
	pr_info("usb_old_status:             %d\n",
		g_usb_notifier.usb_old_status);
	pr_info("stat_wait_cdev_created:     %u\n",
		g_usb_notifier.stat_wait_cdev_created);
	pr_info("stat_usb_no_need_notify:    %u\n",
		g_usb_notifier.stat_usb_no_need_notify);
}

static void usb_notify_handler(struct work_struct *work)
{
	int cur_status = g_usb_notifier.usb_status;

	if (g_usb_notifier.usb_old_status == cur_status) {
		g_usb_notifier.stat_usb_no_need_notify++;
		return;
	}

	switch (cur_status) {
	case USB_MODEM_DEVICE_INSERT:
		bsp_usb_insert_process();
		break;
	case USB_MODEM_ENUM_DONE:
		bsp_usb_enum_done_process();
		break;
	case USB_MODEM_DEVICE_REMOVE:
		bsp_usb_remove_device_process();
		break;
	case USB_MODEM_DEVICE_DISABLE:
		bsp_usb_disable_device_process();
		break;
	default:
		pr_err("%s, invalid status:%d\n", __func__, cur_status);
		return;
	}
	g_usb_notifier.usb_old_status = cur_status;
}

static int bsp_usb_notifier_init(void)
{
	/* init local ctx resource */
	g_usb_notifier.usb_notify_wq =
			create_singlethread_workqueue("usb_notify");
	if (g_usb_notifier.usb_notify_wq == NULL) {
		pr_err("%s: create_singlethread_workqueue fail\n", __func__);
		return -ENOMEM;
	}
	INIT_DELAYED_WORK(&g_usb_notifier.usb_notify_wk, usb_notify_handler);
	return 0;
}

/* we don't need exit for adapter module */
int __init bsp_usb_vendor_init(void)
{
	return bsp_usb_notifier_init();
}
module_init(bsp_usb_vendor_init);
