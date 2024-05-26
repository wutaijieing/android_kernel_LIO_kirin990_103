/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: vendor smc define header.
 *              This is follow ARM SMCCC 1.X.
 * Create : 2019/07/01
 */

/*
 * This is a document file for reference
 *
 * 0xc3000000-0xc300FFFF SMC64: OEM Service Calls
 *
 * 0xc300 0000  ~  0xc300 FFFF +------> sip fid define
 *                        | |
 *                        | |
 *                        | +----------> Fid in module  (0 ~ 255)
 *                        |
 *                        +------------> Module Group   (0 ~ 255)
 *
 * check usage:
 *              FID_USE(fid) == FID_ARF_GROUP
 */

#ifndef __BL31_SMC_H__
#define __BL31_SMC_H__

/* Get the group number of FID */
#define fid_bl31_use(x)                      (((x) & 0xff00) >> 8)

/* The base of OEM Service */
#define OEM_BASE                             0xc3U

/* Module Group Number Define */
#define FID_ARF_GROUP                        0x00
#define FID_ISP_GROUP                        0x01
#define FID_IPP_GROUP                        0x02
#define FID_IVP_GROUP                        0x03
#define FID_LB_GROUP                         0x04
#define FID_PINCTRL_GROUP                    0x05
#define FID_RTC_GROUP                        0x06
#define FID_REGU_GROUP                       0x07
#define FID_SPMI_GROUP                       0x08
#define FID_THERMAL_GROUP                    0x09
#define FID_ITS_GROUP                        0x0a
#define FID_HISTDSVC_GROUP                   0x0b
#define FID_GENERAL_SEE_GROUP                0x0c
#define FID_CRYPTO_ENHANCE_GROUP             0x0d
#define FID_L3CACHE_GROUP                    0x0e
#define FID_MNTN_GROUP                       0x0f
#define FID_NPU_GROUP                        0x10
#define FID_SECIO_GROUP                      0x11
#define FID_RPMB_GROUP                       0x12
#define FID_FREQDUMP_GROUP                   0x13
#define FID_GETVAR_GROUP                     0x14
#define FID_AVS_GROUP                        0x15
#define FID_BL31LOG_GROUP                    0x16
#define FID_SECTEST_GROUP                    0x17
#define FID_EFUSE_GROUP                      0x18
#define FID_EFUSEV1_GROUP                    0x19
#define FID_EFUSEV2_GROUP                    0x1a
#define FID_DSUPCTRL_GROUP                   0x1b
#define FID_HSDTTRACE_GROUP                  0x1c
#define FID_BOOT_GROUP                       0x1d
#define FID_SMMUV3_GROUP                     0x1e
#define FID_HIBERNATE_GROUP                  0x24
#define FID_USB_GROUP                        0x25
#define FID_VDEC_GROUP                       0x26
#define FID_PCIE_GROUP                       0x27
#define FID_CPUTEST_GROUP                    0x28
#define FID_THEE_GROUP                       0x29
#define FID_RAS_GROUP                        0x2c
#define FID_ATF_FRAMEWORK_GROUP              0x2d
#define FID_ATF_FRAMEWORK_TEST               0x2e
#define FID_BL31_MAXGROUP                    0x33 /* max group */

/* Get FID from group and id */
#define make_fid(group, id)                  ((((OEM_BASE << 16) + group) << \
						8) + id)

/*
 * access register
 * 0xc3000000 ~ 0xc30000ff
 */
#define ACCESS_REGISTER_FN_MAIN_ID           make_fid(FID_ARF_GROUP, 0x00)

/*
 * isp:
 * 0xc3000100 ~ 0xc30001ff
 */
