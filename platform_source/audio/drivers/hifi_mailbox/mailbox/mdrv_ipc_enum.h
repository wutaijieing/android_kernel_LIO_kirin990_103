/*
 * mdrv_ipc_enum.h
 *
 * definitions of ipc enums
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#ifndef __MDRV_IPC_ENUM_H__
#define __MDRV_IPC_ENUM_H__

/* Processor type */
enum ipc_int_core {
	IPC_CORE_ARM11 = 0, /* IPC on P500 */
	IPC_CORE_A9,
	IPC_CORE_CEVA,
	IPC_CORE_TENS0,
	IPC_CORE_TENS1,
	IPC_CORE_DSP,
	IPC_CORE_APPARM = 0, /* IPC on V7R1 / V3R2 */
	IPC_CORE_COMARM,
	IPC_CORE_LTEDSP,
	IPC_CORE_VDSP,
	IPC_CORE_ZSP,
	IPC_CORE_DSP_GU,
	IPC_CORE_ACPU = 0, /* IPC on V9R1 SFT */
	IPC_CORE_CCPU,
	IPC_CORE_MCU,
	IPC_CORE_HIFI,
	IPC_CORE_BBE16,
	IPC_CORE_ACORE = 0, /* P531, V7R2 */
	IPC_CORE_CCORE,
	IPC_CORE_MCORE,
	IPC_CORE_LDSP,
	IPC_CORE_SOCDSP,
	IPC_CORE_BBE, /* P531\V7R2 P531 */
	IPC_CORE0_A15,
	IPC_CORE1_A15,

	/* Attention: add new elements to the end */
	IPC_CORE_BUTTOM
};

/*
 * Add new IPC resources, enumerate naming formats:
 * IPC_<target processor>_INT_SRC_<source processor>_<function>
 */
#if defined(CHIP_BB_HI6210)
enum ipc_int_lev {
	/* Define the bit position of CCPU IPC message interrupt source */
	IPC_INT_DSP_MODEM = 0u,
	IPC_CCPU_INT_SRC_HIFI_MSG = 1u,
	IPC_INT_DSP_MSP = 2u,
	IPC_INT_DSP_PS = 3u,
	IPC_CCPU_INT_SRC_MCU_MSG = 5u,
	IPC_CCPU_INT_SRC_ACPU_MSG = 6u,
	IPC_CCPU_INT_SRC_ACPU_IFC = 7u,
	IPC_INT_DSP_HALT = 8u,
	IPC_INT_DSP_RESUME = 9u,
	IPC_CCPU_INT_SRC_MCU_IFC = 10u,
	IPC_INT_WAKE_GU = 11u,
	IPC_INT_SLEEP_GU = 12u,
	/* Placeholder, must be consistent with IPC_INT_DICC_USRDATA */
	IPC_INT_DICC_USRDATA_ACPU = 13u,
	IPC_INT_DICC_RELDATA_ACPU = 14u,
	IPC_INT_ARM_SLEEP = 15u,
	/* Define the mailbox IPC interrupt used by TDS, begin */
	IPC_INT_WAKE_GSM = 16u,
	IPC_INT_WAKE_WCDMA = 17u,
	IPC_INT_DSP_PS_PUB_MBX = 18u,
	IPC_INT_DSP_PS_MAC_MBX = 19u,
	IPC_INT_DSP_MBX_RSD = 20u,
	IPC_CCPU_INT_SRC_DSP_MNTN = 21u,
	IPC_CCPU_INT_SRC_DSP_RCM_PUB_MBX = 22u,
	/* Define the mailbox IPC interrupt used by TDS, end */
	IPC_CCPU_INT_SRC_ACPU_ICC = 30u,
	IPC_CCPU_INT_SDR_CCPU_BBP_MASTER_ERROR = 31u,
	IPC_CCPU_INT_SRC_BUTT = 32u,

