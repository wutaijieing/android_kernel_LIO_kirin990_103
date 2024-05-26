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

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#include <securec.h>
#include "dkmd_log.h"

#include "hdmitx_ctrl.h"
#include "hdmitx_phy.h"
#include "hdmitx_ddc.h"
#include "hdmitx_connector.h"
#include "hdmitx_edid.h"
#include "hdmitx_frl.h"
#include "hdmitx_avgen.h"
#include "hdmitx_core_config.h"
#include "hdmitx_infoframe.h"

/* default edid hdmi device */
static u8 g_edid_default_hdmi[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x52, 0x74, 0xd7, 0x0f, 0x00, 0x0e, 0x00, 0x01,
	0x06, 0x1d, 0x01, 0x03, 0x80, 0x60, 0x36, 0x78, 0x0a, 0xa8, 0x33, 0xab, 0x50, 0x45, 0xa5, 0x27,
	0x0d, 0x48, 0x48, 0x21, 0x08, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28,
	0x55, 0x40, 0x20, 0xc2, 0x31, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20,
	0x6e, 0x28, 0x55, 0x00, 0x20, 0xc2, 0x31, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x18,
	0x78, 0x0f, 0xff, 0x77, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
	0x00, 0x54, 0x53, 0x54, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xe6,
	0x02, 0x03, 0x47, 0xe0, 0x47, 0x03, 0x12, 0x13, 0x04, 0x21, 0x22, 0x10, 0x29, 0x09, 0x7f, 0x07,
	0x15, 0x17, 0x50, 0x57, 0x17, 0x00, 0x83, 0x01, 0x00, 0x00, 0xe2, 0x00, 0x4f, 0xe3, 0x05, 0xc3,
	0x01, 0x68, 0x03, 0x0c, 0x00, 0x40, 0x00, 0xb8, 0x3c, 0x08, 0x6a, 0xd8, 0x5d, 0xc4, 0x01, 0x3c,
	0x00, 0x00, 0x02, 0x00, 0x00, 0xe3, 0x06, 0x0d, 0x01, 0xe5, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xe1,
	0x0e, 0xe5, 0x01, 0x8b, 0x84, 0x90, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58,
	0x2c, 0x45, 0x00, 0x20, 0xc2, 0x31, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72
};

/* default edid dvi device */
static u8 g_edid_default_dvi[] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x10, 0xa6, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01,
	0x30, 0x16, 0x01, 0x03, 0x81, 0x00, 0x00, 0x78, 0x0a, 0xee, 0x9d, 0xa3, 0x54, 0x4c, 0x99, 0x26,
	0x0f, 0x47, 0x4a, 0x21, 0x08, 0x00, 0x41, 0x40, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28,
	0x55, 0x00, 0x20, 0xc2, 0x31, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20,
	0xb8, 0x28, 0x55, 0x40, 0x20, 0xc2, 0x31, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e
};

static void set_band_mode(u32 vic, u32 color_fmt, u32 color_dpth, struct band_mode *band)
{
	band->vic = vic;
	band->color_format = color_fmt;
	band->color_depth = color_dpth;
}

static void copy_valid_mode(struct hdmitx_valid_mode *dst,
	const struct hdmitx_valid_mode *src)
{
	struct list_head head = dst->head;

	*dst = *src;
	dst->head = head;
}

static s8 *get_color_format_name(u32 color_format)
{
	switch (color_format) {
	case RGB444:
		return "RGB444";
	case YCBCR422:
		return "YCBCR422";
	case YCBCR444:
		return "YCBCR444";
	case YCBCR420:
		return "YCBCR420";
	default:
		break;
	}

	return "UNKNOWN COLOR FORMAT";
}

static s8 *get_color_depth_name(u32 color_depth)
{
	switch (color_depth) {
	case CD_24:
		return "CD_24";
	case CD_30:
		return "CD_30";
	case CD_36:
		return "CD_36";
	case CD_48:
		return "CD_48";
	default:
		break;
	}

	return "UNKNOWN COLOR DEPTH";
}

