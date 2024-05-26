/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sc8562.h
 *
 * sc8562 header file
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#ifndef _SC8562_H_
#define _SC8562_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/raid/pq.h>
#include <linux/bitops.h>
#include <chipset_common/hwpower/common_module/power_log.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_error_handle.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic.h>

#define SC8562_SCP_MAX_DATA_LEN                   32
#define SC8562_NOT_USED                           0
#define SC8562_USED                               1
#define SC8562_INIT_FINISH                        1
#define SC8562_BUF_LEN                            80
#define SC8562_DEVICE_ID_GET_FAIL                 (-1)
#define SC8562_SWITCHCAP_DISABLE                  0
#define SC8562_GPIO_ENABLE                        1

/* DEVICE_VER reg=0x00 */
#define SC8562_DEVICE_VER_REG                     0x00
#define SC8562_DEVICE_VER_MASK                    0xFF
#define SC8562_DEVICE_VER_SHIFT                   0

/* VBAT_OVP reg=0x01 */
#define SC8562_VBAT_OVP_REG                       0x01
#define SC8562_VBAT_OVP_DIS_MASK                  BIT(7)
#define SC8562_VBAT_OVP_DIS_SHIFT                 7
#define SC8562_VBAT_OVP_MASK_MASK                 BIT(6)
#define SC8562_VBAT_OVP_MASK_SHIFT                6
#define SC8562_VBAT_OVP_FLAG_MASK                 BIT(5)
#define SC8562_VBAT_OVP_FLAG_SHIFT                5
#define SC8562_VBAT_OVP_TH_MASK                   (BIT(4) | BIT(3) | BIT(2) | \
	BIT(1) | BIT(0))
#define SC8562_VBAT_OVP_TH_SHIFT                  0
#define SC8562_VBAT_OVP_MIN                       4450
#define SC8562_VBAT_OVP_INIT                      4700
#define SC8562_VBAT_OVP_MAX                       5225
#define SC8562_VBAT_OVP_STEP                      25

/* IBAT_OCP reg=0x02 */
#define SC8562_IBAT_OCP_REG                       0x02
#define SC8562_IBAT_OCP_DIS_MASK                  BIT(7)
#define SC8562_IBAT_OCP_DIS_SHIFT                 7
#define SC8562_IBAT_OCP_MASK_MASK                 BIT(5)
#define SC8562_IBAT_OCP_MASK_SHIFT                5
#define SC8562_IBAT_OCP_FLAG_MASK                 BIT(4)
#define SC8562_IBAT_OCP_FLAG_SHIFT                4
#define SC8562_IBAT_OCP_TH_MASK                   (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_IBAT_OCP_TH_SHIFT                  0
#define SC8562_IBAT_OCP_MIN                       6000
#define SC8562_IBAT_OCP_MAX                       13500
#define SC8562_IBAT_OCP_STEP                      500
#define SC8562_IBAT_OCP_FBPSC_INIT                6500
#define SC8562_IBAT_OCP_F21SC_INIT                10000
#define SC8562_IBAT_OCP_F41SC_INIT                12000

/* VUSB_OVP reg=0x03 */
#define SC8562_VUSB_OVP_REG                       0x03
#define SC8562_OVPGATE_ON_DG_SET_MASK             BIT(7)
#define SC8562_OVPGATE_ON_DG_SET_SHIFT            7
#define SC8562_VUSB_OVP_MASK_MASK                 BIT(6)
#define SC8562_VUSB_OVP_MASK_SHIFT                6
#define SC8562_VUSB_OVP_FLAG_MASK                 BIT(5)
#define SC8562_VUSB_OVP_FLAG_SHIFT                5
#define SC8562_VUSB_OVP_STAT_MASK                 BIT(4)
#define SC8562_VUSB_OVP_STAT_SHIFT                4
#define SC8562_VUSB_OVP_TH_MASK                   (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VUSB_OVP_TH_SHIFT                  0
#define SC8562_VUSB_OVP_MIN                       11000
#define SC8562_VUSB_OVP_MAX                       25000
#define SC8562_VUSB_OVP_STEP                      1000
#define SC8562_VUSB_OVP_DEF                       6500
#define SC8562_VUSB_OVP_INIT                      6000
#define SC8562_VUSB_OVP_F21SC_INIT                13000
#define SC8562_VUSB_OVP_F41SC_INIT                25000
#define SC8562_VUSB_OVP_DEF_VAL                   0xf

