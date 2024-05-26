/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * jpgdec irq
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

#include "jpgdec_irq.h"
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include "jpu_def.h"
#include "jpgdec_platform.h"

irqreturn_t jpgdec_irq_isr(int irq, void *ptr)
{
	struct jpu_data_type *jpu_device = NULL;
	uint32_t reg_val;
	uint32_t isr_state;

	jpu_check_null_return(ptr, IRQ_HANDLED);
	jpu_device = (struct jpu_data_type *)ptr;
	jpu_check_null_return(jpu_device->jpu_top_base, IRQ_HANDLED);

	isr_state = inp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG2);
	if (isr_state & BIT(DEC_DONE_ISR_BIT)) { /* use 16bit to decide */
		jpu_info("done isr occured\n");
		reg_val = inp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0);
		reg_val |= BIT(0);
		outp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0, reg_val);
		jpu_device->jpu_dec_done_flag = 1;
		wake_up_interruptible_all(&jpu_device->jpu_dec_done_wq);
	}

	if (isr_state & BIT(DEC_ERR_ISR_BIT)) {  /* use 17bit to decide */
		jpu_info("err isr occured\n");
		reg_val = inp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0);
		reg_val |= BIT(1);
		outp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0, reg_val);
		jpu_dec_error_reset(jpu_device);
	}

	if (isr_state & BIT(DEC_OVERTIME_ISR_BIT)) { /* use 18bit to decide */
		jpu_info("overtime isr occured\n");
		reg_val = inp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0);
		reg_val |= BIT(2); /* 2 is reg assign 1 to second */
		outp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0, reg_val);
		jpu_dec_error_reset(jpu_device);
	}

	return IRQ_HANDLED;
}

int jpgdec_get_irq(struct jpu_data_type *jpu_device, struct device_node *np)
{
	int index;

	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_check_null_return(np, -EINVAL);

	for (index = 0; index < JPGDEC_IRQ_NUM; index++) {
		jpu_device->jpu_irq_num[index] = irq_of_parse_and_map(np, index);
		if (jpu_device->jpu_irq_num[index] == 0) {
			jpu_err("failed to get jpgdec irq resource\n");
			return -ENXIO;
		}
	}

	return 0;
}

int jpgdec_request_irq(struct jpu_data_type *jpu_device)
{
	int index;
	int ret = 0;

	jpu_check_null_return(jpu_device, -EINVAL);

	for (index = 0; index < JPGDEC_IRQ_NUM; index++) {
		ret = request_irq(jpu_device->jpu_irq_num[index], jpgdec_irq_isr, 0,
		"jpgdec_irq", (void *)jpu_device);
		if (ret != 0) {
			jpu_err("request irq failed, irq_num = %d error = %d\n",
				jpu_device->jpu_irq_num[index], ret);
			return ret;
		}
		disable_irq(jpu_device->jpu_irq_num[index]);
	}

	return ret;
}

int jpgdec_enable_irq(struct jpu_data_type *jpu_device)
{
	int index;
	jpu_check_null_return(jpu_device, -EINVAL);

	for (index = 0; index < JPGDEC_IRQ_NUM; index++) {
		if (jpu_device->jpu_irq_num[index] == 0)
			return -EINVAL;
		enable_irq(jpu_device->jpu_irq_num[index]);
	}

	return 0;
}

void jpgdec_disable_irq(struct jpu_data_type *jpu_device)
{
	int index;
	if (jpu_device == NULL) {
		jpu_err("jpu_device is nullptr\n");
		return;
	}

	for (index = 0; index < JPGDEC_IRQ_NUM; index++) {
		if (jpu_device->jpu_irq_num[index] != 0)
			disable_irq(jpu_device->jpu_irq_num[index]);
	}
}

void jpgdec_free_irq(struct jpu_data_type *jpu_device)
{
	int index;
	if (jpu_device == NULL) {
		jpu_err("jpu_device is nullptr\n");
		return;
	}

	for (index = 0; index < JPGDEC_IRQ_NUM; index++) {
		if (jpu_device->jpu_irq_num[index] != 0) {
			free_irq(jpu_device->jpu_irq_num[index], 0);
			jpu_device->jpu_irq_num[index] = 0;
		}
	}
}