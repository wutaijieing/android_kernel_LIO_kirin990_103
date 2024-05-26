/*
 * dpm_kernel.h
 *
 * freqdump test
 *
 * Copyright (C) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef __DPM_KERNEL__
#define __DPM_KERNEL__

#include <linux/cdev.h>
#include "monitor_ap_m3_ipc.h"
#ifdef CONFIG_ITS
#include "soc_its_para.h"
#endif
#define DPM_PERIOD_MAX		4050000000
#define DPM_PERIOD_MIN		16200000
#define DDR_TYPE			4
#define LOADMONITOR_PCLK	162
#define LOADMONITOR_AO_CLK	60000
#define GLOBALDPM_MAJOR		250

#define DPM_DEV_NAME		"dpm_ldm"
#define BDAT_IOC_MAGIC		'k'
#define IP_NAME_SIZE		(64)

#define DPM_SUSPEND				0x2
#define DPM_ON					0x1
#define DPM_OFF					0x0

#define DPM_GET_MAX_PERIOD		0x1
#define DPM_GET_MIN_PERIOD		0x0
#define DPM_LDM_VERSION			2

enum monitor_ip_ids {
	CC712 = 0,
	PCIE0_CTRL_PM_LINKST_IN_L0S,
	PCIE0_CTRL_PM_LINKST_IN_L1,
	PCIE0_CTRL_PM_LINKST_IN_L1SUB,
	USB_20PHY_WORK,
	USB2_PHY_SLEEP_MOM,
	USB2_PHY_SUSPEND_MOM,
	DDRC_LP_IND0,
	DDRC_LP_IND1,
	AICORE_CUBE8,
	AICORE_MET8,
	AICOREVEC8,
	AICORE_BIU8,
	CLK_AIC_TOP_EN,		/* zone peri0 */
	SYS_BUS,
	LPM3,
	DMA_BUS,
	SYSBUS_AUTODIV,
	LPMCU_AUTODIV,
	CFGBUS_AUTODIV,
	DMABUS_AUTODIV,
	VDEC_AUTODIV,
	VENC_AUTODIV,
	VENC2_AUTODIV,
	PERI_AUTODIV_STAT11,
	PERI_AUTODIV_STAT13,
	PERI_AUTODIV_STAT14,
	PERI_AUTODIV_STAT15,
	DMSS_DDR_BUSY,
	DMSS_BUSY,
	PERI_AUTODIV_STAT16,
	PERI_AUTODIV_STAT17,
	PERI_AUTODIV_STAT18,
	MIDDLE0_SVFD,
	MIDDLE0_IDLE,
	LITTLE_SVFD,
	L3_SVFD,
	LITTLE_IDLE,
	GPU_IDLE,
	L3_IDLE,		/* zone peri1 */
	VIVOBUS_IDLE_FLAG,
	RCH0,
	RCH1,
	RCH2,
	RCH3,
	RCH4,
	RCH5,
	RCH6,
	RCH7,
	WCH0,
	WCH1,
	RCH8,
	ARSR_PRE,
	ARSR_POST,
	HIACE,
	DPP,
	IFBC,
	DBUF,
	DBCU,
	CMDLIST,
	MCTL,
	ISP_IM_BUSY0,
	ISP_IM_BUSY1,
	ISP_IM_BUSY2,
	ISP_IM_BUSY3,
	ISP_BUSY,		/* zone media0 */
	IVP_AXI_BUS,
	IVP_DSP,
	VCODECBUS,
	VENC,
	BUSY_TQITQ,
	BUSY_IME,
	BUSY_FME,
	BUSY_MRG,
	BUSY_SAO,
	BUSY_SEL,
	BUSY_PME,
	BUSY_DBLK,
	BUSY_INTRA,
	VDEC,
	SED_IDLE,
	PMV_IDLE,
	PRC_IDLE,
	PRF_IDLE,
	ITRANS_IDLE,
	RCN_IDLE,
	DBLK_SAO_IDLE,
	CMP_IDLE,
	HIEPS2LM_BUSY0,
	HIEPS2LM_BUSY1,
	HIEPS2LM_BUSY2,		/* zone media1 */
	ASP_HIFI,
	ASP_BUS,
	ASP_NOC_PENDINGTRANS,
	IOMCU,
	IOMCU_NOC_PENDINGTRANS,
	UFSBUS,
	UFSBUS_IDLE,
	AOBUS_STAT,
	PLL_AUTODIV,
	FLL_AUTODIV,
	UFSBUS_STAT,
	AO_AUTODIV_STAT4,
	AO_AUTODIV_STAT5,
	AO_AUTODIV_STAT6,
	AO_AUTODIV_STAT7,
	AO_AUTODIV_STAT8,
	PDC_SLEEP,
	PDC_DEEP_SLEEP,
	PDC_SLOW,
	LP_STAT_S0,
	LP_STAT_S1,
	LP_STAT_S2,
	LP_STAT_S3,
	LP_STAT_S4,		/* zone ao */
	MAX_MONITOR_IP_DEVICE
};


