/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: dev drvier to communicate with sensorhub igs app
 * Create: 2017-12-05
 */

#ifndef __LINUX_CONTEXTHUB_IGS_H__
#define __LINUX_CONTEXTHUB_IGS_H__
#include <linux/types.h>

/* ioctl cmd define */
#define IGS_IO                         0xB1

#define IGS_IOCTL_IGS_OPEN         _IOW(IGS_IO, 0xD1, short)
#define IGS_IOCTL_IGS_CLOSE        _IOW(IGS_IO, 0xD2, short)
#define IGS_IOCTL_FUSION_EN          _IOW(IGS_IO, 0xD3, short)
#define IGS_IOCTL_FUSION_SET         _IOW(IGS_IO, 0xD4, short)
#ifdef CONFIG_CONTEXTHUB_IGS_20
#define IGS_IOCTL_FUSION_GETS        _IOW(IGS_IO, 0xD5, short)
#define IGS_IOCTL_FUSION_SETS        _IOW(IGS_IO, 0xD6, short)
#endif
#define IGS_IOCTL_ION_REF_ATTACH     _IOW(IGS_IO, 0xD7, short)
#define IGS_IOCTL_ION_REF_DETACH     _IOW(IGS_IO, 0xD8, short)

#define IGS_IOCTL_SET_AP_INPUT        _IOW(IGS_IO, 0xD9, short)

#define IOMCU_SYSTEMCACHE_HINT         8

struct igs_ion_info_t {
	struct dma_buf *buf;
	u64 id;
	int fd;
};

struct igs_fusion_en_t {
	u32 fusion_id;
	u32 en;
};

struct igs_fusion_en_resp_t {
	u32 ret_code;
	u32 fusion_id;
};

struct igs_fusion_en_param_t {
	struct igs_fusion_en_t en;
	struct igs_fusion_en_resp_t en_resp;
};

struct igs_fusion_set_t {
	u32 fusion_id;
	u32 data_type;
	u32 data_addr;
	u32 data_len;
};

struct igs_fusion_set_resp_t {
	u32 ret_code;
	u32 fusion_id;
};

struct igs_fusion_set_param_t {
	struct igs_fusion_set_t set;
	struct igs_fusion_set_resp_t set_resp;
};

struct igs_upload_t {
	u32 fusion_id;
	u32 notify_type;
	u32 notify_len;
	u32 reserved;
};

struct igs_set_ap_input_info_t {
	int fd;
	u32 fusion_id;
	u32 data_type;
	u32 data_len;
};

struct igs_set_ap_input_resp_t {
	u32 ret_code;
	u32 fusion_id;
	u32 data_type;
};

struct igs_set_ap_input_param_t {
	struct igs_set_ap_input_info_t input;
	struct igs_set_ap_input_resp_t input_resp;
};

#ifdef CONFIG_CONTEXTHUB_IGS_20
struct igs_fusion_gets_t {
	u32 fusion_id;
	u32 data_type;
	u32 sub_type;
	u32 data_len;
	u32 context;
	u32 reserved;
	u64 data_addr;
};

struct igs_fusion_gets_resp_t {
	u32 ret_code;
	u32 fusion_id;
	u32 context;
	u32 data_type;
	u32 sub_type;
	u32 data_len;
	u64 data_addr;
};

struct igs_fusion_gets_param_t {
	struct igs_fusion_gets_t gets_param;
	struct igs_fusion_gets_resp_t gets_resp;
};

struct igs_fusion_sets_t {
	u32 fusion_id;
	u32 data_type;
	u32 data_len;
	u32 reserved;
	u64 data_addr;
};

struct igs_fusion_sets_resp_t {
	u32 ret_code;
	u32 fusion_id;
	u32 data_type;
	u32 data_len;
};

struct igs_fusion_sets_param_t {
	struct igs_fusion_sets_t sets;
	struct igs_fusion_sets_resp_t sets_resp;
};

#endif

#endif
