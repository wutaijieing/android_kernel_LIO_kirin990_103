/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: gputop freq file
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
#ifndef __GPU_TOP_H__
#define __GPU_TOP_H__

#ifdef CONFIG_GPUTOP_FREQ
/* MHz */
void ipa_gputop_freq_limit(unsigned long freq);
void update_gputop_linked_freq(unsigned long freq);
#else
void ipa_gputop_freq_limit(unsigned long freq __maybe_unused){}
void update_gputop_linked_freq(unsigned long freq __maybe_unused){}
#endif /* __CONFIG_GPUTOP_FREQ__ */

#endif /* __GPU_TOP_H__ */