static void print_valid_modes(const struct hdmitx_valid_mode *mode)
{
	struct hdmitx_timing_mode *timing_mode = NULL;
	s8 *color_format = NULL;
	s8 *color_depth = NULL;

	timing_mode = drv_hdmitx_modes_get_timing_mode_by_vic(mode->band.vic);

	color_format = get_color_format_name(mode->band.color_format);
	color_depth = get_color_depth_name(mode->band.color_depth);

	dpu_pr_debug("[%d] %s %s %s", mode->band.vic,
		(timing_mode != NULL) ? timing_mode->name : "DETAIL TIMING",
		color_format, color_depth);
}

static s32 connector_create_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_valid_mode *valid_mode)
{
	struct hdmitx_valid_mode *nmode = NULL;

	nmode = kmalloc(sizeof(struct hdmitx_valid_mode), GFP_KERNEL);
	if (nmode == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return -1;
	}

	if (memset_s(nmode, sizeof(*nmode), 0, sizeof(*nmode)) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		kfree(nmode);
		return -1;
	}

	valid_mode->valid = true;
	copy_valid_mode(nmode, valid_mode);
	print_valid_modes(valid_mode);

	list_add_tail(&nmode->head, &connector->valid_modes);

	return 0;
}

static void connector_destroy_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_valid_mode *valid_mode)
{
	list_del(&valid_mode->head);
	kfree(valid_mode);
}

static void connector_check_valid_mode_by_tmds(const struct hdmitx_connector *connector,
	struct hdmitx_valid_mode *valid_mode)
{
	const struct src_hdmitx *src = &connector->src_cap.hdmi;
	const struct hdmitx_display_info *info = &connector->display_info;

	/* tmds */
	if (valid_mode->tmds_clock < min(info->max_tmds_clock, src->max_tmds_clock)) {
		valid_mode->tmds_encode = true;

		if (valid_mode->tmds_clock > HDMITX14_MAX_TMDS_CLK) {
			if (connector->scdc.present && src->scdc_present) {
				valid_mode->hdmi_mode = HDMITX_MODE_20;
				valid_mode->tmds_scdc_en = true;
			} else {
				/* edid error */
				valid_mode->tmds_encode = false;
			}
		} else {
			valid_mode->hdmi_mode = (info->has_hdmi_infoframe && src->hdmi_support) ?
				HDMITX_MODE_14 : HDMITX_MODE_DVI;
			valid_mode->tmds_scdc_en = !!((valid_mode->hdmi_mode == HDMITX_MODE_14) &&
				src->scdc_lte_340mcsc && connector->scdc.lte_340mcsc);
		}
	} else {
		valid_mode->tmds_encode = false;
		valid_mode->tmds_scdc_en = false;
		valid_mode->hdmi_mode = HDMITX_MODE_21;
	}
}

static void connector_check_valid_mode_by_frl(const struct hdmitx_connector *connector,
	const struct hdmitx_display_mode *dis_mode, struct hdmitx_valid_mode *valid_mode)
{
	struct frl_requirements *frl_req = NULL;
	struct src_hdmitx src = connector->src_cap.hdmi;
	struct hdmitx_display_info info = connector->display_info;
	struct dsc_property dsc = connector->dsc;
	bool dsc_support = false;

	/* frl & dsc */
	if (connector->scdc.present && connector->display_info.max_frl_rate && src.max_frl_rate) {
		frl_req = drv_hdmitx_modes_get_frl_req_by_band(&valid_mode->band);
		if (frl_req != NULL) {
			valid_mode->frl.max_frl_rate = min(info.max_frl_rate, src.max_frl_rate);
			valid_mode->frl.min_frl_rate = frl_req->min_frl_rate;
			valid_mode->frl.frl_uncompress = !!(frl_req->frl_uncompress &&
				frl_req->min_frl_rate <= valid_mode->frl.max_frl_rate);
			valid_mode->frl.ffe_levels = src.ffe_levels;
			//  to fix:
			//  dsc_support ,connector_dsc_check(...)

			valid_mode->dsc.frl_compress = (frl_req->frl_compress && dsc_support);

			if (dsc_support) {
				valid_mode->dsc.max_dsc_frl_rate = min(dsc.dsc_max_rate, src.max_frl_rate);
				valid_mode->dsc.min_dsc_frl_rate = frl_req->min_dsc_frl_rate;
				valid_mode->dsc.hcactive = frl_req->dsc_hcactive;
				valid_mode->dsc.hcblank = frl_req->dsc_hcblank;
			}
		}
	}
}

