/*
 * vcodec_vdec_utils.c
 *
 * This is for vdec utils
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

#include <linux/sched/clock.h>
#include "vcodec_vdec_utils.h"
#include "dbg.h"
#include "securec.h"

uint64_t g_power_operation_cost[FUNC_CNT][FUNC_COST_RANGE];

uint64_t get_time_in_us(void)
{
	uint64_t sys_time;

	sys_time = sched_clock();
	do_div(sys_time, 1000);

	return sys_time;
}

void vdec_init_power_cost_record(void)
{
	(void)memset_s(g_power_operation_cost, sizeof(uint64_t) * FUNC_COST_RANGE * FUNC_CNT,
		0, sizeof(uint64_t) * FUNC_COST_RANGE * FUNC_CNT);
}

uint64_t *vdec_get_power_on_cost_record(void)
{
	return g_power_operation_cost[FUNC_NAME_POWER_ON];
}

uint64_t *vdec_get_power_off_cost_record(void)
{
	return g_power_operation_cost[FUNC_NAME_POWER_OFF];
}


void vdec_print_power_statistics(uint64_t *power_off_duration, uint64_t *power_off_times)
{
	dprint(PRN_ALWS,
		"close vdec: power_off_duration=%llu,power_off_times=%llu, \
		power_on_cost_distribution %llu,%llu,%llu,%llu,%llu,%llu,%llu,\
		power_off_cost_distribution %llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
		*power_off_duration, *power_off_times,
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_1],
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_2],
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_3],
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_4],
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_5],
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_6],
		g_power_operation_cost[FUNC_NAME_POWER_ON][TIME_COST_SEG_7],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_1],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_2],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_3],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_4],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_5],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_6],
		g_power_operation_cost[FUNC_NAME_POWER_OFF][TIME_COST_SEG_7]);

	*power_off_duration = 0;
	*power_off_times = 0;

	(void)memset_s(g_power_operation_cost, sizeof(uint64_t) * FUNC_COST_RANGE * FUNC_CNT,
		0, sizeof(uint64_t) * FUNC_COST_RANGE * FUNC_CNT);
}

void vdec_record_power_operation_cost(uint64_t start_time, uint64_t *time_cost)
{
	uint64_t cost = get_time_in_us() - start_time;
	if (cost <= FUNC_COST_SEG_1)
		time_cost[TIME_COST_SEG_1]++;
	else if (cost <= FUNC_COST_SEG_2)
		time_cost[TIME_COST_SEG_2]++;
	else if (cost <= FUNC_COST_SEG_3)
		time_cost[TIME_COST_SEG_3]++;
	else if (cost <= FUNC_COST_SEG_4)
		time_cost[TIME_COST_SEG_4]++;
	else if (cost <= FUNC_COST_SEG_5)
		time_cost[TIME_COST_SEG_5]++;
	else if (cost <= FUNC_COST_SEG_6)
		time_cost[TIME_COST_SEG_6]++;
	else
		time_cost[TIME_COST_SEG_7]++;
}

