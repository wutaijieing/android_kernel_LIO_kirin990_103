/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * pmic_vm.h
 *
 * Head file for pmic vm DRIVER
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
#ifndef _PMIC_VM
#define _PMIC_VM

#include <linux/device.h>
#include <linux/regmap.h>
#include <linux/spinlock.h>

#define PMIC_VM_ID 0xabcd0000
#define PMIC_READ 0
#define PMIC_WRITE 1

struct pmic_vm_dev {
		struct device *dev;
		u8 bus_id;
		u32 slave_id;
		struct regmap *regmap;
		spinlock_t buffer_lock;
		int reg_bits;
		int val_bits;
};
#endif
