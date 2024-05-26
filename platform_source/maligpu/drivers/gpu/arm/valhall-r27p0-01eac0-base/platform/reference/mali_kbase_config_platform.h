/*
 *
 * (C) COPYRIGHT 2014-2017 ARM Limited. All rights reserved.
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

#ifndef MALI_KBASE_CONFIG_PLATFORM_H
#define MALI_KBASE_CONFIG_PLATFORM_H

#define SYS_REG_PERICTRL_BASE_ADDR         0xFEC3E000
#define SYS_REG_REMAP_SIZE                 0x1000

#if defined (CONFIG_MALI_ODIN)
#define PERI_STAT_FPGA_GPU_EXIST           0xBC
#define PERI_STAT_FPGA_GPU_EXIST_MASK      0x20
#else
#define PERI_STAT_FPGA_GPU_EXIST           0xBC
#define PERI_STAT_FPGA_GPU_EXIST_MASK      0x1
#endif

/**
 * Power management configuration
 *
 * Attached value: pointer to @ref kbase_pm_callback_conf
 * Default value: See @ref kbase_pm_callback_conf
 */
#define POWER_MANAGEMENT_CALLBACKS (&pm_callbacks)

#define KBASE_PLATFORM_CALLBACKS ((uintptr_t)&platform_funcs)

/**
 * Platform specific configuration functions
 *
 * Attached value: pointer to @ref kbase_platform_funcs_conf
 * Default value: See @ref kbase_platform_funcs_conf
 */
#define PLATFORM_FUNCS (KBASE_PLATFORM_CALLBACKS)

extern struct kbase_pm_callback_conf pm_callbacks;
extern struct kbase_platform_funcs_conf platform_funcs;
#endif /* MALI_KBASE_CONFIG_PLATFORM_H */
