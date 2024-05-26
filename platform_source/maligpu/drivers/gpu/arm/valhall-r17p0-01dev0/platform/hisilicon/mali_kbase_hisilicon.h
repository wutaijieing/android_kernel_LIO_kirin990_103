/*
 *
 * (C) COPYRIGHT 2017 ARM Limited. All rights reserved.
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
#ifndef _MALI_KBASE_HISILICON_H_
#define _MALI_KBASE_HISILICON_H_

#include "mali_kbase_hisi_callback.h"
#include "hisi_ipa/mali_kbase_ipa_ctx.h"

/**
 * struct kbase_hisi_device_data - all hisi platform data in device level.
 *
 * @callbacks: The callbacks hisi implements.
 */
struct kbase_hisi_device_data {
	struct kbase_hisi_callbacks *callbacks;

	/* Add other device data here */

	/* Data about hisi dynamic IPA*/
	struct kbase_ipa_context *ipa_ctx;
	struct work_struct bound_detect_work;
	unsigned long bound_detect_freq;
	unsigned long bound_detect_btime;
};

/**
 * struct kbase_hisi_ctx_data - all hisi platform data in context level.
 */
struct kbase_hisi_ctx_data {

/* Add other context data here */
};

#endif /* _MALI_KBASE_HISILICON_H_ */
