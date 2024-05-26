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

#include "dpu_conn_mgr.h"

struct dpu_conn_manager *g_conn_manager = NULL;
EXPORT_SYMBOL(g_conn_manager);

static int32_t connector_manager_on(struct dkmd_connector_info *pinfo)
{
	int32_t ret = 0;
	struct dpu_connector *connector = NULL;

	if (!pinfo) {
		dpu_pr_info("pinfo is null!\n");
		return -EINVAL;
	}

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_info("pipe_sw post_ch=%u is not available!\n", pinfo->sw_post_chn_idx[0]);
		return -EINVAL;
	}

	if (connector->on_func)
		ret = connector->on_func(pinfo);

	return ret;
}

static int32_t connector_manager_off(struct dkmd_connector_info *pinfo)
{
	int32_t ret = 0;
	struct dpu_connector *connector = NULL;

	if (!pinfo) {
		dpu_pr_info("pinfo is null!\n");
		return -EINVAL;
	}

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_info("pipe_sw post_ch=%u is not available!\n", pinfo->sw_post_chn_idx[0]);
		return -EINVAL;
	}

	if (connector->off_func)
		ret = connector->off_func(pinfo);

	return ret;
}

static int32_t connector_manager_ops_handle(struct dkmd_connector_info *pinfo,
	uint32_t ops_cmd_id, void *value)
{
	int32_t ret = 0;
	struct dpu_connector *connector = NULL;

	dpu_pr_info("ops_cmd_id=%d!\n", ops_cmd_id);
	if (!pinfo) {
		dpu_pr_info("pinfo is null!\n");
		return -EINVAL;
	}

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_info("pipe_sw post_ch=%u is not available!\n", pinfo->sw_post_chn_idx[0]);
		return -EINVAL;
	}

	if (connector->ops_handle_func)
		ret = connector->ops_handle_func(pinfo, ops_cmd_id, value);

	return ret;
}

static void calculate_dsc_spr_param(struct dsc_calc_info *dsc, struct dkmd_connector_info *pinfo)
{
	if (dsc->dsc_en != 0)
		dsc_calculation(dsc, pinfo->ifbc_type);

	pinfo->base.dsc_out_width = get_dsc_out_width(dsc, pinfo->ifbc_type, pinfo->base.xres, pinfo->base.xres);
	pinfo->base.dsc_out_height = get_dsc_out_height(dsc, pinfo->ifbc_type, pinfo->base.yres);
	pinfo->base.dsc_en = dsc->dsc_en & 0x1;
	pinfo->base.spr_en = dsc->spr_en;
}

int register_connector(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = NULL;
	struct dpu_connector *bind_connector = NULL;
	uint32_t i;

	if (!pinfo) {
		dpu_pr_info("pinfo is null!\n");
		return -EINVAL;
	}
	pinfo->conn_device = g_conn_manager->device;

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_info("pipe_sw post_ch=%u is not available!\n", pinfo->sw_post_chn_idx[0]);
		return -EINVAL;
	}
	connector->conn_info = pinfo;
	bind_connector = connector->bind_connector;
	if (bind_connector) {
		bind_connector->conn_info = pinfo;
		bind_connector->mipi = connector->mipi;
		for (i = 0; i < CLK_GATE_MAX_IDX; i++)
			bind_connector->connector_clk[i] = connector->connector_clk[i];
	}
	dpu_connector_setup(connector);
	calculate_dsc_spr_param(&connector->dsc, pinfo);

	return register_composer(pinfo);
}

int32_t unregister_connector(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = NULL;

	if (!pinfo) {
		dpu_pr_err("pinfo is NULL!\n");
		return -EINVAL;
	}
	unregister_composer(pinfo);

	/* do something for connector unregister */
	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_info("connector is not available!\n");
		return -EINVAL;
	}
	connector->conn_info = NULL;

	return 0;
}

void connector_device_shutdown(struct dkmd_connector_info *pinfo)
{
	struct dpu_connector *connector = NULL;

	if (!pinfo) {
		dpu_pr_err("pinfo is NULL!\n");
		return;
	}
	composer_device_shutdown(pinfo);

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_info("connector is not available!\n");
		return;
	}
	dpu_pr_info("connector%u link device shutdown!\n", connector->pipe_sw_post_chn_idx);
}

