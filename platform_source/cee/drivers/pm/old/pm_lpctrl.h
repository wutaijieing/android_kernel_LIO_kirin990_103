/*
 * pm_lpctrl.h
 *
 * The AP Kernel communicates with LPM3 through IPC and
 * passes some SR control of the application layer.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#ifndef _HISI_LOWPM_CTRL_H
#define _HISI_LOWPM_CTRL_H

#include <linux/types.h>

enum lowpm_errno {
	LOWPM_IPC_SUCCESS = 0,
	LOWPM_IPC_EINVAL = 1,
	LOWPM_IPC_ECOMM = 2,
	LOWPM_IPC_EFAILED = 3
};

int pm_lpctrl_init(void);

#endif