	/* Define the bit position of MCU IPC message interrupt source */
	IPC_MCU_INT_SRC_ACPU_MSG = 4u,
	IPC_MCU_INT_SRC_CCPU_MSG = 5u,
	IPC_MCU_INT_SRC_HIFI_MSG = 6u,
	IPC_MCU_INT_SRC_CCPU_IFC = 7u,
	IPC_MCU_INT_SRC_CCPU_IPF = 8u,
	IPC_MCU_INT_SRC_ACPU_IFC = 9u,
	IPC_MCU_INT_SRC_ACPU0_PD = 10u,
	IPC_MCU_INT_SRC_ACPU1_PD = 11u,
	IPC_MCU_INT_SRC_ACPU2_PD = 12u,
	IPC_MCU_INT_SRC_ACPU3_PD = 13u,
	IPC_MCU_INT_SRC_ACPU_HOTPLUG = 14u,
	IPC_MCU_INT_SRC_ACPU_DFS = 15u,
	IPC_MCU_INT_SRC_ACPU_PD = 16u,
	IPC_MCU_INT_SRC_CCPU_PD = 17u,
	IPC_MCU_INT_SRC_HIFI_PD = 18u,
	IPC_MCU_INT_SRC_MCU_AGT = 19u,
	IPC_MCU_INT_SRC_HIFI_DDR_VOTE = 20u,
	IPC_MCU_INT_SRC_ACPU_I2S_REMOTE_SLOW = 21u,
	IPC_MCU_INT_SRC_ACPU_I2S_REMOTE_SLEEP = 22u,
	IPC_MCU_INT_SRC_ACPU_I2S_REMOTE_INVALID = 23u,
	IPC_MCU_INT_SRC_HIFI_MEMSHARE_DDR_VOTE = 24u,
	IPC_MCU_INT_SRC_HIFI_MEMSHARE_DDR_EXIT_VOTE = 25u,
	IPC_MCU_INT_SRC_ACPU4_PD = 26u,
	IPC_MCU_INT_SRC_ACPU5_PD = 27u,
	IPC_MCU_INT_SRC_ACPU6_PD = 28u,
	IPC_MCU_INT_SRC_ACPU7_PD = 29u,
	IPC_MCU_INT_SRC_HIFI_IFC = 31u,
	IPC_MCU_INT_SRC_BUTT = 32u,

	/* Define the bit position of ACPU IPC message interrupt source */
	IPC_ACPU_INT_SRC_CCPU_MSG = 1u,
	IPC_ACPU_INT_SRC_HIFI_MSG = 2u,
	IPC_ACPU_INT_SRC_MCU_MSG = 3u,
	IPC_ACPU_INT_SRC_CCPU_NVIM = 4u,
	IPC_ACPU_INT_SRC_CCPU_IFC = 5u,
	IPC_ACPU_INT_SRC_MCU_IFC = 6u,
	IPC_ACPU_INT_SRC_MCU_THERMAL_HIGH = 7u,
	IPC_ACPU_INT_SRC_MCU_THERMAL_LOW = 8u,
	IPC_INT_DSP_APP = 9u,
	IPC_ACPU_INT_SRC_HIFI_PC_VOICE_RX_DATA = 10u,
	IPC_INT_DICC_USRDATA = 13u,
	IPC_INT_DICC_RELDATA = 14u,
	IPC_ACPU_INT_SRC_CCPU_LOG = 25u,
	IPC_ACPU_INI_SRC_MCU_TELE_MNTN_NOTIFY = 26u,
	IPC_ACPU_INI_SRC_MCU_EXC_REBOOT = 27u,
	IPC_ACPU_INT_SRC_CCPU_EXC_REBOOT = 28u,
	IPC_ACPU_INT_SRC_CCPU_NORMAL_REBOOT = 29u,
	IPC_ACPU_INT_SRC_MCU_DDR_EXC = 30u,
	IPC_ACPU_INT_SRC_CCPU_ICC = 31u,
	IPC_ACPU_INT_SRC_BUTT = 32u,

	/* Define the bit position of HIFI IPC message interrupt source */
	IPC_HIFI_INT_SRC_ACPU_MSG = 0u,
	IPC_HIFI_INT_SRC_CCPU_MSG = 1u,
	IPC_HIFI_INT_SRC_BBE_MSG = 4u,
	IPC_HIFI_INT_SRC_MCU_MSG = 6u,
	IPC_HIFI_INT_SRC_MCU_WAKE_DDR = 7u,
	IPC_HIFI_INT_SRC_MCU_IFC = 8u,
	IPC_HIFI_INT_SRC_BUTT = 32u,

	/* Define the bit position of BBE16 IPC message interrupt source */
	IPC_INT_MSP_DSP_OM_MBX = 0u,
	IPC_INT_PS_DSP_PUB_MBX = 1u,
	IPC_INT_PS_DSP_MAC_MBX = 2u,
	IPC_INT_HIFI_DSP_MBX = 3u,
	IPC_BBE16_INT_SRC_HIFI_MSG = 3u,
	IPC_BBE16_INT_SRC_BUTT = 32u,

	IPC_INT_BUTTOM = 32u
};

