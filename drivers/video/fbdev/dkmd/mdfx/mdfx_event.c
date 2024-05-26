/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "mdfx_event.h"
#include "mdfx_utils.h"
#include "mdfx_priv.h"

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif

static struct mdfx_event_listener* mdfx_event_get_listener(struct mdfx_event_manager_t *event_manager, uint32_t type)
{
	struct mdfx_event_listener* _node_ = NULL;
	struct mdfx_event_listener* listener_node = NULL;

	if (IS_ERR_OR_NULL(event_manager))
		return NULL;

	list_for_each_entry_safe(listener_node, _node_, &event_manager->listener_list, node) {
		if (listener_node->actor_type == type)
			return listener_node;
	}

	return NULL;
}

static void mdfx_event_handle(struct mdfx_pri_data *mdfx_data, struct mdfx_event_desc *desc)
{
	struct mdfx_event_listener *_node_ = NULL;
	struct mdfx_event_listener *listener_node = NULL;
	struct mdfx_event_manager_t *event_manager = NULL;
	int i = 0;

	if (IS_ERR_OR_NULL(mdfx_data) || IS_ERR_OR_NULL(desc))
		return;

	mdfx_pr_info("in mdfx_event_handle, id=%lld, visitor_types=%llu, actions=%llu, event_name=%s",
		desc->id, desc->relevance_visitor_types, desc->actions, desc->event_name);
	event_manager = &mdfx_data->event_manager;

	list_for_each_entry_safe(listener_node, _node_, &event_manager->listener_list, node) {
		if (!IS_ERR_OR_NULL(listener_node)) {
			mdfx_pr_info("i=%d, actor_type=%u, listener_node=%pK", i++, listener_node->actor_type, listener_node);
			if (listener_node->handle)
				listener_node->handle(mdfx_data, desc);
		}
	}
}

static void mdfx_event_report_to_user(struct mdfx_event_manager_t *event_manager,
	const struct mdfx_event_desc *desc)
{
	struct mdfx_event_desc *event_desc = NULL;
	uint32_t size;

	if (IS_ERR_OR_NULL(event_manager) || IS_ERR_OR_NULL(desc))
		return;

	down(&event_manager->event_sem);
	event_desc = &event_manager->event_desc;
	if (event_desc->detail != NULL) {
		mdfx_pr_info("event desc detail is not null, id=%lld, count=%u",
			event_desc->id, event_desc->detail_count);
		kfree(event_desc->detail);
		event_desc->detail = NULL;
	}
	memcpy(event_desc, desc, sizeof(*desc));
	size = sizeof(*(desc->detail)) * desc->detail_count;
	event_desc->detail = kmalloc(size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(event_desc->detail)) {
		up(&event_manager->event_sem);
		return;
	}
	memset(event_desc->detail, 0, size);
	memcpy(event_desc->detail, desc->detail, size);
	event_manager->need_report_event = 1;
	up(&event_manager->event_sem);
	mdfx_pr_err("report to user");

	wake_up_interruptible_all(&(event_manager->event_wait));
}

static ssize_t mdfx_event_build_event_buf(struct mdfx_event_manager_t *event_manager,
		char *buf, ssize_t size)
{
	struct mdfx_event_desc *desc = NULL;
	ssize_t ret;
	ssize_t len;
	uint32_t i;

	if (IS_ERR_OR_NULL(event_manager) || IS_ERR_OR_NULL(buf))
		return 0;

	if (size <= 0 || size > PAGE_SIZE) //lint !e574
		return 0;

	desc = &event_manager->event_desc;
	if (mdfx_event_check_parameter(desc) != 0)
		return 0;

	len = scnprintf(buf, size, "types=%" PRIu64 ",action=%" PRIu64 ",count=%u,",
			desc->relevance_visitor_types, desc->actions, desc->detail_count);
	if (len >= size)
		return size;

	buf[len] = '\0';

	for (i = 0; i < desc->detail_count; i++) {
		ret = scnprintf(buf + len, size - len - 1, "detail[%u]=%" PRIu64",",
					i, desc->detail[i]);
		if (ret >= size || ret + len >= size)
			return size;

		len += ret;
		if (len >= size)
			return size;
	}

	ret = scnprintf(buf + len, size - len - 1, "name=%s", desc->event_name);
	if (ret >= size)
		return size;
	len += ret;
	if (len >= size)
		return size;

	return len;
}

