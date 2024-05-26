/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include "dpu_fb.h"
#include "dpu_mmbuf_manager.h"
#include "dpu_fb_dts.h"

static uint32_t dpufd_irq_pdp;
static uint32_t dpufd_irq_sdp;
static uint32_t dpufd_irq_adp;

static uint32_t dpufd_irq_dp;

static uint32_t dpufd_irq_mdc;
static uint32_t dpufd_irq_dsi0;
static uint32_t dpufd_irq_dsi1;
static uint32_t dpufd_irq_dptx;

static uint32_t g_dss_base_phy;

static char __iomem *dpufd_dss_base;
static char __iomem *dpufd_peri_crg_base;
static char __iomem *dpufd_sctrl_base;
static char __iomem *dpufd_pctrl_base;
static char __iomem *dpufd_noc_dss_base;
static char __iomem *dpufd_mmbuf_crg_base;
static char __iomem *dpufd_mmbuf_asc0_base;
static char __iomem *dpufd_pmctrl_base;

static char __iomem *dpufd_media_crg_base;
static char __iomem *dpufd_media_common_base;
static char __iomem *dpufd_dp_base;
static char __iomem *dpufd_mmc0_crg_base;
static char __iomem *dpufd_dp_phy_base;

static const char *g_dss_axi_clk_name;
static const char *g_dss_pclk_dss_name;
static const char *g_dss_pri_clk_name;
static const char *g_dss_pxl0_clk_name;
static const char *g_dss_pxl1_clk_name;
static const char *g_dss_mmbuf_clk_name;
static const char *g_dss_pclk_mmbuf_clk_name;
static const char *g_dss_dphy0_ref_clk_name;
static const char *g_dss_dphy1_ref_clk_name;
static const char *g_dss_dphy0_cfg_clk_name;
static const char *g_dss_dphy1_cfg_clk_name;
static const char *g_dss_pclk_dsi0_name;
static const char *g_dss_pclk_dsi1_name;
static const char *g_dss_pclk_pctrl_name;
static const char *g_dss_clk_gate_dpctrl_16m_name;
static const char *g_dss_pclk_gate_dpctrl_name;
static const char *g_dss_aclk_dpctrl_name;

int dpu_fb_read_property_from_dts(struct device_node *np, struct device *dev)
{
	int ret;

	ret = of_property_read_u32(np, "fastboot_enable_flag", &g_fastboot_enable_flag);
	dpu_check_and_return(ret, -ENXIO, ERR, "failed to get fastboot_display_flag resource\n");
	dev_info(dev, "g_fastboot_enable_flag=%d\n", g_fastboot_enable_flag);

	ret = of_property_read_u32(np, "fake_lcd_flag", &g_fake_lcd_flag);
	dpu_check_and_return(ret, -ENXIO, ERR, "failed to get fake_lcd_flag resource\n");
	dev_info(dev, "g_fake_lcd_flag=%d\n", g_fake_lcd_flag);

	ret = of_property_read_u32(np, "dss_base_phy", &g_dss_base_phy);
	dpu_check_and_return(ret, -ENXIO, ERR, "failed to get dss_base_phy\n");
	dev_info(dev, "g_dss_base_phy=0x%x\n", g_dss_base_phy);

	ret = of_property_read_u32(np, "dss_version_tag", &g_dss_version_tag);
	if (ret)
		dev_err(dev, "failed to get g_dss_version_tag\n");
	dev_info(dev, "g_dss_version_tag=0x%x\n", g_dss_version_tag);

	ret = of_property_read_u32(np, "mmbuf_size_max", &mmbuf_max_size);
	dpu_check_and_return(ret, -ENXIO, ERR, "failed to get mmbuf_size_max\n");
	dev_info(dev, "mmbuf_size_max=0x%x\n", mmbuf_max_size);

	ret = of_property_read_u32(np, "mmbuf_mdc_reserved_size", &dss_mmbuf_reserved_info[SERVICE_MDC].size);
	if (ret)
		dev_err(dev, "failed to get mmbuf_mdc_reserved_size\n");

	dev_info(dev, "mmbuf_mdc_reserved_size=0x%x\n", dss_mmbuf_reserved_info[SERVICE_MDC].size);

	ret = of_property_read_u32(np, "mipi_dphy_version", &g_mipi_dphy_version);
	if (ret) {
		g_mipi_dphy_version = 0;
		dev_info(dev, "mipi_dphy_version=0x%x\n", g_mipi_dphy_version);
	}

	ret = of_property_read_u32(np, "chip_id", &g_chip_id);
	if (ret)
		g_chip_id = 0;
	dev_info(dev, "g_chip_id=0x%x\n", g_chip_id);

	return 0;
}

