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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/err.h>
#include <linux/errno.h>

#include "mdfx_debug.h"
#include "mdfx_priv.h"


module_param_named(mdfx_kmsg_level, mdfx_debug_level, int, 0644);
MODULE_PARM_DESC(mdfx_kmsg_level, "mdfx kmsg log level");

static ssize_t mdfx_debugger_event_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	int64_t visitor_id;
	uint32_t default_event;
	int ret;

	if (IS_ERR_OR_NULL(device) || IS_ERR_OR_NULL(attr) || IS_ERR_OR_NULL(buf))
		return -1;

	ret = sscanf(buf, "%llu %u", &visitor_id, &default_event);
	if (ret < 0)
		mdfx_pr_err("sscanf error");
	mdfx_pr_info("visitor_id=%llu, event=%u", visitor_id, default_event);

	mdfx_report_default_event(visitor_id, default_event);
	return count;
}
static DEVICE_ATTR(debug_event,  (S_IRUGO|S_IWUSR), NULL, mdfx_debugger_event_store);

static ssize_t mdfx_show_debugger(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mdfx_device_data *device_data = NULL;
	struct mdfx_debugger_t *debugger = NULL;
	int32_t ret;
	ssize_t len = 0;

	mdfx_pr_err("enter!");

	if (IS_ERR_OR_NULL(dev)) {
		mdfx_pr_err("dev is null");
		return -1;
	}

	if (IS_ERR_OR_NULL(attr)) {
		mdfx_pr_err("attr is null");
		return -1;
	}

	if (IS_ERR_OR_NULL(buf)) {
		mdfx_pr_err("buf is null");
		return -1;
	}

	device_data = dev_get_drvdata(dev);
	if (IS_ERR_OR_NULL(device_data)) {
		mdfx_pr_err("device data is null");
		return -1;
	}

	debugger = &device_data->data.debugger;

	mdfx_pr_err("start wait update=%u!", debugger->need_update_debugger);
	ret = wait_event_interruptible(debugger->debugger_wait, (debugger->need_update_debugger == 1)); //lint !e578
	if (debugger->need_update_debugger) {
		down(&debugger->debugger_sem);
		len = scnprintf(buf, PAGE_SIZE - 1, "update_debugger=1");
		debugger->need_update_debugger = 0;
		up(&debugger->debugger_sem);
	}

	mdfx_pr_err("debugger update! len=%ld, ret=%d, update=%u", len, ret, debugger->need_update_debugger);
	return len;
}
static DEVICE_ATTR(update_debugger, S_IRUGO, mdfx_show_debugger, NULL);

static ssize_t mdfx_debugger_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdfx_device_data *device_data = NULL;
	struct mdfx_debugger_t *debugger = NULL;
	uint32_t file_count = 0;
	uint32_t update = 0;
	uint32_t mdfx_caps = 0;
	int ret;

	if (IS_ERR_OR_NULL(device)) {
		mdfx_pr_err("device is null");
		return -1;
	}

	if (IS_ERR_OR_NULL(attr)) {
		mdfx_pr_err("attr is null");
		return -1;
	}

	if (IS_ERR_OR_NULL(buf)) {
		mdfx_pr_err("buf is null");
		return -1;
	}

	device_data = dev_get_drvdata(device);
	if (IS_ERR_OR_NULL(device_data)) {
		mdfx_pr_err("device data is null");
		return -1;
	}
	debugger = &device_data->data.debugger;

	mdfx_pr_info("debugger store %s", buf);

	if (strstr(buf, MDFX_UPDATE_DEBUGGER_STR) != NULL) {
		ret = sscanf(buf + strlen(MDFX_UPDATE_DEBUGGER_STR), "%u", &update);
		if (ret < 0)
			mdfx_pr_err("sscanf error");
		down(&debugger->debugger_sem);
		debugger->need_update_debugger = (update == 1) ? 1 : 0;
		up(&debugger->debugger_sem);

		wake_up_interruptible_all(&(debugger->debugger_wait));
		mdfx_pr_info("in mdfx_debugger_store, call wake_up_interruptible_all");

		return count;
	}

	if (strstr(buf, MDFX_SET_LOG_FILE_COUNT_STR) != NULL) {
		ret = sscanf(buf + strlen(MDFX_SET_LOG_FILE_COUNT_STR), "%u", &file_count);
		if (ret < 0)
			mdfx_pr_err("sscanf error");
		if ((file_count <= MDFX_LOG_FILE_MAX_COUNT) && (file_count > 1)) {
			mdfx_pr_info("device_data->data.var_log_file_count = %u before", device_data->data.var_log_file_count);
			down(&debugger->debugger_sem);
			device_data->data.var_log_file_count = file_count;
			up(&debugger->debugger_sem);
			mdfx_pr_info("device_data->data.var_log_file_count = %u after", device_data->data.var_log_file_count);
		}
	}

	if (strstr(buf, MDFX_DEBUG_SET_CAPS_STR) != NULL) {
		ret = sscanf(buf + strlen(MDFX_DEBUG_SET_CAPS_STR), "%u", &mdfx_caps);
		if (ret < 0)
			mdfx_pr_err("sscanf error");
		mdfx_pr_info("device_data->data.mdfx_caps = %u before", device_data->data.mdfx_caps);
		down(&debugger->debugger_sem);
		device_data->data.mdfx_caps = mdfx_caps;
		up(&debugger->debugger_sem);
		mdfx_pr_info("device_data->data.mdfx_caps = %u after", device_data->data.mdfx_caps);
	}

	return count;
}
static DEVICE_ATTR(debug_debugger, S_IWUSR, NULL, mdfx_debugger_store);

void mdfx_debugger_init(struct mdfx_pri_data *mdfx_data)
{
	struct mdfx_debugger_t *debugger = NULL;

	if (IS_ERR_OR_NULL(mdfx_data))
		return;

	debugger = &mdfx_data->debugger;

	sema_init(&debugger->debugger_sem, 1);
	init_waitqueue_head(&debugger->debugger_wait);
	debugger->need_update_debugger = 0;

	mdfx_sysfs_append_attrs(mdfx_data, &dev_attr_update_debugger.attr);
	mdfx_sysfs_append_attrs(mdfx_data, &dev_attr_debug_event.attr);
	mdfx_sysfs_append_attrs(mdfx_data, &dev_attr_debug_debugger.attr);
}

