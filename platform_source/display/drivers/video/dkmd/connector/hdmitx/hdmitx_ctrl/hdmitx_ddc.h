/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#ifndef __HDMITX_DDC_H__
#define __HDMITX_DDC_H__

#include "hdmitx_ddc_config.h"

/* I2C Slave Address */
#define EDID_I2C_SALVE_ADDR 0xA0
#define HDCP_I2C_SALVE_ADDR 0x74
#define SCDC_I2C_SALVE_ADDR 0xA8

/* Status and Control Data Channel Structure */
#define SCDC_SINK_VERSION   0x01
#define SCDC_SOURCE_VERSION 0x02
#define SCDC_UPDATE_FLAG1   0x10
#define SCDC_UPDATE_FLAG2   0x11

#define SCDC_TMDS_CFG                   0x20
#define SCDC_TMDS_BIT_CLOCK_RATIO_BY_40 (1 << 1)
#define SCDC_TMDS_BIT_CLOCK_RATIO_BY_10 (0 << 1)
#define SCDC_SCRAMBLING_ENABLE          (1 << 0)

#define SCDC_TMDS_SRM_CFG      0x21
#define SCDC_SCRAMBLING_STATUS (1 << 0)

#define SCDC_SINK_CFG1    0x30
#define SCDC_SINK_CFG2    0x31
#define SCDC_SOU_TEST_CFG 0x35
#define SCDC_STATUS_FLAG1 0x40
#define SCDC_STATUS_FLAG2 0x41
#define SCDC_STATUS_FLAG3 0x42

s32 hdmitx_ddc_edid_read(struct hdmitx_ddc *ddc, u8 *buffer, u16 block, u16 size);
s32 hdmitx_ddc_scdc_read(struct hdmitx_ddc *ddc, u8 offset, u8 *buffer, u16 size);
s32 hdmitx_ddc_scdc_write(struct hdmitx_ddc *ddc, u8 offset, const void *buffer, u16 size);
s32 hdmitx_ddc_hdcp_read(struct hdmitx_ddc *ddc, u8 offset, u8 *buffer, u16 size);
s32 hdmitx_ddc_hdcp_write(struct hdmitx_ddc *ddc, u8 offset, const u8 *buffer, u16 size);

static inline s32 hdmitx_ddc_scdc_readb(struct hdmitx_ddc *ddc, u8 offset, u8 *value)
{
	return hdmitx_ddc_scdc_read(ddc, offset, value, sizeof(*value));
}

static inline s32 hdmitx_ddc_scdc_writeb(struct hdmitx_ddc *ddc, u8 offset, u8 value)
{
	return hdmitx_ddc_scdc_write(ddc, offset, &value, sizeof(value));
}
struct hdmitx_ddc *hdmitx_ddc_init(struct hdmitx_ctrl *hdmitx);

#endif
