/*
 * loadmonitor_k.h
 *
 * monitor device status file
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LOADMONITOR_K__
#define __LOADMONITOR_K__

#include <linux/cdev.h>
#include <linux/types.h>
#include "../monitor_ap_m3_ipc.h"
#include <loadmonitor_common.h>
#include <loadmonitor_plat.h>
#ifdef CONFIG_DPM_HWMON
#include <dubai_dpm.h>
#endif
#ifdef CONFIG_ITS
#include <dubai_its.h>
#endif

#define DPM_PERIOD_MAX		4150000000
#define DPM_PERIOD_MIN		16600000
#define DPM_PERIOD_MAX_SEC	25000
#define DPM_PERIOD_MIN_SEC	100
#define DDR_TYPE		4
#define GLOBALDPM_MAJOR		250

#define DPM_DEV_NAME		"dpm_ldm"
#define BDAT_IOC_MAGIC		'k'
#define IP_NAME_SIZE		64U

#define DPM_SUSPEND		0x2
#define DPM_ON		0x1
#define DPM_OFF		0x0

#define DPM_GET_MAX_PERIOD		0x1
#define DPM_GET_MIN_PERIOD		0x0
#define DPM_LDM_VERSION		3
#define MAX_PERI_DEVICE		78
#define MAX_AO_DEVICE		20
#define MAX_MEDIA_DEVICE		29
#define MONITOR_STAT_NUM		3

#define MAX_PERI_INDEX		(MAX_PERI_DEVICE * MONITOR_STAT_NUM)
#define MAX_AO_INDEX		(MAX_AO_DEVICE * (MONITOR_STAT_NUM + 1))

#define IP_NAME_MAX_SIZE		4

struct dpm_cdev {
	struct cdev cdev;
};

struct monitor_device {
	char name[IP_NAME_MAX_SIZE];
	s64 idle_nor_time;
	s64 busy_nor_time;
	s64 busy_low_time;
	s64 off_time;
};

struct ip_info {
	char name[IP_NAME_SIZE];
	s64 off_time;
	s64 busy_nor_time;
	s64 busy_low_time;
	s64 idle_nor_time;
};

struct dpm_transmit_t {
	s32 length;
	struct ip_info data[0];
};

struct ddr_freq_data {
	char name[IP_NAME_SIZE];
	u64 freq_data[MAX_DDR_FREQ_NUM];
};

struct ddr_data_t {
	struct ddr_freq_data data[0];
};

struct monitor_dump_info {
	s64 monitor_delay;
	unsigned int div_high;
	unsigned int div_low;
	struct monitor_device mdev[MAX_NUM_LOGIC_IP_DEVICE];
};

typedef int (*IOCTRL_FUNC)(void __user *argp);
struct ioctrl_dpm_ldm_t {
	u_int ioctrl_cmd;
	IOCTRL_FUNC ioctrl_func;
};

typedef int (*IOCTRL_CONST_FUNC)(const void __user *argp);
struct const_ioctrl_dpm_ldm_t {
	u_int ioctrl_cmd;
	IOCTRL_CONST_FUNC ioctrl_func;
};

#define DPM_IOCTL_GET_SUPPORT		_IOR(BDAT_IOC_MAGIC, 0x1, int)
#define DPM_IOCTL_SET_ENABLE		_IOW(BDAT_IOC_MAGIC, 0x2, int)
#define DPM_IOCTL_SET_PERIOD		_IOW(BDAT_IOC_MAGIC, 0x3, int)
#define DPM_IOCTL_GET_IP_LENGTH		_IOR(BDAT_IOC_MAGIC, 0x4, int)
#define DPM_IOCTL_GET_IP_INFO		_IOR(BDAT_IOC_MAGIC, 0x5, struct dpm_transmit_t)
#define DPM_IOCTL_GET_PERIOD_RANGE	_IOWR(BDAT_IOC_MAGIC, 0x6, int)
#define DPM_IOCTL_GET_DDR_FREQ_COUNT	_IOWR(BDAT_IOC_MAGIC, 0x7, int)
#define DPM_IOCTL_GET_DDR_FREQ_INFO	_IOWR(BDAT_IOC_MAGIC, 0x8, \
					      struct ddr_data_t)
#define DPM_IOCTL_SET_FLAGS	_IOW(BDAT_IOC_MAGIC, 0x100, enum EN_FLAGS)

#ifdef CONFIG_DPM_HWMON
#define DPM_IOCTL_DPM_ENABLE	_IOW(BDAT_IOC_MAGIC, 0xb, unsigned int)
#define DPM_IOCTL_DPM_PERI_NUM	_IOR(BDAT_IOC_MAGIC, 0xc, unsigned int)
#define DPM_IOCTL_DPM_PERI_DATA	_IOWR(BDAT_IOC_MAGIC, 0xd, struct dubai_transmit_t)
#define DPM_IOCTL_DPM_GPU_DATA	_IOR(BDAT_IOC_MAGIC, 0xe, unsigned long long)
#define DPM_IOCTL_DPM_NPU_DATA	_IOR(BDAT_IOC_MAGIC, 0xf, unsigned long long)

#ifdef CONFIG_MODEM_DPM
#define DPM_IOCTL_DPM_MODEM_NUM     _IOR(BDAT_IOC_MAGIC, 0x30, unsigned int)
#define DPM_IOCTL_DPM_MODEM_DATA    _IOWR(BDAT_IOC_MAGIC, 0x31, struct mdm_transmit_t)
#endif
#endif

#define DPM_IOCTL_LPM3_VOTE_NUM	_IOR(BDAT_IOC_MAGIC, 0x20, unsigned int)
#define DPM_IOCTL_LPM3_VOTE_DATA	_IOWR(BDAT_IOC_MAGIC, 0x21, struct dubai_transmit_t)

#define DUBAI_IOCTL_GET_PERI_SIZE_INFO	_IOR(BDAT_IOC_MAGIC, 0x22, \
					     struct peri_data_size)
#define DUBAI_IOCTL_GET_PERI_DATA_INFO	_IOC(IOC_OUT, BDAT_IOC_MAGIC, 0x23, \
					     sizeof(struct channel_data) * \
						    PERI_VOTE_DATA)

#define DPM_IOCTL_DDR_VOTE_NUM	_IOR(BDAT_IOC_MAGIC, 0x24, unsigned int)
#define DPM_IOCTL_DDR_VOTE_DATA	_IOWR(BDAT_IOC_MAGIC, 0x25, struct dubai_transmit_t)
#define DPM_IOCTL_DDR_VOTE_DATA_CLR	_IO(BDAT_IOC_MAGIC, 0x26)

#define DUBAI_IOCTL_READ_CLEAR_DISABLE_ENABLE	_IOR(BDAT_IOC_MAGIC, 0x27, \
						     struct dpm_transmit_t)
#define DUBAI_IOCTL_ENABLE_SR_AO_MONITOR	_IOW(BDAT_IOC_MAGIC, 0x28, int)
#define DUBAI_IOCTL_ON_OFF_ERR_LOG	_IOW(BDAT_IOC_MAGIC, 0x29, int)
#ifdef CONFIG_LOWPM_DDR_STATE
#define DPM_IOCTL_DDR_STATE_NUM	_IOR(BDAT_IOC_MAGIC, 0x2a, unsigned int)
#define DPM_IOCTL_DDR_STATE_DATA	_IOWR(BDAT_IOC_MAGIC, 0x2b, struct dubai_transmit_t)
#endif
#ifdef CONFIG_ITS
#define DPM_IOCTL_GET_CPU_POWER	_IOR(BDAT_IOC_MAGIC, 0x9, its_cpu_power_t)
#define DPM_IOCTL_RESET_CPU_POWER	_IO(BDAT_IOC_MAGIC, 0xa)
#endif

void update_dpm_data(struct work_struct *work);
int clear_dpm_data(void);
extern struct platform_driver media_dpm_driver;
#endif
