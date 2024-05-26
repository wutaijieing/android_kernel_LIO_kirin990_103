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

#ifndef __HDMITX_CTRL_H__
#define __HDMITX_CTRL_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/io.h>

#include "hdmitx_ddc_config.h"
#include "hdmitx_hdr.h"
#include "hdmitx_connector.h"

#define HDMITX_DEFAULT_EDID_BUFLEN (128UL)
#define HDMI_SYSFS_ATTRS_NUM	(10)

#define LSB_MASK 0x00FF
#define MSB_MASK 0xFF00
#define SHIFT_8BIT 8

/* FRL_Rate define.see Table 10-21: SCDCS - Sink Configuration */
#define FRL_RATE_DISABLE 0
#define FRL_RATE_3G3L    1
#define FRL_RATE_6G3L    2
#define FRL_RATE_6G4L    3
#define FRL_RATE_8G4L    4
#define FRL_RATE_10G4L   5
#define FRL_RATE_12G4L   6
#define FRL_RATE_RSVD    7

enum hdmitx_hpd_states {
	HPD_OFF,
	HPD_ON
};

struct hdmitx_hw_config {
	u32 work_mode;

	bool tmds_scr_en;
	u32 tmds_clock;
	bool dvi_mode;

	u32 cur_frl_rate;

	u32 min_frl_rate;
	u32 max_frl_rate;
	u32 min_dsc_frl_rate;

	bool dsc_enable;
	u32 bpp_target;
	u32 slice_width;
	u32 slice_count;
	u32 hcactive;
	u32 hcblank;
};

struct hdmitx_soft_status {
	struct dpu_video_info info;
	struct hdmitx_hw_config cur_hw_config;
};

/**
 * struct hdmitx_ctrl - The representation of the hdmi TX core
 * @mutex:
 * @base: Base address of the registers
 * @irq: IRQ number
 * @version: Contents of the IP_VERSION register
 * @max_rate: The maximum rate that the controller supports
 * @max_lanes: The maximum lane count that the controller supports
 * @dev: The struct device
 * @vparams: The video params to use
 * @rx_caps: The sink's receiver capabilities.
 * @edid: The sink's EDID.

 * @multipixel: Controls multipixel configuration. 0-Single, 1-Dual, 2-Quad.
 */
struct hdmitx_ctrl {
	struct mutex hdmitx_mutex; /* generic mutex for hdmitx */

	void __iomem *base;
	void __iomem *aon_base;
	void __iomem *sysctrl_base;
	void __iomem *training_base;
	uint32_t irq;

	uint8_t mode;
	uint8_t id;

	uint32_t version;
	uint8_t multipixel;
	uint8_t max_rate;
	uint8_t max_lanes;
	uint8_t dsc_decoders;
	bool dsc;

	bool hdmitx_underflow_clear;
	bool hdmitx_enable;

	bool hdmitx_plug_type;
	bool hdmitx_detect_inited;
	bool video_transfer_enable;
	bool power_saving_mode;

	struct device *dev;
	struct dpu_connector *connector;

	// struct video_params vparams;
	struct hdr_metadata hdr_metadata;
	struct hdr_infoframe hdr_infoframe;

	struct hdmitx_frl *frl; /* to fix */
	struct hdmitx_ddc *ddc; /* to fix */
	struct hdmitx_connector *hdmitx_connector;
	struct hdmitx_soft_status soft_status;
	struct hdmitx_display_mode select_mode;

	int sysfs_index;
	struct attribute *sysfs_attrs[HDMI_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;
};


#define RIGHT_SHIFT_MAX 32

static inline u32 get_right_shift_cnt(u32 bit_mask)
{
	u32 i;

	for (i = 0; i < RIGHT_SHIFT_MAX; i++) {
		if (bit_mask & (1 << i))
			break;
	}

	return i;
}

static inline uint32_t hdmi_readl(void __iomem *base, uint32_t offset)
{
	uint32_t data = readl(base + offset);

	return data;
}

static inline void hdmi_writel(void __iomem *base, uint32_t offset, uint32_t data)
{
	writel(data, base + offset);
}

static inline u32 hdmi_read_bits(void *base, u32 offset, u32 bit_mask)
{
	u32 reg_val;
	u32 right_shift_cnt;

	right_shift_cnt = get_right_shift_cnt(bit_mask);
	reg_val = hdmi_readl(base, offset);
	return (reg_val & bit_mask) >> right_shift_cnt;
}

static inline void hdmi_write_bits(void *base, u32 offset, u32 bit_mask, u32 val)
{
	u32 shift_cnt;
	u32 reg_val;

	shift_cnt = get_right_shift_cnt(bit_mask);
	reg_val = hdmi_readl(base, offset);
	reg_val &= ~bit_mask;
	reg_val |= (val << shift_cnt) & bit_mask;
	hdmi_writel(base, offset, reg_val);
}

#define hdmi_clr(bs, offset, val) hdmi_writel((bs), (offset), hdmi_readl((bs), (offset)) & ~(val))


#define hdmi_clrset(bs, offset, mask, val) hdmi_writel((bs), (offset), (val) | (hdmi_readl((bs), (offset)) & ~(mask)))

#endif
