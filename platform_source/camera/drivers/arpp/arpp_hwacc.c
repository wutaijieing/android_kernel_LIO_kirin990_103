/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/timer.h>

#include "arpp_hwacc.h"
#include "arpp_smmu.h"
#include "arpp_isr.h"
#include "arpp_log.h"

#define GET_HIGH_32_ADDR(addr)      ((uint32_t)(((addr) >> 32) & 0xFFFFFFFF))
#define GET_LOW_32_ADDR(addr)       ((uint32_t)((addr) & 0xFFFFFFFF))

#define WAIT_HWACC_INI_TIMES        10
#define MAX_WAIT_CALC_CMPL_TIMES    15
#define MAX_WAIT_ACCESS_PERI_TIMES  15

#define HWACC_CFG_CTRL_PD_EN        (1U << 0)
#define HWACC_CFG_CTRL_CLR_EN       (1U << 1)
#define HWACC_CFG_CTRL_START_EN     (1U << 2)

#define MAX_KF_NUM                  20
#define MAX_FF_NUM                  1400
#define MAX_FF_NUM_FOR_CALC         120
#define MAX_MP_NUM                  3500
#define MAX_ITER_TIMES              20
#define MAX_IDP_NUM                 21000
#define MAX_EFF_PARAM_NUM           (MAX_KF_NUM * 15 + MAX_MP_NUM)
#define ITER_TIMES                  MAX_ITER_TIMES
#define MAX_LTH                     (MAX_KF_NUM * 15 + MAX_IDP_NUM * 2)
#define MAX_PARAM_NUM               ((MAX_KF_NUM + 1) * 16 + \
									(MAX_FF_NUM_FOR_CALC - 1) * 7 + MAX_MP_NUM)
#define MAX_PARAM_BLK               ((MAX_KF_NUM + 1) * 3  + \
									(MAX_FF_NUM_FOR_CALC - 1) + MAX_MP_NUM)
#define MAX_RESID_BLK               (MAX_KF_NUM * 2 + MAX_IDP_NUM)

static int hwacc_module_init(struct arpp_ahc *ahc_info,
	char __iomem *hwacc_ini_cfg, char __iomem *hwacc_cfg,
	unsigned long cmdlist_iova)
{
	int ret;
	int times;
	unsigned int hwacc_ini_state;
	uint32_t cmdlist_addr_low;
	uint32_t cmdlist_addr_high;

	if (ahc_info == NULL || hwacc_ini_cfg == NULL || hwacc_cfg == NULL) {
		hiar_loge("param is NULL");
		return -EINVAL;
	}

	/* step 1 */
	times = 0;
	do {
		hwacc_ini_state = readl(hwacc_ini_cfg + HWACC_INI_SAT);
		if (hwacc_ini_state == 0)
			break;

		udelay(1000);
		++times;
	} while (times <= WAIT_HWACC_INI_TIMES);

	if (hwacc_ini_state != 0) {
		hiar_loge("hwacc state is not 0");
		return -1;
	}

	/* step 2: set cmdlist address */
	cmdlist_addr_low = GET_LOW_32_ADDR(cmdlist_iova);
	cmdlist_addr_high = GET_HIGH_32_ADDR(cmdlist_iova);
	writel(cmdlist_addr_low, hwacc_ini_cfg + DAT0_SRC_ADDR_L);
	writel(cmdlist_addr_high, hwacc_ini_cfg + DAT0_SRC_ADDR_H);

	/* step 3 */
	writel(0x0, hwacc_ini_cfg + HWACC_INI_MOD);

	/* step 4 */
	writel(0x1, hwacc_ini_cfg + HWACC_INI_CFG_EN);

	/* step 5 */
	times = 0;
	do {
		udelay(1000);
		hwacc_ini_state = readl(hwacc_ini_cfg + HWACC_INI_SAT);
		if (hwacc_ini_state == 0)
			break;

		hiar_logi("wait hwacc init times=%d", times);
		++times;
	} while (times <= WAIT_HWACC_INI_TIMES);

	if (hwacc_ini_state != 0) {
		hiar_loge("hwacc init enable failed");
		return -1;
	}

	writel(0x0, hwacc_ini_cfg + HWACC_INI_CFG_EN);

	/* step 6 */
	writel(0x00010000, hwacc_cfg + CTRL_DIS0);
	writel(0x00020000, hwacc_cfg + CTRL_DIS0);

	ret = wait_hwacc_init_completed(ahc_info);
	if (ret != 0)
		hiar_logw("wait hwacc init irq timeout, error=%d", ret);

	return ret;
}

