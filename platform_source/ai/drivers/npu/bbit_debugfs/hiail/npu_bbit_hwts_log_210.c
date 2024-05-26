/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: for hwts profiling
 */
#include "npu_bbit_hwts_config.h"
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include "npu_platform_register.h"
#include "npu_hwts_plat.h"
#include "npu_adapter.h"
#include "npu_platform.h"
#include "npu_iova.h"
#include "npu_pm_framework.h"
#include "npu_log.h"

prof_log_buff_t log_buff = {0};

u64 get_hwts_base_addr(void)
{
	u64 hwts_base = 0;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail.\n");
		return 0;
	}
	hwts_base = (u64) (uintptr_t) plat_info->dts_info.reg_vaddr[NPU_REG_HWTS_BASE];

	return hwts_base;
}

u64 get_aicore_base_addr(u8 aic_idx)
{
	u64 aicore_base = 0;
	struct npu_platform_info *plat_info = npu_plat_get_info();

	if (plat_info == NULL) {
		npu_drv_err("get plat info fail.\n");
		return 0;
	}
	if (aic_idx == 0)
		aicore_base = (u64) (uintptr_t) plat_info->dts_info.reg_vaddr[NPU_REG_AIC0_BASE];
	else
		aicore_base = (u64) (uintptr_t) plat_info->dts_info.reg_vaddr[NPU_REG_AIC1_BASE];

	return aicore_base;
}

static int log_buff_alloc(u32 len, vir_addr_t *virt_addr, unsigned long *iova)
{
	int ret = 0;

	if (virt_addr == NULL || iova == NULL) {
		npu_drv_err("virt_addr or iova is null\n");
		return -1;
	}
	ret = npu_iova_alloc(virt_addr, len);
	if (ret != 0) {
		npu_drv_err("fail, ret=%d\n", ret);
		return ret;
	}
	ret = npu_iova_map(*virt_addr, iova);
	if (ret != 0) {
		npu_drv_err("prof_buff_map fail, ret=%d\n", ret);
		npu_iova_free(*virt_addr);
	}
	return ret;
}

void log_buff_free(vir_addr_t virt_addr, unsigned long iova)
{
	int ret = npu_iova_unmap(virt_addr, iova);
	if (ret != 0) {
		npu_drv_err("npu_iova_unmap fail, ret=%d\n", ret);
		return;
	}
	if (virt_addr != 0)
		npu_iova_free(virt_addr);
}

void enable_hwts_prof_log(void)
{
	SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_UNION log_ctrl0 = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL1_UNION log_ctrl1 = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_UNION log_buff_base0 = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR1_UNION log_buff_base1 = {0};
	u64 hwts_base = get_hwts_base_addr();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}
	log_buff.size = 0x200000;
	log_buff_alloc(log_buff.size, &log_buff.virt_addr, &log_buff.iova_addr);
	npu_drv_debug("alloc log buf_size=%llu\n", log_buff.size);

	read_uint32(log_ctrl0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_ADDR(hwts_base));
	read_uint32(log_ctrl1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL1_ADDR(hwts_base));
	log_ctrl0.reg.task_log_en = 0x1;
	log_ctrl0.reg.block_log_en = 0x1;
	log_ctrl0.reg.pmu_log_en = 0x1;
	log_ctrl0.reg.log_buf_length = log_buff.size / HWTS_LOG_SIZE;
	log_ctrl1.reg.log_af_threshold = log_buff.size / HWTS_LOG_SIZE;
	write_uint32(log_ctrl0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_ADDR(hwts_base));
	write_uint32(log_ctrl1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL1_ADDR(hwts_base));

	read_uint32(log_buff_base0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_ADDR(hwts_base));
	read_uint32(log_buff_base1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_ADDR(hwts_base));
	log_buff_base1.reg.log_base_addr_is_virtual = 0x1;
	log_buff_base0.reg.log_base_addr_l = log_buff.iova_addr & UINT32_MAX;
	log_buff_base1.reg.log_base_addr_h = (uint32_t)((uint64_t)log_buff.iova_addr >> 32);
	write_uint32(log_buff_base0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_ADDR(hwts_base));
	write_uint32(log_buff_base1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR1_ADDR(hwts_base));
}