static bool connector_check_valid_mode(struct hdmitx_connector *connector,
	const struct hdmitx_display_mode *dis_mode, struct hdmitx_valid_mode *valid_mode)
{
	struct src_hdmitx *src = &connector->src_cap.hdmi;
	valid_mode->tmds_encode = false;
	valid_mode->frl.frl_uncompress = false;
	valid_mode->dsc.frl_compress = false;

	connector_check_valid_mode_by_tmds(connector, valid_mode);
	connector_check_valid_mode_by_frl(connector, dis_mode, valid_mode);

	if (dis_mode->detail.pixel_clock > src->max_pixel_clock) {
		dpu_pr_err("now pixel clock %d, max is %d\n", dis_mode->detail.pixel_clock, src->max_pixel_clock);
		return false;
	}

	if ((!valid_mode->tmds_encode) &&
		(!valid_mode->frl.frl_uncompress) &&
		(!valid_mode->dsc.frl_compress))
		return false;

	if (connector_create_valid_mode(connector, valid_mode)) {
		dpu_pr_err("create valid mode fail!\n");
		return false;
	}

	return true;
}

static void connector_check_valid_mode_set_color(struct hdmitx_connector *connector,
	struct hdmitx_display_mode *dis_mode, struct hdmitx_valid_mode *valid_mode, u32 color_cap)
{
	if (connector_check_valid_mode(connector, dis_mode, valid_mode)) {
		dis_mode->color_cap |= color_cap;
		dis_mode->status = MODE_OK;
	}
}

