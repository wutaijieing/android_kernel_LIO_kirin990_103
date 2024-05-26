/** @file
 * Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef DPU_FPGA_DEBUG_H
#define DPU_FPGA_DEBUG_H

#include <linux/module.h>

void dss_save_pdev(uint32_t index, struct platform_device *pdev);
bool dss_is_system_on(void);
void dss_init_fpgacontext(void);
void dss_deinit_fpgacontext(void);

#endif
