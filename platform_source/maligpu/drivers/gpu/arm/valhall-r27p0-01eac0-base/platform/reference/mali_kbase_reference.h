/*
 * (C) COPYRIGHT 2019 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#ifndef MALI_KBASE_reference_H
#define MALI_KBASE_reference_H

/**
 * struct kbase_reference_device_data - all reference platform data in device level.
 */
struct kbase_reference_device_data {
	struct clk *clock;
#ifdef CONFIG_REGULATOR
	struct regulator *regulator;
#endif
	struct clk *clk;
#ifdef CONFIG_PM_DEVFREQ
	struct devfreq *devfreq;
#endif /* CONFIG_PM_DEVFREQ */

	u32 gpu_fpga_exist;
};

#endif /* MALI_KBASE_reference_H */
