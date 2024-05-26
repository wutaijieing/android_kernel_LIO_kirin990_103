/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef DISPLAY_EFFECT_ALSC_H
#define DISPLAY_EFFECT_ALSC_H
#include "../composer/operators/dpp/hisi_operator_dpp.h"
#include "display_effect_alsc_interface.h"

extern bool g_reg_inited;
extern bool g_noise_got;

void hisi_alsc_store_data(struct hisi_operator_dpp *hisi_opert, uint32_t isr_flag, char __iomem *dpu_base_addr);
void hisi_alsc_init(struct hisi_operator_dpp *hisi_opert);
void hisi_alsc_set_reg(struct hisi_operator_dpp *hisi_opert, char __iomem *dpu_base_addr);
#endif
