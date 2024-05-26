/*
 * npu_sdma_plat.c
 *
 * about npu sdma plat
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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
#include "npu_sdma_plat.h"
#include <linux/io.h>

u64 npu_sdma_get_base_addr(void)
{
	u64 sdma_base;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, 0ULL, "get platform info failed\n");
	sdma_base = (u64)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_SDMA_BASE];
	npu_drv_debug("sdma_base = 0x%pK\n", sdma_base);
	return sdma_base;
}

void npu_sdma_query_channel_info(u8 sdma_channel)
{
	u32 value_l;
	u32 value_h;
	u32 value;
	u32 sid, wp, rp;
	u64 base = npu_sdma_get_base_addr();
	cond_return_void(base == 0ULL, "sdma base is NULL\n");

	npu_drv_warn("sdma_channel = 0x%02x\n", sdma_channel);

	value = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_CH_CTRL_ADDR(base, sdma_channel));
	npu_drv_warn("CH_CTRL = 0x%08x\n", value);
	value = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_STATUS_ADDR(base, sdma_channel));
	npu_drv_warn("STATUS = 0x%08x\n", value);

	sid = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_SQ_SID_ADDR(base, sdma_channel));
	wp = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_SQ_WP_ADDR(base, sdma_channel));
	rp = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_SQ_RP_ADDR(base, sdma_channel));
	npu_drv_warn("SQ_WP = 0x%08x, SQ_RP = 0x%08x\n",
		wp, rp);

	sid = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_CQ_SID_ADDR(base, sdma_channel));
	wp = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_CQ_WP_ADDR(base, sdma_channel));
	rp = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_CQ_RPL_ADDR(base, sdma_channel));
	npu_drv_warn("CQ_WP = 0x%08x, CQ_RPL = 0x%08x\n",
		wp, rp);

	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_SQ_BASE_H_ADDR(base, sdma_channel));
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_SQ_BASE_L_ADDR(base, sdma_channel));
	npu_drv_warn("SQ_BASE_L = 0x%08x, VAL_H = 0x%08x\n", value_l, value_h);

	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_CQ_BASE_H_ADDR(base, sdma_channel));
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_CQ_BASE_L_ADDR(base, sdma_channel));
	npu_drv_warn("CQ_BASE_L = 0x%08x, VAL_H = 0x%08x\n", value_l, value_h);

	return;
}

void npu_sdma_query_interrupt_info(void)
{
	u32 value_l;
	u32 value_h;
	u64 base = npu_sdma_get_base_addr();
	cond_return_void(base == 0ULL, "sdma base is NULL\n");

	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INT0_MASK_VAL_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INT0_MASK_VAL_H_ADDR(base));
	npu_drv_warn("INT0_MASK_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_MASK_INT0_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_MASK_INT0_H_ADDR(base));
	npu_drv_warn("MASK_INT0_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_RAW_INT0_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_RAW_INT0_H_ADDR(base));
	npu_drv_warn("RAW_INT0_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INT1_MASK_VAL_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INT1_MASK_VAL_H_ADDR(base));
	npu_drv_warn("INT1_MASK_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_MASK_INT1_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_MASK_INT1_H_ADDR(base));
	npu_drv_warn("MASK_INT1_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_RAW_INT1_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_RAW_INT1_H_ADDR(base));
	npu_drv_warn("RAW_INT1_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INT2_MASK_VAL_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INT2_MASK_VAL_H_ADDR(base));
	npu_drv_warn("INT2_MASK_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_MASK_INT2_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_MASK_INT2_H_ADDR(base));
	npu_drv_warn("MASK_INT2_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_RAW_INT2_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_RAW_INT2_H_ADDR(base));
	npu_drv_warn("RAW_INT2_VAL_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);

	return;
}

void npu_sdma_query_global_info(void)
{
	u32 value_l;
	u32 value_h;
	u32 value[10];
	u64 base = npu_sdma_get_base_addr();
	cond_return_void(base == 0ULL, "sdma base is NULL\n");

	value_l = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INNER_MADDR_BASE_L_ADDR(base));
	value_h = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INNER_MADDR_BASE_H_ADDR(base));
	npu_drv_warn("INNER_MADDR_BASE_L = 0x%08x, VAL_H = 0x%08x\n",
		value_l, value_h);
	value[0] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_INNER_MEM_SIZE_ADDR(base));
	npu_drv_warn("INNER_MEM_SIZE = 0x%08x\n", value[0]);
	value[0] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_DMA_TIMEOUT_TH_ADDR(base));
	npu_drv_warn("DMA_TIMEOUT_TH = 0x%08x\n", value[0]);
	value[0] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_DMA_INER_FSM_ADDR(base));
	npu_drv_warn("DMA_INER_FSM = 0x%08x\n", value[0]);

	value[0] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_CTRL0_ADDR(base));
	value[1] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_CTRL1_ADDR(base));
	value[2] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_CTRL2_ADDR(base));
	value[3] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_CTRL3_ADDR(base));
	value[4] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_CTRL4_ADDR(base));
	value[5] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_CTRL5_ADDR(base));
	npu_drv_warn("BIU_CTRL[0~5] = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		value[0], value[1], value[2],
		value[3], value[4], value[5]);

	value[0] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS0_ADDR(base));
	value[1] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS1_ADDR(base));
	value[2] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS2_ADDR(base));
	value[3] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS3_ADDR(base));
	value[4] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS4_ADDR(base));
	value[5] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS5_ADDR(base));
	value[6] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS6_ADDR(base));
	value[7] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS7_ADDR(base));
	value[8] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS8_ADDR(base));
	value[9] = readl(
		(const volatile void *)(uintptr_t)SOC_NPU_sysDMA_BIU_STATUS9_ADDR(base));
	npu_drv_warn("BIU_STATUS[0~9] = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x"
		" 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		value[0], value[1], value[2], value[3], value[4],
		value[5], value[6], value[7], value[8], value[9]);

	return;
}

int npu_sdma_query_exception_info(
	u8 sdma_channel, struct sdma_exception_info *sdma_info)
{
	(void)sdma_info;

	npu_sdma_query_global_info();
	npu_sdma_query_interrupt_info();
	npu_sdma_query_channel_info(sdma_channel);

	return 0;
}