static void connector_add_420_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_display_mode *dis_mode)
{
	bool flag = false;
	bool hdmi_support = false;
	struct hdmitx_valid_mode valid_mode;
	struct color_depth *depth = &connector->color.depth;
	struct src_hdmitx *src = &connector->src_cap.hdmi;
	struct hdmi_detail *detail = &dis_mode->detail;

	if (memset_s(&valid_mode, sizeof(valid_mode), 0x0, sizeof(valid_mode)) != EOK) {
		dpu_pr_err("memset_s failed!\n");
		return;
	}

	hdmi_support = connector->display_info.has_hdmi_infoframe && connector->src_cap.hdmi.hdmi_support;

	dis_mode->color_cap &= ~COLOR_Y420_24;
	flag = hdmi_support && depth->y420_24 && src->ycbcr420;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR420, CD_24, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock / 2; /* 1/2 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y420_24);
	}

	dis_mode->color_cap &= ~COLOR_Y420_30;
	flag = hdmi_support && depth->y420_30 && src->ycbcr420 && src->bpc_30;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR420, CD_30, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock * 5 / 8; /* 5/8: 0.625 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y420_30);
	}

	dis_mode->color_cap &= ~COLOR_Y420_36;
	flag = hdmi_support && depth->y420_36 && src->ycbcr420 && src->bpc_36;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR420, CD_36, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock * 3 / 4; /* 3/4: 0.75 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y420_36);
	}

	dis_mode->color_cap &= ~COLOR_Y420_48;
	flag = hdmi_support && depth->y420_48 && src->ycbcr420 && src->bpc_48;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR420, CD_48, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock;
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y420_48);
	}
}

static void connector_add_rgb_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_display_mode *dis_mode)
{
	struct hdmitx_valid_mode valid_mode;
	struct color_depth *depth = &connector->color.depth;
	struct src_hdmitx *src = &connector->src_cap.hdmi;
	struct hdmi_detail *detail = &dis_mode->detail;
	bool hdmi_support = false;
	bool flag = false;

	if (memset_s(&valid_mode, sizeof(valid_mode), 0x0, sizeof(struct hdmitx_valid_mode)) != EOK) {
		dpu_pr_err("memset_s failed!\n");
		return;
	}

	hdmi_support = connector->display_info.has_hdmi_infoframe && connector->src_cap.hdmi.hdmi_support;

	dis_mode->color_cap &= ~COLOR_RGB_24;
	set_band_mode(dis_mode->vic, RGB444, CD_24, &valid_mode.band);
	valid_mode.tmds_clock = detail->pixel_clock;
	connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_RGB_24);

	dis_mode->color_cap &= ~COLOR_RGB_30;
	flag = hdmi_support && depth->rgb_30 && src->bpc_30;
	if (flag) {
		set_band_mode(dis_mode->vic, RGB444, CD_30, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock * 5 / 4; /* 5/4: 1.25 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_RGB_30);
	}

	dis_mode->color_cap &= ~COLOR_RGB_36;
	flag = hdmi_support && depth->rgb_36 && src->bpc_36;
	if (flag) {
		set_band_mode(dis_mode->vic, RGB444, CD_36, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock * 3 / 2; /* 3/2: 1.5 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_RGB_36);
	}

	dis_mode->color_cap &= ~COLOR_RGB_48;
	flag = hdmi_support && depth->rgb_48 && src->bpc_48;
	if (flag) {
		set_band_mode(dis_mode->vic, RGB444, CD_48, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock << 1; /* 2 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_RGB_48);
	}
}

static void connector_add_444_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_display_mode *dis_mode)
{
	struct hdmitx_valid_mode valid_mode;
	struct color_depth *depth = &connector->color.depth;
	struct src_hdmitx *src = &connector->src_cap.hdmi;
	struct hdmi_detail *detail = &dis_mode->detail;
	bool hdmi_support = false;
	bool flag = false;

	if (memset_s(&valid_mode, sizeof(valid_mode), 0x0, sizeof(valid_mode)) != EOK) {
		dpu_pr_err("memset_s failed!\n");
		return;
	}

	hdmi_support = connector->display_info.has_hdmi_infoframe &&
				   connector->src_cap.hdmi.hdmi_support;

	dis_mode->color_cap &= ~COLOR_Y444_24;
	flag = hdmi_support && depth->y444_24 && src->ycbcr444;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR444, CD_24, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock;
		if (connector_check_valid_mode(connector, dis_mode, &valid_mode))
			dis_mode->color_cap |= COLOR_Y444_24;
	}

	dis_mode->color_cap &= ~COLOR_Y444_30;
	flag = hdmi_support && depth->y444_30 && src->bpc_30 && src->ycbcr444;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR444, CD_30, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock * 5 / 4; /* 5/4: 1.25 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y444_30);
	}

	dis_mode->color_cap &= ~COLOR_Y444_36;
	flag = hdmi_support && depth->y444_36 && src->bpc_36 && src->ycbcr444;
	if (flag) {
		set_band_mode(dis_mode->vic, YCBCR444, CD_36, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock * 3 / 2; /* 3/2: 1.5 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y444_36);
	}

	dis_mode->color_cap &= ~COLOR_Y444_48;
	if (hdmi_support && depth->y444_48 && src->bpc_48 && src->ycbcr444) {
		set_band_mode(dis_mode->vic, YCBCR444, CD_48, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock << 1; /* 2 times */
		connector_check_valid_mode_set_color(connector, dis_mode, &valid_mode, COLOR_Y444_48);
	}
}

