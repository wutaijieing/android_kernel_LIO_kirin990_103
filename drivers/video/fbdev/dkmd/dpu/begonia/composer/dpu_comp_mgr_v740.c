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
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <uapi/linux/sched/types.h>
#include "dpu_comp_mgr.h"
#include "dpu_config_utils.h"
#include "gfxdev_mgr.h"
#include "dkmd_log.h"
#include "dpu_comp_vsync.h"

#define SOC_MEDIA1_CTRL_INTR_QIC_BUS_DSS_GRP0_ADDR(base) ((base) + (0x0868))
#define SOC_MEDIA1_CTRL_INTR_QIC_BUS_DSS_GRP1_ADDR(base) ((base) + (0x086c))

static struct composer_manager *g_composer_manager = NULL;

static int32_t composer_manager_get_prodcut_config(struct composer *comp, struct product_config *out_config)
{
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);

	if (!dpu_comp) {
		dpu_pr_err("dpu_comp is nullptr");
		return -1;
	}

	if (unlikely(!out_config)) {
		dpu_pr_err("out config is null");
		return -1;
	}

	/* used for framebuffer device regitser */
	comp->base.xres = dpu_comp->conn_info->base.xres;
	comp->base.yres = dpu_comp->conn_info->base.yres;
	comp->base.width = dpu_comp->conn_info->base.width;
	comp->base.height = dpu_comp->conn_info->base.height;
	comp->base.fps = dpu_comp->conn_info->base.fps;

	/* used for dsc_info send to dumd */
	comp->base.dsc_out_width = dpu_comp->conn_info->base.dsc_out_width;
	comp->base.dsc_out_height = dpu_comp->conn_info->base.dsc_out_height;
	comp->base.dsc_en = dpu_comp->conn_info->base.dsc_en;
	comp->base.spr_en = dpu_comp->conn_info->base.spr_en;

	/* get product config information
	 * get split mode frome connecotr info, it must get from panel dtsi
	 */
	out_config->display_count = 1;
	out_config->split_count = 1;
	out_config->split_mode = SCENE_SPLIT_MODE_NONE;
	out_config->split_ratio[SCENE_SPLIT_MODE_NONE] = 0;
	out_config->split_ratio[SCENE_SPLIT_MODE_V] = 0;
	out_config->split_ratio[SCENE_SPLIT_MODE_H] = 0;
	out_config->dim_info_count = 0;
	out_config->fps_info_count = 0;

	out_config->color_mode_count = 2;
	out_config->color_modes[0] = COLOR_MODE_NATIVE;
	out_config->color_modes[1] = COLOR_MODE_SRGB;

	out_config->dsc_out_width = comp->base.dsc_out_width;
	out_config->dsc_out_height = comp->base.dsc_out_height;
	out_config->feature_switch.bits.enable_dsc = comp->base.dsc_en;
	out_config->feature_switch.bits.enable_spr = comp->base.spr_en;

	out_config->feature_switch.bits.enable_hdr10 = 1;

	return 0;
}

static int32_t composer_manager_get_composer_frame_index(struct composer *comp)
{
	return comp->comp_frame_index;
}

static ssize_t composer_frame_index_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;

	dpu_check_and_return((!dev || !buf), -1, err, "input is null pointer");

	ret = snprintf(buf, PAGE_SIZE, "%u\n", composer_manager_get_composer_frame_index(get_comp_from_device(dev)));
	buf[strlen(buf) + 1] = '\0';

	dpu_pr_info("buf:%s ", buf);

	return ret;
}

static DEVICE_ATTR(comp_frame_index, 0444, composer_frame_index_show, NULL);

static int32_t composer_manager_get_sysfs_attrs(struct composer *comp, struct dkmd_attr **out_attr)
{
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);

	if (!dpu_comp) {
		dpu_pr_err("dpu_comp is nullptr");
		return -1;
	}

	*out_attr = &(dpu_comp->attrs);
	dkmd_sysfs_attrs_append(&(dpu_comp->attrs), &dev_attr_comp_frame_index.attr);

	return 0;
}

static int32_t composer_manager_create_fence(struct composer *comp)
{
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);

	if (!dpu_comp) {
		dpu_pr_err("dpu_comp is nullptr");
		return -1;
	}

	return dpu_comp->create_fence(dpu_comp);
}