static ssize_t mdfx_show_event(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mdfx_device_data *device_data = NULL;
	struct mdfx_event_manager_t *event_manager = NULL;
	int32_t ret;
	ssize_t len = 0;

	if (IS_ERR_OR_NULL(dev) || IS_ERR_OR_NULL(attr) || IS_ERR_OR_NULL(buf))
		return -1;

	device_data = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(device_data))
		return -1;

	event_manager = &device_data->data.event_manager;

	mdfx_pr_err("waiting report event");
	ret = wait_event_interruptible(event_manager->event_wait, (event_manager->need_report_event == 1)); //lint !e578
	if (event_manager->need_report_event) {
		down(&event_manager->event_sem);
		len = mdfx_event_build_event_buf(event_manager, buf, PAGE_SIZE);
		if (event_manager->event_desc.detail) {
			kfree(event_manager->event_desc.detail);
			event_manager->event_desc.detail = NULL;
		}
		event_manager->need_report_event = 0;
		up(&event_manager->event_sem);
		mdfx_pr_err("report event, buf = %s", buf);
	}

	return len;
}
static DEVICE_ATTR(event_report, S_IRUGO, mdfx_show_event, NULL);

/*
 * desc's client id need be assigned by caller
 */
static void mdfx_event_build_underflow(struct mdfx_event_desc_t *desc)
{
	if (IS_ERR_OR_NULL(desc))
		return;

	desc->relevance_visitor_types = VISITOR_DISPLAY_MASK;
	desc->actions = ACTION_DUMP_INFO | ACTION_TRACE_RECODE | ACTION_SAVE_INFO | ACTION_LOG_PRINT;
	desc->detail_count = mdfx_get_bits(desc->actions);
	strcpy(desc->event_name, "underflow");

	desc->detail = kzalloc(desc->detail_count * sizeof(*desc->detail), GFP_KERNEL);
	if (!desc->detail) {
		mdfx_pr_err("detail alloc fail");
		return;
	}
	desc->detail[ACTOR_DUMPER] = DUMP_FREQ_AND_VOLTAGE | DUMP_CMDLIST;
	desc->detail[ACTOR_TRACING] = TRACING_TYPE_VOTE | TRACING_TYPE_DISPLAY_FLOW;
	desc->detail[ACTOR_SAVER] = SAVER_DETAIL_IMAGE | SAVER_DETAIL_STRING;
	desc->detail[ACTOR_LOGGER] = LOGGER_PRINT_USER_LOG | LOGGER_PRINT_DRIVER_LOG;
}

static void mdfx_event_build_vactive_timeout(struct mdfx_event_desc_t *desc)
{
	if (IS_ERR_OR_NULL(desc))
		return;

	desc->relevance_visitor_types = VISITOR_DISPLAY_MASK;
	desc->actions = ACTION_DUMP_INFO | ACTION_TRACE_RECODE | ACTION_SAVE_INFO | ACTION_LOG_PRINT;
	desc->detail_count = mdfx_get_bits(desc->actions);
	strcpy(desc->event_name, "vactive_timeout");

	desc->detail = kzalloc(desc->detail_count * sizeof(*desc->detail), GFP_KERNEL);
	if (!desc->detail) {
		mdfx_pr_err("detail alloc fail");
		return;
	}
	desc->detail[ACTOR_DUMPER] = DUMP_FREQ_AND_VOLTAGE | DUMP_CMDLIST | DUMP_CPU_RUNNABLE;
	desc->detail[ACTOR_TRACING] = TRACING_TYPE_VOTE | TRACING_TYPE_DISPLAY_FLOW;
	desc->detail[ACTOR_SAVER] = SAVER_DETAIL_STRING;
	desc->detail[ACTOR_LOGGER] = LOGGER_PRINT_USER_LOG | LOGGER_PRINT_DRIVER_LOG;
}

