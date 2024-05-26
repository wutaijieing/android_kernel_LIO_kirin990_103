/*
 * npu_hwts_plat.c
 *
 * about npu hwts plat
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
#include "npu_hwts_plat.h"
#include "npu_shm.h"
#include "profiling/npu_dfx_profiling.h"
#include <linux/io.h>

void profiling_clear_mem_ptr(u8 devid)
{
	int i;
	struct profiling_ring_buff_manager *ring_manager = NULL;
	struct npu_prof_info *profiling_info = NULL;

	for (i = 0; i < PROF_CHANNEL_MAX; i++) {
		profiling_info = npu_calc_profiling_info(devid);
		cond_return_void(profiling_info == NULL,
			"profiling_info is null, channel = %d\n", i);

		ring_manager = &(profiling_info->head.manager.ring_buff[i]);
		cond_return_void(ring_manager == NULL,
			"ring_manager is null, channel = %d\n", i);

		ring_manager->read = 0;
		ring_manager->write = 0;
	}
}

void hwts_profiling_init(u8 devid)
{
	struct npu_platform_info *plat_info = NULL;

	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get plat_ops failed.\n");
		return;
	}

	/* hwts clear wirte&read ptr when power up, */
	/* clear ptr in profiling info remem synchronously */
	profiling_clear_mem_ptr(devid);
}

u64 npu_hwts_get_base_addr(void)
{
	u64 hwts_base = 0ULL;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	cond_return_error(plat_info == NULL, 0ULL, "get platform info failed\n");
	hwts_base = (u64)(uintptr_t)plat_info->dts_info.reg_vaddr[NPU_REG_HWTS_BASE];
	npu_drv_debug("hwts_base = 0x%pK\n", hwts_base);
	return hwts_base;
}

int npu_hwts_query_aicore_pool_status(
	u8 index, u8 pool_sec, u8 *reg_val)
{
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS0_NS_UNION aicore_pool_status0_ns = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS1_NS_UNION aicore_pool_status1_ns = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS2_NS_UNION aicore_pool_status2_ns = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS3_NS_UNION aicore_pool_status3_ns = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS0_S_UNION aicore_pool_status0_s = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS1_S_UNION aicore_pool_status1_s = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS2_S_UNION aicore_pool_status2_s = {0};
	SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS3_S_UNION aicore_pool_status3_s = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	if (pool_sec == 0) {
		if (index == 0) {
			aicore_pool_status0_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS0_NS_ADDR(hwts_base));
			*reg_val = aicore_pool_status0_ns.reg.aic_enabled_status0_ns;
		} else if (index == 1) {
			aicore_pool_status1_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS1_NS_ADDR(hwts_base));
			*reg_val = aicore_pool_status1_ns.reg.aic_enabled_status1_ns;
		} else if (index == 2) {
			aicore_pool_status2_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS2_NS_ADDR(hwts_base));
			*reg_val = aicore_pool_status2_ns.reg.aic_enabled_status2_ns;
		} else {
			aicore_pool_status3_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS3_NS_ADDR(hwts_base));
			*reg_val = aicore_pool_status3_ns.reg.aic_enabled_status3_ns;
		}
		npu_drv_warn("HWTS_AICORE_POOL_STATUS%u_NS = 0x%x\n",
				index, *reg_val);
	} else {
		if (index == 0) {
			aicore_pool_status0_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS0_S_ADDR(hwts_base));
			*reg_val = aicore_pool_status0_s.reg.aic_enabled_status0_s;
		} else if (index == 1) {
			aicore_pool_status1_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS1_S_ADDR(hwts_base));
			*reg_val = aicore_pool_status1_s.reg.aic_enabled_status1_s;
		} else if (index == 2) {
			aicore_pool_status2_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS2_S_ADDR(hwts_base));
			*reg_val = aicore_pool_status2_s.reg.aic_enabled_status2_s;
		} else {
			aicore_pool_status3_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AICORE_POOL_STATUS3_S_ADDR(hwts_base));
			*reg_val = aicore_pool_status3_s.reg.aic_enabled_status3_s;
		}
		npu_drv_warn("HWTS_AICORE_POOL_STATUS%u_S = 0x%x\n",
				index, *reg_val);
	}
	return 0;
}

