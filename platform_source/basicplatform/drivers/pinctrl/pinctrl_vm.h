/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
 *
 * pinctrl_vm.h
 *
 * add pinctrl vm dev for hm_uvmm
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef PINCTRL_VM_H
#define PINCTRL_VM_H

#include <linux/arm-smccc.h>

#define PINCTRL_VM_ID 0xD00D0000
#define PINCTRL_READ 1
#define PINCTRL_WRITE 2
#define PINCTRL_MUX_SET 3

static inline unsigned int pinctrl_hvc(unsigned int pinctrl_hvc_id, unsigned int x1,
		unsigned int x2, unsigned int x3)
{
	struct arm_smccc_res res = {0};

	arm_smccc_hvc((u64)pinctrl_hvc_id, (u64)x1, (u64)x2, (u64)x3,
			0, 0, 0, 0, &res);
	return (unsigned int)res.a0;
}

static inline unsigned pcs_virt_read(void __iomem *reg)
{
	return pinctrl_hvc(PINCTRL_VM_ID | PINCTRL_READ, (size_t)reg, 0, 0);
}

static inline void pcs_virt_write(unsigned val, void __iomem *reg)
{
	pinctrl_hvc(PINCTRL_VM_ID | PINCTRL_WRITE, (size_t)reg, (size_t)val, 0);
}

static inline void pcs_mux_virt_set(size_t reg, size_t val)
{
	pinctrl_hvc(PINCTRL_VM_ID | PINCTRL_MUX_SET, reg, val, 0);
}

#endif