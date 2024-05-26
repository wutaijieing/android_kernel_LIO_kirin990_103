/*
 * npu_dpm_v3.c
 *
 * about npu dpm
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
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
#include "npu_dpm_v3.h"
#include "npu_dpm.h"
#include <dpm_hwmon.h>

/* only NPU_AICORE_0 DPM */
#define NPU_DPM_NUM   1
#define DPM_ENERGY_NUM       1
#define NPU_DPM_COUNTER_PMU  24
#define NPU_DPM_COUNTER_ICG  40
#define VOLT_GEAR_NUM       4
#define STA_VOLT_PARAM_NUM  8
#define FX2_CONST_NUM       4
#define FX2_VAR1_NUM        4
#define FX2_VAR2_NUM        4
#define SIG_POWER_PARAM_NUM 60
#define SOC_NPU_PCR_SIZE            0x1000
#define SOC_NPU_DPM_MAP_SIZE        0x2000
#define SOC_ACPU_PERI_CRG_SIZE      0x1000
#define STATIC_VOLT_PARAM_A 0x5
#define STATIC_VOLT_PARAM_B 0x12C
#define STATIC_VOLT_GEAR 0x50505050
#define STATIC_VOLT_PARAM 0x00400400
#define STATIC_FX2_CONST 0x0
#define STATIC_FX2_VAR1 0x00800080
#define STATIC_FX2_VAR2 0x00800080
#define STATIC_SIG_POWER_PARAM 0x00400400
#define SDPM_ENABLE_ALL 0xFFFFFFFF
#define SG_DDPM_ENABLE_ALL 0x00FF00FF
#define NPU_DP_MONITOR_CTRL_ENABLE_ALL 0xFFFFFFFF
#define NPU_DP_MONITOR_CTRL_DISABLE_ALL 0xFFFF0000
#define NPU_DP_MONITOR_MASK_SHITFT 16

/* Reading dpm data and powering on/off the dpm device are asynchronous.
   In order to avoid accessing the power-off area, a mutex lock is required. */
static DEFINE_MUTEX(dpm_npu_lock);

/* dpm_V3 deleted dpm_power_table and divided it into dyn_power_table
   (dynamic power consumption)  and total_power_table (total power consumption),
   total power consumption = static power consumption + dynamic power consumption) */
static struct dpm_hwmon_ops npu_dpm_ops_v3 = {
	/* the unique identifier of the module */
	.dpm_module_id = DPM_NPU_ID,
	/* module type: (GPU, NPU or PERI) */
	.dpm_type = DPM_NPU_MODULE,
#ifndef CONFIG_DPM_HWMON_PWRSTAT
	/* Length of data read by the DPM, which is related to the number of DPMs and the
	   number of DPM_ENERGYs. */
	.dpm_power_len = DPM_ENERGY_NUM * NPU_DPM_NUM,
	/* save dpm_power to dyn_power_table[0]
	   save SUM(dpm_power, dyn_power) to total_power_table[0] */
	.hi_dpm_update_power = npu_dpm_update_energy,
#ifdef CONFIG_DPM_HWMON_DEBUG
	.dpm_cnt_len = NPU_DPM_NUM * (NPU_DPM_COUNTER_PMU + NPU_DPM_COUNTER_ICG),
#endif
#endif

#ifdef CONFIG_DPM_HWMON_DEBUG
	/* Save the data reported by the counter to dpm_counter_table[]. There are two modes for the
	   input parameter mode, which correspond to non-accumulation (BUSNESS_FIT, 0) and accumulation
	   (DATAREPORTING, 1). The saving order of dpm_counter_table[] is to save pmu_counter of all
	   channels of npu first, and then save icg_counter of all channels of npu. There are two reading
	   methods for pmu_counter and icg_counter, which can be controlled by the macro
	   CONFIG_DPM_HWMON_PWRSTAT to use powerstat to read or apb to read. */
	.hi_dpm_get_counter_for_fitting = npu_dpm_get_counter_for_fitting,
#endif
};

