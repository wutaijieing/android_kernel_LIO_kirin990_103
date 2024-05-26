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
#include "jpeg_platform_config.h"
#include "hjpg_reg_offset.h"
#include "hjpg_reg_offset_field.h"
#include "cam_log.h"

void __weak platform_config_jpeg_cvdr(hjpeg_hw_ctl_t *hw_ctl, jpgenc_config_t *config)
{
	(void)hw_ctl;
	(void)config;
	cam_err("need implement in platform cvdr config");
}

void __weak platform_set_picture_size(void __iomem *base_addr,
	uint32_t width_left, uint32_t width_right, uint32_t height)
{
	SET_FIELD_TO_REG((void __iomem *)((char *)base_addr + JPGENC_JPE_ENC_HRIGHT1_REG),
		JPGENC_ENC_HRIGHT1, width_left - 1);
	SET_FIELD_TO_REG((void __iomem *)((char *)base_addr + JPGENC_JPE_ENC_VBOTTOM_REG),
		JPGENC_ENC_VBOTTOM, height - 1);
	SET_FIELD_TO_REG((void __iomem *)((char *)base_addr + JPGENC_JPE_ENC_HRIGHT2_REG),
		JPGENC_ENC_HRIGHT2, width_right != 0 ? width_right - 1 : 0);
}

void __weak platform_set_jpgenc_stride(void __iomem *base_addr, uint32_t stride)
{
	SET_FIELD_TO_REG((void __iomem *)((char *)base_addr + JPGENC_STRIDE_REG),
		JPGENC_STRIDE, stride >> 4); /* 4:offset */
}

void __weak platform_dump_cvdr_debug_reg(hjpeg_hw_ctl_t *hw_ctl)
{
	(void)hw_ctl;
	cam_err("need implement in platform cvdr config");
}

void __weak platform_dump_jpeg_regs(const char *reg_type, void __iomem *kaddr,
	uint32_t *offsets, uint32_t size)
{
	uint32_t i;
	void __iomem *reg_addr;

	for (i = 0; i < size; ++i) {
		reg_addr = (void __iomem *)((char *)kaddr + offsets[i]);
		cam_debug("%s: %s reg: Offset:%#08x, Value:%#08x",
			__func__, reg_type, offsets[i], ioread32(reg_addr));
	}
}
