/*
 * trace_utils.h
 *
 * trace utils
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

#ifndef _TRACE_UTILS_H_
#define _TRACE_UTILS_H_

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/time.h>

#define TRACE_BASIC_YEAR      1900

void trace_get_time(struct tm *ptm);
void trace_real_log_set(bool real);
bool trace_real_log(void);

#endif /* _TRACE_UTILS_H_ */