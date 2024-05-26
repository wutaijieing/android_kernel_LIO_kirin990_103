/*
 *
 * hardware diaginfo record module.
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
#ifndef __DFX_HW_DIAG_H__
#define __DFX_HW_DIAG_H__

#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include <linux/spinlock_types.h>

struct dfx_diag_noc_info {
	char *init_flow;
	char *target_flow;
};

struct dfx_diag_panic_info {
	unsigned int cpu_num;
};

union dfx_hw_diag_info {
	struct dfx_diag_noc_info noc_info;
	struct dfx_diag_panic_info cpu_info;
};

struct dfx_hw_diag_dev {
	spinlock_t record_lock;
	struct dfx_hw_diag_trace *trace_addr;
	unsigned int trace_size;
	unsigned int trace_max_num;
};

#ifdef CONFIG_DFX_HW_DIAG
void dfx_hw_diaginfo_trace(unsigned int err_id, const union dfx_hw_diag_info *diaginfo);
void dfx_hw_diaginfo_record(const char *date);
void dfx_hw_diag_init(void);
#else
static inline void dfx_hw_diaginfo_trace(unsigned int err_id, const union dfx_hw_diag_info *diaginfo) { return; }
static inline void dfx_hw_diaginfo_record(const char *date) { return; }
static inline void dfx_hw_diag_init(void) { return; }
#endif

#endif
