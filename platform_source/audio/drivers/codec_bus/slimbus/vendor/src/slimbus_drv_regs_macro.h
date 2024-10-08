/*
 * slimbus is a kernel driver which is used to manager slimbus devices
 *
 * Copyright (c) 2012-2018 Huawei Technologies Co., Ltd.
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

#ifndef __SLIMBUS_DRV_REGS_MACRO_H__
#define __SLIMBUS_DRV_REGS_MACRO_H__


/* macros for BlueprintGlobalNameSpace::configuration::config_mode */
#ifndef __CONFIGURATION__CONFIG_MODE_MACRO__
#define __CONFIGURATION__CONFIG_MODE_MACRO__

/* macros for field ENABLE */
#define CONFIGURATION__CONFIG_MODE__ENABLE__SHIFT					0
#define CONFIGURATION__CONFIG_MODE__ENABLE__WIDTH					1
#define CONFIGURATION__CONFIG_MODE__ENABLE__MASK					0x00000001U
#define CONFIGURATION__CONFIG_MODE__ENABLE__READ(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define CONFIGURATION__CONFIG_MODE__ENABLE__WRITE(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define CONFIGURATION__CONFIG_MODE__ENABLE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000001U) | ((uint32_t)(src) &\
					0x00000001U))
#define CONFIGURATION__CONFIG_MODE__ENABLE__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x00000001U)))
#define CONFIGURATION__CONFIG_MODE__ENABLE__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define CONFIGURATION__CONFIG_MODE__ENABLE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field MANAGER_MODE */
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__SHIFT 					  1
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__WIDTH 					  1
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__MASK				0x00000002U
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__WRITE(src) \
					(((uint32_t)(src)\
					<< 1) & 0x00000002U)
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000002U) | (((uint32_t)(src) <<\
					1) & 0x00000002U))
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 1) & ~0x00000002U)))
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define CONFIGURATION__CONFIG_MODE__MANAGER_MODE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field SNIFFER_MODE */
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__SHIFT 					  2
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__WIDTH 					  1
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__MASK				0x00000004U
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__WRITE(src) \
					(((uint32_t)(src)\
					<< 2) & 0x00000004U)
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000004U) | (((uint32_t)(src) <<\
					2) & 0x00000004U))
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 2) & ~0x00000004U)))
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define CONFIGURATION__CONFIG_MODE__SNIFFER_MODE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field FR_EN */
#define CONFIGURATION__CONFIG_MODE__FR_EN__SHIFT							  3
#define CONFIGURATION__CONFIG_MODE__FR_EN__WIDTH							  1
#define CONFIGURATION__CONFIG_MODE__FR_EN__MASK 					0x00000008U
#define CONFIGURATION__CONFIG_MODE__FR_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define CONFIGURATION__CONFIG_MODE__FR_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 3) & 0x00000008U)
#define CONFIGURATION__CONFIG_MODE__FR_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000008U) | (((uint32_t)(src) <<\
					3) & 0x00000008U))
#define CONFIGURATION__CONFIG_MODE__FR_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 3) & ~0x00000008U)))
#define CONFIGURATION__CONFIG_MODE__FR_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define CONFIGURATION__CONFIG_MODE__FR_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field DEV_EN */
#define CONFIGURATION__CONFIG_MODE__DEV_EN__SHIFT							  4
#define CONFIGURATION__CONFIG_MODE__DEV_EN__WIDTH							  1
#define CONFIGURATION__CONFIG_MODE__DEV_EN__MASK					0x00000010U
#define CONFIGURATION__CONFIG_MODE__DEV_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define CONFIGURATION__CONFIG_MODE__DEV_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 4) & 0x00000010U)
#define CONFIGURATION__CONFIG_MODE__DEV_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000010U) | (((uint32_t)(src) <<\
					4) & 0x00000010U))
#define CONFIGURATION__CONFIG_MODE__DEV_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 4) & ~0x00000010U)))
#define CONFIGURATION__CONFIG_MODE__DEV_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define CONFIGURATION__CONFIG_MODE__DEV_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field GO_ABSENT */
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__SHIFT						  5
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__WIDTH						  1
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__MASK 				0x00000020U
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__WRITE(src) \
					(((uint32_t)(src)\
					<< 5) & 0x00000020U)
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000020U) | (((uint32_t)(src) <<\
					5) & 0x00000020U))
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 5) & ~0x00000020U)))
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define CONFIGURATION__CONFIG_MODE__GO_ABSENT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field RETRY_LMT */
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__SHIFT						  8
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__WIDTH						  4
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__MASK 				0x00000f00U
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000f00U) >> 8)
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__WRITE(src) \
					(((uint32_t)(src)\
					<< 8) & 0x00000f00U)
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000f00U) | (((uint32_t)(src) <<\
					8) & 0x00000f00U))
#define CONFIGURATION__CONFIG_MODE__RETRY_LMT__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 8) & ~0x00000f00U)))

/* macros for field REPORT_AT_EVENT */
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__SHIFT					 12
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__WIDTH					  1
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__MASK			0x00001000U
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__READ(src) \
					(((uint32_t)(src)\
					& 0x00001000U) >> 12)
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__WRITE(src) \
					(((uint32_t)(src)\
					<< 12) & 0x00001000U)
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00001000U) | (((uint32_t)(src) <<\
					12) & 0x00001000U))
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 12) & ~0x00001000U)))
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__SET(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(1) << 12))
#define CONFIGURATION__CONFIG_MODE__REPORT_AT_EVENT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(0) << 12))

/* macros for field CRC_CALC_DISABLE */
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__SHIFT 				 13
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__WIDTH 				  1
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__MASK			0x00002000U
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__READ(src) \
					(((uint32_t)(src)\
					& 0x00002000U) >> 13)
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__WRITE(src) \
					(((uint32_t)(src)\
					<< 13) & 0x00002000U)
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00002000U) | (((uint32_t)(src) <<\
					13) & 0x00002000U))
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 13) & ~0x00002000U)))
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__SET(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(1) << 13))
#define CONFIGURATION__CONFIG_MODE__CRC_CALC_DISABLE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(0) << 13))

/* macros for field LMTD_REPORT */
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__SHIFT						 14
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__WIDTH						  1
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__MASK				0x00004000U
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__READ(src) \
					(((uint32_t)(src)\
					& 0x00004000U) >> 14)
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__WRITE(src) \
					(((uint32_t)(src)\
					<< 14) & 0x00004000U)
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00004000U) | (((uint32_t)(src) <<\
					14) & 0x00004000U))
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 14) & ~0x00004000U)))
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__SET(dst) \
					((dst) = ((dst) &\
					~0x00004000U) | ((uint32_t)(1) << 14))
#define CONFIGURATION__CONFIG_MODE__LMTD_REPORT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00004000U) | ((uint32_t)(0) << 14))

/* macros for field RECONF_TX_DIS */
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__SHIFT					 15
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__WIDTH					  1
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__MASK 			0x00008000U
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__READ(src) \
					(((uint32_t)(src)\
					& 0x00008000U) >> 15)
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__WRITE(src) \
					(((uint32_t)(src)\
					<< 15) & 0x00008000U)
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00008000U) | (((uint32_t)(src) <<\
					15) & 0x00008000U))
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 15) & ~0x00008000U)))
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__SET(dst) \
					((dst) = ((dst) &\
					~0x00008000U) | ((uint32_t)(1) << 15))
#define CONFIGURATION__CONFIG_MODE__RECONF_TX_DIS__CLR(dst) \
					((dst) = ((dst) &\
					~0x00008000U) | ((uint32_t)(0) << 15))
#define CONFIGURATION__CONFIG_MODE__TYPE							   uint32_t
#define CONFIGURATION__CONFIG_MODE__READ							0x0000ff3fU
#define CONFIGURATION__CONFIG_MODE__WRITE							0x0000ff3fU

#endif /* __CONFIGURATION__CONFIG_MODE_MACRO__ */


/* macros for configuration.config_mode */
#define INST_CONFIGURATION__CONFIG_MODE__NUM								  1

/* macros for BlueprintGlobalNameSpace::configuration::config_ea */
#ifndef __CONFIGURATION__CONFIG_EA_MACRO__
#define __CONFIGURATION__CONFIG_EA_MACRO__

/* macros for field PRODUCT_ID */
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__SHIFT 						  0
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__WIDTH 						 16
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__MASK					0x0000ffffU
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__READ(src) \
					((uint32_t)(src)\
					& 0x0000ffffU)
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__WRITE(src) \
					((uint32_t)(src)\
					& 0x0000ffffU)
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x0000ffffU) | ((uint32_t)(src) &\
					0x0000ffffU))
#define CONFIGURATION__CONFIG_EA__PRODUCT_ID__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x0000ffffU)))

/* macros for field INSTANCE_VAL */
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__SHIFT						 16
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__WIDTH						  8
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__MASK				0x00ff0000U
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__READ(src) \
					(((uint32_t)(src)\
					& 0x00ff0000U) >> 16)
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__WRITE(src) \
					(((uint32_t)(src)\
					<< 16) & 0x00ff0000U)
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00ff0000U) | (((uint32_t)(src) <<\
					16) & 0x00ff0000U))
#define CONFIGURATION__CONFIG_EA__INSTANCE_VAL__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 16) & ~0x00ff0000U)))
#define CONFIGURATION__CONFIG_EA__TYPE								   uint32_t
#define CONFIGURATION__CONFIG_EA__READ								0x00ffffffU
#define CONFIGURATION__CONFIG_EA__WRITE 							0x00ffffffU

#endif /* __CONFIGURATION__CONFIG_EA_MACRO__ */


/* macros for configuration.config_ea */
#define INST_CONFIGURATION__CONFIG_EA__NUM									  1

/* macros for BlueprintGlobalNameSpace::configuration::config_pr_tp */
#ifndef __CONFIGURATION__CONFIG_PR_TP_MACRO__
#define __CONFIGURATION__CONFIG_PR_TP_MACRO__

/* macros for field PR_SUPP */
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__SHIFT 						  0
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__WIDTH 						 24
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__MASK					0x00ffffffU
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__READ(src) \
					((uint32_t)(src)\
					& 0x00ffffffU)
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__WRITE(src) \
					((uint32_t)(src)\
					& 0x00ffffffU)
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00ffffffU) | ((uint32_t)(src) &\
					0x00ffffffU))
#define CONFIGURATION__CONFIG_PR_TP__PR_SUPP__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x00ffffffU)))

/* macros for field TP_SUPP */
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__SHIFT 						 24
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__WIDTH 						  3
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__MASK					0x07000000U
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__READ(src) \
					(((uint32_t)(src)\
					& 0x07000000U) >> 24)
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__WRITE(src) \
					(((uint32_t)(src)\
					<< 24) & 0x07000000U)
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x07000000U) | (((uint32_t)(src) <<\
					24) & 0x07000000U))
#define CONFIGURATION__CONFIG_PR_TP__TP_SUPP__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 24) & ~0x07000000U)))
#define CONFIGURATION__CONFIG_PR_TP__TYPE							   uint32_t
#define CONFIGURATION__CONFIG_PR_TP__READ							0x07ffffffU
#define CONFIGURATION__CONFIG_PR_TP__WRITE							0x07ffffffU

#endif /* __CONFIGURATION__CONFIG_PR_TP_MACRO__ */


/* macros for configuration.config_pr_tp */
#define INST_CONFIGURATION__CONFIG_PR_TP__NUM								  1

/* macros for BlueprintGlobalNameSpace::configuration::config_fr */
#ifndef __CONFIGURATION__CONFIG_FR_MACRO__
#define __CONFIGURATION__CONFIG_FR_MACRO__

/* macros for field RF_SUPP */
#define CONFIGURATION__CONFIG_FR__RF_SUPP__SHIFT							  0
#define CONFIGURATION__CONFIG_FR__RF_SUPP__WIDTH							 16
#define CONFIGURATION__CONFIG_FR__RF_SUPP__MASK 					0x0000ffffU
#define CONFIGURATION__CONFIG_FR__RF_SUPP__READ(src) \
					((uint32_t)(src)\
					& 0x0000ffffU)
#define CONFIGURATION__CONFIG_FR__RF_SUPP__WRITE(src) \
					((uint32_t)(src)\
					& 0x0000ffffU)
#define CONFIGURATION__CONFIG_FR__RF_SUPP__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x0000ffffU) | ((uint32_t)(src) &\
					0x0000ffffU))
#define CONFIGURATION__CONFIG_FR__RF_SUPP__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x0000ffffU)))