#define ISP_FN_SET_PARAMS                    make_fid(FID_ISP_GROUP, 0x00)
#define ISP_FN_ISPCPU_NS_DUMP                make_fid(FID_ISP_GROUP, 0x02)
#define ISP_FN_ISP_INIT                      make_fid(FID_ISP_GROUP, 0x03)
#define ISP_FN_ISP_EXIT                      make_fid(FID_ISP_GROUP, 0x04)
#define ISP_FN_ISPCPU_INIT                   make_fid(FID_ISP_GROUP, 0x05)
#define ISP_FN_ISPCPU_EXIT                   make_fid(FID_ISP_GROUP, 0x06)
#define ISP_FN_ISPCPU_MAP                    make_fid(FID_ISP_GROUP, 0x07)
#define ISP_FN_ISPCPU_UNMAP                  make_fid(FID_ISP_GROUP, 0x08)
#define ISP_FN_SET_NONSEC                    make_fid(FID_ISP_GROUP, 0x09)
#define ISP_FN_DISRESET_ISPCPU               make_fid(FID_ISP_GROUP, 0x0a)
#define ISP_FN_ISPSMMU_NS_INIT               make_fid(FID_ISP_GROUP, 0x0b)
#define ISP_FN_GET_ISPCPU_IDLE               make_fid(FID_ISP_GROUP, 0x0c)
#define ISP_FN_ISPTOP_PU                     make_fid(FID_ISP_GROUP, 0x0d)
#define ISP_FN_ISPTOP_PD                     make_fid(FID_ISP_GROUP, 0x0e)
#define ISP_PHY_CSI_CONNECT                  make_fid(FID_ISP_GROUP, 0x0f)
#define ISP_PHY_CSI_DISCONNECT               make_fid(FID_ISP_GROUP, 0x10)
#define ISP_FN_SEND_FIQ_TO_ISPCPU            make_fid(FID_ISP_GROUP, 0x11)
#define ISP_FN_QOS_CFG                       make_fid(FID_ISP_GROUP, 0x12)
#define ISP_FN_ISPNOC_R8_PU                  make_fid(FID_ISP_GROUP, 0x13)
#define ISP_FN_ISPNOC_R8_PD                  make_fid(FID_ISP_GROUP, 0x14)
#define ISP_FN_MEDIA2_VBUS_PU                make_fid(FID_ISP_GROUP, 0x15)
#define ISP_FN_MEDIA2_VBUS_PD                make_fid(FID_ISP_GROUP, 0x16)
#define ISP_FN_VQ_MAP                        make_fid(FID_ISP_GROUP, 0x17)
#define ISP_FN_VQ_UNMAP                      make_fid(FID_ISP_GROUP, 0x18)
#define ISP_FN_HICSI_SEL                     make_fid(FID_ISP_GROUP, 0x20)
#define ISP_FN_I2C_PAD_TYPE                  make_fid(FID_ISP_GROUP, 0x21)
#define ISP_FN_I2C_DEADLOCK                  make_fid(FID_ISP_GROUP, 0x22)

/*
 * hipp
 * 0xc3000200 ~ 0xc30002ff
 */
#define IPP_FID_SMMUENABLE              make_fid(FID_IPP_GROUP, 0x00)
#define IPP_FID_SMMUDISABLE             make_fid(FID_IPP_GROUP, 0x01)
#define IPP_FID_SMMUSMRX                make_fid(FID_IPP_GROUP, 0x02)
#define IPP_FID_SMMUPREFCFG             make_fid(FID_IPP_GROUP, 0x03)

#define IPP_FID_TRUSTEDMAP              make_fid(FID_IPP_GROUP, 0x04)
#define IPP_FID_TRUSTEDUNMAP            make_fid(FID_IPP_GROUP, 0x05)
#define IPP_FID_SHAREDMEMORY            make_fid(FID_IPP_GROUP, 0x06)
#define IPP_FID_PTBINIT                 make_fid(FID_IPP_GROUP, 0x07)
#define IPP_FID_PTBEXIT                 make_fid(FID_IPP_GROUP, 0x08)

#define IPP_FID_ORBINIT                 make_fid(FID_IPP_GROUP, 0x09)
#define IPP_FID_ORBDEINIT               make_fid(FID_IPP_GROUP, 0x0A)

#define IPP_FID_VBUSPWRUP               make_fid(FID_IPP_GROUP, 0x0B)
#define IPP_FID_VBUSPWRDN               make_fid(FID_IPP_GROUP, 0x0C)
#define IPP_FID_IPPPWRUP                make_fid(FID_IPP_GROUP, 0x0D)
#define IPP_FID_IPPPWRDN                make_fid(FID_IPP_GROUP, 0x0E)
#define IPP_FID_MAXTYPE                 make_fid(FID_IPP_GROUP, 0x0F)
/*
 * ivp
 * 0xc3000300 ~ 0xc30003ff
 */
#define IVP_FID_VALUE                   make_fid(FID_IVP_GROUP, 0x00)

/*
 * lb
 * 0xc3000400 ~ 0xc30004ff
 */
#define MM_LB_FID_VALUE                    make_fid(FID_LB_GROUP, 0x00)