int npu_hwts_query_sdma_pool_status(
	u8 index, u8 pool_sec, u8 *reg_val)
{
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS0_NS_UNION sdma_pool_status0_ns = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS1_NS_UNION sdma_pool_status1_ns = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS2_NS_UNION sdma_pool_status2_ns = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS3_NS_UNION sdma_pool_status3_ns = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS0_S_UNION sdma_pool_status0_s = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS1_S_UNION sdma_pool_status1_s = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS2_S_UNION sdma_pool_status2_s = {0};
	SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS3_S_UNION sdma_pool_status3_s = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	if (pool_sec == 0) {
		if (index == 0) {
			sdma_pool_status0_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS0_NS_ADDR(hwts_base));
			*reg_val = sdma_pool_status0_ns.reg.sdma_enabled_status0_ns;
		} else if (index == 1) {
			sdma_pool_status1_ns.value  = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS1_NS_ADDR(hwts_base));
			*reg_val = sdma_pool_status1_ns.reg.sdma_enabled_status1_ns;
		} else if (index == 2) {
			sdma_pool_status2_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS2_NS_ADDR(hwts_base));
			*reg_val = sdma_pool_status2_ns.reg.sdma_enabled_status2_ns;
		} else {
			sdma_pool_status3_ns.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS3_NS_ADDR(hwts_base));
			*reg_val = sdma_pool_status3_ns.reg.sdma_enabled_status3_ns;
		}
		npu_drv_warn("HWTS_SDMA_POOL_STATUS%u_NS = 0x%x\n",
				index, *reg_val);
	} else {
		if (index == 0) {
			sdma_pool_status0_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS0_S_ADDR(hwts_base));
			*reg_val = sdma_pool_status0_s.reg.sdma_enabled_status0_s;
		} else if (index == 1) {
			sdma_pool_status1_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS1_S_ADDR(hwts_base));
			*reg_val = sdma_pool_status1_s.reg.sdma_enabled_status1_s;
		} else if (index == 2) {
			sdma_pool_status2_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS2_S_ADDR(hwts_base));
			*reg_val = sdma_pool_status2_s.reg.sdma_enabled_status2_s;
		} else {
			sdma_pool_status3_s.value = readq(
				(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_POOL_STATUS3_S_ADDR(hwts_base));
			*reg_val = sdma_pool_status3_s.reg.sdma_enabled_status3_s;
		}
		npu_drv_warn("HWTS_SDMA_POOL_STATUS%u_S = 0x%x\n",
				index, *reg_val);
	}
	return 0;
}

int npu_hwts_query_bus_error_type(u8 *reg_val)
{
	SOC_NPU_HWTS_HWTS_BUS_ERR_INFO_UNION bus_err_info = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	bus_err_info.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_BUS_ERR_INFO_ADDR(hwts_base));
	*reg_val = bus_err_info.reg.bus_err_type;
	npu_drv_warn("HWTS_BUS_ERR_INFO.bus_err_type = 0x%x\n",
			bus_err_info.reg.bus_err_type);
	return 0;
}

int npu_hwts_query_bus_error_id(u8 *reg_val)
{
	SOC_NPU_HWTS_HWTS_BUS_ERR_INFO_UNION bus_err_info = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	bus_err_info.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_BUS_ERR_INFO_ADDR(hwts_base));
	*reg_val = bus_err_info.reg.bus_err_id;
	npu_drv_warn(
			"HWTS_BUS_ERR_INFO.bus_err_id = 0x%x\n",
			bus_err_info.reg.bus_err_id);
	return 0;
}

