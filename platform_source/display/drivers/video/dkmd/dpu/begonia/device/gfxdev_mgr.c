/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dkmd_log.h"

#include <linux/module.h>
#include <linux/of_device.h>

#include "gfxdev_mgr.h"
#include "dpu_fb.h"
#include "dpu_drm.h"
#include "dpu_gfx.h"
#include "dvfs.h"

static uint32_t g_comp_count;
static struct composer *g_device_comp[DEVICE_COMP_MAX_COUNT];
static uint32_t g_disp_device_arch = FBDEV_ARCH;
static uint32_t g_fastboot_enable_flag;
static uint32_t g_fake_lcd_flag;

void gfxdev_blank_power_on(struct composer *comp)
{
	int32_t ret = -1;
	dpu_pr_info("blank power on current status %d", comp->power_on);

	if (comp->power_on)
		return;

	ret = comp->on(comp);
	if (ret) {
		dpu_pr_err("next composer power on fail");
		comp->power_on = false;
		return;
	}

	comp->power_on = true;
}

void gfxdev_blank_power_off(struct composer *comp)
{
	int32_t ret = -1;
	dpu_pr_info("blank power off current status %d", comp->power_on);

	dpu_dvfs_reset_dvfs_info();

	if (!comp->power_on)
		return;

	ret = comp->off(comp);
	if (ret) {
		dpu_pr_warn("next composer power off fail");
		comp->power_on = true;
		return;
	}

	comp->power_on = false;
}

void device_mgr_primary_frame_refresh(struct composer *comp, char *trigger)
{
	char *envp[2] = { "Refresh=1", NULL };

	if (!comp || !trigger)
		return;

	kobject_uevent_env(&(comp->base.peri_device->dev.kobj), KOBJ_CHANGE, envp);
	dpu_pr_info("%s need frame refresh!", trigger);
}

struct composer *get_comp_from_device(struct device *dev)
{
	if (strncmp(dev->kobj.name, "fb", 2) == 0)
		return get_comp_from_fb_device(dev);

	return get_comp_from_gfx_device(dev);
}

int32_t device_mgr_create_gfxdev(struct composer *comp)
{
	if (!comp) {
		dpu_pr_err("input comp is null!");
		return -EINVAL;
	}

	if (comp->index >= DEVICE_COMP_MAX_COUNT) {
		dpu_pr_err("invalid comp index=%d!", comp->index);
		return -EINVAL;
	}

	if (comp != g_device_comp[comp->index]) {
		dpu_pr_err("%s input comp[%pK] have not registed match for comp[%pK]!",
			comp->base.name, comp, g_device_comp[comp->index]);
		return -EINVAL;
	}

	if (is_offline_panel(&comp->base) || is_dp_panel(&comp->base) || is_hdmi_panel(&comp->base))
		return gfx_device_register(comp);

	comp->fastboot_display_enabled = g_fastboot_enable_flag;
	if (g_disp_device_arch == FBDEV_ARCH)
		return fb_device_register(comp);
	else
		return drm_device_register(comp);
}
EXPORT_SYMBOL(device_mgr_create_gfxdev);

void device_mgr_destroy_gfxdev(struct composer *comp)
{
	if (!comp) {
		dpu_pr_err("input comp is null!");
		return;
	}

	if (comp->index >= DEVICE_COMP_MAX_COUNT) {
		dpu_pr_err("invalid comp index=%d!", comp->index);
		return;
	}

	if (comp != g_device_comp[comp->index]) {
		dpu_pr_err("%s input comp[%pK] have not registed match for comp[%pK]!",
			comp->base.name, comp, g_device_comp[comp->index]);
		return;
	}
	g_device_comp[comp->index] = NULL;

	if (is_offline_panel(&comp->base) || is_dp_panel(&comp->base)) {
		gfx_device_unregister(comp);
		return;
	}

	if (g_disp_device_arch == FBDEV_ARCH)
		fb_device_unregister(comp);
	else
		drm_device_unregister(comp);
}
EXPORT_SYMBOL(device_mgr_destroy_gfxdev);

void device_mgr_shutdown_gfxdev(struct composer *comp)
{
	if (!comp) {
		dpu_pr_err("input comp is null!");
		return;
	}

	if (comp->index >= DEVICE_COMP_MAX_COUNT) {
		dpu_pr_err("invalid comp index=%d!", comp->index);
		return;
	}

	if (comp != g_device_comp[comp->index]) {
		dpu_pr_err("%s input comp[%pK] have not registed match for comp[%pK]!",
			comp->base.name, comp, g_device_comp[comp->index]);
		return;
	}

	if (is_offline_panel(&comp->base) || is_dp_panel(&comp->base))
		return;

	if (g_disp_device_arch == FBDEV_ARCH)
		fb_device_shutdown(comp);
}
EXPORT_SYMBOL(device_mgr_shutdown_gfxdev);

void device_mgr_register_comp(struct composer *comp)
{
	if (!comp) {
		dpu_pr_err("input comp is null!");
		return;
	}

	comp->index = (g_comp_count++) % DEVICE_COMP_MAX_COUNT;
	if (g_device_comp[comp->index] != NULL) {
		dpu_pr_err("%s register comp:%pK failed, please check", comp->base.name, comp);
		return;
	}

	g_device_comp[comp->index] = comp;
	dpu_pr_info("%s register comp:%pK success", comp->base.name, comp);
}
EXPORT_SYMBOL(device_mgr_register_comp);

static int32_t gfxdev_probe(struct platform_device *pdev)
{
	int32_t ret;

	ret = of_property_read_u32(pdev->dev.of_node, "disp_device_arch", &g_disp_device_arch);
	if (ret) {
		dpu_pr_info("get disp_device_arch failed!");
		g_disp_device_arch = FBDEV_ARCH;
	}
	dpu_pr_info("g_disp_device_arch=%#x", g_disp_device_arch);

	ret = of_property_read_u32(pdev->dev.of_node, "fastboot_enable_flag", &g_fastboot_enable_flag);
	if (ret) {
		dpu_pr_info("get fastboot_enable_flag failed!");
		g_fastboot_enable_flag = 0;
	}
	dpu_pr_info("g_fastboot_enable_flag=%#x", g_fastboot_enable_flag);

	ret = of_property_read_u32(pdev->dev.of_node, "fake_lcd_flag", &g_fake_lcd_flag);
	if (ret) {
		dpu_pr_info("get fake_lcd_flag failed!");
		g_fake_lcd_flag = 0;
	}
	dpu_pr_info("g_fake_lcd_flag=%#x", g_fake_lcd_flag);

	return 0;
}

static int32_t gfxdev_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id gfxdev_match_table[] = {
	{
		.compatible = "dkmd,dpu_device",
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, gfxdev_match_table);

static struct platform_driver gfxdev_driver = {
	.probe = gfxdev_probe,
	.remove = gfxdev_remove,
	.driver = {
		.name = "gfxdev",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(gfxdev_match_table),
	},
};

static int32_t __init gfxdev_driver_init(void)
{
	return platform_driver_register(&gfxdev_driver);
}

static void __exit gfxdev_driver_deinit(void)
{
	platform_driver_unregister(&gfxdev_driver);
}

module_init(gfxdev_driver_init);
module_exit(gfxdev_driver_deinit);

MODULE_DESCRIPTION("Display Graphics Driver");
MODULE_LICENSE("GPL");
