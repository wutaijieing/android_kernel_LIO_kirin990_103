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

#ifndef DPU_SCL_H
#define DPU_SCL_H

#include <linux/types.h>


uint32_t dpu_scl_alloc_dm_node(void);
void dpu_scl_free_dm_node(void);
uint8_t dpu_get_scl_idx(void);
bool is_arsr1_arsr0_pipe(void);
void dpu_arsr_cnt(void);
#endif /* DPU_SCL_H */
