/*
 * hwts_plat.h
 *
 * about hwts plat
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#ifndef _HWTS_PLAT_H_
#define _HWTS_PLAT_H_

#include <linux/types.h>
#include "hwts_config.h"

#define HWTS_SP_LEVEL 3
#define HWTS_GROUP_NUM 5
#define HWTS_CHANNEL_NUM 64

struct hwts_group_item *get_hwts_group_config_table(void);
struct hwts_priority_item *get_hwts_priority_config_table(void);
uint16_t get_hwts_group_config_table_size(void);
uint16_t get_hwts_priority_config_table_size(void);
#endif
