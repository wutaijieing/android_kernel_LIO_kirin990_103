/*
 * npu_pm_config_tscpu.c
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

#include "npu_adapter.h"
#include "npu_atf_subsys.h"
#include "npu_proc_ctx.h"
#include "npu_stream.h"

int npu_tscpu_powerup(u32 work_mode, u32 subip_set, void **para)
{
	int ret;
	struct npu_dev_ctx *dev_ctx = NULL;
	struct npu_proc_ctx *curr_proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;

	unused(subip_set);
	dev_ctx = (struct npu_dev_ctx *)para;
#ifndef CONFIG_NPU_SWTS
	ret = npu_plat_powerup_till_ts(work_mode, 0);
	if (ret)
		return ret;
#endif
	list_for_each_entry_safe(curr_proc_ctx, next_proc_ctx, &dev_ctx->proc_ctx_list,
		dev_ctx_list) {
		npu_proc_clear_sqcq_info(curr_proc_ctx);
	}

	dev_ctx->ts_work_status = NPU_TS_WORK;
	/* sdma init */
	ret = npu_plat_init_sdma(work_mode);
	return ret;
}

int npu_tscpu_powerdown(u32 work_mode, u32 subip_set, void *para)
{
	struct npu_dev_ctx *dev_ctx = NULL;
	struct npu_proc_ctx *curr_proc_ctx = NULL;
	struct npu_proc_ctx *next_proc_ctx = NULL;

	unused(subip_set);
	dev_ctx = (struct npu_dev_ctx *)para;
	dev_ctx->ts_work_status = NPU_TS_DOWN;

	(void)npu_plat_powerdown_tbu(NPU_FLAGS_DISBALE_SYSDMA);
	list_for_each_entry_safe(curr_proc_ctx, next_proc_ctx, &dev_ctx->proc_ctx_list,
		dev_ctx_list) {
		npu_proc_clear_sqcq_info(curr_proc_ctx);
	}
	npu_recycle_rubbish_stream(dev_ctx);
#ifndef CONFIG_NPU_SWTS
	return npu_plat_powerdown_ts(0, work_mode);
#else
	return 0;
#endif
}