/* npu dpm and pcr related */
static void __iomem *acpu_npu_crg_base_vir_addr = NULL;
static void __iomem *acpu_npu_pcr_base_vir_addr = NULL;
static void __iomem *acpu_npu_dpm_base_vir_addr = NULL;

bool npu_dpm_enable_flag(void)
{
	return npu_dpm_ops_v3.module_enabled;
}

#ifdef CONFIG_DPM_HWMON_DEBUG
int npu_dpm_update_counter(int mode)
{
	uint32_t val;
	uint32_t count_reg_num_updated = 0;
	struct npu_dev_ctx *dev_ctx = NULL;

	/* default read strategy is powerstat, if apb, should change to apb */
#ifdef CONFIG_DPM_HWMON_PWRSTAT
	const uint32_t apb_read_cnt_en = 0;
#else
	const uint32_t apb_read_cnt_en = 1;
#endif

	/* dubai disable npu dpm, return 0 */
	if (npu_dpm_enable_flag() == false)
		return 0;

#ifndef CONFIG_DPM_HWMON_PWRSTAT
	if (npu_dpm_ops_v3.dpm_counter_table == NULL) {
		npu_drv_err("npu_dpm_ops_v3.dpm_counter_table is null");
		return 0;
	}
#endif
	dev_ctx = get_dev_ctx_by_id(0);
	if (dev_ctx == NULL) {
		npu_drv_err("get current device failed");
		return 0;
	}

	if (atomic_read(&dev_ctx->power_access) == 0) {
		mutex_lock(&dpm_npu_lock);
		/* trigger pmu soft cnt pulse, pwrstat and icg is seperated by MARCO, NO support to change
		   BETWEEN pwrstat and icg!!! */
		/* default read strategy is powerstat, use default value */
		SOC_DP_MONITOR_DPM_CTRL_EN_UNION dp_monitor_ctrl_en = {0};
		dp_monitor_ctrl_en.reg.pmu_counter_en   = 1;
		dp_monitor_ctrl_en.reg.apb_read_cnt_en  = apb_read_cnt_en;
		dp_monitor_ctrl_en.value |= (1 << SOC_DP_MONITOR_DPM_CTRL_EN_pmu_counter_en_START)<< NPU_DP_MONITOR_MASK_SHITFT;
		dp_monitor_ctrl_en.value |= (1 << SOC_DP_MONITOR_DPM_CTRL_EN_apb_read_cnt_en_START) << NPU_DP_MONITOR_MASK_SHITFT;
		writel(0xFFFF0014, SOC_DP_MONITOR_DPM_CTRL_EN_ADDR(acpu_npu_dpm_base_vir_addr));

		/* read pmu */
#ifdef CONFIG_DPM_HWMON_PWRSTAT
		writel(0xFFFF0010, SOC_DP_MONITOR_DPM_CTRL_EN_ADDR(acpu_npu_dpm_base_vir_addr));
#else
		/* STEP19 apb send cnt pulse, because before apb read counter or energy, MUST send a pulse firstly */
		SOC_DP_MONITOR_SOFT_PULSE_UNION soft_pulse = {0};
		soft_pulse.reg.soft_cnt_pulse = 1;
		soft_pulse.value |= (1 << SOC_DP_MONITOR_SOFT_PULSE_soft_cnt_pulse_START) << NPU_DP_MONITOR_MASK_SHITFT;
		writel(soft_pulse.value, SOC_DP_MONITOR_SOFT_PULSE_ADDR(acpu_npu_dpm_base_vir_addr));

		/* STEP20 apb read pmu counter */
		if (mode == BUSINESSFIT) {
			for (count_reg_num_updated = 0; count_reg_num_updated < NPU_DPM_COUNTER_PMU; count_reg_num_updated++) {
				/* STEP20 APB read dpm_counter_pmu to dpm_counter_table[] */
				val = readl(SOC_DP_MONITOR_DPM_COUNTER_PMU_ADDR(acpu_npu_dpm_base_vir_addr, 0, count_reg_num_updated));
				npu_dpm_ops_v3.dpm_counter_table[count_reg_num_updated] = val;
				npu_drv_warn("[dpm_npu] count_reg_num_updated: %d, val: %d.\n", count_reg_num_updated, val);
			}
		} else {
			for (count_reg_num_updated = 0; count_reg_num_updated < NPU_DPM_COUNTER_PMU; count_reg_num_updated++) {
				/* STEP20 APB read dpm_counter_pmu to dpm_counter_table[] */
				val = readl(SOC_DP_MONITOR_DPM_COUNTER_PMU_ADDR(acpu_npu_dpm_base_vir_addr, 0, count_reg_num_updated));
				npu_dpm_ops_v3.dpm_counter_table[count_reg_num_updated] += val;
				npu_drv_warn("[dpm_npu] count_reg_num_updated: %d, val: %d.\n", count_reg_num_updated, val);
			}
		}
#endif

		/* trigger icg soft cnt pulse */
		dp_monitor_ctrl_en.value = 0;
		dp_monitor_ctrl_en.reg.icg_counter_en  = 1;
		dp_monitor_ctrl_en.reg.pmu_counter_en  = 0;
		dp_monitor_ctrl_en.reg.apb_read_cnt_en = apb_read_cnt_en;
		dp_monitor_ctrl_en.value |= (1 << SOC_DP_MONITOR_DPM_CTRL_EN_icg_counter_en_START)  << NPU_DP_MONITOR_MASK_SHITFT;
		dp_monitor_ctrl_en.value |= (1 << SOC_DP_MONITOR_DPM_CTRL_EN_pmu_counter_en_START)  << NPU_DP_MONITOR_MASK_SHITFT;
		dp_monitor_ctrl_en.value |= (1 << SOC_DP_MONITOR_DPM_CTRL_EN_apb_read_cnt_en_START) << NPU_DP_MONITOR_MASK_SHITFT;
		writel(0xFFFF000C, SOC_DP_MONITOR_DPM_CTRL_EN_ADDR(acpu_npu_dpm_base_vir_addr));

		/* read icg */
#ifdef CONFIG_DPM_HWMON_PWRSTAT
		writel(0xFFFF0008, SOC_DP_MONITOR_DPM_CTRL_EN_ADDR(acpu_npu_dpm_base_vir_addr));
#else
		/* STEP19 apb send cnt pulse, because before apb read counter or energy, MUST send a pulse firstly */
		soft_pulse.value = 0;
		soft_pulse.reg.soft_cnt_pulse = 1;
		soft_pulse.value |= (1 << SOC_DP_MONITOR_SOFT_PULSE_soft_cnt_pulse_START) << NPU_DP_MONITOR_MASK_SHITFT;
		writel(0xFFFF0003, SOC_DP_MONITOR_SOFT_PULSE_ADDR(acpu_npu_dpm_base_vir_addr));

		/* STEP21-1 wait for icg read ready */
		SOC_DP_MONITOR_ICG_READ_READY_APB_UNION icg_read_ready_apb = {0};
		do
			icg_read_ready_apb.value = readl(SOC_DP_MONITOR_ICG_READ_READY_APB_ADDR(acpu_npu_dpm_base_vir_addr, 0));
		while(icg_read_ready_apb.reg.icg_read_ready_apb == 0);

		if (mode == BUSINESSFIT) {
			for (count_reg_num_updated = 0; count_reg_num_updated < NPU_DPM_COUNTER_ICG; count_reg_num_updated++) {
				/* STEP21-2 APB read dpm_counter_icg to dpm_counter_table[] */
				val = readl(SOC_DP_MONITOR_DPM_COUNTER_ICG_ADDR(acpu_npu_dpm_base_vir_addr, 0, count_reg_num_updated));
				npu_dpm_ops_v3.dpm_counter_table[NPU_DPM_COUNTER_PMU + count_reg_num_updated] = val;
				npu_drv_warn("[dpm_npu] count_reg_num_updated: %d, val: %d.\n", count_reg_num_updated, val);
			}
		} else {
			for (count_reg_num_updated = 0; count_reg_num_updated < NPU_DPM_COUNTER_ICG; count_reg_num_updated++) {
				/* STEP21-2 APB read dpm_counter_icg, and add its value to dpm_counter_table[] */
				val = readl(SOC_DP_MONITOR_DPM_COUNTER_ICG_ADDR(acpu_npu_dpm_base_vir_addr, 0, count_reg_num_updated));
				npu_dpm_ops_v3.dpm_counter_table[NPU_DPM_COUNTER_PMU + count_reg_num_updated] += val;
				npu_drv_warn("[dpm_npu] count_reg_num_updated: %d, val: %d.\n", count_reg_num_updated, val);
			}
		}
#endif
		mutex_unlock(&dpm_npu_lock);
	}

	return (NPU_DPM_COUNTER_PMU + NPU_DPM_COUNTER_ICG);
}