static void dpu_comp_mgr_common_init(struct dpu_composer *dpu_comp)
{
	char tmp_name[256] = {0};
	struct composer *comp = &dpu_comp->comp;
	struct sched_param param = {
		.sched_priority = MAX_RT_PRIO - 1,
	};

	dpu_comp->comp_mgr = g_composer_manager;
	dpu_comp->init_scene_cmdlist = 0;
	device_mgr_register_comp(comp);
	dkmd_sysfs_init(&dpu_comp->attrs);
	sema_init(&comp->blank_sem, 1);

	/* copy obj_info from connector */
	comp->base = dpu_comp->conn_info->base;

	/* update dpu_comp peri device pointer to connector manager */
	comp->base.peri_device = dpu_comp->conn_info->conn_device;

	/* update link relationships
	 * connector->prev is dpu_composer
	 * dpu_composer->next is connector
	 */
	dpu_comp->conn_info->base.comp_obj_info = &comp->base;

	/* initialize the composer interface which would be called by device component */
	comp->on = composer_manager_on; /* MUST */
	comp->off = composer_manager_off; /* MUST */
	comp->present = composer_manager_present; /* MUST */

	/* follow interfaces are optional */
	comp->set_fastboot = composer_manager_set_fastboot;
	comp->create_fence = composer_manager_create_fence;
	comp->get_product_config = composer_manager_get_prodcut_config;
	comp->get_sysfs_attrs = composer_manager_get_sysfs_attrs;

	/* public each scene kernel processing threads, would be initialized only once */
	kthread_init_worker(&dpu_comp->handle_worker);
	(void)snprintf(tmp_name, sizeof(tmp_name), "dpu_%s", comp->base.name);
	dpu_comp->handle_thread = kthread_create(kthread_worker_fn, &dpu_comp->handle_worker, tmp_name);
	if (IS_ERR_OR_NULL(dpu_comp->handle_thread)) {
		dpu_pr_warn("%s failed to create handle_thread!", comp->base.name);
		return;
	}
	(void)sched_setscheduler_nocheck(dpu_comp->handle_thread, SCHED_FIFO, &param);
	(void)wake_up_process(dpu_comp->handle_thread);
}

void composer_dpu_free_logo_buffer(struct dpu_composer *dpu_comp)
{
	int ret, i;
	uint32_t logo_buffer_base = 0;
	uint32_t logo_buffer_size = 0;
	struct device_node *np = NULL;

	np = of_find_node_by_path(DTS_PATH_LOGO_BUFFER);
	if (!np)
		dpu_pr_err("NOT FOUND dts path: %s!\n", DTS_PATH_LOGO_BUFFER);

	ret = of_property_read_u32_index(np, "reg", 1, &logo_buffer_base);
	if (ret != 0) {
		dpu_pr_err("failed to get logo_buffer_base resource! ret=%d\n", ret);
		logo_buffer_base = 0;
	}
	ret = of_property_read_u32_index(np, "reg", 3, &logo_buffer_size);
	if (ret != 0) {
		dpu_pr_err("failed to get logo_buffer_base resource! ret=%d\n", ret);
		logo_buffer_size = 0;
	}
	dpu_pr_info("free logo_buffer_size=0x%x, logo_buffer_size=0x%x\n", logo_buffer_base, logo_buffer_size);

	for (i = 0; i < (int32_t)(logo_buffer_size / PAGE_SIZE); i++) {
		free_reserved_page(phys_to_page(logo_buffer_base));
		logo_buffer_base += PAGE_SIZE;
	}
	memblock_free(logo_buffer_base, logo_buffer_size);
}

int32_t register_composer(struct dkmd_connector_info *pinfo)
{
	int ret = 0;
	size_t present_data_size = 0;
	struct dpu_composer *dpu_comp = NULL;
	struct platform_device *peri_device = NULL;

	dpu_check_and_return(!g_composer_manager, -EINVAL, warn, "composer manager is not ready!");
	dpu_check_and_return(!pinfo, -EINVAL, err, "pinfo is null!");

	peri_device = pinfo->base.peri_device;
	dpu_check_and_return(!peri_device, -EINVAL, err, "peri_device is null!");
	dpu_pr_info("register %s connector device!", peri_device->name);

	/* each connector interface has owner dpu_composer struct */
	dpu_comp = (struct dpu_composer *)devm_kzalloc(&peri_device->dev, sizeof(*dpu_comp), GFP_KERNEL);
	dpu_check_and_return(!dpu_comp, -ENOMEM, err, "alloc dpu_comp failed!");

	/* Initialize the dpu_comp struct */
	dpu_comp->conn_info = pinfo;
	dpu_comp_mgr_common_init(dpu_comp);

	present_data_size = is_offline_panel(&pinfo->base) ?
		sizeof(struct comp_offline_present) : sizeof(struct comp_online_present);
	dpu_comp->present_data = devm_kzalloc(&peri_device->dev, present_data_size, GFP_KERNEL);
	if (!dpu_comp->present_data) {
		kthread_stop(dpu_comp->handle_thread);
		free_kthread_struct(dpu_comp->handle_thread);
		list_del(&dpu_comp->isr_ctrl.list_node);
		devm_kfree(&peri_device->dev, dpu_comp);
		dpu_pr_err("alloc present_data failed!");
		return -ENOMEM;
	}
	composer_present_data_setup(dpu_comp, true);

	ret = device_mgr_create_gfxdev(&dpu_comp->comp);
	if (ret != 0) {
		kthread_stop(dpu_comp->handle_thread);
		free_kthread_struct(dpu_comp->handle_thread);
		list_del(&dpu_comp->isr_ctrl.list_node);
		composer_present_data_release(dpu_comp);
		devm_kfree(&peri_device->dev, dpu_comp->present_data);
		devm_kfree(&peri_device->dev, dpu_comp);
		return -EINVAL;
	}

	return 0;
}

