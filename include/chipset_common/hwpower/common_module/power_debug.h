/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_debug.h
 *
 * debug for power module
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _POWER_DEBUG_H_
#define _POWER_DEBUG_H_

#define POWER_DBG_ROOT_FILE        "power_debug"
#define POWER_DBG_FILE_NAME_MAX    32
#define POWER_DBG_FILE_PERMS       0600

typedef ssize_t (*power_dbg_show)(void *, char *, size_t);
typedef ssize_t (*power_dbg_store)(void *, const char *, size_t);

struct power_dbg_attr {
	char dir[POWER_DBG_FILE_NAME_MAX];
	char file[POWER_DBG_FILE_NAME_MAX];
	unsigned short mode;
	void *dev_data;
	power_dbg_show show;
	power_dbg_store store;
	struct list_head list;
};

#ifdef CONFIG_HUAWEI_POWER_DEBUG
void power_dbg_ops_register(const char *dir, const char *file,
	void *dev_data, power_dbg_show show, power_dbg_store store);
#else
static inline void power_dbg_ops_register(const char *dir, const char *file,
	void *dev_data, power_dbg_show show, power_dbg_store store)
{
}
#endif /* CONFIG_HUAWEI_POWER_DEBUG */

#endif /* _POWER_DEBUG_H_ */
