/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include "dpu_config_utils.h"
#include "dpu_comp_smmu.h"
#include "dpu_comp_mgr.h"
#include "gfxdev_mgr.h"
#include "dpu_comp_vsync.h"
#include "dvfs.h"

static inline int32_t vsync_timestamp_changed(struct dpu_vsync *vsync_ctrl, ktime_t prev_timestamp)
{
	return !(prev_timestamp == vsync_ctrl->timestamp);
}

static struct comp_online_present *get_present_data(struct device *dev)
{
	struct dpu_composer *dpu_comp = NULL;

	dpu_comp = to_dpu_composer(get_comp_from_device(dev));
	dpu_check_and_return(!dpu_comp, NULL, err, "dpu_comp is null pointer");

	return (struct comp_online_present *)dpu_comp->present_data;
}

static ssize_t vsync_event_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	ktime_t prev_timestamp;
	struct dpu_vsync *vsync_ctrl = NULL;
	struct comp_online_present *present = NULL;

	dpu_check_and_return((!dev || !buf), -1, err, "input is null pointer");
	present = get_present_data(dev);
	dpu_check_and_return(!present, -1, err, "present_data is null pointer");

	vsync_ctrl = &present->vsync_ctrl;
	prev_timestamp = vsync_ctrl->timestamp;
	ret = wait_event_interruptible(vsync_ctrl->wait,
		(vsync_timestamp_changed(vsync_ctrl, prev_timestamp) && vsync_ctrl->enabled));
	if (ret) {
		dpu_pr_err("vsync wait event be interrupted abnormal!!!");
		return -1;
	}
	ret = snprintf(buf, PAGE_SIZE, "VSYNC=%llu\n", ktime_to_ns(vsync_ctrl->timestamp));
	buf[strlen(buf) + 1] = '\0';

	return ret;
}

static ssize_t vsync_timestamp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct dpu_vsync *vsync_ctrl = NULL;
	struct comp_online_present *present = NULL;

	dpu_check_and_return((!dev || !buf), -1, err, "input is null pointer");
	present = get_present_data(dev);
	dpu_check_and_return(!present, -1, err, "present_data is null pointer");

	vsync_ctrl = &present->vsync_ctrl;
	ret = snprintf(buf, PAGE_SIZE, "%llu\n", ktime_to_ns(vsync_ctrl->timestamp));
	buf[strlen(buf) + 1] = '\0';

	dpu_pr_info("buf:%s ", buf);

	return ret;
}

static ssize_t vsync_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	int32_t enable;
	struct dpu_vsync *vsync_ctrl = NULL;
	struct comp_online_present *present = NULL;

	dpu_check_and_return((!dev || !buf), -1, err, "input is null pointer");
	present = get_present_data(dev);
	dpu_check_and_return(!present, -1, err, "present_data is null pointer");

	vsync_ctrl = &present->vsync_ctrl;
	ret = sscanf(buf, "%d", &enable);
	if (!ret) {
		dpu_pr_err("get buf (%s) enable fail\n", buf);
		return -1;
	}
	dpu_pr_debug("dev->kobj.name: %s vsync enable=%d", dev->kobj.name, enable);
	dpu_vsync_enable(vsync_ctrl, enable);

	return count;
}

static DEVICE_ATTR(vsync_event, 0444, vsync_event_show, NULL);
static DEVICE_ATTR(vsync_enable, 0200, NULL, vsync_enable_store);
static DEVICE_ATTR(vsync_timestamp, 0444, vsync_timestamp_show, NULL);

