/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 *
 * driver for vbat drop protect
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __HISI_VBAT_DROP_PROTECT_CHAR_H
#define __HISI_VBAT_DROP_PROTECT_CHAR_H


/* pmic vsys drop reg */
#define PMIC_VSYS_DROP_VOL_SET			PMIC_NP_VSYS_DROP_SET_ADDR(0)
#define PMIC_VSYS_DROP_IRQ_MASK_REG		PMIC_IRQ_MASK_20_ADDR(0)
#define PMIC_VBAT_DROP_VOL_SET_REG		PMIC_NP_VSYS_DROP_SET_ADDR(0)

/* core enable reg in pmctrl */
#define LITTLE_VOL_DROP_EN_ADDR(base)       0
#define MIDDLE_VOL_DROP_EN_ADDR(base)       0
#define BIG_VOL_DROP_EN_ADDR(base)          0
#define L3_VOL_DROP_EN_ADDR(base)           0
#define GPU_VOL_DROP_EN_ADDR(base)          0
#define NPU_VOL_DROP_EN_ADDR(base)          0

#define LITTLE_VOL_DROP_EN_BIT          0
#define MIDDLE_VOL_DROP_EN_BIT          0
#define BIG_VOL_DROP_EN_BIT             0
#define L3_VOL_DROP_EN_BIT              0
#define GPU_VOL_DROP_EN_BIT             0

/* reg bit write en */
#define LITTLE_VOL_DROP_MASK          0
#define MIDDLE_VOL_DROP_MASK          0
#define BIG_VOL_DROP_MASK             0
#define L3_VOL_DROP_MASK              0
#define GPU_VOL_DROP_MASK             0
#endif
