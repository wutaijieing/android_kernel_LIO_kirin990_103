/*
 * drv_omxvdec.c
 *
 * This is for omxvdec export header file.
 *
 * Copyright (c) 2014-2020 Huawei Technologies CO., Ltd.
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

#ifndef __DRV_OMXVDEC_H__
#define __DRV_OMXVDEC_H__

#include <linux/ioctl.h>
#include "vcodec_types.h"

#define OMXVDEC_NAME                    "vcodec_vdec"
#define PATH_LEN                        256

/*
 * IOCTL for interaction with omx components
 * For Compat USER 32 -> KER 64, all pointer menbers set in last in structure.
 * NOTICE: MUST NOT ALTER the member sequence of this structure
 */
typedef struct vcodec_omxvdec_ioctl_msg {
	int32_t chan_num;
	int32_t in_size;
	int32_t out_size;
	void *in;
	void *out;
} vdec_ioctl_msg;

// Modified for 64-bit platform
typedef struct vcodec_compat_ioctl_msg {
	int32_t chan_num;
	int32_t in_size;
	int32_t out_size;
	compat_ulong_t in;
	compat_ulong_t out;
} compat_ioctl_msg;

#define VDEC_IOCTL_MAGIC 'v'

#define VDEC_IOCTL_CANCEL_DECODE    \
	_IO(VDEC_IOCTL_MAGIC, 19)

#define VDEC_IOCTL_SET_CLK_RATE    \
	_IO(VDEC_IOCTL_MAGIC, 20)

#define VDEC_IOCTL_GET_VDM_HWSTATE \
	_IO(VDEC_IOCTL_MAGIC, 21)

#define VDEC_IOCTL_SCD_PROC        \
	_IO(VDEC_IOCTL_MAGIC, 22)

#define VDEC_IOCTL_VDM_PROC        \
	_IO(VDEC_IOCTL_MAGIC, 23)

#define VDEC_IOCTL_LOCK_HW         \
	_IO(VDEC_IOCTL_MAGIC, 24)

#define VDEC_IOCTL_UNLOCK_HW       \
	_IO(VDEC_IOCTL_MAGIC, 25)

#define VDEC_IOCTL_GET_ACTIVE_REG   \
	_IO(VDEC_IOCTL_MAGIC, 28)

#define VDEC_IOCTL_DEC_IRQ_QUERY    \
	_IO(VDEC_IOCTL_MAGIC, 29)

#define VDEC_IOCTL_ENABLE_SEC_MODE   \
	_IO(VDEC_IOCTL_MAGIC, 32)

#define VDEC_IOCTL_IOMMU_MAP \
	_IO('V', 0x33)
#define VDEC_IOCTL_IOMMU_UNMAP \
	_IO('V', 0x34)

#endif