int dpu_fb_get_irq_no_from_dts(struct device_node *np, struct device *dev)
{
	dpufd_irq_pdp = irq_of_parse_and_map(np, 0);
	dev_check_and_return(dev, !dpufd_irq_pdp, -ENXIO, err, "failed to get dpufd_irq_pdp resource\n");

	dpufd_irq_sdp = irq_of_parse_and_map(np, 1);
	dev_check_and_return(dev, !dpufd_irq_sdp, -ENXIO, err, "failed to get dpufd_irq_sdp resource\n");

	dpufd_irq_adp = irq_of_parse_and_map(np, 2);
	dev_check_and_return(dev, !dpufd_irq_adp, -ENXIO, err, "failed to get dpufd_irq_adp resource\n");

	dpufd_irq_dsi0 = irq_of_parse_and_map(np, 3);
	dev_check_and_return(dev, !dpufd_irq_dsi0, -ENXIO, err, "failed to get dpufd_irq_dsi0 resource\n");

	dpufd_irq_dsi1 = irq_of_parse_and_map(np, 4);
	dev_check_and_return(dev, !dpufd_irq_dsi1, -ENXIO, err, "failed to get dpufd_irq_dsi1 resource\n");

	dpufd_irq_dptx = irq_of_parse_and_map(np, 5);
	dev_check_and_return(dev, !dpufd_irq_dptx, -ENXIO, err, "failed to get dpufd_irq_dptx resource\n");

	dpufd_irq_mdc = irq_of_parse_and_map(np, 6);
	dev_check_and_return(dev, !dpufd_irq_mdc, -ENXIO, err, "failed to get dpufd_irq_mdc resource\n");

	dpufd_irq_dp = irq_of_parse_and_map(np, 7);
	dev_check_and_return(dev, !dpufd_irq_dp, -ENXIO, err, "failed to get dpufd_irq_dp resource\n");

	return 0;
}

int dpu_fb_get_baseaddr_from_dts(struct device_node *np, struct device *dev)
{
	dpufd_dss_base = of_iomap(np, 0);
	dev_check_and_return(dev, !dpufd_dss_base, -ENXIO, err, "failed to get dpufd_dss_base resource\n");

	dpufd_peri_crg_base = of_iomap(np, 1);
	dev_check_and_return(dev, !dpufd_peri_crg_base, -ENXIO, err, "failed to get dpufd_peri_crg_base resource\n");

	dpufd_sctrl_base = of_iomap(np, 2);
	dev_check_and_return(dev, !dpufd_sctrl_base, -ENXIO, err, "failed to get dpufd_sctrl_base resource\n");

	dpufd_pctrl_base = of_iomap(np, 3);
	dev_check_and_return(dev, !dpufd_pctrl_base, -ENXIO, err, "failed to get dpufd_pctrl_base resource\n");

	dpufd_noc_dss_base = of_iomap(np, 4);
	dev_check_and_return(dev, !dpufd_noc_dss_base, -ENXIO, err, "failed to get dpufd_noc_dss_base resource\n");

	dpufd_mmbuf_crg_base = of_iomap(np, 5);
	dev_check_and_return(dev, !dpufd_mmbuf_crg_base, -ENXIO, err,
		"failed to get dpufd_mmbuf_crg_base resource\n");

	dpufd_pmctrl_base = of_iomap(np, 6);
	dev_check_and_return(dev, !dpufd_pmctrl_base, -ENXIO, err, "failed to get dpufd_pmctrl_base resource\n");

	dpufd_media_crg_base = of_iomap(np, 7);
	dev_check_and_return(dev, !dpufd_media_crg_base, -ENXIO, err,
		"failed to get dpufd_media_crg_base resource\n");

	dpufd_dp_base = of_iomap(np, 9);
	dev_check_and_return(dev, !dpufd_dp_base, -ENXIO, err, "failed to get dpufd_dp_base resource\n");

	dpufd_media_common_base = of_iomap(np, 10);
	dev_check_and_return(dev, !dpufd_media_common_base, -ENXIO, err,
		"failed to get dpufd_media_common_base resource\n");

	dpufd_mmc0_crg_base = of_iomap(np, 11);
	dev_check_and_return(dev, !dpufd_mmc0_crg_base, -ENXIO, err, "failed to get dpufd_mmc0_crg_base resource\n");

	dpufd_dp_phy_base = of_iomap(np, 12);
	dev_check_and_return(dev, !dpufd_dp_phy_base, -ENXIO, err, "failed to get dpufd_dp_phy_base resource\n");

	return 0;
}