void disable_hwts_prof_log(void)
{
	u64 hwts_base = get_hwts_base_addr();
	SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_UNION log_ctrl0 = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL1_UNION log_ctrl1 = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_UNION log_buff_base0 = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR1_UNION log_buff_base1 = {0};

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}
	log_buff_free(log_buff.virt_addr, log_buff.iova_addr);
	npu_drv_debug("free log buf_size=%llu\n", log_buff.size);

	read_uint32(log_ctrl0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_ADDR(hwts_base));
	read_uint32(log_ctrl1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL1_ADDR(hwts_base));
	log_ctrl0.reg.task_log_en = 0x0;
	log_ctrl0.reg.block_log_en = 0x0;
	log_ctrl0.reg.pmu_log_en = 0x0;
	log_ctrl0.reg.log_buf_length = 0x0;
	log_ctrl1.reg.log_af_threshold = 0x0;
	write_uint32(log_ctrl0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_ADDR(hwts_base));
	write_uint32(log_ctrl1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL1_ADDR(hwts_base));

	read_uint32(log_buff_base0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_ADDR(hwts_base));
	read_uint32(log_buff_base1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR1_ADDR(hwts_base));
	log_buff_base1.reg.log_base_addr_is_virtual = 0x0;
	log_buff_base0.reg.log_base_addr_l = 0x0;
	log_buff_base1.reg.log_base_addr_h = 0x0;
	write_uint32(log_buff_base0.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR0_ADDR(hwts_base));
	write_uint32(log_buff_base1.value, SOC_NPU_HWTS_HWTS_DFX_LOG_BASE_ADDR1_ADDR(hwts_base));
}

void enable_sq_log(u8 sq_idx)
{
	SOC_NPU_HWTS_HWTS_SQ_CFG1_UNION sq_cfg1 = {0};
	u64 hwts_base = get_hwts_base_addr();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}
	read_uint64(sq_cfg1.value,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_ADDR(hwts_base, sq_idx));
	sq_cfg1.reg.sq_log_en = 0x1;
	write_uint64(sq_cfg1.value,
		SOC_NPU_HWTS_HWTS_SQ_CFG1_ADDR(hwts_base, sq_idx));
}

void disable_sq_log(u8 sq_idx)
{
	SOC_NPU_HWTS_HWTS_SQ_CFG1_UNION sq_cfg1 = {0};
	u64 hwts_base = get_hwts_base_addr();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return;
	}
	read_uint64(sq_cfg1.value, SOC_NPU_HWTS_HWTS_SQ_CFG1_ADDR(hwts_base,
		sq_idx));
	sq_cfg1.reg.sq_log_en = 0x0;
	write_uint64(sq_cfg1.value, SOC_NPU_HWTS_HWTS_SQ_CFG1_ADDR(hwts_base,
		sq_idx));
}

void set_aicore_pmu_events(u64 aicore_base, pmu_config_t pmu)
{
	SOC_NPU_AICORE_PMU_CNT0_IDX_UNION cnt0_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT1_IDX_UNION cnt1_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT2_IDX_UNION cnt2_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT3_IDX_UNION cnt3_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT4_IDX_UNION cnt4_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT5_IDX_UNION cnt5_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT6_IDX_UNION cnt6_idx1 = {0};
	SOC_NPU_AICORE_PMU_CNT7_IDX_UNION cnt7_idx1 = {0};

	read_uint32(cnt0_idx1.value, SOC_NPU_AICORE_PMU_CNT0_IDX_ADDR(aicore_base));
	cnt0_idx1.reg.pmu_cnt0_idx = pmu.events[0];
	write_uint32(cnt0_idx1.value, SOC_NPU_AICORE_PMU_CNT0_IDX_ADDR(aicore_base));

	read_uint32(cnt1_idx1.value, SOC_NPU_AICORE_PMU_CNT1_IDX_ADDR(aicore_base));
	cnt1_idx1.reg.pmu_cnt1_idx = pmu.events[1];
	write_uint32(cnt1_idx1.value, SOC_NPU_AICORE_PMU_CNT1_IDX_ADDR(aicore_base));

	read_uint32(cnt2_idx1.value, SOC_NPU_AICORE_PMU_CNT2_IDX_ADDR(aicore_base));
	cnt2_idx1.reg.pmu_cnt2_idx = pmu.events[2];
	write_uint32(cnt2_idx1.value, SOC_NPU_AICORE_PMU_CNT2_IDX_ADDR(aicore_base));

	read_uint32(cnt3_idx1.value, SOC_NPU_AICORE_PMU_CNT3_IDX_ADDR(aicore_base));
	cnt3_idx1.reg.pmu_cnt3_idx = pmu.events[3];
	write_uint32(cnt3_idx1.value, SOC_NPU_AICORE_PMU_CNT3_IDX_ADDR(aicore_base));

	read_uint32(cnt4_idx1.value, SOC_NPU_AICORE_PMU_CNT4_IDX_ADDR(aicore_base));
	cnt4_idx1.reg.pmu_cnt4_idx = pmu.events[4];
	write_uint32(cnt4_idx1.value, SOC_NPU_AICORE_PMU_CNT4_IDX_ADDR(aicore_base));

	read_uint32(cnt5_idx1.value, SOC_NPU_AICORE_PMU_CNT5_IDX_ADDR(aicore_base));
	cnt5_idx1.reg.pmu_cnt5_idx = pmu.events[5];
	write_uint32(cnt5_idx1.value, SOC_NPU_AICORE_PMU_CNT5_IDX_ADDR(aicore_base));

	read_uint32(cnt6_idx1.value, SOC_NPU_AICORE_PMU_CNT6_IDX_ADDR(aicore_base));
	cnt6_idx1.reg.pmu_cnt6_idx = pmu.events[6];
	write_uint32(cnt6_idx1.value, SOC_NPU_AICORE_PMU_CNT6_IDX_ADDR(aicore_base));

	read_uint32(cnt7_idx1.value, SOC_NPU_AICORE_PMU_CNT7_IDX_ADDR(aicore_base));
	cnt7_idx1.reg.pmu_cnt7_idx = pmu.events[7];
	write_uint32(cnt7_idx1.value, SOC_NPU_AICORE_PMU_CNT7_IDX_ADDR(aicore_base));
}