void unregister_composer(struct dkmd_connector_info *pinfo)
{
	struct composer *comp = NULL;
	struct dpu_composer *dpu_comp = NULL;

	comp = container_of(pinfo->base.comp_obj_info, struct composer, base);
	dpu_check_and_no_retval(!comp, err, "comp is null!");

	device_mgr_destroy_gfxdev(comp);

	dpu_comp = to_dpu_composer(comp);
	composer_present_data_release(dpu_comp);
	kthread_stop(dpu_comp->handle_thread);
	free_kthread_struct(dpu_comp->handle_thread);
	list_del(&dpu_comp->isr_ctrl.list_node);

	kfree(dpu_comp);
}

void composer_device_shutdown(struct dkmd_connector_info *pinfo)
{
	struct composer *comp = NULL;

	if (!pinfo || !pinfo->base.comp_obj_info) {
		dpu_pr_err("pinfo or comp_obj_info is nullptr!");
		return;
	}

	comp = container_of(pinfo->base.comp_obj_info, struct composer, base);
	if (!comp) {
		dpu_pr_err("comp is nullptr!");
		return;
	}

	device_mgr_shutdown_gfxdev(comp);
}

void composer_active_vsync(struct dkmd_connector_info *pinfo, bool need_active_vsync)
{
	struct composer *comp = NULL;
	struct dpu_composer *dpu_comp = NULL;

	if (!pinfo || !pinfo->base.comp_obj_info) {
		dpu_pr_err("pinfo or comp_obj_info is nullptr!");
		return;
	}

	comp = container_of(pinfo->base.comp_obj_info, struct composer, base);
	if (!comp) {
		dpu_pr_err("comp is nullptr!");
		return;
	}

	dpu_comp = to_dpu_composer(comp);
	if (need_active_vsync)
		dpu_comp_active_vsync(dpu_comp);
	else
		dpu_comp_deactive_vsync(dpu_comp);
}

static int32_t composer_manager_parse_dt(struct composer_manager *comp_mgr)
{
	int32_t ret;
	struct platform_device *pdev = comp_mgr->device;

	comp_mgr->dpu_base = of_iomap(comp_mgr->parent_node, 0);
	if (!comp_mgr->dpu_base) {
		dpu_pr_err("failed to get dpu_base!\n");
		return -ENXIO;
	}

	comp_mgr->actrl_base = of_iomap(comp_mgr->parent_node, 1);
	if (!comp_mgr->actrl_base) {
		dpu_pr_err("failed to get actrl_base!\n");
		return -ENXIO;
	}

	comp_mgr->media1_ctrl_base = of_iomap(comp_mgr->parent_node, 2);
	if (!comp_mgr->media1_ctrl_base) {
		dpu_pr_err("failed to get media1_ctrl_base!\n");
		return -ENXIO;
	}

	comp_mgr->dsssubsys_regulator = devm_regulator_get(&pdev->dev, "regulator_dsssubsys");
	if (IS_ERR(comp_mgr->dsssubsys_regulator)) {
		ret = PTR_ERR(comp_mgr->dsssubsys_regulator);
		dpu_pr_err("dsssubsys_regulator error, ret=%d", ret);
		return ret;
	}

	comp_mgr->dpsubsys_regulator = devm_regulator_get(&pdev->dev, "regulator_dpsubsys");
	if (IS_ERR(comp_mgr->dpsubsys_regulator)) {
		ret = PTR_ERR(comp_mgr->dpsubsys_regulator);
		dpu_pr_err("dpsubsys_regulator error, ret=%d", ret);
		return ret;
	}

	comp_mgr->dsisubsys_regulator = devm_regulator_get(&pdev->dev, "regulator_dsisubsys");
	if (IS_ERR(comp_mgr->dsisubsys_regulator)) {
		ret = PTR_ERR(comp_mgr->dsisubsys_regulator);
		dpu_pr_err("dsisubsys_regulator error, ret=%d", ret);
		return ret;
	}

	comp_mgr->vivobus_regulator = devm_regulator_get(&pdev->dev, "regulator_vivobus");
	if (IS_ERR(comp_mgr->vivobus_regulator)) {
		ret = PTR_ERR(comp_mgr->vivobus_regulator);
		dpu_pr_err("vivobus_regulator error, ret=%d", ret);
		return ret;
	}

	comp_mgr->media1_subsys_regulator = devm_regulator_get(&pdev->dev, "regulator_media_subsys");
	if (IS_ERR(comp_mgr->media1_subsys_regulator)) {
		ret = PTR_ERR(comp_mgr->media1_subsys_regulator);
		dpu_pr_err("media1_subsys_regulator error, ret=%d", ret);
		return ret;
	}

	comp_mgr->sdp_irq = of_irq_get(comp_mgr->parent_node, 0);
	dpu_pr_info("comp_mgr sdp_irq=%d", comp_mgr->sdp_irq);

	comp_mgr->m1_qic_irq = of_irq_get(comp_mgr->parent_node, 1);
	dpu_pr_info("comp_mgr m1_qic_irq=%d", comp_mgr->m1_qic_irq);

	return 0;
}

