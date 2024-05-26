/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_DRV_COMPOSER_H
#define HISI_DRV_COMPOSER_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp_panel.h"
#include "hisi_connector_utils.h"
#include "hisi_composer_core.h"
#include "hisi_dpu_module.h"
#include "operators/cld/dpu_disp_cld.h"
#include "operators/itfsw/hisi_operator_itfsw.h"
#include "dm/hisi_dm.h"

#include "isr/hisi_isr_utils.h"
#include "timeline/hisi_disp_timeline.h"
#include "hisi_disp_pm.h"
#include "dpu_offline_define.h"

#define HISI_OVERLAY_CONNECTOR_MAX 32
#define HISI_OVERLAY_NAME_LEN      25

enum {
	COMP_ISR_PRE = 0,
	COMP_ISR_POST,
	COMP_ISR_MAX
};

struct hisi_composer_ops {
	struct hisi_pipeline_ops base;
};

struct hisi_composer_data {
	uint32_t ov_id;
	uint32_t ov_caps;
	const char *ov_name;

	ulong regulator_bits;
	struct hisi_disp_pm_regulator* regulators[PM_REGULATOR_MAX];

	ulong clk_bits;
	struct hisi_disp_pm_clk* clks[PM_CLK_MAX];

	struct hisi_disp_isr isr_ctrl[COMP_ISR_MAX];
	char __iomem *ip_base_addrs[DISP_IP_MAX];

	/* other overlay data */
	struct hisi_panel_info *fix_panel_info;
	struct hisi_dm_param *dm_param;
	uint32_t scene_id;
	enum DPU_FORMAT layer_ov_format;

	struct dpu_offline_module offline_module_default;
	struct dpu_offline_module offline_module;
	struct offline_isr_info *offline_isr_info;
	struct disp_overlay_block ov_block_infos[23]; // HISI_DSS_OV_BLOCK_NUMS
	struct disp_rect *ov_block_rects[32]; // HISI_DSS_CMDLIST_BLOCK_MAX
	struct ov_req_infos ov_req; // block result
	struct semaphore ov_data_sem;
};

struct hisi_display_info {
	struct hisi_panel_info *fix_panel_info;

};

struct hisi_composer_connector_device {
	uint32_t next_id;

	struct hisi_panel_info *fix_panel_info;
	struct platform_device *next_pdev;
	struct hisi_pipeline_ops *next_dev_ops;
};

struct dpu_cld_data {
	struct dpu_cld_params cld_params;
};

struct hisi_itfsw_data {
	// TODO:add other info
	struct hisi_itfsw_params itfsw_params;
};

struct hisi_composer_device {
	uint32_t id;
	atomic_t ref_cnt;
	struct platform_device *pdev;
	struct hisi_disp_chrdev ov_chrdev;

	struct hisi_composer_data ov_data;
	struct hisi_composer_priv_ops ov_priv_ops;

	struct list_head pre_layer_buf_list;
	struct list_head post_wb_buf_list;

	/* link dm data cmdlist list header
	 * @client --> client
	 *                +-- cmd_header
	 *                +-- cmd_items
	 *                +-- cmd_items
	 */
	struct cmdlist_client *client;
	struct hisi_disp_timeline timeline;

	/* one overlay can connect multiply connector, such as
	 * Overlay1 ---->mipi1---->lcd1
	 *          |--->mipi2---->lcd2
	 */
	uint32_t next_dev_count;
	struct hisi_composer_connector_device *next_device[HISI_OVERLAY_CONNECTOR_MAX];

	struct hisi_display_frame frame;
};

int hisi_drv_primary_composer_create_device(void *connector_data);
void hisi_drv_primary_composer_register_connector(struct hisi_connector_device *connector_data);

int hisi_drv_external_composer_create_device(void *connector_data);
void hisi_drv_external_composer_register_connector(struct hisi_connector_device *connector_data);

int hisi_drv_tmp_fb2_create_device(void *connector_data);
void hisi_drv_tmp_fb2_register_connector(struct hisi_connector_device *connector_data);

#endif /* HISI_OVERLAY_H */
