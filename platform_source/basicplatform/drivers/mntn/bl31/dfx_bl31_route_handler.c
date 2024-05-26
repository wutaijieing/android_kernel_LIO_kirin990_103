/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * bl31 route to kernel with spi functions moudle
 *
 * dfx_bl31_route_handler.c
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

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/irqreturn.h>
#include <linux/thread_info.h>
#include <linux/module.h>
#include <global_ddr_map.h>
#include "rdr_print.h"
#include "dfx_bl31_route_handler.h"

#define SPI_MNTN_INFORM   378
#define DMSS_FLAG_OFFSET   24
#define DMSS_ERROR      1
#define DMSS_WARNING    0
#define BL31_SPI_MNTN_NAME "hisilicon,bl31_spi_mntn"

static void *bl31_mntn_shmem_addr = NULL;
static uint32_t bl31_mntn_shmem_size = 0;

static int bl31_mntn_shmem_map(void)
{
	struct device_node *np = NULL;
	uint32_t data[2];
	int ret;
	void *bl31_mntn_addr = NULL;

	np = of_find_compatible_node(NULL, NULL, "hisilicon, bl31_mntn");
	if (unlikely(!np)) {
		BB_PRINT_ERR("%s fail: no compatible node found\n", __func__);
		return -1;
	}

	ret = of_property_read_u32_array(np, "dfx,bl31-share-mem", &data[0], 2UL);
	if (unlikely(ret)) {
		BB_PRINT_ERR("%s fail: get val mem compatible node err\n", __func__);
		return -1;
	}

	bl31_mntn_addr = ioremap(ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE + data[0], (u64) data[1]);
	if (unlikely(bl31_mntn_addr == NULL)) {
		BB_PRINT_ERR("%s fail: allocate memory for bl31_mntn_addr failed\n", __func__);
		return -1;
	}

	bl31_mntn_shmem_addr = bl31_mntn_addr;
	bl31_mntn_shmem_size = data[1];
	return 0;
}

/*
 * In virtualization scenarios, BL31 uses SPI instead of FIQ and IPI to notify the kernel.
 */
static irqreturn_t bl31_irq_handle(int irq, void *ptr)
{
	u64 dmss_exception_flag;
	struct pt_regs *dmss_el_sp = NULL;

	BB_PRINT_START();
	if (!bl31_mntn_shmem_addr || bl31_mntn_shmem_size == 0) {
		BB_PRINT_ERR("%s: bl31_mntn addr or size is error!", __func__);
		return IRQ_NONE;
	}

	dmss_exception_flag = readq(bl31_mntn_shmem_addr + DMSS_FLAG_OFFSET);
	BB_PRINT_PN("%s: dmss_exception_flag = %lld\n",__func__, dmss_exception_flag);
	if (dmss_exception_flag == DMSS_ERROR) {
		dmss_el_sp =  get_irq_regs();
		fiq_dump(dmss_el_sp, 0);
	} else {
		dfx_mntn_inform();
	}

	BB_PRINT_END();
	return IRQ_HANDLED;
}

static unsigned int get_bl31_spi_irq_num(void)
{
	u32 irq_num;
	struct device_node *dev_node = NULL;

	dev_node = of_find_compatible_node(NULL, NULL, BL31_SPI_MNTN_NAME);
	if (!dev_node) {
		BB_PRINT_ERR("%s: find device node %s by compatible failed\n", __func__, BL31_SPI_MNTN_NAME);
		return 0;
	}

	irq_num = irq_of_parse_and_map(dev_node, 0);
	if (irq_num == 0) {
		BB_PRINT_ERR("%s: irq parse and map failed, irq: %u\n", __func__, irq_num);
		return 0;
	}

	return irq_num;
}

static void bl31_irq_register(void)
{
	int ret;
	unsigned int irq;

	irq = get_bl31_spi_irq_num();
	if (irq == 0) {
		BB_PRINT_ERR("%s: get irq failed\n", __func__);
		return;
	}

	ret = request_irq(irq, bl31_irq_handle, IRQF_TRIGGER_HIGH, "bl31_mntn", NULL);
	if (ret)
		BB_PRINT_ERR("%s: request irq %d return %d\n", __func__, irq, ret);
	return;
}

static int __init bl31_mntn_init(void)
{
	int ret;

	ret = bl31_mntn_shmem_map();
	if (ret) {
		BB_PRINT_ERR("%s: bl31_mntn share memory map failed, return %d\n", __func__, ret);
		return -1;
	}

	bl31_irq_register();

	return 0;
}

module_init(bl31_mntn_init);
MODULE_LICENSE("GPL");
