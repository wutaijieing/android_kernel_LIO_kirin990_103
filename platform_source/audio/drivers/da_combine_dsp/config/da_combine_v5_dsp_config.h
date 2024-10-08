/*
 * da_combine_v5_dsp_config.h
 *
 * dsp init
 *
 * Copyright (c) 2015-2019 Huawei Technologies Co., Ltd.
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

#ifndef __DA_COMBINE_V5_DSP_CONFIG_H__
#define __DA_COMBINE_V5_DSP_CONFIG_H__

#include <linux/platform_drivers/da_combine/da_combine_irq.h>
#include <linux/platform_drivers/da_combine/da_combine_resmgr.h>
#include <linux/platform_drivers/da_combine/da_combine_mbhc.h>
#include <linux/platform_drivers/da_combine/da_combine_v5_type.h>

int da_combine_v5_dsp_config_init(struct snd_soc_component *codec,
	struct da_combine_v5_platform_data *data);
void da_combine_v5_dsp_config_deinit(void);

#endif

