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

#ifndef DKMD_OPR_ID_H
#define DKMD_OPR_ID_H

#include <linux/types.h>

enum {
	OPERATOR_NONE      = -1,
	OPERATOR_SDMA      = 0,
	OPERATOR_SROT,
	OPERATOR_HEMCD,
	OPERATOR_UVUP,
	OPERATOR_VSCF,
	OPERATOR_ARSR,
	OPERATOR_HDR,
	OPERATOR_CLD,
	OPERATOR_OV,
	OPERATOR_PRELOAD,
	OPERATOR_DPP,
	OPERATOR_DSC,
	OPERATOR_WCH,
	OPERATOR_ITFSW,
	OPERATOR_TYPE_MAX,
};

union dkmd_opr_id {
	int32_t id;
	struct {
		int32_t type : 8;
		int32_t ins : 8;
		int32_t dm_index : 4;
		int32_t rsv : 12;
	} info;
};

#endif