enum ipc_sem_id {
	IPC_SEM_ICC = 0,
	IPC_SEM_NAND = 1,
	IPC_SEM_MEM = 2,
	IPC_SEM_DICC = 3,
	IPC_SEM_RFILE_LOG = 4,
	IPC_SEM_EMMC = 5,
	IPC_SEM_NVIM = 6,
	IPC_SEM_TELE_MNTN = 7,
	IPC_SEM_MEDPLL_STATE = 8,
	IPC_SEM_EFUSE = 9,
	IPC_SEM_BBPMASTER_0 = 10,
	IPC_SEM_BBPMASTER_1 = 11,
	IPC_SEM_BBPMASTER_2 = 12,
	IPC_SEM_BBPMASTER_3 = 13,
	IPC_SEM_GU_SLEEP = 14,
	IPC_SEM_BBPMASTER_5 = 15,
	IPC_SEM_BBPMASTER_6 = 16,
	IPC_SEM_BBPMASTER_7 = 17,
	IPC_SEM_DPDT_CTRL_ANT = 19,
	IPC_SEM_SMP_CPU0 = 21,
	IPC_SEM_SMP_CPU1 = 22,
	IPC_SEM_SMP_CPU2 = 23,
	IPC_SEM_SMP_CPU3 = 24,
	IPC_SEM_SYNC = 25,
	IPC_SEM_BBP = 26,
	IPC_SEM_CPUIDLE = 27,
	IPC_SEM_BBPPS = 28,
	IPC_SEM_HKADC = 29,
	IPC_SEM_SYSCTRL = 30,
	IPC_SEM_ZSP_HALT = 31,
	IPC_SEM_BUTTOM
};

#else
enum ipc_int_lev {
	IPC_CCPU_INT_SRC_HIFI_MSG = 0,
	IPC_CCPU_INT_SRC_MCU_MSG = 1,
	IPC_INT_DSP_HALT = 2,
	IPC_INT_DSP_RESUME = 3,

	IPC_INT_WAKE_GU = 6,
	IPC_INT_SLEEP_GU,

	/* Define the mailbox IPC interrupt used by TDS */
	IPC_INT_WAKE_GSM,
	IPC_INT_WAKE_WCDMA,
	IPC_INT_DSP_PS_PUB_MBX,
	IPC_INT_DSP_PS_MAC_MBX,
	IPC_INT_DSP_MBX_RSD,
	IPC_CCPU_INT_SRC_DSP_MNTN,
	IPC_CCPU_INT_SRC_DSP_RCM_PUB_MBX,
	IPC_CCPU_INT_SDR_CCPU_BBP_MASTER_ERROR,
	IPC_CCPU_INT_SRC_COMMON_END,

	/* C core receiving, v7 specific ipc interrupt */
	IPC_CCPU_INT_SRC_ACPU_RESET =
		IPC_CCPU_INT_SRC_COMMON_END,
	IPC_CCPU_SRC_ACPU_DUMP,
	IPC_CCPU_INT_SRC_ICC_PRIVATE,
	IPC_CCPU_INT_SRC_MCPU,
	IPC_CCPU_INT_SRC_MCPU_WDT,
	IPC_CCPU_INT_SRC_XDSP_1X_HALT,
	IPC_CCPU_INT_SRC_XDSP_HRPD_HALT,
	IPC_CCPU_INT_SRC_XDSP_1X_RESUME,
	IPC_CCPU_INT_SRC_XDSP_HRPD_RESUME,
	IPC_CCPU_INT_SRC_XDSP_MNTN,
	IPC_CCPU_INT_SRC_XDSP_PS_MBX,
	IPC_CCPU_INT_SRC_XDSP_RCM_MBX,

	IPC_CCPU_INT_SRC_ACPU_ICC = 31,

	/* C core receiving, v8 specific ipc interrupt */
	IPC_CCPU_INT_SRC_ACPU_MSG = IPC_CCPU_INT_SRC_COMMON_END,
	IPC_CCPU_INT_SRC_ACPU_IFC,
	IPC_CCPU_INT_SRC_MCU_IFC,
	IPC_INT_ARM_SLEEP,

