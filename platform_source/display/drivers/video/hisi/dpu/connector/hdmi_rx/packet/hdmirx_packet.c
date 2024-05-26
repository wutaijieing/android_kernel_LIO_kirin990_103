/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hdmirx_packet.h"
#include <hdmirx_common.h>
#include <securec.h>
#include "video/hdmirx_video_hdr.h"

static hdmirx_packet_context g_hdmirx_packet_ctx;

static const uint32_t g_packet_base_addr[HDMIRX_PACKET_MAX] = {
	REG_GCPRX_WORD0,
	REG_ACPRX_WORD0,
	REG_ISRC1RX_WORD0,
	REG_ISRC2RX_WORD0,
	REG_GMPRX_WORD0,
	REG_VSIRX_WORD0,
	REG_HF_VSI_3DRX_WORD0,
	REG_HF_VSI_HDRRX_WORD0,
	REG_AVIRX_WORD0,
	REG_SPDRX_WORD0,
	REG_AIFRX_WORD0,
	REG_MPEGRX_WORD0,
	REG_HDRRX_WORD0,
	REG_AUD_METARX_WORD0,
	REG_UNRECRX_WORD0,
	REG_FVAVRRRX_WORD0
};

#define HMDIRX_PACKET_INVALID_VALUE 255
uint32_t g_bit_width = HMDIRX_PACKET_INVALID_VALUE;
uint32_t g_color_space = HMDIRX_PACKET_INVALID_VALUE;

void hdmirx_set_cd_and_bpp(uint32_t color_space, uint32_t bit_width)
{
	g_color_space = color_space;
	g_bit_width = bit_width;
}
EXPORT_SYMBOL_GPL(hdmirx_set_cd_and_bpp);

static hdmirx_packet_context *hdmirx_pakcet_get_ctx(void)
{
	return &g_hdmirx_packet_ctx;
}

static bool hdmirx_get_packet_content_check(uint32_t mod_val, uint32_t offset, uint32_t div_val, uint8_t reg_num)
{
	return ((mod_val >= reg_num) && (offset == div_val));
}

static int hdmirx_get_empty_packet_content(struct hdmirx_ctrl_st *hdmirx,
														hdmirx_packet_type type,
														uint8_t *data,
														uint32_t max_len)
{
	uint32_t len = 31; /* 31 : Maximum length of a packet */
	uint32_t value[8] = {0}; /* 8 register */
	uint8_t raw[32] = {0}; /* 32 register */
	uint32_t reg_addr;
	uint32_t first_byte;
	uint32_t second_byte;
	uint32_t third_byte;
	uint32_t forth_byte;
	uint32_t offset;
	errno_t err_ret;

	if (type != HDMIRX_PACKET_FVAVRR)
		return 0;

	if (len > max_len)
		len = max_len;

	reg_addr = g_packet_base_addr[type];
	hdmirx_reg_read_block(hdmirx->hdmirx_pwd_base, reg_addr, value, 8); /* 8 register */
	for (offset = 0; offset < 8; offset++) { /* 8 register */
		first_byte = value[offset] & 0xFF;
		raw[offset * 4 + 0] = first_byte; /* 4/0 Register offset first byte */
		second_byte = (value[offset] & 0xFF00) >> 8; /* Shift right 8 bit */
		raw[offset * 4 + 1] = second_byte; /* 4/1 Register offset second byte */
		third_byte = (value[offset] & 0xFF0000) >> 16; /* Shift right 16 bit */
		raw[offset * 4 + 2] = third_byte; /* 4/2 Register offset third byte */
		forth_byte = (value[offset] & 0xFF000000) >> 24; /* Shift right 24 bit */
		raw[offset * 4 + 3] = forth_byte; /* 4/3 Register offset fourth byte */
	}

	err_ret = memcpy_s(data, max_len, raw + 1, sizeof(raw) - 1);
	if (err_ret != EOK) {
		disp_pr_err("secure func call error\n");
		return 0;
	}

	return len;
}

static bool hdmirx_is_empty_packet(hdmirx_packet_type type)
{
	if (type == HDMIRX_PACKET_FVAVRR)
		return true;

	return false;
}