int npu_dpm_get_counter_for_fitting(int mode)
{
	if (mode != BUSINESSFIT && mode != DATAREPORTING) {
		npu_drv_err("invalid mode=%d");
		return 0;
	}
	return npu_dpm_update_counter(mode);
}
#endif

int npu_dpm_update_energy(void)
{
#ifdef CONFIG_DPM_HWMON_PWRSTAT
	npu_drv_err("unsupport update energy in pwrstat mode");
	return 0;
#else
	struct npu_dev_ctx *dev_ctx = NULL;
	uint32_t val = 0;

	/* dubai disable npu dpm, return 0 */
	if (npu_dpm_enable_flag() == false)
		return 0;

	if (npu_dpm_ops_v3.total_power_table == NULL) {
		npu_drv_err("npu_dpm_ops_v3.dpm_counter_table is null");
		return 0;
	}

	if (npu_dpm_ops_v3.dyn_power_table == NULL) {
		npu_drv_err("npu_dpm_ops_v3.dyn_power_table is null");
		return 0;
	}

	dev_ctx = get_dev_ctx_by_id(0);
	if (dev_ctx == NULL) {
		npu_drv_err("get current device failed");
		return 0;
	}

	if (atomic_read(&dev_ctx->power_access) == 0) {
		mutex_lock(&dpm_npu_lock);
		/* STEP19 apb send energy pulse, because before apb read counter or energy, MUST send a pulse firstly */
		SOC_DP_MONITOR_SOFT_PULSE_UNION dp_monitor_soft_pulse = {0};
		dp_monitor_soft_pulse.reg.soft_energy_pulse = 1;
		dp_monitor_soft_pulse.value |= (1 << SOC_DP_MONITOR_SOFT_PULSE_soft_energy_pulse_START) << NPU_DP_MONITOR_MASK_SHITFT;
		writel(dp_monitor_soft_pulse.value, SOC_DP_MONITOR_SOFT_PULSE_ADDR(acpu_npu_dpm_base_vir_addr));

		/* STEP22 APB read acc_dpm_energy, BECAUSE aicore only 1 in this platform,
		   only update total_power_table[0] and dyn_power_table[0] */
		val = readl(SOC_DP_MONITOR_ACC_DPM_ENERGY_SNAP_ADDR(acpu_npu_dpm_base_vir_addr, 0));
		npu_dpm_ops_v3.total_power_table[0] = val;
		npu_drv_warn("[dpm_npu] acc_dpm_energy val = 0x%X.\n", val);

#ifdef CONFIG_DPM_HWMON_DEBUG
		/* STEP23 APB read acc_dyn_energy */
		val = readl(SOC_DP_MONITOR_ACC_DYN_ENERGY_SNAP_ADDR(acpu_npu_dpm_base_vir_addr, 0));
		npu_dpm_ops_v3.dyn_power_table[0] = val;
		npu_drv_warn("[dpm_npu] acc_dyn_energy val = 0x%X.\n", val);
#endif

		mutex_unlock(&dpm_npu_lock);
	}
	return DPM_ENERGY_NUM;
#endif
}

