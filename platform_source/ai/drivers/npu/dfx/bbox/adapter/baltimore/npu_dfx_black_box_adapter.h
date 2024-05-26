/*
 * npu_dfx_black_box_adapter.h
 *
 * about npu black box adapter
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __NPU_BLACK_BOX_ADAPTER_H
#define __NPU_BLACK_BOX_ADAPTER_H

#include <platform_include/basicplatform/linux/dfx_noc_modid_para.h>

#include "soc_actrl_interface.h"
#include "soc_pctrl_interface.h"
#include "soc_crgperiph_interface.h"
#include "soc_sctrl_interface.h"
#include "soc_mid.h"
#include "mntn_subtype_exception.h"
#include "bbox/npu_dfx_black_box.h"


#define PERI_CRG_BASE   (0xFFF05000)
#define ACTRL_BASE      (0xFA894000)
#define PCTRL_BASE      (0xFEC3E000)
#define SCTRL_BASE      (0xFA89B000)

static struct rdr_exception_info_s npu_rdr_excetption_info[] = {
	{
		.e_modid = (u32)EXC_TYPE_TS_AICORE_EXCEPTION,
		.e_modid_end = (u32)EXC_TYPE_TS_AICORE_EXCEPTION,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = AICORE_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "AICORE_EXCP",
	}, {
		.e_modid = (u32)EXC_TYPE_TS_AICORE_TIMEOUT,
		.e_modid_end = (u32)EXC_TYPE_TS_AICORE_TIMEOUT,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype  = AICORE_TIMEOUT,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "AICORE_TIMEOUT",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_TS_RUNNING_EXCEPTION,
		.e_modid_end = (u32)RDR_EXC_TYPE_TS_RUNNING_EXCEPTION,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = TS_RUNNING_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "TS_RUNNING_EXCP",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_TS_RUNNING_TIMEOUT,
		.e_modid_end = (u32)RDR_EXC_TYPE_TS_RUNNING_TIMEOUT,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = TS_RUNNING_TIMEOUT,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "TS_RUNNING_TIMEOUT",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_TS_INIT_EXCEPTION,
		.e_modid_end = (u32)RDR_EXC_TYPE_TS_INIT_EXCEPTION,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = TS_INIT_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "TS_INIT_EXCP",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NPU_POWERUP_FAIL,
		.e_modid_end = (u32)RDR_EXC_TYPE_NPU_POWERUP_FAIL,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = POWERUP_FAIL,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "POWERUP_FAIL",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NPU_POWERDOWN_FAIL,
		.e_modid_end = (u32)RDR_EXC_TYPE_NPU_POWERDOWN_FAIL,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = POWERDOWN_FAIL,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "POWERDOWN_FAIL",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU0,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU1,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU2,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU2,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_AICORE0_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_AICORE0_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU3,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU3,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_AICORE1_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_AICORE1_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU4,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU4,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_SDMA1_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_SDMA1_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU5,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU5,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_TS_CPU_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_TS_CPU_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU6,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU6,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_TS_HWTS_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_TS_HWTS_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NOC_NPU7,
		.e_modid_end = (u32)RDR_EXC_TYPE_NOC_NPU7,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask  = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = NPU_TCU_NOC_ERR,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "NPU_TCU_NOC_ERR",
	}, {
		.e_modid = (u32)RDR_EXC_TYPE_NPU_SMMU_EXCEPTION,
		.e_modid_end = (u32)RDR_EXC_TYPE_NPU_SMMU_EXCEPTION,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = SMMU_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "SMMU_EXCP",
	}, {
		.e_modid = (u32)RDR_TYPE_HWTS_BUS_ERROR,
		.e_modid_end = (u32)RDR_TYPE_HWTS_BUS_ERROR,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = HWTS_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "HWTS_EXCP",
	}, {
		.e_modid = (u32)RDR_TYPE_HWTS_EXCEPTION_ERROR,
		.e_modid_end = (u32)RDR_TYPE_HWTS_EXCEPTION_ERROR,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = HWTS_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "HWTS_EXCP",
	}, {
		.e_modid = (u32)EXC_TYPE_TS_SDMA_EXCEPTION,
		.e_modid_end = (u32)EXC_TYPE_TS_SDMA_TIMEOUT,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_NPU,
		.e_reset_core_mask = RDR_NPU,
		.e_from_core = RDR_NPU,
		.e_reentrant = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type = NPU_S_EXCEPTION,
		.e_exce_subtype = HWTS_EXCP,
		.e_upload_flag = (u32)RDR_UPLOAD_YES,
		.e_save_log_flags = RDR_SAVE_BL31_LOG,
		.e_from_module = "NPU",
		.e_desc = "SDMA_EXCP_OR_TIMEOUT",
	},
};

#ifdef CONFIG_NPU_NOC
static struct noc_err_para_s npu_noc_para[] = {
	{
		.masterid = (u32)SOC_NPU_AICORE0_MID,
		.targetflow = TARGET_FLOW_DEFAULT,
		.bus = 1, /* NOC_ERRBUS_NPU */
	}, {
		.masterid = (u32)SOC_NPU_AICORE1_MID,
		.targetflow = TARGET_FLOW_DEFAULT,
		.bus = 1, /* NOC_ERRBUS_NPU */
	}, {
		.masterid = (u32)SOC_NPU_SYSDMA_1_MID,
		.targetflow = TARGET_FLOW_DEFAULT,
		.bus = 1, /* NOC_ERRBUS_NPU */
	}, {
		.masterid = (u32)SOC_NPU_TS_0_MID,
		.targetflow = TARGET_FLOW_DEFAULT,
		.bus = 1, /* NOC_ERRBUS_NPU */
	}, {
		.masterid = (u32)SOC_NPU_TS_1_MID,
		.targetflow = TARGET_FLOW_DEFAULT,
		.bus = 1, /* NOC_ERRBUS_NPU */
	}, {
		.masterid = (u32)SOC_NPU_TCU_MID,
		.targetflow = TARGET_FLOW_DEFAULT,
		.bus = 1, /* NOC_ERRBUS_NPU */
	},
};