static int hdmirx_get_packet_content(struct hdmirx_ctrl_st *hdmirx,
												hdmirx_packet_type type,
												uint8_t *data,
												uint32_t max_len)
{
	uint32_t div_val;
	uint32_t mod_val;
	uint32_t first_byte;
	uint32_t second_byte;
	uint32_t third_byte;
	uint32_t forth_byte;
	uint32_t offset;
	uint32_t reg_addr;
	uint32_t len = 31; /* 31 : Maximum length of a packet */
	uint32_t value[8] = {0}; /* 8 register */

	if (data == NULL) {
		disp_pr_info("data is null\n");
		return 0;
	}
	if (hdmirx_is_empty_packet(type))
		return hdmirx_get_empty_packet_content(hdmirx, type, data, max_len);

	if (len > max_len)
		len = max_len;

	if (len != 0 && type < HDMIRX_PACKET_MAX) {
		div_val = len / 4; /* 4 bytes 1 register */
		mod_val = len % 4; /* 4 bytes 1 register */
		reg_addr = g_packet_base_addr[type];
		hdmirx_reg_read_block(hdmirx->hdmirx_pwd_base, reg_addr, value, 8); /* 8 register */
		for (offset = 0; offset <= div_val; offset++) {
			if (offset < div_val
				|| hdmirx_get_packet_content_check(mod_val, offset, div_val, 1)) {
				first_byte = value[offset] & 0xFF;
				data[offset * 4 + 0] = first_byte; /* 4/0 Register offset first byte */
			}
			if (offset < div_val
				|| hdmirx_get_packet_content_check(mod_val, offset, div_val, 2)) { /* 2 : register second byte */
				second_byte = (value[offset] & 0xFF00) >> 8; /* Shift right 8 bit */
				data[offset * 4 + 1] = second_byte; /* 4/1 Register offset second byte */
			}
			if (offset < div_val
				|| hdmirx_get_packet_content_check(mod_val, offset, div_val, 3)) { /* 3 : register third byte */
				third_byte = (value[offset] & 0xFF0000) >> 16; /* Shift right 16 bit */
				data[offset * 4 + 2] = third_byte; /* 4/2 Register offset third byte */
			}
			if (offset < div_val) {
				forth_byte = (value[offset] & 0xFF000000) >> 24; /* Shift right 24 bit */
				data[offset * 4 + 3] = forth_byte; /* 4/3 Register offset fourth byte */
			}
		}
	}

	return len;
}

uint32_t hdmirx_packet_get_color_depth(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t cd_value;
	uint32_t bit_width;

	if (g_bit_width != HMDIRX_PACKET_INVALID_VALUE) {
		disp_pr_info("g_bit_width is %d.\n", g_bit_width);
		return g_bit_width;
	}

	cd_value = inp32(hdmirx->hdmirx_pwd_base + 0x1034) & 0x7;
	switch (cd_value) {
	case 4: /* cd value = 4 for 24 bits per pixel */
		bit_width = HDMIRX_INPUT_WIDTH_24;
		break;

	case 5: /* cd value = 5 for 30 bits per pixel */
		bit_width = HDMIRX_INPUT_WIDTH_30;
		break;

	case 6: /* cd value = 6 for 36 bits per pixel */
		bit_width = HDMIRX_INPUT_WIDTH_36;
		break;

	case 7: /* cd value = 7 for 48 bits per pixel */
		bit_width = HDMIRX_INPUT_WIDTH_48;
		break;

	default:
		bit_width = HDMIRX_INPUT_WIDTH_24;
		break;
	}

	disp_pr_info("bit_width is %d.\n", bit_width);

	return bit_width;
}

bool hdmirxv2_packet_avi_is_data_valid(void)
{
	bool valid = false;
	hdmirx_packet_context *packet_ctx = NULL;

	packet_ctx = hdmirx_pakcet_get_ctx();
	if (packet_ctx->avi.version >= AVI_VERSION)
		valid = true;

	return valid;
}

hdmirx_color_space hdmirx_packet_avi_get_color_space(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_packet_context *packet_ctx = NULL;

	if (g_color_space != HMDIRX_PACKET_INVALID_VALUE) {
		disp_pr_info("g_color_space is %d.\n", g_color_space);
		return g_color_space;
	}

	packet_ctx = hdmirx_pakcet_get_ctx();

	if (!hdmirxv2_packet_avi_is_data_valid()) {
		disp_pr_info("avi version not support\n");
		return HDMIRX_COLOR_SPACE_RGB;
	}

	return (hdmirx_color_space)((packet_ctx->avi.pkt_buffer[IF_HEADER_LENGTH] >> 5) & 0x03); /* 5: get high 3 bits */
}

