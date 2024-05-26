/*
 * vcodec_types.h
 *
 * This is omx self define type.
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

#ifndef VCODEC_TYPES_H
#define VCODEC_TYPES_H

#if defined(__KERNEL__)
#include <linux/version.h>
#include <linux/types.h>
#else
#include <stdint.h>
#endif
#include "securec.h"

typedef unsigned long           vcodec_size_t;

typedef unsigned long           vcodec_virt_addr_t;
typedef uint32_t                vcodec_handle;
typedef uint32_t                UADDR;

typedef enum {
	VCODEC_FALSE = 0,
	VCODEC_TRUE = 1,
} vcodec_bool;

#define VCODEC_NULL           0L

#define VCODEC_SUCCESS        0
#define VCODEC_FAILURE        (-1)
#define unused_param(p) (void)(p)

#endif
