#ifndef __SOC_MID_H__
#define __SOC_MID_H__ 
#define SOC_LPMCU_MID 0x00
#define SOC_IOMCU_M7_MID 0x01
#define SOC_PCIE_1_MID 0x02
#define SOC_PERF_STAT_MID 0x03
#define SOC_IPF_MID 0x04
#define SOC_DJTAG_M_MID 0x05
#define SOC_IOMCU_DMA_MID 0x06
#define SOC_AO_TCP_MID 0x06
#define SOC_UFS_MID 0x07
#define SOC_SD_MID 0x08
#define SOC_SDIO_MID 0x09
#define SOC_CC712_MID 0x0A
#define SOC_FD_UL_MID 0x0B
#define SOC_DPC_MID 0x0C
#define SOC_SOCP_MID 0x0D
#define SOC_USB31OTG_MID 0x0E
#define SOC_TOP_CSSYS_MID 0x0F
#define SOC_DMAC_MID 0x10
#define SOC_ASP_HIFI_MID 0x11
#define SOC_PCIE_0_MID 0x12
#define SOC_ATGS_MID 0x13
#define SOC_ASP_DMA_MID 0x14
#define SOC_LATMON_MID 0x15
#define SOC_MODEM_DFC_MID 0x24
#define SOC_MODEM_CIPHER_MID 0x16
#define SOC_MODEM_HDLC_MID 0x36
#define SOC_MODEM_CICOM0_MID 0x17
#define SOC_MODEM_CICOM1_MID 0x37
#define SOC_MODEM_NXDSP_MID 0x18
#define SOC_MODEM_TL_BBP_DMA_TCM_MID 0x19
#define SOC_MODEM_TL_BBP_DMA_DDR_MID 0x39
#define SOC_MODEM_GU_BBP_MST_TCM_MID 0x1A
#define SOC_MODEM_GU_BBP_MST_DDR_MID 0x3A
#define SOC_MODEM_EDMA0_MID 0x1B
#define SOC_MODEM_EDMA1_MID 0x3B
#define SOC_MODEM_HARQ_L_MID 0x1C
#define SOC_MODEM_HARQ_H_MID 0x3C
#define SOC_MODEM_UPACC_MID 0x1D
#define SOC_MODEM_RSR_ACC_MID 0x3D
#define SOC_MODEM_CIPHER_WRITE_THOUGH_MID 0x1E
#define SOC_MODEM_CCPU_CFG_MID 0x3F
#define SOC_MODEM_CCPU_L2C_MID 0x1F
#define SOC_ISP_1_ISP_CORE_0_MID 0x40
#define SOC_ISP_1_ISP_CORE_1_MID 0x41
#define SOC_ISP_1_ISP_CORE_2_MID 0x42
#define SOC_ISP_1_ISP_CORE_3_MID 0x43
#define SOC_ISP_1_ISP_CORE_4_MID 0x44
#define SOC_ISP_1_ISP_CORE_5_MID 0x45
#define SOC_ISP_1_ISP_CORE_6_MID 0x46
#define SOC_ISP_1_ISP_CORE_7_MID 0x47
#define SOC_ISP_2_ISP_CORE_0_MID 0x48
#define SOC_ISP_2_ISP_CORE_1_MID 0x49
#define SOC_ISP_2_ISP_CORE_2_MID 0x4A
#define SOC_ISP_2_ISP_CORE_3_MID 0x4B
#define SOC_DSS_CMDLIST_MID 0x50
#define SOC_DSS_WR_1_MID 0x51
#define SOC_DSS_WR_0_MID 0x52
#define SOC_DSS_RD_8_MID 0x53
#define SOC_DSS_RD_7_MID 0x54
#define SOC_DSS_RD_6_MID 0x55
#define SOC_DSS_RD_5_MID 0x56
#define SOC_DSS_RD_4_MID 0x57
#define SOC_DSS_RD_3_MID 0x58
#define SOC_DSS_RD_2_MID 0x59
#define SOC_DSS_RD_1_MID 0x5A
#define SOC_DSS_RD_0_MID 0x5B
#define SOC_IPP_SUBSYS_JPGENC_MID 0x5D
#define SOC_MEDIA_COMMON_CMDLIST_MID 0x5E
#define SOC_MEDIA_COMMON_RCH_WCH_MID 0x5F
#define SOC_VENC_0_MID 0x60
#define SOC_VENC_1_MID 0x61
#define SOC_VDEC1_MID 0x62
#define SOC_VDEC2_MID 0x63
#define SOC_VDEC3_MID 0x64
#define SOC_VDEC4_MID 0x65
#define SOC_VDEC5_MID 0x66
#define SOC_VDEC6_MID 0x67
#define SOC_VENC2_0_MID 0x68
#define SOC_IVP32_DSP_DSP_CORE_INST_MID 0x69
#define SOC_IVP32_DSP_DSP_DMA_MID 0x6A
#define SOC_ISP_ISP_A7_CFG_MID 0x6B
#define SOC_IPP_SUBSYS_JPGDEC_MID 0x6C
#define SOC_IPP_SUBSYS_FD_MID 0x6D
#define SOC_IPP_SUBSYS_CPE_MID 0x6E
#define SOC_IPP_SUBSYS_SLAM_MID 0x6F
#define SOC_NPU0_MID 0x70
#define SOC_NPU1_MID 0x71
#define SOC_IVP32_DSP_DSP_CORE_DATA_MID 0x74
#define SOC_VENC2_1_MID 0x75
#define SOC_FCM_M0_MID 0x78
#define SOC_FCM_M1_MID 0x79
#define SOC_GPU0_NON_DRM_MID 0x7A
#define SOC_GPU0_DRM_MID 0x7B
#define SOC_IDI2AXI_MID 0x7C
#define SOC_HIEPS_HIEPS_MMU_PTW_MID 0x7D
#define SOC_HIEPS_HIEPS_ARC_MID 0x7E
#define SOC_HIEPS_HIEPS_SCE_MID 0x7F
#define SOC_MAX_MID 0x80
#define SEC_ASI0_MASK 0x1F
#define SEC_ASI12_MASK 0x1F
#define SEC_ASI34_MASK 0x1F
#define SEC_ASI5_MASK 0x1F
#define SEC_ASI67_MASK 0x1
#define SEC_ASI89_MASK 0x1
#define SEC_ASIABCD_MASK 0x1
#define TZMP2_ASI0_MASK 0x1F
#define TZMP2_ASI12_MASK 0x1F
#define TZMP2_ASI34_MASK 0x1F
#define TZMP2_ASI5_MASK 0x3F
#define TZMP2_ASI67_MASK 0x1F
#define TZMP2_ASI89_MASK 0x1F
#define TZMP2_ASIABCD_MASK 0x1F
#endif