static irqreturn_t dpu_sdp_isr(int32_t irq, void *ptr)
{
	uint32_t isr1_state;
	char __iomem *dpu_base = NULL;
	struct composer_manager *comp_mgr = (struct composer_manager *)ptr;

	dpu_check_and_return(!comp_mgr, IRQ_NONE, err, "comp_mgr is null!");
	dpu_base = comp_mgr->dpu_base;

	isr1_state = inp32(DPU_GLB_NS_SDP_TO_GIC_O_ADDR(dpu_base + DPU_GLB0_OFFSET));
	outp32(DPU_GLB_NS_SDP_TO_GIC_O_ADDR(dpu_base + DPU_GLB0_OFFSET), isr1_state);
	isr1_state &= ~((uint32_t)inp32(DPU_GLB_NS_SDP_TO_GIC_MSK_ADDR(dpu_base + DPU_GLB0_OFFSET)));
	if ((isr1_state & DPU_DPP0_HIACE_NS_INT) == DPU_DPP0_HIACE_NS_INT) {
		dpu_pr_info("get dpp hiace intr:%#x", isr1_state);
		dkmd_isr_notify_listener(&comp_mgr->sdp_isr_ctrl, DPU_DPP0_HIACE_NS_INT);
	}

	return IRQ_HANDLED;
}

static void dpu_isr_notify_listener(struct dkmd_isr *isr_ctrl, uint32_t isr_state, uint32_t listen_bit)
{
	if ((isr_state & listen_bit) == listen_bit)
		dkmd_isr_notify_listener(isr_ctrl, listen_bit);
}

static irqreturn_t dpu_m1_qic_isr(int32_t irq, void *ptr)
{
	uint32_t isr1_state;
	uint32_t isr2_state;
	char __iomem *media1_ctrl_base = NULL;
	struct composer_manager *comp_mgr = (struct composer_manager *)ptr;

	dpu_check_and_return(!comp_mgr, IRQ_NONE, err, "comp_mgr is null!");
	media1_ctrl_base = comp_mgr->media1_ctrl_base;

	isr1_state = inp32(SOC_MEDIA1_CTRL_INTR_QIC_BUS_DSS_GRP0_ADDR(media1_ctrl_base));
	dpu_pr_err("get m1 qic intr:intr_qic_bus_dss_grp0(%#x)", isr1_state);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_TB_DSS_CFG);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_TB_DSS_CFG);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_WR);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_WR);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT1_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT1_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_WR);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_WR);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT3_DATA_RD);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr1_state, DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT3_DATA_RD);

	isr2_state = inp32(SOC_MEDIA1_CTRL_INTR_QIC_BUS_DSS_GRP1_ADDR(media1_ctrl_base));
	dpu_pr_err("get m1 qic intr:intr_qic_bus_dss_grp1(%#x)", isr2_state);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr2_state, DPU_M1_QIC_INTR_SAFETY_TB_QCP_DSS);
	dpu_isr_notify_listener(&comp_mgr->m1_qic_isr_ctrl, isr2_state, DPU_M1_QIC_INTR_SAFETY_ERR_TB_QCP_DSS);

	return IRQ_HANDLED;
}

