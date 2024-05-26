/*
 * npu_platform_pm.c
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

#include "npu_log.h"
#include "npu_platform.h"
#include "npu_pm_config_aicore.h"
#include "npu_pm_config_nputop.h"
#include "npu_pm_config_npusubsys.h"
#include "npu_pm_config_tscpu.h"
#include "npu_pm_config_smmutbu.h"
#include "npu_pm_framework.h"
#include "npu_adapter.h"

enum npu_subip g_powerup_order[NPU_SUBIP_MAX] = {
	NPU_TOP,
	NPU_SUBSYS,
	NPU_SMMU,
	NPU_AICORE0,
	NPU_AICORE1,
	NPU_TSCPU
};

enum npu_subip g_powerdown_order[NPU_SUBIP_MAX] = {
	NPU_TSCPU,
	NPU_AICORE0,
	NPU_AICORE1,
	NPU_SUBSYS,
	NPU_SMMU,
	NPU_TOP
};

/*
 * NPU_TOP : call npu_pm_power_on/off
 *     POWER ON    : set_pu_npubus + set_pu_npu_subsys
 *     POWER OFF   : set_pd_npubus + set_pd_npu_subsys
 * NPU_SUBSYS
 *     POWER ON    : ts_subsys_powerup + hwts_init + smmu_tcu_init
 *     POWER OFF   : wait tscpu_idle + disable ts_tbu + ts_subsys_powerdown
 * NPU_TSCPU
 *     POWER ON    : unrst ts_cpu + ts_boot_pooling + clear_sqcq + init_sdma
 *     POWER OFF   : disable sdma_tbu + clear_sqcq + inform_ts + ts_powerdown_pooling
 * NPU_SMMU
 *     POWER ON    : mm_smmu_poweron + tbu_connect ts_tbu + set_swid ts_tbu + XXXXX(about ts_tbu secure,sid/ssid...)
 *     POWER OFF   : mm_smmu_poweroff
 * NPU_AICORE
 *     POWER ON    : powerup aicore0/1 + tbu_connect aic_tbu + set_swid aic_tbu + aicore init + flow contrl
 *     POWER OFF   : disable aic_tbu + powerdown aicore0/1
 */
struct npu_pm_subip_action g_pm_subip_action_talbe[NPU_SUBIP_MAX] = {
	{NPU_TOP, npu_top_powerup, npu_top_powerdown},
	{NPU_SUBSYS, npu_npusubsys_powerup, npu_npusubsys_powerdown},
	{NPU_TSCPU, npu_tscpu_powerup, npu_tscpu_powerdown},
	{NPU_SMMU, npu_smmu_tbu_powerup, npu_smmu_tbu_powerdown},
	{NPU_AICORE0, npu_aicore_powerup, npu_aicore_powerdown},
	{NPU_AICORE1, npu_aicore_powerup, npu_aicore_powerdown}
};

/*
 * NONSEC = 0              :cannot work with NPU_SEC;                       support idle powerdown;     need all subips
 * SEC = 1                 :cannot work with NONSEC&ISPNN&INIT;             not support idle powerdown; need all subips except NPU_TSCPU
 * ISPNN_SEPARATED = 2     :cannot work with NPU_SEC and ISPNN_SHARED;      not support idle powerdown; need all subips except NPU_AICORE0
 * ISPNN_SHARED = 3        :cannot work with NPU_SEC and ISPNN_SEPARATED;   not support idle powerdown; need all subips
 * INIT = 4                :cannot work with NPU_SEC;                       not support idle powerdown; need all subips except NPU_AICORE0/1
 */
struct npu_pm_work_strategy g_pm_work_strategy_table[NPU_WORKMODE_MAX] = {
	{NPU_NONSEC, {npu_bitmap_set(0, NPU_SEC)}, 1, {NPU_SUBIP_ALL}},
	{NPU_SEC,
		{npu_bitmap_set(0, NPU_NONSEC) |
		npu_bitmap_set(0, NPU_ISPNN_SEPARATED) |
		npu_bitmap_set(0, NPU_ISPNN_SHARED) | npu_bitmap_set(0, NPU_INIT)},
		0, {npu_bitmap_clear(NPU_SUBIP_ALL, NPU_TSCPU)}},
	{NPU_ISPNN_SEPARATED,
		{npu_bitmap_set(0, NPU_SEC) | npu_bitmap_set(0, NPU_ISPNN_SHARED)},
		0, {npu_bitmap_clear(NPU_SUBIP_ALL, NPU_AICORE0)}},
	{NPU_ISPNN_SHARED,
		{npu_bitmap_set(0, NPU_SEC) | npu_bitmap_set(0, NPU_ISPNN_SEPARATED)},
		0, {NPU_SUBIP_ALL}},
	{NPU_INIT,
		{npu_bitmap_set(0, NPU_SEC)}, 0,
		{npu_bitmap_clear(npu_bitmap_clear(NPU_SUBIP_ALL, NPU_AICORE0),
		NPU_AICORE1)}}
};

