/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * Header file for device driver pmic23V200 PMIC
 */

#ifndef	__HISI_6423_PMIC_H
#define	__HISI_6423_PMIC_H

#include <linux/irqdomain.h>

struct pmic_23v200 {
	struct resource  	*res;
	struct device    	*dev;
	void __iomem		*regs;
	struct delayed_work   check_6423_vbatt_work;
	struct pinctrl *pctrl;
	struct pinctrl_state *pctrl_default;
	struct pinctrl_state *pctrl_idle;
	int		  	       irq;
	int                gpio;
	unsigned int                sid;
	unsigned int                irq_mask_addr;
	unsigned int                irq_addr;
	unsigned int                irq_np_record;
};

#endif		 /* __HISI_6423_PMIC_H */