int npu_hwts_query_sw_status(u16 channel_id, u32 *reg_val)
{
	SOC_NPU_HWTS_HWTS_SQ_SW_STATUS_UNION sq_sw_status = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sq_sw_status.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_SW_STATUS_ADDR(hwts_base, channel_id));
	*reg_val = sq_sw_status.reg.sq_sw_status;
	npu_drv_warn("HWTS_BUS_ERR_INFO.sq_sw_status = 0x%08x\n",
			sq_sw_status.reg.sq_sw_status);
	return 0;
}

int npu_hwts_query_sq_head(u16 channel_id, u16 *reg_val)
{
	SOC_NPU_HWTS_HWTS_SQ_CFG0_UNION sq_cfg0 = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sq_cfg0.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base, channel_id));
	*reg_val = sq_cfg0.reg.sq_head;
	npu_drv_warn("HWTS_SQ_CFG0.sq_head = 0x%04x\n", sq_cfg0.reg.sq_head);
	return 0;
}

int npu_hwts_query_sqe_type(u16 channel_id, u8 *reg_val)
{
	SOC_NPU_HWTS_SQCQ_FSM_MISC_STATE_UNION sqcq_fsm_misc_state = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sqcq_fsm_misc_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_MISC_STATE_ADDR(hwts_base, channel_id));
	*reg_val = sqcq_fsm_misc_state.reg.sqe_type;
	npu_drv_warn("SQCQ_FSM_MISC_STATE.sqe_type = 0x%x\n",
			sqcq_fsm_misc_state.reg.sqe_type);
	return 0;
}

int npu_hwts_query_aic_own_bitmap(u16 channel_id, u8 *reg_val)
{
	SOC_NPU_HWTS_SQCQ_FSM_AIC_OWN_STATE_UNION sqcq_fsm_state = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sqcq_fsm_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_AIC_OWN_STATE_ADDR(hwts_base, channel_id));
	*reg_val = sqcq_fsm_state.reg.aic_own_bitmap;
	npu_drv_warn("SQCQ_FSM_AIC_OWN_STATE.aic_own_bitmap = 0x%x\n",
			sqcq_fsm_state.reg.aic_own_bitmap);
	return 0;
}

int npu_hwts_query_aic_exception_bitmap(u16 channel_id, u8 *reg_val)
{
	SOC_NPU_HWTS_SQCQ_FSM_AIC_EXCEPTION_STATE_UNION sqcq_fsm_state = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sqcq_fsm_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_AIC_EXCEPTION_STATE_ADDR(hwts_base, channel_id));
	*reg_val = sqcq_fsm_state.reg.aic_exception_bitmap;
	npu_drv_warn("SQCQ_FSM_AIC_EXCEPTION_STATE.aic_exception_bitmap = 0x%x\n",
			sqcq_fsm_state.reg.aic_exception_bitmap);
	return 0;
}

int npu_hwts_query_aic_trap_bitmap(u16 channel_id, u8 *reg_val)
{
	SOC_NPU_HWTS_SQCQ_FSM_AIC_TRAP_STATE_UNION sqcq_fsm_state = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sqcq_fsm_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_AIC_TRAP_STATE_ADDR(hwts_base, channel_id));
	*reg_val = sqcq_fsm_state.reg.aic_trap_bitmap;
	npu_drv_warn("SQCQ_FSM_AIC_TRAP_STATE.aic_trap_bitmap = 0x%x\n",
			sqcq_fsm_state.reg.aic_trap_bitmap);
	return 0;
}

int npu_hwts_query_aic_task_config(void)
{
	u64 value;
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0, -EINVAL, "hwts_base is NULL\n");

	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_FREE_AI_CORE_BITMAP_ADDR(hwts_base));
	npu_drv_warn("HWTS_FREE_AI_CORE_BITMAP = 0x%llx\n", value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AIC_BLK_FSM_SEL_ADDR(hwts_base));
	npu_drv_warn("HWTS_AIC_BLK_FSM_SEL = 0x%llx\n", value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_AIC_BLK_FSM_STATE_ADDR(hwts_base));
	npu_drv_warn("HWTS_AIC_BLK_FSM_STATE = 0x%llx\n", value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_L2_OWNER_ADDR(hwts_base));
	npu_drv_warn("HWTS_L2_OWNER = 0x%llx\n", value);

	return 0;
}