static void connector_add_422_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_display_mode *dis_mode)
{
	struct hdmitx_valid_mode valid_mode;
	struct src_hdmitx *src = &connector->src_cap.hdmi;
	struct hdmi_detail *detail = &dis_mode->detail;
	bool hdmi_support = false;

	if (memset_s(&valid_mode, sizeof(valid_mode), 0x0, sizeof(valid_mode)) != EOK) {
		dpu_pr_err("memset_s failed!\n");
		return;
	}

	hdmi_support = connector->display_info.has_hdmi_infoframe &&
				   connector->src_cap.hdmi.hdmi_support;

	dis_mode->color_cap &= ~COLOR_Y422;
	if (hdmi_support && connector->color.format.ycbcr422 && src->ycbcr422) {
		set_band_mode(dis_mode->vic, YCBCR422, CD_24, &valid_mode.band);
		valid_mode.tmds_clock = detail->pixel_clock;
		if (connector_check_valid_mode(connector, dis_mode, &valid_mode)) {
			dis_mode->color_cap |= COLOR_Y422;
			dis_mode->status = MODE_OK;
		}
	}
}

static void connector_add_n420_valid_mode(struct hdmitx_connector *connector,
	struct hdmitx_display_mode *dis_mode)
{
	connector_add_rgb_valid_mode(connector, dis_mode);
	connector_add_444_valid_mode(connector, dis_mode);
	connector_add_422_valid_mode(connector, dis_mode);
}

static s32 connector_add_validmodes(struct hdmitx_connector *connector)
{
	struct list_head *n = NULL;
	struct list_head *pos = NULL;
	struct hdmitx_display_mode *dis_mode = NULL;

	if (connector == NULL)
		return -1;

	if (list_empty(&connector->probed_modes))
		return 0;

	list_for_each_safe(pos, n, &connector->probed_modes) {
		dis_mode = list_entry(pos, struct hdmitx_display_mode, head);
		if (dis_mode == NULL)
			continue;

		if ((dis_mode->parse_type & MODE_TYPE_Y420VDB) != MODE_TYPE_Y420VDB)
			connector_add_n420_valid_mode(connector, dis_mode);

		if ((dis_mode->parse_type & MODE_TYPE_Y420VDB) ||
			(dis_mode->parse_type & MODE_TYPE_VDB_Y420CMDB))
			connector_add_420_valid_mode(connector, dis_mode);
	}

	return 0;
}

static s32 connector_destroy_modes(struct hdmitx_connector *connector)
{
	struct list_head *n = NULL;
	struct list_head *p = NULL;
	struct hdmitx_valid_mode *valid_mode = NULL;
	struct hdmitx_display_mode *dis_mode = NULL;

	if (!list_empty(&connector->valid_modes)) {
		list_for_each_safe(p, n, &connector->valid_modes) {
			valid_mode = list_entry(p, struct hdmitx_valid_mode, head);
			if (valid_mode != NULL) {
				connector_destroy_valid_mode(connector, valid_mode);
				valid_mode = NULL;
			}
		}
	}

	if (!list_empty(&connector->probed_modes)) {
		list_for_each_safe(p, n, &connector->probed_modes) {
			dis_mode = list_entry(p, struct hdmitx_display_mode, head);
			if (dis_mode != NULL) {
				list_del(&dis_mode->head);
				drv_hdmitx_modes_destroy_mode(dis_mode);
			}
		}
	}
	connector->probed_mode_cnt = 0;

	return 0;
}