	/* Define the bit position of MCU IPC message interrupt source */
	IPC_MCU_INT_SRC_ACPU_MSG = 0,
	IPC_MCU_INT_SRC_CCPU_MSG,
	IPC_MCU_INT_SRC_HIFI_MSG,
	IPC_MCU_INT_SRC_CCPU_IPF,
	IPC_MCU_INT_SRC_ACPU_PD,
	IPC_MCU_INT_SRC_HIFI_PD,
	IPC_MCU_INT_SRC_HIFI_DDR_VOTE,
	IPC_MCU_INT_SRC_ACPU_I2S_REMOTE_SLOW,
	IPC_MCU_INT_SRC_ACPU_I2S_REMOTE_SLEEP,
	IPC_MCU_INT_SRC_ACPU_I2S_REMOTE_INVALID,
	IPC_MCU_INT_SRC_COMMON_END,

	/* v7 specific ipc interrupt */
	IPC_MCU_INT_SRC_ACPU_DRX = IPC_MCU_INT_SRC_COMMON_END,
	IPC_MCU_INT_SRC_CCPU_DRX,
	IPC_MCU_INT_SRC_ICC_PRIVATE,
	IPC_MCU_INT_SRC_DUMP,
	IPC_MCU_INT_SRC_HIFI_PU,
	IPC_MCU_INT_SRC_HIFI_DDR_DFS_QOS,
	IPC_MCU_INT_SRC_TEST,
	IPC_MCPU_INT_SRC_ACPU_USB_PME_EN,
	IPC_MCU_INT_SRC_ICC = 29,
	IPC_MCU_INT_SRC_CCPU_PD = 30,
	IPC_MCU_INT_SRC_CCPU_START = 31,

	/* v8 specific ipc interrupt */
	IPC_MCU_INT_SRC_CCPU_IFC = IPC_MCU_INT_SRC_COMMON_END,
	IPC_MCU_INT_SRC_ACPU_IFC,
	IPC_MCU_INT_SRC_HIFI_IFC,
	IPC_MCU_INT_SRC_ACPU0_PD,
	IPC_MCU_INT_SRC_ACPU1_PD,
	IPC_MCU_INT_SRC_ACPU2_PD,
	IPC_MCU_INT_SRC_ACPU3_PD,
	IPC_MCU_INT_SRC_ACPU4_PD,
	IPC_MCU_INT_SRC_ACPU5_PD,
	IPC_MCU_INT_SRC_ACPU6_PD,
	IPC_MCU_INT_SRC_ACPU7_PD,
	IPC_MCU_INT_SRC_ACPU_HOTPLUG,
	IPC_MCU_INT_SRC_MCU_AGT,
	IPC_MCU_INT_SRC_HIFI_MEMSHARE_DDR_VOTE,
	IPC_MCU_INT_SRC_HIFI_MEMSHARE_DDR_EXIT_VOTE,
	IPC_MCU_INT_SRC_ACPU_DFS,
	IPC_MCU_INT_SRC_END,

	/* Define the bit position of ACPU IPC message interrupt source */
	IPC_ACPU_INT_SRC_CCPU_MSG = 0,
	IPC_ACPU_INT_SRC_HIFI_MSG = 1,
	IPC_ACPU_INT_SRC_MCU_MSG = 2,
	IPC_ACPU_INT_SRC_CCPU_NVIM = 3,
	IPC_INT_DICC_USRDATA = 4,
	IPC_INT_DICC_RELDATA = 5,
	IPC_ACPU_INT_SRC_CCPU_ICC,
	IPC_ACPU_INT_SRC_COMMON_END,

	/* v7 specific ipc interrupt */
	IPC_ACPU_INT_SRC_ICC_PRIVATE = IPC_ACPU_INT_SRC_COMMON_END,
	IPC_ACPU_SRC_CCPU_DUMP,
	IPC_ACPU_INT_SRC_MCPU,
	IPC_ACPU_INT_SRC_MCPU_WDT,
	IPC_ACPU_INT_MCU_SRC_DUMP,
	IPC_ACPU_INT_SRC_CCPU_RESET_IDLE,
	IPC_ACPU_INT_SRC_CCPU_RESET_SUCC,

	IPC_ACPU_INT_SRC_CCPU_LOG,
	IPC_ACPU_INT_SRC_MCU_FOR_TEST,
	IPC_ACPU_INT_SRC_CCPU_TEST_ENABLE,
	IPC_ACPU_INT_SRC_MCPU_USB_PME,
	IPC_ACPU_INT_SRC_HIFI_PC_VOICE_RX_DATA,
	IPC_ACPU_INT_SRC_CCPU_PM_OM,

