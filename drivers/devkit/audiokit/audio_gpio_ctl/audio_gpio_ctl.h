/*
 * audio_gpio_ctl.h
 *
 * audio_gpio_ctl driver
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#ifndef __AUDIO_GPIO_CTL_H__
#define __AUDIO_GPIO_CTL_H__

#include <linux/mutex.h>
#include <linux/notifier.h>

#define TDD_CTRL      _IOW('A', 0x01, int)

struct audio_gpio_ctrl_priv {
	int tdd_gpio;
	struct mutex do_ioctl_lock;
	struct device *dev;
	struct notifier_block event_nb;
};

#endif  /* __AUDIO_GPIO_CTL_H__ */
