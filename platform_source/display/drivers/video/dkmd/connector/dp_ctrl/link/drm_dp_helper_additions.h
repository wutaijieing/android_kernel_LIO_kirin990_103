/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DRM_DP_HELPER_ADDITIONS_H__
#define __DRM_DP_HELPER_ADDITIONS_H__
#include <linux/version.h>

/**
 * Training patterns and bandwidths for dp1.4
 */
#define DPTX_PHYIF_CTRL_TPS_NONE	0
#define DPTX_PHYIF_CTRL_TPS_1		1
#define DPTX_PHYIF_CTRL_TPS_2		2
#define DPTX_PHYIF_CTRL_TPS_3		3
#define DPTX_PHYIF_CTRL_TPS_4		4
#define DPTX_PHYIF_CTRL_RATE_RBR	0x0
#define DPTX_PHYIF_CTRL_RATE_HBR	0x1
#define DPTX_PHYIF_CTRL_RATE_HBR2	0x2
#define DPTX_PHYIF_CTRL_RATE_HBR3	0x3

#define FHD_TIMING_H_ACTIVE 1920
#define FHD_TIMING_V_ACTIVE 1080

/*
 * The following are cut from (dpu-dp)drm_dp_helper.h
 */
#define DP_ADJUST_POST_CCURSOR2		0x20c

#define DP_SYMBOL_ERROR_COUNT_LANE0_L 0x210
#define DP_SYMBOL_ERROR_COUNT_LANE0_H 0x211
#define DP_SYMBOL_ERROR_COUNT_LANE1_L 0x212
#define DP_SYMBOL_ERROR_COUNT_LANE1_H 0x213
#define DP_SYMBOL_ERROR_COUNT_LANE2_L 0x214
#define DP_SYMBOL_ERROR_COUNT_LANE2_H 0x215
#define DP_SYMBOL_ERROR_COUNT_LANE3_L 0x216
#define DP_SYMBOL_ERROR_COUNT_LANE3_H 0x217

#define DPTX_PHY_TEST_PATTERN_SEL_MASK	GENMASK(2, 0)

#define DPTX_NO_TEST_PATTERN_SEL		0
#define DPTX_D102_WITHOUT_SCRAMBLING	1
#define DPTX_SYMBOL_ERROR_MEASUREMENT_COUNT	2
#define DPTX_TEST_PATTERN_PRBS7		3
#define DPTX_80BIT_CUSTOM_PATTERN_TRANS		4
#define DPTX_CP2520_HBR2_COMPLIANCE_EYE_PATTERN	5
#define DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_2	6
#define DPTX_CP2520_HBR2_COMPLIANCE_PATTERN_3	7

#define DP_DPCD_REV_EXT 0x2200

/*
 * The following aren't yet defined in kernel headers
 */
#define DP_LINK_BW_8_1				0x1e
#define DP_TRAINING_PATTERN_4			7

#define DP_PSR2_WITH_Y_COORD_IS_SUPPORTED  3      /* eDP 1.4a */

#define DP_DSC_DEC_COLOR_FORMAT_CAP         0x069
#define DP_DSC_RGB                         (1 << 0)
#define DP_DSC_YCBCR_444                    (1 << 1)
#define DP_DSC_YCBCR_422_SIMPLE             (1 << 2)
#define DP_DSC_YCBCR_422_NATIVE             (1 << 3)
#define DP_DSC_YCBCR_420_NATIVE             (1 << 4)

#define DP_DSC_ENABLE            0x160   /* DP 1.4 */
#define DP_DSC_DECOMPRESSION_IS_ENABLED  (1 << 0)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#define DP_TEST_LINK_AUDIO_PATTERN		BIT(5)
#pragma clang diagnostic pop
#define DP_TEST_H_TOTAL_MSB                     0x222
#define DP_TEST_H_TOTAL_LSB			0x223
#define DP_TEST_V_TOTAL_MSB                     0x224
#define DP_TEST_V_TOTAL_LSB			0x225
#define DP_TEST_H_START_MSB			0x226
#define DP_TEST_H_START_LSB			0x227
#define DP_TEST_V_START_MSB			0x228
#define DP_TEST_V_START_LSB			0x229
#define DP_TEST_H_SYNC_WIDTH_MSB		0x22A
#define DP_TEST_H_SYNC_WIDTH_LSB		0x22B
#define DP_TEST_V_SYNC_WIDTH_MSB		0x22C
#define DP_TEST_V_SYNC_WIDTH_LSB		0x22D
#define DP_TEST_H_WIDTH_MSB			0x22E
#define DP_TEST_H_WIDTH_LSB			0x22F
#define DP_TEST_V_WIDTH_MSB			0x230
#define DP_TEST_V_WIDTH_LSB			0x231
#define DP_TEST_PHY_PATTERN			0x248
#define DP_TEST_PATTERN_NONE			0x0
#define DP_TEST_PATTERN_COLOR_RAMPS		0x1
#define DP_TEST_PATTERN_BW_VERITCAL_LINES	0x2
#define DP_TEST_PATTERN_COLOR_SQUARE		0x3

