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

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/module.h>

#include "dkmd_product_interface.h"

int hostprocessing_get_project_id_for_udp(char *out, int len)
{
	if (!out)
		return -1;

	if (len < MIPI_LCD_PROJECT_ID_LEN) {
		pr_err("input is invalid len!\n");
		return -1;
	}
	strncpy(out, "P149931300", MIPI_LCD_PROJECT_ID_LEN);

	return 0;
}
EXPORT_SYMBOL(hostprocessing_get_project_id_for_udp);

int dpufb_esd_recover_disable(int value)
{
	pr_info("value=%d\n", value);

	return 0;
}
EXPORT_SYMBOL(dpufb_esd_recover_disable);