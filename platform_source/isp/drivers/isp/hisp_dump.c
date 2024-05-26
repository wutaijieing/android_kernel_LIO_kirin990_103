/*
 * driver, hisp_dsm.c
 *
 * Copyright (c) 2013- ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include "hisp_internel.h"
#include "isp_ddr_map.h"

#define MAX_RESULT_LENGTH                   (8)

#define ISPCPU_STONE_OFFSET                 (0xF00)

#define ISPCPU_EXC_TYPE_MAX                 6
#define ISPCPU_EXC_INFO_MAX                 30

/* CRG Regs Offset */
#define IP_RST_ISPCPU                       (1 << 4)
#define IP_RST_ISPCORE                      (1 << 0)

struct hisp_ispcpu_stone_s {
	unsigned int type; /**< EXCPTION TYPE                      */
	unsigned int uw_cpsr; /**< Current program status register (CPSR)     */
	unsigned int uw_r0; /**< Register R0                        */
	unsigned int uw_r1; /**< Register R1                        */
	unsigned int uw_r2; /**< Register R2                        */
	unsigned int uw_r3; /**< Register R3                        */
	unsigned int uw_r4; /**< Register R4                        */
	unsigned int uw_r5; /**< Register R5                        */
	unsigned int uw_r6; /**< Register R6                        */
	unsigned int uw_r7; /**< Register R7                        */
	unsigned int uw_r8; /**< Register R8                        */
	unsigned int uw_r9; /**< Register R9                        */
	unsigned int uw_r10; /**< Register R10                      */
	unsigned int uw_r11; /**< Register R11                      */
	unsigned int uw_r12; /**< Register R12                      */
	unsigned int uw_sp; /**< Stack pointer                      */
	unsigned int uw_lr; /**< Program returning address          */
	unsigned int uw_pc; /**< PC pointer of the exceptional function    */
	unsigned int dfsr; /**< DataAbort STATE      -- DFSR       */
	unsigned int dfar; /**< DataAbort ADDR       -- DFAR       */
	unsigned int ifsr; /**< PrefetchAbort STATE  -- IFSR       */
	unsigned int ifar; /**< PrefetchAbort ADDR   -- IFAR       */
	unsigned int smmu_far_low;  /**< SMMU500 ADDR -- FAR       */
	unsigned int smmu_far_high; /**< SMMU500 ADDR -- FAR       */
	unsigned int smmu_fsr; /**< SMMU500 STATE    -- FSR        */
	unsigned int smmu_fsynr0; /**< SMMU500 STATE -- FSYNR      */
};

struct isp_cpu_dump_s {
	unsigned int reg_addr;
	unsigned int actual_value;
	unsigned int expected_value;
	unsigned int care_bits;
	unsigned int compare_result;
	char result[MAX_RESULT_LENGTH];
};

struct isp_nsec_cpu_dump_s {
	unsigned int reg_addr;
	unsigned int actual_value;
	unsigned int expected_value;
	unsigned int care_bits;
	unsigned int compare_result;
	char result[MAX_RESULT_LENGTH];
};

#ifdef DEBUG_HISP
static char ispcpu_exc_type[ISPCPU_EXC_TYPE_MAX][ISPCPU_EXC_INFO_MAX] = {
	{"NULL"},
	{"EXCEPT_UNDEF_INSTR"},
	{"EXCEPT_SWI"},
	{"EXCEPT_PREFETCH_ABORT"},
	{"EXCEPT_DATA_ABORT"},
	{"EXCEPT_FIQ"},
};
#endif

/*lint -e559 */
static void hisp_dump_boot_stone(void)
{
	struct hisp_shared_para *param = NULL;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param == NULL) {
		pr_err("[%s] Failed : hisp_share_get_para.%pK\n",
			__func__, param);
		hisp_unlock_sharedbuf();
		return;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
	pr_err("[%s]: FIRMWARE.0x%x, OS.0x%x\n", __func__,
		param->bw_stat, param->fw_stat);
#pragma GCC diagnostic pop
	hisp_unlock_sharedbuf();
}
/*lint +e559 */

