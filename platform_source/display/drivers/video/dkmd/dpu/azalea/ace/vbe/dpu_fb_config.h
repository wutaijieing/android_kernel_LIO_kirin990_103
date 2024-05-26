/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#ifndef DPU_FB_CONFIG_H
#define DPU_FB_CONFIG_H

#define SUPPORT_HEBC_ROT_CAP

#define SUPPORT_SPR_DSC1_2_SEPARATE

/* mipi_dsi_bit_clk_update wait count, unit is ms */
#define MIPI_CLK_UPDT_TIMEOUT 30
/* esd check wait for mipi_resource_available count, unit is ms */
#define ESD_WAIT_MIPI_AVAILABLE_TIMEOUT 64

#define LOW_TEMP_VOLTAGE_LIMIT

/* after 990, if dsc configure to single pipe, pic_width must
 * consistent with DDIC,when dsc configure to double pipe,pic_width
 * must be half the size of the panel
 */
#define SUPPORT_DSC_PIC_WIDTH

/* after 990, supports two dpp channels */
#define SUPPORT_DISP_CHANNEL


#define SUPPORT_DSS_BUS_IDLE_CTRL

#define SUPPORT_FULL_CHN_10BIT_COLOR

/* after 990, supports copybit vote(core clock,voltage) */
#define SUPPORT_COPYBIT_VOTE_CTRL

/* after 990, dump disp dbuf dsi info */
#define SUPPORT_DUMP_REGISTER_INFO

#define SUPPORT_DSS_HEBCE
#define SUPPORT_DSS_HEBCD

#define SUPPORT_DSS_HFBCD

#define SUPPORT_DSS_HFBCE

#define SUPPORT_DSS_AFBC_ENHANCE

#define ESD_RESTORE_DSS_VOLTAGE_CLK

/* low power cmds trans under video mode */
#define MIPI_DSI_HOST_VID_LP_MODE

/* after 501, mmbuf's clock is configurable */
#define SUPPORT_SET_MMBUF_CLK

/* after 970, no need to manually clean cmdlist */
#define SUPPORT_CLR_CMDLIST_BY_HARDWARE

#define SUPPORT_DP_VER_1_4_LATER

#define SUPPORT_DP_DSC

#define ARSR_POST_NEED_PADDING
#endif /* DPU_FB_CONFIG_H */