static void hdmirx_packet_avi_store_data(struct hdmirx_ctrl_st *hdmirx, const uint8_t *data, uint32_t len)
{
	hdmirx_packet_context *packet_ctx = hdmirx_pakcet_get_ctx();
	errno_t err_ret;

	err_ret = memcpy_s(packet_ctx->avi.pkt_buffer, sizeof(packet_ctx->avi.pkt_buffer),
		data, len); /* 2: get low 6 bits */
	if (err_ret != EOK) {
		disp_pr_err("secure func call error\n");
		return;
	}

	packet_ctx->avi.version = data[1];
	if (packet_ctx->avi.version < 2) { /* 2: check avi version */
		packet_ctx->avi.pkt_buffer[3 + IF_HEADER_LENGTH] = 0; /* 3: get 8st avi data, set VIC to 0 */
		packet_ctx->avi.pkt_buffer[4 + IF_HEADER_LENGTH] = 0; /* 4: get 9st avi data, repetition */
	}
}

static bool hdmirx_packet_check_sum_is_ok(const uint8_t *data, uint32_t length, uint32_t max_len)
{
	uint32_t i;
	uint8_t check_sum = 0;

	if (length > max_len)
		return false;

	for (i = 0; i < length; i++, data++)
		check_sum += (uint8_t)((*data) & 0xff);

	return (check_sum == 0);
}

static hdmirx_color_metry hdmirx_packet_update_color_metry(hdmirx_color_space in_color_space,
	const uint8_t* data, uint32_t len)
{
	hdmirx_color_metry in_color_metry;
	uint32_t extend_metry;

	extend_metry = data[2 + IF_HEADER_LENGTH]; /* 2: 7st avi data value indicate extend_metry */
	extend_metry = (extend_metry >> 4); /* 4: remove low 4 bits */
	extend_metry &= 0x07;

	if (extend_metry < 2) { /* 2: HDMIRX_COLOR_METRY_XV601 or HDMIRX_COLOR_METRY_XV709 */
		in_color_metry = HDMIRX_COLOR_METRY_XV601 + extend_metry;
	} else if (extend_metry == 5) { /* 5: HDMIRX_COLOR_METRY_BT2020_YCCBCCRC */
		in_color_metry = HDMIRX_COLOR_METRY_BT2020_YCCBCCRC;
	} else if (extend_metry == 6) { /* 6: HDMIRX_COLOR_METRY_BT2020_RGB or HDMIRX_COLOR_METRY_BT2020_YCBCR */
		if (in_color_space == HDMIRX_COLOR_SPACE_RGB) {
			in_color_metry = HDMIRX_COLOR_METRY_BT2020_RGB;
		} else {
			in_color_metry = HDMIRX_COLOR_METRY_BT2020_YCBCR;
		}
	} else {
		in_color_metry = HDMIRX_COLOR_METRY_NOINFO;
	}

	return in_color_metry;
}

void hdmirx_packet_depack_avi(struct hdmirx_ctrl_st *hdmirx)
{
	uint8_t data[PACKET_BUFFER_LENGTH] = {0};
	uint32_t length;
	hdmirx_oversample replication;
	hdmirx_color_space in_color_space;
	hdmirx_color_metry in_color_metry;
	bool check_sum = false;
	int i;
	hdmirx_packet_context *packet_ctx = hdmirx_pakcet_get_ctx();

	static int test_cnt = 0;
	int ret;

	ret = hdmirx_get_packet_content(hdmirx, HDMIRX_PACKET_AVI, data, PACKET_BUFFER_LENGTH);
	if (test_cnt <= 10) {
		for (i = 0; i < PACKET_BUFFER_LENGTH; i++)
			disp_pr_info("[hdmirx]avi packet %d:0x%x\n", i, data[i]);
	}

	if (memcmp(data, packet_ctx->avi.pkt_buffer, sizeof(packet_ctx->avi.pkt_buffer)) == 0)
		return;

	length = data[IF_LENGTH_INDEX];

	if ((length < IF_MIN_AVI_LENGTH) || (length > IF_MAX_AVI_LENGTH)) {
		disp_pr_err("avi length invalid %d\n", length);
		return;
	}

	check_sum = hdmirx_packet_check_sum_is_ok(data, length + IF_HEADER_LENGTH, sizeof(data) / sizeof(data[0]));
	if (!check_sum) {
		disp_pr_info("avi check sum error\n");
		return;
	}
	replication = (data[4 + IF_HEADER_LENGTH] & 0x0f); /* 4: 9st avi data indicate replication */
	in_color_space = ((data[IF_HEADER_LENGTH] >> 5) & 0x03); /* 5: get high 3 bits */
	in_color_metry = ((data[1 + IF_HEADER_LENGTH] >> 6) & 0x03); /* 6: get high 2 bits */
	if (in_color_metry == HDMIRX_COLOR_METRY_EXTENDED)
		in_color_metry = hdmirx_packet_update_color_metry(in_color_space, data, PACKET_BUFFER_LENGTH);

	disp_pr_info("replication 0x%x, in_color_space 0x%x, in_color_metry 0x%x\n",
		replication, in_color_space, in_color_metry);
	hdmirx_packet_avi_store_data(hdmirx, data, length + IF_HEADER_LENGTH);

	disp_pr_info("avi vic 0x%x\n", packet_ctx->avi.pkt_buffer[3 + IF_HEADER_LENGTH] & 0x7f); /* 3: get 8st avi data */
}

