/* SPDX-License-Identifier: GPL-2.0 */
/*
 * board.c
 *
 * board config
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

#include "board.h"
#include <linux/of_gpio.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include "../swi/ns3300_swi.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG ns3300_swi
HWLOG_REGIST();

/* time measurement setting */
#ifndef us2cycles
#define us2cycles(x)  (((((x) * 0x10c7UL) * loops_per_jiffy * HZ) + \
	(1UL << 31)) >> 32)
#endif

static int g_onewire_gpio;

void pin_init(struct ns3300_dev *dev)
{
	if (!dev)
		return;

	g_onewire_gpio = dev->onewire_gpio;
}

uint8_t get_pin(void)
{
	return gpio_get_value(g_onewire_gpio);
}

void set_pin(uint8_t level)
{
	gpio_set_value(g_onewire_gpio, level);
}

void set_pin_dir(uint8_t dir, uint8_t level)
{
	if (dir)
		gpio_direction_output(g_onewire_gpio, level);
	else
		gpio_direction_input(g_onewire_gpio);
}

void ns3300_udelay(uint32_t us)
{
	cycles_t start = get_cycles();
	cycles_t cnt_gap = 0;
	unsigned long cnt_delay = us2cycles(us);

	while (cnt_gap < cnt_delay)
		cnt_gap = (get_cycles() - start);
}

void ns3300_ndelay(uint32_t ns)
{
	cycles_t start = get_cycles();
	cycles_t cnt_gap = 0;
	unsigned long cnt_1us = us2cycles(1);
	unsigned long cnt_delay = (cnt_1us * ns) / 1000; /* ns to us */

	while (cnt_gap < cnt_delay)
		cnt_gap = (get_cycles() - start);
}

int swi_init(struct ns3300_dev *dev)
{
	if (!dev) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return -EINVAL;
	}

	pin_init(dev);
	timing_init(dev);
	return 0;
}