static s32 connector_edid_info_reset(struct hdmitx_connector *connector)
{
	if (memset_s(&connector->audio, sizeof(connector->audio), 0, sizeof(connector->audio)) != EOK)
		goto err_memset;

	if (memset_s(&connector->base, sizeof(connector->base), 0, sizeof(connector->base)) != EOK)
		goto err_memset;

	if (memset_s(&connector->color, sizeof(connector->color), 0, sizeof(connector->color)) != EOK)
		goto err_memset;

	if (memset_s(&connector->dolby, sizeof(connector->dolby), 0, sizeof(connector->dolby)) != EOK)
		goto err_memset;

	if (memset_s(&connector->cuva, sizeof(connector->cuva), 0, sizeof(connector->cuva)) != EOK)
		goto err_memset;

	if (memset_s(&connector->dsc, sizeof(connector->dsc), 0, sizeof(connector->dsc)) != EOK)
		goto err_memset;

	if (memset_s(&connector->hdr, sizeof(connector->hdr), 0, sizeof(connector->hdr)) != EOK)
		goto err_memset;

	if (memset_s(&connector->latency, sizeof(connector->latency), 0, sizeof(connector->latency)) != EOK)
		goto err_memset;

	if (memset_s(&connector->scdc, sizeof(connector->scdc), 0, sizeof(connector->scdc)) != EOK)
		goto err_memset;

	if (memset_s(&connector->timing, sizeof(connector->timing), 0, sizeof(connector->timing)) != EOK)
		goto err_memset;

	if (memset_s(&connector->vrr, sizeof(connector->vrr), 0, sizeof(connector->vrr)) != EOK)
		goto err_memset;

	if (memset_s(&connector->display_info, sizeof(connector->display_info),
				 0, sizeof(connector->display_info)) != EOK)
		goto err_memset;

	return 0;
err_memset:
	dpu_pr_err("memset failed!\n");
	return -1;
}

static void connector_edid_reset(struct hdmitx_connector *connector)
{
	mutex_lock(&connector->mutex);
	if (connector->edid_src_type == EDID_FROM_SINK && connector->edid_raw)
		kfree(connector->edid_raw);

	connector->edid_raw = NULL;
	connector->edid_size = 0;
	connector->detail_vic_base = VIC_DETAIL_TIMING_BASE;

	if (connector_edid_info_reset(connector))
		dpu_pr_err("edid info reset failed!\n");

	connector->edid_src_type = EDID_EMPTY;

	if (connector_destroy_modes(connector))
		dpu_pr_err("destroy mode fail!\n");

	mutex_unlock(&connector->mutex);
}

static void connector_set_edid_info(struct hdmitx_connector *connector,
	const u8 *edid, u32 size, s32 type)
{
	mutex_lock(&connector->mutex);
	connector->edid_size = size;
	connector->edid_raw = (struct edid *)edid;
	connector->edid_src_type = type;
	/* add modes */
	if (!drv_hdmitx_edid_add_modes(connector, connector->edid_raw))
		dpu_pr_err("no mode!\n");

	if (connector_add_validmodes(connector))
		dpu_pr_err("add valid modes fail.\n");
	mutex_unlock(&connector->mutex);
}

static void connector_set_default_edid(struct hdmitx_connector *connector)
{
	if (!connector->src_cap.hdmi.hdmi_support || !connector->user_dvi_mode) {
		dpu_pr_info("force dvi mode edid.user_dvi_mode=%d.\n", connector->user_dvi_mode);
		connector_set_edid_info(connector, g_edid_default_dvi, sizeof(g_edid_default_dvi), EDID_DEFAULT_DVI);
	} else {
		dpu_pr_info("force hdmi mode edid.user_dvi_mode=%d.\n", connector->user_dvi_mode);
		connector_set_edid_info(connector, g_edid_default_hdmi, sizeof(g_edid_default_hdmi), EDID_DEFAULT_HDMITX);
	}
}

