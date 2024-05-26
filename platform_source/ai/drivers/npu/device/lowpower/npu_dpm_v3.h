/*
 * npu_dpm_v3.h
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
#ifndef __NPU_DMP_V3_H
#define __NPU_DMP_V3_H

#include <linux/types.h>

int npu_dpm_update_counter(int mode);
int npu_dpm_update_energy(void);

#endif /* __NPU_DMP_V3_H */