/* macros for field QUALITY */
#define CONFIGURATION__CONFIG_FR__QUALITY__SHIFT							 16
#define CONFIGURATION__CONFIG_FR__QUALITY__WIDTH							  2
#define CONFIGURATION__CONFIG_FR__QUALITY__MASK 					0x00030000U
#define CONFIGURATION__CONFIG_FR__QUALITY__READ(src) \
					(((uint32_t)(src)\
					& 0x00030000U) >> 16)
#define CONFIGURATION__CONFIG_FR__QUALITY__WRITE(src) \
					(((uint32_t)(src)\
					<< 16) & 0x00030000U)
#define CONFIGURATION__CONFIG_FR__QUALITY__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00030000U) | (((uint32_t)(src) <<\
					16) & 0x00030000U))
#define CONFIGURATION__CONFIG_FR__QUALITY__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 16) & ~0x00030000U)))

/* macros for field PAUSE_AT_RFCHNG */
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__SHIFT					 18
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__WIDTH					  1
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__MASK 			0x00040000U
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__READ(src) \
					(((uint32_t)(src)\
					& 0x00040000U) >> 18)
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__WRITE(src) \
					(((uint32_t)(src)\
					<< 18) & 0x00040000U)
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00040000U) | (((uint32_t)(src) <<\
					18) & 0x00040000U))
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 18) & ~0x00040000U)))
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__SET(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(1) << 18))
#define CONFIGURATION__CONFIG_FR__PAUSE_AT_RFCHNG__CLR(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(0) << 18))
#define CONFIGURATION__CONFIG_FR__TYPE								   uint32_t
#define CONFIGURATION__CONFIG_FR__READ								0x0007ffffU
#define CONFIGURATION__CONFIG_FR__WRITE 							0x0007ffffU

#endif /* __CONFIGURATION__CONFIG_FR_MACRO__ */


/* macros for configuration.config_fr */
#define INST_CONFIGURATION__CONFIG_FR__NUM									  1

/* macros for BlueprintGlobalNameSpace::configuration::config_dport */
#ifndef __CONFIGURATION__CONFIG_DPORT_MACRO__
#define __CONFIGURATION__CONFIG_DPORT_MACRO__

/* macros for field SINK_START_LVL */
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__SHIFT					  0
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__WIDTH					  8
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__MASK			0x000000ffU
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__READ(src) \
					((uint32_t)(src)\
					& 0x000000ffU)
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__WRITE(src) \
					((uint32_t)(src)\
					& 0x000000ffU)
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x000000ffU) | ((uint32_t)(src) &\
					0x000000ffU))
#define CONFIGURATION__CONFIG_DPORT__SINK_START_LVL__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x000000ffU)))

/* macros for field DPORT_CLK_PRESC */
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__SHIFT 				  8
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__WIDTH 				  4
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__MASK			0x00000f00U
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__READ(src) \
					(((uint32_t)(src)\
					& 0x00000f00U) >> 8)
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__WRITE(src) \
					(((uint32_t)(src)\
					<< 8) & 0x00000f00U)
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000f00U) | (((uint32_t)(src) <<\
					8) & 0x00000f00U))
#define CONFIGURATION__CONFIG_DPORT__DPORT_CLK_PRESC__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 8) & ~0x00000f00U)))

/* macros for field REFCLK_SEL */
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__SHIFT						 12
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__WIDTH						  2
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__MASK				0x00003000U
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__READ(src) \
					(((uint32_t)(src)\
					& 0x00003000U) >> 12)
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__WRITE(src) \
					(((uint32_t)(src)\
					<< 12) & 0x00003000U)
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00003000U) | (((uint32_t)(src) <<\
					12) & 0x00003000U))
#define CONFIGURATION__CONFIG_DPORT__REFCLK_SEL__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 12) & ~0x00003000U)))
#define CONFIGURATION__CONFIG_DPORT__TYPE							   uint32_t
#define CONFIGURATION__CONFIG_DPORT__READ							0x00003fffU
#define CONFIGURATION__CONFIG_DPORT__WRITE							0x00003fffU

#endif /* __CONFIGURATION__CONFIG_DPORT_MACRO__ */


/* macros for configuration.config_dport */
#define INST_CONFIGURATION__CONFIG_DPORT__NUM								  1

/* macros for BlueprintGlobalNameSpace::configuration::config_cport */
#ifndef __CONFIGURATION__CONFIG_CPORT_MACRO__
#define __CONFIGURATION__CONFIG_CPORT_MACRO__

/* macros for field CPORT_CLK_DIV */
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__SHIFT					  0
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__WIDTH					  3
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__MASK			0x00000007U
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__READ(src) \
					((uint32_t)(src)\
					& 0x00000007U)
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__WRITE(src) \
					((uint32_t)(src)\
					& 0x00000007U)
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000007U) | ((uint32_t)(src) &\
					0x00000007U))
#define CONFIGURATION__CONFIG_CPORT__CPORT_CLK_DIV__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x00000007U)))
#define CONFIGURATION__CONFIG_CPORT__TYPE							   uint32_t
#define CONFIGURATION__CONFIG_CPORT__READ							0x00000007U
#define CONFIGURATION__CONFIG_CPORT__WRITE							0x00000007U

#endif /* __CONFIGURATION__CONFIG_CPORT_MACRO__ */


/* macros for configuration.config_cport */
#define INST_CONFIGURATION__CONFIG_CPORT__NUM								  1

/* macros for BlueprintGlobalNameSpace::configuration::config_ea2 */
#ifndef __CONFIGURATION__CONFIG_EA2_MACRO__
#define __CONFIGURATION__CONFIG_EA2_MACRO__

/* macros for field DEVICE_ID_1 */
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__SHIFT						  0
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__WIDTH						  8
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__MASK				0x000000ffU
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__READ(src) \
					((uint32_t)(src)\
					& 0x000000ffU)
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__WRITE(src) \
					((uint32_t)(src)\
					& 0x000000ffU)
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x000000ffU) | ((uint32_t)(src) &\
					0x000000ffU))
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_1__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x000000ffU)))

/* macros for field DEVICE_ID_2 */
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__SHIFT						  8
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__WIDTH						  8
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__MASK				0x0000ff00U
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__READ(src) \
					(((uint32_t)(src)\
					& 0x0000ff00U) >> 8)
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__WRITE(src) \
					(((uint32_t)(src)\
					<< 8) & 0x0000ff00U)
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x0000ff00U) | (((uint32_t)(src) <<\
					8) & 0x0000ff00U))
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_2__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 8) & ~0x0000ff00U)))

/* macros for field DEVICE_ID_3 */
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__SHIFT						 16
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__WIDTH						  8
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__MASK				0x00ff0000U
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__READ(src) \
					(((uint32_t)(src)\
					& 0x00ff0000U) >> 16)
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__WRITE(src) \
					(((uint32_t)(src)\
					<< 16) & 0x00ff0000U)
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00ff0000U) | (((uint32_t)(src) <<\
					16) & 0x00ff0000U))
#define CONFIGURATION__CONFIG_EA2__DEVICE_ID_3__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 16) & ~0x00ff0000U)))
#define CONFIGURATION__CONFIG_EA2__TYPE 							   uint32_t
#define CONFIGURATION__CONFIG_EA2__READ 							0x00ffffffU
#define CONFIGURATION__CONFIG_EA2__WRITE							0x00ffffffU

#endif /* __CONFIGURATION__CONFIG_EA2_MACRO__ */


/* macros for configuration.config_ea2 */
#define INST_CONFIGURATION__CONFIG_EA2__NUM 								  1

/* macros for BlueprintGlobalNameSpace::configuration::config_thr */
#ifndef __CONFIGURATION__CONFIG_THR_MACRO__
#define __CONFIGURATION__CONFIG_THR_MACRO__

/* macros for field SRC_THR */
#define CONFIGURATION__CONFIG_THR__SRC_THR__SHIFT							  0
#define CONFIGURATION__CONFIG_THR__SRC_THR__WIDTH							 16
#define CONFIGURATION__CONFIG_THR__SRC_THR__MASK					0x0000ffffU
#define CONFIGURATION__CONFIG_THR__SRC_THR__READ(src) \
					((uint32_t)(src)\
					& 0x0000ffffU)
#define CONFIGURATION__CONFIG_THR__SRC_THR__WRITE(src) \
					((uint32_t)(src)\
					& 0x0000ffffU)
#define CONFIGURATION__CONFIG_THR__SRC_THR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x0000ffffU) | ((uint32_t)(src) &\
					0x0000ffffU))
#define CONFIGURATION__CONFIG_THR__SRC_THR__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x0000ffffU)))

/* macros for field SINK_THR */
#define CONFIGURATION__CONFIG_THR__SINK_THR__SHIFT							 16
#define CONFIGURATION__CONFIG_THR__SINK_THR__WIDTH							 16
#define CONFIGURATION__CONFIG_THR__SINK_THR__MASK					0xffff0000U
#define CONFIGURATION__CONFIG_THR__SINK_THR__READ(src) \
					(((uint32_t)(src)\
					& 0xffff0000U) >> 16)
#define CONFIGURATION__CONFIG_THR__SINK_THR__WRITE(src) \
					(((uint32_t)(src)\
					<< 16) & 0xffff0000U)
#define CONFIGURATION__CONFIG_THR__SINK_THR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0xffff0000U) | (((uint32_t)(src) <<\
					16) & 0xffff0000U))
#define CONFIGURATION__CONFIG_THR__SINK_THR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 16) & ~0xffff0000U)))
#define CONFIGURATION__CONFIG_THR__TYPE 							   uint32_t
#define CONFIGURATION__CONFIG_THR__READ 							0xffffffffU
#define CONFIGURATION__CONFIG_THR__WRITE							0xffffffffU

#endif /* __CONFIGURATION__CONFIG_THR_MACRO__ */


/* macros for configuration.config_thr */
#define INST_CONFIGURATION__CONFIG_THR__NUM 								  1

/* macros for BlueprintGlobalNameSpace::command_status::command */
#ifndef __COMMAND_STATUS__COMMAND_MACRO__
#define __COMMAND_STATUS__COMMAND_MACRO__

/* macros for field TX_PUSH */
#define COMMAND_STATUS__COMMAND__TX_PUSH__SHIFT 							  0
#define COMMAND_STATUS__COMMAND__TX_PUSH__WIDTH 							  1
#define COMMAND_STATUS__COMMAND__TX_PUSH__MASK						0x00000001U
#define COMMAND_STATUS__COMMAND__TX_PUSH__WRITE(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define COMMAND_STATUS__COMMAND__TX_PUSH__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000001U) | ((uint32_t)(src) &\
					0x00000001U))
#define COMMAND_STATUS__COMMAND__TX_PUSH__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x00000001U)))
#define COMMAND_STATUS__COMMAND__TX_PUSH__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define COMMAND_STATUS__COMMAND__TX_PUSH__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field RX_PULL */
#define COMMAND_STATUS__COMMAND__RX_PULL__SHIFT 							  1
#define COMMAND_STATUS__COMMAND__RX_PULL__WIDTH 							  1
#define COMMAND_STATUS__COMMAND__RX_PULL__MASK						0x00000002U
#define COMMAND_STATUS__COMMAND__RX_PULL__WRITE(src) \
					(((uint32_t)(src)\
					<< 1) & 0x00000002U)
#define COMMAND_STATUS__COMMAND__RX_PULL__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000002U) | (((uint32_t)(src) <<\
					1) & 0x00000002U))
#define COMMAND_STATUS__COMMAND__RX_PULL__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 1) & ~0x00000002U)))
#define COMMAND_STATUS__COMMAND__RX_PULL__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define COMMAND_STATUS__COMMAND__RX_PULL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field TX_CLR */
#define COMMAND_STATUS__COMMAND__TX_CLR__SHIFT								  2
#define COMMAND_STATUS__COMMAND__TX_CLR__WIDTH								  1
#define COMMAND_STATUS__COMMAND__TX_CLR__MASK						0x00000004U
#define COMMAND_STATUS__COMMAND__TX_CLR__WRITE(src) \
					(((uint32_t)(src)\
					<< 2) & 0x00000004U)
#define COMMAND_STATUS__COMMAND__TX_CLR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000004U) | (((uint32_t)(src) <<\
					2) & 0x00000004U))
#define COMMAND_STATUS__COMMAND__TX_CLR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 2) & ~0x00000004U)))
#define COMMAND_STATUS__COMMAND__TX_CLR__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define COMMAND_STATUS__COMMAND__TX_CLR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field UNFREEZE */
#define COMMAND_STATUS__COMMAND__UNFREEZE__SHIFT							  3
#define COMMAND_STATUS__COMMAND__UNFREEZE__WIDTH							  1
#define COMMAND_STATUS__COMMAND__UNFREEZE__MASK 					0x00000008U
#define COMMAND_STATUS__COMMAND__UNFREEZE__WRITE(src) \
					(((uint32_t)(src)\
					<< 3) & 0x00000008U)