void enable_aicore_pmu_ctrl(u64 aicore_base)
{
	SOC_NPU_AICORE_PMU_CTRL_0_UNION pmu_ctrl = {0};
	SOC_NPU_AICORE_PMU_START_CNT_CYC_0_UNION start_cnt0 = {0};
	SOC_NPU_AICORE_PMU_START_CNT_CYC_1_UNION start_cnt1 = {0};
	SOC_NPU_AICORE_PMU_STOP_CNT_CYC_0_UNION stop_cnt0 = {0};
	SOC_NPU_AICORE_PMU_STOP_CNT_CYC_1_UNION stop_cnt1 = {0};

	read_uint32(pmu_ctrl.value, SOC_NPU_AICORE_PMU_CTRL_0_ADDR(aicore_base));
	pmu_ctrl.reg.pmu_en = 0x1;
	write_uint32(pmu_ctrl.value, SOC_NPU_AICORE_PMU_CTRL_0_ADDR(aicore_base));

	start_cnt0.reg.pmu_start_cnt_cyc_0 = 0x0;
	start_cnt1.reg.pmu_start_cnt_cyc_1 = 0x0;
	write_uint32(start_cnt0.value, SOC_NPU_AICORE_PMU_START_CNT_CYC_0_ADDR(aicore_base));
	write_uint32(start_cnt1.value, SOC_NPU_AICORE_PMU_START_CNT_CYC_1_ADDR(aicore_base));

	stop_cnt0.reg.pmu_stop_cnt_cyc_0 = 0xFFFFFFFF;
	stop_cnt1.reg.pmu_stop_cnt_cyc_1 = 0xFFFFFFFF;
	write_uint32(stop_cnt0.value, SOC_NPU_AICORE_PMU_STOP_CNT_CYC_0_ADDR(aicore_base));
	write_uint32(stop_cnt1.value, SOC_NPU_AICORE_PMU_STOP_CNT_CYC_1_ADDR(aicore_base));
}

void enable_aicore_pmu(pmu_config_t pmu)
{
	u64 aicore_base = 0;
	int i;

	for (i = 0; i < AICORE_COUNT; i++) {
		if (pmu.aic_on[i] == 0)
			continue;

		aicore_base = get_aicore_base_addr(i);
		cond_return_void(aicore_base == 0, "enable pmu get aicore%d base failed.\n", i);

		enable_aicore_pmu_ctrl(aicore_base);
		set_aicore_pmu_events(aicore_base, pmu);
	}
}