/* VWPC_OVP reg=0x04 */
#define SC8562_VWPC_OVP_REG                       0x04
#define SC8562_WPCGATE_ON_DG_SET_MASK             BIT(7)
#define SC8562_WPCGATE_ON_DG_SET_SHIFT            7
#define SC8562_VWPC_OVP_MASK_MASK                 BIT(6)
#define SC8562_VWPC_OVP_MASK_SHIFT                6
#define SC8562_VWPC_OVP_FLAG_MASK                 BIT(5)
#define SC8562_VWPC_OVP_FLAG_SHIFT                5
#define SC8562_VWPC_OVP_STAT_MASK                 BIT(4)
#define SC8562_VWPC_OVP_STAT_SHIFT                4
#define SC8562_VWPC_OVP_TH_MASK                   (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VWPC_OVP_TH_SHIFT                  0

/* VOUT_VBUS_OVP reg=0x05 */
#define SC8562_VOUT_VBUS_OVP_REG                  0x05
#define SC8562_VBUS_OVP_TH_MASK                   (BIT(7) | BIT(6) | BIT(5) | \
	BIT(4) | BIT(3) | BIT(2))
#define SC8562_VBUS_OVP_TH_SHIFT                  2
#define SC8562_VOUT_OVP_TH_MASK                   (BIT(1) | BIT(0))
#define SC8562_VOUT_OVP_TH_SHIFT                  0
#define SC8562_VBUS_OVP_F41SC_MIN                 14000
#define SC8562_VBUS_OVP_F21SC_MIN                 7000
#define SC8562_VBUS_OVP_FBPSC_MIN                 3500
#define SC8562_VBUS_OVP_F41SC_MAX                 26600
#define SC8562_VBUS_OVP_F21SC_MAX                 13300
#define SC8562_VBUS_OVP_FBPSC_MAX                 6650
#define SC8562_VBUS_OVP_F41SC_INIT                22000
#define SC8562_VBUS_OVP_F21SC_INIT                11000
#define SC8562_VBUS_OVP_FBPSC_INIT                5800
#define SC8562_VBUS_OVP_FBPSC_BASE                6500
#define SC8562_VBUS_OVP_F41SC_STEP                200
#define SC8562_VBUS_OVP_F21SC_STEP                100
#define SC8562_VBUS_OVP_FBPSC_STEP                50
#define SC8562_VOUT_OVP_MIN                       4800
#define SC8562_VOUT_OVP_DEF                       5000
#define SC8562_VOUT_OVP_INIT                      4800
#define SC8562_VOUT_OVP_MAX                       5400
#define SC8562_VOUT_OVP_STEP                      200

/* IBUS_OCP reg=0x06 */
#define SC8562_IBUS_OCP_REG                       0x06
#define SC8562_IBUS_OCP_DIS_MASK                  BIT(7)
#define SC8562_IBUS_OCP_DIS_SHIFT                 7
#define SC8562_IBUS_OCP_MASK_MASK                 BIT(6)
#define SC8562_IBUS_OCP_MASK_SHIFT                6
#define SC8562_IBUS_OCP_FLAG_MASK                 BIT(5)
#define SC8562_IBUS_OCP_FLAG_SHIFT                5
#define SC8562_IBUS_OCP_TH_MASK                   (BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_IBUS_OCP_TH_SHIFT                  0
#define SC8562_IBUS_OCP_FBPSC_INIT                6375
#define SC8562_IBUS_OCP_F21SC_INIT                5500
#define SC8562_IBUS_OCP_F41SC_INIT                3500
#define SC8562_IBUS_OCP_MIN                       2500
#define SC8562_IBUS_OCP_MAX                       6375
#define SC8562_IBUS_OCP_STEP                      125
#define SC8562_IBUS_OCP_ENABLE                    1

