/*
 * sec_intf.h
 *
 * This is for vdec driver interface.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
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

#ifndef VFMW_INTF_H
#define VFMW_INTF_H

#include "vfmw_ext.h"

typedef enum {
	VFMW_CID_STM_PROCESS,
	VFMW_CID_GET_ACTIVE_REG,
	VFMW_CID_DEC_PROCESS,
	VFMW_CID_STM_CANCEL,
	VFMW_CID_DEC_CANCEL,
	VFMW_CID_MAX_PROCESS,
} vfmw_cmd_type;

typedef enum {
	VFMW_STM_RESET,
	VFMW_DEC_RESET,
	VFMW_ALL_RESET
} vfmw_reset_type;

typedef enum {
	EXIT_NORMAL = 1,
	EXIT_ABNORMAL
} vfmw_exit_type;

/* standard type for vfmw common use */
enum vfmw_vid_std {
	VFMW_START_RESERVED = 0,
	VFMW_H264,
	VFMW_MPEG4,
	VFMW_MPEG2,
	VFMW_HEVC,
	VFMW_VP9,
	VFMW_STD_MAX
};

/* standard type for stm only */
enum vfmw_stm_vid {
	VFMW_STM_AVC_HEVC = 0,
	VFMW_STM_MPEG4 = 2,
	VFMW_STM_MPEG2 = 3,
	VFMW_STM_VP9 = 18,
	VFMW_STM_VID_MAX
};

#define vfmw_check_return(cond, ret) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, "check error: condition %s not match.\n", #cond); \
			return ret; \
		} \
	} while (0)

int32_t tee_vdec_drv_init(int pid);
int32_t tee_vdec_drv_exit(int exit_type);
int32_t tee_vdec_drv_scd_start(void *stm_cfg, int cfg_size, void *scd_state_reg, int reg_size);
int32_t tee_vdec_drv_iommu_map(void *mem_param, int in_size, void *out_mem_param, int out_size);
int32_t tee_vdec_drv_iommu_unmap(void *mem_param, int in_size);
int32_t vfmw_sec_map_cfg_to_std(uint32_t std_cfg);
int32_t vfmw_sec_dec_param_check(void *dev_cfg);
int32_t vfmw_sec_stm_param_check(void *stm_cfg);
int32_t tee_vdec_drv_get_active_reg(void *dev_cfg, int cfg_size);
int32_t tee_vdec_drv_dec_start(void *dev_cfg, int cfg_size);
int32_t tee_vdec_drv_irq_query(void *dev_cfg, int cfg_size, void *read_backup, int backup_size);
int32_t tee_vdec_drv_set_dev_reg(uint32_t dev_state);
int32_t tee_vdec_drv_resume(void);
int32_t tee_vdec_drv_suspend(void);
int32_t vfmw_acquire_back_up_fifo(uint32_t instance_id);
void vfmw_release_back_up_fifo(uint32_t instance_id);

#endif