/*
 * pin ctrl
 * 0xc3000500 ~ 0xc300005ff
 */
#define PINCTRL_FID_VALUE                    make_fid(FID_PINCTRL_GROUP, 0x00)

/*
 * trc
 * 0xc3000600 ~ 0xc3000600
 */
#define RTC_FID_VALUE                        make_fid(FID_RTC_GROUP, 0x00)

/*
 * regulator
 * 0xc3000700 ~ 0xc30007ff
 */
#define IP_REGULATOR_FID_VALUE               make_fid(FID_REGU_GROUP, 0x00)

/*
 * spmi
 * 0xc3000800 ~ 0xc30008ff
 */
#define SPMI_FID_VALUE                       make_fid(FID_SPMI_GROUP, 0x00)

/*
 * thermal
 * 0xc3000900 ~ 0xc30009ff
 */
#define THERMAL_FID_VALUE                    make_fid(FID_THERMAL_GROUP, 0x00)

/*
 * ITS
 * 0xc3000a00 ~ 0xc3000aff
 */
#define ITS_FID_VALUE                        make_fid(FID_ITS_GROUP, 0x00)

/*
 * std svc call
 * 0xc3000b00 ~ 0xc3000bff
 */
#define STDSVC_CALL_COUNT                    make_fid(FID_HISTDSVC_GROUP, 0x00)
#define STDSVC_UID                           make_fid(FID_HISTDSVC_GROUP, 0x01)
#define STDSVC_VERSION                       make_fid(FID_HISTDSVC_GROUP, 0x02)

/*
 * general_see
 * 0xc3000c00 ~ 0xc3000cff
 */
#define GENERAL_SEE_FID_VALUE                make_fid(FID_GENERAL_SEE_GROUP, 0x01)
#define GENERAL_SEE_FID_MNTN_VALUE           make_fid(FID_GENERAL_SEE_GROUP, 0x02)
#define GENERAL_SEE_FID_MNTN_MASK            make_fid(FID_GENERAL_SEE_GROUP, 0x03)
#define GENERAL_SEE_TEST_FID_MASK            make_fid(FID_GENERAL_SEE_GROUP, 0x04)
#define GENERAL_SEE_TEST_FID_VALUE           make_fid(FID_GENERAL_SEE_GROUP, 0x05)
#define GENERAL_SEE_FIQ_FID_MASK             make_fid(FID_GENERAL_SEE_GROUP, 0x06)
#define GENERAL_SEE_FIQ_FID_VALUE            make_fid(FID_GENERAL_SEE_GROUP, 0x07)

/*
 * crypto_enhance
 * 0xc3000d00 ~ 0xc3000dff
 */
#define CRYPTO_ENHANCE_FID_VALUE             make_fid(FID_CRYPTO_ENHANCE_GROUP, 0x01)
#define CRYPTO_ENHANCE_KERNEL_FID_MASK       make_fid(FID_CRYPTO_ENHANCE_GROUP, 0x02)
#define CRYPTO_ENHANCE_KERNEL_FID_VALUE      make_fid(FID_CRYPTO_ENHANCE_GROUP, 0x03)


/*
 * L3Cache
 * 0xc3000e00 ~ 0xc3000eff
 */
#define L3EXCLUSIVE_FID_VALUE               make_fid(FID_L3CACHE_GROUP, 0x01)
#define L3ECC_FID_VALUE                     make_fid(FID_L3CACHE_GROUP, 0x02)
#define L3SHARE_FID_VALUE                   make_fid(FID_L3CACHE_GROUP, 0x03)

/*
 * mntn
 * 0xc3000f00 ~ 0xc3000fff
 */
