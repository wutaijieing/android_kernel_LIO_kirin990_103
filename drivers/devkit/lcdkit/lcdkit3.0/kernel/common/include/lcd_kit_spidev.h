/*
 * lcd_kit_spidev.h^M
 *
 * lcd kit backlight driver of lcd backlight ic^M
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
#ifndef SPIDEV_H
#define SPIDEV_H

struct lcd_kit_spi_hrdy {
	int  gpio;
	int  status;
	int  irq;
	struct mutex hrdy_mutex;
};

#endif /* SPIDEV_H */
