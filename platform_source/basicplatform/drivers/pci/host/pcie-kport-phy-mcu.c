/*
 * pcie-kport-phy-mcu.c
 *
 * kport PCIePHY Driver
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>

#include "pcie-kport-phy.h"
#include "pcie-kport-phy-mcu.h"

#ifdef XW_PHY_MCU
#include "pcie-kport-phy-core-image.h"
#endif

static inline void phy_core_write(struct pcie_phy *phy, uint32_t val, uint32_t reg)
{
	writel(val, phy->core_base + reg);
}

static inline uint32_t phy_core_read(struct pcie_phy *phy, uint32_t reg)
{
	return readl(phy->core_base + reg);
}

#ifdef XW_PHY_MCU
static void phy_core_init(struct pcie_phy *phy)
{
	/* clk for core sub-module */
	phy_core_write(phy, UCE_CRG_GT_BIT,
		PHY_CORE_SCTRL_OFFSET +  UCE_CRG_GT_EN);

	/* unrst core */
	phy_core_write(phy, UCE_CRG_RST_BIT,
		PHY_CORE_SCTRL_OFFSET +  UCE_CRG_RST_DIS);
}

static int32_t phy_image_load(struct pcie_phy *phy)
{
	uint32_t reg_addr;

	I("%s, image size: %d\n", __func__, phy_core_image_size);
	if (phy_core_image_size > CORE_RAM_SIZE) {
		E("Image Load Fail, Exceeded size\n");
		return -1;
	}

	for (reg_addr = 0; reg_addr < phy_core_image_size; reg_addr++)
		writel(phy_core_image[reg_addr],
			phy->core_base + reg_addr * REG_DWORD_ALIGN);

	return 0;
}

static inline void phy_set_core_pc(struct pcie_phy *phy)
{
	phy_core_write(phy, CORE_PC_START_ADDR,
		PHY_CORE_SCTRL_OFFSET +  UCE_MCU_POR_PC);
}

static void pcie_mcu_result_check(struct pcie_phy *phy, enum mcu_test_cmd cmd, u32 uce_ctx_addr)
{
	switch (cmd) {
	case MCU_RD_PHY_APB:
		E("%s: readback PHY_APB[0x%x]: 0x%x\n", __func__,
			phy->core_base + uce_ctx_addr,
			phy_core_read(phy, uce_ctx_addr));
		/* fall-through */
	case MCU_WR_PHY_APB:
		E("%s: check PHY_APB[0x%x]\n", __func__,
			PHYAPB_TEST_ADDR);
		return;
	case MCU_RD_PHY:
		E("%s: readback PCS_TOP[0x%x]: 0x%x\n", __func__,
			phy->core_base + uce_ctx_addr,
			phy_core_read(phy, uce_ctx_addr));
		/* fall-through */
	case MCU_WR_PHY:
		E("%s: check PCS_TOP[0x%x]: 0x%x\n", __func__,
			phy->phy_base + PCS_TOP_TEST_ADDR,
			readl(phy->phy_base + PCS_TOP_TEST_ADDR));
		return;
	default:
		return;
	}
}

static void pcie_mcu_set_cmd(struct pcie_phy *phy, enum mcu_test_cmd cmd,
				u32 ctx, u32 uce_cmd_addr, u32 uce_ctx_addr)
{
	u32 val;

	/* cmd */
	val = phy_core_read(phy, uce_cmd_addr);
	val &= (~UCE_PROG_CMD_MASK);
	val |= (cmd & UCE_PROG_CMD_MASK);
	phy_core_write(phy, val, uce_cmd_addr);

	/* ctx */
	phy_core_write(phy, ctx, uce_ctx_addr);

	D("%s: cmd[0x%x]: 0x%x\n", __func__,
		phy->core_base + uce_cmd_addr,
		phy_core_read(phy, uce_cmd_addr));
	D("%s: ctx[0x%x]: 0x%x\n", __func__,
		phy->core_base + uce_ctx_addr,
		phy_core_read(phy, uce_ctx_addr));
}

