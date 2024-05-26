/*
 * vcodec_type.h
 *
 * This is The common data type defination.
 *
 * Copyright (c) 2008-2020 Huawei Technologies CO., Ltd.
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

#ifndef VCODEC_TYPE_H
#define VCODEC_TYPE_H

#include "securec.h"

#ifdef VCODECV500
#define UADDR uint64_t
#else
#define UADDR uint32_t
#endif

#ifndef NULL
#define NULL             0L
#endif
#define VCODEC_NULL          0L
#define VCODEC_FAILURE       (-1)

#ifdef PCIE_LINK
#define VCODEC_ATTR_WEEK __attribute__((weak))
#else
#define VCODEC_ATTR_WEEK
#endif

/* magic number 0-255 */
#define IOC_TYPE_VENC     'V'
#define VERSION_STRING    "1234"
#endif