int hwacc_init(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info)
{
	int ret;
	char __iomem *hwacc_ini0_cfg = NULL;
	char __iomem *hwacc0_cfg = NULL;
	struct arpp_iomem *io_info = NULL;
	struct mapped_buffer *cmdlist_buf = NULL;
	struct lba_buffer_info *lba_buf_info = NULL;

	hiar_logi("+");

	if (hwacc_info == NULL || ahc_info == NULL) {
		hiar_loge("hwacc or ahc info is NULL");
		return -EINVAL;
	}

	if (hwacc_info->arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	io_info = &hwacc_info->arpp_dev->io_info;
	hwacc_ini0_cfg = io_info->hwacc_ini0_cfg_base;
	hwacc0_cfg = io_info->hwacc0_cfg_base;
	if (hwacc_ini0_cfg == NULL || hwacc0_cfg == NULL) {
		hiar_loge("hwacc init or cfg is NULL");
		return -EINVAL;
	}

	/* only map once */
	cmdlist_buf = &hwacc_info->cmdlist_buf;
	if (cmdlist_buf->iova == 0) {
		ret = map_cmdlist_buffer(hwacc_info->dev, cmdlist_buf);
		if (ret != 0) {
			hiar_loge("map cmdlist buffer failed, error=%d", ret);
			return ret;
		}
	}

	lba_buf_info = &hwacc_info->lba_buf_info;
	ret = map_buffer_from_user(hwacc_info->dev, lba_buf_info);
	if (ret != 0) {
		hiar_loge("map user buffer failed!");
		goto err_unmap_cmdlist_buffer;
	}

	/* hwacc 0(module_id) init */
	ret = hwacc_module_init(ahc_info, hwacc_ini0_cfg,
		hwacc0_cfg, cmdlist_buf->iova);
	if (ret != 0) {
		hiar_loge("hwacc 0 init failed");
		goto err_unmap_buffer_from_user;
	}

	change_clk_level(hwacc_info->arpp_dev, CLK_LEVEL_LP);

	return ret;

err_unmap_buffer_from_user:
	unmap_buffer_from_user(hwacc_info->dev, lba_buf_info);

err_unmap_cmdlist_buffer:
	unmap_cmdlist_buffer(hwacc_info->dev, cmdlist_buf);

	return ret;
}

static void hwacc_cfg_ctrl(struct arpp_device *arpp_dev, uint32_t data)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *hwacc0_cfg = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;
	hwacc0_cfg = io_info->hwacc0_cfg_base;
	if (hwacc0_cfg == NULL) {
		hiar_logi("hwacc0_cfg is NULL");
		return;
	}

	writel(data, hwacc0_cfg + CRTL_EN0);
}

/**
 * is_arpp_access_peripherals() - Check whether arpp access to peripherals.
 * @io_info: ARPP's registers.
 *
 * Return:
 * 1 - OK
 * 0 - still access peripherals
 */