/* IBUS_UCP reg=0x07 */
#define SC8562_IBUS_UCP_REG                       0x07
#define SC8562_IBUS_UCP_DIS_MASK                  BIT(7)
#define SC8562_IBUS_UCP_DIS_SHIFT                 7
#define SC8562_IBUS_UCP_FALL_DG_SET_MASK          (BIT(5) | BIT(4))
#define SC8562_IBUS_UCP_FALL_DG_SET_SHIFT         4
#define SC8562_IBUS_UCP_RISE_MASK_MASK            BIT(3)
#define SC8562_IBUS_UCP_RISE_MASK_SHIFT           3
#define SC8562_IBUS_UCP_RISE_FLAG_MASK            BIT(2)
#define SC8562_IBUS_UCP_RISE_FLAG_SHIFT           2
#define SC8562_IBUS_UCP_FALL_MASK_MASK            BIT(1)
#define SC8562_IBUS_UCP_FALL_MASK_SHIFT           1
#define SC8562_IBUS_UCP_FALL_FLAG_MASK            BIT(0)
#define SC8562_IBUS_UCP_FALL_FLAG_SHIFT           0
#define SC8562_IBUS_UCP_DIS_MASK_DISABLE          0x90
#define SC8562_IBUS_UCP_REG_INIT                  0x10

/* PMID2OUT_OVP reg=0x08 */
#define SC8562_PMID2OUT_OVP_REG                   0x08
#define SC8562_PMID2OUT_OVP_DIS_MASK              BIT(7)
#define SC8562_PMID2OUT_OVP_DIS_SHIFT             7
#define SC8562_PMID2OUT_OVP_MASK_MASK             BIT(4)
#define SC8562_PMID2OUT_OVP_MASK_SHIFT            4
#define SC8562_PMID2OUT_OVP_FLAG_MASK             BIT(3)
#define SC8562_PMID2OUT_OVP_FLAG_SHIFT            3
#define SC8562_PMID2OUT_OVP_TH_MASK               (BIT(2) | BIT(1) | BIT(0))
#define SC8562_PMID2OUT_OVP_TH_SHIFT              0

/* PMID2OUT_UVP reg=0x09 */
#define SC8562_PMID2OUT_UVP_REG                   0x09
#define SC8562_PMID2OUT_UVP_DIS_MASK              BIT(7)
#define SC8562_PMID2OUT_UVP_DIS_SHIFT             7
#define SC8562_PMID2OUT_UVP_MASK_MASK             BIT(4)
#define SC8562_PMID2OUT_UVP_MASK_SHIFT            4
#define SC8562_PMID2OUT_UVP_FLAG_MASK             BIT(3)
#define SC8562_PMID2OUT_UVP_FLAG_SHIFT            3
#define SC8562_PMID2OUT_UVP_TH_MASK               (BIT(2) | BIT(1) | BIT(0))
#define SC8562_PMID2OUT_UVP_TH_SHIFT              0

/* CONVERTER_STATE reg=0x0A */
#define SC8562_CONVERTER_STATE_REG                0x0A
#define SC8562_POR_FLAG_MASK                      BIT(7)
#define SC8562_POR_FLAG_SHIFT                     7
#define SC8562_ACRB_WPC_STAT_MASK                 BIT(6)
#define SC8562_ACRB_WPC_STAT_SHIFT                6
#define SC8562_ACRB_USB_STAT_MASK                 BIT(5)
#define SC8562_ACRB_USB_STAT_SHIFT                5
#define SC8562_VBUS_ERRORLO_STAT_MASK             BIT(4)
#define SC8562_VBUS_ERRORLO_STAT_SHIFT            4
#define SC8562_VBUS_ERRORHI_STAT_MASK             BIT(3)
#define SC8562_VBUS_ERRORHI_STAT_SHIFT            3
#define SC8562_QB_ON_STAT_MASK                    BIT(2)
#define SC8562_QB_ON_STAT_SHIFT                   2
#define SC8562_CP_SWITCHING_STAT_MASK             BIT(1)
#define SC8562_CP_SWITCHING_STAT_SHIFT            1
#define SC8562_PIN_DIAG_FAIL_FLAG_MASK            BIT(0)
#define SC8562_PIN_DIAG_FAIL_FLAG_SHIFT           0

