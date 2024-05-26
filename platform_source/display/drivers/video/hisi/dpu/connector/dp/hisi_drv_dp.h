/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_DRV_DP_H
#define HISI_DRV_DP_H

#include "hisi_dp_core.h"
#include "link/drm_dp_helper_additions.h"
#include "hisi_dp_dbg.h"
#include "link/dp_aux.h"

#include "hisi_fb.h"
#include "hisi_disp_gadgets.h"
#include "hisi_connector_utils.h"
#include "vsync/hisi_disp_vactive.h"

#define CONFIG_DP_HDCP_ENABLE
#define CONFIG_DP_EDID_DEBUG

#define DP_PLUG_TYPE_NORMAL 0
#define DP_PLUG_TYPE_FLIPPED 1

#define DPTX_HDCP_REG_DPK_CRC_ORIG	0x331c1169
#define DPTX_HDCP_MAX_AUTH_RETRY	10
#define DPTX_HDCP_MAX_REPEATER_AUTH_RETRY 5

#define DPTX_AUX_TIMEOUT	(2000)

#define DEFAULT_AUXCLK_DPCTRL_RATE	16000000UL
#define DEFAULT_ACLK_DPCTRL_RATE_ES	288000000UL

#define PIXELS_PER_GROUP 3
#define PIXEL_HOLD_DELAY 5
#define PIXEL_GROUP_DELAY 3
#define MUXER_INITIAL_BUFFERING_DELAY 9
#define DSC_MAX_AUX_ORIG_WIDTH 24
#define PIXEL_FLATNESSBUF_DELAY DSC_MAX_AUX_ORIG_WIDTH
#define INITIAL_SCALE_VALUE_SHIFT 3
#define DSC_DEFAULT_DECODER 2

/* Rounding up to the nearest multiple of a number */
#define ROUND_UP_TO_NEAREST(numToRound, mult) (((numToRound+(mult)-1) / (mult)) * (mult))

#define DEFAULT_ACLK_DPCTRL_RATE 207500000UL
#define DEFAULT_MIDIA_PPLL7_CLOCK_FREQ 1290000000UL
#define KIRIN_VCO_MIN_FREQ_OUPUT         800000 /* dssv501: 800 * 1000 */
#define KIRIN_SYS_FREQ   38400 /* dssv501: 38.4 * 1000 */
#define MIDIA_PPLL7_CTRL0	0x838
#define MIDIA_PPLL7_CTRL1	0x83c

#define DPTX_CTRL_RESET_OFFSET 0x20
#define DPTX_CTRL_DIS_RESET_OFFSET 0x24
#define HIDPTX_CTRL_DIS_RESET_BIT BIT(16)
#define HIDPTX_CTRL_RESET_BIT BIT(16)
#define DPTX_CTRL_STATUS_OFFSET 0x28
#define HIDPTX_CTRL_RESET_STATUS_BIT BIT(16)

#define MAX_DIFF_SOURCE_X_SIZE	1920

#define MIDIA_PPLL7_FREQ_DEVIDER_MASK	GENMASK(25, 2)
#define MIDIA_PPLL7_FRAC_MODE_MASK	GENMASK(25, 0)

// TODO: defined in soc_dss_interface.h
#define PMCTRL_PERI_CTRL4_TEMPERATURE_MASK		GENMASK(27, 26)
#define PMCTRL_PERI_CTRL4_TEMPERATURE_SHIFT		26
#define NORMAL_TEMPRATURE 0

#define ACCESS_REGISTER_FN_MAIN_ID_HDCP           0xc500aa01
#define ACCESS_REGISTER_FN_SUB_ID_HDCP_CTRL   (0x55bbccf1)
#define ACCESS_REGISTER_FN_SUB_ID_HDCP_INT   (0x55bbccf2)

/* #define DPTX_DEVICE_INFO(pdev) platform_get_drvdata(pdev)->panel_info->dp */

#define DPTX_CHANNEL_NUM_MAX 8
#define DPTX_DATA_WIDTH_MAX 24

extern uint32_t g_fpga_flag;
extern uint32_t g_sdp_reg_num;
extern uint32_t g_pixel_clock;
extern uint32_t g_video_code;
extern uint32_t g_hblank;

int dp_on(struct dp_ctrl *dptx);
int dp_off(struct dp_ctrl *dptx);

bool dptx_check_low_temperature(struct dp_ctrl *dptx);
void dptx_notify(struct dp_ctrl *dptx);
void dptx_notify_shutdown(struct dp_ctrl *dptx);
void dp_send_cable_notification(struct dp_ctrl *dptx, int val);
int hisi_dptx_switch_source(uint32_t user_mode, uint32_t user_format);

int dptx_phy_rate_to_bw(uint8_t rate);
int dptx_bw_to_phy_rate(uint8_t bw);
void dptx_audio_params_reset(struct audio_params *aparams);
void dptx_video_params_reset(struct video_params *params);
void dwc_dptx_dtd_reset(struct dtd *mdtd);

static inline uint32_t dptx_readl(struct dp_ctrl *dp, uint32_t offset)
{
	/*lint -e529*/
	uint32_t data = readl(dp->base + offset);
	/*lint +e529*/
	return data;
}

static inline void dptx_writel(struct dp_ctrl *dp, uint32_t offset, uint32_t data)
{
	writel(data, dp->base + offset);
}

/*
 * Wait functions
 */
#define dptx_wait(_dptx, _cond, _timeout)				\
	({								\
		int __retval;						\
		__retval = wait_event_interruptible_timeout(		\
			_dptx->waitq,					\
			((_cond) || (atomic_read(&_dptx->shutdown))),	\
			msecs_to_jiffies(_timeout));			\
		if (atomic_read(&_dptx->shutdown)) {			\
			__retval = -ESHUTDOWN;				\
		}							\
		else if (!__retval) {					\
			__retval = -ETIMEDOUT;				\
		}							\
		__retval;						\
	})

#endif /* HISI_DRV_DP_H */