static void mdfx_event_build_fence_timeout(struct mdfx_event_desc_t *desc)
{
	if (IS_ERR_OR_NULL(desc))
		return;

	desc->relevance_visitor_types = VISITOR_GRAPHIC_MASK;
	desc->actions = ACTION_DUMP_INFO | ACTION_TRACE_RECODE | ACTION_SAVE_INFO | ACTION_LOG_PRINT;
	desc->detail_count = mdfx_get_bits(desc->actions);
	strcpy(desc->event_name, "fence_timeout");

	desc->detail = kzalloc(desc->detail_count * sizeof(*desc->detail), GFP_KERNEL);
	if (!desc->detail) {
		mdfx_pr_err("detail alloc fail");
		return;
	}
	desc->detail[ACTOR_DUMPER] = DUMP_FREQ_AND_VOLTAGE | DUMP_CMDLIST | DUMP_CPU_RUNNABLE;
	desc->detail[ACTOR_TRACING] = TRACING_TYPE_VOTE | TRACING_TYPE_DISPLAY_FLOW;
	desc->detail[ACTOR_SAVER] = SAVER_DETAIL_STRING;
	desc->detail[ACTOR_LOGGER] = LOGGER_PRINT_USER_LOG | LOGGER_PRINT_DRIVER_LOG;
}

static struct mdfx_event_node *mdfx_get_event_node(struct mdfx_event_manager_t *manager, const char *event_name)
{
	struct mdfx_event_node *event_node = NULL;
	struct mdfx_event_node *_event_node_ = NULL;

	if (IS_ERR_OR_NULL(manager) || IS_ERR_OR_NULL(event_name))
		return NULL;

	list_for_each_entry_safe(event_node, _event_node_, &manager->event_list, node) {
		if (strcmp(event_node->event_name, event_name) == 0)
			return event_node;
	}

	return NULL;
}

static bool mdfx_update_event_timestamp(struct mdfx_event_node *event)
{
	struct timespec current_time;
	int interval;

	if (IS_ERR_OR_NULL(event))
		return false;

	ktime_get_ts(&current_time);
	interval = current_time.tv_sec - event->time.tv_sec;
	if (interval >= MDFX_REPORT_EVENT_INTERVAL_SEC) {
		event->time = current_time;
		return true;
	}

	mdfx_pr_info("update event timestamp fail interval=%d", interval);
	return false;
}

static bool mdfx_add_event_to_list(struct mdfx_event_manager_t *manager, const char *event_name)
{
	struct mdfx_event_node *event = NULL;

	if (IS_ERR_OR_NULL(event_name) || IS_ERR_OR_NULL(manager))
		return false;

	event = mdfx_get_event_node(manager, event_name);

	/* find the event, judge the timediff and update timestamp */
	if (event)
		return mdfx_update_event_timestamp(event);

	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (!event)
		return false;

	strncpy(event->event_name, event_name, MDFX_EVENT_NAME_MAX - 1);
	ktime_get_ts(&event->time);
	list_add_tail(&event->node, &manager->event_list);

	return true;
}

int mdfx_event_check_parameter(struct mdfx_event_desc *desc)
{
	if (IS_ERR_OR_NULL(desc))
		return -1;

	if (desc->id <= INVALID_VISITOR_ID) {
		mdfx_pr_err("invalid client id=%lld", desc->id);
		return -1;
	}

	if (desc->detail_count == 0 || desc->detail_count > ACTOR_MAX) {
		mdfx_pr_err("invalid detail count=%u", desc->detail_count);
		return -1;
	}

	if (desc->actions == 0) {
		mdfx_pr_err("invalid actions=0x%"PRIx64, desc->actions);
		return -1;
	}

	if (IS_ERR_OR_NULL(desc->detail)) {
		mdfx_pr_err("invalid nullptr detail");
		return -1;
	}

	return 0;
}

int mdfx_report_dmd(struct dsm_client *client, long dmd_no,
	const char *dmd_info)
{
#ifdef CONFIG_HUAWEI_DSM
	if (IS_ERR_OR_NULL(dmd_info) || IS_ERR_OR_NULL(client)) {
		mdfx_pr_info("para error: %lx\n", dmd_no);
		return -1;
	}

	/* it should be record in kmsg when dmd happened */
	mdfx_pr_info("dmd no: %lx - %s", dmd_no, dmd_info);
	if (!dsm_client_ocuppy(client)) {
		dsm_client_record(client, "DMD info:%s", dmd_info);
		dsm_client_notify(client, dmd_no);
	}
#endif
	return 0;
}