#define MNTN_BB_CPUID_FID_VALUE            make_fid(FID_MNTN_GROUP, 0x00)
#define MNTN_BB_BL31INIT_FID_VALUE         make_fid(FID_MNTN_GROUP, 0x01)
#define MNTN_BB_TST_FID_VALUE               make_fid(FID_MNTN_GROUP, 0x02)
#define MNTN_MEMUPLOAD                       make_fid(FID_MNTN_GROUP, 0x03)
#define MNTN_MEMREAD                         make_fid(FID_MNTN_GROUP, 0x04)
#define MNTN_MEMWRITE                        make_fid(FID_MNTN_GROUP, 0x05)
#define MNTN_DUMP                            make_fid(FID_MNTN_GROUP, 0x06)
#define MNTN_JTAG_SET                        make_fid(FID_MNTN_GROUP, 0x07)
#define MNTN_GET_SLT_FLG                     make_fid(FID_MNTN_GROUP, 0x08)
#define MNTN_SAVE_BL31_SLT_FLG               make_fid(FID_MNTN_GROUP, 0x09)
#define MNTN_DUMP_M3_SRAM                    make_fid(FID_MNTN_GROUP, 0x0a)
#define MNTN_SET_LPMCU_REASON                make_fid(FID_MNTN_GROUP, 0x0b)
#define MNTN_DUMP_AOTCP                      make_fid(FID_MNTN_GROUP, 0x0c)
#define MNTN_NOC_SET_NPU_NOSEC               make_fid(FID_MNTN_GROUP, 0x0d)
#define MNTN_QIC_FID_VALUE                   make_fid(FID_MNTN_GROUP, 0x0e)
#define MNTN_LOG_BUF_DUMP                    make_fid(FID_MNTN_GROUP, 0x0f)
#define MNTN_DISABLE_DMSS_INT_VALUE          make_fid(FID_MNTN_GROUP, 0x10)
#define MNTN_SET_REBOOT_REASON_SUBTYPE       make_fid(FID_MNTN_GROUP, 0x11)

enum MNTN_QIC_FID_SUBTYPE {
	QIC_MEDIA1_FID_SUBTYPE = 0xFFFF0001,
	QIC_MEDIA2_FID_SUBTYPE,
#ifdef CONFIG_DFX_SEC_QIC
	QIC_INIT_FIQ_SUBTYPE,
	QIC_SECIRQ_FID_SUBTYPE,
#endif
	QIC_FID_SUBTYPE_MAX,
};

/*
 * npu
 * 0xc3001000 ~ 0xc30010ff
 */
#define NPU_FID_VALUE                        make_fid(FID_NPU_GROUP, 0x00)

/*
 * secure io
 * 0xc3001100 ~ 0xc30011ff
 */
#define SECUREIO_FID_VALUE                   make_fid(FID_SECIO_GROUP, 0x01)
#define SECURE_I2C_READ_REG                  make_fid(FID_SECIO_GROUP, 0x02)
#define SECURE_I2C_WRITE_REG                 make_fid(FID_SECIO_GROUP, 0x03)
#define SECURE_I2C_XFER_LOCK                 make_fid(FID_SECIO_GROUP, 0x04)
#define SECURE_I2C_XFER_UNLOCK               make_fid(FID_SECIO_GROUP, 0x05)
#define SECURE_GPIO_READ_REG                 make_fid(FID_SECIO_GROUP, 0x06)
#define SECURE_GPIO_WRITE_REG                make_fid(FID_SECIO_GROUP, 0x07)

/*
 * rpmb
 * 0xc3001200 ~ 0xc30012ff
 */
#define RPMB_SVC_CALL_COUNT                  make_fid(FID_RPMB_GROUP, 0x00)
#define RPMB_SVC_UID                         make_fid(FID_RPMB_GROUP, 0x01)
#define RPMB_SVC_VERSION                     make_fid(FID_RPMB_GROUP, 0x02)
#define RPMB_SVC_TEST                        make_fid(FID_RPMB_GROUP, 0x03)
#define RPMB_SVC_REQUEST_ADDR                make_fid(FID_RPMB_GROUP, 0x04)
#define RPMB_SVC_SET_KEY                     make_fid(FID_RPMB_GROUP, 0x05)
#define RPMB_SVC_REQUEST_DONE                make_fid(FID_RPMB_GROUP, 0x06)
#define RPMB_SVC_MULTI_KEY_STATUS            make_fid(FID_RPMB_GROUP, 0x07)
#define RPMB_SVC_GET_DEV_VER                 make_fid(FID_RPMB_GROUP, 0x08)
/* for secure os */
#define RPMB_SVC_SECURE_OS_INFO              make_fid(FID_RPMB_GROUP, 0x09)
/* for ufs secure */
#define UFS_SVC_REG_SET                      make_fid(FID_RPMB_GROUP, 0x0a)
/* debug only */
#define RPMB_SVC_READ                        make_fid(FID_RPMB_GROUP, 0x0b)
#define RPMB_SVC_WRITE                       make_fid(FID_RPMB_GROUP, 0x0c)
#define RPMB_SVC_COUNTER                     make_fid(FID_RPMB_GROUP, 0x0d)
#define RPMB_SVC_FORMAT                      make_fid(FID_RPMB_GROUP, 0x0e)
#define RPMB_SVC_WRITE_CAPABILITY            make_fid(FID_RPMB_GROUP, 0x0f)
#define RPMB_SVC_READ_CAPABILITY             make_fid(FID_RPMB_GROUP, 0x11)
#define RPMB_SVC_PARTITION                   make_fid(FID_RPMB_GROUP, 0x12)
#define RPMB_SVC_MULTI_KEY                   make_fid(FID_RPMB_GROUP, 0x13)
#define RPMB_SVC_CONFIG_VIEW                 make_fid(FID_RPMB_GROUP, 0x14)


