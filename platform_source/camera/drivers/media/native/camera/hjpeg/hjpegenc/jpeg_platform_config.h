/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: platform register default config
 * Create: 2021-07-20
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef PLATFORM_CVDR_CONFIG_H
#define PLATFORM_CVDR_CONFIG_H
#include <linux/io.h>
#include <platform_include/camera/native/hjpeg_cfg.h>
#include "hjpeg_common.h"

void platform_config_jpeg_cvdr(hjpeg_hw_ctl_t *hw_ctl, jpgenc_config_t *config);

void platform_set_picture_size(void __iomem *base_addr,
	uint32_t width_left, uint32_t width_right, uint32_t height);
void platform_set_jpgenc_stride(void __iomem *base_addr, uint32_t stride);

void platform_dump_cvdr_debug_reg(hjpeg_hw_ctl_t *hw_ctl);
void platform_dump_jpeg_regs(const char *reg_type, void __iomem *kaddr, uint32_t *offsets, uint32_t size);
#endif /* end of include guard: PLATFORM_CVDR_CONFIG_H */
