/*
 * hwts_driver.h
 *
 * about hwts driver
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _HWTS_DRIVER_H_
#define _HWTS_DRIVER_H_

#include <linux/types.h>
#include "soc_npu_hwts_interface.h"
#include "soc_acpu_baseaddr_interface.h"
#include "hwts_sq_cq.h"

void hwts_driver_schedule_sq(struct hwts_sq_cq *sqcq_info, uint16_t channel_id);
void hwts_driver_reset_cq_head(uint16_t cq_idx);
void hwts_driver_clear_channel_sq_en(uint16_t channel_id);
void hwts_driver_reset_event_table(uint16_t event_id);
#endif