void hdmirx_packet_depack_vsi(struct hdmirx_ctrl_st *hdmirx)
{
	uint8_t data[PACKET_BUFFER_LENGTH] = {0};
	static int test_cnt = 0;
	int i;
	hdmirx_get_packet_content(hdmirx, HDMIRX_PACKET_VSI, data, PACKET_BUFFER_LENGTH);
	if (test_cnt <= 10) {
		for (i = 0; i < PACKET_BUFFER_LENGTH; i++)
			disp_pr_info("[hdmirx]vsi packet %d:0x%x\n", i, data[i]);
	}
}

void hdmirx_packet_depack_hfvsi(struct hdmirx_ctrl_st *hdmirx)
{
	uint8_t data[PACKET_BUFFER_LENGTH] = {0};
	static int test_cnt = 0;
	int i;

	hdmirx_get_packet_content(hdmirx, HDMIRX_PACKET_HFVSI_3D, data, PACKET_BUFFER_LENGTH);
	if (test_cnt <= 10) {
		for (i = 0; i < PACKET_BUFFER_LENGTH; i++)
			disp_pr_info("[hdmirx]hfvsi packet %d:0x%x\n", i, data[i]);
	}
}

void hdmirx_packet_depack_hdr(struct hdmirx_ctrl_st *hdmirx)
{
	uint8_t data[PACKET_BUFFER_LENGTH];
	static int test_cnt = 0;
	uint32_t length;
	uint32_t ret_len;
	const uint32_t if_max_hdr_length = 26; /* 26: if length check threshold */
	hdmirx_packet_context *packet_ctx = NULL;
	bool check_sum = false;
	int i;
	errno_t err_ret;

	packet_ctx = hdmirx_pakcet_get_ctx();

	ret_len = hdmirx_get_packet_content(hdmirx, HDMIRX_PACKET_HDR, data, PACKET_BUFFER_LENGTH);
	if (test_cnt <= 10) {
		for (i = 0; i < PACKET_BUFFER_LENGTH; i++)
			disp_pr_info("[hdmirx]hdr packet %d:0x%x\n", i, data[i]);
	}

	length = data[IF_LENGTH_INDEX];
	if ((length > if_max_hdr_length) || (length == 0)) {
		disp_pr_err("hdr length invalid %d\n", length);
		return;
	}

	if (memcmp(data, packet_ctx->hdr.pkt_buffer, sizeof(packet_ctx->hdr.pkt_buffer)) == 0) {
		disp_pr_info("hdr not change\n");
		return;
	}

	check_sum = hdmirx_packet_check_sum_is_ok(data, length + IF_HEADER_LENGTH, ret_len);
	if (!check_sum) {
		disp_pr_err("hdr check sum error\n");
		return;
	}

	err_ret = memcpy_s(packet_ctx->hdr.pkt_buffer, sizeof(packet_ctx->hdr.pkt_buffer),
		data, length + IF_HEADER_LENGTH);
	if (err_ret != EOK) {
		disp_pr_err("memcpy_s error, dst_len:%u, src_len:%u\n",
			sizeof(packet_ctx->hdr.pkt_buffer), ret_len);
		return;
	}

	packet_ctx->hdr.pkt_received = true;
	packet_ctx->hdr.pkt_len = length + IF_HEADER_LENGTH;
	packet_ctx->hdr.pkt_type = HDMIRX_PACKET_HDR;
	hdmirx_hdr_set_hdr_type(&(packet_ctx->hdr));

	disp_pr_info("hdr depack success.\n");
}


void hdmirx_packet_proc_avi(struct hdmirx_ctrl_st *hdmirx, uint32_t interrupts)
{
	if (!(interrupts & INTR_CEA_UPDATE_AVI))
		return;

	hdmirx_packet_depack_avi(hdmirx);
}

void hdmirx_packet_proc_vsi(struct hdmirx_ctrl_st *hdmirx, uint32_t interrupts)
{
	if (!(interrupts & INTR_CEA_UPDATE_VSI))
		return;

	hdmirx_packet_depack_vsi(hdmirx);
}