#define COMMAND_STATUS__COMMAND__UNFREEZE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000008U) | (((uint32_t)(src) <<\
					3) & 0x00000008U))
#define COMMAND_STATUS__COMMAND__UNFREEZE__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 3) & ~0x00000008U)))
#define COMMAND_STATUS__COMMAND__UNFREEZE__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define COMMAND_STATUS__COMMAND__UNFREEZE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field CFG_STROBE */
#define COMMAND_STATUS__COMMAND__CFG_STROBE__SHIFT							  4
#define COMMAND_STATUS__COMMAND__CFG_STROBE__WIDTH							  1
#define COMMAND_STATUS__COMMAND__CFG_STROBE__MASK					0x00000010U
#define COMMAND_STATUS__COMMAND__CFG_STROBE__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define COMMAND_STATUS__COMMAND__CFG_STROBE__WRITE(src) \
					(((uint32_t)(src)\
					<< 4) & 0x00000010U)
#define COMMAND_STATUS__COMMAND__CFG_STROBE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000010U) | (((uint32_t)(src) <<\
					4) & 0x00000010U))
#define COMMAND_STATUS__COMMAND__CFG_STROBE__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 4) & ~0x00000010U)))
#define COMMAND_STATUS__COMMAND__CFG_STROBE__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define COMMAND_STATUS__COMMAND__CFG_STROBE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field CFG_STROBE_CLR */
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__SHIFT						  5
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__WIDTH						  1
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__MASK				0x00000020U
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__WRITE(src) \
					(((uint32_t)(src)\
					<< 5) & 0x00000020U)
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000020U) | (((uint32_t)(src) <<\
					5) & 0x00000020U))
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 5) & ~0x00000020U)))
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define COMMAND_STATUS__COMMAND__CFG_STROBE_CLR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))
#define COMMAND_STATUS__COMMAND__TYPE								   uint32_t
#define COMMAND_STATUS__COMMAND__READ								0x00000010U
#define COMMAND_STATUS__COMMAND__WRITE								0x00000010U

#endif /* __COMMAND_STATUS__COMMAND_MACRO__ */


/* macros for command_status.command */
#define INST_COMMAND_STATUS__COMMAND__NUM									  1

/* macros for BlueprintGlobalNameSpace::command_status::state */
#ifndef __COMMAND_STATUS__STATE_MACRO__
#define __COMMAND_STATUS__STATE_MACRO__

/* macros for field TX_FULL */
#define COMMAND_STATUS__STATE__TX_FULL__SHIFT								  0
#define COMMAND_STATUS__STATE__TX_FULL__WIDTH								  1
#define COMMAND_STATUS__STATE__TX_FULL__MASK						0x00000001U
#define COMMAND_STATUS__STATE__TX_FULL__READ(src) ((uint32_t)(src) & 0x00000001U)
#define COMMAND_STATUS__STATE__TX_FULL__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define COMMAND_STATUS__STATE__TX_FULL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field TX_NOTEMPTY */
#define COMMAND_STATUS__STATE__TX_NOTEMPTY__SHIFT							  1
#define COMMAND_STATUS__STATE__TX_NOTEMPTY__WIDTH							  1
#define COMMAND_STATUS__STATE__TX_NOTEMPTY__MASK					0x00000002U
#define COMMAND_STATUS__STATE__TX_NOTEMPTY__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define COMMAND_STATUS__STATE__TX_NOTEMPTY__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define COMMAND_STATUS__STATE__TX_NOTEMPTY__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field RX_NOTEMPTY */
#define COMMAND_STATUS__STATE__RX_NOTEMPTY__SHIFT							  2
#define COMMAND_STATUS__STATE__RX_NOTEMPTY__WIDTH							  1
#define COMMAND_STATUS__STATE__RX_NOTEMPTY__MASK					0x00000004U
#define COMMAND_STATUS__STATE__RX_NOTEMPTY__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define COMMAND_STATUS__STATE__RX_NOTEMPTY__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define COMMAND_STATUS__STATE__RX_NOTEMPTY__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field TX_PUSH */
#define COMMAND_STATUS__STATE__TX_PUSH__SHIFT								  4
#define COMMAND_STATUS__STATE__TX_PUSH__WIDTH								  1
#define COMMAND_STATUS__STATE__TX_PUSH__MASK						0x00000010U
#define COMMAND_STATUS__STATE__TX_PUSH__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define COMMAND_STATUS__STATE__TX_PUSH__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define COMMAND_STATUS__STATE__TX_PUSH__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field RX_PULL */
#define COMMAND_STATUS__STATE__RX_PULL__SHIFT								  5
#define COMMAND_STATUS__STATE__RX_PULL__WIDTH								  1
#define COMMAND_STATUS__STATE__RX_PULL__MASK						0x00000020U
#define COMMAND_STATUS__STATE__RX_PULL__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define COMMAND_STATUS__STATE__RX_PULL__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define COMMAND_STATUS__STATE__RX_PULL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field F_SYNC */
#define COMMAND_STATUS__STATE__F_SYNC__SHIFT								  8
#define COMMAND_STATUS__STATE__F_SYNC__WIDTH								  1
#define COMMAND_STATUS__STATE__F_SYNC__MASK 						0x00000100U
#define COMMAND_STATUS__STATE__F_SYNC__READ(src) \
					(((uint32_t)(src)\
					& 0x00000100U) >> 8)
#define COMMAND_STATUS__STATE__F_SYNC__SET(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(1) << 8))
#define COMMAND_STATUS__STATE__F_SYNC__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(0) << 8))

/* macros for field SF_SYNC */
#define COMMAND_STATUS__STATE__SF_SYNC__SHIFT								  9
#define COMMAND_STATUS__STATE__SF_SYNC__WIDTH								  1
#define COMMAND_STATUS__STATE__SF_SYNC__MASK						0x00000200U
#define COMMAND_STATUS__STATE__SF_SYNC__READ(src) \
					(((uint32_t)(src)\
					& 0x00000200U) >> 9)
#define COMMAND_STATUS__STATE__SF_SYNC__SET(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(1) << 9))
#define COMMAND_STATUS__STATE__SF_SYNC__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(0) << 9))

/* macros for field M_SYNC */
#define COMMAND_STATUS__STATE__M_SYNC__SHIFT								 10
#define COMMAND_STATUS__STATE__M_SYNC__WIDTH								  1
#define COMMAND_STATUS__STATE__M_SYNC__MASK 						0x00000400U
#define COMMAND_STATUS__STATE__M_SYNC__READ(src) \
					(((uint32_t)(src)\
					& 0x00000400U) >> 10)
#define COMMAND_STATUS__STATE__M_SYNC__SET(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(1) << 10))
#define COMMAND_STATUS__STATE__M_SYNC__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(0) << 10))

/* macros for field SFB_SYNC */
#define COMMAND_STATUS__STATE__SFB_SYNC__SHIFT								 11
#define COMMAND_STATUS__STATE__SFB_SYNC__WIDTH								  1
#define COMMAND_STATUS__STATE__SFB_SYNC__MASK						0x00000800U
#define COMMAND_STATUS__STATE__SFB_SYNC__READ(src) \
					(((uint32_t)(src)\
					& 0x00000800U) >> 11)
#define COMMAND_STATUS__STATE__SFB_SYNC__SET(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(1) << 11))
#define COMMAND_STATUS__STATE__SFB_SYNC__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(0) << 11))

/* macros for field PH_SYNC */
#define COMMAND_STATUS__STATE__PH_SYNC__SHIFT								 12
#define COMMAND_STATUS__STATE__PH_SYNC__WIDTH								  1
#define COMMAND_STATUS__STATE__PH_SYNC__MASK						0x00001000U
#define COMMAND_STATUS__STATE__PH_SYNC__READ(src) \
					(((uint32_t)(src)\
					& 0x00001000U) >> 12)
#define COMMAND_STATUS__STATE__PH_SYNC__SET(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(1) << 12))
#define COMMAND_STATUS__STATE__PH_SYNC__CLR(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(0) << 12))

/* macros for field DETACHED */
#define COMMAND_STATUS__STATE__DETACHED__SHIFT								 15
#define COMMAND_STATUS__STATE__DETACHED__WIDTH								  1
#define COMMAND_STATUS__STATE__DETACHED__MASK						0x00008000U
#define COMMAND_STATUS__STATE__DETACHED__READ(src) \
					(((uint32_t)(src)\
					& 0x00008000U) >> 15)
#define COMMAND_STATUS__STATE__DETACHED__SET(dst) \
					((dst) = ((dst) &\
					~0x00008000U) | ((uint32_t)(1) << 15))
#define COMMAND_STATUS__STATE__DETACHED__CLR(dst) \
					((dst) = ((dst) &\
					~0x00008000U) | ((uint32_t)(0) << 15))

/* macros for field SUBFRAME_MODE */
#define COMMAND_STATUS__STATE__SUBFRAME_MODE__SHIFT 						 16
#define COMMAND_STATUS__STATE__SUBFRAME_MODE__WIDTH 						  5
#define COMMAND_STATUS__STATE__SUBFRAME_MODE__MASK					0x001f0000U
#define COMMAND_STATUS__STATE__SUBFRAME_MODE__READ(src) \
					(((uint32_t)(src)\
					& 0x001f0000U) >> 16)

/* macros for field CLOCK_GEAR */
#define COMMAND_STATUS__STATE__CLOCK_GEAR__SHIFT							 24
#define COMMAND_STATUS__STATE__CLOCK_GEAR__WIDTH							  4
#define COMMAND_STATUS__STATE__CLOCK_GEAR__MASK 					0x0f000000U
#define COMMAND_STATUS__STATE__CLOCK_GEAR__READ(src) \
					(((uint32_t)(src)\
					& 0x0f000000U) >> 24)

/* macros for field ROOT_FR */
#define COMMAND_STATUS__STATE__ROOT_FR__SHIFT								 28
#define COMMAND_STATUS__STATE__ROOT_FR__WIDTH								  4
#define COMMAND_STATUS__STATE__ROOT_FR__MASK						0xf0000000U
#define COMMAND_STATUS__STATE__ROOT_FR__READ(src) \
					(((uint32_t)(src)\
					& 0xf0000000U) >> 28)
#define COMMAND_STATUS__STATE__TYPE 								   uint32_t
#define COMMAND_STATUS__STATE__READ 								0xff1f9f37U

#endif /* __COMMAND_STATUS__STATE_MACRO__ */


/* macros for command_status.state */
#define INST_COMMAND_STATUS__STATE__NUM 									  1

/* macros for BlueprintGlobalNameSpace::command_status::ie_state */
#ifndef __COMMAND_STATUS__IE_STATE_MACRO__
#define __COMMAND_STATUS__IE_STATE_MACRO__

/* macros for field EX_ERROR_IF */
#define COMMAND_STATUS__IE_STATE__EX_ERROR_IF__SHIFT						  0
#define COMMAND_STATUS__IE_STATE__EX_ERROR_IF__WIDTH						  1
#define COMMAND_STATUS__IE_STATE__EX_ERROR_IF__MASK 				0x00000001U
#define COMMAND_STATUS__IE_STATE__EX_ERROR_IF__READ(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define COMMAND_STATUS__IE_STATE__EX_ERROR_IF__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define COMMAND_STATUS__IE_STATE__EX_ERROR_IF__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field UNSPRTD_MSG_IF */
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_IF__SHIFT 					  2
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_IF__WIDTH 					  1
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_IF__MASK				0x00000004U
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_IF__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_IF__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_IF__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field MC_TX_COL */
#define COMMAND_STATUS__IE_STATE__MC_TX_COL__SHIFT							  3
#define COMMAND_STATUS__IE_STATE__MC_TX_COL__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__MC_TX_COL__MASK					0x00000008U
#define COMMAND_STATUS__IE_STATE__MC_TX_COL__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define COMMAND_STATUS__IE_STATE__MC_TX_COL__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define COMMAND_STATUS__IE_STATE__MC_TX_COL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field LOST_FS */
#define COMMAND_STATUS__IE_STATE__LOST_FS__SHIFT							  4
#define COMMAND_STATUS__IE_STATE__LOST_FS__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__LOST_FS__MASK 					0x00000010U
#define COMMAND_STATUS__IE_STATE__LOST_FS__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define COMMAND_STATUS__IE_STATE__LOST_FS__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define COMMAND_STATUS__IE_STATE__LOST_FS__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field LOST_SFS */
#define COMMAND_STATUS__IE_STATE__LOST_SFS__SHIFT							  5
#define COMMAND_STATUS__IE_STATE__LOST_SFS__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__LOST_SFS__MASK					0x00000020U
#define COMMAND_STATUS__IE_STATE__LOST_SFS__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define COMMAND_STATUS__IE_STATE__LOST_SFS__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define COMMAND_STATUS__IE_STATE__LOST_SFS__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field LOST_MS */
#define COMMAND_STATUS__IE_STATE__LOST_MS__SHIFT							  6
#define COMMAND_STATUS__IE_STATE__LOST_MS__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__LOST_MS__MASK 					0x00000040U
#define COMMAND_STATUS__IE_STATE__LOST_MS__READ(src) \
					(((uint32_t)(src)\
					& 0x00000040U) >> 6)