/* CTRL1 reg=0x0B */
#define SC8562_CTRL1_REG                          0x0B
#define SC8562_CTRL1_CP_EN_MASK                   BIT(7)
#define SC8562_CTRL1_CP_EN_SHIFT                  7
#define SC8562_CTRL1_QB_EN_MASK                   BIT(6)
#define SC8562_CTRL1_QB_EN_SHIFT                  6
#define SC8562_CTRL1_ACDRV_MANUAL_EN_MASK         BIT(5)
#define SC8562_CTRL1_ACDRV_MANUAL_EN_SHIFT        5
#define SC8562_CTRL1_WPCGATE_EN_MASK              BIT(4)
#define SC8562_CTRL1_WPCGATE_EN_SHIFT             4
#define SC8562_CTRL1_OVPGATE_EN_MASK              BIT(3)
#define SC8562_CTRL1_OVPGATE_EN_SHIFT             3
#define SC8562_CTRL1_VBUS_PD_EN_MASK              BIT(2)
#define SC8562_CTRL1_VBUS_PD_EN_SHIFT             2
#define SC8562_CTRL1_VWPC_PD_EN_MASK              BIT(1)
#define SC8562_CTRL1_VWPC_PD_EN_SHIFT             1
#define SC8562_CTRL1_VUSB_PD_EN_MASK              BIT(0)
#define SC8562_CTRL1_VUSB_PD_EN_SHIFT             0
#define SC8562_CTRL1_CP_ENABLE                    1
#define SC8562_CTRL1_CP_DISABLE                   0
#define SC8562_CTRL1_VUSB_PD_ENABLE               1
#define SC8562_CTRL1_VUSB_PD_DISABLE              0

/* CTRL2 reg=0x0C */
#define SC8562_CTRL2_REG                          0x0C
#define SC8562_CTRL2_FSW_SET_MASK                 (BIT(7) | BIT(6) | BIT(5) | \
	BIT(4) | BIT(3))
#define SC8562_CTRL2_FSW_SET_SHIFT                3
#define SC8562_CTRL2_FREQ_DITHER_MASK             BIT(2)
#define SC8562_CTRL2_FREQ_DITHER_SHIFT            2
#define SC8562_CTRL2_SYNC_MASK                    (BIT(1) | BIT(0))
#define SC8562_CTRL2_SYNC_SHIFT                   0
#define SC8562_SW_FREQ_MIN                        300
#define SC8562_SW_FREQ_750KHZ                     750
#define SC8562_SW_FREQ_MAX                        1075
#define SC8562_SW_FREQ_STEP                       25
#define SC8562_SW_FREQ_SHIFT_NORMAL               0
#define SC8562_SW_FREQ_SHIFT_MP_P10               1 /* +/-10% */

/* CTRL3 reg=0x0D */
#define SC8562_CTRL3_REG                          0x0D
#define SC8562_CTRL3_SS_TIMEOUT_MASK              (BIT(5) | BIT(4) | BIT(3))
#define SC8562_CTRL3_SS_TIMEOUT_SHIFT             3
#define SC8562_CTRL3_WD_TIMEOUT_MASK              (BIT(2) | BIT(1) | BIT(0))
#define SC8562_CTRL3_WD_TIMEOUT_SHIFT             0
#define SC8562_WD_TMR_30000MS                     0x5
#define SC8562_WD_TMR_5000MS                      0x4
#define SC8562_WD_TMR_1000MS                      0x3
#define SC8562_WD_TMR_500MS                       0x2
#define SC8562_WD_TMR_200MS                       0x1
#define SC8562_WD_TMR_TIMING_30000MS              30000
#define SC8562_WD_TMR_TIMING_5000MS               5000
#define SC8562_WD_TMR_TIMING_1000MS               1000
#define SC8562_WD_TMR_TIMING_500MS                500
#define SC8562_WD_TMR_TIMING_200MS                200
#define SC8562_WD_TMR_TIMING_DIS                  0
#define SC8562_SS_TIMEOUT_DISABLE                 0

/* CTRL4 reg=0x0E */
#define SC8562_CTRL4_REG                          0x0E
#define SC8562_CTRL4_SYNC_FUNCTION_EN_MASK        BIT(7)
#define SC8562_CTRL4_SYNC_FUNCTION_EN_SHIFT       7
#define SC8562_CTRL4_SYNC_MASTER_EN_MASK          BIT(6)
#define SC8562_CTRL4_SYNC_MASTER_EN_SHIFT         6
#define SC8562_CTRL4_VBAT_OVP_DG_SET_MASK         BIT(5)
#define SC8562_CTRL4_VBAT_OVP_DG_SET_SHIFT        5
#define SC8562_CTRL4_SET_IBAT_SNS_RES_MASK        BIT(4)
#define SC8562_CTRL4_SET_IBAT_SNS_RES_SHIFT       4
#define SC8562_CTRL4_REG_RST_MASK                 BIT(3)
#define SC8562_CTRL4_REG_RST_SHIFT                3
#define SC8562_CTRL4_MODE_MASK                    (BIT(2) | BIT(1) | BIT(0))
#define SC8562_CTRL4_MODE_SHIFT                   0
#define SC8562_REG_RST_ENABLE                     1
#define SC8562_CHG_FBYPASS_MODE                   2
#define SC8562_CHG_F21SC_MODE                     1
#define SC8562_CHG_F41SC_MODE                     0
#define SC8562_IBAT_SNS_RES_1MOHM                 0
#define SC8562_IBAT_SNS_RES_2MOHM                 1