void hdmirx_packet_proc_hfvsi(struct hdmirx_ctrl_st *hdmirx, uint32_t interrupts)
{
	if (!(interrupts & INTR_CEA_UPDATE_HFVSI_3D))
		return;

	hdmirx_packet_depack_vsi(hdmirx);
}

void hdmirx_packet_proc_hdr(struct hdmirx_ctrl_st *hdmirx, uint32_t interrupts)
{
	if (!(interrupts & INTR_CEA_UPDATE_HDR))
		return;

	hdmirx_packet_depack_hdr(hdmirx);
}

int hdmirx_color_depth_process(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t temp;
	uint32_t color_depth;
	temp = inp32(hdmirx->hdmirx_pwd_base + 0x13e8);
	if (!(temp & 0x1000))
		return 0;

	color_depth = inp32(hdmirx->hdmirx_pwd_base + 0x1034) & 0x7;
	disp_pr_info("color depth : 0x%x\n",color_depth);
	hdmirx_ctrl_irq_clear(hdmirx->hdmirx_pwd_base + 0x13e8);
	return 0;
}

int hdmirx_video_gcp_mute(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t temp;
	temp = inp32(hdmirx->hdmirx_pwd_base + 0x13e8);
	if (!((temp & 0x1) || (temp & 0x8)))
		return 0;

	if (temp & 0x1)
		disp_pr_info("gcp_avmute get\n");

	if (temp & 0x8)
		disp_pr_info("set mute irq get\n");

	hdmirx_ctrl_irq_clear(hdmirx->hdmirx_pwd_base + 0x13e8);
	return 0;
}

int hdmirx_packet_reg_depack(struct hdmirx_ctrl_st *hdmirx)
{
	static int test_cnt = 0;
	uint32_t temp;
	int ret = 0;

	test_cnt++;

	temp = inp32(hdmirx->hdmirx_pwd_base + 0x18);
	if (test_cnt <= 10)
		disp_pr_info("[hdmirx]depack flag 0x%x\n", temp);

	if (!(temp & 0x00000001))
		return 0;

	temp = inp32(hdmirx->hdmirx_pwd_base + 0x13D8);
	if (test_cnt <= 10)
		disp_pr_info("[hdmirx]depack irq status 0x%x\n", temp);

	if (temp == 0) {
		disp_pr_err("[hdmirx]depack irq status error\n", temp);
		return -1;
	}
	hdmirx_ctrl_irq_clear(hdmirx->hdmirx_pwd_base + 0x13D8);

	hdmirx_packet_proc_avi(hdmirx, temp);
	hdmirx_packet_proc_vsi(hdmirx, temp);
	hdmirx_packet_proc_hfvsi(hdmirx, temp);
	hdmirx_packet_proc_hdr(hdmirx, temp);

	return ret;
}

int hdmirx_packet_ram_depack(struct hdmirx_ctrl_st *hdmirx)
{
	static int test_cnt = 0;
	uint8_t data[PACKET_BUFFER_LENGTH] = {0};
	uint32_t temp;
	int ret = 0;
	int i;

	test_cnt++;

	temp = inp32(hdmirx->hdmirx_sysctrl_base + 0x44);
	if (test_cnt <= 10)
		disp_pr_info("[hdmirx]depack flag 0x%x\n", temp);

	if (!(temp & 0x00000001))
		return 0;

	temp = inp32(hdmirx->hdmirx_sysctrl_base + 0x38);
	if (test_cnt <= 10)
		disp_pr_info("[hdmirx]depack ram 0x%x\n", temp);

	if (temp >= HDMIRX_PACKET_MAX) {
		disp_pr_err("[hdmirx]depack ram error 0x%x\n", temp);
		return -1;
	}

	if (test_cnt <= 10) {
		for (i = 0; i < PACKET_BUFFER_LENGTH; i++)
			disp_pr_info("[hdmirx] packet %d:0x%x\n", i, data[i]);
	}

	return ret;
}

void hdmirx_packet_set_ram_irq_mask(struct hdmirx_ctrl_st *hdmirx)
{
	set_reg(hdmirx->hdmirx_sysctrl_base + 0x040, 0x3, 2, 0);
}

void hdmirx_packet_depack_enable(struct hdmirx_ctrl_st *hdmirx)
{
	// PACKET TYPE irq MASK
	set_reg(hdmirx->hdmirx_pwd_base + 0x13D4, 0x7FFFF, 19, 0);
}
