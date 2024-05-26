/*
 * npu_dpm.h
 *
 * about npu dpm
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __NPU_DMP_V2_H
#define __NPU_DMP_V2_H

#include <linux/types.h>

struct npu_dpm_addr {
	unsigned long long phy_addr;
	void __iomem *vir_addr;
};

int npu_dpm_update_counter(void);

#endif /* __NPU_DMP_V2_H */