/*
 * freqdump
 * 0xc3001300 ~ 0xc30013ff
 */
#define FREQDUMP_SVC_CALL_COUNT              make_fid(FID_FREQDUMP_GROUP, 0x00)
#define FREQDUMP_SVC_UID                     make_fid(FID_FREQDUMP_GROUP, 0x01)
#define FREQDUMP_SVC_VERSION                 make_fid(FID_FREQDUMP_GROUP, 0x02)
#define FREQDUMP_SVC_REG_RD                  make_fid(FID_FREQDUMP_GROUP, 0x03)
#define FREQDUMP_LOADMONITOR_SVC_REG_RD      make_fid(FID_FREQDUMP_GROUP, 0x04)
#define FREQDUMP_LOADMONITOR_SVC_ENABLE      make_fid(FID_FREQDUMP_GROUP, 0x05)
#define FREQDUMP_LOADMONITOR_SVC_DISABLE     make_fid(FID_FREQDUMP_GROUP, 0x06)
#define FREQDUMP_ADC_SVC_SET_PARAM           make_fid(FID_FREQDUMP_GROUP, 0x07)
#define FREQDUMP_ADC_SVC_DISABLE             make_fid(FID_FREQDUMP_GROUP, 0x08)
#define FREQDUMP_LOADMONITOR_SVC_ENABLE_READ make_fid(FID_FREQDUMP_GROUP, 0x09)
#define MEDIA_MONITOR_SVC_ENABLE             make_fid(FID_FREQDUMP_GROUP, 0x0a)
#define MEDIA_MONITOR_SVC_DISABLE            make_fid(FID_FREQDUMP_GROUP, 0x0b)
#define MEDIA_MONITOR_SVC_SET_FLAG           make_fid(FID_FREQDUMP_GROUP, 0x0c)
#define FREQDUMP_GCOV_INIT_CALL              make_fid(FID_FREQDUMP_GROUP, 0x0d)
#define LOADMONITOR_SVC_REG_GET              make_fid(FID_FREQDUMP_GROUP, 0x0e)
#define LOADMONITOR_SVC_NPU_RD               make_fid(FID_FREQDUMP_GROUP, 0x0f)

/*
 * get var
 * 0xc3001400 ~ 0xc30014ff
 */
#define GET_CPU_VOLT_FID_VALUE          make_fid(FID_GETVAR_GROUP, 0x00U)
#define HISI_GET_WA_COUNTER_FID_VALUE        make_fid(FID_GETVAR_GROUP, 0x01U)

/*
 * avs
 * 0xc3001500 ~ 0xc30015ff
 */
#define AVS_STD_SVC_VER                     make_fid(FID_AVS_GROUP, 0x00)
#define AVS_SVC_CALL_COUNT                  make_fid(FID_AVS_GROUP, 0x01)
#define AVS_SVC_UID                         make_fid(FID_AVS_GROUP, 0x02)
#define AVS_SVC_VERSION                     make_fid(FID_AVS_GROUP, 0x03)
#define AVS_NPU_PA_MODE_CFG                 make_fid(FID_AVS_GROUP, 0x04)
#define AVS_NPU_PA_MODE_RECOVERY            make_fid(FID_AVS_GROUP, 0x05)
#define AVS_NPU_GET_PA_VALUE                make_fid(FID_AVS_GROUP, 0x06)

/*
 * bl31 log
 * 0xc3001600 ~ 0xc30016ff
 */
#define ATF_LOG_FID_VALUE                    make_fid(FID_BL31LOG_GROUP, 0x00)
#define ATF_BL31_LOG_INFO                    make_fid(FID_BL31LOG_GROUP, 0X01)
#define ATF_BL2_LOG_INFO                     make_fid(FID_BL31LOG_GROUP, 0X02)