/* CTRL5 reg=0x0F */
#define SC8562_CTRL5_REG                          0x0F
#define SC8562_CTRL5_OVPGATE_STAT_MASK            BIT(7)
#define SC8562_CTRL5_OVPGATE_STAT_SHIFT           7
#define SC8562_CTRL5_WPCGATE_STAT_MASK            BIT(6)
#define SC8562_CTRL5_WPCGATE_STAT_SHIFT           6
#define SC8562_CTRL5_VBUS_OVP_DIS_MASK            BIT(1)
#define SC8562_CTRL5_VBUS_OVP_DIS_SHIFT           1
#define SC8562_CTRL5_VOUT_OVP_DIS_MASK            BIT(0)
#define SC8562_CTRL5_VOUT_OVP_DIS_SHIFT           0

/* INT_STAT reg=0x10 */
#define SC8562_INT_STAT_REG                       0x10
#define SC8562_VOUT_OK_REV_STAT_MASK              BIT(5)
#define SC8562_VOUT_OK_REV_STAT_SHIFT             5
#define SC8562_VOUT_OK_CHG_STAT_MASK              BIT(4)
#define SC8562_VOUT_OK_CHG_STAT_SHIFT             4
#define SC8562_VOUT_INSERT_STAT_MASK              BIT(3)
#define SC8562_VOUT_INSERT_STAT_SHIFT             3
#define SC8562_VBUS_PRESENT_STAT_MASK             BIT(2)
#define SC8562_VBUS_PRESENT_STAT_SHIFT            2
#define SC8562_VWPC_INSERT_STAT_MASK              BIT(1)
#define SC8562_VWPC_INSERT_STAT_SHIFT             1
#define SC8562_VUSB_INSERT_STAT_MASK              BIT(0)
#define SC8562_VUSB_INSERT_STAT_SHIFT             0

/* INT_FLAG reg=0x11 */
#define SC8562_INT_FLAG_REG                       0x11
#define SC8562_VOUT_OK_REV_FLAG_MASK              BIT(5)
#define SC8562_VOUT_OK_REV_FLAG_SHIFT             5
#define SC8562_VOUT_OK_CHG_FLAG_MASK              BIT(4)
#define SC8562_VOUT_OK_CHG_FLAG_SHIFT             4
#define SC8562_VOUT_INSERT_FLAG_MASK              BIT(3)
#define SC8562_VOUT_INSERT_FLAG_SHIFT             3
#define SC8562_VBUS_PRESENT_FLAG_MASK             BIT(2)
#define SC8562_VBUS_PRESENT_FLAG_SHIFT            2
#define SC8562_VWPC_INSERT_FLAG_MASK              BIT(1)
#define SC8562_VWPC_INSERT_FLAG_SHIFT             1
#define SC8562_VUSB_INSERT_FLAG_MASK              BIT(0)
#define SC8562_VUSB_INSERT_FLAG_SHIFT             0

/* INT_MASK reg=0x12 */
#define SC8562_INT_MASK_REG                       0x12
#define SC8562_INT_MASK_REG_INIT                  0x3F
#define SC8562_VOUT_OK_REV_MASK_MASK              BIT(5)
#define SC8562_VOUT_OK_REV_MASK_SHIFT             5
#define SC8562_VOUT_OK_CHG_MASK_MASK              BIT(4)
#define SC8562_VOUT_OK_CHG_MASK_SHIFT             4
#define SC8562_VOUT_INSERT_MASK_MASK              BIT(3)
#define SC8562_VOUT_INSERT_MASK_SHIFT             3
#define SC8562_VBUS_PRESENT_MASK_MASK             BIT(2)
#define SC8562_VBUS_PRESENT_MASK_SHIFT            2
#define SC8562_VWPC_INSERT_MASK_MASK              BIT(1)
#define SC8562_VWPC_INSERT_MASK_SHIFT             1
#define SC8562_VUSB_INSERT_MASK_MASK              BIT(0)
#define SC8562_VUSB_INSERT_MASK_SHIFT             0

