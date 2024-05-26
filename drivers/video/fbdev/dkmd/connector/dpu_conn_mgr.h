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

#ifndef DPU_CONN_MGR_H
#define DPU_CONN_MGR_H

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/clk.h>
#include <linux/genalloc.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <dpu/soc_dpu_define.h>

#include "dpu_connector.h"

struct dpu_conn_manager {
	/* private device data */
	struct platform_device *device;

	/* Through this node access to public resources */
	struct device_node *parent_node;

	/* Base address register, power supply, clock is public */
	char __iomem *dpu_base;
	char __iomem *peri_crg_base;
	char __iomem *pctrl_base;

	struct regulator *disp_ch1subsys_regulator;
	struct regulator *dsssubsys_regulator;
	struct regulator *vivobus_regulator;
	struct regulator *media1_subsys_regulator;

	struct regulator *dsisubsys_regulator;
	struct regulator *dpsubsys_regulator;
	struct regulator *dss_lite1_regulator;

	struct clk *clk_gate_edc;

	/* Receive each connector registration */
	struct dpu_connector *connector[PIPE_SW_POST_CH_MAX];
};

extern struct dpu_conn_manager *g_conn_manager;

static inline struct dpu_connector *get_primary_connector(struct dkmd_connector_info *conn_info)
{
	uint32_t sw_post_chn_idx = conn_info->sw_post_chn_idx[PRIMARY_CONNECT_CHN_IDX];

	return g_conn_manager->connector[sw_post_chn_idx % PIPE_SW_POST_CH_MAX];
}

static inline struct dpu_connector *get_external_connector(struct dkmd_connector_info *conn_info)
{
	uint32_t sw_post_chn_idx = conn_info->sw_post_chn_idx[EXTERNAL_CONNECT_CHN_IDX];

	return g_conn_manager->connector[sw_post_chn_idx % PIPE_SW_POST_CH_MAX];
}

static inline bool is_connector_manager_available(void)
{
	return (g_conn_manager != NULL);
}

#endif
