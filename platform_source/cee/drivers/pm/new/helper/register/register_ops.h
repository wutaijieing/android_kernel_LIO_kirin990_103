/*
 *
 * regesiter_ops.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#ifndef __H_PM_REGESITER_OPS_H__
#define __H_PM_REGESITER_OPS_H__

enum LP_REG_DOMAIN {
	SCTRL,
	PMCCTRL,
	CRGPERI,
	PMU,
	IPC,
	AO_IPC,
	REG_INVALID,
};

int map_reg_base(enum LP_REG_DOMAIN domain);

enum LP_REG_DOMAIN lp_get_reg_domain(const char *domain);
unsigned int lp_pmic_readl(int offset);
void lp_pmic_writel(int offset, int val);
unsigned int read_reg(enum LP_REG_DOMAIN domain, int offset);
void write_reg(enum LP_REG_DOMAIN domain, int offset, int val);
void enable_bit(enum LP_REG_DOMAIN domain, int offset, unsigned int nr);
void disable_bit(enum LP_REG_DOMAIN domain, int offset, unsigned int nr);

#ifdef CONFIG_LPMCU_BB
int init_runtime_base(void);
inline unsigned int runtime_read(int offset);
inline int is_runtime_base_inited(void);
#else
static inline int init_runtime_base(void) { return 0; };
static inline unsigned int runtime_read(int offset) { return 0; };
static inline int is_runtime_base_inited(void) { return 0; };
#endif /* CONFIG_LPMCU_BB */

#endif /* __H_PM_REGESITER_OPS_H__ */