static void vsync_idle_handle_work(struct kthread_work *work)
{
	struct dpu_vsync *vsync_ctrl = NULL;
	struct composer_manager *comp_mgr = NULL;
	struct dkmd_connector_info *pinfo = NULL;

	vsync_ctrl = container_of(work, struct dpu_vsync, idle_handle_work);
	dpu_check_and_no_retval(!vsync_ctrl, err, "vsync_ctrl is null pointer");
	dpu_check_and_no_retval(!vsync_ctrl->dpu_comp, err, "dpu_comp is null pointer");
	comp_mgr = vsync_ctrl->dpu_comp->comp_mgr;
	dpu_check_and_no_retval(!comp_mgr, err, "comp_mgr is null pointer");
	pinfo = vsync_ctrl->dpu_comp->conn_info;
	dpu_check_and_no_retval(!pinfo, err, "pinfo is null pointer");

	mutex_lock(&comp_mgr->idle_lock);
	/* enter vsync idle */
	if (comp_mgr->idle_enable && (comp_mgr->idle_expire_count == 0) && (g_debug_idle_enable != 0)) {
		dpu_pr_debug("pinfo->vsync_ctrl_type=%d", pinfo->vsync_ctrl_type);
		if (((pinfo->vsync_ctrl_type & VSYNC_IDLE_MIPI_ULPS) != 0) &&
			((comp_mgr->idle_func_flag & VSYNC_IDLE_MIPI_ULPS) == 0)) {
			pipeline_next_ops_handle(pinfo->conn_device, pinfo, HANDLE_MIPI_ULPS, (void *)&comp_mgr->idle_enable);
			comp_mgr->idle_func_flag |= VSYNC_IDLE_MIPI_ULPS;
		}

		if (((pinfo->vsync_ctrl_type & VSYNC_IDLE_CLK_OFF) != 0) &&
			((comp_mgr->idle_func_flag & VSYNC_IDLE_CLK_OFF) == 0)) {
			dpu_comp_smmuv3_off(comp_mgr, vsync_ctrl->dpu_comp);
			pipeline_next_ops_handle(pinfo->conn_device, pinfo, DISABLE_ISR, &vsync_ctrl->dpu_comp->isr_ctrl);
			dpu_disable_core_clock();
			dpu_dvfs_reset_dvfs_info();

			comp_mgr->idle_func_flag |= VSYNC_IDLE_CLK_OFF;
		}
	} else {
		if (comp_mgr->idle_expire_count > 0)
			comp_mgr->idle_expire_count--;
	}
	dpu_pr_debug("comp_mgr->idle_func_flag=%d", comp_mgr->idle_func_flag);
	mutex_unlock(&comp_mgr->idle_lock);
}

/**
 * @brief exit vsync idle right now
 *
 */
void dpu_comp_active_vsync(struct dpu_composer *dpu_comp)
{
	struct composer_manager *comp_mgr = NULL;
	struct comp_online_present *present = NULL;

	dpu_check_and_no_retval(!dpu_comp, err, "dpu_comp is null pointer");
	comp_mgr = dpu_comp->comp_mgr;
	dpu_check_and_no_retval(!comp_mgr, err, "comp_mgr is null pointer");
	present = (struct comp_online_present *)dpu_comp->present_data;
	dpu_check_and_no_retval(!present, err, "present is null pointer");

	mutex_lock(&comp_mgr->idle_lock);
	/* Immediately exit the idle state */
	comp_mgr->idle_expire_count = 0;
	comp_mgr->idle_enable = false;
	comp_mgr->active_status.refcount.value[dpu_comp->comp.index]++;

	dpu_pr_debug("comp_mgr->active_status.status=%d", comp_mgr->active_status.status);
	if (((dpu_comp->conn_info->vsync_ctrl_type & VSYNC_IDLE_CLK_OFF) == 0) &&
		((comp_mgr->idle_func_flag & VSYNC_IDLE_CLK_OFF) != 0)) {
		dpu_enable_core_clock();
		mutex_unlock(&comp_mgr->idle_lock);
		return;
	}

	if ((comp_mgr->idle_func_flag & VSYNC_IDLE_CLK_OFF) != 0) {
		dpu_enable_core_clock();

		pipeline_next_ops_handle(dpu_comp->conn_info->conn_device,
			dpu_comp->conn_info, ENABLE_ISR, &dpu_comp->isr_ctrl);

		dpu_comp_smmuv3_on(comp_mgr, dpu_comp);

		comp_mgr->idle_func_flag &= ~VSYNC_IDLE_CLK_OFF;
	}

	/* need exit vsync idle */
	if ((comp_mgr->idle_func_flag & VSYNC_IDLE_MIPI_ULPS) != 0) {
		pipeline_next_ops_handle(dpu_comp->conn_info->conn_device,
			dpu_comp->conn_info, HANDLE_MIPI_ULPS, (void *)&comp_mgr->idle_enable);
		comp_mgr->idle_func_flag &= ~VSYNC_IDLE_MIPI_ULPS;
	}

	mutex_unlock(&comp_mgr->idle_lock);
}
EXPORT_SYMBOL(dpu_comp_active_vsync);

