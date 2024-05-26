/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * jpeg jpu iommu
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef JPU_IOMMU_H
#define JPU_IOMMU_H

#include "jpu.h"

int jpu_enable_iommu(struct jpu_data_type *jpu_device);
phys_addr_t jpu_domain_get_ttbr(struct jpu_data_type *jpu_device);
int jpu_lb_alloc(struct jpu_data_type *jpu_device);
void jpu_lb_free(struct jpu_data_type *jpu_device);
int jpu_check_inbuff_addr(struct jpu_data_type *jpu_device,
	struct jpu_data_t *jpu_req);
int jpu_check_outbuff_addr(struct jpu_data_type *jpu_device,
	struct jpu_data_t *jpu_req);
#endif
