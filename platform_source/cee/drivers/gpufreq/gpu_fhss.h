/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Frequency Hopped Spread Spectrum for gpu freq
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
#ifndef __GPU_FHSS_H__
#define __GPU_FHSS_H__

#ifdef CONFIG_GPU_FHSS
int fhss_init(struct device *dev);
#else
static inline int fhss_init(struct device *dev __maybe_unused){return 0;}
#endif /* CONFIG_GPU_FHSS */

#endif /* __GPU_FHSS_H__ */
