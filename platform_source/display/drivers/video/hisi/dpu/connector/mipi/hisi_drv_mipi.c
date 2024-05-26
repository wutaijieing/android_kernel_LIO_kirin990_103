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
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/module.h>

#include "hisi_connector_utils.h"
#include "hisi_disp_debug.h"
#include "hisi_mipi_core.h"
#include "hisi_disp_composer.h"
#include "hisi_drv_mipi_chrdev.h"
#include "hisi_disp_config.h"
#include "hisi_mipi_dsi.h"
#include "hisi_disp_gadgets.h"
static void hisi_connector_init_base_addr(struct hisi_connector_base_addr *base_addr)
{
	base_addr->dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);
	base_addr->peri_crg_base = hisi_config_get_ip_base(DISP_IP_BASE_PERI_CRG);

	disp_pr_info("dpu base = 0x%x", base_addr->dpu_base_addr);
	disp_pr_info("peri base = 0x%x", base_addr->peri_crg_base);
}

static int hisi_drv_mipi_probe(struct platform_device *pdev)
{
	struct hisi_connector_device *connector_dev = NULL;
	int ret;

	disp_pr_info("enter ++++++++++++ ");

	connector_dev = platform_get_drvdata(pdev);
	disp_assert_if_cond(connector_dev == NULL);

	disp_pr_err("hisi_drv_mipi_probe enter! pdev=%p, connector_data=%p \n", pdev, connector_dev);

	/* create overlay device, will generate overlay platform device */
	ret = hisi_drv_primary_composer_create_device(connector_dev);
	if (ret)
		return -1;
	mipi_transfer_lock_init();
	hisi_connector_init_base_addr(&connector_dev->base_addr);

	/* registe mipi master */
	hisi_mipi_init_component(connector_dev);

	/* mipi set backlight ops */
	hisi_drv_connector_register_bl_ops(&connector_dev->bl_ctrl.bl_ops);

	/* backlight register */
	sema_init(&connector_dev->brightness_esd_sem, 1);
	hisi_backlight_register(connector_dev);

	/* register connector to overlay device */
	hisi_drv_primary_composer_register_connector(connector_dev);
	hisi_drv_mipi_create_chrdev(connector_dev);

	disp_pr_info("exit ------------- ");
	return 0;
}

static int hisi_drv_mipi_remove(struct platform_device *pdev)
{
	struct hisi_connector_device *connector_data = NULL;

	disp_assert_if_cond(pdev == NULL);

	connector_data = platform_get_drvdata(pdev);

	/* backlight unregister */
	hisi_backlight_unregister(connector_data);

	hisi_drv_mipi_destroy_chrdev(connector_data);
	return 0;
}

static struct platform_driver g_hisi_mipi_driver = {
	.probe = hisi_drv_mipi_probe,
	.remove = hisi_drv_mipi_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_CONNECTOR_PRIMARY_MIPI_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init hisi_drv_mipi_init(void)
{
	int ret;

	disp_pr_info(" ++++ \n", ret);
	ret = platform_driver_register(&g_hisi_mipi_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(hisi_drv_mipi_init);

MODULE_DESCRIPTION("Hisilicon mipi dsi Driver");
MODULE_LICENSE("GPL v2");