/* FLT_FLAG reg=0x13 */
#define SC8562_FLT_FLAG_REG                       0x13
#define SC8562_TSBAT_FLT_FLAG_MASK                BIT(7)
#define SC8562_TSBAT_FLT_FLAG_SHIFT               7
#define SC8562_TSHUT_FLAG_MASK                    BIT(6)
#define SC8562_TSHUT_FLAG_SHIFT                   6
#define SC8562_SS_TIMEOUT_FLAG_MASK               BIT(5)
#define SC8562_SS_TIMEOUT_FLAG_SHIFT              5
#define SC8562_WD_TIMEOUT_FLAG_MASK               BIT(4)
#define SC8562_WD_TIMEOUT_FLAG_SHIFT              4
#define SC8562_CONV_OCP_FLAG_MASK                 BIT(3)
#define SC8562_CONV_OCP_FLAG_SHIFT                3
#define SC8562_SS_FAIL_FLAG_MASK                  BIT(2)
#define SC8562_SS_FAIL_FLAG_SHIFT                 2
#define SC8562_VBUS_OVP_FLAG_MASK                 BIT(1)
#define SC8562_VBUS_OVP_FLAG_SHIFT                1
#define SC8562_VOUT_OVP_FLAG_MASK                 BIT(0)
#define SC8562_VOUT_OVP_FLAG_SHIFT                0

/* FLT_MASK reg=0x14 */
#define SC8562_FLT_MASK_REG                       0x14
#define SC8562_TSBAT_FLT_MASK_MASK                BIT(7)
#define SC8562_TSBAT_FLT_MASK_SHIFT               7
#define SC8562_TSHUT_MASK_MASK                    BIT(6)
#define SC8562_TSHUT_MASK_SHIFT                   6
#define SC8562_SS_TIMEOUT_MASK_MASK               BIT(5)
#define SC8562_SS_TIMEOUT_MASK_SHIFT              5
#define SC8562_WD_TIMEOUT_MASK_MASK               BIT(4)
#define SC8562_WD_TIMEOUT_MASK_SHIFT              4
#define SC8562_CONV_OCP_MASK_MASK                 BIT(3)
#define SC8562_CONV_OCP_MASK_SHIFT                3
#define SC8562_SS_FAIL_MASK_MASK                  BIT(2)
#define SC8562_SS_FAIL_MASK_SHIFT                 2
#define SC8562_VBUS_OVP_MASK_MASK                 BIT(1)
#define SC8562_VBUS_OVP_MASK_SHIFT                1
#define SC8562_VOUT_OVP_MASK_MASK                 BIT(0)
#define SC8562_VOUT_OVP_MASK_SHIFT                0

/* ADC_CTRL reg=0x15 */
#define SC8562_ADC_CTRL_REG                       0x15
#define SC8562_ADC_CTRL_ADC_EN_MASK               BIT(7)
#define SC8562_ADC_CTRL_ADC_EN_SHIFT              7
#define SC8562_ADC_CTRL_ADC_RATE_MASK             BIT(6)
#define SC8562_ADC_CTRL_ADC_RATE_SHIFT            6
#define SC8562_ADC_CTRL_ADC_DONE_STAT_MASK        BIT(5)
#define SC8562_ADC_CTRL_ADC_DONE_STAT_SHIFT       5
#define SC8562_ADC_CTRL_ADC_DONE_FLAG_MASK        BIT(4)
#define SC8562_ADC_CTRL_ADC_DONE_FLAG_SHIFT       4
#define SC8562_ADC_CTRL_ADC_DONE_MASK_MASK        BIT(3)
#define SC8562_ADC_CTRL_ADC_DONE_MASK_SHIFT       3
#define SC8562_ADC_CTRL_IBUS_ADC_DIS_MASK         BIT(0)
#define SC8562_ADC_CTRL_IBUS_ADC_DIS_SHIFT        0
#define SC8562_ADC_ENABLE                         1
#define SC8562_ADC_DISABLE                        0