#define COMMAND_STATUS__IE_STATE__LOST_MS__SET(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(1) << 6))
#define COMMAND_STATUS__IE_STATE__LOST_MS__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(0) << 6))

/* macros for field DATA_SLOT_OVERLAP */
#define COMMAND_STATUS__IE_STATE__DATA_SLOT_OVERLAP__SHIFT					  7
#define COMMAND_STATUS__IE_STATE__DATA_SLOT_OVERLAP__WIDTH					  1
#define COMMAND_STATUS__IE_STATE__DATA_SLOT_OVERLAP__MASK			0x00000080U
#define COMMAND_STATUS__IE_STATE__DATA_SLOT_OVERLAP__READ(src) \
					(((uint32_t)(src)\
					& 0x00000080U) >> 7)
#define COMMAND_STATUS__IE_STATE__DATA_SLOT_OVERLAP__SET(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(1) << 7))
#define COMMAND_STATUS__IE_STATE__DATA_SLOT_OVERLAP__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(0) << 7))

/* macros for field EX_ERROR_FR */
#define COMMAND_STATUS__IE_STATE__EX_ERROR_FR__SHIFT						  8
#define COMMAND_STATUS__IE_STATE__EX_ERROR_FR__WIDTH						  1
#define COMMAND_STATUS__IE_STATE__EX_ERROR_FR__MASK 				0x00000100U
#define COMMAND_STATUS__IE_STATE__EX_ERROR_FR__READ(src) \
					(((uint32_t)(src)\
					& 0x00000100U) >> 8)
#define COMMAND_STATUS__IE_STATE__EX_ERROR_FR__SET(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(1) << 8))
#define COMMAND_STATUS__IE_STATE__EX_ERROR_FR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(0) << 8))

/* macros for field RCFG_OBJECTION */
#define COMMAND_STATUS__IE_STATE__RCFG_OBJECTION__SHIFT 					  9
#define COMMAND_STATUS__IE_STATE__RCFG_OBJECTION__WIDTH 					  1
#define COMMAND_STATUS__IE_STATE__RCFG_OBJECTION__MASK				0x00000200U
#define COMMAND_STATUS__IE_STATE__RCFG_OBJECTION__READ(src) \
					(((uint32_t)(src)\
					& 0x00000200U) >> 9)
#define COMMAND_STATUS__IE_STATE__RCFG_OBJECTION__SET(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(1) << 9))
#define COMMAND_STATUS__IE_STATE__RCFG_OBJECTION__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(0) << 9))

/* macros for field UNSPRTD_MSG_FR */
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_FR__SHIFT 					 10
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_FR__WIDTH 					  1
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_FR__MASK				0x00000400U
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_FR__READ(src) \
					(((uint32_t)(src)\
					& 0x00000400U) >> 10)
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_FR__SET(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(1) << 10))
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_FR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(0) << 10))

/* macros for field ACTIVE_FRAMER */
#define COMMAND_STATUS__IE_STATE__ACTIVE_FRAMER__SHIFT						 11
#define COMMAND_STATUS__IE_STATE__ACTIVE_FRAMER__WIDTH						  1
#define COMMAND_STATUS__IE_STATE__ACTIVE_FRAMER__MASK				0x00000800U
#define COMMAND_STATUS__IE_STATE__ACTIVE_FRAMER__READ(src) \
					(((uint32_t)(src)\
					& 0x00000800U) >> 11)
#define COMMAND_STATUS__IE_STATE__ACTIVE_FRAMER__SET(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(1) << 11))
#define COMMAND_STATUS__IE_STATE__ACTIVE_FRAMER__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(0) << 11))

/* macros for field FS_TX_COL */
#define COMMAND_STATUS__IE_STATE__FS_TX_COL__SHIFT							 12
#define COMMAND_STATUS__IE_STATE__FS_TX_COL__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__FS_TX_COL__MASK					0x00001000U
#define COMMAND_STATUS__IE_STATE__FS_TX_COL__READ(src) \
					(((uint32_t)(src)\
					& 0x00001000U) >> 12)
#define COMMAND_STATUS__IE_STATE__FS_TX_COL__SET(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(1) << 12))
#define COMMAND_STATUS__IE_STATE__FS_TX_COL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(0) << 12))

/* macros for field FI_TX_COL */
#define COMMAND_STATUS__IE_STATE__FI_TX_COL__SHIFT							 13
#define COMMAND_STATUS__IE_STATE__FI_TX_COL__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__FI_TX_COL__MASK					0x00002000U
#define COMMAND_STATUS__IE_STATE__FI_TX_COL__READ(src) \
					(((uint32_t)(src)\
					& 0x00002000U) >> 13)
#define COMMAND_STATUS__IE_STATE__FI_TX_COL__SET(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(1) << 13))
#define COMMAND_STATUS__IE_STATE__FI_TX_COL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(0) << 13))

/* macros for field GC_TX_COL */
#define COMMAND_STATUS__IE_STATE__GC_TX_COL__SHIFT							 14
#define COMMAND_STATUS__IE_STATE__GC_TX_COL__WIDTH							  1
#define COMMAND_STATUS__IE_STATE__GC_TX_COL__MASK					0x00004000U
#define COMMAND_STATUS__IE_STATE__GC_TX_COL__READ(src) \
					(((uint32_t)(src)\
					& 0x00004000U) >> 14)
#define COMMAND_STATUS__IE_STATE__GC_TX_COL__SET(dst) \
					((dst) = ((dst) &\
					~0x00004000U) | ((uint32_t)(1) << 14))
#define COMMAND_STATUS__IE_STATE__GC_TX_COL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00004000U) | ((uint32_t)(0) << 14))

/* macros for field EX_ERROR_DEV */
#define COMMAND_STATUS__IE_STATE__EX_ERROR_DEV__SHIFT						 16
#define COMMAND_STATUS__IE_STATE__EX_ERROR_DEV__WIDTH						  1
#define COMMAND_STATUS__IE_STATE__EX_ERROR_DEV__MASK				0x00010000U
#define COMMAND_STATUS__IE_STATE__EX_ERROR_DEV__READ(src) \
					(((uint32_t)(src)\
					& 0x00010000U) >> 16)
#define COMMAND_STATUS__IE_STATE__EX_ERROR_DEV__SET(dst) \
					((dst) = ((dst) &\
					~0x00010000U) | ((uint32_t)(1) << 16))
#define COMMAND_STATUS__IE_STATE__EX_ERROR_DEV__CLR(dst) \
					((dst) = ((dst) &\
					~0x00010000U) | ((uint32_t)(0) << 16))

/* macros for field DATA_TX_COL */
#define COMMAND_STATUS__IE_STATE__DATA_TX_COL__SHIFT						 17
#define COMMAND_STATUS__IE_STATE__DATA_TX_COL__WIDTH						  1
#define COMMAND_STATUS__IE_STATE__DATA_TX_COL__MASK 				0x00020000U
#define COMMAND_STATUS__IE_STATE__DATA_TX_COL__READ(src) \
					(((uint32_t)(src)\
					& 0x00020000U) >> 17)
#define COMMAND_STATUS__IE_STATE__DATA_TX_COL__SET(dst) \
					((dst) = ((dst) &\
					~0x00020000U) | ((uint32_t)(1) << 17))
#define COMMAND_STATUS__IE_STATE__DATA_TX_COL__CLR(dst) \
					((dst) = ((dst) &\
					~0x00020000U) | ((uint32_t)(0) << 17))

/* macros for field UNSPRTD_MSG_DEV */
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_DEV__SHIFT					 18
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_DEV__WIDTH					  1
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_DEV__MASK 			0x00040000U
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_DEV__READ(src) \
					(((uint32_t)(src)\
					& 0x00040000U) >> 18)
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_DEV__SET(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(1) << 18))
#define COMMAND_STATUS__IE_STATE__UNSPRTD_MSG_DEV__CLR(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(0) << 18))
#define COMMAND_STATUS__IE_STATE__TYPE								   uint32_t
#define COMMAND_STATUS__IE_STATE__READ								0x00077ffdU

#endif /* __COMMAND_STATUS__IE_STATE_MACRO__ */


/* macros for command_status.ie_state */
#define INST_COMMAND_STATUS__IE_STATE__NUM									  1

/* macros for BlueprintGlobalNameSpace::command_status::mch_usage */
#ifndef __COMMAND_STATUS__MCH_USAGE_MACRO__
#define __COMMAND_STATUS__MCH_USAGE_MACRO__

/* macros for field mch_usage */
#define COMMAND_STATUS__MCH_USAGE__MCH_USAGE__SHIFT 						  0
#define COMMAND_STATUS__MCH_USAGE__MCH_USAGE__WIDTH 						 11
#define COMMAND_STATUS__MCH_USAGE__MCH_USAGE__MASK					0x000007ffU
#define COMMAND_STATUS__MCH_USAGE__MCH_USAGE__READ(src) \
					((uint32_t)(src)\
					& 0x000007ffU)

/* macros for field MCH_CAPACITY */
#define COMMAND_STATUS__MCH_USAGE__MCH_CAPACITY__SHIFT						 16
#define COMMAND_STATUS__MCH_USAGE__MCH_CAPACITY__WIDTH						 11
#define COMMAND_STATUS__MCH_USAGE__MCH_CAPACITY__MASK				0x07ff0000U
#define COMMAND_STATUS__MCH_USAGE__MCH_CAPACITY__READ(src) \
					(((uint32_t)(src)\
					& 0x07ff0000U) >> 16)

/* macros for field MCH_LAPSE */
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__SHIFT 						 27
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__WIDTH 						  5
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__MASK					0xf8000000U
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__READ(src) \
					(((uint32_t)(src)\
					& 0xf8000000U) >> 27)
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__WRITE(src) \
					(((uint32_t)(src)\
					<< 27) & 0xf8000000U)
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0xf8000000U) | (((uint32_t)(src) <<\
					27) & 0xf8000000U))
#define COMMAND_STATUS__MCH_USAGE__MCH_LAPSE__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 27) & ~0xf8000000U)))
#define COMMAND_STATUS__MCH_USAGE__TYPE 							   uint32_t
#define COMMAND_STATUS__MCH_USAGE__READ 							0xffff07ffU
#define COMMAND_STATUS__MCH_USAGE__WRITE							0xffff07ffU

#endif /* __COMMAND_STATUS__MCH_USAGE_MACRO__ */


/* macros for command_status.mch_usage */
#define INST_COMMAND_STATUS__MCH_USAGE__NUM 								  1

/* macros for BlueprintGlobalNameSpace::interrupts::int_en */
#ifndef __INTERRUPTS__INT_EN_MACRO__
#define __INTERRUPTS__INT_EN_MACRO__

/* macros for field int_en */
#define INTERRUPTS__INT_EN__INT_EN__SHIFT									  0
#define INTERRUPTS__INT_EN__INT_EN__WIDTH									  1
#define INTERRUPTS__INT_EN__INT_EN__MASK							0x00000001U
#define INTERRUPTS__INT_EN__INT_EN__READ(src)	  ((uint32_t)(src) & 0x00000001U)
#define INTERRUPTS__INT_EN__INT_EN__WRITE(src)	((uint32_t)(src) & 0x00000001U)
#define INTERRUPTS__INT_EN__INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000001U) | ((uint32_t)(src) &\
					0x00000001U))
#define INTERRUPTS__INT_EN__INT_EN__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x00000001U)))
#define INTERRUPTS__INT_EN__INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define INTERRUPTS__INT_EN__INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field RX_INT_EN */
#define INTERRUPTS__INT_EN__RX_INT_EN__SHIFT								  1
#define INTERRUPTS__INT_EN__RX_INT_EN__WIDTH								  1
#define INTERRUPTS__INT_EN__RX_INT_EN__MASK 						0x00000002U
#define INTERRUPTS__INT_EN__RX_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define INTERRUPTS__INT_EN__RX_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 1) & 0x00000002U)
#define INTERRUPTS__INT_EN__RX_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000002U) | (((uint32_t)(src) <<\
					1) & 0x00000002U))
