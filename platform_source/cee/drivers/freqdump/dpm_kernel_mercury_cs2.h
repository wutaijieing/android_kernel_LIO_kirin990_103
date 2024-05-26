/*
 * dpm_kernel_mercury_cs2.h
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
#ifndef __DPM_KERNEL_MERCURY_CS2__
#define __DPM_KERNEL_MERCURY_CS2__

#include <linux/cdev.h>
#include "monitor_ap_m3_ipc.h"
#ifdef CONFIG_ITS
#include "soc_its_para.h"
#endif
#include "dubai_peri_mercury_common.h"
#include <linux/platform_drivers/dpm_hwmon_user.h>

#define DPM_DATA_BUF_SIZE	2048
#define DPM_PERIOD_MAX	4150000000
#define DPM_PERIOD_MIN	16600000
#define DDR_TYPE	4
#define LOADMONITOR_PCLK	166
#define LOADMONITOR_AO_CLK	157290

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
	CC712_STAT = 0,
	PCI0_LNKST_L0,
	PCI0_LNKST_L1,
	PCI0_LNKST_L1SUB,
	PCI1_LNKST_L0,
	PCI1_LNKST_L1,
	PCI1_LNKST_L1SUB,
	NPU_CUBE,
	NPU_MTE,
	NPU_VEC,
	NPU_BIU,
	NPU_AIC_TOPEN,
	DDRC_IND0,
	DDRC_IND1,
	DDRC_IND2,
	DDRC_IND3,
	MODEM_CCPU,
	MODEM_DSP0,
	MODEM_NOC_BUS,
	MODEM_CIPHER,
	MODEM_GUC_DMA,
	MODEM_TL_DMA,
	MODEM_EICC,
	MODEM_EDMA2,
	MODEM_EDMA1,
	MODEM_EDMA0,
	MODEM_UPACC,
	MODEM_CICOM1,
	MODEM_CICOM0,
	MODEM_UDC,
	MODEM_HLDC_CLK, /* zone peri0 */
	SYS_BUS,
	LPM3,
	NPU_AUTODIV,
	DMA_BUS,
	SYSBUS_AUTODIV,
	LPMCU_AUTODIV,
	CFGBUS_AUTODIV,
	DMABUS_AUTODIV,
	VDEC_AUTODIV,
	VENC_AUTODIV,
	VENC2_AUTODIV,
	DMSS_DDR_BUSY,
	DMSS_BUSY,
	BIG_SVFD,
	MID_SVFD,
	BIG_IDLE,
	MID_IDLE,
	LITTLE_SVFD,
	NPU_SVFD,
	L3_SVFD,
	LITTLE_IDLE,
	GPU_IDLE,
	L3_IDLE, /* zone peri1 */
	NPU2_CUBE,
	NPU2_MTE,
	NPU2_VEC,
	NPU2_BIU,
	NPU2_AIC_TOPEN, /* zone peri2 */
	VIVO_BUS,
	MDCOM,
	DSS_RCH0,
	DSS_RCH1,
	DSS_RCH2,
	DSS_RCH3,
	DSS_RCH4,
	DSS_RCH5,
	DSS_RCH6,
	DSS_RCH7,
	DSS_WCH0,
	DSS_WCH1,
	DSS_RCH8,
	DSS_ARSR_PRE,
	DSS_ARSR_POST,
	DSS_HIACE,
	DSS_DPP,
	DSS_IFBC,
	DSS_DBUF,
	DSS_DBCU,
	DSS_CMDLIST,
	DSS_MCTL,
	ISP_VDO_REC,
	ISP_BOKEH,
	ISP_PREVIEW,
	ISP_CAP, /* zone media0 */
	IVP_MST,
	IVP_DSP_BUSY,
	VCODECBUS,
	VENC,
	VENC_264I,
	VENC_264P,
	VENC_265I,
	VENC_265P,
	VENC_TQITQ,
	VENC_SEL,
	VENC_IME,
	VENC2,
	VENC2_264I,
	VENC2_264P,
	VENC2_265I,
	VENC2_265P,
	VENC2_TQITQ,
	VENC2_SEL,
	VENC2_IME,
	VDEC,
	VDEC_SED,
	VDEC_PMV,
	VDEC_PRC,
	VDEC_PRF,
	VDEC_ITRANS,
	VDEC_RCN,
	VDEC_DBLK_SAO,
	VDEC_CMP,
	EPS_BUSY0,
	EPS_BUSY1,
	EPS_BUSY2,
	HIFACE_BUSY, /* zone media1 */