static void hisp_dump_ispcpu_stone(void)
{
#ifdef DEBUG_HISP
	struct hisp_shared_para *param = NULL;
	struct hisp_ispcpu_stone_s *cpu_info = NULL;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param == NULL) {
		pr_err("[%s] err:hisp_share_get_para.%pK\n", __func__, param);
		hisp_unlock_sharedbuf();
		return;
	}
	cpu_info = (struct hisp_ispcpu_stone_s *)
			((unsigned char *)param + ISPCPU_STONE_OFFSET);
	if (cpu_info->type <= ISPCPU_EXC_TYPE_MAX)
		pr_info("uwExcType.%x : %s\n", cpu_info->type,
			ispcpu_exc_type[cpu_info->type]);
	pr_info("[%s] +", __func__);
	pr_info("R0    %08x        R8    %08x\n",
		cpu_info->uw_r0, cpu_info->uw_r8);
	pr_info("R1    %08x        R9    %08x\n",
		cpu_info->uw_r1, cpu_info->uw_r9);
	pr_info("R2    %08x        R10   %08x\n",
		cpu_info->uw_r2, cpu_info->uw_r10);
	pr_info("R3    %08x        R11   %08x\n",
		cpu_info->uw_r3, cpu_info->uw_r11);
	pr_info("R4    %08x        R12   %08x\n",
		cpu_info->uw_r4, cpu_info->uw_r12);
	pr_info("R5    %08x        SP    %08x\n",
		cpu_info->uw_r5, cpu_info->uw_sp);
	pr_info("R6    %08x        LR    %08x\n",
		cpu_info->uw_r6, cpu_info->uw_lr);
	pr_info("R7    %08x        PC    %08x\n",
		cpu_info->uw_r7, cpu_info->uw_pc);
	pr_info("SPSR  %08x        CPSR  %08x\n",
		cpu_info->uw_cpsr, cpu_info->uw_cpsr);
	pr_info("DFSR  %08x        DFAR  %08x\n",
		cpu_info->dfsr, cpu_info->dfar);
	pr_info("IFSR  %08x        IFAR  %08x\n",
		cpu_info->ifsr, cpu_info->ifar);
	pr_info("SMMU_FAR_H %08x   SMMU_FAR_L  %08x\n",
		cpu_info->smmu_far_high, cpu_info->smmu_far_low);
	pr_info("SMMU_FSR   %08x   SMMU_FSYNR  %08x\n",
		cpu_info->smmu_fsr, cpu_info->smmu_fsynr0);
	hisp_unlock_sharedbuf();
#endif
}

void hisp_boot_stat_dump(void)
{
	pr_alert("[%s] +\n", __func__);

	hisp_dump_boot_stone();
	if (hisp_sec_boot_mode()) {
		return;
	} else if (hisp_nsec_boot_mode()) {
		hisp_dump_ispcpu_stone();
	}

	pr_alert("[%s] -\n", __func__);
}

int hisp_mntn_dumpregs(void)
{
#ifdef DEBUG_HISP
	int ret;
	unsigned int i = 0;
	unsigned int ispcpu_dumpnum = 0;
	struct isp_nsec_cpu_dump_s *isp_cpu_cfg = NULL;
	struct isp_nsec_cpu_dump_s *isp_cpu_stat_dump = NULL;

	ret = hisp_smc_cpuinfo_dump(hisp_get_param_info_pa());
	if (ret < 0) {
		pr_err("[%s] hisp_smc_cpuinfo_dump.%d\n", __func__, ret);
		return ret;
	}

	isp_cpu_cfg = (struct isp_nsec_cpu_dump_s *)hisp_get_param_info_va();
	if (isp_cpu_cfg == NULL) {
		pr_err("[%s] isp_cpu_cfg is NULL!\n", __func__);
		return -ENODEV;
	}

	ispcpu_dumpnum = isp_cpu_cfg->reg_addr;
	pr_info("|  reg_addr  |  ac_value  |  ex_value  |  care bit  | res\n");
	for (i = 1; i < ispcpu_dumpnum; i++) {
		isp_cpu_stat_dump = isp_cpu_cfg + i;
		pr_info("| 0x%08x | 0x%08x | 0x%08x | 0x%08x | 0x%08x | %s\n",
			isp_cpu_stat_dump->reg_addr,
			isp_cpu_stat_dump->actual_value,
			isp_cpu_stat_dump->expected_value,
			isp_cpu_stat_dump->care_bits,
			isp_cpu_stat_dump->compare_result,
			isp_cpu_stat_dump->result);
	}

	isp_cpu_stat_dump = isp_cpu_cfg;
	pr_info("ISP CPU PC 0 : 0x%x\n", isp_cpu_stat_dump->actual_value);
	pr_info("ISP CPU PC 1 : 0x%x\n", isp_cpu_stat_dump->expected_value);
	pr_info("ISP CPU PC 2 : 0x%x\n", isp_cpu_stat_dump->care_bits);
#endif
	return 0;
}
//lint -restore