#define INTERRUPTS__INT_EN__RX_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 1) & ~0x00000002U)))
#define INTERRUPTS__INT_EN__RX_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define INTERRUPTS__INT_EN__RX_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field TX_INT_EN */
#define INTERRUPTS__INT_EN__TX_INT_EN__SHIFT								  2
#define INTERRUPTS__INT_EN__TX_INT_EN__WIDTH								  1
#define INTERRUPTS__INT_EN__TX_INT_EN__MASK 						0x00000004U
#define INTERRUPTS__INT_EN__TX_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define INTERRUPTS__INT_EN__TX_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 2) & 0x00000004U)
#define INTERRUPTS__INT_EN__TX_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000004U) | (((uint32_t)(src) <<\
					2) & 0x00000004U))
#define INTERRUPTS__INT_EN__TX_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 2) & ~0x00000004U)))
#define INTERRUPTS__INT_EN__TX_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define INTERRUPTS__INT_EN__TX_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field TX_ERR_EN */
#define INTERRUPTS__INT_EN__TX_ERR_EN__SHIFT								  3
#define INTERRUPTS__INT_EN__TX_ERR_EN__WIDTH								  1
#define INTERRUPTS__INT_EN__TX_ERR_EN__MASK 						0x00000008U
#define INTERRUPTS__INT_EN__TX_ERR_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define INTERRUPTS__INT_EN__TX_ERR_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 3) & 0x00000008U)
#define INTERRUPTS__INT_EN__TX_ERR_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000008U) | (((uint32_t)(src) <<\
					3) & 0x00000008U))
#define INTERRUPTS__INT_EN__TX_ERR_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 3) & ~0x00000008U)))
#define INTERRUPTS__INT_EN__TX_ERR_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define INTERRUPTS__INT_EN__TX_ERR_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field SYNC_LOST_EN */
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__SHIFT 							  4
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__WIDTH 							  1
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__MASK						0x00000010U
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 4) & 0x00000010U)
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000010U) | (((uint32_t)(src) <<\
					4) & 0x00000010U))
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 4) & ~0x00000010U)))
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define INTERRUPTS__INT_EN__SYNC_LOST_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field RCFG_INT_EN */
#define INTERRUPTS__INT_EN__RCFG_INT_EN__SHIFT								  5
#define INTERRUPTS__INT_EN__RCFG_INT_EN__WIDTH								  1
#define INTERRUPTS__INT_EN__RCFG_INT_EN__MASK						0x00000020U
#define INTERRUPTS__INT_EN__RCFG_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define INTERRUPTS__INT_EN__RCFG_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 5) & 0x00000020U)
#define INTERRUPTS__INT_EN__RCFG_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000020U) | (((uint32_t)(src) <<\
					5) & 0x00000020U))
#define INTERRUPTS__INT_EN__RCFG_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 5) & ~0x00000020U)))
#define INTERRUPTS__INT_EN__RCFG_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define INTERRUPTS__INT_EN__RCFG_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field MCH_INT_EN */
#define INTERRUPTS__INT_EN__MCH_INT_EN__SHIFT								  6
#define INTERRUPTS__INT_EN__MCH_INT_EN__WIDTH								  1
#define INTERRUPTS__INT_EN__MCH_INT_EN__MASK						0x00000040U
#define INTERRUPTS__INT_EN__MCH_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000040U) >> 6)
#define INTERRUPTS__INT_EN__MCH_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 6) & 0x00000040U)
#define INTERRUPTS__INT_EN__MCH_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000040U) | (((uint32_t)(src) <<\
					6) & 0x00000040U))
#define INTERRUPTS__INT_EN__MCH_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 6) & ~0x00000040U)))
#define INTERRUPTS__INT_EN__MCH_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(1) << 6))
#define INTERRUPTS__INT_EN__MCH_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(0) << 6))
#define INTERRUPTS__INT_EN__TYPE									   uint32_t
#define INTERRUPTS__INT_EN__READ									0x0000007fU
#define INTERRUPTS__INT_EN__WRITE									0x0000007fU

#endif /* __INTERRUPTS__INT_EN_MACRO__ */


/* macros for interrupts.int_en */
#define INST_INTERRUPTS__INT_EN__NUM										  1

/* macros for BlueprintGlobalNameSpace::interrupts::interrupt */
#ifndef __INTERRUPTS__INT_MACRO__
#define __INTERRUPTS__INT_MACRO__

/* macros for field RX_INT */
#define INTERRUPTS__INT__RX_INT__SHIFT										  1
#define INTERRUPTS__INT__RX_INT__WIDTH										  1
#define INTERRUPTS__INT__RX_INT__MASK								0x00000002U
#define INTERRUPTS__INT__RX_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define INTERRUPTS__INT__RX_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define INTERRUPTS__INT__RX_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field TX_INT */
#define INTERRUPTS__INT__TX_INT__SHIFT										  2
#define INTERRUPTS__INT__TX_INT__WIDTH										  1
#define INTERRUPTS__INT__TX_INT__MASK								0x00000004U
#define INTERRUPTS__INT__TX_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define INTERRUPTS__INT__TX_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define INTERRUPTS__INT__TX_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field TX_ERR */
#define INTERRUPTS__INT__TX_ERR__SHIFT										  3
#define INTERRUPTS__INT__TX_ERR__WIDTH										  1
#define INTERRUPTS__INT__TX_ERR__MASK								0x00000008U
#define INTERRUPTS__INT__TX_ERR__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define INTERRUPTS__INT__TX_ERR__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define INTERRUPTS__INT__TX_ERR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field SYNC_LOST */
#define INTERRUPTS__INT__SYNC_LOST__SHIFT									  4
#define INTERRUPTS__INT__SYNC_LOST__WIDTH									  1
#define INTERRUPTS__INT__SYNC_LOST__MASK							0x00000010U
#define INTERRUPTS__INT__SYNC_LOST__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define INTERRUPTS__INT__SYNC_LOST__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define INTERRUPTS__INT__SYNC_LOST__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field RCFG_INT */
#define INTERRUPTS__INT__RCFG_INT__SHIFT									  5
#define INTERRUPTS__INT__RCFG_INT__WIDTH									  1
#define INTERRUPTS__INT__RCFG_INT__MASK 							0x00000020U
#define INTERRUPTS__INT__RCFG_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define INTERRUPTS__INT__RCFG_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define INTERRUPTS__INT__RCFG_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field MCH_INT */
#define INTERRUPTS__INT__MCH_INT__SHIFT 									  6
#define INTERRUPTS__INT__MCH_INT__WIDTH 									  1
#define INTERRUPTS__INT__MCH_INT__MASK								0x00000040U
#define INTERRUPTS__INT__MCH_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000040U) >> 6)
#define INTERRUPTS__INT__MCH_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(1) << 6))
#define INTERRUPTS__INT__MCH_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(0) << 6))

/* macros for field PORT_INT */
#define INTERRUPTS__INT__PORT_INT__SHIFT									  7
#define INTERRUPTS__INT__PORT_INT__WIDTH									  1
#define INTERRUPTS__INT__PORT_INT__MASK 							0x00000080U
#define INTERRUPTS__INT__PORT_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000080U) >> 7)
#define INTERRUPTS__INT__PORT_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(1) << 7))
#define INTERRUPTS__INT__PORT_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(0) << 7))
#define INTERRUPTS__INT__TYPE										   uint32_t
#define INTERRUPTS__INT__READ										0x000000feU
#define INTERRUPTS__INT__RCLR										0x000000feU

#endif /* __INTERRUPTS__INT_MACRO__ */


/* macros for interrupts.interrupt */
#define INST_INTERRUPTS__INT__NUM											  1

/* macros for BlueprintGlobalNameSpace::message_fifos::mc_fifo */
#ifndef __MESSAGE_FIFOS__MC_FIFO_MACRO__
#define __MESSAGE_FIFOS__MC_FIFO_MACRO__

/* macros for field FIFO_DATA */
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__SHIFT							  0
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__WIDTH							 32
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__MASK 					0xffffffffU
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__READ(src) \
					((uint32_t)(src)\
					& 0xffffffffU)
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__WRITE(src) \
					((uint32_t)(src)\
					& 0xffffffffU)
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0xffffffffU) | ((uint32_t)(src) &\
					0xffffffffU))
#define MESSAGE_FIFOS__MC_FIFO__FIFO_DATA__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0xffffffffU)))
#define MESSAGE_FIFOS__MC_FIFO__TYPE								   uint32_t
#define MESSAGE_FIFOS__MC_FIFO__READ								0xffffffffU
#define MESSAGE_FIFOS__MC_FIFO__WRITE								0xffffffffU

#endif /* __MESSAGE_FIFOS__MC_FIFO_MACRO__ */


/* macros for message_fifos.mc_fifo */
#define INST_MESSAGE_FIFOS__MC_FIFO__NUM									 16

/* macros for BlueprintGlobalNameSpace::port_interrupts::p_int_en */
#ifndef __PORT_INTERRUPTS__P_INT_EN_MACRO__
#define __PORT_INTERRUPTS__P_INT_EN_MACRO__

/* macros for field P0_ACT_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__SHIFT 					  0
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__MASK				0x00000001U
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__READ(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__WRITE(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000001U) | ((uint32_t)(src) &\
					0x00000001U))
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0x00000001U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define PORT_INTERRUPTS__P_INT_EN__P0_ACT_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field P0_CON_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__SHIFT 					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__MASK				0x00000002U
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 1) & 0x00000002U)
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000002U) | (((uint32_t)(src) <<\
					1) & 0x00000002U))
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 1) & ~0x00000002U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define PORT_INTERRUPTS__P_INT_EN__P0_CON_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field P0_CHAN_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__SHIFT					  2
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__WIDTH					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__MASK 			0x00000004U
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 2) & 0x00000004U)
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000004U) | (((uint32_t)(src) <<\
					2) & 0x00000004U))
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 2) & ~0x00000004U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define PORT_INTERRUPTS__P_INT_EN__P0_CHAN_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field P0_DMA_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__SHIFT 					  3
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__MASK				0x00000008U
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 3) & 0x00000008U)
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000008U) | (((uint32_t)(src) <<\
					3) & 0x00000008U))
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 3) & ~0x00000008U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define PORT_INTERRUPTS__P_INT_EN__P0_DMA_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field P0_OVF_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__SHIFT 					  4
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__MASK				0x00000010U
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 4) & 0x00000010U)
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000010U) | (((uint32_t)(src) <<\
					4) & 0x00000010U))
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 4) & ~0x00000010U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define PORT_INTERRUPTS__P_INT_EN__P0_OVF_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field P0_UND_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__SHIFT 					  5
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__MASK				0x00000020U
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 5) & 0x00000020U)
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000020U) | (((uint32_t)(src) <<\
					5) & 0x00000020U))
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 5) & ~0x00000020U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define PORT_INTERRUPTS__P_INT_EN__P0_UND_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field P0_FIFO_CLR */
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__SHIFT						  6
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__MASK				0x00000040U
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__READ(src) \
					(((uint32_t)(src)\
					& 0x00000040U) >> 6)
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__WRITE(src) \
					(((uint32_t)(src)\
					<< 6) & 0x00000040U)
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000040U) | (((uint32_t)(src) <<\
					6) & 0x00000040U))
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 6) & ~0x00000040U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__SET(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(1) << 6))
#define PORT_INTERRUPTS__P_INT_EN__P0_FIFO_CLR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(0) << 6))

/* macros for field P0_PR_GEN_EN */
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__SHIFT						  7
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__MASK				0x00000080U
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000080U) >> 7)
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 7) & 0x00000080U)
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000080U) | (((uint32_t)(src) <<\
					7) & 0x00000080U))
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 7) & ~0x00000080U)))
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(1) << 7))
#define PORT_INTERRUPTS__P_INT_EN__P0_PR_GEN_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(0) << 7))

