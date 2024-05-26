/*
 * secs_power_ctrl.h
 *
 * function declaration for secs power ctrl
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
 */
#ifndef SECS_POWER_CTRL_H
#define SECS_POWER_CTRL_H

#include <tc_ns_log.h>

#ifdef CONFIG_HISI_SECS_CTRL
#include <platform_include/see/secs_power_ctrl.h>

#define SECS_SUSPEND_STATUS 0xA5A5
unsigned long get_secs_suspend_status(void);

static int power_on_cc(void)
{
	return hisi_secs_power_on();
}

static int power_down_cc(void)
{
	return hisi_secs_power_down();
}

static void secs_suspend_status(uint64_t target)
{
	if (get_secs_suspend_status() == SECS_SUSPEND_STATUS)
		tloge("WARNING irq during suspend! No = %lld\n", target);
}

#else

static int power_on_cc(void)
{
	return 0;
}

static int power_down_cc(void)
{
	return 0;
}

static void secs_suspend_status(uint64_t target)
{
	(void)target;

	return;
}

#endif

#endif
