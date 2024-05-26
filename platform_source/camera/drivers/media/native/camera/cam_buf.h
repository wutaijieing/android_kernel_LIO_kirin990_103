/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: camera buf
 * Create: 2018-11-22
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#ifndef __CAM_BUF_H_INCLUDED__
#define __CAM_BUF_H_INCLUDED__
#include <linux/types.h>

struct device;
struct sg_table;

#define CAM_BUF_FILE_MAX_LEN 20
#define CAM_BUF_SYNC_READ (1u << 0)
#define CAM_BUF_SYNC_WRITE (1u << 1)

/* need align to AP CameraBufDevType */
enum cam_buf_dev_type {
	CAM_DEV_ISP,
	CAM_DEV_ARCH,
	CAM_DEV_JENC,
	CAM_DEV_ORB,
	CAM_DEV_MC,
	CAM_DEV_CMP,
	CAM_DEV_ANF,
	CAM_DEV_ORB_ENH,
	CAM_DEV_JDEC,
	CAM_DEV_CMDLIST,
	CAM_DEV_RDR,
	CAM_DEV_GF,
	CAM_DEV_VBK,
	CAM_DEV_IVP,
	CAM_DEV_IVP1,
	CAM_DEV_MAX
};

enum cam_buf_cfg_type {
	CAM_BUF_MAP_IOMMU,
	CAM_BUF_UNMAP_IOMMU,
	CAM_BUF_SYNC,
	CAM_BUF_LOCAL_SYNC,
	CAM_BUF_GET_PHYS,
	CAM_BUF_OPEN_SECURITY_TA,
	CAM_BUF_CLOSE_SECURITY_TA,
	CAM_BUF_SET_SECBOOT,
	CAM_BUF_RELEASE_SECBOOT,
	CAM_BUF_GET_SECMEMTYPE,
	CAM_BUF_SC_AVAILABLE_SIZE,
	CAM_BUF_SC_WAKEUP,
};
/* kernel coding style prefers __xx as types shared with userspace */
struct iommu_format {
	__u32 prot;
	__u64 iova;
	__u64 size;
};

struct phys_format {
	__u64 phys;
};

struct sync_format {
	/* sync direction read/write from CPU view point */
	__u32 direction;
};

struct local_sync_format {
	/* sync direction read/write from CPU view point */
	__u32 direction;
	/* local sync needs to set apva/offset/length */
	__u64 apva;
	__u64 offset;
	__u64 length;
};

struct sec_mem_format {
	/* sec mem buffer type */
	__u32 type;
	__u32 prot;
	__u64 iova;
	__u64 size;
};

struct systemcache_format {
	/* systemcache info */
	__u32 pid;
	__u32 prio;
	__u32 size;
};

struct cam_buf_cfg {
	__s32 fd;
	__u32 secmemtype;
	enum cam_buf_cfg_type type;
	union {
		struct iommu_format iommu_format;
		struct phys_format phys_format;
		struct sync_format sync_format;
		struct local_sync_format local_sync_format;
		struct sec_mem_format sec_mem_format;
		struct systemcache_format systemcache_format;
	};
};

#define CAM_BUF_BASE 'H'
#define CAM_BUF_IOC_CFG _IOWR(CAM_BUF_BASE, 0, struct cam_buf_cfg)

int cam_buf_map_iommu(int fd, struct iommu_format *fmt);
void cam_buf_unmap_iommu(int fd, struct iommu_format *fmt);
void cam_buf_sc_available_size(struct systemcache_format *fmt);
int cam_buf_sc_wakeup(struct systemcache_format *fmt);
int cam_buf_get_phys(int fd, struct phys_format *fmt);
int get_secmem_type(void);
phys_addr_t cam_buf_get_pgd_base(void);

/*
 * keep in mind, get sgtable will hold things of sg_table,
 * please release after use.
 */
struct sg_table *cam_buf_get_sgtable(int fd);
void cam_buf_put_sgtable(struct sg_table *sgt);

int cam_buf_v3_map_iommu(struct device *dev, int fd, struct iommu_format *fmt,
	int padding_support);
void cam_buf_v3_unmap_iommu(struct device *dev, int fd,
	struct iommu_format *fmt, int padding_support);
#endif /* ifndef __CAM_BUF_H_INCLUDED__ */