static int is_arpp_access_peripherals(struct arpp_iomem *io_info)
{
	dw_bus_peri_state peri_stat;
	dw_bus_data_state data_stat;
	char __iomem *pctrl_base = NULL;
	char __iomem *sctrl_base = NULL;

	if (io_info == NULL) {
		hiar_logi("io_info is NULL");
		return 0;
	}

	pctrl_base = io_info->pctrl_base;
	sctrl_base = io_info->sctrl_base;
	if (pctrl_base == NULL || sctrl_base == NULL) {
		hiar_logi("ctrl_base is NULL");
		return 0;
	}

	peri_stat.value = readl(pctrl_base + DW_BUS_TOP_PERI_STAT);
	data_stat.value = readl(sctrl_base + DW_BUS_TOP_DATA_STAT);
	hiar_logi("pctrl[%x], sctrl[%x]", peri_stat.value, data_stat.value);
	if (peri_stat.reg.dw_bus_top_peri_s1_st == 0 &&
		peri_stat.reg.dw_bus_top_peri_s2_st == 0 &&
		peri_stat.reg.dw_bus_top_peri_m1_st == 0 &&
		peri_stat.reg.dw_bus_top_peri_m2_st == 0 &&
		data_stat.reg.dw_bus_top_data_s1_st == 0 &&
		data_stat.reg.dw_bus_top_data_s2_st == 0 &&
		data_stat.reg.dw_bus_top_data_m1_st == 0 &&
		data_stat.reg.dw_bus_top_data_m2_st == 0) {
		return 0;
	}

	return 1;
}

static int wait_arpp_access_peripherals(struct arpp_device *arpp_dev)
{
	int is_accessed;
	uint32_t wait_access_peri_times;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	wait_access_peri_times = 0;
	do {
		is_accessed = is_arpp_access_peripherals(&arpp_dev->io_info);
		if (is_accessed == 0)
			break;

		wait_access_peri_times++;
		udelay(1000);
	} while (wait_access_peri_times <= MAX_WAIT_ACCESS_PERI_TIMES);

	if (is_accessed == 0)
		return 0;

	return -1;
}

int hwacc_deinit(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info)
{
	int ret;
	int cur_level;
	struct arpp_device *arpp_dev = NULL;

	hiar_logi("+");

	if (hwacc_info == NULL || ahc_info == NULL) {
		hiar_loge("hwacc or ahc info is NULL");
		return -EINVAL;
	}

	arpp_dev = hwacc_info->arpp_dev;
	if (arpp_dev != NULL) {
		cur_level = arpp_dev->clk_level;
		change_clk_level(arpp_dev, cur_level);
	}

	/* If power down failed, maybe arpp still access ddr. */
	ret = hwacc_wait_for_power_down(hwacc_info, ahc_info);
	if (ret != 0) {
		hiar_loge("hwacc wait for power down failed! err=%d", ret);
		return ret;
	}

	unmap_cmdlist_buffer(hwacc_info->dev, &hwacc_info->cmdlist_buf);

	unmap_buffer_from_user(hwacc_info->dev, &hwacc_info->lba_buf_info);

	hiar_logi("-");

	return 0;
}

int hwacc_wait_for_power_down(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info)
{
	int ret;

	hiar_logi("+");

	if (hwacc_info == NULL || ahc_info == NULL) {
		hiar_loge("hwacc or ahc info is NULL");
		return -EINVAL;
	}

	hwacc_cfg_ctrl(hwacc_info->arpp_dev, HWACC_CFG_CTRL_PD_EN);

	/*
	 * wait ahc ipc
	 * if calculate lba failed.
	 * power down interrupt may not be received.
	 */
	ret = wait_hwacc_pd_request(ahc_info);
	if (ret != 0)
		hiar_loge("wait pd request timeout!");

	/* wait power down */
	ret = wait_arpp_access_peripherals(hwacc_info->arpp_dev);
	if (ret != 0) {
		hiar_loge("wait access peripherals timeout");
		return ret;
	}

	hiar_logi("-");

	return 0;
}