/* ADC_FN_DISABLE1 reg=0x16 */
#define SC8562_ADC_FN_DISABLE1_REG                0x16
#define SC8562_VBUS_ADC_DIS_MASK                  BIT(7)
#define SC8562_VBUS_ADC_DIS_SHIFT                 7
#define SC8562_VUSB_ADC_DIS_MASK                  BIT(6)
#define SC8562_VUSB_ADC_DIS_SHIFT                 6
#define SC8562_VWPC_ADC_DIS_MASK                  BIT(5)
#define SC8562_VWPC_ADC_DIS_SHIFT                 5
#define SC8562_VOUT_ADC_DIS_MASK                  BIT(4)
#define SC8562_VOUT_ADC_DIS_SHIFT                 4
#define SC8562_VBAT_ADC_DIS_MASK                  BIT(3)
#define SC8562_VBAT_ADC_DIS_SHIFT                 3
#define SC8562_IBAT_ADC_DIS_MASK                  BIT(2)
#define SC8562_IBAT_ADC_DIS_SHIFT                 2
#define SC8562_TSBAT_ADC_DIS_MASK                 BIT(1)
#define SC8562_TSBAT_ADC_DIS_SHIFT                1
#define SC8562_TDIE_ADC_DIS_MASK                  BIT(0)
#define SC8562_TDIE_ADC_DIS_SHIFT                 0

/* IBUS_ADC1 reg=0x17 */
#define SC8562_IBUS_ADC1_REG                      0x17
#define SC8562_IBUS_ADC1_IBUS_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_IBUS_ADC1_IBUS_ADC_SHIFT           0

/* IBUS_ADC0 reg=0x18 */
#define SC8562_IBUS_ADC0_REG                      0x18
#define SC8562_IBUS_ADC0_IBUS_ADC_MASK            0xFF
#define SC8562_IBUS_ADC0_IBUS_ADC_SHIFT           0

/* VBUS_ADC1 reg=0x19 */
#define SC8562_VBUS_ADC1_REG                      0x19
#define SC8562_VBUS_ADC1_VBUS_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VBUS_ADC1_VBUS_ADC_SHIFT           0

/* VBUS_ADC0 reg=0x1A */
#define SC8562_VBUS_ADC0_REG                      0x1A
#define SC8562_VBUS_ADC0_VBUS_ADC_MASK            0xFF
#define SC8562_VBUS_ADC0_VBUS_ADC_SHIFT           0

/* VUSB_ADC1 reg=0x1B */
#define SC8562_VUSB_ADC1_REG                      0x1B
#define SC8562_VUSB_ADC1_VUSB_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VUSB_ADC1_VUSB_ADC_SHIFT           0

/* VUSB_ADC0 reg=0x1C */
#define SC8562_VUSB_ADC0_REG                      0x1C
#define SC8562_VUSB_ADC0_VUSB_ADC_MASK            0xFF
#define SC8562_VUSB_ADC0_VUSB_ADC_SHIFT           0

/* VWPC_ADC1 reg=0x1D */
#define SC8562_VWPC_ADC1_REG                      0x1D
#define SC8562_VWPC_ADC1_VWPC_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VWPC_ADC1_VWPC_ADC_SHIFT           0

/* VWPC_ADC0 reg=0x1E */
#define SC8562_VWPC_ADC0_REG                      0x1E
#define SC8562_VWPC_ADC0_VWPC_ADC_MASK            0xFF
#define SC8562_VWPC_ADC0_VWPC_ADC_SHIFT           0

/* VOUT_ADC1 reg=0x1F */
#define SC8562_VOUT_ADC1_REG                      0x1F
#define SC8562_VOUT_ADC1_VOUT_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VOUT_ADC1_VOUT_ADC_SHIFT           0

/* VOUT_ADC0 reg=0x20 */
#define SC8562_VOUT_ADC0_REG                      0x20
#define SC8562_VOUT_ADC0_VOUT_ADC_MASK            0xFF
#define SC8562_VOUT_ADC0_VOUT_ADC_SHIFT           0

/* VBAT_ADC1 reg=0x21 */
#define SC8562_VBAT_ADC1_REG                      0x21
#define SC8562_VBAT_ADC1_VBAT_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_VBAT_ADC1_VBAT_ADC_SHIFT           0

/* VBAT_ADC0 reg=0x22 */
#define SC8562_VBAT_ADC0_REG                      0x22
#define SC8562_VBAT_ADC0_VBAT_ADC_MASK            0xFF
#define SC8562_VBAT_ADC0_VBAT_ADC_SHIFT           0

/* IBAT_ADC1 reg=0x23 */
#define SC8562_IBAT_ADC1_REG                      0x23
#define SC8562_IBAT_ADC1_IBAT_ADC_MASK            (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_IBAT_ADC1_IBAT_ADC_SHIFT           0

