/*
 * vfmw_ext.h
 *
 * This is for vfmw.
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

#ifndef VFMW_EXT_H
#define VFMW_EXT_H

#include "vcodec_types.h"

typedef enum {
	MFDE_MODULE,
	SCD_MOUDLE,
	CRG_MOUDLE,
	MMU_SID_MOUDLE,
	DP_MONIT_MOUDLE,
	MMU_TBU_MODULE,
	MAX_INNER_MODULE,
} vfmw_inner_module;

typedef struct {
	UADDR reg_phy_addr;
	uint32_t reg_range;
} vfmw_module_reg_info;

typedef struct {
	uint32_t irq_norm;
	uint32_t irq_safe;
	uint32_t is_es_plat;
	uint32_t is_fpga;
	uint32_t support_power_control_per_frame;
	uint32_t pcie_mmap_host_start_reg;
	vfmw_module_reg_info module_reg[MAX_INNER_MODULE];
	struct device *dev;
} vdec_dts;

typedef enum {
	MEM_SHR_ERR = 0,
	MEM_SHR_CTG,
	MEM_MMU_MMU,
	MEM_MMU_SEC,
} mem_mode;

typedef struct {
	uint64_t fd;
	UADDR phy_addr;
	UADDR mmu_addr;
	uint64_t vir_addr;
	int32_t length;
	mem_mode mode;
	vcodec_handle ssm_handle;
	vcodec_handle vdec_handle;
} vfmw_mem_desc;

typedef struct {
	uint8_t int_mask;
	uint8_t scd_start;
	uint8_t scd_init_clr;
	uint8_t scd_mmu_en;
	uint8_t vdh_mmu_en;
	uint8_t avs_flag;
	uint8_t slice_check;
	uint8_t safe_flag;
	uint8_t scd_over;
	int32_t buffer_share_fd;
	UADDR buffer_first;
	UADDR buffer_last;
	UADDR buffer_init;
	UADDR roll_addr;
	UADDR next_addr;
	int32_t msg_share_fd;
	UADDR dn_msg_phy;
	UADDR up_msg_phy;
	uint32_t up_len;
	uint32_t up_step;
	uint32_t std_type;
	uint32_t pre_lsb;
	uint32_t pre_msb;
	uint32_t byte_valid;
	uint32_t short_num;
	uint32_t start_num;
	uint32_t src_eaten;
	uint32_t cfg_unid;
	uint32_t int_unid;
	int32_t ret_errno;
} scd_reg;

typedef struct {
	int32_t scd_id;
	scd_reg reg;
} scd_reg_ioctl;

typedef enum {
	DEC_DEV_TYPE_NULL = 0,
	DEC_DEV_TYPE_VDH,
	DEC_DEV_TYPE_MDMA,
	DEC_DEV_TYPE_MAX,
} dec_dev_type;

typedef struct {
	uint32_t dev_id;
	dec_dev_type type;
	uint16_t reg_id;
	uint16_t work_mode;
	uint32_t chan_id;
} dev_cfg_ioctl;

typedef struct {
	dev_cfg_ioctl dev_cfg_info;
	uint16_t cfg_id;
	uint16_t list_num;
	uint8_t vdh_mmu_en;
	uint8_t bsp_internal_ram;
	uint8_t pxp_internal_ram;
	uint8_t vdh_safe_flag;
	uint8_t mask_mmu_irq;
	uint8_t wireless_ldy_en;
	uint8_t apply_grain;
	uint8_t av1_intrabc;
	UADDR pub_msg_phy_addr;
	int32_t msg_shared_fd;
	uint32_t is_slc_ldy;
	uint32_t is_frm_cpy;
	uint32_t std_cfg;
	uint64_t cur_time;
	int32_t ret_errno;
	uint32_t use_pxp_num;
	uint32_t enhance_layer_valid_num;
} dec_dev_cfg;

typedef struct {
	uint32_t int_reg;
	uint32_t int_flag;
	uint32_t int_unid;
	uint32_t err_flag;
	uint32_t rd_slc_msg;
	uint32_t slice_num;
	uint32_t ctb_num;
	uint32_t bsp_cycle;
	uint32_t pxp_cycle;
	uint32_t is_need_wait;
	int32_t ret_errno;
} dec_back_up;

typedef enum {
	ASYNC_MODE,
	SYNC_MODE,
} work_mode;

#endif