static int hwacc_process(struct arpp_hwacc *hwacc_info, struct arpp_ahc *ahc_info)
{
	int ret;

	hiar_logi("+");

	if (hwacc_info == NULL) {
		hiar_loge("hwacc_info is NULL");
		return -EINVAL;
	}

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return -EINVAL;
	}

	/* start to calculate */
	hwacc_cfg_ctrl(hwacc_info->arpp_dev, HWACC_CFG_CTRL_START_EN);

	ret = wait_lba_calculation_completed(ahc_info);
	if (ret != 0) {
		hiar_logw("calulate lba timeout");
		if (ahc_info->hwacc_reset_intr != NULL)
			ahc_info->hwacc_reset_intr();
	}

	hiar_logi("-");

	return ret;
}

static void set_lba_first_group_params(
	struct arpp_lba_info *lba_info, char __iomem *lba0_base)
{
	int i;
	int loop_size;
	uint32_t start_offset;
	uint32_t *val = NULL;

	if (lba_info == NULL) {
		hiar_loge("lba_info is NULL!");
		return;
	}

	if (lba0_base == NULL) {
		hiar_loge("lba0_base is NULL!");
		return;
	}

	/* LBA_NUM_KF to LBA_MAX_SS_RETRIES */
	start_offset = LBA_NUM_KF;
	val = &(lba_info->num_kf);
	loop_size = (offsetof(struct arpp_lba_info, num_edge_relpos) -
		offsetof(struct arpp_lba_info, num_kf)) / sizeof(uint32_t) + 1;
	for (i = 0; i < loop_size; ++i) {
		writel(*val, lba0_base + start_offset);
		hiar_logd("1st_1 group: val[%u, %x], offset[%x]",
			*val, *val, start_offset);
		start_offset += sizeof(uint32_t);
		++val;
	}

	/* LBA_LOSS_PRV to LBA_CALIB_EXT */
	start_offset = LBA_LOSS_PRV;
	val = lba_info->init_prv_loss;
	loop_size =
		(offsetof(struct arpp_lba_info, calib_ext[23]) -
		offsetof(struct arpp_lba_info, init_prv_loss[0])) /
		sizeof(uint32_t) + 1;
	for (i = 0; i < loop_size; ++i) {
		writel(*val, lba0_base + start_offset);
		hiar_logd("1st_2 group: val[%u, %x], offset[%x]",
			*val, *val, start_offset);
		start_offset += sizeof(uint32_t);
		++val;
	}
}