int npu_hwts_query_sdma_own_state(u16 channel_id, u8 *reg_val)
{
	SOC_NPU_HWTS_SQCQ_FSM_SDMA_OWN_STATE_UNION sqcq_fsm_sdma_own_state = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(reg_val == NULL, -EINVAL, "reg_val is NULL\n");

	sqcq_fsm_sdma_own_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_SDMA_OWN_STATE_ADDR(hwts_base, channel_id));
	*reg_val = sqcq_fsm_sdma_own_state.reg.owned_sdma_ch_bitmap;
	npu_drv_warn("SQCQ_FSM_SDMA_OWN_STATE.owned_sdma_ch_bitmap = 0x%x\n",
			sqcq_fsm_sdma_own_state.reg.owned_sdma_ch_bitmap);
	return 0;
}

int npu_hwts_query_sdma_task_config(void)
{
	uint32_t idx;
	u64 value[4];
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0, -EINVAL, "hwts_base is NULL\n");

	/* HWTS_GLOABL_DFX_STATUS */
	value[0] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_NS_SQ_BASE_ADDR_CFG_ADDR(hwts_base));
	npu_drv_warn("HWTS_SDMA_NS_SQ_BASE_ADDR_CFG = 0x%llx\n", value[0]);
	value[0] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_FREE_SDMA_SQ_BITMAP_ADDR(hwts_base));
	npu_drv_warn("HWTS_FREE_SDMA_SQ_BITMAP = 0x%llx\n", value[0]);

	value[0] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_BLK_FSM_SEL_ADDR(hwts_base));
	npu_drv_warn("HWTS_SDMA_BLK_FSM_SEL = 0x%llx\n", value[0]);
	value[0] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SDMA_BLK_FSM_STATE_ADDR(hwts_base));
	npu_drv_warn("HWTS_SDMA_BLK_FSM_STATE = 0x%llx\n", value[0]);

	value[0] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SDMA_GLB_SETTING1_ADDR(hwts_base));
	value[1] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SDMA_GLB_SETTING2_ADDR(hwts_base));
	value[2] = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SDMA_GLB_SETTING3_ADDR(hwts_base));
	npu_drv_warn("SDMA_GLB_SETTING[0~2] = 0x%llx 0x%llx 0x%llx\n",
			value[0], value[1], value[2]);

	for (idx = 0; idx < 4; idx++) {
		value[0] = readq(
			(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SDMA_SQCQ_DB_ADDR(hwts_base, idx));
		value[1] = readq(
			(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SDMA_CQE_STATUS0_ADDR(hwts_base, idx));
		value[2] = readq(
			(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SDMA_CQE_STATUS1_ADDR(hwts_base, idx));
		npu_drv_warn("SDMA channal = %u SDMA_SQCQ_DB = 0x%llx"
			" SDMA_CQE_STATUS0 = 0x%llx"
			" SDMA_CQE_STATUS1 = 0x%llx\n",
			idx, value[0], value[1], value[2]);
	}

	return 0;
}

int npu_hwts_query_interrupt_info(
	struct hwts_interrupt_info *interrupt_info)
{
	SOC_NPU_HWTS_HWTS_GLB_CTRL0_UNION glb_ctrl0 = {0};
	SOC_NPU_HWTS_HWTS_GLB_CTRL1_UNION glb_ctrl1 = {0};
	SOC_NPU_HWTS_HWTS_GLB_CTRL2_UNION glb_ctrl2 = {0};
	SOC_NPU_HWTS_HWTS_L1_NORMAL_NS_INTERRUPT_UNION l1_normal_ns_interrupt = {0};
	SOC_NPU_HWTS_HWTS_L1_DEBUG_NS_INTERRUPT_UNION l1_debug_ns_interrupt = {0};
	SOC_NPU_HWTS_HWTS_L1_ERROR_NS_INTERRUPT_UNION l1_error_ns_interrupt = {0};
	SOC_NPU_HWTS_HWTS_L1_DFX_INTERRUPT_UNION l1_dfx_interrupt = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(interrupt_info == NULL,
		-EINVAL, "interrupt_info is NULL\n");

	glb_ctrl0.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_GLB_CTRL0_ADDR(hwts_base));
	glb_ctrl1.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_GLB_CTRL1_ADDR(hwts_base));
	glb_ctrl2.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_GLB_CTRL2_ADDR(hwts_base));
	interrupt_info->aic_batch_mode_timeout_man =
		glb_ctrl1.reg.aic_batch_mode_timeout_man;
	interrupt_info->aic_task_runtime_limit_exp =
		glb_ctrl1.reg.aic_task_runtime_limit_exp;
	interrupt_info->wait_task_limit_exp = glb_ctrl1.reg.wait_task_limit_exp;
	npu_drv_warn("HWTS_GLB_CTRL0.value = 0x%llx\n", glb_ctrl0.value);
	npu_drv_warn("HWTS_GLB_CTRL1.aic_batch_mode_timeout_man = 0x%x"
		" aic_task_runtime_limit_exp = 0x%x"
		" wait_task_limit_exp = 0x%x\n",
		glb_ctrl1.reg.aic_batch_mode_timeout_man,
		glb_ctrl1.reg.aic_task_runtime_limit_exp,
		glb_ctrl1.reg.wait_task_limit_exp);
	npu_drv_warn("HWTS_GLB_CTRL2.sdma_task_runtime_limit_exp = 0x%x\n",
		glb_ctrl2.reg.sdma_task_runtime_limit_exp);

	l1_normal_ns_interrupt.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_L1_NORMAL_NS_INTERRUPT_ADDR(hwts_base));
	interrupt_info->l1_normal_ns_interrupt = l1_normal_ns_interrupt.value;
	npu_drv_warn("HWTS_L1_NORMAL_NS_INTERRUPT.value = 0x%016llx\n",
			l1_normal_ns_interrupt.value);

	l1_debug_ns_interrupt.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_L1_DEBUG_NS_INTERRUPT_ADDR(hwts_base));
	interrupt_info->l1_debug_ns_interrupt = l1_debug_ns_interrupt.value;
	npu_drv_warn("HWTS_L1_DEBUG_NS_INTERRUPT.value = 0x%016llx\n",
			l1_debug_ns_interrupt.value);

	l1_error_ns_interrupt.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_L1_ERROR_NS_INTERRUPT_ADDR(hwts_base));
	interrupt_info->l1_error_ns_interrupt = l1_error_ns_interrupt.value;
	npu_drv_warn("HWTS_L1_ERROR_NS_INTERRUPT.value = 0x%016llx\n",
			l1_error_ns_interrupt.value);

	l1_dfx_interrupt.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_L1_DFX_INTERRUPT_ADDR(hwts_base));
	interrupt_info->l1_dfx_interrupt = l1_dfx_interrupt.value;
	npu_drv_warn("HWTS_L1_DFX_INTERRUPT.value = 0x%016llx\n",
			l1_dfx_interrupt.value);

	return 0;
}

