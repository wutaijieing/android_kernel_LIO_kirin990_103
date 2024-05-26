/*
 * vfmw_proc_stm.h
 *
 * This is for vfmw proc.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
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

#ifndef VFMW_PROC_STM
#define VFMW_PROC_STM

#include "linux_proc.h"

int stm_read_proc(PROC_FILE *file, void *data);
int stm_write_proc(struct file *file, const char __user *buffer, size_t count,
	loff_t *data);

#endif