static int32_t dpu_conn_manager_parse_dt(struct dpu_conn_manager *conn_mgr,
	struct device_node *parent_node)
{
	struct platform_device *pdev = conn_mgr->device;
	conn_mgr->clk_gate_edc = of_clk_get(parent_node, 0);
	if (IS_ERR_OR_NULL(conn_mgr->clk_gate_edc)) {
		dpu_pr_err("failed to get clk_gate_edc!\n");
		return -ENXIO;
	}

	conn_mgr->dsssubsys_regulator = devm_regulator_get(&pdev->dev, "regulator_dsssubsys");
	if (IS_ERR_OR_NULL(conn_mgr->dsssubsys_regulator)) {
		dpu_pr_err("get dsssubsys_regulator error\n");
		return PTR_ERR(conn_mgr->dsssubsys_regulator);
	}

	conn_mgr->dpsubsys_regulator = devm_regulator_get(&pdev->dev, "regulator_dpsubsys");
	if (IS_ERR_OR_NULL(conn_mgr->dpsubsys_regulator)) {
		dpu_pr_err("get dpsubsys_regulator error\n");
		return PTR_ERR(conn_mgr->dpsubsys_regulator);
	}

	conn_mgr->dsisubsys_regulator = devm_regulator_get(&pdev->dev, "regulator_dsisubsys");
	if (IS_ERR_OR_NULL(conn_mgr->dsisubsys_regulator)) {
		dpu_pr_err("get dsisubsys_regulator error\n");
		return PTR_ERR(conn_mgr->dsisubsys_regulator);
	}

	conn_mgr->vivobus_regulator = devm_regulator_get(&pdev->dev, "regulator_vivobus");
	if (IS_ERR(conn_mgr->vivobus_regulator)) {
		dpu_pr_err("get vivobus_regulator error\n");
		return PTR_ERR(conn_mgr->vivobus_regulator);
	}

	conn_mgr->media1_subsys_regulator = devm_regulator_get(&pdev->dev, "regulator_media_subsys");
	if (IS_ERR(conn_mgr->media1_subsys_regulator)) {
		dpu_pr_err("get media1_subsys_regulator error\n");
		return PTR_ERR(conn_mgr->media1_subsys_regulator);
	}

	conn_mgr->dpu_base = of_iomap(parent_node, 0);
	if (!conn_mgr->dpu_base) {
		dpu_pr_err("failed to get dpu_base!\n");
		return -ENXIO;
	}

	conn_mgr->peri_crg_base = of_iomap(parent_node, 1);
	if (!conn_mgr->peri_crg_base) {
		dpu_pr_err("failed to get peri_crg_base!\n");
		return -ENXIO;
	}

	conn_mgr->pctrl_base = of_iomap(parent_node, 2);
	if (!conn_mgr->pctrl_base) {
		dpu_pr_err("failed to get pctrl_base!\n");
		return -ENXIO;
	}

	return 0;
}

static void dpu_child_connector_parse_dt(struct dpu_connector *connector,
	struct device_node *node)
{
	int32_t i;
	int32_t ret;

	for (i = 0; i < CLK_GATE_MAX_IDX; i++) {
		connector->connector_clk[i] = of_clk_get(node, i);
		if (IS_ERR_OR_NULL(connector->connector_clk[i])) {
			dpu_pr_info("of clk get %d failed, maybe have no clk!", i);
			break;
		}
	}
	connector->connector_irq = of_irq_get(node, 0);
	dpu_pr_info("connector_irq=%d", connector->connector_irq);

	if ((connector->pipe_sw_post_chn_idx != PIPE_SW_POST_CH_DP) &&
		(connector->pipe_sw_post_chn_idx != PIPE_SW_POST_CH_EDP))
		return;

	for (i = 0; i < DPTX_COMBOPHY_PARAM_NUM; i++) {
		ret = of_property_read_u32_index(node, "preemphasis_swing",
			i, &(connector->combophy_ctrl.combophy_pree_swing[i]));
		if (ret) {
			dpu_pr_info("[DP] preemphasis_swing[%d] is got fail!", i);
			break;
		}
		dpu_pr_info("combophy_pree_swing[%d]=0x%x", i, connector->combophy_ctrl.combophy_pree_swing[i]);
	}

	/* parse ssc ppm configure values */
	for (i = 0; i < DPTX_COMBOPHY_SSC_PPM_PARAM_NUM; i++) {
		ret = of_property_read_u32_index(node, "ssc_ppm",
			i, &(connector->combophy_ctrl.combophy_ssc_ppm[i]));
		if (ret) {
			dpu_pr_info("[DP] combophy_ssc_ppm[%d] is got fail!", i);
			break;
		}
		dpu_pr_info("combophy_ssc_ppm[%d]=0x%x", i, connector->combophy_ctrl.combophy_ssc_ppm[i]);
	}
}