/*
 * interframe_idle_work_strategy:
 * NONSEC = 0             :cannot work with NPU_SEC;                    support idle powerdown;     NPU_AICORE0/1
 * SEC = 1                :cannot work with NONSEC&ISPNN&INIT;          not support idle powerdown; NPU_AICORE0/1
 * ISPNN_SEPARATED = 2    :cannot work with NPU_SEC&ISPNN_SHARED;       not support idle powerdown; NPU_AICORE1
 * ISPNN_SHARED = 3       :cannot work with NPU_SEC&ISPNN_SEPARATED;    not support idle powerdown; NPU_AICORE0/1
 * INIT = 4               :cannot work with NPU_SEC;                    not support idle powerdown; null
 */
static struct npu_pm_work_strategy g_pm_interframe_idle_work_strategy_table[
	NPU_WORKMODE_MAX] = {
	{NPU_NONSEC, {npu_bitmap_set(0, NPU_SEC)}, 1,
		{npu_bitmap_set(0, NPU_AICORE0) | npu_bitmap_set(0, NPU_AICORE1)}},
	{NPU_SEC, {npu_bitmap_set(0, NPU_NONSEC) |
		npu_bitmap_set(0, NPU_ISPNN_SEPARATED) |
		npu_bitmap_set(0, NPU_ISPNN_SHARED) | npu_bitmap_set(0, NPU_INIT)}, 0,
		{npu_bitmap_set(0, NPU_AICORE0) | npu_bitmap_set(0, NPU_AICORE1)}},
	{NPU_ISPNN_SEPARATED, {npu_bitmap_set(0, NPU_SEC) |
		npu_bitmap_set(0, NPU_ISPNN_SHARED)}, 0,
		{npu_bitmap_set(0, NPU_AICORE1)}},
	{NPU_ISPNN_SHARED, {npu_bitmap_set(0, NPU_SEC) |
		npu_bitmap_set(0, NPU_ISPNN_SEPARATED)}, 0,
		{npu_bitmap_set(0, NPU_AICORE0) | npu_bitmap_set(0, NPU_AICORE1)}},
	{NPU_INIT, {npu_bitmap_set(0, NPU_SEC)}, 0, {0}}
};

u32 g_pm_interfram_idle_subip = npu_bitmap_set(0, NPU_AICORE0) |
	npu_bitmap_set(0, NPU_AICORE1);

void npu_pm_reset_subip(enum npu_subip subip)
{
	u32 idx;
	u32 strategy_size = ARRAY_SIZE(g_pm_work_strategy_table);
	struct npu_pm_work_strategy *strategy = NULL;

	for (idx = 0; idx < strategy_size; idx++) {
		strategy = &g_pm_work_strategy_table[idx];
		strategy->subip_set.data = npu_bitmap_clear(strategy->subip_set.data,
			subip);
	}

	strategy_size = ARRAY_SIZE(g_pm_interframe_idle_work_strategy_table);
	for (idx = 0; idx < strategy_size; idx++) {
		strategy = &g_pm_interframe_idle_work_strategy_table[idx];
		strategy->subip_set.data = npu_bitmap_clear(strategy->subip_set.data,
			subip);
	}

	g_pm_interfram_idle_subip = npu_bitmap_clear(g_pm_interfram_idle_subip,
		subip);
}

int npu_pm_init(struct npu_dev_ctx *dev_ctx)
{
	struct npu_power_manage *power_manager = NULL;
	struct npu_interframe_idle_manager *interframe_power_manager = NULL;
	struct npu_platform_info *plat_info = NULL;
	static int npu_pm_subip_has_reset;

	cond_return_error(dev_ctx == NULL, -1, "dev_ctx is null\n");
	plat_info = dev_ctx->plat_info;
	cond_return_error(plat_info == NULL, -1, "plat_info is null\n");

	if (npu_pm_subip_has_reset == 0) {
		if (npu_plat_aicore_get_disable_status(0))
			npu_pm_reset_subip(NPU_AICORE0);

		if (npu_plat_aicore_get_disable_status(1))
			npu_pm_reset_subip(NPU_AICORE1);

		npu_pm_subip_has_reset = 1;
	}

	power_manager = &dev_ctx->pm;
	power_manager->strategy_table_addr =
		(vir_addr_t)(uintptr_t)&g_pm_work_strategy_table[0];
	power_manager->action_table_addr =
		(vir_addr_t)(uintptr_t)&g_pm_subip_action_talbe[0];

	if (plat_info->dts_info.feature_switch[NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN]) {
		interframe_power_manager = &power_manager->interframe_idle_manager;
		interframe_power_manager->idle_subip = g_pm_interfram_idle_subip;
		interframe_power_manager->strategy_table_addr =
			(vir_addr_t)(uintptr_t)&g_pm_interframe_idle_work_strategy_table[0];
	}

	return 0;
}

enum npu_subip *get_powerup_order(void)
{
	return g_powerup_order;
}

enum npu_subip *get_powerdown_order(void)
{
	return g_powerdown_order;
}
