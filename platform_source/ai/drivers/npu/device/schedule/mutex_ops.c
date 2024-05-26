/*
 * placeholder.c
 *
 * about placeholder
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#include "mutex_ops.h"
#include <linux/mutex.h>

static struct mutex g_swts_mutex;

void swts_mutex_acquire(void)
{
	mutex_lock(&g_swts_mutex);
}

void swts_mutex_release(void)
{
	mutex_unlock(&g_swts_mutex);
}

void swts_mutex_destory(void)
{
	mutex_destroy(&g_swts_mutex);
}

void swts_mutex_create(void)
{
	mutex_init(&g_swts_mutex);
}