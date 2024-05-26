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
#ifndef HISI_DSSV700_UTILS_H
#define HISI_DSSV700_UTILS_H

#ifdef CONFIG_DKMD_DPU_V720
#define HISIFB_DSS_PLATFORM_TYPE  (FB_ACCEL_DSSV720 | FB_ACCEL_PLATFORM_TYPE_ASIC)
#else
#define HISIFB_DSS_PLATFORM_TYPE  (FB_ACCEL_DSSV700 | FB_ACCEL_PLATFORM_TYPE_ASIC)
#endif

#endif /* HISI_DSSV700_UTILS_H */