static u8 *connector_get_edid_base_block(struct hdmitx_ddc *ddc)
{
	u32 i;
	u8 *edid = NULL;

	edid = kmalloc(EDID_LENGTH, GFP_KERNEL);
	if (edid == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return NULL;
	}

	if (memset_s(edid, EDID_LENGTH, 0, EDID_LENGTH) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		kfree(edid);
		return NULL;
	}

	/* base block fetch */
	for (i = 0; i < 4; i++) { /* max retry 4 times. */
		if (hdmitx_ddc_edid_read(ddc, edid, 0, EDID_LENGTH) != 0) {
			kfree(edid);
			return NULL;
		}
		/* &connector->edid_corrupt */
		if (drv_hdmitx_edid_block_is_valid(edid, 0, false))
			break;

		if (i == 0 && drv_hdmitx_edid_data_is_zero(edid, EDID_LENGTH)) {
			kfree(edid);
			return NULL;
		}
	}

	if (i == 4) { /* retry 4 times. */
		kfree(edid);
		return NULL;
	}

	return edid;
}

static u8 *connector_get_edid_ext_block(struct hdmitx_ddc *ddc, u8 *base_edid, u32 *valid_extensions)
{
	u32 i, j;
	u32 extensions_num = *valid_extensions;
	u8 *edid = NULL;
	u8 *block = NULL;

	edid = kmalloc((extensions_num + 1) * EDID_LENGTH, GFP_KERNEL);
	if (edid == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return NULL;
	}

	if (memset_s(edid, (extensions_num + 1) * EDID_LENGTH, 0, (extensions_num + 1) * EDID_LENGTH) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		kfree(edid);
		return NULL;
	}

	if (memcpy_s(edid, (extensions_num + 1) * EDID_LENGTH, base_edid, EDID_LENGTH) != EOK) {
		kfree(edid);
		dpu_pr_err("memcpy_s failed!\n");
		return NULL;
	}

	for (j = 1; j <= extensions_num; j++) {
		block = edid + j * EDID_LENGTH;
		for (i = 0; i < 4; i++) { /* max retry 4 times. */
			if (hdmitx_ddc_edid_read(ddc, block, j, EDID_LENGTH) != 0) {
				kfree(edid);
				return NULL;
			}

			if (drv_hdmitx_edid_block_is_valid(block, j, false))
				break;
		}

		if (i == 4) /* retry 4 times. */
			extensions_num--;
	}

	*valid_extensions = extensions_num;
	return edid;
}

static u8 *connector_get_fixed_edid(u8 *raw_edid, u32 valid_extensions)
{
	u32 i;
	u8 *edid = NULL;
	u8 *block = NULL;
	u8 *base = NULL;

	raw_edid[EDID_LENGTH - 1] += raw_edid[0x7e] - valid_extensions;
	raw_edid[0x7e] = valid_extensions;

	edid = kmalloc((valid_extensions + 1) * EDID_LENGTH, GFP_KERNEL);
	if (edid == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return NULL;
	}

	if (memset_s(edid, (valid_extensions + 1) * EDID_LENGTH, 0, (valid_extensions + 1) * EDID_LENGTH) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		kfree(edid);
		return NULL;
	}

	base = edid;
	for (i = 0; i <= valid_extensions; i++) {
		block = raw_edid + i * EDID_LENGTH;
		if (!drv_hdmitx_edid_block_is_valid(block, i, false))
			continue;

		if (memcpy_s(base, (valid_extensions + 1 - i) * EDID_LENGTH, block, EDID_LENGTH) != EOK) {
			kfree(edid);
			return NULL;
		}
		base += EDID_LENGTH;
	}

	return edid;
}

static struct edid *hdmitx_connector_get_edid(struct hdmitx_connector *connector, struct hdmitx_ddc *ddc)
{
	u32 size;
	u32 valid_extensions;
	u8 *edid = NULL;
	u8 *new_edid = NULL;
	u64 temp;

	edid = connector_get_edid_base_block(ddc);
	if (edid == NULL) {
		dpu_pr_err("get base block fail.\n");
		goto out;
	}

