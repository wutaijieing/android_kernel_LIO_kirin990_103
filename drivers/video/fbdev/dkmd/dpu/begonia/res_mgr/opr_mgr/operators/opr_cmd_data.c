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

#include <dpu/soc_dpu_define.h>
#include "opr_cmd_data.h"
#include "dkmd_log.h"

int32_t opr_dpu_to_soc_type(int32_t opr_dpu_type)
{
	switch (opr_dpu_type) {
	case OPERATOR_SDMA:
		return OPR_SDMA;
	case OPERATOR_SROT:
		return OPR_SROT;
	case OPERATOR_HEMCD:
		return OPR_HEMCD;
	case OPERATOR_UVUP:
		return OPR_UVUP;
	case OPERATOR_VSCF:
	case OPERATOR_ARSR:
		return OPR_SCL;
	case OPERATOR_HDR:
		return OPR_HDR;
	case OPERATOR_CLD:
		return OPR_CLD;
	case OPERATOR_OV:
		return OPR_OV;
	case OPERATOR_PRELOAD:
		return OPR_PRELOAD;
	case OPERATOR_DPP:
		return OPR_DPP;
	case OPERATOR_DSC:
		return OPR_DSC;
	case OPERATOR_WCH:
		return OPR_WCH;
	case OPERATOR_ITFSW:
		return OPR_ITFSW;
	default:
		return -1;
	}
}

void set_common_cmd_data(struct opr_cmd_data *cmd_data, const struct opr_cmd_data *pre_cmd_data)
{
	dpu_assert(!cmd_data);

	if (pre_cmd_data) {
		cmd_data->data->in_common_data = pre_cmd_data->data->out_common_data;
		cmd_data->data->out_common_data = cmd_data->data->in_common_data;
	}
}