static int npu_dpm_pcr_remap(void)
{
	mutex_lock(&dpm_npu_lock);

	if (acpu_npu_crg_base_vir_addr == NULL) {
		acpu_npu_crg_base_vir_addr = ioremap(SOC_ACPU_npu_crg_BASE_ADDR,
			SOC_NPU_CRG_SIZE);
		if (acpu_npu_crg_base_vir_addr == NULL) {
			npu_drv_err("ioremap npu crg base addr fail.\n");
			goto npu_crg_fail;
		}
	}

	if (acpu_npu_pcr_base_vir_addr == NULL) {
		acpu_npu_pcr_base_vir_addr = ioremap(SOC_ACPU_npu_pcr_BASE_ADDR,
			SOC_NPU_PCR_SIZE);
		if (acpu_npu_pcr_base_vir_addr == NULL) {
			npu_drv_err("ioremap npu pcr base addr fail.\n");
			goto npu_pcr_fail;
		}
	}

	if (acpu_npu_dpm_base_vir_addr == NULL) {
		acpu_npu_dpm_base_vir_addr = ioremap(SOC_ACPU_dpm_BASE_ADDR,
			SOC_NPU_DPM_MAP_SIZE);
		if (acpu_npu_dpm_base_vir_addr == NULL) {
			npu_drv_err("ioremap npu dpm base addr fail.\n");
			goto npu_dpm_fail;
		}
	}

	mutex_unlock(&dpm_npu_lock);
	return 0;
npu_dpm_fail:
	if (acpu_npu_pcr_base_vir_addr != NULL) {
		iounmap(acpu_npu_pcr_base_vir_addr);
		acpu_npu_pcr_base_vir_addr = NULL;
	}
npu_pcr_fail:
	if (acpu_npu_crg_base_vir_addr != NULL) {
		iounmap(acpu_npu_crg_base_vir_addr);
		acpu_npu_crg_base_vir_addr = NULL;
	}

npu_crg_fail:
	npu_drv_err("npu dpm remap register addr fail\n");
	mutex_unlock(&dpm_npu_lock);
	return -1;
}