void mdfx_event_init(struct mdfx_pri_data *mdfx_data)
{
	struct mdfx_event_manager_t *event_manager = NULL;

	if (IS_ERR_OR_NULL(mdfx_data))
		return;

	event_manager = &mdfx_data->event_manager;

	INIT_LIST_HEAD(&event_manager->listener_list);
	INIT_LIST_HEAD(&event_manager->event_list);
	sema_init(&event_manager->event_sem, 1);
	init_waitqueue_head(&event_manager->event_wait);
	event_manager->need_report_event = 0;
	memset(&event_manager->event_desc, 0, sizeof(event_manager->event_desc));

	mdfx_sysfs_append_attrs(mdfx_data, &dev_attr_event_report.attr);

	mdfx_event_build_underflow(&event_manager->default_event[DEF_EVENT_UNDER_FLOW]);
	mdfx_event_build_vactive_timeout(&event_manager->default_event[DEF_EVENT_VACTIVE_TIMEOUT]);
	mdfx_event_build_fence_timeout(&event_manager->default_event[DEF_EVENT_FENCE_TIMEOUT]);
}

void mdfx_event_deinit(struct mdfx_event_manager_t *event_manager)
{
	struct mdfx_event_listener* _node_ = NULL;
	struct mdfx_event_listener* listener_node = NULL;
	struct mdfx_event_desc_t *default_event = NULL;
	struct mdfx_event_node *event_node = NULL;
	struct mdfx_event_node *_event_node_ = NULL;
	uint32_t i;

	if (IS_ERR_OR_NULL(event_manager))
		return;

	list_for_each_entry_safe(listener_node, _node_, &event_manager->listener_list, node) {
		list_del(&listener_node->node);
		kfree(listener_node);
		listener_node = NULL;
	}

	list_for_each_entry_safe(event_node, _event_node_, &event_manager->event_list, node) {
		list_del(&event_node->node);
		kfree(event_node);
		event_node = NULL;
	}

	for (i = 0; i < DEF_EVENT_MAX; i++) {
		default_event = &event_manager->default_event[i];
		if (default_event->detail) {
			kfree(default_event->detail);
			default_event->detail = NULL;
		}

		default_event->relevance_visitor_types = 0;
		default_event->detail_count = 0;
		default_event->actions = 0;
	}
}

int mdfx_event_register_listener(uint32_t actor_type, listener_act_func cb)
{
	struct mdfx_event_listener *listener = NULL;
	struct mdfx_event_manager_t *event_manager = NULL;

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return -1;

	mdfx_pr_info("actor_type = %u, cb=%pK", actor_type, cb);

	event_manager = &g_mdfx_data->event_manager;

	listener = mdfx_event_get_listener(event_manager, actor_type);
	if (listener && IS_ERR_OR_NULL(listener->handle)) {
		listener->handle = cb;
		return 0;
	}

	/* can't find the type listener, create a new one */
	if (IS_ERR_OR_NULL(listener)) {
		listener = kzalloc(sizeof(*listener), GFP_KERNEL);
		if (IS_ERR_OR_NULL(listener))
			return -1;

		listener->actor_type = actor_type;
		listener->handle = cb;

		list_add_tail(&listener->node, &event_manager->listener_list);

		mdfx_pr_info("actor_type=%u, listener=%pK", actor_type, listener);
	}

	return 0;
}

uint64_t mdfx_event_get_action_detail(const struct mdfx_event_desc *desc, uint32_t actor_type)
{
	if (IS_ERR_OR_NULL(desc))
		return 0;

	mdfx_pr_info("actor_type=%u, actions=%llu, detail_count=%u", actor_type, desc->actions, desc->detail_count);
	if (actor_type >= ACTOR_MAX)
		return 0;

	if (!(desc->actions & BIT64(actor_type)))
		return 0;

	if (desc->detail_count <= actor_type)
		return 0;

	return desc->detail[actor_type];
}

struct mdfx_event_desc *mdfx_copy_event_desc(struct mdfx_event_desc *desc)
{
	struct mdfx_event_desc *desc_buf = NULL;
	uint64_t *detail = NULL;
	uint32_t size;

	if (IS_ERR_OR_NULL(desc))
		return NULL;

	mdfx_pr_info("enter mdfx_copy_event_desc");
	desc_buf = kmalloc(sizeof(*desc_buf), GFP_KERNEL);
	if (IS_ERR_OR_NULL(desc_buf)) {
		mdfx_pr_err("kmalloc fail");
		return NULL;
	}
	memset(desc_buf, 0, sizeof(*desc_buf));
	memcpy(desc_buf, desc, sizeof(*desc_buf));

