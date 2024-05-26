/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#ifndef __SLIMBUS_STDTYPES_H__
#define __SLIMBUS_STDTYPES_H__

#include <linux/types.h>

/* Define NULL constant */
#ifndef NULL
#define NULL	((void *)0)
#endif

/* Define bool data type */
#define bool	_Bool
#define true	1
#define false	0
#define __bool_true_false_are_defined 1

#endif	/* __SLIMBUS_STDTYPES_H__ */