static void npu_dpm_pcr_unremap(void)
{
	mutex_lock(&dpm_npu_lock);

	if (acpu_npu_pcr_base_vir_addr != NULL) {
		iounmap(acpu_npu_pcr_base_vir_addr);
		acpu_npu_pcr_base_vir_addr = NULL;
	}

	if (acpu_npu_crg_base_vir_addr != NULL) {
		iounmap(acpu_npu_crg_base_vir_addr);
		acpu_npu_crg_base_vir_addr = NULL;
	}

	if (acpu_npu_dpm_base_vir_addr != NULL) {
		iounmap(acpu_npu_dpm_base_vir_addr);
		acpu_npu_dpm_base_vir_addr = NULL;
	}
	mutex_unlock(&dpm_npu_lock);
}

static int npu_pcr_enable(void)
{
	mutex_lock(&dpm_npu_lock);

	/* step1 Platform Software Dept provides APIs. */
	/* step2 Configure windows and thresholds (choose the best configuration based on
	   requirements and measured results) */

	/* PCR power consumption sampling granularity=sample_interval*clk_period, it is
	   recommended to set the same granularity as DPM power consumption statistics.
	   The setting range is 1~1023, and it is forbidden to set to 0 */
	writel(0x00000014, SOC_PCR_SAMPLE_INTERVAL_ADDR(acpu_npu_pcr_base_vir_addr));

	/* didt statistics window = didt_window*sample_interval*clk_period. For example,
	   the sampling granularity of PCR power consumption is 60ns. If the statistical
	   window is to be set to 300ns, the register value can be set to 0x5. */
	writel(0x00000005, SOC_PCR_DIDT_WINDOW_ADDR(acpu_npu_pcr_base_vir_addr));

	/* maxpower statistics window=maxpower_window*sample_interval*clk_period */
	writel(0x00000005, SOC_PCR_MAXPOWER_WINDOW_ADDR(acpu_npu_pcr_base_vir_addr));

	/* The PCR statistical result is the energy in the statistical window, that is,
	   energy (nJ) = power (W) * maxpower statistical window (ns), so it needs to be
	   converted when the threshold is set.
		1) Set the conversion: threshold_dn/up_budget=power_th * maxpower statistics window.
		If you want to set the power consumption threshold to 6W and the maxpower statistical
		   window to be 300ns, the register value can be set to 1800 (nJ).
		2) Constraints need to be met: UP_BUDGET0 < DN_BUDGET0 < UP_BUDGET1 < DN_BUDGET1
		   < UP_BUDGET2 < DN_BUDGET2 < UP_BUDGET3 < DN_BUDGET3 .
		3) Example: Assuming that the DPM has calibrated the power consumption, the maxpower
		   statistical window is 300ns, the power consumption down-frequency threshold is expected
		   to be set to 5.5W/6W/6.5W/7W, and the up-frequency threshold is expected to be set to
		   5.3W/5.8W/6.2W/ 6.8W, the register values ​​need to be set as follows:
			DN_BUDGET0=300*5.5=1650=0x672, UP_BUDGET0=300*5.3=1590=0x636;
			DN_BUDGET1=300*6.0=1800=0x708, UP_BUDGET1=300*5.8=1740=0x6CC;
			DN_BUDGET2=300*6.5=1950=0x79E, UP_BUDGET2=300*6.2=1860=0x744;
			DN_BUDGET3=300*7.0=2100=0x834, UP_BUDGET3=300*6.8=2040=0x7F8;
		4) Note: The actual use needs to be calibrated according to the DPM power consumption fitting. */
	writel(0x00000672, SOC_PCR_THRESHOLD_DN_BUDGET0_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000708, SOC_PCR_THRESHOLD_DN_BUDGET1_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x0000079E, SOC_PCR_THRESHOLD_DN_BUDGET2_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000834, SOC_PCR_THRESHOLD_DN_BUDGET3_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000636, SOC_PCR_THRESHOLD_UP_BUDGET0_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x000006CC, SOC_PCR_THRESHOLD_UP_BUDGET1_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000744, SOC_PCR_THRESHOLD_UP_BUDGET2_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x000007F8, SOC_PCR_THRESHOLD_UP_BUDGET3_ADDR(acpu_npu_pcr_base_vir_addr));

	/* The setting method is the same as the maxpower threshold, except that the formula becomes:
	   threshold_dn/up_didt=delta_power_th * didt statistics window
	   (delta_power_th represents the threshold of power consumption transition) */
	writel(0x00000177, SOC_PCR_THRESHOLD_DN_DIDT0_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000186, SOC_PCR_THRESHOLD_DN_DIDT1_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000196, SOC_PCR_THRESHOLD_DN_DIDT2_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x000001A5, SOC_PCR_THRESHOLD_DN_DIDT3_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000170, SOC_PCR_THRESHOLD_UP_DIDT0_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000180, SOC_PCR_THRESHOLD_UP_DIDT1_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x00000190, SOC_PCR_THRESHOLD_UP_DIDT2_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x000001A0, SOC_PCR_THRESHOLD_UP_DIDT3_ADDR(acpu_npu_pcr_base_vir_addr));

	/* step 3 Configure the frequency reduction maintenance time
	   (select the best configuration according to the requirements and actual measurement results) */
	/* Minimum hold time for each gear before downclocking */
	writel(0x000F000F, SOC_PCR_FREQ_DN_PERIOD_ADDR(acpu_npu_pcr_base_vir_addr));

	/* Minimum hold time for gears 0-3 before upscaling */
	writel(0x0000001F, SOC_PCR_FREQ_UP_PERIOD0_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x0000001F, SOC_PCR_FREQ_UP_PERIOD1_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x0000001F, SOC_PCR_FREQ_UP_PERIOD2_ADDR(acpu_npu_pcr_base_vir_addr));
	writel(0x0000001F, SOC_PCR_FREQ_UP_PERIOD3_ADDR(acpu_npu_pcr_base_vir_addr));

	/* step4 start PCR */
	/* Enable maxpower detection and maxdidt detection. */
	writel(0x00000003, SOC_PCR_CTRL0_ADDR(acpu_npu_pcr_base_vir_addr));

	/* The frequency reduction state machine is forced to remain in the IDLE state
	   (the frequency reduction command is not issued) */
	writel(0x00000001, SOC_PCR_CTRL1_ADDR(acpu_npu_pcr_base_vir_addr));

	/* enable PCR */
	writel(0x00000001, SOC_PCR_EN_ADDR(acpu_npu_pcr_base_vir_addr));

	/* Wait for 5us (make sure the PCR buffer is filled first) */
	udelay(5 * 1000);

	/* The frequency reduction state machine exits IDLE */
	writel(0x00000000, SOC_PCR_CTRL1_ADDR(acpu_npu_pcr_base_vir_addr));

	mutex_unlock(&dpm_npu_lock);
	return 0;
}