/**
 * @brief Restore vsync Idle Count
 *
 */
void dpu_comp_deactive_vsync(struct dpu_composer *dpu_comp)
{
	struct composer_manager *comp_mgr = NULL;

	dpu_check_and_no_retval(!dpu_comp, err, "dpu_comp is null pointer");
	comp_mgr = dpu_comp->comp_mgr;
	dpu_check_and_no_retval(!comp_mgr, err, "comp_mgr is null pointer");

	mutex_lock(&comp_mgr->idle_lock);
	dpu_pr_debug("comp_mgr->active_status.status=%d", comp_mgr->active_status.status);
	comp_mgr->active_status.refcount.value[dpu_comp->comp.index]--;
	if (comp_mgr->active_status.status == 0) {
		comp_mgr->idle_enable = true;
		comp_mgr->idle_expire_count = VSYNC_IDLE_EXPIRE_COUNT;
	}
	mutex_unlock(&comp_mgr->idle_lock);
}
EXPORT_SYMBOL(dpu_comp_deactive_vsync);

static int32_t vsync_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	ktime_t pre_vsync_timestamp;
	struct dpu_vsync *vsync_ctrl = (struct dpu_vsync *)data;

	if ((action & vsync_ctrl->listening_isr_bit) == 0)
		return 0;

	pre_vsync_timestamp = vsync_ctrl->timestamp;
	vsync_ctrl->timestamp = ktime_get();

	if (dpu_vsync_is_enabled(vsync_ctrl))
		wake_up_interruptible_all(&(vsync_ctrl->wait));

	if (g_debug_vsync_dump)
		dpu_pr_info("VSYNC = %llu, time_diff = %llu us", ktime_to_ns(vsync_ctrl->timestamp),
			ktime_us_delta(vsync_ctrl->timestamp, pre_vsync_timestamp));

	if (vsync_ctrl->dpu_comp->conn_info->vsync_ctrl_type != 0)
		kthread_queue_work(&vsync_ctrl->dpu_comp->handle_worker, &vsync_ctrl->idle_handle_work);

	return 0;
}

static struct notifier_block vsync_isr_notifier = {
	.notifier_call = vsync_isr_notify,
};

void dpu_vsync_init(struct dpu_vsync *vsync_ctrl, struct dkmd_attr *attrs)
{
	spin_lock_init(&(vsync_ctrl->spin_enable));
	init_waitqueue_head(&vsync_ctrl->wait);

	vsync_ctrl->enabled = 0;
	vsync_ctrl->notifier = &vsync_isr_notifier;
	kthread_init_work(&vsync_ctrl->idle_handle_work, vsync_idle_handle_work);

	dkmd_sysfs_attrs_append(attrs, &dev_attr_vsync_event.attr);
	dkmd_sysfs_attrs_append(attrs, &dev_attr_vsync_enable.attr);
	dkmd_sysfs_attrs_append(attrs, &dev_attr_vsync_timestamp.attr);
}
