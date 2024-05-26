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

#ifndef HISI_DISP_RM_OPERATOR_H
#define HISI_DISP_RM_OPERATOR_H


#include <linux/types.h>
#include "hisi_disp_rm.h"
#include "hisi_composer_operator.h"
#include "hisi_disp.h"

struct hisi_disp_resource *hisi_disp_rm_create_operator(union operator_id id, struct hisi_comp_operator *operator_data);
request_resource_cb hisi_disp_rm_get_request_func(uint32_t type);


#endif /* HISI_DISP_RM_H */
