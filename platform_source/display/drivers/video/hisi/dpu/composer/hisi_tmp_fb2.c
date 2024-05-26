/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/types.h>

#include "hisi_fb.h"
#include "hisi_disp_composer.h"
#include "hisi_composer_core.h"
#include "hisi_connector_utils.h"
#include "hisi_disp_gadgets.h"
#include "hisi_disp_pm.h"
#include "hisi_operators_manager.h"
//#include "isr/hisi_isr_external.h"
#include "isr/hisi_isr_primary.h"
#include "hisi_comp_utils.h"
#include "hisi_disp_pm.h"
#include "hisi_disp_config.h"
#include "hisi_disp_rm.h"
#include <asm/traps.h>
#include "vsync/hisi_disp_vactive.h"
#include "isr/hisi_isr_utils.h"

#define HISI_CORE_CLK_DEFAULT_RATE (20 * DISP_1M)

static struct hisi_composer_device *g_hisi_fb2_ov_data;

static void hisi_fb2_init_default_clk(struct hisi_composer_data *ov_data)
{
	struct hisi_disp_pm_clk *core_clk = NULL;

	disp_pr_info(" +++++++ \n");
	core_clk = ov_data->clks[PM_CLK_CORE];
	if (core_clk)
		core_clk->default_clk_rate = HISI_CORE_CLK_DEFAULT_RATE;

	/* TODO: other clk */
}

static int hisi_drv_tmp_fb2_probe(struct platform_device *pdev)
{
	struct hisi_composer_device *ov_dev = NULL;
	const uint32_t *irq_signals;
	uint32_t post_irq_unmask[IRQ_UNMASK_MAX] = {0};
	int ret;
	uint32_t irq_mask;

	disp_pr_info("enter +++++++, pdev=%p \n", pdev);

	ov_dev = platform_get_drvdata(pdev);
	if (!ov_dev)
		return -1;

	/* 1, init power manager such regulator and clk */
	hisi_disp_pm_init(hisi_disp_config_get_regulators(), PM_REGULATOR_MAX, hisi_disp_config_get_clks(), PM_CLK_MAX);
	hisi_fb2_init_default_clk(&ov_dev->ov_data);

	/* 2, init isr
	 * FIXME: registe connector irq to composer
	 */
#if 0
	irq_signals = hisi_disp_config_get_irqs();
	hisi_primary_isr_init(&ov_dev->ov_data.isr_ctrl[COMP_ISR_PRE], irq_signals[DISP_IRQ_SIGNAL_NS_SDP], NULL, 0, NULL, ov_dev);

	/* init dsi0 irq */
	memset(post_irq_unmask, 0, sizeof(post_irq_unmask));
	post_irq_unmask[IRQ_UNMASK_GLB] = hisi_disp_isr_get_vsync_bit(ov_dev->ov_data.fix_panel_info->type);
	post_irq_unmask[IRQ_UNMASK_GLB] = DSI_INT_UNDER_FLOW | DSI_INT_VACT0_START | DSI_INT_FRM_END;
	hisi_primary_isr_init(&ov_dev->ov_data.isr_ctrl[COMP_ISR_POST], irq_signals[DISP_IRQ_SIGNAL_DSI0],
			post_irq_unmask, IRQ_UNMASK_MAX, hisi_config_get_irq_name(DISP_IRQ_SIGNAL_DSI0), ov_dev);
	irq_mask = DSI_INT_VACT0_START;
	hisi_disp_isr_register_listener(&ov_dev->ov_data.isr_ctrl[COMP_ISR_POST], get_vactive0_isr_notifier(), IRQ_UNMASK_GLB, irq_mask);
#else
	irq_signals = hisi_disp_config_get_irqs();
	hisi_primary_isr_init(&ov_dev->ov_data.isr_ctrl[COMP_ISR_PRE], irq_signals[DISP_IRQ_SIGNAL_NS_SDP], NULL, 0, NULL, ov_dev);
	hisi_primary_isr_init(&ov_dev->ov_data.isr_ctrl[COMP_ISR_POST], irq_signals[DISP_IRQ_SIGNAL_DPTX0], NULL, 0, NULL, ov_dev);
#endif
	/* 3, init ip base addr */
	hisi_config_init_ip_base_addr(ov_dev->ov_data.ip_base_addrs, DISP_IP_MAX);

	/* 4, FIXME: init timeline */
	//hisi_disp_timline_init(ov_dev, &ov_dev->timeline, "external_timeline");
	INIT_LIST_HEAD(&ov_dev->pre_layer_buf_list);
	INIT_LIST_HEAD(&ov_dev->post_wb_buf_list);
	/* init private ops */
	hisi_online_composer_register_private_ops(&ov_dev->ov_priv_ops);

	hisi_vactive0_init();
	/* create fb platform device */
	hisi_fb_create_platform_device(HISI_PRIMARY_FB_DEV_NAME, ov_dev, hisi_composer_get_public_ops());
	disp_pr_info(" ---- \n");
	return 0;
}

