/*
 * hwts_plat.c
 *
 * about hwts plat
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

#include <linux/kernel.h>
#include "hwts_config.h"

struct hwts_group_item g_hwts_group_config_table[] = {
	// {NA, HWTS_HOST_TYPE_SENSORHUB, 0, 8},
	{HWTS_CHANNEL_GROUP0, HWTS_HOST_TYPE_ACPU, 8, 4},
	{HWTS_CHANNEL_GROUP1, HWTS_HOST_TYPE_ACPU, 12, 4},
	{HWTS_CHANNEL_GROUP2, HWTS_HOST_TYPE_ACPU, 16, 4},
	{HWTS_CHANNEL_GROUP3, HWTS_HOST_TYPE_ACPU, 20, 4},
	{HWTS_CHANNEL_GROUP4, HWTS_HOST_TYPE_ACPU, 24, 6}
};

struct hwts_priority_item g_hwts_priority_config_table[] = {
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_PLATFORM0, false, HWTS_CHANNEL_GROUP0},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_PLATFORM1, false, HWTS_CHANNEL_GROUP1},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_PLATFORM2, false, HWTS_CHANNEL_GROUP2},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_HIGHEST, false, HWTS_CHANNEL_GROUP3},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_HIGHER, true, HWTS_CHANNEL_GROUP4},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_HIGH, true, HWTS_CHANNEL_GROUP4},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_MIDDLE, true, HWTS_CHANNEL_GROUP4},
	{HWTS_RTSQ_TYPE_NS_AIC, HWTS_PRIORITY_GRP_LOW, true, HWTS_CHANNEL_GROUP4}
};

uint16_t get_hwts_group_config_table_size(void)
{
	return ARRAY_SIZE(g_hwts_group_config_table);
}

uint16_t get_hwts_priority_config_table_size(void)
{
	return ARRAY_SIZE(g_hwts_priority_config_table);
}

struct hwts_group_item *get_hwts_group_config_table(void)
{
	return g_hwts_group_config_table;
}

struct hwts_priority_item *get_hwts_priority_config_table(void)
{
	return g_hwts_priority_config_table;
}