static void dpu_conn_manager_parse_child_node(struct dpu_conn_manager *conn_mgr,
	struct device_node *parent_node)
{
	uint32_t chn_idx = 0;
	struct device_node *node = NULL;
	struct dpu_connector *connector = NULL;
	struct platform_device *pdev = conn_mgr->device;

	for_each_child_of_node(parent_node, node) {
		if (!of_device_is_available(node))
			continue;

		of_property_read_u32(node, "pipe_sw_post_chn_idx", &chn_idx);
		if (chn_idx >= PIPE_SW_POST_CH_MAX) {
			dpu_pr_warn("pipe_sw_post_chn_idx=%u over %u\n", chn_idx, PIPE_SW_POST_CH_MAX);
			continue;
		}
		if (conn_mgr->connector[chn_idx]) {
			dpu_pr_warn("connector pipe_sw_post_chn_idx=%u has been alloced\n", chn_idx);
			continue;
		}

		connector = devm_kzalloc(&pdev->dev, sizeof(*connector), GFP_KERNEL);
		if (!connector) {
			dpu_pr_warn("alloc connector struct failed!\n");
			return;
		}
		connector->dpu_base = conn_mgr->dpu_base;
		connector->peri_crg_base = conn_mgr->peri_crg_base;
		connector->pctrl_base = conn_mgr->pctrl_base;
		connector->pipe_sw_post_chn_idx = chn_idx;
		connector->conn_mgr = conn_mgr;
		dpu_child_connector_parse_dt(connector, node);
		conn_mgr->connector[chn_idx] = connector;
		dpu_pr_info("initailize connector pipe_sw_post_chn_idx=%u\n", chn_idx);
	}
}

static int32_t dpu_conn_manager_probe(struct platform_device *pdev)
{
	struct dpu_conn_manager *conn_mgr = NULL;
	struct dkmd_conn_handle_data *conn_ops = NULL;

	if (g_conn_manager != NULL) {
		dpu_pr_info("connector manager is ready!\n");
		return 0;
	}

	dpu_pr_info("conn manager probe v740!\n");
	conn_mgr = (struct dpu_conn_manager *)devm_kzalloc(&pdev->dev, sizeof(*conn_mgr), GFP_KERNEL);
	if (!conn_mgr) {
		dpu_pr_err("alloc dpu_conn_mgr failed!\n");
		return -ENOMEM;
	}
	conn_mgr->device = pdev;
	conn_mgr->parent_node = pdev->dev.of_node;

	if (dpu_conn_manager_parse_dt(conn_mgr, conn_mgr->parent_node) != 0) {
		dpu_pr_err("connector manager parse dts failed!\n");
		return -EINVAL;
	}
	dpu_conn_manager_parse_child_node(conn_mgr, conn_mgr->parent_node);
	platform_set_drvdata(pdev, conn_mgr);

	conn_ops = devm_kzalloc(&pdev->dev, sizeof(*conn_ops), GFP_KERNEL);
	if (!conn_ops) {
		dpu_pr_err("alloc conn_ops failed!\n");
		return -ENOMEM;
	}
	conn_ops->conn_info = NULL;
	conn_ops->on_func = connector_manager_on;
	conn_ops->off_func = connector_manager_off;
	conn_ops->ops_handle_func = connector_manager_ops_handle;
	/* add ops handle data to platform device */
	if (platform_device_add_data(pdev, conn_ops, sizeof(*conn_ops)) != 0) {
		dpu_pr_err("add conn_ops device data failed!\n");
		return -EINVAL;
	}
	g_conn_manager = conn_mgr;

	return 0;
}

static const struct of_device_id g_conn_manager_match_table[] = {
	{
		.compatible = "dkmd,connector",
		.data = NULL,
	},
	{},
};

static struct platform_driver g_conn_manager_driver = {
	.probe = dpu_conn_manager_probe,
	.remove = NULL,
	.driver = {
		.name = "dpu_connector_manager",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_conn_manager_match_table),
	}
};

static int32_t __init dpu_conn_manager_register(void)
{
	return platform_driver_register(&g_conn_manager_driver);
}

module_init(dpu_conn_manager_register);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Connector Manager Module Driver");
MODULE_LICENSE("GPL");