static void hisi_tmp_fb2_init_data(struct platform_device *pdev, void *dev_data, void *input_data, void *input_ops)
{
	struct hisi_composer_device *overlay_data = NULL;
	struct hisi_connector_device *connector_data = NULL;

	disp_pr_info("enter +++++++\n");

	overlay_data = (struct hisi_composer_device *)dev_data;
	connector_data = (struct hisi_connector_device *)input_data;
	connector_data->master.connector_id = 2;//FIX

	overlay_data->pdev = pdev;
	overlay_data->next_dev_count = 0;

	overlay_data->ov_data.fix_panel_info = connector_data->fix_panel_info;
	overlay_data->ov_data.ov_id = ONLINE_OVERLAY_ID_EXTERNAL2;
	overlay_data->ov_data.scene_id = DEBUG_ONLINE_SCENE_ID; // debug

	overlay_data->ov_data.clk_bits = BIT(PM_CLK_ACLK_GATE) | BIT(PM_CLK_CORE) | BIT(PM_CLK_PCKL_GATE);
	hisi_disp_pm_get_clks(overlay_data->ov_data.clk_bits, overlay_data->ov_data.clks, PM_CLK_MAX);

	overlay_data->ov_data.regulator_bits = BIT(PM_REGULATOR_SMMU) | BIT(PM_REGULATOR_DPU) | BIT(PM_REGULATOR_VIVOBUS) | BIT(PM_REGULATOR_MEDIA1);
	hisi_disp_pm_get_regulators(overlay_data->ov_data.regulator_bits, overlay_data->ov_data.regulators, PM_REGULATOR_MAX);

	/* TODO other data */
}

static struct platform_driver g_hisi_tmp_fb2_driver = {
	.probe = hisi_drv_tmp_fb2_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_TMP_FB2_DEV_NAME,
	},
};

int hisi_drv_tmp_fb2_create_device(void *connector_data)
{
	if (!g_hisi_fb2_ov_data) {
		g_hisi_fb2_ov_data = hisi_drv_create_device(HISI_TMP_FB2_DEV_NAME, sizeof(*g_hisi_fb2_ov_data),
					hisi_tmp_fb2_init_data, connector_data, NULL);
		if (!g_hisi_fb2_ov_data) {
			disp_pr_err("create %s device fail", HISI_TMP_FB2_DEV_NAME);
			return -1;
		}
	}

	return 0;
}

void hisi_drv_tmp_fb2_register_connector(struct hisi_connector_device *connector_data)
{
	struct hisi_composer_connector_device *next_device = NULL;

	if (!g_hisi_fb2_ov_data)
		return;

	disp_pr_info("enter +++++++, g_hisi_fb2_ov_data=%p \n", g_hisi_fb2_ov_data);

	next_device = devm_kzalloc(&(g_hisi_fb2_ov_data->pdev->dev), sizeof(*next_device), GFP_KERNEL);
	next_device->next_id = hisi_get_connector_id();
	next_device->next_pdev = connector_data->pdev;
	next_device->fix_panel_info = connector_data->fix_panel_info;
	hisi_drv_connector_register_ops(&(next_device->next_dev_ops));

	g_hisi_fb2_ov_data->next_device[g_hisi_fb2_ov_data->next_dev_count] = next_device;
	g_hisi_fb2_ov_data->next_dev_count++;
}

static int __init hisi_drv_tmp_fb2_init(void)
{
	int ret;

	disp_pr_info("enter +++++++ \n");

	ret = platform_driver_register(&g_hisi_tmp_fb2_driver);
	if (ret) {
		disp_pr_info("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("enter -------- \n");
	return ret;
}


module_init(hisi_drv_tmp_fb2_init);

MODULE_DESCRIPTION("Hisilicon Tmp Fb2 Driver");
MODULE_LICENSE("GPL v2");