static void dpu_isr_ctrl_init(struct composer_manager *comp_mgr, struct dkmd_isr *isr_ctrl, int32_t irq_no,
								char *irq_name, uint32_t unmask, irqreturn_t (*isr_fnc)(int32_t irq, void *ptr))
{
	(void)snprintf(isr_ctrl->irq_name, sizeof(isr_ctrl->irq_name), irq_name);
	isr_ctrl->irq_no = irq_no;
	isr_ctrl->isr_fnc = isr_fnc;
	isr_ctrl->parent = comp_mgr;
	isr_ctrl->unmask = unmask;
	dkmd_isr_setup(isr_ctrl);
}

static void composer_manager_setup(struct composer_manager *comp_mgr)
{
	struct dkmd_isr *isr_ctrl = &comp_mgr->sdp_isr_ctrl;

	mutex_init(&(comp_mgr->idle_lock));
	comp_mgr->active_status.status = 0;
	comp_mgr->idle_func_flag = 0;
	comp_mgr->idle_enable = true;
	comp_mgr->power_status.status = 0;
	sema_init(&comp_mgr->power_sem, 1);
	mutex_init(&comp_mgr->tbu_sr_lock);
	INIT_LIST_HEAD(&comp_mgr->isr_list);

	dpu_isr_ctrl_init(comp_mgr, &comp_mgr->sdp_isr_ctrl, comp_mgr->sdp_irq, "dpu_irq_sdp",
						~(DPU_DPP0_HIACE_NS_INT | DPU_DACC_NS_INT), dpu_sdp_isr);
	dpu_isr_ctrl_init(comp_mgr, &comp_mgr->m1_qic_isr_ctrl, comp_mgr->m1_qic_irq, "dpu_irq_m1_qic",
						0, dpu_m1_qic_isr);

	list_add_tail(&isr_ctrl->list_node, &comp_mgr->isr_list);
}

static int32_t composer_manager_probe(struct platform_device *pdev)
{
	uint32_t scene_id;
	struct device_node *node = NULL;
	struct composer_manager *comp_mgr = NULL;

	comp_mgr = (struct composer_manager *)devm_kzalloc(&pdev->dev, sizeof(*comp_mgr), GFP_KERNEL);
	if (!comp_mgr) {
		dpu_pr_err("alloc dpu_comp_mgr failed!");
		return -EINVAL;
	}
	dpu_pr_info("v740 +");
	comp_mgr->device = pdev;
	comp_mgr->parent_node = pdev->dev.of_node;
	comp_mgr->hardware_reseted = 0;
	composer_manager_parse_dt(comp_mgr);

	/* parse child scene node for composer manager */
	for_each_child_of_node(comp_mgr->parent_node, node) {
		if (!of_device_is_available(node))
			continue;

		of_property_read_u32(node, "scene_id", &scene_id);
		dpu_pr_info("get composer scene id=%u", scene_id);
		if (scene_id > DPU_SCENE_OFFLINE_2) {
			dpu_pr_warn("scene id=%u over %d", scene_id, DPU_SCENE_OFFLINE_2);
			continue;
		}

		comp_mgr->scene[scene_id] = devm_kzalloc(&pdev->dev, sizeof(struct composer_scene), GFP_KERNEL);
		if (!comp_mgr->scene[scene_id]) {
			dpu_pr_warn("alloc scene struct failed!");
			return -ENOMEM;
		}
		comp_mgr->scene[scene_id]->dpu_base = comp_mgr->dpu_base;
		comp_mgr->scene[scene_id]->scene_id = scene_id;

		/* To initialize composer scene module interface */
		dpu_comp_scene_device_setup(comp_mgr->scene[scene_id]);
	}
	platform_set_drvdata(pdev, comp_mgr);
	composer_manager_setup(comp_mgr);
	g_composer_manager = comp_mgr;

	return 0;
}

static const struct of_device_id g_composer_manager_match_table[] = {
	{
		.compatible = "dkmd,composer_manager",
		.data = NULL,
	},
	{},
};

static struct platform_driver g_composer_manager_driver = {
	.probe = composer_manager_probe,
	.remove = NULL,
	.driver = {
		.name = "composer_manager",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_composer_manager_match_table),
	}
};

static int32_t __init composer_manager_register(void)
{
	return platform_driver_register(&g_composer_manager_driver);
}
module_init(composer_manager_register);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Composer Manager Module Driver");
MODULE_LICENSE("GPL");
