/*
 * npu_irq.c
 *
 * about npu irq
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
#include "npu_irq.h"
#include "npu_log.h"
#include "npu_feature.h"

int npu_plat_parse_irq(struct platform_device *pdev,
	struct npu_platform_info *plat_info)
{
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_EASC] == 1)
		plat_info->dts_info.irq_easc = platform_get_irq(pdev, NPU_IRQ_EASC);
#ifndef CONFIG_NPU_SWTS
	plat_info->dts_info.irq_cq_update = platform_get_irq(pdev,
		NPU_IRQ_CALC_CQ_UPDATE0);
	cond_return_error(plat_info->dts_info.irq_cq_update < 0,
		-EINVAL, "get cq update irq fail\n");

	plat_info->dts_info.irq_dfx_cq = platform_get_irq(pdev,
		NPU_IRQ_DFX_CQ_UPDATE);
	cond_return_error(plat_info->dts_info.irq_dfx_cq < 0,
		-EINVAL, "get dfx cq update irq fail\n");

	plat_info->dts_info.irq_mailbox_ack = platform_get_irq(pdev,
		NPU_IRQ_MAILBOX_ACK);
	cond_return_error(plat_info->dts_info.irq_mailbox_ack < 0,
		-EINVAL, "get mailbox irq fail\n");

#ifdef CONFIG_NPU_INTR_HUB
	plat_info->dts_info.irq_intr_hub = platform_get_irq(pdev,
		NPU_IRQ_INTR_HUB);
	cond_return_error(plat_info->dts_info.irq_intr_hub < 0,
		-EINVAL, "get intr hub irq fail\n");
#endif
	npu_drv_debug("calc_cq_update0=%d, dfx_cq_update=%d, mailbox_ack=%d\n",
		plat_info->dts_info.irq_cq_update,
		plat_info->dts_info.irq_dfx_cq,
		plat_info->dts_info.irq_mailbox_ack);
#else
	plat_info->dts_info.irq_swts_hwts_normal = platform_get_irq(pdev,
		NPU_IRQ_HWTS_NORMAL_NS);
	plat_info->dts_info.irq_swts_hwts_error = platform_get_irq(pdev,
		NPU_IRQ_HWTS_ERROR_NS);
	plat_info->dts_info.irq_dmmu = platform_get_irq(pdev,
		NPU_IRQ_DMMU);
	npu_drv_debug("hwts_normal_ns=%d, hwts_error_ns=%d, dmmu=%d\n",
		plat_info->dts_info.irq_swts_hwts_normal,
		plat_info->dts_info.irq_swts_hwts_error,
		plat_info->dts_info.irq_dmmu);
#endif
	if (plat_info->dts_info.feature_switch[NPU_FEATURE_EASC] == 1) {
		plat_info->dts_info.irq_easc = platform_get_irq(pdev, NPU_IRQ_EASC);
		cond_return_error(plat_info->dts_info.irq_easc < 0,
			-EINVAL, "get easc irq fail\n");
		npu_drv_debug("easc=%d\n", plat_info->dts_info.irq_easc);
	}
	return 0;
}
