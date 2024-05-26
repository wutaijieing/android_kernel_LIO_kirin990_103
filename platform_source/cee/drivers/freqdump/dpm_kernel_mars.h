/*
 * dpm_kernel_mars.h
 *
 * dpm
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DPM_KERNEL_MARS__
#define __DPM_KERNEL_MARS__

#include "monitor_ap_m3_ipc.h"
#include <linux/cdev.h>
#include <linux/types.h>

#define DPM_DATA_BUF_SIZE	2048
#define DPM_PERIOD_MAX	4150000000
#define DPM_PERIOD_MIN	16600000
#define DDR_TYPE	4
#define LOADMONITOR_PCLK	166
#define LOADMONITOR_AO_CLK	134400
#define GLOBALDPM_MAJOR	250

#define DPM_DEV_NAME	"dpm_ldm"
#define BDAT_IOC_MAGIC	'k'
#define IP_NAME_SIZE	64

#define DPM_SUSPEND	0x2
#define DPM_ON	0x1
#define DPM_OFF	0x0

#define DPM_GET_MAX_PERIOD	0x1
#define DPM_GET_MIN_PERIOD	0x0
#define DPM_LDM_VERSION	2

enum monitor_ip_ids {
	/* peri0 begin */
	DSS_CORE = 0,
	SYS_BUS,
	LPM3,
	ISP,
	VDEC,
	VENC,
	CFG_BUS,
	DMA_BUS,
	IVP,
	VIVO_BUS,
	VCODEC_BUS,
	DDRC_AUTOGT,
	DDRC_IND0,
	DDRC_IND1,
	MODEM_CCPU,
	MODEM_DFC,
	MODEM_DSP0,
	MODEM_NOCBUS,
	MODEM_CIPHER,
	MODEM_GUC_DMA,
	MODEM_TL_DMA,
	MODEM_EDMA1,
	MODEM_EDMA0,
	MODEM_UPACC,
	MODEM_HDLC,
	MODEM_CICOM0,
	PERI0_RESERVED0,
	PERI0_RESERVED1,
	PERI0_RESERVED2,
	PERI0_RESERVED3,
	/* peri1 begin */
	MMC0_PERIBUS,
	MMC1_PERIBUS,
	LITTLE_VDM,
	LITTLE_SVFD,
	BIG_VDM,
	BIG_SVFD,
	GPU_VDM,
	GPU_SVFD,
	DMSS,
	PERI1_RESERVED0,
	PERI1_RESERVED1,
	/* AO begin */
	ASP_HIFI,
	ASP_BUS,
	ASP_MST,
	ASP_USB,
	IOMCU,
	UFS_STAT,
	IOMCU_MST,
	UFS_SUBSYS,
	PDC_SLEEP,
	PDC_DEEP_SLEEP,
	PDC_SLOW,
	LP_STAT_S0,
	LP_STAT_S1,
	LP_STAT_S2,
	LP_STAT_S3,
	LP_STAT_S4,
	MAX_MONITOR_IP_DEVICE
};

#define MAX_PERI_DEVICE	41
#define MAX_AO_DEVICE	16
#define MONITOR_STAT_NUM	3

#define MAX_PERI_INDEX	(MAX_PERI_DEVICE * MONITOR_STAT_NUM)
#define MAX_AO_INDEX	(MAX_AO_DEVICE * (MONITOR_STAT_NUM + 1))

struct dpm_cdev {
	struct cdev cdev;
};

struct monitor_device {
	char *name;
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
	struct monitor_device mdev[MAX_MONITOR_IP_DEVICE];
};

#define DPM_IOCTL_GET_SUPPORT	_IOR(BDAT_IOC_MAGIC, 0x1, int)
#define DPM_IOCTL_SET_ENABLE	_IOW(BDAT_IOC_MAGIC, 0x2, int)
#define DPM_IOCTL_SET_PERIOD	_IOW(BDAT_IOC_MAGIC, 0x3, int)
#define DPM_IOCTL_GET_IP_LENGTH	_IOR(BDAT_IOC_MAGIC, 0x4, int)
#define DPM_IOCTL_GET_IP_INFO	_IOR(BDAT_IOC_MAGIC, 0x5, struct dpm_transmit_t)
#define DPM_IOCTL_GET_PERIOD_RANGE	_IOWR(BDAT_IOC_MAGIC, 0x6, int)
#define DPM_IOCTL_GET_DDR_FREQ_COUNT	_IOWR(BDAT_IOC_MAGIC, 0x7, int)
#define DPM_IOCTL_GET_DDR_FREQ_INFO	_IOWR(BDAT_IOC_MAGIC, 0X8, struct ddr_data_t)

extern struct monitor_dump_info g_monitor_info;

void update_dpm_data(struct work_struct *work);
void clear_dpm_data(void);
void dpm_common_platform_init(void);
extern void (*g_calc_monitor_time_media_hook)(int index, u32 media_pclk,
					      u64 *busy_nor, u64 *busy_low,
					      unsigned long monitor_delay);
#endif
