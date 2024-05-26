/*
 *
 * QIC Mntn Module.
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

#include <linux/module.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/syscore_ops.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <securec.h>
#include "dfx_qic.h"

#define PR_LOG_TAG QIC_TAG

#ifdef CONFIG_DFX_BB
#include <platform_include/basicplatform/linux/rdr_platform.h>
#endif

static void dfx_qic_dump_save(unsigned int *dump_addr, struct dfx_qic_clk *qic_clk_s)
{
	void __iomem *reg_base = NULL;
	unsigned int i, ret;

	reg_base = qic_clk_s->reg_base;
	for (i = 0; i < qic_clk_s->dump_reg_nums; i++) {
		ret = readl_relaxed((u8 __iomem *)reg_base + qic_clk_s->reg_offset[i]);
		pr_err("%s reg_offset[0x%x] = 0x%x\n", qic_clk_s->bus_name, qic_clk_s->reg_offset[i], ret);
		*dump_addr = ret;
		dump_addr++;
	}
	*dump_addr = DIVIDED_NUMBER;
}

static int dfx_qic_dump(void *dump_addr, unsigned int size)
{
	struct dfx_qic_clk **qic_clock_info = NULL;
	struct dfx_qic_clk *qic_clk_s = NULL;
	struct dfx_qic_device *qic_dev = NULL;
	unsigned int i, bus_num;
	unsigned int used_size = 0;

	pr_info("[%s]:addr=0x%pK,size=[0x%x]", __func__, dump_addr, size);

	qic_clock_info = get_qic_clk_info();
	qic_dev = get_qic_dev();
	if (!qic_clock_info || !qic_dev) {
		pr_err("qic_clk_s or qic_dev is null\n");
		return 0;
	}

	bus_num = qic_dev->bus_num;
	for (i = 0; i < bus_num; i++) {
		qic_clk_s = qic_clock_info[i];
		if (dfx_qic_check_crg_status(qic_clk_s)) {
			if ((used_size + qic_clk_s->dump_reg_nums) * sizeof(unsigned int) > size)
				break;
			/* vote power on */
			if (dfx_qic_vote_power_on(qic_clk_s)) {
				pr_info("[%s]used size = %u\n", __func__, used_size);
				dfx_qic_dump_save((unsigned int *)dump_addr + used_size, qic_clk_s);
				used_size += qic_clk_s->dump_reg_nums + 1;
				/* vote power down */
				dfx_qic_vote_power_down(qic_clk_s);
			} else {
				continue;
			}
		} else {
			pr_err("[%s] is power down\n", qic_clk_s->bus_name);
		}
	}

	return used_size;
}

int dfx_qic_dump_init(void)
{
	int ret = -1;

#ifdef CONFIG_DFX_BB
	ret = register_module_dump_mem_func(dfx_qic_dump, "QIC", MODU_QIC);
#endif

	return ret;
}

