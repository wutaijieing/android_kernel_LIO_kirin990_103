/*
 * npu_platform_register.h
 *
 * about npu platform register
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
#ifndef __NPU_PLATFORM_REGISTER_H
#define __NPU_PLATFORM_REGISTER_H

#include "soc_npu_ts_sysctrl_reg_offset.h"
#include "soc_npu_hwts_interface.h"
#include "soc_acpu_baseaddr_interface.h"
#include "soc_npu_aicore_interface.h"
#include "soc_npucrg_interface.h"
#include "soc_npu_sctrl_interface.h"
#include "soc_crgperiph_interface.h"
#include "soc_pcr_interface.h"
#include "soc_npu_aic_sysdma_reg_reg_offset.h"
#include "soc_npu_aic_sysdma_reg_c_union_define.h"
#include "soc_npu_sysdma_interface.h"
#include "soc_npu_noc_bus_interface.h"

#define NPU_SC_TESTREG0_OFFSET \
		(SOC_TS_SYSCTRL_SC_TESTREG0_REG - SOC_TS_SYSCTRL_BASE)

#define NPU_SC_SYSSTAT14_OFFSET \
		(SOC_TS_SYSCTRL_SC_SYSSTAT14_REG - SOC_TS_SYSCTRL_BASE)

#define soc_crgperiph_npucrg_stat_addr(base)   ((base) + 0x658UL)
#define SOC_CRGPERIPH_NPUCRG_IP_RST_BIT_OFFSET_START  12

#define SOC_ACPU_CRG_NPU_SIZE       0x1000

#define NPU_NPU2ACPU_HW_EXP_IRQ_0      67
#define NPU_NPU2ACPU_HW_EXP_IRQ_1      (NPU_NPU2ACPU_HW_EXP_IRQ_0 + 1)
#define NPU_NPU2ACPU_HW_EXP_IRQ_2      (NPU_NPU2ACPU_HW_EXP_IRQ_0 + 2)
#define NPU_NPU2ACPU_HW_EXP_IRQ_3      (NPU_NPU2ACPU_HW_EXP_IRQ_0 + 3)
#define NPU_NPU2ACPU_HW_EXP_IRQ_NS_0   (NPU_NPU2ACPU_HW_EXP_IRQ_0 + 4)
#define NPU_NPU2ACPU_HW_EXP_IRQ_NS_1   (NPU_NPU2ACPU_HW_EXP_IRQ_NS_0 + 1)
#define NPU_NPU2ACPU_HW_EXP_IRQ_NS_2   (NPU_NPU2ACPU_HW_EXP_IRQ_NS_0 + 2)
#define NPU_NPU2ACPU_HW_EXP_IRQ_NS_3   (NPU_NPU2ACPU_HW_EXP_IRQ_NS_0 + 3)

#endif