/*
 * efuse
 * 0xc3001800 ~ 0xc30018ff
 */
#define EFUSE_FN_VERSION                     make_fid(FID_EFUSE_GROUP, 0x01)
#define EFUSE_FN_TEST_WR                     make_fid(FID_EFUSE_GROUP, 0x02)
#define EFUSE_FN_GET_KCE                     make_fid(FID_EFUSE_GROUP, 0x03)
#define EFUSE_FN_SET_DJTAGDEBUG              make_fid(FID_EFUSE_GROUP, 0x04)
#define EFUSE_FN_GET_DJTAGDEBUG              make_fid(FID_EFUSE_GROUP, 0x05)
#define EFUSE_FN_GET_DESKEW                  make_fid(FID_EFUSE_GROUP, 0x06)

/*
 * efuse v1
 * 0xc3001900 ~ 0xc30019ff
 */
#define EFUSE_FN_GET_DIEID                   make_fid(FID_EFUSEV1_GROUP, 0x00)
#define EFUSE_FN_GET_CHIPID                  make_fid(FID_EFUSEV1_GROUP, 0x01)
#define EFUSE_FN_SET_CHIPID                  make_fid(FID_EFUSEV1_GROUP, 0x02)
#define EFUSE_FN_GET_AUTHKEY                 make_fid(FID_EFUSEV1_GROUP, 0x03)
#define EFUSE_FN_SET_AUTHKEY                 make_fid(FID_EFUSEV1_GROUP, 0x04)
#define EFUSE_FN_GET_SECURITYDEBUG           make_fid(FID_EFUSEV1_GROUP, 0x05)
#define EFUSE_FN_SET_SECURITYDEBUG           make_fid(FID_EFUSEV1_GROUP, 0x06)
#define EFUSE_FN_SET_KCE                     make_fid(FID_EFUSEV1_GROUP, 0x09)
#define EFUSE_FN_GET_SLTFINISHFLAG           make_fid(FID_EFUSEV1_GROUP, 0x0c)
#define EFUSE_FN_SET_SLTFINISHFLAG           make_fid(FID_EFUSEV1_GROUP, 0x0d)
#define EFUSE_FN_SET_GENERAL_SEE             make_fid(FID_EFUSEV1_GROUP, 0x0e)
#define EFUSE_FN_GET_GENERAL_SEE             make_fid(FID_EFUSEV1_GROUP, 0x0f)
#define EFUSE_FN_GET_AVS                     make_fid(FID_EFUSEV1_GROUP, 0x10)
#define EFUSE_FN_GET_PARTIAL_PASS_INFO       make_fid(FID_EFUSEV1_GROUP, 0x11)
#define EFUSE_FN_GET_NVCNT                   make_fid(FID_EFUSEV1_GROUP, 0x12)
#define EFUSE_FN_SET_NVCNT                   make_fid(FID_EFUSEV1_GROUP, 0x13)
/*
 * efuse v2
 * 0xc3001a00 ~ 0xc3001aff
 */
#define EFUSE_FN_ENABLE_DEBUG                make_fid(FID_EFUSEV2_GROUP, 0x08)
#define EFUSE_FN_DISABLE_DEBUG               make_fid(FID_EFUSEV2_GROUP, 0x09)
#define EFUSE_FN_RD_NVCNT                    make_fid(FID_EFUSEV2_GROUP, 0x12)
#define EFUSE_FN_WR_NVCNT                    make_fid(FID_EFUSEV2_GROUP, 0x13)

/*
 * dsu pctrl
 * 0xc3001b00 ~ 0xc3001bff
 */
#define DSU_PCTRL_FID_VALUE             make_fid(FID_DSUPCTRL_GROUP, 0x00)

/*
 * hsdt trace
 * 0xc3001c00 ~ 0xc3001cff
 */
#define HSDT_TRACE_FID_VALUE                 make_fid(FID_HSDTTRACE_GROUP, 0x00)

/*
 * uefi boot
 * 0xc3001d00 ~ 0xc3001dff
 */
#define FID_BOOT_ONCHIPROM_DL                make_fid(FID_BOOT_GROUP, 0x00)
#define FID_BOOT_BL31_READ_CHIPID            make_fid(FID_BOOT_GROUP, 0x01)
#define FID_BOOT_READ_SEC_REGISTER           make_fid(FID_BOOT_GROUP, 0x02)
#define FID_BOOT_WRITE_SEC_REGISTER          make_fid(FID_BOOT_GROUP, 0x03)

