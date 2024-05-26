/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: v350 platform jpegenc register config
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
#include "soc_ipp_jpgenc_interface.h"
#include "jpeg_platform_config.h"
#include "cam_log.h"

void platform_set_picture_size(void __iomem *base_addr,
	uint32_t width_left, uint32_t width_right, uint32_t height)
{
	void __iomem *hright1_addr = SOC_IPP_JPGENC_JPE_ENC_HRIGHT1_ADDR((char *)base_addr);
	void __iomem *hright2_addr = SOC_IPP_JPGENC_JPE_ENC_HRIGHT2_ADDR((char *)base_addr);
	void __iomem *vbottom_addr = SOC_IPP_JPGENC_JPE_ENC_VBOTTOM_ADDR((char *)base_addr);
	SOC_IPP_JPGENC_JPE_ENC_HRIGHT1_UNION hright1;
	SOC_IPP_JPGENC_JPE_ENC_HRIGHT2_UNION hright2;
	SOC_IPP_JPGENC_JPE_ENC_VBOTTOM_UNION vbottom;

	hright1.value = ioread32(hright1_addr);
	hright2.value = ioread32(hright2_addr);
	vbottom.value = ioread32(vbottom_addr);

	hright1.reg.enc_hright1 = width_left - 1;
	hright2.reg.enc_hright2 = width_right != 0 ? width_right - 1 : 0;
	vbottom.reg.enc_vbottom = height - 1;

	iowrite32(hright1.value, hright1_addr);
	iowrite32(hright2.value, hright2_addr);
	iowrite32(vbottom.value, vbottom_addr);
}

void platform_set_jpgenc_stride(void __iomem *base_addr, uint32_t stride)
{
	void __iomem *stride_addr = SOC_IPP_JPGENC_STRIDE_ADDR((char *)base_addr);
	SOC_IPP_JPGENC_STRIDE_UNION reg_stride;

	reg_stride.value = ioread32(stride_addr);
	reg_stride.reg.stride = stride >> 4;
	iowrite32(reg_stride.value, stride_addr);
}