static void set_lba_second_group_params(struct arpp_lba_info *lba_info,
	char __iomem *lba0_base, unsigned long inout_iova)
{
	int i;
	uint32_t start_offset;
	int loop_size;
	uint64_t inout_addr;
	uint32_t inout_high_addr;
	uint32_t inout_low_addr;
	uint32_t *val = NULL;

	if (lba0_base == NULL) {
		hiar_loge("lba0_base is NULL!");
		return;
	}

	if (lba_info == NULL) {
		hiar_loge("lba_info is NULL!");
		return;
	}

	inout_addr = inout_iova;

	/* LBA_PARAMETER_ADDR to LBA_EFFECTIVE_OFFSET_ADDR */
	start_offset = LBA_PARAMETER_ADDR;
	val = &(lba_info->param_offset);
	loop_size =
		(offsetof(struct arpp_lba_info, eff_offset_param_offset) -
		offsetof(struct arpp_lba_info, param_offset)) /
		sizeof(uint32_t) + 1;
	for (i = 0; i < loop_size; ++i, ++val) {
		inout_low_addr = GET_LOW_32_ADDR(inout_addr + *val);
		inout_high_addr = GET_HIGH_32_ADDR(inout_addr + *val);
		writel(inout_low_addr, lba0_base + start_offset);
		hiar_logd("2nd_1 group: val[%u, %x], offset[%x]",
			*val, *val, start_offset);
		start_offset += sizeof(uint32_t);
		writel(inout_high_addr, lba0_base + start_offset);
		start_offset += sizeof(uint32_t);
	}

	/* LBA_MSK_HBL_ADDR to LBA_NC_MP_STA_ADDR */
	start_offset = LBA_MSK_HBL_ADDR;
	val = &(lba_info->msk_hbl_offset);
	loop_size =
		(offsetof(struct arpp_lba_info, mp_stat_nc_offset) -
		offsetof(struct arpp_lba_info, msk_hbl_offset)) /
		sizeof(uint32_t) + 1;
	for (i = 0; i < loop_size; ++i, ++val) {
		inout_low_addr = GET_LOW_32_ADDR(inout_addr + *val);
		inout_high_addr = GET_HIGH_32_ADDR(inout_addr + *val);
		writel(inout_low_addr, lba0_base + start_offset);
		hiar_logd("2nd_2 group: val[%u, %x], offset[%x]",
			*val, *val, start_offset);
		start_offset += sizeof(uint32_t);
		writel(inout_high_addr, lba0_base + start_offset);
		start_offset += sizeof(uint32_t);
	}

	/* LBA_EDGEVERTEX_RELPOS_ADDR to LBA_M2_NC_RCBI_ADDR */
	start_offset = LBA_EDGEVERTEX_RELPOS_ADDR;
	val = &(lba_info->edge_vertex_relpos_offset);
	loop_size =
		(offsetof(struct arpp_lba_info, m2_nc_rcbi_offset) -
		offsetof(struct arpp_lba_info, edge_vertex_relpos_offset)) /
		sizeof(uint32_t) + 1;
	for (i = 0; i < loop_size; ++i, ++val) {
		inout_low_addr = GET_LOW_32_ADDR(inout_addr + *val);
		inout_high_addr = GET_HIGH_32_ADDR(inout_addr + *val);
		writel(inout_low_addr, lba0_base + start_offset);
		hiar_logd("2nd_3 group: val[%u, %x], offset[%x]",
			*val, *val, start_offset);
		start_offset += sizeof(uint32_t);
		writel(inout_high_addr, lba0_base + start_offset);
		start_offset += sizeof(uint32_t);
	}
}

static void set_lba_intermediate_buffers_address(
	struct lba_buffer_info *lba_buf_info, char __iomem *lba0_base)
{
	uint64_t swap_addr;
	uint64_t free_addr;
	uint64_t out_addr;
	uint32_t swap_high_addr;
	uint32_t swap_low_addr;
	uint32_t free_high_addr;
	uint32_t free_low_addr;
	uint32_t out_high_addr;
	uint32_t out_low_addr;

	if (lba_buf_info == NULL) {
		hiar_loge("lba_buf_info is NULL!");
		return;
	}

	if (lba0_base == NULL) {
		hiar_loge("lba0_base is NULL!");
		return;
	}

	swap_addr = lba_buf_info->swap_buf.iova;
	free_addr = lba_buf_info->free_buf.iova;
	out_addr = lba_buf_info->out_buf.iova;
	swap_high_addr = GET_HIGH_32_ADDR(swap_addr);
	swap_low_addr = GET_LOW_32_ADDR(swap_addr);
	free_high_addr = GET_HIGH_32_ADDR(free_addr);
	free_low_addr = GET_LOW_32_ADDR(free_addr);
	out_high_addr = GET_HIGH_32_ADDR(out_addr);
	out_low_addr = GET_LOW_32_ADDR(out_addr);

	/* LBA_SWAP_BASE_ADDR to LBA_OUT_BASE_ADDR */
	writel(swap_low_addr, lba0_base + LBA_SWAP_BASE_ADDR);
	writel(swap_high_addr, lba0_base + LBA_SWAP_BASE_ADDR +
		sizeof(uint32_t));
	writel(free_low_addr, lba0_base + LBA_FREE_BASE_ADDR);
	writel(free_high_addr, lba0_base + LBA_FREE_BASE_ADDR +
		sizeof(uint32_t));
	writel(out_low_addr, lba0_base + LBA_OUT_BASE_ADDR);
	writel(out_high_addr, lba0_base + LBA_OUT_BASE_ADDR +
		sizeof(uint32_t));
}