#define DP_TEST_80BIT_CUSTOM_PATTERN_0		0x250
#define DP_TEST_80BIT_CUSTOM_PATTERN_1		0x251
#define DP_TEST_80BIT_CUSTOM_PATTERN_2		0x252
#define DP_TEST_80BIT_CUSTOM_PATTERN_3		0x253
#define DP_TEST_80BIT_CUSTOM_PATTERN_4		0x254
#define DP_TEST_80BIT_CUSTOM_PATTERN_5		0x255
#define DP_TEST_80BIT_CUSTOM_PATTERN_6		0x256
#define DP_TEST_80BIT_CUSTOM_PATTERN_7		0x257
#define DP_TEST_80BIT_CUSTOM_PATTERN_8		0x258
#define DP_TEST_80BIT_CUSTOM_PATTERN_9		0x259

#define DP_TEST_PHY_PATTERN_SEL_MASK		GENMASK(2, 0)
#define DP_TEST_PHY_PATTERN_NONE		0x0
#define DP_TEST_PHY_PATTERN_D10			0x1
#define DP_TEST_PHY_PATTERN_SEMC		0x2
#define DP_TEST_PHY_PATTERN_PRBS7		0x3
#define DP_TEST_PHY_PATTERN_CUSTOM		0x4
#define DP_TEST_PHY_PATTERN_CP2520_1		0x5
#define DP_TEST_PHY_PATTERN_CP2520_2		0x6
#define DP_TEST_PHY_PATTERN_CP2520_3_TPS4	0x7

#define DP_TEST_DYNAMIC_RANGE_SHIFT             3
#define DP_TEST_DYNAMIC_RANGE_MASK		BIT(3)
#define DP_TEST_YCBCR_COEFF_SHIFT		4
#define DP_TEST_YCBCR_COEFF_MASK		BIT(4)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#define DP_TEST_DYNAMIC_RANGE_VESA		0x0
#pragma clang diagnostic pop

#define DP_TEST_COLOR_FORMAT_RGB	        0x0
#define DP_TEST_COLOR_FORMAT_YCBCR422           0x2
#define DP_TEST_COLOR_FORMAT_YCBCR444		0x4
#define DP_TEST_YCBCR_COEFF_ITU601		0x0
#define DP_TEST_YCBCR_COEFF_ITU709		0x1

#define DP_TEST_AUDIO_MODE			0x271
#define DP_TEST_AUDIO_SAMPLING_RATE_MASK GENMASK(3, 0)
#define DP_TEST_AUDIO_CH_COUNT_SHIFT 4
#define DP_TEST_AUDIO_CH_COUNT_MASK GENMASK(7, 4)

#define DP_TEST_AUDIO_SAMPLING_RATE_32		0x0
#define DP_TEST_AUDIO_SAMPLING_RATE_44_1	0x1
#define DP_TEST_AUDIO_SAMPLING_RATE_48		0x2
#define DP_TEST_AUDIO_SAMPLING_RATE_88_2	0x3
#define DP_TEST_AUDIO_SAMPLING_RATE_96		0x4
#define DP_TEST_AUDIO_SAMPLING_RATE_176_4	0x5
#define DP_TEST_AUDIO_SAMPLING_RATE_192		0x6

#define DP_TEST_AUDIO_CHANNEL1			0x0
#define DP_TEST_AUDIO_CHANNEL2			0x1
#define DP_TEST_AUDIO_CHANNEL3			0x2
#define DP_TEST_AUDIO_CHANNEL4			0x3
#define DP_TEST_AUDIO_CHANNEL5			0x4
#define DP_TEST_AUDIO_CHANNEL6			0x5
#define DP_TEST_AUDIO_CHANNEL7			0x6
#define DP_TEST_AUDIO_CHANNEL8			0x7

#define DP_EDP_14A                         0x04    /* eDP 1.4a */
#define DP_EDP_14B                         0x05    /* eDP 1.4b */

#define DP_DS_MAX_BPC_MASK                 (3 << 0)
#define DP_DS_8BPC                         0
#define DP_DS_10BPC                        1
#define DP_DS_12BPC                        2
#define DP_DS_16BPC                        3

#define DP_FEC_CONFIGURATION 0x120
#define DP_FEC_STATUS               0x280
#define DP_FEC_ERROR_COUNT    0x281
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#define DP_FEC_READY                 BIT(0)
#pragma clang diagnostic pop

#define DP_EXTENDED_RECEIVER_CAPABILITY_FIELD_PRESENT BIT(7)

#endif
