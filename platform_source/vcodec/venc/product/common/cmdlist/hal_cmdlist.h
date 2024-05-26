/*
 * hal_cmdlist.h
 *
 * cmdlist config
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef HAL_CMDLIST_H
#define HAL_CMDLIST_H

#include "drv_cmdlist.h"
#include "soc_venc_reg_interface.h"

#define CMDLIST_DUMMY_DATA 0xFF1FFFFD
#define CMDLIST_WRCMD_MASK 0x1FFFFC
#define CMDLIST_OFFSET (offsetof(S_HEVC_AVC_REGS_TYPE, CMD_LIST))

/* The cmdlist_read_back_info_t modification must be synchronized hal_cmdlist_update_len1. */
typedef volatile struct cmdlist_read_back_info {
	volatile U_VEDU_VCPI_PICSIZE_PIX VEDU_VCPI_PICSIZE_PIX;
	volatile U_VEDU_VCPI_MODE VEDU_VCPI_MODE;
	volatile U_FUNC_SEL_OPT_4X4_CNT FUNC_SEL_OPT_4X4_CNT;
	volatile U_FUNC_SEL_INTRA_OPT_8X8_CNT FUNC_SEL_INTRA_OPT_8X8_CNT;
	volatile U_FUNC_SEL_INTRA_OPT_16X16_CNT FUNC_SEL_INTRA_OPT_16X16_CNT;
	volatile U_FUNC_SEL_INTRA_OPT_32X32_CNT FUNC_SEL_INTRA_OPT_32X32_CNT;
	volatile U_FUNC_PME_MADI_SUM FUNC_PME_MADI_SUM;
	volatile U_FUNC_PME_MADP_SUM FUNC_PME_MADP_SUM;
	volatile U_FUNC_SEL_TOTAL_LUMA_QP FUNC_SEL_TOTAL_LUMA_QP;
	volatile uint32_t FUNC_SAO_MSE_SUM;
	volatile uint32_t FUNC_VLC_PIC_STRMSIZE;
	volatile uint32_t FUNC_CABAC_PIC_STRMSIZE;
	volatile U_FUNC_VLCST_DSRPTR00 FUNC_VLCST_DSRPTR00[MAX_SLICE_NUM];
	volatile U_FUNC_VLCST_DSRPTR01 FUNC_VLCST_DSRPTR01[MAX_SLICE_NUM];
	volatile U_FUNC_SEL_LUMA_QP0_CNT FUNC_SEL_LUMA_QP_CNT[QP_HISTOGRAM_NUM];
	volatile U_FUNC_VCPI_RAWINT FUNC_VCPI_RAWINT;
} cmdlist_read_back_info_t;

void hal_cmdlist_wr_cmd_in_len0(uint32_t addr, uint32_t value);
void hal_cmdlist_get_reg(struct stream_info *stream_info, const uint32_t *base_addr);
bool hal_cmdlist_is_encode_done(uint32_t *base_addr);
void hal_cmdlist_encode(struct cmdlist_head *head, uint32_t *reg_base);
void hal_cmdlist_cfg(struct cmdlist_head *head, struct cmdlist_node *cur_node,
	struct encode_info *info);
void vedu_hal_cfg(volatile uint32_t *base, uint32_t offset, uint32_t value);
void vedu_hal_cfg_smmu(struct encode_info *channel_cfg, uint32_t core_id);
void vedu_hal_set_int(uint32_t *reg_base);

#endif
