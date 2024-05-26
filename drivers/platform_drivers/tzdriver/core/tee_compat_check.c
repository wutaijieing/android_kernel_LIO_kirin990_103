/*
 * tee_compat_check.c
 *
 * check compatibility between tzdriver and tee.
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

#include "tee_compat_check.h"
#include <linux/types.h>
#include <linux/err.h>
#include "teek_ns_client.h"
#include "tc_ns_log.h"

int32_t check_teeos_compat_level(uint32_t *buffer, uint32_t size)
{
	const uint16_t major = TEEOS_COMPAT_LEVEL_MAJOR;
	const uint16_t minor = TEEOS_COMPAT_LEVEL_MINOR;

	if (!buffer || size != COMPAT_LEVEL_BUF_LEN) {
		tloge("check teeos compat level failed, invalid param\n");
		return -EINVAL;
	}

	if (buffer[0] != VER_CHECK_MAGIC_NUM) {
		tloge("check ver magic num %u failed\n", buffer[0]);
		return -EPERM;
	}
	if (buffer[1] != major) {
		tloge("check major ver failed, major tz=%u, major tee=%u\n",
			major, buffer[1]);
		return -EPERM;
	}
	/* just print warning */
	if (buffer[2] != minor)
		tlogw("check minor ver failed, minor tz=%u, minor tee=%u\n",
			minor, buffer[2]);
	return 0;
}