	/* if there's no extensions, we're done */
	valid_extensions = edid[0x7e];
	size = EDID_LENGTH;
	if (valid_extensions == 0) {
		connector_set_edid_info(connector, edid, size, EDID_FROM_SINK);
		return (struct edid *)edid;
	}

	new_edid = connector_get_edid_ext_block(ddc, edid, &valid_extensions);
	if (new_edid == NULL) {
		dpu_pr_err("get ext blocks fail.\n");
		goto free;
	}
	kfree(edid);
	edid = new_edid;

	if (valid_extensions != edid[0x7e]) {
		new_edid = connector_get_fixed_edid(edid, valid_extensions);
		if (new_edid == NULL) {
			dpu_pr_err("get fixed edid fail.\n");
			goto free;
		}
		kfree(edid);
		edid = new_edid;
	}
	temp = (valid_extensions + 1) * EDID_LENGTH;
	size = (u32)temp;

	connector_set_edid_info(connector, edid, size, EDID_FROM_SINK);

	return (struct edid *)edid;

free:
	kfree(edid);
out:
	mutex_lock(&connector->mutex);
	connector->edid_size = 0;
	mutex_unlock(&connector->mutex);
	return NULL;
}


bool drv_hdmitx_connector_is_scdc_present(struct hdmitx_ctrl *hdmitx)
{
	if (hdmitx == NULL) {
		dpu_pr_err("ptr is null!\n");
		return false;
	}

	if (!hdmitx->hdmitx_enable) {
		dpu_pr_info("scdc unkonw, no plug in!\n");
		return false;
	} else {
		return hdmitx->hdmitx_connector->scdc.present;
	}
}

int hdmitx_connector_init(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_connector *connector = NULL;

	connector = kmalloc(sizeof(struct hdmitx_connector), GFP_KERNEL);
	if (connector == NULL) {
		dpu_pr_err("kmalloc fail.\n");
		return -1;
	}

	if (memset_s(connector, sizeof(*connector), 0, sizeof(*connector)) != EOK) {
		dpu_pr_err("memset_s fail.\n");
		return -1;
	}

	connector->detail_vic_base = VIC_DETAIL_TIMING_BASE;

	mutex_init(&connector->mutex);
	INIT_LIST_HEAD(&connector->valid_modes);
	INIT_LIST_HEAD(&connector->probed_modes);
	connector->name = "hdmitx-connector0";

	//  connector_source_capability_init
	//  connector_use_default_compat_info

	hdmitx->hdmitx_connector = connector;
	return 0;
}

static void hdmitx_subsys_init(struct hdmitx_ctrl *hdmitx)
{
	core_enable_memory(hdmitx->sysctrl_base);
	// dis reset pwd
	core_reset_pwd(hdmitx->base, false);
	// set hdmi mode
	hdmitx_set_mode(true);
	// audo CTS
	core_set_audio_acr(hdmitx->base);
}

int hdmitx_handle_hotplug(struct hdmitx_ctrl *hdmitx)
{
	// subsys init
	hdmitx_subsys_init(hdmitx);

	// FPGA phy init
	hdmitx_phy_init(hdmitx);

	// get edid and add modes
	hdmitx_connector_get_edid(hdmitx->hdmitx_connector, hdmitx->ddc);

	hdmitx_timing_config_safe_mode(hdmitx);

	// start frl link training
	hdmitx_frl_start(hdmitx);

	// choose timing by kernel and config timing
	hdmitx_timing_config(hdmitx);

	// init infoframe from status
	hdmitx_init_infoframe(hdmitx);

	hdmitx_frl_video_config(hdmitx);
	return 0;
}

int hdmitx_handle_unhotplug(struct hdmitx_ctrl *hdmitx)
{
	core_disable_memory(hdmitx->sysctrl_base);
	// pwd reset
	core_reset_pwd(hdmitx->base, true);
	return 0;
}

