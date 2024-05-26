/*
 * tee_compat_check.h
 *
 * check compatibility between tzdriver and teeos.
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
 */

#ifndef TEE_COMPAT_CHECK_H
#define TEE_COMPAT_CHECK_H

#include <linux/types.h>

/*
 * this version number MAJOR.MINOR is used
 * to identify the compatibility of tzdriver and teeos
 */
#define TEEOS_COMPAT_LEVEL_MAJOR 0
#define TEEOS_COMPAT_LEVEL_MINOR 1

#define VER_CHECK_MAGIC_NUM 0x5A5A5A5A
#define COMPAT_LEVEL_BUF_LEN 12

int32_t check_teeos_compat_level(uint32_t *buffer, uint32_t size);
#endif