#ifndef CONFIG_LIBLINUX
	ASP_HIFI,
	ASP_BUS,
	ASP_MST,
	ASP_USB,
	IOMCU,
	IOMCU_MST,
	UFSBUS_STAT,
	UFS,
	AOBUS_AUTODIV,
	PLL_AUTODIV,
	FLL_AUTODIV,
	UFSBUS_AUTODIV,
	FD_MST,
	FD_BUSMON,
	FD_HWACC,
	FD_AOBUS,
	FD_MINIISP,
	FD_DMA,
	FD_TINY,
	PDC_SLEEP,
	PDC_DEEPSLEEP,
	PDC_SLOW,
	LP_STAT_S0,
	LP_STAT_S1,
	LP_STAT_S2,
	LP_STAT_S3,
	LP_STAT_S4, /* zone ao */
#endif
	MAX_MONITOR_IP_DEVICE
};

#define MAX_PERI_DEVICE	(54 + 58 + 5)
#define MAX_AO_DEVICE	27
#define MAX_MEDIA_DEVICE	58
#define MONITOR_STAT_NUM	3

#define MAX_PERI_INDEX	(MAX_PERI_DEVICE * MONITOR_STAT_NUM)
#define MAX_AO_INDEX	(MAX_AO_DEVICE * (MONITOR_STAT_NUM + 1))

struct dpm_cdev {
	struct cdev cdev;
};

struct monitor_device {
	char *name;
	u64 idle_nor_time;
	u64 busy_nor_time;
	u64 busy_low_time;
	u64 off_time;
};

struct ip_info {
	char name[IP_NAME_SIZE];
	u64 off_time;
	u64 busy_nor_time;
	u64 busy_low_time;
	u64 idle_nor_time;
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
	u64 monitor_delay;
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
#ifdef CONFIG_ITS
#define DPM_IOCTL_GET_CPU_POWER	_IOR(BDAT_IOC_MAGIC, 0x9, its_cpu_power_t)
#define DPM_IOCTL_RESET_CPU_POWER	_IO(BDAT_IOC_MAGIC, 0xA)
#endif
#define DUBAI_IOCTL_GET_PERI_SIZE_INFO	_IOR(BDAT_IOC_MAGIC, 0x22, \
					     struct peri_data_size)
#define DUBAI_IOCTL_GET_PERI_DATA_INFO	_IOC(IOC_OUT, BDAT_IOC_MAGIC, 0x23, \
					     sizeof(struct channel_data) * \
					     PERI_VOTE_DATA)

#ifdef CONFIG_LOWPM_STATS
#define DPM_IOCTL_LPM3_VOTE_NUM	_IOR(BDAT_IOC_MAGIC, 0x20, unsigned int)
#define DPM_IOCTL_LPM3_VOTE_DATA	_IOWR(BDAT_IOC_MAGIC, 0x21, struct dubai_transmit_t)
#define DPM_IOCTL_DDR_VOTE_NUM	_IOR(BDAT_IOC_MAGIC, 0x24, unsigned int)
#define DPM_IOCTL_DDR_VOTE_DATA	_IOWR(BDAT_IOC_MAGIC, 0x25, struct dubai_transmit_t)
#define DPM_IOCTL_DDR_VOTE_DATA_CLR	_IO(BDAT_IOC_MAGIC, 0x26)
#endif

extern struct monitor_dump_info g_monitor_info;

void update_dpm_data(struct work_struct *work);
void clear_dpm_data(void);
void dpm_common_platform_init(void);

extern void (*g_calc_monitor_time_media_hook)(int index, u32 media_pclk,
					      u64 *busy_nor, u64 *busy_low,
					      unsigned long monitor_delay);
#ifdef CONFIG_ITS
int get_its_power_result(its_cpu_power_t *result);
int reset_power_result(void);
#endif
#endif
