/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef HISI_DRV_OVERLAY_UTILS_H
#define HISI_DRV_OVERLAY_UTILS_H

#include "hisi_disp_composer.h"

int hisi_comp_traverse_src_layer(struct hisi_composer_device *composer_device, struct hisi_display_frame *frame);
int hisi_comp_traverse_online_post(struct hisi_composer_device *composer_device, struct hisi_display_frame *frame);
int hisi_comp_traverse_offline_post(struct hisi_composer_device *composer_device, struct hisi_display_frame *frame);
struct hisi_pipeline_ops *hisi_composer_get_public_ops(void);
void dpu_comp_set_dm_section_info(struct hisi_composer_device *composer_device);
#endif /* HISI_DRV_OVERLAY_UTILS_H */
