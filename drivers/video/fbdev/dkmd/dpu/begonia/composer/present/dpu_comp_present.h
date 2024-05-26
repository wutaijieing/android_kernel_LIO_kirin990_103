/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef COMPOSER_PRESENT_H
#define COMPOSER_PRESENT_H

void composer_online_timeline_resync(struct dpu_composer *dpu_comp, struct comp_online_present *present);
void composer_present_data_setup(struct dpu_composer *dpu_comp, bool inited);
void composer_present_data_release(struct dpu_composer *dpu_comp);

#endif