	size = sizeof(*detail) * desc->detail_count;
	detail = kmalloc(size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(detail)) {
		mdfx_pr_err("kmalloc fail");
		kfree(desc_buf);
		return NULL;
	}
	memcpy(detail, desc->detail, size);
	desc_buf->detail = detail;

	return desc_buf;
}

int mdfx_deliver_event(struct mdfx_pri_data *mdfx_data, const void __user *argp)
{
	struct mdfx_event_desc desc;
	uint64_t *detail = NULL;
	uint32_t size;

	if (IS_ERR_OR_NULL(mdfx_data))
		return -1;

	if (IS_ERR_OR_NULL(argp))
		return -1;

	if (!MDFX_HAS_CAPABILITY(mdfx_data->mdfx_caps, MDFX_CAP_EVENT))
		return 0;

	if (copy_from_user(&desc, argp, sizeof(desc))) {
		mdfx_pr_err("copy event desc fail from user");
		return -1;
	}

	if (mdfx_event_check_parameter(&desc)) {
		mdfx_pr_err("check parameter fail");
		return -1;
	}

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return -1;

	/* event can't be reported too often, the same events must have at least 5 second interval */
	if (!mdfx_add_event_to_list(&g_mdfx_data->event_manager, desc.event_name)) {
		mdfx_pr_info("add event to list fail");
		return -1;
	}

	size = sizeof(*detail) * desc.detail_count;
	detail = kmalloc(size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(detail))
		return -1;
	memset(detail, 0, size);

	if (copy_from_user(detail, desc.detail, size)) {
		mdfx_pr_err("copy detail fail, id=%lld, count=%u", desc.id, desc.detail_count);
		kfree(detail);
		detail = NULL;
		return -1;
	}

	desc.detail = detail;
	mdfx_event_handle(mdfx_data, &desc);

	kfree(detail);
	detail = NULL;
	return 0;
}

void mdfx_report_event(int64_t visitor_id, struct mdfx_event_desc_t *event_desc)
{
	struct mdfx_event_desc desc;

	if (IS_ERR_OR_NULL(event_desc))
		return;

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return;

	if (!MDFX_HAS_CAPABILITY(g_mdfx_data->mdfx_caps, MDFX_CAP_EVENT))
		return;

	desc.id = visitor_id;
	desc.relevance_visitor_types = event_desc->relevance_visitor_types;
	desc.actions = event_desc->actions;
	desc.detail = event_desc->detail;
	desc.detail_count = event_desc->detail_count;
	memcpy(desc.event_name, event_desc->event_name, MDFX_EVENT_NAME_MAX);

	if (mdfx_event_check_parameter(&desc)) {
		mdfx_pr_err("invalid event desc");
		return;
	}

	/* event can't be reported too often, the same events must have at least 5 second interval */
	if (!mdfx_add_event_to_list(&g_mdfx_data->event_manager, desc.event_name)) {
		mdfx_pr_info("add event to list fail");
		return;
	}

	mdfx_pr_info("report event=%s", desc.event_name);

	// listeners will handle this event
	mdfx_event_handle(g_mdfx_data, &desc);

	/* some events need mdfx server to handle it,
	 * so need report to user.
	 */
	mdfx_event_report_to_user(&g_mdfx_data->event_manager, &desc);
}
EXPORT_SYMBOL(mdfx_report_event); //lint !e580

void mdfx_report_default_event(int64_t visitor_id, uint32_t default_event_type)
{
	struct mdfx_event_desc_t *default_event = NULL;

	if (default_event_type >= DEF_EVENT_MAX)
		return;

	if (visitor_id <= INVALID_VISITOR_ID)
		return;

	if (IS_ERR_OR_NULL(g_mdfx_data))
		return;

	mdfx_pr_err("report default event visitor=%lld, event_type=%u", visitor_id, default_event_type);

	default_event = &g_mdfx_data->event_manager.default_event[default_event_type];
	if (IS_ERR_OR_NULL(default_event->detail))
		return;

	mdfx_report_event(visitor_id, default_event);
}
EXPORT_SYMBOL(mdfx_report_default_event); //lint !e580



