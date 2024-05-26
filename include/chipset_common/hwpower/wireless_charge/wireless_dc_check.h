/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_dc_check.h
 *
 * head file for wireless dc check
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

#ifndef _WIRELESS_DC_CHECK_H_
#define _WIRELESS_DC_CHECK_H_

#include <linux/types.h>

#define WLDC_ERR_CNT_MAX                  8
#define WLDC_WARNING_CNT_MAX              8
#define WLDC_OPEN_RETRY_CNT_MAX           3

#define WLDC_VOUT_ACCURACY_CHECK_CNT      3
#define WLDC_OPEN_PATH_CNT                6
#define WLDC_OPEN_PATH_IOUT_MIN           500
#define WLDC_LEAK_CURRENT_CHECK_CNT       6
#define WLDC_VDIFF_CHECK_CNT              3

struct wldc_dev_info;

bool wldc_is_tmp_forbidden(struct wldc_dev_info *di, int mode);
bool wlrx_pmode_dc_judge_crit(const char *mode_name, int pid);

#endif /* _WIRELESS_DC_CHECK_H_ */