static void set_lba_reg_info(struct arpp_hwacc *hwacc_info,
	struct arpp_lba_info *lba_info)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *lba0_base = NULL;
	struct lba_buffer_info *lba_buf_info = NULL;

	if (hwacc_info == NULL) {
		hiar_loge("hwacc_info is NULL!");
		return;
	}

	if (lba_info == NULL) {
		hiar_loge("lba_info is NULL!");
		return;
	}

	if (hwacc_info->arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL!");
		return;
	}

	io_info = &hwacc_info->arpp_dev->io_info;
	lba0_base = io_info->lba0_base;
	if (lba0_base == NULL) {
		hiar_logi("lba0_base is NULL");
		return;
	}

	lba_buf_info = &hwacc_info->lba_buf_info;

	set_lba_first_group_params(lba_info, lba0_base);
	set_lba_second_group_params(lba_info, lba0_base,
		lba_buf_info->inout_buf.iova);
	set_lba_intermediate_buffers_address(lba_buf_info, lba0_base);
}

static int check_lba_num_info(struct arpp_lba_info *lba_info)
{
	if (unlikely(lba_info == NULL)) {
		hiar_loge("lba_info is NULL!");
		return -EINVAL;
	}

	if (unlikely(lba_info->num_kf > MAX_KF_NUM)) {
		hiar_loge("num_kf[%d] is invalid", lba_info->num_kf);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_ff > MAX_FF_NUM)) {
		hiar_loge("num_ff[%d] is invalid", lba_info->num_ff);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_mp > MAX_MP_NUM)) {
		hiar_loge("num_mp[%d] is invalid", lba_info->num_mp);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_max_iter > MAX_ITER_TIMES)) {
		hiar_loge("num_max_iter[%d] is invalid",
			lba_info->num_max_iter);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_edge > MAX_IDP_NUM)) {
		hiar_loge("num_edge[%d] is invalid", lba_info->num_edge);
		return -EINVAL;
	}

	return 0;
}

static int check_lba_param_info(struct arpp_lba_info *lba_info)
{
	if (unlikely(lba_info == NULL)) {
		hiar_loge("lba_info is NULL!");
		return -EINVAL;
	}

	if (unlikely(lba_info->num_param > MAX_PARAM_NUM)) {
		hiar_loge("num_param[%d] is invalid", lba_info->num_param);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_eff_param > MAX_EFF_PARAM_NUM)) {
		hiar_loge("num_eff_param[%d] is invalid",
			lba_info->num_eff_param);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_residual > MAX_LTH)) {
		hiar_loge("num_residual[%d] is invalid",
			lba_info->num_residual);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_res_blk > MAX_RESID_BLK)) {
		hiar_loge("num_res_blk[%d] is invalid", lba_info->num_res_blk);
		return -EINVAL;
	}
	if (unlikely(lba_info->num_param_blk > MAX_PARAM_BLK)) {
		hiar_loge("num_param_blk[%d] is invalid",
			lba_info->num_param_blk);
		return -EINVAL;
	}

	return 0;
}

static int check_lba_param_1st_group_is_zero(struct arpp_lba_info *lba_info)
{
	if (unlikely(lba_info == NULL)) {
		hiar_loge("lba_info is NULL!");
		return -EINVAL;
	}
	if (unlikely((lba_info->init_prv_loss[0] == 0) &&
		(lba_info->init_prv_loss[1] == 0))) {
		hiar_loge("init_prv_loss is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->init_bias_loss[0] == 0) &&
		(lba_info->init_bias_loss[1] == 0))) {
		hiar_loge("init_bias_loss is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->init_idp_loss[0] == 0) &&
		(lba_info->init_idp_loss[1] == 0))) {
		hiar_loge("init_idp_loss is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->grad_tol[0] == 0) &&
		(lba_info->grad_tol[1] == 0))) {
		hiar_loge("grad_tol is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->param_tol[0] == 0) &&
		(lba_info->param_tol[1] == 0))) {
		hiar_loge("param_tol is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->lose_func_tol[0] == 0) &&
		(lba_info->lose_func_tol[1] == 0))) {
		hiar_loge("lose_func_tol is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->min_rel_dec[0] == 0) &&
		(lba_info->min_rel_dec[1] == 0))) {
		hiar_loge("min_rel_dec is 0");
		return -EINVAL;
	}

	return 0;
}