static void pcie_mcu_test(struct pcie_phy *phy, enum mcu_test_cmd cmd, u32 ctx)
{
	u32 val;
	int timecnt = 0;
	u32 uce_cmd_addr = PHY_CORE_SCTRL_OFFSET + UCE_PROG_CMD;
	u32 uce_ctx_addr = PHY_CORE_SCTRL_OFFSET + UCE_PROG_CTRL;

	D("+ %s +\n", __func__);

	if (cmd >= MCU_DONE_FLAG || cmd == MCU_NONE) {
		E("%s: CMD exec Error!\n", __func__);
		return;
	}

	D("%s: MCU_MASK[0x%x]: 0x%x\n", __func__,
		phy->phy_base + MCU_APB_INT_MASK,
		readl(phy->phy_base + MCU_APB_INT_MASK));

	pcie_mcu_set_cmd(phy, cmd, ctx, uce_cmd_addr, uce_ctx_addr);

	/* Trigger Interrupt: cpu to mcu */
	writel(0x1, phy->phy_base + PCS_IRQ_CPU_2_MCU);

	I("Waiting cmd finish...\n");
	while (1) {
		udelay(1);

		val = phy_core_read(phy, uce_cmd_addr);
		if ((val & UCE_PROG_CMD_MASK) == MCU_DONE_FLAG) {
			I("%s: cmd finished in %d us!\n", __func__, timecnt);
			break;
		} else if ((val & UCE_PROG_CMD_MASK) == MCU_NONE) {
			E("%s: cmd exec Error!\n", __func__);
			return;
		}

		if (timecnt++ > CMD_WAIT_TIMEOUT) {
			E("%s: cmd exec Timeout!\n", __func__);
			E("cmd[0x%x]: 0x%x\n", phy->core_base + uce_cmd_addr, val);
			break;
		}
	}

	/* clear irq */
	writel(0x0, phy->phy_base + PCS_IRQ_CPU_2_MCU);

	/* check mcu result */
	pcie_mcu_result_check(phy, cmd, uce_ctx_addr);
	D("- %s -\n", __func__);
}

static int32_t phy_core_utest(struct pcie_phy *phy)
{
	D("+ %s +\n", __func__);

	E("\n");
	E("MCU_WR_PHY_APB-->0xABCDDCBA:\n");
	pcie_mcu_test(phy, MCU_WR_PHY_APB, 0xABCDDCBA);
	E("\n");

	E("MCU_RD_PHY_APB:\n");
	writel(0x12345678, phy->apb_phy_base + PHYAPB_TEST_ADDR);
	pcie_mcu_test(phy, MCU_RD_PHY_APB, 0x0);
	E("\n");

	E("MCU_RD_PHY_APB-->0xCDEFFECD:\n");
	pcie_mcu_test(phy, MCU_WR_PHY_APB, 0xCDEFFECD);
	E("\n");

	E("MCU_RD_PHY_APB:\n");
	writel(0x87654321, phy->apb_phy_base + PHYAPB_TEST_ADDR);
	pcie_mcu_test(phy, MCU_RD_PHY_APB, 0x0);
	E("\n");

	E("MCU_WR_PHY-->0xABCDDCBA:\n");
	pcie_mcu_test(phy, MCU_WR_PHY, 0xABCDDCBA);
	E("\n");

	E("MCU_RD_PHY:\n");
	writel(0x12345678, phy->phy_base + PCS_TOP_TEST_ADDR);
	pcie_mcu_test(phy, MCU_RD_PHY, 0x0);
	E("\n");

	E("MCU_WR_PHY-->0xCDEFFECD:\n");
	pcie_mcu_test(phy, MCU_WR_PHY, 0xCDEFFECD);
	E("\n");

	E("MCU_RD_PHY:\n");
	writel(0x87654321, phy + PCS_TOP_TEST_ADDR);
	pcie_mcu_test(phy, MCU_RD_PHY, 0x0);
	E("\n");

	D("- %s -\n", __func__);

	return 0;
}

static int32_t phy_core_start(struct pcie_phy *phy)
{
	uint32_t val;

	I("+%s+\n", __func__);

	phy_core_init(phy);

	phy_image_load(phy);

	/* set pc */
	phy_set_core_pc(phy);

	/* exit halt */
	val = phy_core_read(phy, PHY_CORE_SCTRL_OFFSET + UCE_MCU_CTRL);
	val &= ~UCE_MCU_CORE_WAIT;
	phy_core_write(phy, val, PHY_CORE_SCTRL_OFFSET +  UCE_MCU_CTRL);

	phy_core_write(phy, 0x66666666, 0xA000 + 0x010);
	I("-%s-\n", __func__);

	return 0;
}

static int32_t phy_get_core_clk(struct pcie_phy *phy, struct device_node *np)
{
	phy->mcu_19p2_clk = of_clk_get_by_name(np, "pcie_mcu_19p2");
	if (IS_ERR(phy->mcu_19p2_clk)) {
		E("Failed to get pcie_mcu_19p2 clock\n");
		return PTR_ERR(phy->mcu_19p2_clk);
	}

	phy->mcu_clk = of_clk_get_by_name(np, "pcie_mcu");
	if (IS_ERR(phy->mcu_clk)) {
		E("Failed to get pcie_mcu clock\n");
		return PTR_ERR(phy->mcu_clk);
	}

	phy->mcu_bus_clk = of_clk_get_by_name(np, "pcie_mcu_bus");
	if (IS_ERR(phy->mcu_bus_clk)) {
		E("Failed to get pcie_mcu_bus clock\n");
		return PTR_ERR(phy->mcu_bus_clk);
	}

	phy->mcu_32k_clk = of_clk_get_by_name(np, "pcie_mcu_32k");
	if (IS_ERR(phy->mcu_32k_clk)) {
		E("Failed to get pcie_mcu_32k clock\n");
		return PTR_ERR(phy->mcu_32k_clk);
	}

	I("Successed to get all mcu clock\n");

	return 0;
}

