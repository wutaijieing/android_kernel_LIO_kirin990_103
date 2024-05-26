/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include "opr_cmd_data_interface.h"
#include <linux/slab.h>
#include <dpu/dpu_dm.h>
#include "sdma/sdma_cmd_data.h"
#include "ov/ov_cmd_data.h"
#include "dpp/dpp_cmd_data.h"
#include "itfsw/itfsw_cmd_data.h"
#include "dkmd_log.h"

struct opr_cmd_datas {
	init_cmd_data_func init_func;
	struct opr_cmd_data **cmd_datas;
	uint32_t size;
};

static struct opr_cmd_datas g_oprs_cmd_data[OPERATOR_TYPE_MAX] = {
	{init_sdma_cmd_data,  NULL, OPR_SDMA_NUM},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{init_ov_cmd_data,    NULL, OPR_OV_NUM},
	{NULL,                NULL, 0},
	{init_dpp_cmd_data,   NULL, OPR_DPP_NUM},
	{NULL,                NULL, 0},
	{NULL,                NULL, 0},
	{init_itfsw_cmd_data, NULL, OPR_ITFSW_NUM},
};

struct opr_cmd_data *get_opr_cmd_data(union dkmd_opr_id opr_id)
{
	if (unlikely(opr_id.info.type >= OPERATOR_TYPE_MAX)) {
		dpu_pr_debug("opr_id=%#x is invalid", opr_id.id);
		return NULL;
	}

	dpu_pr_debug("opr=%#x, support opr cmd_data size=%u", opr_id.id, g_oprs_cmd_data[opr_id.info.type].size);
	if (unlikely(opr_id.info.ins >= g_oprs_cmd_data[opr_id.info.type].size)) {
		dpu_pr_err("opr is not support");
		return NULL;
	}

	if (unlikely(!g_oprs_cmd_data[opr_id.info.type].cmd_datas))
		return NULL;

	return g_oprs_cmd_data[opr_id.info.type].cmd_datas[opr_id.info.ins];
}

void init_opr_cmd_data(void)
{
	union dkmd_opr_id opr_id;
	uint32_t i;
	uint32_t j;

	for (i = 0; i < OPERATOR_TYPE_MAX; ++i) {
		if (g_oprs_cmd_data[i].size == 0)
			continue;

		g_oprs_cmd_data[i].cmd_datas = (struct opr_cmd_data **)kzalloc(g_oprs_cmd_data[i].size *
			sizeof(struct opr_cmd_data *), GFP_KERNEL);
		if (unlikely(!g_oprs_cmd_data[i].cmd_datas)) {
			dpu_pr_err("alloc opr cmd_datas %d mem fail", i);
			continue;
		}

		opr_id.id = 0;
		opr_id.info.type = (int32_t)i;
		for (j = 0; j < g_oprs_cmd_data[i].size; ++j) {
			opr_id.info.ins = (int32_t)j;
			g_oprs_cmd_data[i].cmd_datas[j] = g_oprs_cmd_data[i].init_func(opr_id);
			if (unlikely(!g_oprs_cmd_data[i].cmd_datas[j]))
				dpu_pr_err("alloc cmd_data %d mem fail", j);
		}
	}
}

void deinit_opr_cmd_data(void)
{
	uint32_t i;
	uint32_t j;

	for (i = 0; i < OPERATOR_TYPE_MAX; ++i) {
		if (g_oprs_cmd_data[i].size == 0)
			continue;

		if (unlikely(!g_oprs_cmd_data[i].cmd_datas))
			continue;

		for (j = 0; j < g_oprs_cmd_data[i].size; ++j) {
			if (unlikely(!g_oprs_cmd_data[i].cmd_datas[j]))
				continue;
			kfree(g_oprs_cmd_data[i].cmd_datas[j]->data);
			kfree(g_oprs_cmd_data[i].cmd_datas[j]);
		}
		kfree(g_oprs_cmd_data[i].cmd_datas);
	}
}