void disable_aicore_pmu_ctrl(u64 aicore_base)
{
	SOC_NPU_AICORE_PMU_CTRL_0_UNION pmu_ctrl = {0};
	SOC_NPU_AICORE_PMU_STOP_CNT_CYC_0_UNION stop_cnt0 = {0};
	SOC_NPU_AICORE_PMU_STOP_CNT_CYC_1_UNION stop_cnt1 = {0};

	read_uint32(pmu_ctrl.value, SOC_NPU_AICORE_PMU_CTRL_0_ADDR(aicore_base));
	pmu_ctrl.reg.pmu_en = 0x0;
	write_uint32(pmu_ctrl.value, SOC_NPU_AICORE_PMU_CTRL_0_ADDR(aicore_base));

	stop_cnt0.reg.pmu_stop_cnt_cyc_0 = 0x0;
	stop_cnt1.reg.pmu_stop_cnt_cyc_1 = 0x0;
	write_uint32(stop_cnt0.value, SOC_NPU_AICORE_PMU_STOP_CNT_CYC_0_ADDR(aicore_base));
	write_uint32(stop_cnt1.value, SOC_NPU_AICORE_PMU_STOP_CNT_CYC_1_ADDR(aicore_base));
}

void disable_aicore_pmu(pmu_config_t pmu)
{
	u64 aicore_base = 0;
	int i;

	for (i = 0; i < AICORE_COUNT; i++) {
		if (pmu.aic_on[i] == 0)
			continue;

		aicore_base = get_aicore_base_addr(i);
		cond_return_void(aicore_base == 0, "disable pmu get aicore%d base failed.\n", i);

		disable_aicore_pmu_ctrl(aicore_base);
	}
}

int enable_ispnn_profiling(pmu_config_t pmu)
{
	int idx;

	enable_hwts_prof_log();
	for (idx = 0; idx < ISPNN_HWTS_SQ_COUNT; idx++)
		enable_sq_log(idx);

	enable_aicore_pmu(pmu);

	return 0;
}

int disable_ispnn_profiling(pmu_config_t pmu)
{
	int idx;

	disable_hwts_prof_log();
	for (idx = 0; idx < ISPNN_HWTS_SQ_COUNT; idx++)
		disable_sq_log(idx);

	disable_aicore_pmu(pmu);

	return 0;
}

int get_ispnn_profiling_log(const char __user *prof_buf)
{
	int ret = 0;
	u64 log_addr = 0;
	u32 log_size = 0;
	u16 log_wptr = 0;
	u16 log_rptr = 0;
	u32 log_buf_len = 0;
	SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_UNION log_ctrl = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_PTR_UNION log_pos_ptr = {0};
	u64 hwts_base = get_hwts_base_addr();

	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return -1;
	}

	read_uint32(log_ctrl.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_ADDR(hwts_base));
	log_buf_len = log_ctrl.reg.log_buf_length;

	read_uint32(log_pos_ptr.value, SOC_NPU_HWTS_HWTS_DFX_LOG_PTR_ADDR(hwts_base));
	log_wptr = log_pos_ptr.reg.log_wptr;
	log_rptr = log_pos_ptr.reg.log_rptr;
	if (log_wptr == log_rptr) {
		npu_drv_warn("There is no task log data\n");
		return 0;
	}

	log_addr = log_buff.virt_addr + log_rptr * sizeof(hwts_task_log_t);
	if (log_wptr < log_rptr) {
		log_size = (log_buf_len - log_rptr) * sizeof(hwts_task_log_t);
		ret = copy_to_user_safe((void *)prof_buf,
			(void *)(uintptr_t)log_addr, log_size);
		if (ret != 0) {
			npu_drv_err("copy to user buffer failed.\n");
			return -1;
		}
		log_rptr = 0;
		log_addr = log_buff.virt_addr;
	}
	log_size = (log_wptr - log_rptr) * sizeof(hwts_task_log_t);
	npu_drv_debug("copy hwts log to user");
	ret = copy_to_user_safe((void *)prof_buf, (void *)(uintptr_t)log_addr,
		log_size);
	if (ret != 0) {
		npu_drv_err("copy to user buffer failed.\n");
		return -1;
	}
	log_pos_ptr.reg.log_rptr = log_wptr;
	write_uint64(log_pos_ptr.value, SOC_NPU_HWTS_HWTS_DFX_LOG_PTR_ADDR(hwts_base));

	return 0;
}

