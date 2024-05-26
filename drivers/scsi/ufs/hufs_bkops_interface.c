/*
 * hufs_bkops_interface.c
 *
 * ufs block interface
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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
#include "hufs_bkops_interface.h"


int __ufshcd_bkops_status_query(void *bkops_data, u32 *status)
{
	return ufshcd_bkops_status_query(bkops_data, status);
}


int __ufshcd_bkops_start_stop(void *bkops_data, int start)
{
	return ufshcd_bkops_start_stop(bkops_data, start);
}
