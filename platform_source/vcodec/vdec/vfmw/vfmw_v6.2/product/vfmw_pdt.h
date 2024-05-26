/*
 * vfmw_pdt.h
 *
 * This is for product global interface.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef PRODUCT_H
#define PRODUCT_H

#include "vfmw_osal.h"
#include "vfmw_sys.h"

struct pdt_config {
	uint32_t is_fpga;
	uint32_t is_smmu_bypass;
};

struct vfmw_global_info *pdt_get_glb_info(void);
int32_t pdt_init(void *args, struct pdt_config *cfg, void *dev);
void pdt_deinit(void);

#endif