/* macros for field P1_ACT_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__SHIFT 					  8
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__MASK				0x00000100U
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000100U) >> 8)
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 8) & 0x00000100U)
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000100U) | (((uint32_t)(src) <<\
					8) & 0x00000100U))
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 8) & ~0x00000100U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(1) << 8))
#define PORT_INTERRUPTS__P_INT_EN__P1_ACT_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(0) << 8))

/* macros for field P1_CON_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__SHIFT 					  9
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__MASK				0x00000200U
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000200U) >> 9)
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 9) & 0x00000200U)
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000200U) | (((uint32_t)(src) <<\
					9) & 0x00000200U))
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 9) & ~0x00000200U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(1) << 9))
#define PORT_INTERRUPTS__P_INT_EN__P1_CON_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(0) << 9))

/* macros for field P1_CHAN_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__SHIFT					 10
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__WIDTH					  1
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__MASK 			0x00000400U
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000400U) >> 10)
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 10) & 0x00000400U)
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000400U) | (((uint32_t)(src) <<\
					10) & 0x00000400U))
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 10) & ~0x00000400U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(1) << 10))
#define PORT_INTERRUPTS__P_INT_EN__P1_CHAN_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(0) << 10))

/* macros for field P1_DMA_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__SHIFT 					 11
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__MASK				0x00000800U
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00000800U) >> 11)
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 11) & 0x00000800U)
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00000800U) | (((uint32_t)(src) <<\
					11) & 0x00000800U))
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 11) & ~0x00000800U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(1) << 11))
#define PORT_INTERRUPTS__P_INT_EN__P1_DMA_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(0) << 11))

/* macros for field P1_OVF_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__SHIFT 					 12
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__MASK				0x00001000U
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00001000U) >> 12)
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 12) & 0x00001000U)
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00001000U) | (((uint32_t)(src) <<\
					12) & 0x00001000U))
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 12) & ~0x00001000U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(1) << 12))
#define PORT_INTERRUPTS__P_INT_EN__P1_OVF_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(0) << 12))

/* macros for field P1_UND_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__SHIFT 					 13
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__MASK				0x00002000U
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00002000U) >> 13)
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 13) & 0x00002000U)
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00002000U) | (((uint32_t)(src) <<\
					13) & 0x00002000U))
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 13) & ~0x00002000U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(1) << 13))
#define PORT_INTERRUPTS__P_INT_EN__P1_UND_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(0) << 13))

/* macros for field P1_FIFO_CLR */
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__SHIFT						 14
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__MASK				0x00004000U
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__READ(src) \
					(((uint32_t)(src)\
					& 0x00004000U) >> 14)
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__WRITE(src) \
					(((uint32_t)(src)\
					<< 14) & 0x00004000U)
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00004000U) | (((uint32_t)(src) <<\
					14) & 0x00004000U))
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 14) & ~0x00004000U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__SET(dst) \
					((dst) = ((dst) &\
					~0x00004000U) | ((uint32_t)(1) << 14))
#define PORT_INTERRUPTS__P_INT_EN__P1_FIFO_CLR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00004000U) | ((uint32_t)(0) << 14))

/* macros for field P1_PR_GEN_EN */
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__SHIFT						 15
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__MASK				0x00008000U
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00008000U) >> 15)
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 15) & 0x00008000U)
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00008000U) | (((uint32_t)(src) <<\
					15) & 0x00008000U))
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 15) & ~0x00008000U)))
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00008000U) | ((uint32_t)(1) << 15))
#define PORT_INTERRUPTS__P_INT_EN__P1_PR_GEN_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00008000U) | ((uint32_t)(0) << 15))

/* macros for field P2_ACT_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__SHIFT 					 16
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__MASK				0x00010000U
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00010000U) >> 16)
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 16) & 0x00010000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00010000U) | (((uint32_t)(src) <<\
					16) & 0x00010000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 16) & ~0x00010000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00010000U) | ((uint32_t)(1) << 16))
#define PORT_INTERRUPTS__P_INT_EN__P2_ACT_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00010000U) | ((uint32_t)(0) << 16))

/* macros for field P2_CON_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__SHIFT 					 17
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__MASK				0x00020000U
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00020000U) >> 17)
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 17) & 0x00020000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00020000U) | (((uint32_t)(src) <<\
					17) & 0x00020000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 17) & ~0x00020000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00020000U) | ((uint32_t)(1) << 17))
#define PORT_INTERRUPTS__P_INT_EN__P2_CON_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00020000U) | ((uint32_t)(0) << 17))

/* macros for field P2_CHAN_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__SHIFT					 18
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__WIDTH					  1
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__MASK 			0x00040000U
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00040000U) >> 18)
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 18) & 0x00040000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00040000U) | (((uint32_t)(src) <<\
					18) & 0x00040000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 18) & ~0x00040000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(1) << 18))
#define PORT_INTERRUPTS__P_INT_EN__P2_CHAN_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(0) << 18))

/* macros for field P2_DMA_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__SHIFT 					 19
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__MASK				0x00080000U
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00080000U) >> 19)
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 19) & 0x00080000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00080000U) | (((uint32_t)(src) <<\
					19) & 0x00080000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 19) & ~0x00080000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00080000U) | ((uint32_t)(1) << 19))
#define PORT_INTERRUPTS__P_INT_EN__P2_DMA_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00080000U) | ((uint32_t)(0) << 19))

/* macros for field P2_OVF_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__SHIFT 					 20
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__MASK				0x00100000U
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00100000U) >> 20)
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 20) & 0x00100000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00100000U) | (((uint32_t)(src) <<\
					20) & 0x00100000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 20) & ~0x00100000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00100000U) | ((uint32_t)(1) << 20))
#define PORT_INTERRUPTS__P_INT_EN__P2_OVF_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00100000U) | ((uint32_t)(0) << 20))

/* macros for field P2_UND_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__SHIFT 					 21
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__MASK				0x00200000U
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00200000U) >> 21)
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 21) & 0x00200000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00200000U) | (((uint32_t)(src) <<\
					21) & 0x00200000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 21) & ~0x00200000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00200000U) | ((uint32_t)(1) << 21))
#define PORT_INTERRUPTS__P_INT_EN__P2_UND_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00200000U) | ((uint32_t)(0) << 21))

/* macros for field P2_FIFO_CLR */
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__SHIFT						 22
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__MASK				0x00400000U
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__READ(src) \
					(((uint32_t)(src)\
					& 0x00400000U) >> 22)
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__WRITE(src) \
					(((uint32_t)(src)\
					<< 22) & 0x00400000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00400000U) | (((uint32_t)(src) <<\
					22) & 0x00400000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 22) & ~0x00400000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__SET(dst) \
					((dst) = ((dst) &\
					~0x00400000U) | ((uint32_t)(1) << 22))
#define PORT_INTERRUPTS__P_INT_EN__P2_FIFO_CLR__CLR(dst) \
					((dst) = ((dst) &\
					~0x00400000U) | ((uint32_t)(0) << 22))

/* macros for field P2_PR_GEN_EN */
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__SHIFT						 23
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__MASK				0x00800000U
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x00800000U) >> 23)
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 23) & 0x00800000U)
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x00800000U) | (((uint32_t)(src) <<\
					23) & 0x00800000U))
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 23) & ~0x00800000U)))
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x00800000U) | ((uint32_t)(1) << 23))
#define PORT_INTERRUPTS__P_INT_EN__P2_PR_GEN_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x00800000U) | ((uint32_t)(0) << 23))

/* macros for field P3_ACT_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__SHIFT 					 24
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__MASK				0x01000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x01000000U) >> 24)
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 24) & 0x01000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x01000000U) | (((uint32_t)(src) <<\
					24) & 0x01000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 24) & ~0x01000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x01000000U) | ((uint32_t)(1) << 24))
#define PORT_INTERRUPTS__P_INT_EN__P3_ACT_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x01000000U) | ((uint32_t)(0) << 24))

/* macros for field P3_CON_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__SHIFT 					 25
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__MASK				0x02000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x02000000U) >> 25)
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 25) & 0x02000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x02000000U) | (((uint32_t)(src) <<\
					25) & 0x02000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 25) & ~0x02000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x02000000U) | ((uint32_t)(1) << 25))
#define PORT_INTERRUPTS__P_INT_EN__P3_CON_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x02000000U) | ((uint32_t)(0) << 25))

/* macros for field P3_CHAN_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__SHIFT					 26
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__WIDTH					  1
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__MASK 			0x04000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x04000000U) >> 26)
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 26) & 0x04000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x04000000U) | (((uint32_t)(src) <<\
					26) & 0x04000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 26) & ~0x04000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x04000000U) | ((uint32_t)(1) << 26))
#define PORT_INTERRUPTS__P_INT_EN__P3_CHAN_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x04000000U) | ((uint32_t)(0) << 26))

/* macros for field P3_DMA_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__SHIFT 					 27
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__MASK				0x08000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x08000000U) >> 27)
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 27) & 0x08000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x08000000U) | (((uint32_t)(src) <<\
					27) & 0x08000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 27) & ~0x08000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x08000000U) | ((uint32_t)(1) << 27))
#define PORT_INTERRUPTS__P_INT_EN__P3_DMA_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x08000000U) | ((uint32_t)(0) << 27))

/* macros for field P3_OVF_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__SHIFT 					 28
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__MASK				0x10000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x10000000U) >> 28)
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 28) & 0x10000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x10000000U) | (((uint32_t)(src) <<\
					28) & 0x10000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 28) & ~0x10000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x10000000U) | ((uint32_t)(1) << 28))
#define PORT_INTERRUPTS__P_INT_EN__P3_OVF_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x10000000U) | ((uint32_t)(0) << 28))

/* macros for field P3_UND_INT_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__SHIFT 					 29
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__WIDTH 					  1
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__MASK				0x20000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x20000000U) >> 29)
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 29) & 0x20000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x20000000U) | (((uint32_t)(src) <<\
					29) & 0x20000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 29) & ~0x20000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x20000000U) | ((uint32_t)(1) << 29))
#define PORT_INTERRUPTS__P_INT_EN__P3_UND_INT_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x20000000U) | ((uint32_t)(0) << 29))

/* macros for field P3_FIFO_CLR */
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__SHIFT						 30
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__MASK				0x40000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__READ(src) \
					(((uint32_t)(src)\
					& 0x40000000U) >> 30)
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__WRITE(src) \
					(((uint32_t)(src)\
					<< 30) & 0x40000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x40000000U) | (((uint32_t)(src) <<\
					30) & 0x40000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 30) & ~0x40000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__SET(dst) \
					((dst) = ((dst) &\
					~0x40000000U) | ((uint32_t)(1) << 30))
#define PORT_INTERRUPTS__P_INT_EN__P3_FIFO_CLR__CLR(dst) \
					((dst) = ((dst) &\
					~0x40000000U) | ((uint32_t)(0) << 30))

/* macros for field P3_PR_GEN_EN */
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__SHIFT						 31
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__WIDTH						  1
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__MASK				0x80000000U
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__READ(src) \
					(((uint32_t)(src)\
					& 0x80000000U) >> 31)
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__WRITE(src) \
					(((uint32_t)(src)\
					<< 31) & 0x80000000U)
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0x80000000U) | (((uint32_t)(src) <<\
					31) & 0x80000000U))
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__VERIFY(src) \
					(!((((uint32_t)(src)\
					<< 31) & ~0x80000000U)))
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__SET(dst) \
					((dst) = ((dst) &\
					~0x80000000U) | ((uint32_t)(1) << 31))
#define PORT_INTERRUPTS__P_INT_EN__P3_PR_GEN_EN__CLR(dst) \
					((dst) = ((dst) &\
					~0x80000000U) | ((uint32_t)(0) << 31))
#define PORT_INTERRUPTS__P_INT_EN__TYPE 							   uint32_t
#define PORT_INTERRUPTS__P_INT_EN__READ 							0xffffffffU
#define PORT_INTERRUPTS__P_INT_EN__WRITE							0xffffffffU

#endif /* __PORT_INTERRUPTS__P_INT_EN_MACRO__ */


/* macros for port_interrupts.p_int_en */
#define INST_PORT_INTERRUPTS__P_INT_EN__NUM 								 16

/* macros for BlueprintGlobalNameSpace::port_interrupts::p_int */
#ifndef __PORT_INTERRUPTS__P_INT_MACRO__
#define __PORT_INTERRUPTS__P_INT_MACRO__

/* macros for field P0_ACT_INT */
#define PORT_INTERRUPTS__P_INT__P0_ACT_INT__SHIFT							  0
#define PORT_INTERRUPTS__P_INT__P0_ACT_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P0_ACT_INT__MASK					0x00000001U
#define PORT_INTERRUPTS__P_INT__P0_ACT_INT__READ(src) \
					((uint32_t)(src)\
					& 0x00000001U)