static int phy_core_clk_config(struct pcie_phy *phy, bool enable)
{
	int ret;

	if (!enable) {
		clk_disable_unprepare(phy->mcu_clk);
		clk_disable_unprepare(phy->mcu_32k_clk);
		clk_disable_unprepare(phy->mcu_19p2_clk);
		clk_disable_unprepare(phy->mcu_bus_clk);
		return 0;
	}

	ret = clk_prepare_enable(phy->mcu_bus_clk);
	if (ret) {
		E("Failed to enable mcu_bus_clk");
		goto MCU_BUS_CLK;
	}

	ret = clk_prepare_enable(phy->mcu_19p2_clk);
	if (ret) {
		E("Failed to enable mcu_19p2_clk");
		goto MCU_19P2_CLK;
	}

	ret = clk_prepare_enable(phy->mcu_32k_clk);
	if (ret) {
		E("Failed to enable mcu_32k_clk");
		goto MCU_32K_CLK;
	}

	ret = clk_prepare_enable(phy->mcu_clk);
	if (ret) {
		E("Failed to enable mcu_clk");
		goto MCU_CLK;
	}

	return 0;
MCU_CLK:
	clk_disable_unprepare(phy->mcu_32k_clk);
MCU_32K_CLK:
	clk_disable_unprepare(phy->mcu_19p2_clk);
MCU_19P2_CLK:
	clk_disable_unprepare(phy->mcu_bus_clk);
MCU_BUS_CLK:
	return ret;
}

#define MCORE_TCM_PROP_SIZE 2
static int32_t phy_parse_tcm_info(struct pcie_phy *phy_info,
		struct device_node *np)
{
	uint64_t val[MCORE_TCM_PROP_SIZE] = { 0 };
	int32_t ret;
	resource_size_t addr;
	resource_size_t size;

	ret = of_property_read_u64_array(np, "mcore_tcm",
			val, MCORE_TCM_PROP_SIZE);
	if (ret) {
		E("get mcore_tcm property\n");
		return ret;
	}

	addr = (resource_size_t)val[0];
	size = (resource_size_t)val[1];

	phy_info->core_base = ioremap(addr, size);
	if (IS_ERR_OR_NULL(phy_info->core_base)) {
		E("mcore_tcm ioremap\n");
		return -EINVAL;
	}

	return 0;
}
#else

static inline int32_t phy_get_core_clk(struct pcie_phy *phy,
		struct device_node *np)
{
	return 0;
}

static inline int phy_core_clk_config(struct pcie_phy *phy, bool enable)
{
	return 0;
}

static inline int32_t phy_core_start(struct pcie_phy *phy)
{
	return 0;
}

static int32_t phy_core_utest(struct pcie_phy *phy)
{
	return 0;
}

static inline int32_t phy_parse_tcm_info(struct pcie_phy *phy_info,
		struct device_node *np)
{
	return 0;
}

#endif

struct pcie_mcu_ops phy_core_ops = {
	.phy_core_start = phy_core_start,
	.phy_core_utest = phy_core_utest,
	.phy_core_clk_config = phy_core_clk_config,
};

static int32_t phy_core_get_host_id(struct platform_device *pdev, u32 *rc_id)
{
	int32_t ret;

	ret = of_property_read_u32(pdev->dev.of_node, "rc-id", rc_id);
	if (ret) {
		E("Fail to read Host id\n");
		return ret;
	}

	return 0;
}

int32_t pcie_mcu_register(struct platform_device *pdev, struct pcie_phy *phy_info)
{
	struct device_node *phy_np = NULL;
	int32_t rc_id, ret;

	if (!phy_info)
		return 0;

	ret = phy_core_get_host_id(pdev, &rc_id);
	if (ret)
		return ret;

	phy_np = of_find_compatible_node(pdev->dev.of_node,
		NULL, "pcie,xw-phy");
	if (!phy_np) {
		E("Get PHY Node\n");
		return -ENODEV;
	}

	ret = phy_get_core_clk(phy_info, phy_np);
	if (ret)
		return ret;
	ret = phy_parse_tcm_info(phy_info, phy_np);
	if (ret)
		return ret;

	ret = pcie_mcu_ops_register(rc_id, &phy_core_ops);
	if (ret)
		goto CORE_TCM_BASE_ERR;

	return 0;
CORE_TCM_BASE_ERR:
#ifdef XW_PHY_MCU
	iounmap(phy_info->core_base);
#endif
	E("-%s-\n", __func__);
	return ret;
}