static u32 modid_array[] = {
	(u32)RDR_EXC_TYPE_NOC_NPU2, (u32)RDR_EXC_TYPE_NOC_NPU3,
	(u32)RDR_EXC_TYPE_NOC_NPU4, (u32)RDR_EXC_TYPE_NOC_NPU5,
	(u32)RDR_EXC_TYPE_NOC_NPU6, (u32)RDR_EXC_TYPE_NOC_NPU7
};
#endif

static struct npu_dump_offset pctrl_regs[] = {{SOC_PCTRL_PERI_CTRL2_ADDR(0), "PCTRL_PERI_CTRL2"}};

static struct npu_dump_offset actrl_regs[] = {
	{SOC_ACTRL_AO_MEM_CTRL10_ADDR(0),        "ACTRL_AO_MEM_CTRL10"},
	{SOC_ACTRL_ISO_EN_GROUP1_PERI_ADDR(0),   "ACTRL_ISO_EN_GROUP1"}
};

static struct npu_dump_offset peri_crg_regs[] = {
	{SOC_CRGPERIPH_PERSTAT0_ADDR(0),      "CRGPERIPH_PERSTAT0"},
	{SOC_CRGPERIPH_CLKDIV19_ADDR(0),      "CRGPERIPH_CLKDIV19"},
	{SOC_CRGPERIPH_PERSTAT7_ADDR(0),      "CRGPERIPH_PERSTAT7"},
	{SOC_CRGPERIPH_NPU_ADBLPCTRL_ADDR(0), "CRGPERIPH_NPU_ADBLPCTRL"},
	{SOC_CRGPERIPH_CLKDIV2_ADDR(0),       "CRGPERIPH_CLKDIV2"},
	{SOC_CRGPERIPH_CLKDIV7_ADDR(0),       "CRGPERIPH_CLKDIV7"},
	{SOC_CRGPERIPH_CLKDIV13_ADDR(0),      "CRGPERIPH_CLKDIV13"},
	{SOC_CRGPERIPH_CLKDIV15_ADDR(0),      "CRGPERIPH_CLKDIV15"},
	{SOC_CRGPERIPH_CLKDIV19_ADDR(0),      "CRGPERIPH_CLKDIV19"},
	{SOC_CRGPERIPH_CLKDIV26_ADDR(0),      "CRGPERIPH_CLKDIV26"},
	{SOC_CRGPERIPH_CLKDIV28_ADDR(0),      "CRGPERIPH_CLKDIV28"},
	{SOC_CRGPERIPH_CLKDIV32_ADDR(0),      "CRGPERIPH_CLKDIV32"},
};

static struct npu_dump_offset sctrl_regs[] = {
	{SOC_SCTRL_SC_NPU_LPCTRL0_SEC_ADDR(0),   "SCTRL_SC_NPU_LPCTRL0_SEC"},
	{SOC_SCTRL_SC_NPU_LPCTRL1_SEC_ADDR(0),   "SCTRL_SC_NPU_LPCTRL1_SEC"},
	{SOC_SCTRL_SC_NPU_LPCTRL2_SEC_ADDR(0),   "SCTRL_SC_NPU_LPCTRL2_SEC"}
};

static struct npu_dump_reg peri_regs[] = {
	{PCTRL_BASE, 0xFFF, pctrl_regs, ARRAY_SIZE(pctrl_regs)},
	{ACTRL_BASE, 0xFFF, actrl_regs, ARRAY_SIZE(actrl_regs)},
	{PERI_CRG_BASE, 0xFFF, peri_crg_regs, ARRAY_SIZE(peri_crg_regs)},
	{SCTRL_BASE, 0xFFF, sctrl_regs, ARRAY_SIZE(sctrl_regs)},
};

#endif /* __NPU_BLACK_BOX_ADAPTER_H */
