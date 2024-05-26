/* Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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

#ifndef __DPU_DP_H__
#define __DPU_DP_H__

#include <linux/platform_drivers/usb/tca.h>

#if defined(CONFIG_HISI_DSS_V510) || defined(CONFIG_HISI_DSS_V600) || \
	defined(CONFIG_DPU_DP_EN) || defined(CONFIG_DKMD_DPU_DP)
extern int dpu_dptx_hpd_trigger(TCA_IRQ_TYPE_E irq_type,
	TCPC_MUX_CTRL_TYPE mode, TYPEC_PLUG_ORIEN_E typec_orien);
extern int dpu_dptx_notify_switch(void);
extern bool dpu_dptx_ready(void);
extern int dpu_dptx_triger(bool benable);
#else
static inline int dpu_dptx_hpd_trigger(TCA_IRQ_TYPE_E irq_type,
	TCPC_MUX_CTRL_TYPE mode, TYPEC_PLUG_ORIEN_E typec_orien)
{
	return 0;
}
static inline int dpu_dptx_notify_switch(void)
{
	return 0;
}
static inline bool dpu_dptx_ready(void)
{
	return true;
}
static inline int dpu_dptx_triger(bool benable)
{
	return 0;
}
#endif

#endif /* DPU_DP_H */