int npu_hwts_query_sq_info(
	u16 channel_id, struct sq_exception_info *sq_info)
{
	SOC_NPU_HWTS_HWTS_SQ_DB_UNION sq_db = {0};
	SOC_NPU_HWTS_HWTS_SQ_CFG0_UNION sq_cfg0 = {0};
	SOC_NPU_HWTS_HWTS_SQ_CFG1_UNION sq_cfg1 = {0};
	SOC_NPU_HWTS_SQCQ_FSM_STATE_UNION sqcq_fsm_state = {0};
	SOC_NPU_HWTS_SQCQ_FSM_MISC_STATE_UNION sqcq_misc_state = {0};
	SOC_NPU_HWTS_SQCQ_FSM_AIC_OWN_STATE_UNION sqcq_fsm_aic_own_state = {0};
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");
	cond_return_error(sq_info == NULL, -EINVAL, "interrupt_info is NULL\n");

	sq_db.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_DB_ADDR(hwts_base, channel_id));
	sq_info->sq_tail = sq_db.reg.sq_tail;
	npu_drv_warn("HWTS_SQ_DB.sq_tail = 0x%04x\n", sq_db.reg.sq_tail);

	sq_cfg0.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG0_ADDR(hwts_base, channel_id));
	sq_info->sq_head = sq_cfg0.reg.sq_head;
	sq_info->sq_length = sq_cfg0.reg.sq_length;
	npu_drv_warn("HWTS_SQ_CFG0.sq_head = 0x%04x, sq_length = 0x%04x, "
			"sq_en = 0x%01x\n", sq_cfg0.reg.sq_head, sq_cfg0.reg.sq_length,
			sq_cfg0.reg.sq_en);

	sq_cfg1.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HWTS_SQ_CFG1_ADDR(hwts_base, channel_id));
	npu_drv_warn("HWTS_SQ_CFG1.value = 0x%016llx\n", sq_cfg1.value);

	sqcq_fsm_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_STATE_ADDR(hwts_base, channel_id));
	sq_info->sqcq_fsm_state = sqcq_fsm_state.value;
	npu_drv_warn(
		"SQCQ_FSM_STATE.sqcq_fsm_state = 0x%016llx\n",
		sqcq_fsm_state.value);

	sqcq_misc_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_MISC_STATE_ADDR(hwts_base, channel_id));
	npu_drv_warn(
		"SQCQ_FSM_STATE.sqcq_fsm_misc_state = 0x%016llx\n",
		sqcq_misc_state.value);

	sqcq_fsm_aic_own_state.value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_AIC_OWN_STATE_ADDR(hwts_base, channel_id));
	sq_info->aic_own_bitmap = sqcq_fsm_aic_own_state.reg.aic_own_bitmap;
	npu_drv_warn("SQCQ_FSM_AIC_OWN_STATE.aic_own_bitmap = 0x%x\n",
		sqcq_fsm_aic_own_state.reg.aic_own_bitmap);

	return 0;
}