static int check_lba_param_2nd_group_is_zero(struct arpp_lba_info *lba_info)
{
	if (unlikely(lba_info == NULL)) {
		hiar_loge("lba_info is NULL!");
		return -EINVAL;
	}
	if (unlikely((lba_info->min_trust_rgn_rad[0] == 0) &&
		(lba_info->min_trust_rgn_rad[1] == 0))) {
		hiar_loge("min_trust_rgn_rad is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->init_radius[0] == 0) &&
		(lba_info->init_radius[1] == 0))) {
		hiar_loge("init_radius is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->max_radius[0] == 0) &&
		(lba_info->max_radius[1] == 0))) {
		hiar_loge("max_radius is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->min_diagonal[0] == 0) &&
		(lba_info->min_diagonal[1] == 0))) {
		hiar_loge("min_diagonal is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->max_diagonal[0] == 0) &&
		(lba_info->max_diagonal[1] == 0))) {
		hiar_loge("max_diagonal is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->diagonal_factor[0] == 0) &&
		(lba_info->diagonal_factor[1] == 0))) {
		hiar_loge("diagonal_factor is 0");
		return -EINVAL;
	}
	if (unlikely((lba_info->fixed_cost_val[0] == 0) &&
		(lba_info->fixed_cost_val[1] == 0))) {
		hiar_loge("fixed_cost_val is 0");
		return -EINVAL;
	}

	return 0;
}

static int check_lba_param_is_zero(struct arpp_lba_info *lba_info)
{
	int ret;
	ret = check_lba_param_1st_group_is_zero(lba_info);
	ret += check_lba_param_2nd_group_is_zero(lba_info);
	return ret;
}

int do_lba_calculation(struct arpp_hwacc *hwacc_info,
	struct arpp_ahc *ahc_info, struct arpp_lba_info *lba_info)
{
	int ret;
	int cur_level;
	struct arpp_device *arpp_dev = NULL;

	if (unlikely(hwacc_info == NULL)) {
		hiar_loge("hwacc_info is NULL!");
		return -EINVAL;
	}

	if (unlikely(ahc_info == NULL)) {
		hiar_loge("ahc_info is NULL!");
		return -EINVAL;
	}

	if (unlikely(lba_info == NULL)) {
		hiar_loge("lba_info is NULL!");
		return -EINVAL;
	}

	arpp_dev = hwacc_info->arpp_dev;
	if (unlikely(arpp_dev == NULL)) {
		hiar_loge("arpp_dev is NULL!");
		return -EINVAL;
	}

	ret = check_lba_num_info(lba_info);
	if (unlikely(ret != 0)) {
		hiar_loge("number is invalid!");
		return -EINVAL;
	}

	ret = check_lba_param_info(lba_info);
	if (unlikely(ret != 0)) {
		hiar_loge("param is invalid!");
		return -EINVAL;
	}

	ret = check_lba_param_is_zero(lba_info);
	if (unlikely(ret != 0)) {
		hiar_loge("param is zero!");
		return -EINVAL;
	}

	/* set to current work freq */
	cur_level = arpp_dev->clk_level;
	change_clk_level(arpp_dev, cur_level);

	set_lba_reg_info(hwacc_info, lba_info);

	ret = hwacc_process(hwacc_info, ahc_info);
	if (unlikely(ret != 0))
		hiar_loge("hwacc process failed, error=%d", ret);

	/* Set freq to low power, after process done */
	change_clk_level(arpp_dev, CLK_LEVEL_LP);

	return ret;
}
