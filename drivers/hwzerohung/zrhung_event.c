/*
 * zrhung_event.c
 *
 * Interfaces implementation for sending hung event from kernel
 *
 * Copyright (c) 2017-2019 Huawei Technologies Co., Ltd.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <log/hiview_hievent.h>
#include <asm/current.h>
#include "huawei_platform/log/hw_log.h"
#include "chipset_common/hwzrhung/zrhung.h"
#include "zrhung_common.h"
#include "zrhung_transtation.h"
#define HWLOG_TAG zrhung
#define MAX_PATH_LEN     256
HWLOG_REGIST();

struct event_info {
	char *domain;
	char *stringid;
};

static int zrhung_send_hievent(const char *domain, const char *stringid,
	const char *msg_buf)
{
	int ret;

	hwlog_info("zrhung report: domain: %s, stringid: %s, pid: %d, name: %s",
		domain, stringid, current->pid, current->comm);
	ret = hiview_send_hisysevent(domain, stringid, FAULT,
		"%s,%d,%s,%d,%s,%s,%s,%s,%s,%s", "PID", current->pid,
		"UID", current->tgid, "PACKAGE_NAME", current->comm,
		"PROCESS_NAME", current->comm, "MSG", msg_buf);
	if (ret < 0)
		hwlog_err("failed to send hisysevent, domain: %s, stringid: %s",
			domain, stringid);
	return ret;
}

static struct event_info get_event_info(enum zrhung_wp_id wpid)
{
	struct event_info info;

	info.domain = "KERNEL_VENDOR";
	switch (wpid) {
	case ZRHUNG_EVENT_POWERKEY:
		info.stringid = "POWER_KEY";
		break;
	case ZRHUNG_WP_FENCE:
		info.stringid = "FENCE";
		break;
	case ZRHUNG_EVENT_LONGPRESS:
		info.stringid = "LONG_PRESS";
		break;
	case ZRHUNG_WP_INIT:
		info.stringid = "INIT_FREEZE";
		break;
	case ZRHUNG_WP_HTSK_WARNING:
		info.stringid = "HUNGTASK_WARING";
		break;
#ifdef CONFIG_HW_ICS_ZEROHUNG
	case ZRHUNG_WP_HUNGTASK:
		info.stringid = "CDC_HUNGTASK";
		break;
	case ZRHUNG_WP_SCREENOFF:
		info.stringid = "CDC_SCREEN_OFF";
		break;
	case ZRHUNG_WP_SCREENON:
		info.stringid = "CDC_SCREEN_ON";
		break;
#else
	case ZRHUNG_WP_HUNGTASK:
		info.stringid = "HUNGTASK";
		break;
	case ZRHUNG_WP_SCREENOFF:
		info.stringid = "SCREEN_OFF";
		break;
	case ZRHUNG_WP_SCREENON:
		info.stringid = "SCREEN_ON";
		break;
#endif
	case ZRHUNG_WP_SOFT_LOCKUP:
		info.stringid = "SOFT_LOCKUP";
		break;
	default:
		info.domain = "UNKOWN POINT";
		info.stringid = "UNKOWN POINT";
		break;
	}
	return info;
}

int zrhung_send_event(enum zrhung_wp_id id, const char *cmd_buf,
	const char *msg_buf)
{
	int ret;
	int msg_len = 0;
	struct event_info info = get_event_info(id);

	if (zrhung_is_id_valid(id) < 0) {
		hwlog_err("Bad watchpoint id");
		return -EINVAL;
	}
	if (msg_buf)
		msg_len = strlen(msg_buf);
	if (msg_len > ZRHUNG_MSG_LEN_MAX) {
		hwlog_err("watchpoint msg buffer too long");
		return -EINVAL;
	}
	if (cmd_buf)
		hwlog_info("zrhung old cmd: %s", cmd_buf);
	ret = zrhung_send_hievent(info.domain, info.stringid,  msg_buf);
	hwlog_info("zrhung send event from kernel: wp=%d, ret=%d", id, ret);
	return ret;
}
EXPORT_SYMBOL(zrhung_send_event);