int dpu_fb_get_clk_name_from_dts(struct device_node *np, struct device *dev)
{
	int ret;

	ret = of_property_read_string_index(np, "clock-names", 0, &g_dss_axi_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get axi_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 1, &g_dss_pclk_dss_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get pclk_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 2, &g_dss_pri_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get pri_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 4, &g_dss_pxl1_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get pxl1_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 5, &g_dss_mmbuf_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get mmbuf_clk resourc! ret = %d n", ret);

	ret = of_property_read_string_index(np, "clock-names", 6, &g_dss_pclk_mmbuf_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get pclk_mmbuf_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 7, &g_dss_dphy0_ref_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dphy0_ref_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 8, &g_dss_dphy1_ref_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dphy1_ref_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 9, &g_dss_dphy0_cfg_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dphy0_cfg_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 10, &g_dss_dphy1_cfg_clk_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dphy1_cfg_clk resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 11, &g_dss_pclk_dsi0_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dss_pclk_dsi0 resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 12, &g_dss_pclk_dsi1_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dss_pclk_dsi1 resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 13, &g_dss_pclk_pctrl_name);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dss_pclk_pctrl resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 14, &g_dss_clk_gate_dpctrl_16m_name);  /* aux */
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dss_clk_gate_dpctrl_16m resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 15, &g_dss_pclk_gate_dpctrl_name);  /* pclk */
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dss_pclk_gate_dpctrl resource! ret = %d\n", ret);

	ret = of_property_read_string_index(np, "clock-names", 16, &g_dss_aclk_dpctrl_name);  /* ack */
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get dss_aclk_gate_dpctrl resource! ret = %d\n", ret);

	return ret;
}

static int dpu_of_get_iova_info(struct device_node *np, const char *name,
	struct dpu_iova_info *iova_info)
{
	struct device_node *node = NULL;
	int ret = 0;

	node = of_get_child_by_name(np, name);
	if (node == NULL) {
		DPU_FB_ERR("find iommu_info node error\n");
		return -ENODEV;
	}
	ret = of_property_read_u64(node, "start-addr", &iova_info->iova_start);
	if (ret != 0) {
		DPU_FB_ERR("read iova start address error\n");
		return -EINVAL;
	}
	ret = of_property_read_u64(node, "size", &iova_info->iova_size);
	if (ret != 0) {
		DPU_FB_ERR("read iova size error\n");
		return -EINVAL;
	}
	ret = of_property_read_u64(node, "iova-align", &iova_info->iova_align);
	if (ret != 0)
		iova_info->iova_align = SZ_256K;

	DPU_FB_DEBUG("%s: start_addr 0x%lx, size 0x%lx, align 0x%lx\n",
		name, iova_info->iova_start, iova_info->iova_size, iova_info->iova_align);

	return 0;
}

int dpu_fb_get_iommu_info_from_dts(struct device *dev)
{
	struct device_node *node = NULL;
	int ret;

	node = of_find_node_by_name(NULL, "smmu_dss");
	if (node == NULL) {
		DPU_FB_ERR("find iommu info node error\n");
		return -ENODEV;
	}

	ret = dpu_of_get_iova_info(node, "iova_info", &g_dpu_iova_info);
	dev_check_and_return(dev, ret, -ENXIO, err, "get iova info fail! ret = %d\n", ret);

	ret = dpu_of_get_iova_info(node, "vm_iova_info", &g_dpu_vm_iova_info);
	dev_check_and_return(dev, ret, -ENXIO, err, "get vm iova info fail! ret = %d\n", ret);
	return 0;
}

void dpu_fb_read_logo_buffer_from_dts(struct device_node *np, struct device *dev)
{
	int ret;

	ret = of_property_read_u32_index(np, "reg", 1, &g_logo_buffer_base);
	if (ret != 0) {
		dev_err(dev, "failed to get g_logo_buffer_base resource! ret=%d\n", ret);
		g_logo_buffer_base = 0;
	}
	ret = of_property_read_u32_index(np, "reg", 3, &g_logo_buffer_size);
	if (ret != 0) {
		dev_err(dev, "failed to get g_logo_buffer_size resource! ret=%d\n", ret);
		g_logo_buffer_size = 0;
	}

	dev_info(dev, "g_logo_buffer_base = 0x%x, g_logo_buffer_size = 0x%x\n", g_logo_buffer_base, g_logo_buffer_size);
}

