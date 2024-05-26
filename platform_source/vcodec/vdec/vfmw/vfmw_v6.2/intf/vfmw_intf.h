/*
 * vfmw_intf.h
 *
 * This is for vfmw_intf.
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

#ifndef VFMW_INTF_H
#define VFMW_INTF_H

#include "vfmw_ext.h"
#ifdef VCODEC_TVP_SUPPORT
#include "tvp_adapter.h"
#endif

typedef enum {
	VFMW_CID_STM_PROCESS,
	VFMW_CID_GET_ACTIVE_REG,
	VFMW_CID_DEC_PROCESS,
	VFMW_CID_CANCEL_PROCESS,
	VFMW_CID_MAX_PROCESS,
} vfmw_cmd_type;

typedef enum {
	VFMW_STM_RESET,
	VFMW_DEC_RESET,
	VFMW_ALL_RESET
} vfmw_reset_type;

#define vfmw_check_return(cond, ret) \
	do { \
		if (!(cond)) { \
			dprint(PRN_ERROR, "check error: condition %s not match.\n", #cond); \
			return ret; \
		} \
	} while (0)

struct vfmw_glb_vdec_callback {
	void (*vdec_lock_power_state)(unsigned long);
	void (*vdec_unlock_power_state)(unsigned long);
	bool (*vdec_get_power_state)(void);
	bool (*vdec_get_smmu_glb_bypass)(void);
	int32_t (*vdec_mem_iommu_map)(void *, int32_t, UADDR *, unsigned long *);
	int32_t (*vdec_mem_iommu_unmap)(void *, int32_t, UADDR);
};

int32_t vfmw_control(struct file *file_handle, vfmw_cmd_type cmd, void *arg_in, void *arg_out);
int32_t vfmw_query_image(struct file *file_handle, void *dev_cfg, void *backup_out);
int32_t vfmw_init(void *args, struct vfmw_glb_vdec_callback *callback);
void vfmw_deinit(void);
void vfmw_suspend(bool sec_device_online);
void vfmw_resume(bool sec_device_online);
int32_t vdec_device_proc(uint32_t *device_stat);
vcodec_bool vfmw_scene_ident(void);
irqreturn_t vfmw_isr(int, void *);

int32_t vfmw_acquire_back_up_fifo(uint32_t instance_id);
void vfmw_release_back_up_fifo(uint32_t instance_id);
struct vfmw_glb_vdec_callback* vfmw_get_vdec_glb_callback(void);
#endif
