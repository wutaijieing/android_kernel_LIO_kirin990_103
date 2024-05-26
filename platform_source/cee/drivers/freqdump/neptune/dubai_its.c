/*
 * dubai_its.c
 *
 * dubai its module
 *
 * Copyright (C) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <soc_its_para.h>
#include "dubai_its.h"

static its_cpu_power_t *g_its_cpu_power_data;
static size_t g_its_cpu_power_data_size;

int ioctrl_get_cpu_power(void __user *argp)
{
	int ret;

	if (argp == NULL) {
		pr_err("%s argp null\n", __func__);
		return -EFAULT;
	}

	if (g_its_cpu_power_data == NULL || g_its_cpu_power_data_size == 0)
		return -EFAULT;

	ret = get_its_power_result(g_its_cpu_power_data);
	if (ret != 0) {
		pr_err("dpm get ITS cpu power fail, ret %d\n", ret);
		return -EFAULT;
	}
	if (copy_to_user(argp, g_its_cpu_power_data,
			 g_its_cpu_power_data_size) != 0) {
		pr_err("dpm get g_its_cpu_power_data failed!\n");
		return -EFAULT;
	}
	return 0;
}

int ioctrl_reset_cpu_power(void __user *argp)
{
	int ret;

	(void)argp;
	ret = reset_power_result();
	if (ret != 0)
		pr_err("dpm reset ITS power data fail!\n");

	return ret;
}

void free_its_cpu_power_mem(void)
{
	if (g_its_cpu_power_data != NULL) {
		kfree(g_its_cpu_power_data);
		g_its_cpu_power_data = NULL;
	}
}

bool create_its_cpu_power_mem(void)
{
	g_its_cpu_power_data_size = sizeof(*g_its_cpu_power_data);
	g_its_cpu_power_data = kzalloc(g_its_cpu_power_data_size, GFP_KERNEL);
	if (g_its_cpu_power_data == NULL)
		return false;
	return true;
}
