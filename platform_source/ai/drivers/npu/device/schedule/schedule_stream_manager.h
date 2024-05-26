/*
 * schedule_stream_manager.h
 *
 * about schedule stream manager
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

#ifndef _SCHEDULE_STREAM_MANAGER_H_
#define _SCHEDULE_STREAM_MANAGER_H_

#include <linux/types.h>
#include "schedule_stream.h"

struct schedule_stream *sched_stream_mngr_get_stream(int32_t stream_id);

#endif
