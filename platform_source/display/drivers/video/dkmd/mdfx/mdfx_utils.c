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

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sched/clock.h>

#include "mdfx_priv.h"
#include "mdfx_utils.h"

// level manager
int32_t mdfx_debug_level = DFX_KERN_EMERG;

struct dfx_time mdfx_get_rtc_time(void)
{
	struct dfx_time dt;

	do_gettimeofday(&dt.tv);
	dt.tv.tv_sec -= sys_tz.tz_minuteswest * 60; /*lint !e647*/
	time_to_tm(dt.tv.tv_sec, 0, &dt.tm_rtc);

	return dt;
}

static void mdfx_get_file_specify(struct device_node *np, struct mdfx_file_spec *file_spec)
{
	uint32_t file_size = 0;
	uint32_t file_nums = 0;
	int ret;

	if (IS_ERR_OR_NULL(np) || IS_ERR_OR_NULL(file_spec))
		return;

	if (mdfx_user_mode()) {
		ret = of_property_read_u32(np, "user_file_max_size", &file_size);
		log_and_return_if(ret, "failed to get user_file_max_size resource\n");

		ret = of_property_read_u32(np, "user_file_max_num", &file_nums);
		log_and_return_if(ret, "failed to get user_file_max_num resource\n");

		mdfx_pr_info("user file_max_size=%u\n", file_size);
		mdfx_pr_info("user file_max_num=%u\n", file_nums);
	} else {
		ret = of_property_read_u32(np, "file_max_size", &file_size);
		log_and_return_if(ret, "failed to get file_max_size resource\n");

		ret = of_property_read_u32(np, "file_max_num", &file_nums);
		log_and_return_if(ret, "failed to get file_max_num resource\n");

		mdfx_pr_info("eng file_max_size=%u\n", file_size);
		mdfx_pr_info("eng file_max_num=%u\n", file_nums);
	}

	file_spec->file_max_size = file_size;
	file_spec->file_max_num = file_nums;
}

int mdfx_read_dtsi(struct mdfx_device_data *dfx_data)
{
	struct device_node *np = NULL;
	struct mdfx_pri_data *priv_data = NULL;
	struct device *dev = NULL;
	int ret;

	if (IS_ERR_OR_NULL(dfx_data)) {
		mdfx_pr_err("mdfx_data NULL Pointer\n");
		return -1;
	}

	dev = dfx_data->dev;
	priv_data = &dfx_data->data;

	// get file spec from dtsi
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_DFX_NAME);
	if (IS_ERR_OR_NULL(np)) {
		mdfx_pr_err("NOT FOUND device node %s!\n", DTS_COMP_DFX_NAME);
		return -ENXIO;
	}

	mdfx_get_file_specify(np, &priv_data->file_spec);

	// read mdfx capability from dtsi
	ret = of_property_read_u32(np, "mdfx_caps", &priv_data->mdfx_caps);
	if (ret != 0)
		mdfx_pr_err("failed to get mdfx_caps resource\n");

	mdfx_pr_err("mdfx_caps=0x%x", priv_data->mdfx_caps);

	return 0;
}

void mdfx_sysfs_init(struct mdfx_pri_data *mdfx_data)
{
	uint32_t i;

	if (IS_ERR_OR_NULL(mdfx_data))
		return;

	mdfx_data->attrs_count = 0;
	for (i = 0; i < MDFX_MAX_ATTRS_NUM; i++)
		mdfx_data->sysfs_attrs[i] = NULL;

	mdfx_data->attr_group.attrs = mdfx_data->sysfs_attrs;
}

void mdfx_sysfs_append_attrs(struct mdfx_pri_data *mdfx_data, struct attribute *attr)
{
	if (IS_ERR_OR_NULL(mdfx_data) || IS_ERR_OR_NULL(attr))
		return;

	if (mdfx_data->attrs_count >= MDFX_MAX_ATTRS_NUM) {
		mdfx_pr_err("attrs_count is over %d", MDFX_MAX_ATTRS_NUM);
		return;
	}

	mdfx_data->sysfs_attrs[mdfx_data->attrs_count] = attr;
	mdfx_data->attrs_count++;
}

void mdfx_sysfs_create_attrs(struct platform_device *pdev)
{
	struct mdfx_device_data *device_data = NULL;
	int ret;

	if (IS_ERR_OR_NULL(pdev))
		return;

	device_data = platform_get_drvdata(pdev);
	if (IS_ERR_OR_NULL(device_data))
		return;

	ret = sysfs_create_group(&device_data->dfx_cdevice->kobj, &(device_data->data.attr_group));
	if (ret) {
		mdfx_pr_err("create sysfs attrs fail");
		return;
	}

	mdfx_pr_info("create attrs succes");
}

void mdfx_sysfs_remove_attrs(struct platform_device *pdev)
{
	struct mdfx_device_data *device_data = NULL;

	if (IS_ERR_OR_NULL(pdev))
		return;

	device_data = platform_get_drvdata(pdev);
	if (IS_ERR_OR_NULL(device_data))
		return;

	sysfs_remove_group(&device_data->dfx_cdevice->kobj, &(device_data->data.attr_group));
	mdfx_sysfs_init(&device_data->data);
}

uint32_t mdfx_get_bits(uint64_t value)
{
	uint32_t count = 0;

	for (count = 0; value > 0; count++)
		value &= value - 1;

	return count;
}

uint32_t mdfx_get_msg_header(char *buf, uint32_t buf_max_len)
{
	struct dfx_time dt;
	unsigned long long ts;
	unsigned long rem_nsec;
	uint32_t out_len;

	if (IS_ERR_OR_NULL(buf))
		return 0;

	ts = local_clock();
	rem_nsec = do_div(ts, 1000000000);

	dt = mdfx_get_rtc_time();
	out_len = scnprintf(buf, buf_max_len, "%.2d-%.2d %.2d:%.2d:%.2d.%03lu [%5lu.%06lus][pid:%d,cpu%d,%s]",
					dt.tm_rtc.tm_mon + 1, dt.tm_rtc.tm_mday, dt.tm_rtc.tm_hour,
					dt.tm_rtc.tm_min, dt.tm_rtc.tm_sec, dt.tv.tv_usec / 1000,
					(unsigned long)ts, rem_nsec / 1000, current->pid, smp_processor_id(),
					in_irq() ? "in irq" : current->comm);

	return out_len;
}


#ifdef MDFX_USER
bool user_mode = true;
#else
bool user_mode = false;
#endif

inline bool mdfx_user_mode(void)
{
	return user_mode;
}

