/* MD5: 595e6e337392f972b7ae17960efe2592*/
/*
 * copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if !defined(__PRODUCT_CONFIG_TLPHY_H__)
#define __PRODUCT_CONFIG_TLPHY_H__

#ifndef XTENSA_CORE
#define XTENSA_CORE Phoenix_NX 
#endif 

#ifndef XTENSA_SYSTEM
#define XTENSA_SYSTEM "$(ROOT_XTENSA_PATH_W)/XtDevTools/install/builds/RH-2018.5-linux/$(CFG_XTENSA_CORE)/config" 
#endif 

#ifndef TENSILICA_BUILDS
#define TENSILICA_BUILDS "$(ROOT_XTENSA_PATH_W)/XtDevTools/install/builds/RH-2018.5-linux" 
#endif 

#ifndef TENSILICA_TOOLS
#define TENSILICA_TOOLS "$(ROOT_XTENSA_PATH_W)/XtDevTools/install/tools/RH-2018.5-linux" 
#endif 

#ifndef XTENSA_PREDICT_BUG
#endif 

#ifndef XTENSA_INST_PREFETCH_BUG
#endif 

#ifndef XTENSA_DATA_PREFETCH_BUG
#endif 

#ifndef PHY_EXTERN_BOOT
#endif 

#ifndef LPHY_DTCM_BASE
#define LPHY_DTCM_BASE 0xE3400000 
#endif 

#ifndef LPHY_ITCM_BASE
#define LPHY_ITCM_BASE 0xE3500000 
#endif 

#ifndef LPHY_DTCM_SIZE
#define LPHY_DTCM_SIZE 0xC0000 
#endif 

#ifndef LPHY_ITCM_SIZE
#define LPHY_ITCM_SIZE 0xC0000 
#endif 

#ifndef LPHY_L2M_BASE
#define LPHY_L2M_BASE 0xE3680000 
#endif 

#ifndef LPHY_L2M_SIZE
#define LPHY_L2M_SIZE 0x100000 
#endif 

#ifndef LPHY_L2C_BASE
#define LPHY_L2C_BASE 0xE3780000 
#endif 

#ifndef LPHY_L2C_SIZE
#define LPHY_L2C_SIZE 0x80000 
#endif 

#ifndef LPHY_SRAM_BASE
#define LPHY_SRAM_BASE 0x0 
#endif 

#ifndef LPHY_SRAM_SIZE
#define LPHY_SRAM_SIZE 0x0 
#endif 

#ifndef LPHY_DDR_BASE
#define LPHY_DDR_BASE ((DDR_TLPHY_IMAGE_ADDR)+0x10000) 
#endif 

#ifndef LPHY_DDR_SIZE
#define LPHY_DDR_SIZE 0x600000 
#endif 

#ifndef LPHY_TOTAL_IMG_SIZE
#define LPHY_TOTAL_IMG_SIZE ((LPHY_DTCM_SIZE)+(LPHY_ITCM_SIZE)+(LPHY_L2M_SIZE)+(LPHY_DDR_SIZE)) 
#endif 

#ifndef LPHY_SLT_DDR_BASE
#define LPHY_SLT_DDR_BASE ((TLPHY_IMAGE_LOAD_ADDR_ATE)) 
#endif 

#ifndef LPHY_SLT_DDR_SIZE
#define LPHY_SLT_DDR_SIZE ((TLPHY_IMAGE_LOAD_SIZE_ATE)) 
#endif 

#ifndef TL_PHY_ASIC
#define TL_PHY_ASIC 
#endif 

#ifndef TL_PHY_V760
#define TL_PHY_V760 
#endif 

#ifndef TL_PHY_HI3690
#define TL_PHY_HI3690 
#endif 

#ifndef TL_PHY_BBE16_CACHE
#endif 

#ifndef TL_PHY_SUPPORT_DUAL_MODEM
#define TL_PHY_SUPPORT_DUAL_MODEM 
#endif 

#ifndef TL_PHY_SUPPORT_IMAGE_HEADER
#define TL_PHY_SUPPORT_IMAGE_HEADER 
#endif 

#ifndef FEATURE_LTE_4RX
#define FEATURE_LTE_4RX FEATURE_OFF 
#endif 

#ifndef FEATURE_LTE_8RX
#define FEATURE_LTE_8RX FEATURE_OFF 
#endif 

#ifndef PHY_NX_CORE_ID
#define PHY_NX_CORE_ID 0 
#endif 

#ifndef FEATURE_TLPHY_LOWER_SAR
#define FEATURE_TLPHY_LOWER_SAR FEATURE_OFF 
#endif 

#ifndef BBPCONFIG_VERIOSN
#define BBPCONFIG_VERIOSN bbp_config_hi6965 
#endif 

#ifndef TL_PHY_FEATURE_LTE_LCS
#endif 

#ifndef LPHY_FEATURE_LTEV_SWITCH
#define LPHY_FEATURE_LTEV_SWITCH FEATURE_OFF 
#endif 

#ifndef FEATURE_TLPHY_SINGLE_XO
#define FEATURE_TLPHY_SINGLE_XO 
#endif 

#ifndef MULTI_PHY_CO_PROCESSOR
#define MULTI_PHY_CO_PROCESSOR 
#endif 

#ifndef MULTI_PHY_CO_PROCEDURE
#define MULTI_PHY_CO_PROCEDURE 
#endif 

#ifndef FEATURE_TDS_ENABLE
#define FEATURE_TDS_ENABLE FEATURE_OFF 
#endif 

#ifndef FEATURE_TLPHY_ET
#define FEATURE_TLPHY_ET FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_NEWET
#define FEATURE_TLPHY_NEWET FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_DPD
#define FEATURE_TLPHY_DPD FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_ETM_ONLY
#define FEATURE_TLPHY_ETM_ONLY FEATURE_OFF 
#endif 

#ifndef FEATURE_TLPHY_DPD_OPT
#define FEATURE_TLPHY_DPD_OPT FEATURE_ON 
#endif 

#ifndef FEATURE_NEWMRX_CAL_APC
#define FEATURE_NEWMRX_CAL_APC FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_FEIC_ENABLE
#define FEATURE_TLPHY_FEIC_ENABLE FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_FEIC_MULTI_AGC_LEVEL
#define FEATURE_TLPHY_FEIC_MULTI_AGC_LEVEL FEATURE_ON 
#endif 

#ifndef TL_PHY_FEATURE_SCELL_USE_LISTEN
#define TL_PHY_FEATURE_SCELL_USE_LISTEN 
#endif 

#ifndef TLPHY_DSDS_OPTIMIZE_ENABLE
#define TLPHY_DSDS_OPTIMIZE_ENABLE 
#endif 

#ifndef FEATURE_TLPHY_WTC_SWTICH
#define FEATURE_TLPHY_WTC_SWTICH FEATURE_ON 
#endif 

#ifndef TL_RTT_TEST_ENABLED
#define TL_RTT_TEST_ENABLED 
#endif 

#ifndef FEATURE_TLPHY_ANT_BLANK
#define FEATURE_TLPHY_ANT_BLANK FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_HI6353_SUPPORT
#define FEATURE_TLPHY_HI6353_SUPPORT FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_HI6H03_SUPPORT
#define FEATURE_TLPHY_HI6H03_SUPPORT FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_HI6363A_SUPPORT
#define FEATURE_TLPHY_HI6363A_SUPPORT FEATURE_OFF 
#endif 

#ifndef FEATURE_TLPHY_MULTIBAND_TUNER
#define FEATURE_TLPHY_MULTIBAND_TUNER FEATURE_ON 
#endif 

#ifndef FEATURE_LPHY_POR_EXT_SWITCH
#define FEATURE_LPHY_POR_EXT_SWITCH FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_SAR_GROUP
#define FEATURE_TLPHY_SAR_GROUP FEATURE_ON 
#endif 

#ifndef FEATURE_LPHY_UL_AMPR_R14
#define FEATURE_LPHY_UL_AMPR_R14 FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_HITUNER_SWITCH
#define FEATURE_TLPHY_HITUNER_SWITCH FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_HALL_SWITCH
#define FEATURE_TLPHY_HALL_SWITCH FEATURE_OFF 
#endif 

#ifndef FEATURE_TLPHY_HPUE_SWITCH
#define FEATURE_TLPHY_HPUE_SWITCH FEATURE_ON 
#endif 

#ifndef FEATURE_TLPHY_PHR_ENHANCEMENT_SWITCH
#define FEATURE_TLPHY_PHR_ENHANCEMENT_SWITCH FEATURE_ON 
#endif 

#ifndef FEATURE_LPHY_UL_AMPR_R15
#define FEATURE_LPHY_UL_AMPR_R15 FEATURE_ON 
#endif 

#ifndef FEATURE_LPHY_NEW_NVID_SWITCH
#define FEATURE_LPHY_NEW_NVID_SWITCH FEATURE_OFF 
#endif 

#ifndef FEATURE_MC_TS_CNT_FROM_GPIO
#define FEATURE_MC_TS_CNT_FROM_GPIO FEATURE_OFF 
#endif 

#ifndef FEATURE_LPHY_SLT_ALLOCATED_ON_DDR
#define FEATURE_LPHY_SLT_ALLOCATED_ON_DDR FEATURE_OFF 
#endif 

#endif /*__PRODUCT_CONFIG_H__*/ 
