/*
 * vfmw_osal.c
 *
 * This is for vfmw_osal interface.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "vfmw_osal.h"
#include "dbg.h"

vfmw_osal_ops g_vfmw_osal_ops;

sec_mode get_current_sec_mode(void)
{
	return linux_get_cur_sec_mode();
}
