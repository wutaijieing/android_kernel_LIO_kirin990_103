/*
 * vcodec_vdec.h
 *
 * This is for vdec management
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

#ifndef VCODEC_VDEC
#define VCODEC_VDEC

#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include "drv_omxvdec.h"
#include "vcodec_types.h"
#include "vcodec_vdec_dbg.h"

#define MAX_OPEN_COUNT 32
#define POWER_OFF_GAP_NUM 3
#define POWER_OFF_THRESHOLD 6500

#ifdef PCIE_FPGA_VERIFY
#define VCODEC_ATTR_WEEK __attribute__((weak))
#else
#define VCODEC_ATTR_WEEK
#endif

typedef struct {
	uint64_t vdec_open_timestamp_us;
	uint64_t power_off_timestamp_us;
	uint64_t power_off_on_times;
	uint64_t power_off_duration_sum;
	uint32_t power_off_duration[POWER_OFF_GAP_NUM];  /* store the duration between power off and power on. */
} vdec_power_info;

typedef struct {
	struct file *file;
} vdec_mode;

typedef struct {
	uint32_t sec_chan_num;
	vdec_mode vdec_chan_info[MAX_OPEN_COUNT];
} vdec_chan_mode;

typedef struct {
	uint32_t open_count;
	uint32_t power_ref_cnt;
	atomic_t nor_chan_num;
	atomic_t sec_chan_num;
	struct mutex vdec_mutex;
	struct mutex vdec_mutex_scd;
	struct mutex vdec_mutex_vdh;
	struct mutex vdec_mutex_sec_scd;
	struct mutex vdec_mutex_sec_vdh;
	vcodec_bool device_locked;
	struct cdev cdev;
	struct device *class_dev;
	struct device *device;
	vdec_chan_mode chan_mode;
	vdec_power_info vdec_power;
	bool power_state; /* false: power off; true: power on */
	spinlock_t power_state_spin_lock;
	struct vdec_setting_info setting_info;
} vdec_entry;

enum ioctl_cid {
	IOCTL_SCD_PROC,
	IOCTL_IOMMU_MAP,
	IOCTL_IOMMU_UNMAP,
	IOCTL_GET_ACTIVE_REG,
	IOCTL_VDM_PROC,
	IOCTL_DEC_IRQ_QUERY,
	IOCTL_SET_CLK_RATE
};

typedef struct {
	int32_t share_fd;
	UADDR iova;
	uint64_t vir_addr;
	uint32_t iova_size;
} vdec_buffer_record;

struct dec_file_handle {
	uint32_t instance_id;
	uint32_t is_used;
	vdec_entry *vdec_entry;
};

int32_t vdec_check_power_on(void);
int32_t vdec_check_power_off(unsigned int cmd);
vdec_entry *vdec_get_entry(void);
#endif