int print_ispnn_profiling_log(void)
{
	u64 log_base_addr = log_buff.virt_addr;
	u32 log_buf_len = 0;
	u16 log_wptr = 0;
	u16 log_rptr = 0;
	u8 log_task_en = 0;
	SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_UNION log_ctrl = {0};
	SOC_NPU_HWTS_HWTS_DFX_LOG_PTR_UNION log_pos_ptr = {0};
	u64 hwts_base = get_hwts_base_addr();
	if (hwts_base == 0) {
		npu_drv_err("get hwts base failed.\n");
		return -1;
	}

	read_uint32(log_ctrl.value, SOC_NPU_HWTS_HWTS_DFX_LOG_CTRL0_ADDR(hwts_base));
	log_task_en = log_ctrl.reg.task_log_en;
	log_buf_len = log_ctrl.reg.log_buf_length;
	read_uint32(log_pos_ptr.value, SOC_NPU_HWTS_HWTS_DFX_LOG_PTR_ADDR(hwts_base));
	log_wptr = log_pos_ptr.reg.log_wptr;
	log_rptr = log_pos_ptr.reg.log_rptr;

	if (log_wptr == log_rptr || log_buf_len == 0) {
		npu_drv_warn("There is no task log data\n");
		return 0;
	}
	npu_drv_debug("log_rptr=%u, log_wptr=%u, log_buf_len=%u, log_task_en=%u\n",
		log_rptr, log_wptr, log_buf_len, log_task_en);

	while (log_wptr != log_rptr) {
		u64 log_addr = log_base_addr + log_rptr * sizeof(hwts_task_log_t);
		hwts_task_log_t *task_log = (hwts_task_log_t *)(uintptr_t)log_addr;

		unused(task_log);
		npu_drv_debug("streamid=%u, type=%u, taskid=%u, coreid=%u, "
			"syscnt_hi=%u, syscnt_lo=%u, "
			"cyc_hi=%u, cyc_lo=%u, "
			"pmu0=%u, pmu1=%u, pmu2=%u, pmu3=%u, pmu4=%u, pmu5=%u, pmu6=%u, pmu7=%u\n",
			task_log->streamid, task_log->type, task_log->taskid,
			task_log->res0_coreid, task_log->syscnt_hi,
			task_log->syscnt_lo, task_log->cyc_hi, task_log->cyc_lo,
			task_log->pmu0, task_log->pmu1, task_log->pmu2,
			task_log->pmu3, task_log->pmu4, task_log->pmu5,
			task_log->pmu6, task_log->pmu7);
		log_rptr = (log_rptr + 1) % log_buf_len;
	}

	log_pos_ptr.reg.log_rptr = log_rptr;
	write_uint32(log_pos_ptr.value, SOC_NPU_HWTS_HWTS_DFX_LOG_PTR_ADDR(hwts_base));

	return 0;
}

int npu_bbit_enable_prof(struct user_msg msg)
{
	int ret = 0;
	pmu_config_t pmu = {{0}, {0}};

	cond_return_error(msg.buf == NULL, -1, "enable prof msg.buf is null\n");

	ret = copy_from_user_safe(&pmu, msg.buf, sizeof(pmu));
	cond_return_error(ret != 0, -1, "enable prof copy failed ret %d.\n", ret);

	ret = enable_ispnn_profiling(pmu);
	if (ret != 0)
		npu_drv_err("enable_ispnn_profiling ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_disable_prof(struct user_msg msg)
{
	int ret = 0;
	pmu_config_t pmu = {{0}, {0}};

	cond_return_error(msg.buf == NULL, -1, "disable prof msg.buf is null\n");

	ret = copy_from_user_safe(&pmu, msg.buf, sizeof(pmu));
	cond_return_error(ret != 0, -1, "disable prof copy failed ret %d.\n", ret);

	ret = disable_ispnn_profiling(pmu);
	if (ret != 0)
		npu_drv_err("disable_ispnn_profiling ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_get_prof_log(struct user_msg msg)
{
	int ret = 0;

	if (msg.buf == NULL) {
		npu_drv_err("msg.buf is NULL.\n");
		return -1;
	}
	ret = get_ispnn_profiling_log((char __user *)msg.buf);
	if (ret != 0)
		npu_drv_err("get_ispnn_profiling_log ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

int npu_bbit_print_prof_log(struct user_msg msg)
{
	int ret = print_ispnn_profiling_log();
	if (ret != 0)
		npu_drv_err("print_ispnn_profiling_log ret %d", ret);
	unused(msg);

	npu_bbit_set_result(ret);
	return 0;
}
