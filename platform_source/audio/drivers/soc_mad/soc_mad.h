/*
 * soc_mad.h codec driver.
 *
 * Copyright (c) 2019 Huawei Technologies Co., Ltd.
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


#ifndef __SOC_MAD_H__
#define __SOC_MAD_H__

#define PINCTRL_STATE_NORMAL_1 "normal_1"

enum mad_mode {
	HIGH_FREQ_MODE,
	LOW_FREQ_MODE,
	MODE_CNT,
};

enum mad_application_mode {
	MAD_AU,
	MAD_DMIC,
	MAD_MODE_CNT,
	MAD_MODE_INVALID,
};

void soc_mad_set_pinctrl_state(unsigned int mode);
void soc_mad_select_din(unsigned int mode);
int soc_mad_request_pinctrl_state(unsigned int mode);
int soc_mad_release_pinctrl_state(unsigned int mode);
#endif