	/* v8 specific ipc interrupt */
	IPC_ACPU_INT_SRC_CCPU_IFC = IPC_ACPU_INT_SRC_COMMON_END,
	IPC_ACPU_INT_SRC_MCU_IFC,
	IPC_ACPU_INT_SRC_MCU_THERMAL_HIGH,
	IPC_ACPU_INT_SRC_MCU_THERMAL_LOW,
	IPC_ACPU_INI_SRC_MCU_TELE_MNTN_NOTIFY,
	IPC_ACPU_INI_SRC_MCU_EXC_REBOOT,
	IPC_ACPU_INT_SRC_CCPU_EXC_REBOOT,
	IPC_ACPU_INT_SRC_CCPU_NORMAL_REBOOT,
	IPC_ACPU_INT_SRC_MCU_DDR_EXC,
	IPC_ACPU_INT_SRC_END,

	/* Define the bit position of HIFI IPC message interrupt source */
	IPC_HIFI_INT_SRC_ACPU_MSG = 0,
	IPC_HIFI_INT_SRC_CCPU_MSG,
	IPC_HIFI_INT_SRC_BBE_MSG,
	IPC_HIFI_INT_SRC_MCU_MSG,
	IPC_HIFI_INT_SRC_COMMON_END,

	/* v8 specific ipc interrupt */
	IPC_HIFI_INT_SRC_MCU_WAKE_DDR = IPC_HIFI_INT_SRC_COMMON_END,
	IPC_HIFI_INT_SRC_MCU_IFC,

	IPC_HIFI_INT_SRC_END,

	/* Define the bit position of BBE16 IPC message interrupt source */
	IPC_INT_MSP_DSP_OM_MBX = 0,
	IPC_INT_PS_DSP_PUB_MBX,
	IPC_INT_PS_DSP_MAC_MBX,
	IPC_INT_HIFI_DSP_MBX,
	IPC_BBE16_INT_SRC_HIFI_MSG,

	IPC_BBE16_INT_SRC_END,

	/* Define the bit position of XDSP IPC message interrupt source */
	IPC_XDSP_INT_SRC_CCPU_1X_WAKE = 0,
	IPC_XDSP_INT_SRC_CCPU_HRPD_WAKE,
	IPC_XDSP_INT_SRC_CCPU_OM_MBX,
	IPC_XDSP_INT_SRC_CCPU_PUB_MBX,

	IPC_XDSP_INT_SRC_END,

	IPC_INT_BUTTOM = 32
};

enum ipc_sem_id {
	IPC_SEM_MEM,
	IPC_SEM_DICC,
	IPC_SEM_EMMC,
	IPC_SEM_SYNC,
	IPC_SEM_SYSCTRL,
	IPC_SEM_BBP,
	IPC_SEM_RFILE_LOG,
	IPC_SEM_NVIM,
	IPC_SEM_EFUSE,
	IPC_SEM_DPDT_CTRL_ANT,
	IPC_SEM_BBPMASTER_0,
	IPC_SEM_BBPMASTER_1,
	IPC_SEM_BBPMASTER_2,
	IPC_SEM_BBPMASTER_3,
	IPC_SEM_BBPMASTER_5,
	IPC_SEM_BBPMASTER_6,
	IPC_SEM_BBPMASTER_7,
	IPC_SEM_COMMON_END,

	/* v7 private IPC SEM */
	IPC_SEM_SPI0 = IPC_SEM_COMMON_END,
	IPC_SEM_NV,
	IPC_SEM_GPIO,
	IPC_SEM_CLK,
	IPC_SEM_PMU,
	IPC_SEM_MTCMOS,
	IPC_SEM_IPF_PWCTRL,
	IPC_SEM_PMU_FPGA,
	IPC_SEM_NV_CRC,
	IPC_SEM_PM_OM_LOG,
	IPC_SEM_MDRV_LOCK,
	IPC_SEM_CDMA_DRX,
	IPC_SEM_GU_SLEEP,
	IPC_SEM2_IPC_TEST,

	/* v8 private IPC SEM */
	IPC_SEM_ICC = IPC_SEM_COMMON_END,
	IPC_SEM_NAND,
	IPC_SEM_TELE_MNTN,
	IPC_SEM_MEDPLL_STATE,
	IPC_SEM_SMP_CPU0,
	IPC_SEM_SMP_CPU1,
	IPC_SEM_SMP_CPU2,
	IPC_SEM_SMP_CPU3,
	IPC_SEM_CPUIDLE,
	IPC_SEM_BBPPS,
	IPC_SEM_HKADC,

	IPC_SEM_END,

	IPC_SEM_BUTTOM = 32
};
#endif

#endif /* __MDRV_IPC_ENUM_H__ */