int dpufb_init_irq(struct dpu_fb_data_type *dpufd, uint32_t type)
{
	dpufd->dsi0_irq = dpufd_irq_dsi0;
	dpufd->dsi1_irq = dpufd_irq_dsi1;
	dpufd->dp_irq = dpufd_irq_dptx;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dpufd->fb_num = DPU_FB0_NUM;
		dpufd->dpe_irq = dpufd_irq_pdp;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		dpufd->fb_num = DPU_FB1_NUM;

		if (type & PANEL_DP)
			dpufd->dpe_irq = dpufd_irq_dp;
		else
			dpufd->dpe_irq = dpufd_irq_sdp;
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		dpufd->fb_num = DPU_FB2_NUM;
		dpufd->dpe_irq = dpufd_irq_adp;
	} else if (dpufd->index == MEDIACOMMON_PANEL_IDX) {
		dpufd->fb_num = DPU_FB3_NUM;
		dpufd->dpe_irq = dpufd_irq_mdc;
	} else {
		DPU_FB_ERR("fb%d not support now!\n", dpufd->index);
		return -EINVAL;
	}

	return 0;
}

void dpufb_init_base_addr(struct dpu_fb_data_type *dpufd)
{
	dpufd->dss_base = dpufd_dss_base;
	dpufd->peri_crg_base = dpufd_peri_crg_base;
	dpufd->sctrl_base = dpufd_sctrl_base;
	dpufd->pctrl_base = dpufd_pctrl_base;
	dpufd->noc_dss_base = dpufd_noc_dss_base;
	dpufd->mmbuf_crg_base = dpufd_mmbuf_crg_base;
	dpufd->mmbuf_asc0_base = dpufd_mmbuf_asc0_base;
	dpufd->pmctrl_base = dpufd_pmctrl_base;
	dpufd->media_crg_base = dpufd_media_crg_base;
	dpufd->media_common_base = dpufd_media_common_base;
	dpufd->dp_base = dpufd_dp_base;
	dpufd->mmc0_crg = dpufd_mmc0_crg_base;
	dpufd->dp_phy_base = dpufd_dp_phy_base;

	dpufd->mipi_dsi0_base = dpufd->dss_base + DSS_MIPI_DSI0_OFFSET;
	dpufd->mipi_dsi1_base = dpufd->dss_base + DSS_MIPI_DSI1_OFFSET;
	dpufd->dss_base_phy = g_dss_base_phy;
}

void dpufb_init_clk_name(struct dpu_fb_data_type *dpufd)
{
	dpufd->dss_axi_clk_name = g_dss_axi_clk_name; /* only enable */
	dpufd->dss_pclk_dss_name = g_dss_pclk_dss_name; /* only enable */
	dpufd->dss_pri_clk_name = g_dss_pri_clk_name;
	dpufd->dss_pxl0_clk_name = g_dss_pxl0_clk_name;
	dpufd->dss_pxl1_clk_name = g_dss_pxl1_clk_name;
	dpufd->dss_mmbuf_clk_name = g_dss_mmbuf_clk_name;
	dpufd->dss_pclk_mmbuf_name = g_dss_pclk_mmbuf_clk_name;
	dpufd->dss_dphy0_ref_clk_name = g_dss_dphy0_ref_clk_name;
	dpufd->dss_dphy1_ref_clk_name = g_dss_dphy1_ref_clk_name;
	dpufd->dss_dphy0_cfg_clk_name = g_dss_dphy0_cfg_clk_name;
	dpufd->dss_dphy1_cfg_clk_name = g_dss_dphy1_cfg_clk_name;
	dpufd->dss_pclk_dsi0_name = g_dss_pclk_dsi0_name;
	dpufd->dss_pclk_dsi1_name = g_dss_pclk_dsi1_name;
	dpufd->dss_pclk_pctrl_name = g_dss_pclk_pctrl_name;
	dpufd->dss_auxclk_dpctrl_name = g_dss_clk_gate_dpctrl_16m_name;
	dpufd->dss_pclk_dpctrl_name = g_dss_pclk_gate_dpctrl_name;
	dpufd->dss_aclk_dpctrl_name = g_dss_aclk_dpctrl_name;
}