static void npu_pcr_disable(void)
{
	mutex_lock(&dpm_npu_lock);

	/* STEP1 disable PCR */
	/* The frequency reduction state machine is forced to remain in the IDLE state
	   (the frequency reduction command is not issued) */
	writel(0x00000001, SOC_PCR_CTRL1_ADDR(acpu_npu_pcr_base_vir_addr));

	udelay(5 * 1000);

	/* disable PCR */
	writel(0x00000000, SOC_PCR_EN_ADDR(acpu_npu_pcr_base_vir_addr));

	/* disable maxpower and maxdidt test */
	writel(0x00000000, SOC_PCR_CTRL0_ADDR(acpu_npu_pcr_base_vir_addr));

	mutex_unlock(&dpm_npu_lock);
}

static int npu_dpm_enable(void)
{
	int i = 0;

	/* Reading dpm data and powering on/off the dpm device are asynchronous.
       In order to avoid accessing the power-off area, a mutex lock is required. */
	mutex_lock(&dpm_npu_lock);

	/* step 7 volt_param_a,volt_param_b */
	SOC_DP_MONITOR_VOLT_UNION dp_monitor_volt_cfg = {0};
	dp_monitor_volt_cfg.reg.volt_param_a = STATIC_VOLT_PARAM_A;
	dp_monitor_volt_cfg.reg.volt_param_b = STATIC_VOLT_PARAM_B;
	writel(dp_monitor_volt_cfg.value, SOC_DP_MONITOR_VOLT_ADDR(acpu_npu_dpm_base_vir_addr));

	/* step 8 volt_gear */
	for(i = 0; i < VOLT_GEAR_NUM; ++i)
		writel(STATIC_VOLT_GEAR, SOC_DP_MONITOR_VOLT_GEAR_ADDR(acpu_npu_dpm_base_vir_addr, i));

	/* step 9 sta_volt_param */
	for(i = 0; i < STA_VOLT_PARAM_NUM; ++i)
		writel(STATIC_VOLT_PARAM, SOC_DP_MONITOR_STA_VOLT_PARAM_ADDR(acpu_npu_dpm_base_vir_addr, i));

	/* step 10 fx2_const */
	for(i = 0; i < FX2_CONST_NUM; ++i)
		writel(STATIC_FX2_CONST, SOC_DP_MONITOR_FX2_CONST_ADDR(acpu_npu_dpm_base_vir_addr, i));

	/* step 11 fx2_var1 */
	for(i = 0; i < FX2_VAR1_NUM; ++i)
		writel(STATIC_FX2_VAR1, SOC_DP_MONITOR_FX2_VAR1_ADDR(acpu_npu_dpm_base_vir_addr, i));

	/* step 12 fx2_var2 */
	for(i = 0; i < FX2_VAR2_NUM; ++i)
		writel(STATIC_FX2_VAR2, SOC_DP_MONITOR_FX2_VAR2_ADDR(acpu_npu_dpm_base_vir_addr, i));

	/* step 13 sig_power_param */
	for(i = 0; i < SIG_POWER_PARAM_NUM; ++i)
		writel(STATIC_SIG_POWER_PARAM, SOC_DP_MONITOR_SIG_POWER_PARAM_ADDR(acpu_npu_dpm_base_vir_addr, i));

	/* step 14 shift_bit */
	SOC_DP_MONITOR_SHIFT_BIT_UNION dp_monitor_shift_bit = {0};
	/* static statistics timer, which controls how many cycles to report a piece of static data */
	/* every 32 cycles report once. if 0, no report */
	dp_monitor_shift_bit.reg.cfg_cnt_init_sta    = 0x20;

	/* magnification of variables in fx2, 1 means to zoom 1^10=1 */
	dp_monitor_shift_bit.reg.volt_temp_var_shift = 0x1;

	/* magnification of variables in fx1, 2 means to zoom 2^10=1024 */
	dp_monitor_shift_bit.reg.sdpm_shift_bit      = 0x2;

	/* the number of bits to be shifted for (cnt*param…)*dyn_scale */
	dp_monitor_shift_bit.reg.dyn_shift_bit       = 0x2;

	/* DPM controls how many cnts initiate a REQ request to RSM */
	dp_monitor_shift_bit.reg.cfg_cnt_init_dyn    = 0x6;
	writel(dp_monitor_shift_bit.value, SOC_DP_MONITOR_SHIFT_BIT_ADDR(acpu_npu_dpm_base_vir_addr));

	/* enable dpm */
	/* step 16 sdpm_enable ALL */
	writel(SDPM_ENABLE_ALL, SOC_DP_MONITOR_SDPM_ENABLE_ADDR(acpu_npu_dpm_base_vir_addr));

	/* step 17 sg_ddpm_en ALL */
	writel(SG_DDPM_ENABLE_ALL, SOC_DP_MONITOR_SG_DDPM_EN_ADDR(acpu_npu_dpm_base_vir_addr));

	/* step 18 monitor_ctrl_en ALL */
	writel(NPU_DP_MONITOR_CTRL_ENABLE_ALL, SOC_DP_MONITOR_DPM_ENABLE_ADDR(acpu_npu_dpm_base_vir_addr));

	mutex_unlock(&dpm_npu_lock);
	npu_drv_warn("succ\n");

	return 0;
}

