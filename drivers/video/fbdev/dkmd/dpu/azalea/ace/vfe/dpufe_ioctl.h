/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPUFE_IOCTL_H
#define DPUFE_IOCTL_H

#include "dpufe.h"
#include "dpufe_iommu.h"

#define DPUFE_IOCTL_MAGIC 'M'

#define DPUFE_VSYNC_CTRL _IOW(DPUFE_IOCTL_MAGIC, 0x01, uint32_t)
#define DPUFE_PLATFORM_PRODUCT_INFO_GET _IOW(DPUFE_IOCTL_MAGIC, 0x02, struct platform_product_info)
#define DPUFE_OV_OFFLINE_PLAY _IOW(DPUFE_IOCTL_MAGIC, 0x04, struct dss_overlay)
#define DPUFE_OV_ASYNC_PLAY _IOW(DPUFE_IOCTL_MAGIC, 0x05, struct dss_overlay)
#define DPUFE_GET_RELEASE_AND_RETIRE_FENCE _IOW(DPUFE_IOCTL_MAGIC, 0x06, struct dss_fence)
#define DPUFE_LCD_DIRTY_REGION_INFO_GET _IOW(DPUFE_IOCTL_MAGIC, 0x07, struct lcd_dirty_region_info)
#define DPUFE_PLATFORM_PANEL_INFO_GET _IOW(DPUFE_IOCTL_MAGIC, 0x08, struct platform_panel_info)
#define DPUFE_PLATFORM_TYPE_GET _IOW(DPUFE_IOCTL_MAGIC, 802, int)

#define DPUFE_OV_ONLINE_PLAY _IOW(DPUFE_IOCTL_MAGIC, 0x21, struct dss_overlay)
#define DPUFE_GRALLOC_MAP_IOVA _IOW(DPUFE_IOCTL_MAGIC, 807, struct iova_info)
#define DPUFE_GRALLOC_UNMAP_IOVA _IOW(DPUFE_IOCTL_MAGIC, 808, struct iova_info)
#define DPUFE_EVS_SWITCH _IOW(DPUFE_IOCTL_MAGIC, 0xB0, int)

#define DPUFE_MMBUF_ALLOC _IOW(DPUFE_IOCTL_MAGIC, 0x08, struct dss_mmbuf)
#define DPUFE_MMBUF_FREE _IOW(DPUFE_IOCTL_MAGIC, 0x09, struct dss_mmbuf)
#define DPUFE_MMBUF_FREE_ALL _IOW(DPUFE_IOCTL_MAGIC, 0x14, int)
#endif
