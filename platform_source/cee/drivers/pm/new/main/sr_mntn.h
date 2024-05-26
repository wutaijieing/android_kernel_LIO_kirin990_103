/*
 * sr_mntn.h
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

#ifndef __H_PM_SR_MNTN_H__
#define __H_PM_SR_MNTN_H__

#include "pm.h"

void lowpm_sr_mntn_suspend(void);
void lowpm_sr_mntn_resume(void);

#endif /* __H_PM_SR_MNTN_H__ */