/*
 * smmu tcu
 * 0xc3001e00 ~ 0xc3001eff
 */
#define MM_SMMU_FID_TCUCTL_UNSEC             make_fid(FID_SMMUV3_GROUP, 0x00)
#define MM_SMMU_FID_TCUCTL_SEC               make_fid(FID_SMMUV3_GROUP, 0x01)
#define MM_SMMU_FID_TCU_UNSEC                make_fid(FID_SMMUV3_GROUP, 0x02)
#define MM_SMMU_FID_NODE_STATUS              make_fid(FID_SMMUV3_GROUP, 0x03)
#define MM_SMMU_FID_TCU_CLK_ON               make_fid(FID_SMMUV3_GROUP, 0x05)
#define MM_SMMU_FID_TCU_CLK_OFF              make_fid(FID_SMMUV3_GROUP, 0x06)
#define MM_SMMU_FID_TCU_TTWC_BYPASS_ON       make_fid(FID_SMMUV3_GROUP, 0x07)

/*
 * bl31_hibernate
 * 0xc3002300 ~ 0xc30023ff
 */
#define BL31_HIBERNATE_FREEZE                make_fid(FID_HIBERNATE_GROUP, 0x00)
#define BL31_HIBERNATE_RESTORE               make_fid(FID_HIBERNATE_GROUP, 0x01)
#define BL31_HIBERNATE_STORE_K               make_fid(FID_HIBERNATE_GROUP, 0x02)
#define BL31_HIBERNATE_GET_K                 make_fid(FID_HIBERNATE_GROUP, 0x03)
#define BL31_HIBERNATE_QUERY_STATUS          make_fid(FID_HIBERNATE_GROUP, 0x04)
#define BL31_HIBERNATE_CLEAN_K               make_fid(FID_HIBERNATE_GROUP, 0x05)
#define BL31_HIBERNATE_TEST                  make_fid(FID_HIBERNATE_GROUP, 0x06)

/*
 * usb
 * 0xc3002500 ~ 0xc30025ff
 */
#define FID_USB_SET_PHY_RESET            make_fid(FID_USB_GROUP, 0x00)
#define FID_USB_GET_PHY_RESET            make_fid(FID_USB_GROUP, 0x01)
#define FID_USB_SET_VREGBYPASS_EFUSE     make_fid(FID_USB_GROUP, 0x02)
#define FID_USB_GET_VREGBYPASS_EFUSE     make_fid(FID_USB_GROUP, 0x03)

/*
 * vdec
 * 0xc3002600 ~ 0xc30026ff
 */
#define VDEC_FID_VALUE                      make_fid(FID_VDEC_GROUP, 0x0)

/*
 * pcie
 * 0xc3002700 ~ 0xc30027ff
 */
#define FID_PCIE_SET_TBU_BYPASS             make_fid(FID_PCIE_GROUP, 0x00)
#define FID_PCIE_MSIC_FUNC_SET              make_fid(FID_PCIE_GROUP, 0x01)
#define FID_PCIE_SET_MEM_UNSEC              make_fid(FID_PCIE_GROUP, 0x02)
#define FID_PCIE_GET_VREF_SEL               make_fid(FID_PCIE_GROUP, 0x03)
#define FID_PCIE_PG_FLAG                    make_fid(FID_PCIE_GROUP, 0x04)
#define FID_PCIE_PG_LEVEL                   make_fid(FID_PCIE_GROUP, 0x05)

/*
 * thee
 * 0xc3002900 ~ 0xc30029ff
 */
#define THEE_REQ_VM1                         make_fid(FID_THEE_GROUP, 0x07)
#define THEE_REQ_VM2                         make_fid(FID_THEE_GROUP, 0x08)
#define THEE_REQ_EL3                         make_fid(FID_THEE_GROUP, 0x09)
#define THEE_REQ_OTT                         make_fid(FID_THEE_GROUP, 0x10)
#define THEE_SEL2_DONE                       make_fid(FID_THEE_GROUP, 0x11)

/*
 * ras
 * 0xc3002c00 ~ 0xc3002cff
 */
#define RAS_TEST_FID_VALUE                   make_fid(FID_RAS_GROUP, 0x00)
#endif
