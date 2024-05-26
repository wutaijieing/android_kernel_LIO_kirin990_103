// SPDX-License-Identifier: GPL-2.0
/*
 * charger_common_interface.c
 *
 * common interface for charger module
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
 *
 */

#include <chipset_common/hwpower/charger/charger_common_interface.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG charger_common
HWLOG_REGIST();

static unsigned int g_charge_wakelock_flag = CHARGE_WAKELOCK_NO_NEED;
static unsigned int g_charge_monitor_work_flag;
static unsigned int g_charge_quicken_work_flag;
static unsigned int g_charge_run_time;
static bool g_pd_charge_flag;
static bool g_pd_init_flag;
static int g_first_check;

int charge_get_first_insert(void)
{
	return g_first_check;
}

void charge_set_first_insert(int flag)
{
	pr_info("set insert flag %d\n", flag);
	g_first_check = flag;
}

unsigned int charge_get_wakelock_flag(void)
{
	return g_charge_wakelock_flag;
}

void charge_set_wakelock_flag(unsigned int flag)
{
	g_charge_wakelock_flag = flag;
	hwlog_info("set_wakelock_flag: flag=%u\n", flag);
}

unsigned int charge_get_monitor_work_flag(void)
{
	if (g_charge_monitor_work_flag == CHARGE_MONITOR_WORK_NEED_STOP)
		hwlog_info("charge monitor work already stop\n");
	return g_charge_monitor_work_flag;
}

void charge_set_monitor_work_flag(unsigned int flag)
{
	g_charge_monitor_work_flag = flag;
	hwlog_info("set_monitor_work_flag: flag=%u\n", flag);
}

unsigned int charge_get_quicken_work_flag(void)
{
	return g_charge_quicken_work_flag;
}

void charge_reset_quicken_work_flag(void)
{
	g_charge_quicken_work_flag = 0;
	hwlog_info("reset_quicken_work_flag\n");
}

void charge_update_quicken_work_flag(void)
{
	g_charge_quicken_work_flag++;
	hwlog_info("update_quicken_work_flag flag=%u\n", g_charge_quicken_work_flag);
}

unsigned int charge_get_run_time(void)
{
	return g_charge_run_time;
}

void charge_set_run_time(unsigned int time)
{
	g_charge_run_time = time;
	hwlog_info("set_run_time: time=%u\n", time);
}

void charge_set_pd_charge_flag(bool flag)
{
	g_pd_charge_flag = flag;
	hwlog_info("set_pd_charge_flag: flag=%u\n", flag);
}

bool charge_get_pd_charge_flag(void)
{
	return g_pd_charge_flag;
}

void charge_set_pd_init_flag(bool flag)
{
	g_pd_init_flag = flag;
	hwlog_info("set_pd_init_flag: flag=%u\n", flag);
}

bool charge_get_pd_init_flag(void)
{
	return g_pd_init_flag;
}