#define MAX_PERI_DEVICE 	91
#define MAX_AO_DEVICE 		24
#define MAX_MEDIA_DEVICE 	51
#define MONITOR_STAT_NUM 	3

#define MAX_PERI_INDEX 		(MAX_PERI_DEVICE * MONITOR_STAT_NUM)
#define MAX_AO_INDEX 		(MAX_AO_DEVICE * (MONITOR_STAT_NUM + 1))

struct dpm_cdev {
	struct cdev cdev;
};

struct monitor_device {
	char *name;
	s64 idle_nor_time;
	s64 busy_nor_time;
	s64 busy_low_time;
	s64 off_time;
} ;

struct ip_info {
	char name[IP_NAME_SIZE];
	s64 off_time;
	s64 busy_nor_time;
	s64 busy_low_time;
	s64 idle_nor_time;
};

struct dpm_transmit_t {
	int32_t length;
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



#define DPM_IOCTL_GET_SUPPORT			_IOR(BDAT_IOC_MAGIC, 0x1, int)
#define DPM_IOCTL_SET_ENABLE			_IOW(BDAT_IOC_MAGIC, 0x2, int)
#define DPM_IOCTL_SET_PERIOD			_IOW(BDAT_IOC_MAGIC, 0x3, int)
#define DPM_IOCTL_GET_IP_LENGTH		_IOR(BDAT_IOC_MAGIC, 0x4, int)
#define DPM_IOCTL_GET_IP_INFO			_IOR(BDAT_IOC_MAGIC, 0x5, struct dpm_transmit_t)
#define DPM_IOCTL_GET_PERIOD_RANGE	_IOWR(BDAT_IOC_MAGIC, 0x6, int)
#define DPM_IOCTL_GET_DDR_FREQ_COUNT		_IOWR(BDAT_IOC_MAGIC, 0x7, int)
#define DPM_IOCTL_GET_DDR_FREQ_INFO 			_IOWR(BDAT_IOC_MAGIC, 0X8, struct ddr_data_t)
#ifdef CONFIG_ITS
#define DPM_IOCTL_GET_CPU_POWER			_IOR(BDAT_IOC_MAGIC, 0x9, its_cpu_power_t)
#define DPM_IOCTL_RESET_CPU_POWER		_IO(BDAT_IOC_MAGIC, 0xA)
#endif

extern struct monitor_dump_info g_monitor_info;
void update_dpm_data(struct work_struct *work);
void clear_dpm_data(void);
extern void dpm_common_platform_init(void);
extern void (*g_calc_monitor_time_media_hook)(int index, u32 media_pclk, u64 *busy_nor, u64 *busy_low, unsigned long monitor_delay);
#ifdef CONFIG_ITS
extern int get_its_power_result(its_cpu_power_t *result);
extern int reset_power_result(void);
#endif
#endif
