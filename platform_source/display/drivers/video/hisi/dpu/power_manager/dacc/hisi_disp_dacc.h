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

#ifndef HISI_DISP_DACC_H
#define HISI_DISP_DACC_H

void hisi_disp_dacc_init(char __iomem *dss_base);
void hisi_disp_dacc_scene_config(unsigned scene_id, bool enable_cmdlist, char __iomem *dpu_base,
	struct hisi_display_frame *frame);

#endif