#define PORT_INTERRUPTS__P_INT__P0_ACT_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define PORT_INTERRUPTS__P_INT__P0_ACT_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field P0_CON_INT */
#define PORT_INTERRUPTS__P_INT__P0_CON_INT__SHIFT							  1
#define PORT_INTERRUPTS__P_INT__P0_CON_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P0_CON_INT__MASK					0x00000002U
#define PORT_INTERRUPTS__P_INT__P0_CON_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define PORT_INTERRUPTS__P_INT__P0_CON_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define PORT_INTERRUPTS__P_INT__P0_CON_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field P0_CHAN_INT */
#define PORT_INTERRUPTS__P_INT__P0_CHAN_INT__SHIFT							  2
#define PORT_INTERRUPTS__P_INT__P0_CHAN_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P0_CHAN_INT__MASK					0x00000004U
#define PORT_INTERRUPTS__P_INT__P0_CHAN_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define PORT_INTERRUPTS__P_INT__P0_CHAN_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define PORT_INTERRUPTS__P_INT__P0_CHAN_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field P0_DMA_INT */
#define PORT_INTERRUPTS__P_INT__P0_DMA_INT__SHIFT							  3
#define PORT_INTERRUPTS__P_INT__P0_DMA_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P0_DMA_INT__MASK					0x00000008U
#define PORT_INTERRUPTS__P_INT__P0_DMA_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define PORT_INTERRUPTS__P_INT__P0_DMA_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define PORT_INTERRUPTS__P_INT__P0_DMA_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field P0_OVF_INT */
#define PORT_INTERRUPTS__P_INT__P0_OVF_INT__SHIFT							  4
#define PORT_INTERRUPTS__P_INT__P0_OVF_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P0_OVF_INT__MASK					0x00000010U
#define PORT_INTERRUPTS__P_INT__P0_OVF_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define PORT_INTERRUPTS__P_INT__P0_OVF_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define PORT_INTERRUPTS__P_INT__P0_OVF_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field P0_UND_INT */
#define PORT_INTERRUPTS__P_INT__P0_UND_INT__SHIFT							  5
#define PORT_INTERRUPTS__P_INT__P0_UND_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P0_UND_INT__MASK					0x00000020U
#define PORT_INTERRUPTS__P_INT__P0_UND_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define PORT_INTERRUPTS__P_INT__P0_UND_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define PORT_INTERRUPTS__P_INT__P0_UND_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field P1_ACT_INT */
#define PORT_INTERRUPTS__P_INT__P1_ACT_INT__SHIFT							  8
#define PORT_INTERRUPTS__P_INT__P1_ACT_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P1_ACT_INT__MASK					0x00000100U
#define PORT_INTERRUPTS__P_INT__P1_ACT_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000100U) >> 8)
#define PORT_INTERRUPTS__P_INT__P1_ACT_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(1) << 8))
#define PORT_INTERRUPTS__P_INT__P1_ACT_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000100U) | ((uint32_t)(0) << 8))

/* macros for field P1_CON_INT */
#define PORT_INTERRUPTS__P_INT__P1_CON_INT__SHIFT							  9
#define PORT_INTERRUPTS__P_INT__P1_CON_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P1_CON_INT__MASK					0x00000200U
#define PORT_INTERRUPTS__P_INT__P1_CON_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000200U) >> 9)
#define PORT_INTERRUPTS__P_INT__P1_CON_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(1) << 9))
#define PORT_INTERRUPTS__P_INT__P1_CON_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000200U) | ((uint32_t)(0) << 9))

/* macros for field P1_CHAN_INT */
#define PORT_INTERRUPTS__P_INT__P1_CHAN_INT__SHIFT							 10
#define PORT_INTERRUPTS__P_INT__P1_CHAN_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P1_CHAN_INT__MASK					0x00000400U
#define PORT_INTERRUPTS__P_INT__P1_CHAN_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000400U) >> 10)
#define PORT_INTERRUPTS__P_INT__P1_CHAN_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(1) << 10))
#define PORT_INTERRUPTS__P_INT__P1_CHAN_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000400U) | ((uint32_t)(0) << 10))

/* macros for field P1_DMA_INT */
#define PORT_INTERRUPTS__P_INT__P1_DMA_INT__SHIFT							 11
#define PORT_INTERRUPTS__P_INT__P1_DMA_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P1_DMA_INT__MASK					0x00000800U
#define PORT_INTERRUPTS__P_INT__P1_DMA_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00000800U) >> 11)
#define PORT_INTERRUPTS__P_INT__P1_DMA_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(1) << 11))
#define PORT_INTERRUPTS__P_INT__P1_DMA_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000800U) | ((uint32_t)(0) << 11))

/* macros for field P1_OVF_INT */
#define PORT_INTERRUPTS__P_INT__P1_OVF_INT__SHIFT							 12
#define PORT_INTERRUPTS__P_INT__P1_OVF_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P1_OVF_INT__MASK					0x00001000U
#define PORT_INTERRUPTS__P_INT__P1_OVF_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00001000U) >> 12)
#define PORT_INTERRUPTS__P_INT__P1_OVF_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(1) << 12))
#define PORT_INTERRUPTS__P_INT__P1_OVF_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00001000U) | ((uint32_t)(0) << 12))

/* macros for field P1_UND_INT */
#define PORT_INTERRUPTS__P_INT__P1_UND_INT__SHIFT							 13
#define PORT_INTERRUPTS__P_INT__P1_UND_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P1_UND_INT__MASK					0x00002000U
#define PORT_INTERRUPTS__P_INT__P1_UND_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00002000U) >> 13)
#define PORT_INTERRUPTS__P_INT__P1_UND_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(1) << 13))
#define PORT_INTERRUPTS__P_INT__P1_UND_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00002000U) | ((uint32_t)(0) << 13))

/* macros for field P2_ACT_INT */
#define PORT_INTERRUPTS__P_INT__P2_ACT_INT__SHIFT							 16
#define PORT_INTERRUPTS__P_INT__P2_ACT_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P2_ACT_INT__MASK					0x00010000U
#define PORT_INTERRUPTS__P_INT__P2_ACT_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00010000U) >> 16)
#define PORT_INTERRUPTS__P_INT__P2_ACT_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00010000U) | ((uint32_t)(1) << 16))
#define PORT_INTERRUPTS__P_INT__P2_ACT_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00010000U) | ((uint32_t)(0) << 16))

/* macros for field P2_CON_INT */
#define PORT_INTERRUPTS__P_INT__P2_CON_INT__SHIFT							 17
#define PORT_INTERRUPTS__P_INT__P2_CON_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P2_CON_INT__MASK					0x00020000U
#define PORT_INTERRUPTS__P_INT__P2_CON_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00020000U) >> 17)
#define PORT_INTERRUPTS__P_INT__P2_CON_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00020000U) | ((uint32_t)(1) << 17))
#define PORT_INTERRUPTS__P_INT__P2_CON_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00020000U) | ((uint32_t)(0) << 17))

/* macros for field P2_CHAN_INT */
#define PORT_INTERRUPTS__P_INT__P2_CHAN_INT__SHIFT							 18
#define PORT_INTERRUPTS__P_INT__P2_CHAN_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P2_CHAN_INT__MASK					0x00040000U
#define PORT_INTERRUPTS__P_INT__P2_CHAN_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00040000U) >> 18)
#define PORT_INTERRUPTS__P_INT__P2_CHAN_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(1) << 18))
#define PORT_INTERRUPTS__P_INT__P2_CHAN_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00040000U) | ((uint32_t)(0) << 18))

/* macros for field P2_DMA_INT */
#define PORT_INTERRUPTS__P_INT__P2_DMA_INT__SHIFT							 19
#define PORT_INTERRUPTS__P_INT__P2_DMA_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P2_DMA_INT__MASK					0x00080000U
#define PORT_INTERRUPTS__P_INT__P2_DMA_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00080000U) >> 19)
#define PORT_INTERRUPTS__P_INT__P2_DMA_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00080000U) | ((uint32_t)(1) << 19))
#define PORT_INTERRUPTS__P_INT__P2_DMA_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00080000U) | ((uint32_t)(0) << 19))

/* macros for field P2_OVF_INT */
#define PORT_INTERRUPTS__P_INT__P2_OVF_INT__SHIFT							 20
#define PORT_INTERRUPTS__P_INT__P2_OVF_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P2_OVF_INT__MASK					0x00100000U
#define PORT_INTERRUPTS__P_INT__P2_OVF_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00100000U) >> 20)
#define PORT_INTERRUPTS__P_INT__P2_OVF_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00100000U) | ((uint32_t)(1) << 20))
#define PORT_INTERRUPTS__P_INT__P2_OVF_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00100000U) | ((uint32_t)(0) << 20))

/* macros for field P2_UND_INT */
#define PORT_INTERRUPTS__P_INT__P2_UND_INT__SHIFT							 21
#define PORT_INTERRUPTS__P_INT__P2_UND_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P2_UND_INT__MASK					0x00200000U
#define PORT_INTERRUPTS__P_INT__P2_UND_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x00200000U) >> 21)
#define PORT_INTERRUPTS__P_INT__P2_UND_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x00200000U) | ((uint32_t)(1) << 21))
#define PORT_INTERRUPTS__P_INT__P2_UND_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x00200000U) | ((uint32_t)(0) << 21))

/* macros for field P3_ACT_INT */
#define PORT_INTERRUPTS__P_INT__P3_ACT_INT__SHIFT							 24
#define PORT_INTERRUPTS__P_INT__P3_ACT_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P3_ACT_INT__MASK					0x01000000U
#define PORT_INTERRUPTS__P_INT__P3_ACT_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x01000000U) >> 24)
#define PORT_INTERRUPTS__P_INT__P3_ACT_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x01000000U) | ((uint32_t)(1) << 24))
#define PORT_INTERRUPTS__P_INT__P3_ACT_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x01000000U) | ((uint32_t)(0) << 24))

/* macros for field P3_CON_INT */
#define PORT_INTERRUPTS__P_INT__P3_CON_INT__SHIFT							 25
#define PORT_INTERRUPTS__P_INT__P3_CON_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P3_CON_INT__MASK					0x02000000U
#define PORT_INTERRUPTS__P_INT__P3_CON_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x02000000U) >> 25)
#define PORT_INTERRUPTS__P_INT__P3_CON_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x02000000U) | ((uint32_t)(1) << 25))
#define PORT_INTERRUPTS__P_INT__P3_CON_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x02000000U) | ((uint32_t)(0) << 25))

/* macros for field P3_CHAN_INT */
#define PORT_INTERRUPTS__P_INT__P3_CHAN_INT__SHIFT							 26
#define PORT_INTERRUPTS__P_INT__P3_CHAN_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P3_CHAN_INT__MASK					0x04000000U
#define PORT_INTERRUPTS__P_INT__P3_CHAN_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x04000000U) >> 26)
#define PORT_INTERRUPTS__P_INT__P3_CHAN_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x04000000U) | ((uint32_t)(1) << 26))
#define PORT_INTERRUPTS__P_INT__P3_CHAN_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x04000000U) | ((uint32_t)(0) << 26))

/* macros for field P3_DMA_INT */
#define PORT_INTERRUPTS__P_INT__P3_DMA_INT__SHIFT							 27
#define PORT_INTERRUPTS__P_INT__P3_DMA_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P3_DMA_INT__MASK					0x08000000U
#define PORT_INTERRUPTS__P_INT__P3_DMA_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x08000000U) >> 27)
#define PORT_INTERRUPTS__P_INT__P3_DMA_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x08000000U) | ((uint32_t)(1) << 27))
#define PORT_INTERRUPTS__P_INT__P3_DMA_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x08000000U) | ((uint32_t)(0) << 27))

/* macros for field P3_OVF_INT */
#define PORT_INTERRUPTS__P_INT__P3_OVF_INT__SHIFT							 28
#define PORT_INTERRUPTS__P_INT__P3_OVF_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P3_OVF_INT__MASK					0x10000000U
#define PORT_INTERRUPTS__P_INT__P3_OVF_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x10000000U) >> 28)
#define PORT_INTERRUPTS__P_INT__P3_OVF_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x10000000U) | ((uint32_t)(1) << 28))
#define PORT_INTERRUPTS__P_INT__P3_OVF_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x10000000U) | ((uint32_t)(0) << 28))

/* macros for field P3_UND_INT */
#define PORT_INTERRUPTS__P_INT__P3_UND_INT__SHIFT							 29
#define PORT_INTERRUPTS__P_INT__P3_UND_INT__WIDTH							  1
#define PORT_INTERRUPTS__P_INT__P3_UND_INT__MASK					0x20000000U
#define PORT_INTERRUPTS__P_INT__P3_UND_INT__READ(src) \
					(((uint32_t)(src)\
					& 0x20000000U) >> 29)
#define PORT_INTERRUPTS__P_INT__P3_UND_INT__SET(dst) \
					((dst) = ((dst) &\
					~0x20000000U) | ((uint32_t)(1) << 29))
