/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description:
 * Author:
 * Create: 2018-12-22
 */
#include <linux/idr.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <asm/uaccess.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include "npu_hw_exp_irq.h"
#include "soc_npu_hw_exp_irq_interface.h"
#include "npu_log.h"
#include "npu_platform.h"

// get hw irq offset according the receving irq num
#define npu_shift_left_val(irq_offset)   (0x1 << (irq_offset))

static void update_reg32(u64 addr, u32 value, u32 mask)
{
	u32 reg_val = 0;

	reg_val = readl((const volatile void *)(uintptr_t)addr);
	reg_val = (reg_val & (~mask)) | (value & mask);
	writel(reg_val, (volatile void *)(uintptr_t)addr);
}
static int npu_parse_gic_irq(u32 gic_irq)
{
	cond_return_error((gic_irq < NPU_NPU2ACPU_HW_EXP_IRQ_0) ||
		(gic_irq > NPU_NPU2ACPU_HW_EXP_IRQ_NS_3),
		-1, "gic_irq = %u is invalid \n", gic_irq);
	return (gic_irq >= NPU_NPU2ACPU_HW_EXP_IRQ_NS_0) ?
		gic_irq - NPU_NPU2ACPU_HW_EXP_IRQ_NS_0 :
		gic_irq - NPU_NPU2ACPU_HW_EXP_IRQ_0;
}

void npu_clr_hw_exp_irq_int(u64 reg_base, u32 gic_irq)
{
	u32 setval;
	int irq_offset = npu_parse_gic_irq(gic_irq);
	if (irq_offset == -1) {
		npu_drv_err("invalide gic_irq:%d\n", gic_irq);
		return;
	}
	if (reg_base == 0) {
		npu_drv_err("invalide hwirq reg base\n");
		return;
	}

	setval = npu_shift_left_val((u32)irq_offset);
	update_reg32(SOC_NPU_HW_EXP_IRQ_CLR_ADDR(reg_base), setval, setval);
}