int npu_hwts_query_ispnn_info(u16 channel_id)
{
	u64 value;
	u64 hwts_base = npu_hwts_get_base_addr();

	cond_return_error(hwts_base == 0ULL, -EINVAL, "hwts_base is NULL\n");

	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_SQCQ_FSM_AIC_EXCEPTION_STATE_ADDR(hwts_base, channel_id));
	npu_drv_warn("CH = %u SQCQ_FSM_AIC_EXCEPTION_STATE = 0x%llx\n",
		channel_id, value);

	/* HWTS_HW_HS */
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HW_HS_GLB_CFG_ADDR(hwts_base));
	npu_drv_warn("CH = %u HW_HS_GLB_CFG = 0x%llx\n", channel_id, value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HW_HS_CHANNEL_CFG_ADDR(hwts_base, channel_id));
	npu_drv_warn("CH = %u HW_HS_CHANNEL_CFG = 0x%llx\n", channel_id, value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HW_HS_CHANNEL_CTRL_ADDR(hwts_base, channel_id));
	npu_drv_warn("CH = %u HW_HS_CHANNEL_CTRL = 0x%llx\n", channel_id, value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HW_HS_SHADOW0_ADDR(hwts_base, channel_id));
	npu_drv_warn("CH = %u HW_HS_SHADOW0 = 0x%llx\n", channel_id, value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HW_HS_SHADOW1_ADDR(hwts_base, channel_id));
	npu_drv_warn("CH = %u HW_HS_SHADOW1 = 0x%llx\n", channel_id, value);
	value = readq(
		(const volatile void *)(uintptr_t)SOC_NPU_HWTS_HW_HS_DFX0_ADDR(hwts_base, channel_id));
	npu_drv_warn("CH = %u HW_HS_DFX0 = 0x%llx\n", channel_id, value);

	return 0;
}
