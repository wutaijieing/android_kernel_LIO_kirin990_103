/*
 * npu_atf_subsys.c
 *
 * about npu atf subsys
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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
#ifndef _NPU_ATF_H_
#define _NPU_ATF_H_
#include "npu_common.h"

/* 1dd must be the same */
#define NPU_SLV_SECMODE                          0xC501DD00
#define NPU_MST_SECMODE                          0xC501DD01
#define NPU_START_SECMODE                        0xC501DD02
#define NPU_ENABLE_SECMODE                       0xC501DD03
#define GIC_CFG_CHECK_SECMODE                    0xC501DD04
#define GIC_ONLINE_READY_SECMODE                 0xC501DD05
#define NPU_CPU_POWER_DOWN_SECMODE               0xC501DD06
#define NPU_INFORM_POWER_DOWN_SECMODE            0xC501DD07
#define NPU_POWER_DOWN_TS_SEC_REG                0xC501DD08
#define NPU_SMMU_TCU_INIT_NS                     0xC501DD09
#define NPU_SMMU_TCU_CACHE_INIT                  0xC501DD0A
#define NPU_SMMU_TCU_DISABLE                     0xC501DD0B
#define NPU_POWER_UP_SMMU_TBU                    0xC501DD0C
#define NPU_POWER_DOWN_SMMU_TBU                  0xC501DD0D
#define NPU_LOWERPOWER_OPS                       0xC501DD0E
#define NPU_SWITCH_HWTS_AICORE_POOL              0xC501DD0F
#define NPU_POWER_UP_TOP_SPECIFY                 0xC501DD10
#define NPU_POWER_DOWN_TOP_SPECIFY               0xC501DD11
#define NPU_POWER_UP_NON_TOP_COMMON              0xC501DD12
#define NPU_POWER_DOWN_NON_TOP_COMMON            0xC501DD13
#define NPU_POWER_UP_NON_TOP_SPECIFY             0xC501DD14
#define NPU_POWER_DOWN_NON_TOP_SPECIFY           0xC501DD15
#define NPU_EXCEPTION_PROC                       0xC501DD16
#define NPU_PROFILING_SETTING                    0xC501DD17
#define NPU_POWER_UP_NON_TOP_SPECIFY_NOSEC       0xC501DD18
#define NPU_POWER_DOWN_NON_TOP_SPECIFY_NOSEC     0xC501DD19

/* smc command flags[0-31]: x3 */
#define NPU_FLAGS_POWER_ON                  (0x1 << 0) /* bit0: npu subsys poweron/off */
#define NPU_FLAGS_POWER_OFF                 (0x1 << 1) /* bit1: npu subsys poweron/off */
#define NPU_FLAGS_PWONOFF_AIC0              (0x1 << 2) /* bit2: core0 poweron/off */
#define NPU_FLAGS_PWONOFF_AIC1              (0x1 << 3) /* bit3: core1 poweron/off */
#define NPU_FLAGS_PWONOFF_AIC0_1            (0x3 << 2) /* bit2&bit3: core0&core1 poweron/off */
#define NPU_FLAGS_INIT_SYSDMA               (0x1 << 4) /* bit4: sysdma init */
#define NPU_FLAGS_DISBALE_SYSDMA            (0x1 << 5) /* bit5: sysdma disable */

typedef union {
	uint64_t value;
	struct {
		uint64_t work_mode : 16;
		uint64_t work_mode_flags : 16; /* for dfx */
		uint64_t aic_status : 16; /* for soc quality improvement */
		uint64_t power_status : 1; /* 0: power off; 1: power on */
		uint64_t reserved : 15;
	} info;
} npu_atf_hwts_aic_pool_switch;

enum npu_atf_error_code {
	NPU_ATF_SUCC = 0,
	NPU_ATF_MULTICHIP_GIC_ONLINE_FAILED  = 1,
	NPU_ATF_MULTICHIP_GIC_RWP_FAILED = 2,
	NPU_ATF_MULTICHIP_GIC_PUP_FAILED = 4, // Power update in Progress check
	NPU_ATF_MULTICHIP_GIC_RTS_FAILED = 8, // Route Table Status check
};

enum npu_gic_chipid {
	NPU_GIC_1  = 1
};

enum {
	HW_IRQ_LEVEL_TRIGGER_TYPE = 0x0,
	HW_IRQ_EDGE_TRIGGER_TYPE = 0x1
};

enum npuip {
	NPUIP_NONE = 0,
	NPUIP_NPU_SUBSYS,
	NPUIP_NPU_BUS,
	NPUIP_TS_SUBSYS,
	NPUIP_AICORE,
	NPUIP_NPUCPU_SUBSYS,
	NPUIP_SDMA,
	NPUIP_TCU,
	NPUIP_MAX
};

enum npu_gic_cmd {
	NPU_GIC_POWER_OFF_INFORM = 0,
	TSCPU_GIC_WAKER_OPS_READY = 100,
	TSCPU_GIC_WAKER_OPS_DONE = 200,
	TSCPU_GIC_PWRR_OPS_READY = 300,
	TSCPU_GIC_PWRR_OPS_DONE = 400,
};

enum npu_lowpower_dev_target {
	NPU_LOWPOWER_PCR = 0,
	NPU_LOWPOWER_MAX
};

enum npu_lowpower_flag {
	NPU_LOWPOWER_FLAG_ENABLE = 0,
	NPU_LOWPOWER_FLAG_DISABLE,
	NPU_LOWPOWER_FLAG_MAX
};

enum npu_excp_dev_target {
	NPU_EXCP_EASC = 0,
	NPU_EXCP_MAX,
};

int atfd_service_npu_smc(u64 _service_id, u64 _cmd, u64 _arg1, u64 _arg2);

int npuatf_start_secmod(u64 work_mode, u64 canary);

int npuatf_enable_secmod(u64 cmd);

int npuatf_powerup_aicore(u64 secure_mode, u32 aic_flag);

int npuatf_power_down_aicore(u64 work_mode, u32 aic_flag);

int npuatf_power_down_tcu(void);

int npuatf_power_down(u32 mode);

int atf_query_gic0_state(u64 cmd);

int acpu_gic0_online_ready(u64 cmd);

int npuatf_power_down_ts_secreg(u32 work_mode);

int npuatf_inform_power_down(int gic_cmd);

int npuatf_enable_tbu(u64 cmd);

int npuatf_disable_tbu(u64 cmd, u32 aic_flag);

int npuatf_switch_hwts_aicore_pool(u64 aic_pool_switch_info);

int npu_init_sdma_tbu(u64 secure_mode, u32 flag);

int npuatf_lowpower_ops(u64 lowpower_dev, u64 lowpower_flag);

int npuatf_aicore_pmu_set(void);
#endif