#define PORT_INTERRUPTS__P_INT__P3_UND_INT__CLR(dst) \
					((dst) = ((dst) &\
					~0x20000000U) | ((uint32_t)(0) << 29))
#define PORT_INTERRUPTS__P_INT__TYPE								   uint32_t
#define PORT_INTERRUPTS__P_INT__READ								0x3f3f3f3fU
#define PORT_INTERRUPTS__P_INT__RCLR								0x3f3f3f3fU

#endif /* __PORT_INTERRUPTS__P_INT_MACRO__ */


/* macros for port_interrupts.p_int */
#define INST_PORT_INTERRUPTS__P_INT__NUM									 16

/* macros for BlueprintGlobalNameSpace::port_state::p_state_0 */
#ifndef __PORT_STATE__P_STATE_0_MACRO__
#define __PORT_STATE__P_STATE_0_MACRO__

/* macros for field ACTIVE */
#define PORT_STATE__P_STATE_0__ACTIVE__SHIFT								  0
#define PORT_STATE__P_STATE_0__ACTIVE__WIDTH								  1
#define PORT_STATE__P_STATE_0__ACTIVE__MASK 						0x00000001U
#define PORT_STATE__P_STATE_0__ACTIVE__READ(src)  ((uint32_t)(src) & 0x00000001U)
#define PORT_STATE__P_STATE_0__ACTIVE__SET(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(1))
#define PORT_STATE__P_STATE_0__ACTIVE__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000001U) | (uint32_t)(0))

/* macros for field CONTENT_DEFINED */
#define PORT_STATE__P_STATE_0__CONTENT_DEFINED__SHIFT						  1
#define PORT_STATE__P_STATE_0__CONTENT_DEFINED__WIDTH						  1
#define PORT_STATE__P_STATE_0__CONTENT_DEFINED__MASK				0x00000002U
#define PORT_STATE__P_STATE_0__CONTENT_DEFINED__READ(src) \
					(((uint32_t)(src)\
					& 0x00000002U) >> 1)
#define PORT_STATE__P_STATE_0__CONTENT_DEFINED__SET(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(1) << 1))
#define PORT_STATE__P_STATE_0__CONTENT_DEFINED__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000002U) | ((uint32_t)(0) << 1))

/* macros for field CHANNEL_DEFINED */
#define PORT_STATE__P_STATE_0__CHANNEL_DEFINED__SHIFT						  2
#define PORT_STATE__P_STATE_0__CHANNEL_DEFINED__WIDTH						  1
#define PORT_STATE__P_STATE_0__CHANNEL_DEFINED__MASK				0x00000004U
#define PORT_STATE__P_STATE_0__CHANNEL_DEFINED__READ(src) \
					(((uint32_t)(src)\
					& 0x00000004U) >> 2)
#define PORT_STATE__P_STATE_0__CHANNEL_DEFINED__SET(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(1) << 2))
#define PORT_STATE__P_STATE_0__CHANNEL_DEFINED__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000004U) | ((uint32_t)(0) << 2))

/* macros for field SINK */
#define PORT_STATE__P_STATE_0__SINK__SHIFT									  3
#define PORT_STATE__P_STATE_0__SINK__WIDTH									  1
#define PORT_STATE__P_STATE_0__SINK__MASK							0x00000008U
#define PORT_STATE__P_STATE_0__SINK__READ(src) \
					(((uint32_t)(src)\
					& 0x00000008U) >> 3)
#define PORT_STATE__P_STATE_0__SINK__SET(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(1) << 3))
#define PORT_STATE__P_STATE_0__SINK__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000008U) | ((uint32_t)(0) << 3))

/* macros for field OVF */
#define PORT_STATE__P_STATE_0__OVF__SHIFT									  4
#define PORT_STATE__P_STATE_0__OVF__WIDTH									  1
#define PORT_STATE__P_STATE_0__OVF__MASK							0x00000010U
#define PORT_STATE__P_STATE_0__OVF__READ(src) \
					(((uint32_t)(src)\
					& 0x00000010U) >> 4)
#define PORT_STATE__P_STATE_0__OVF__SET(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(1) << 4))
#define PORT_STATE__P_STATE_0__OVF__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000010U) | ((uint32_t)(0) << 4))

/* macros for field UND */
#define PORT_STATE__P_STATE_0__UND__SHIFT									  5
#define PORT_STATE__P_STATE_0__UND__WIDTH									  1
#define PORT_STATE__P_STATE_0__UND__MASK							0x00000020U
#define PORT_STATE__P_STATE_0__UND__READ(src) \
					(((uint32_t)(src)\
					& 0x00000020U) >> 5)
#define PORT_STATE__P_STATE_0__UND__SET(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(1) << 5))
#define PORT_STATE__P_STATE_0__UND__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000020U) | ((uint32_t)(0) << 5))

/* macros for field DPORT_READY */
#define PORT_STATE__P_STATE_0__DPORT_READY__SHIFT							  6
#define PORT_STATE__P_STATE_0__DPORT_READY__WIDTH							  1
#define PORT_STATE__P_STATE_0__DPORT_READY__MASK					0x00000040U
#define PORT_STATE__P_STATE_0__DPORT_READY__READ(src) \
					(((uint32_t)(src)\
					& 0x00000040U) >> 6)
#define PORT_STATE__P_STATE_0__DPORT_READY__SET(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(1) << 6))
#define PORT_STATE__P_STATE_0__DPORT_READY__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000040U) | ((uint32_t)(0) << 6))

/* macros for field S_INTERVAL */
#define PORT_STATE__P_STATE_0__S_INTERVAL__SHIFT							 16
#define PORT_STATE__P_STATE_0__S_INTERVAL__WIDTH							 11
#define PORT_STATE__P_STATE_0__S_INTERVAL__MASK 					0x07ff0000U
#define PORT_STATE__P_STATE_0__S_INTERVAL__READ(src) \
					(((uint32_t)(src)\
					& 0x07ff0000U) >> 16)

/* macros for field TR_PROTOCOL */
#define PORT_STATE__P_STATE_0__TR_PROTOCOL__SHIFT							 28
#define PORT_STATE__P_STATE_0__TR_PROTOCOL__WIDTH							  4
#define PORT_STATE__P_STATE_0__TR_PROTOCOL__MASK					0xf0000000U
#define PORT_STATE__P_STATE_0__TR_PROTOCOL__READ(src) \
					(((uint32_t)(src)\
					& 0xf0000000U) >> 28)
#define PORT_STATE__P_STATE_0__TYPE 								   uint32_t
#define PORT_STATE__P_STATE_0__READ 								0xf7ff007fU

#endif /* __PORT_STATE__P_STATE_0_MACRO__ */


/* macros for port_state.p_state_0 */
#define INST_PORT_STATE__P_STATE_0__NUM 									  1

/* macros for BlueprintGlobalNameSpace::port_state::p_state_1 */
#ifndef __PORT_STATE__P_STATE_1_MACRO__
#define __PORT_STATE__P_STATE_1_MACRO__

/* macros for field P_RATE */
#define PORT_STATE__P_STATE_1__P_RATE__SHIFT								  0
#define PORT_STATE__P_STATE_1__P_RATE__WIDTH								  7
#define PORT_STATE__P_STATE_1__P_RATE__MASK 						0x0000007fU
#define PORT_STATE__P_STATE_1__P_RATE__READ(src)  ((uint32_t)(src) & 0x0000007fU)

/* macros for field FR_LOCK */
#define PORT_STATE__P_STATE_1__FR_LOCK__SHIFT								  7
#define PORT_STATE__P_STATE_1__FR_LOCK__WIDTH								  1
#define PORT_STATE__P_STATE_1__FR_LOCK__MASK						0x00000080U
#define PORT_STATE__P_STATE_1__FR_LOCK__READ(src) \
					(((uint32_t)(src)\
					& 0x00000080U) >> 7)
#define PORT_STATE__P_STATE_1__FR_LOCK__SET(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(1) << 7))
#define PORT_STATE__P_STATE_1__FR_LOCK__CLR(dst) \
					((dst) = ((dst) &\
					~0x00000080U) | ((uint32_t)(0) << 7))

/* macros for field DATA_TYPE */
#define PORT_STATE__P_STATE_1__DATA_TYPE__SHIFT 							  8
#define PORT_STATE__P_STATE_1__DATA_TYPE__WIDTH 							  4
#define PORT_STATE__P_STATE_1__DATA_TYPE__MASK						0x00000f00U
#define PORT_STATE__P_STATE_1__DATA_TYPE__READ(src) \
					(((uint32_t)(src)\
					& 0x00000f00U) >> 8)

/* macros for field DATA_LENGTH */
#define PORT_STATE__P_STATE_1__DATA_LENGTH__SHIFT							 16
#define PORT_STATE__P_STATE_1__DATA_LENGTH__WIDTH							  6
#define PORT_STATE__P_STATE_1__DATA_LENGTH__MASK					0x003f0000U
#define PORT_STATE__P_STATE_1__DATA_LENGTH__READ(src) \
					(((uint32_t)(src)\
					& 0x003f0000U) >> 16)

/* macros for field PORT_LINKED */
#define PORT_STATE__P_STATE_1__PORT_LINKED__SHIFT							 24
#define PORT_STATE__P_STATE_1__PORT_LINKED__WIDTH							  6
#define PORT_STATE__P_STATE_1__PORT_LINKED__MASK					0x3f000000U
#define PORT_STATE__P_STATE_1__PORT_LINKED__READ(src) \
					(((uint32_t)(src)\
					& 0x3f000000U) >> 24)

/* macros for field CH_LINK */
#define PORT_STATE__P_STATE_1__CH_LINK__SHIFT								 31
#define PORT_STATE__P_STATE_1__CH_LINK__WIDTH								  1
#define PORT_STATE__P_STATE_1__CH_LINK__MASK						0x80000000U
#define PORT_STATE__P_STATE_1__CH_LINK__READ(src) \
					(((uint32_t)(src)\
					& 0x80000000U) >> 31)
#define PORT_STATE__P_STATE_1__CH_LINK__SET(dst) \
					((dst) = ((dst) &\
					~0x80000000U) | ((uint32_t)(1) << 31))
#define PORT_STATE__P_STATE_1__CH_LINK__CLR(dst) \
					((dst) = ((dst) &\
					~0x80000000U) | ((uint32_t)(0) << 31))
#define PORT_STATE__P_STATE_1__TYPE 								   uint32_t
#define PORT_STATE__P_STATE_1__READ 								0xbf3f0fffU

#endif /* __PORT_STATE__P_STATE_1_MACRO__ */


/* macros for port_state.p_state_1 */
#define INST_PORT_STATE__P_STATE_1__NUM 									  1

/* macros for BlueprintGlobalNameSpace::port_fifo_space::port_fifo */
#ifndef __PORT_FIFO_SPACE__PORT_FIFO_MACRO__
#define __PORT_FIFO_SPACE__PORT_FIFO_MACRO__

/* macros for field FIFO_DATA */
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__SHIFT						  0
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__WIDTH						 32
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__MASK 				0xffffffffU
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__READ(src) \
					((uint32_t)(src)\
					& 0xffffffffU)
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__WRITE(src) \
					((uint32_t)(src)\
					& 0xffffffffU)
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__MODIFY(dst, src) \
					((dst) = ((dst) &\
					~0xffffffffU) | ((uint32_t)(src) &\
					0xffffffffU))
#define PORT_FIFO_SPACE__PORT_FIFO__FIFO_DATA__VERIFY(src) \
					(!(((uint32_t)(src)\
					& ~0xffffffffU)))
#define PORT_FIFO_SPACE__PORT_FIFO__TYPE							   uint32_t
#define PORT_FIFO_SPACE__PORT_FIFO__READ							0xffffffffU
#define PORT_FIFO_SPACE__PORT_FIFO__WRITE							0xffffffffU

#endif /* __PORT_FIFO_SPACE__PORT_FIFO_MACRO__ */


/* macros for port_fifo_space.port_fifo */
#define INST_PORT_FIFO_SPACE__PORT_FIFO__NUM								 16
#define RFILE_INST_CONFIGURATION__NUM										  1
#define RFILE_INST_COMMAND_STATUS__NUM										  1
#define RFILE_INST_INTERRUPTS__NUM											  1
#define RFILE_INST_MESSAGE_FIFOS__NUM										  1
#define RFILE_INST_PORT_INTERRUPTS__NUM 									  1
#define RFILE_INST_PORT_STATE__NUM											 64
#define RFILE_INST_PORT_FIFO_SPACE__NUM 									 64

#endif /* __SLIMBUS_DRV_REGS_MACRO_H__ */