/* IBAT_ADC0 reg=0x24 */
#define SC8562_IBAT_ADC0_REG                      0x24
#define SC8562_IBAT_ADC0_IBAT_ADC_MASK            0xFF
#define SC8562_IBAT_ADC0_IBAT_ADC_SHIFT           0

/* TSBAT_ADC1 reg=0x25 */
#define SC8562_TSBAT_ADC1_REG                     0x25
#define SC8562_TSBAT_ADC1_TSBAT_ADC_MASK          (BIT(1) | BIT(0))
#define SC8562_TSBAT_ADC1_TSBAT_ADC_SHIFT         0

/* TSBAT_ADC0 reg=0x26 */
#define SC8562_TSBAT_ADC0_REG                     0x26
#define SC8562_TSBAT_ADC0_TSBAT_ADC_MASK          0xFF
#define SC8562_TSBAT_ADC0_TSBAT_ADC_SHIFT         0

/* TDIE_ADC1 reg=0x27 */
#define SC8562_TDIE_ADC1_REG                      0x27
#define SC8562_TDIE_ADC1_TDIE_ADC_MASK            BIT(0)
#define SC8562_TDIE_ADC1_TDIE_ADC_SHIFT           0

/* TDIE_ADC0 reg=0x28 */
#define SC8562_TDIE_ADC0_REG                      0x28
#define SC8562_TDIE_ADC0_TDIE_ADC_MASK            0xFF
#define SC8562_TDIE_ADC0_TDIE_ADC_SHIFT           0

/* TSBAT_FLT reg=0x29 */
#define SC8562_TSBAT_FLT_REG                      0x29
#define SC8562_TSBAT_FLT_TH_MASK                  0xFF
#define SC8562_TSBAT_FLT_TH_SHIFT                 0

/* DEVICE_ID reg=0x6E */
#define SC8562_DEVICE_ID_REG                      0x6E
#define SC8562_DEVICE_ID_TH_MASK                  0xFF
#define SC8562_DEVICE_ID_TH_SHIFT                 0
#define SC8562_DEVICE_ID_SC8562                   0x61

/* FUN_DIS reg=0x70 */
#define SC8562_FUN_DIS_REG                        0x70
#define SC8562_TSBAT_FLT_DIS_MASK                 BIT(3)
#define SC8562_TSBAT_FLT_DIS_SHIFT                3
#define SC8562_TSBAT_FLT_DIS_DISABLE              1

struct sc8562_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	struct mutex scp_detect_lock;
	struct mutex accp_adapter_reg_lock;
	struct nty_data nty_data;
	struct dc_ic_ops lvc_ops;
	struct dc_ic_ops sc_ops;
	struct dc_ic_ops sc4_ops;
	struct dc_batinfo_ops batinfo_ops;
	struct power_log_ops log_ops;
	char name[CHIP_DEV_NAME_LEN];
	int gpio_int;
	int gpio_enable;
	int gpio_lpm;
	int irq_int;
	int irq_active;
	int chip_already_init;
	int device_id;
	u32 ic_role;
	int get_id_time;
	int init_finish_flag;
	int int_notify_enable_flag;
	int switching_frequency;
	int sense_r_actual;
	int sense_r_config;
	int dts_scp_support;
	int dts_fcp_support;
	u32 parallel_mode;
	u32 slave_mode;
	u8 charge_mode;
	u32 scp_error_flag;
	u8 scp_data[SC8562_SCP_MAX_DATA_LEN];
	u8 scp_op;
	u8 scp_op_num;
	bool scp_trans_done;
	bool fcp_support;
};

struct sc8562_dump_value {
	int vbat;
	int ibus;
	int vbus;
	int ibat;
	int vusb;
	int vout;
	int temp;
};

enum irq_flag {
	SC8562_IRQ_VBAT_OVP,
	SC8562_IRQ_FLT_FLAG,
	SC8562_IRQ_VUSB_OVP,
	SC8562_IRQ_IBUS_OCP,
	SC8562_IRQ_IBAT_OCP,
	SC8562_IRQ_SCP_FLAG,
	SC8562_IRQ_INT_FLAG,
	SC8562_IRQ_END,
};

int sc8562_config_vusb_ovp_th_mv(struct sc8562_device_info *di, int ovp_th);
int sc8562_config_vbus_ovp_th_mv(struct sc8562_device_info *di, int ovp_th, int mode);
int sc8562_get_vbus_mv(int *vbus, void *dev_data);

#endif /* _SC8562_H_ */