static void npu_dpm_disable(void)
{
	mutex_lock(&dpm_npu_lock);

	/* step1: disable dpm */
	writel(NPU_DP_MONITOR_CTRL_DISABLE_ALL, SOC_DP_MONITOR_DPM_ENABLE_ADDR(acpu_npu_dpm_base_vir_addr));

	mutex_unlock(&dpm_npu_lock);
}

// npu powerup
void npu_dpm_init(void)
{
	/* dubai disable npu dpm, return (it is not an error) */
	if (npu_dpm_enable_flag() == false)
		return;

	/* remap register address for dpm settings */
	cond_return_void(npu_dpm_pcr_remap() != 0, "dpm remap register address fail\n");
	/* pcr enable */
	cond_return_void(npu_pcr_enable() != 0, "npu_pcr_enable fail\n");
	/* dpm enable */
	cond_return_void(npu_dpm_enable() != 0, "npu_dpm_enable fail\n");
}

// npu powerdown
void npu_dpm_exit(void)
{
#ifdef CONFIG_DPM_HWMON_DEBUG
	(void)npu_dpm_update_counter(DATAREPORTING);
#endif

	/* pcr must disable before dpm, because the data source of PCR is DPM */
	npu_pcr_disable();
	npu_dpm_disable();
	npu_dpm_pcr_unremap();

	npu_drv_warn("[dpm_npu]dpm_npu is successfully deinitialized\n");
}

static int __init dpm_npu_init(void)
{
	if (npu_plat_bypass_status() == NPU_BYPASS)
		return -1;

	return dpm_hwmon_register(&npu_dpm_ops_v3);
}
module_init(dpm_npu_init);

static void __exit dpm_npu_exit(void)
{
	if (npu_plat_bypass_status() == NPU_BYPASS)
		return;

	dpm_hwmon_unregister(&npu_dpm_ops_v3);
}
module_exit(dpm_npu_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DPM");
MODULE_VERSION("V1.0");

