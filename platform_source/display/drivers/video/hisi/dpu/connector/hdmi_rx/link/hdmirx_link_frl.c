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

#include "hdmirx_link_frl.h"
#include "hdmirx_common.h"
#include "control/hdmirx_ctrl.h"
#include <linux/delay.h>
#include <securec.h>

uint32_t g_pattern = LTP6;
uint32_t g_pattern1 = LTP5;
uint32_t g_pattern2 = LTP8;
uint32_t g_flag_cnt = PATTERN_CNT;

const static uint32_t g_frl_10b9b_lut[] = {
	0x016f016f, 0x00f70190, 0x016f016f, 0x00770190, 0x01af01af, 0x00b70150, 0x013700d0, 0x002f01c8,
	0x01cf01cf, 0x00d70130, 0x015700b0, 0x004f0028, 0x01970070, 0x008f0108, 0x010f0088, 0x01f001f0,
	0x016f016f, 0x00e70190, 0x816f016f, 0x01908190, 0x81af01af, 0x01508150, 0x00d080d0, 0x802f002f,
	0x81cf01cf, 0x01308130, 0x00b080b0, 0x804f004f, 0x00708070, 0x808f008f, 0x810f010f, 0x01f081f0,
	0x00f700f7, 0x80f700f7, 0x81770177, 0x80778188, 0x81b701b7, 0x80b78148, 0x813780c8, 0x803781c8,
	0x81d701d7, 0x80d78128, 0x815780a8, 0x81a88028, 0x81978068, 0x81688108, 0x80e88088, 0x01e881e8,
	0x81e701e7, 0x80e78118, 0x81678098, 0x81988018, 0x81a78058, 0x815881c0, 0x80d881d0, 0x01d881d8,
	0x81c78038, 0x813880f0, 0x80b881b0, 0x814f81b8, 0x80788170, 0x818f8178, 0x800780f8, 0x01f881f8,
	0x00fb00fb, 0x80fb00fb, 0x817b017b, 0x807b8184, 0x81bb01bb, 0x80bb8144, 0x813b80c4, 0x803b81c4,
	0x81db01db, 0x80db8124, 0x815b80a4, 0x81a48024, 0x819b8064, 0x81648104, 0x80e48084, 0x01e481e4,
	0x81eb01eb, 0x80eb8114, 0x816b8094, 0x81948014, 0x81ab8054, 0x81548100, 0x80d480e0, 0x01d481d4,
	0x81cb8034, 0x813481fd, 0x80b481fe, 0x801f81b4, 0x80748030, 0x800f8174, 0x800b80f4, 0x01f481f4,
	0x81f301f3, 0x80f3810c, 0x8173808c, 0x818c800c, 0x81b3804c, 0x814c8008, 0x80cc80ef, 0x01cc81cc,
	0x81d3802c, 0x812c8000, 0x80ac81ef, 0x805081ac, 0x806c8044, 0x8090816c, 0x801380ec, 0x01ec81ec,
	0x81e301e3, 0x811c801c, 0x809c819f, 0x8060819c, 0x805c81fc, 0x80a0815c, 0x812080dc, 0x802381dc,
	0x803c003c, 0x80c0813c, 0x814080bc, 0x804381bc, 0x8003807c, 0x8083817c, 0x810380fc, 0x010300fc,
	0x00fd00fd, 0x80fd00fd, 0x817d017d, 0x807d8182, 0x81bd01bd, 0x80bd8142, 0x813d80c2, 0x803d81c2,
	0x81dd01dd, 0x80dd8122, 0x815d80a2, 0x81a28022, 0x819d8062, 0x81628102, 0x80e28082, 0x01e281e2,
	0x81ed01ed, 0x80ed8112, 0x816d8092, 0x81928012, 0x81ad8052, 0x815281f7, 0x80d200d2, 0x01d281d2,
	0x81cd8032, 0x813281fb, 0x80b28080, 0x81df81b2, 0x80728040, 0x01728172, 0x800d80f2, 0x01f281f2,
	0x81f501f5, 0x80f5810a, 0x8175808a, 0x818a800a, 0x81b5804a, 0x814a8002, 0x80ca8048, 0x01ca81ca,
	0x81d5802a, 0x812a8180, 0x80aa00aa, 0x817f81aa, 0x806a8042, 0x80ff816a, 0x801580ea, 0x01ea81ea,
	0x81e501e5, 0x811a801a, 0x809a009a, 0x806f819a, 0x805a81ff, 0x80af815a, 0x812f80da, 0x802581da,
	0x803a003a, 0x80cf813a, 0x816080ba, 0x804581ba, 0x8005807a, 0x8085817a, 0x810580fa, 0x01fa81fa,
	0x81f901f9, 0x80f98106, 0x81798086, 0x81868006, 0x81b98046, 0x81468004, 0x80c600c6, 0x01c681c6,
	0x81d98026, 0x81268016, 0x80a600a6, 0x807f81a6, 0x80660066, 0x80bf8166, 0x801980e6, 0x01e681e6,
	0x81e901e9, 0x8116811f, 0x8096809f, 0x80418196, 0x8056805f, 0x81a08156, 0x813f80d6, 0x802981d6,
	0x80360036, 0x80df8136, 0x815f80b6, 0x804981b6, 0x80098076, 0x80898176, 0x810980f6, 0x01f681f6,
	0x81f101f1, 0x810e803f, 0x808e81bf, 0x81e0818e, 0x804e800e, 0x8010814e, 0x802080ce, 0x803181ce,
	0x802e002e, 0x8001812e, 0x811080ae, 0x805181ae, 0x8011806e, 0x8091816e, 0x811180ee, 0x01ee81ee,
	0x801e001e, 0x8081811e, 0x8101809e, 0x8061819e, 0x8021805e, 0x80a1815e, 0x812180de, 0x01de81de,
	0x81c1803e, 0x80c1813e, 0x814180be, 0x01be81be, 0x8181807e, 0x017e817e, 0x00fe80fe, 0x00fe00fe,
	0x00fe00fe, 0x80fe00fe, 0x817e017e, 0x807e8181, 0x81be01be, 0x80be8141, 0x813e80c1, 0x803e81c1,
	0x81de01de, 0x80de8121, 0x815e80a1, 0x81a18021, 0x819e8061, 0x81618101, 0x80e18081, 0x01e181e1,
	0x81ee01ee, 0x80ee8111, 0x816e8091, 0x81918011, 0x81ae8051, 0x81518110, 0x80d18001, 0x01d181d1,
	0x81ce8031, 0x81318020, 0x80b18010, 0x800e81b1, 0x807181e0, 0x81bf8171, 0x803f80f1, 0x01f181f1,
	0x81f601f6, 0x80f68109, 0x81768089, 0x81898009, 0x81b68049, 0x8149815f, 0x80c980df, 0x01c981c9,
	0x81d68029, 0x8129813f, 0x80a981a0, 0x805f81a9, 0x80698041, 0x809f8169, 0x811f80e9, 0x01e981e9,
	0x81e601e6, 0x81198019, 0x809980bf, 0x01998199, 0x8059807f, 0x01598159, 0x801680d9, 0x802681d9,
	0x80390039, 0x01398139, 0x800480b9, 0x804681b9, 0x80068079, 0x80868179, 0x810680f9, 0x01f981f9,
	0x81fa01fa, 0x80fa8105, 0x817a8085, 0x81858005, 0x81ba8045, 0x81458160, 0x80c580cf, 0x01c581c5,
	0x81da8025, 0x8125812f, 0x80a580af, 0x81ff81a5, 0x8065806f, 0x01658165, 0x801a80e5, 0x01e581e5,
	0x81ea01ea, 0x81158015, 0x809580ff, 0x80428195, 0x8055817f, 0x01558155, 0x818080d5, 0x802a81d5,
	0x80350035, 0x80488135, 0x800280b5, 0x804a81b5, 0x800a8075, 0x808a8175, 0x810a80f5, 0x01f581f5,
	0x81f201f2, 0x810d800d, 0x808d008d, 0x8040818d, 0x804d81df, 0x8080814d, 0x81fb80cd, 0x803281cd,
	0x802d002d, 0x012d812d, 0x81f780ad, 0x805281ad, 0x8012806d, 0x8092816d, 0x811280ed, 0x01ed81ed,
	0x801d001d, 0x8082811d, 0x8102809d, 0x8062819d, 0x8022805d, 0x80a2815d, 0x812280dd, 0x01dd81dd,
	0x81c2803d, 0x80c2813d, 0x814280bd, 0x01bd81bd, 0x8182807d, 0x017d817d, 0x00fd80fd, 0x00fd00fd,
	0x00fc0103, 0x80fc8103, 0x817c8083, 0x81838003, 0x81bc8043, 0x81438140, 0x80c380c0, 0x01c381c3,
	0x81dc8023, 0x81238120, 0x80a380a0, 0x81fc81a3, 0x80638060, 0x819f8163, 0x801c80e3, 0x01e381e3,
	0x81ec01ec, 0x81138013, 0x80938090, 0x80448193, 0x80538050, 0x81ef8153, 0x800080d3, 0x802c81d3,
	0x80330033, 0x80ef8133, 0x800880b3, 0x804c81b3, 0x800c8073, 0x808c8173, 0x810c80f3, 0x01f381f3,
	0x81f401f4, 0x810b800b, 0x808b800f, 0x8030818b, 0x804b801f, 0x81fe814b, 0x81fd80cb, 0x803481cb,
	0x802b002b, 0x80e0812b, 0x810080ab, 0x805481ab, 0x8014806b, 0x8094816b, 0x811480eb, 0x01eb81eb,
	0x801b001b, 0x8084811b, 0x8104809b, 0x8064819b, 0x8024805b, 0x80a4815b, 0x812480db, 0x01db81db,
	0x81c4803b, 0x80c4813b, 0x814480bb, 0x01bb81bb, 0x8184807b, 0x017b817b, 0x00fb80fb, 0x00fb00fb,
	0x81f801f8, 0x81078007, 0x8087818f, 0x81708187, 0x8047814f, 0x81b08147, 0x80f080c7, 0x803881c7,
	0x80270027, 0x81d08127, 0x81c080a7, 0x805881a7, 0x80188067, 0x80988167, 0x811880e7, 0x01e781e7,
	0x80170017, 0x80888117, 0x81088097, 0x80688197, 0x80288057, 0x80a88157, 0x812880d7, 0x01d781d7,
	0x81c88037, 0x80c88137, 0x814880b7, 0x01b781b7, 0x81888077, 0x01778177, 0x00f780f7, 0x00f700f7,
	0x81f001f0, 0x010f810f, 0x008f808f, 0x80700070, 0x004f804f, 0x80b000b0, 0x81300130, 0x01cf81cf,
	0x002f802f, 0x80d000d0, 0x81500150, 0x01af81af, 0x81900190, 0x016f816f, 0x019000e7, 0x016f016f,
	0x01f001f0, 0x0088010f, 0x0108008f, 0x00700197, 0x0028004f, 0x00b00157, 0x013000d7, 0x01cf01cf,
	0x01c8002f, 0x00d00137, 0x015000b7, 0x01af01af, 0x01900077, 0x016f016f, 0x019000f7, 0x016f016f,
};

const static uint32_t g_frl_8b7b_lut[] = {
	0x00370048, 0x00370048, 0x00570028, 0x000f001f, 0x00670018, 0x000f002f, 0x000f004f, 0x800f000f,
	0x00370048, 0x80378048, 0x80578028, 0x8068801f, 0x80678018, 0x8058802f, 0x8038804f, 0x00788078,
	0x003b0044, 0x803b8044, 0x805b8024, 0x80648060, 0x806b8014, 0x80548077, 0x80348004, 0x00748074,
	0x80730073, 0x804c806f, 0x802c803f, 0x8003806c, 0x801c800c, 0x8030805c, 0x8050803c, 0x007c807c,
	0x003d0042, 0x803d8042, 0x805d8022, 0x80620062, 0x806d8012, 0x80528070, 0x80328002, 0x00728072,
	0x80750075, 0x804a8040, 0x802a8020, 0x8005806a, 0x801a800a, 0x807d805a, 0x807f803a, 0x007a807a,
	0x80790079, 0x8046805f, 0x8026807e, 0x80098066, 0x80168006, 0x80008056, 0x80088036, 0x00768076,
	0x800e000e, 0x8001804e, 0x8010802e, 0x8011806e, 0x807b801e, 0x8021805e, 0x8041803e, 0x0041003e,
	0x003e0041, 0x803e8041, 0x805e8021, 0x8061807b, 0x806e8011, 0x80518010, 0x80318001, 0x00718071,
	0x80760076, 0x80498008, 0x80298000, 0x80068069, 0x80198009, 0x807e8059, 0x805f8039, 0x00798079,
	0x807a007a, 0x8045807f, 0x8025807d, 0x800a8065, 0x80158005, 0x80208055, 0x80408035, 0x00758075,
	0x800d000d, 0x8002804d, 0x8070802d, 0x8012806d, 0x001d801d, 0x8022805d, 0x8042803d, 0x0042003d,
	0x807c007c, 0x80438050, 0x80238030, 0x800c8063, 0x80138003, 0x803f8053, 0x806f8033, 0x00738073,
	0x800b000b, 0x8004804b, 0x8077802b, 0x8014806b, 0x8060801b, 0x8024805b, 0x8044803b, 0x0044003b,
	0x80070007, 0x804f8047, 0x802f8027, 0x80188067, 0x801f8017, 0x80288057, 0x80488037, 0x00480037,
	0x000f800f, 0x004f000f, 0x002f000f, 0x00180067, 0x001f000f, 0x00280057, 0x00480037, 0x00480037,
};

const char *g_frl_state_string[] = {
	"idel",
	"ltp req",
	"patternchk",
	"gap check",
	"frl trans",
	"done",
	"max"
};

const char *g_frl_rate_string[] = {
	"tmds",
	"3lane_3g",
	"3lane_6g",
	"4lane_6g",
	"4lane_8g",
	"4lane_10g",
	"4lane_12g",
	"max"
};

const char *g_ltp_state_string[] = {
	"idel",
	"checking",
	"done",
	"timeout",
	"max"
};

const char *g_gap_state_string[] = {
	"idel",
	"checking",
	"done",
	"timeout",
	"max"
};

static hdmirx_frl_ctx g_hdmirx_frl_ctx;

static hdmirx_frl_ctx *hdmirx_frl_get_ctx()
{
	return &g_hdmirx_frl_ctx;
}

int hdmirx_link_frl_init(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_frl_ctx *frl_ctx = NULL;

	disp_pr_info("frl init +++++++++\n");

	frl_ctx = hdmirx_frl_get_ctx();

	set_reg(hdmirx->hdmirx_aon_base + 0x1094, 0x21ff, 32, 0);

	set_reg(hdmirx->hdmirx_pwd_base + 0x0028, 0x1, 1, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0x0028, 0x1, 1, 3);
	set_reg(hdmirx->hdmirx_pwd_base + 0x0028, 0x0, 1, 2);
	set_reg(hdmirx->hdmirx_pwd_base + 0x0028, 0x0, 2, 16);

	hdmirx_link_frl_lut_data(hdmirx);

	hdmirx_link_frl_ltp_gap_checkcycle(hdmirx);

	hdmirx_link_frl_ltp_irq_set(hdmirx);

	disp_pr_info("wait for ready\n");

	frl_clear_init(hdmirx);

	frl_ctx->state = FRL_STATE_LTP_REQ;

	disp_pr_info("frl init --------\n");
	return 0;
}

int hdmirx_link_frl_lut_check(struct hdmirx_ctrl_st *hdmirx, const uint32_t *g_lut, uint32_t len)
{
	uint32_t checksum_read;
	uint32_t checksum_cal;
	uint32_t num;
	disp_pr_info("lut check +++++++++len : %d\n", len);
	checksum_cal = 0;
	for (num = 0; num < len; num++){
		if (len == NUM10B8B)
			checksum_cal += (g_lut[num] & 0x01FF) + ((g_lut[num] >> 16) & 0x01FF);
		else
			checksum_cal += (g_lut[num] & 0x07F) + ((g_lut[num] >> 16) & 0x07F);

		checksum_cal += (((g_lut[num] >> 15) & 0x1) * len);
		checksum_cal += (((g_lut[num] >> 31) & 0x1) * len);
	}
	disp_pr_info("checksum_cal:0x%x\n", checksum_cal);
	mdelay(10);
	if (len == NUM10B8B)
		checksum_read = inp32(hdmirx->hdmirx_pwd_base + 0xBB18);
	else
		checksum_read = inp32(hdmirx->hdmirx_pwd_base + 0xBB20);

	disp_pr_info("checksum_read:0x%x\n", checksum_read);

	if (checksum_read != checksum_cal)
		disp_pr_err("cheacksum error!\n");
	else
		disp_pr_info("len %d success\n", len);

	disp_pr_info("lut check ----------len : %d\n", len);
	return 0;
}

int hdmirx_link_frl_lut_data(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t lut_10b9b_size;
	uint32_t lut_8b7b_size;
	uint32_t num_10b9b;
	uint32_t num_8b7b;
	uint32_t offset_10b9b;
	uint32_t offset_8b7b;
	uint32_t lut10;
	uint32_t lut8;
	disp_pr_info("lut data +++++++++\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0xa, 4, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0x1, 1, 4);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0x0, 2, 5);

	lut_10b9b_size = sizeof(g_frl_10b9b_lut) / sizeof(g_frl_10b9b_lut[0]);

	for (num_10b9b = 0; num_10b9b < lut_10b9b_size; num_10b9b++){
		offset_10b9b = 0xB100 + 0x4 * num_10b9b;
		set_reg(hdmirx->hdmirx_pwd_base + offset_10b9b, 0, 32, 0);
	}

	lut_8b7b_size = sizeof(g_frl_8b7b_lut) / sizeof(g_frl_8b7b_lut[0]);

	for (num_8b7b = 0; num_8b7b < lut_8b7b_size; num_8b7b++){
		offset_8b7b = 0xB900 + 0x4 * num_8b7b;
		set_reg(hdmirx->hdmirx_pwd_base + offset_8b7b, 0, 32, 0);
	}
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0x15, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0xa, 4, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0x1, 1, 4);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0x0, 2, 5);
	num_10b9b = 0;
	num_8b7b = 0;

	set_reg(hdmirx->hdmirx_pwd_base + 0xBB14, 0x0, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB1C, 0x0, 32, 0);

	lut10 = inp32(hdmirx->hdmirx_pwd_base + 0xBB18);
	lut8 = inp32(hdmirx->hdmirx_pwd_base + 0xBB20);
	disp_pr_info("lut10:0x%x\n", lut10);
	disp_pr_info("lut8:0x%x\n", lut8);

	set_reg(hdmirx->hdmirx_pwd_base + 0xBB14, 0x1, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB1C, 0x1, 32, 0);

	lut_10b9b_size = sizeof(g_frl_10b9b_lut) / sizeof(g_frl_10b9b_lut[0]);

	for (num_10b9b = 0; num_10b9b < lut_10b9b_size; num_10b9b++){
		offset_10b9b = 0xB100 + 0x4 * num_10b9b;
		set_reg(hdmirx->hdmirx_pwd_base + offset_10b9b, g_frl_10b9b_lut[num_10b9b], 32, 0);
	}

	lut_8b7b_size = sizeof(g_frl_8b7b_lut) / sizeof(g_frl_8b7b_lut[0]);

	for (num_8b7b = 0; num_8b7b < lut_8b7b_size; num_8b7b++){
		offset_8b7b = 0xB900 + 0x4 * num_8b7b;
		set_reg(hdmirx->hdmirx_pwd_base + offset_8b7b, g_frl_8b7b_lut[num_8b7b], 32, 0);
	}

	lut10 = inp32(hdmirx->hdmirx_pwd_base + 0xBB18);
	lut8 = inp32(hdmirx->hdmirx_pwd_base + 0xBB20);
	disp_pr_info("after lut10:0x%x\n", lut10);
	disp_pr_info("after lut8:0x%x\n", lut8);

	disp_pr_info("lut_10b9b_size:%d\n", lut_10b9b_size);
	hdmirx_link_frl_lut_check(hdmirx, g_frl_10b9b_lut, lut_10b9b_size);

	disp_pr_info("lut_8b7b_size:%d\n", lut_8b7b_size);
	hdmirx_link_frl_lut_check(hdmirx, g_frl_8b7b_lut, lut_8b7b_size);

	set_reg(hdmirx->hdmirx_pwd_base + 0xBB14, 0x0, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB1C, 0x0, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBB00, 0x15, 32, 0);

	set_reg(hdmirx->hdmirx_pwd_base + 0xBC08, 0x0, 32, 0);

	if (g_flag_cnt == 16) {
		set_reg(hdmirx->hdmirx_pwd_base + 0xBC0C, 0x10, 32, 0);
		disp_pr_info("check 16 times\n");
	} else {
		set_reg(hdmirx->hdmirx_pwd_base + 0xBC0C, 0x1, 32, 0);
		set_reg(hdmirx->hdmirx_pwd_base + 0x1b094, 0x7C, 32, 0);
		disp_pr_info("check 1 times\n");
	}

	disp_pr_info("lut data ---------\n");
	return 0;
}

int hdmirx_link_frl_ltp_gap_checkcycle(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("gap checkcycle\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0xBC20, 0x0, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBC24, 0x0, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xBC28, 0x10, 32, 0);
	return 0;
}

int hdmirx_link_frl_ltp_irq_set(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("irq set\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0xB014, 0x1, 1, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xB018, 0xf, 4, 3);
	set_reg(hdmirx->hdmirx_pwd_base + 0xB01C, 0x3, 2, 0);
	return 0;
}

int hdmirx_link_frl_ready(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t frl_ready;

	set_reg(hdmirx->hdmirx_pwd_base + 0x8500, 0x1, 1, 6);

	frl_ready = inp32(hdmirx->hdmirx_pwd_base + 0x8500);
	disp_pr_info("frl ready:0x%x\n", frl_ready);
	return 0;
}

void hdmirx_frl_clear_all(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t b000;
	uint32_t b004;
	uint32_t b008;
	uint32_t b00c;
	uint32_t b010;

	b000 = inp32(hdmirx->hdmirx_pwd_base + 0xb000);
	b004 = inp32(hdmirx->hdmirx_pwd_base + 0xb004);
	b008 = inp32(hdmirx->hdmirx_pwd_base + 0xb008);
	b00c = inp32(hdmirx->hdmirx_pwd_base + 0xb00c);
	b010 = inp32(hdmirx->hdmirx_pwd_base + 0xb010);
	disp_pr_info("B000:0x%x\n", b000);
	disp_pr_info("B004:0x%x\n", b004);
	disp_pr_info("B008:0x%x\n", b008);
	disp_pr_info("B00c:0x%x\n", b00c);
	disp_pr_info("B010:0x%x\n", b010);
	if (b008 != 0)
		set_reg(hdmirx->hdmirx_pwd_base + 0xB008, b008, 8, 0);

	if (b004 != 0)
		set_reg(hdmirx->hdmirx_pwd_base + 0xB004, b004, 8, 0);

	if (b000 != 0)
		set_reg(hdmirx->hdmirx_pwd_base + 0xB000, b000, 8, 0);

	if (b00c != 0)
		set_reg(hdmirx->hdmirx_pwd_base + 0xB00c, b004, 8, 0);

	if (b010 != 0)
		set_reg(hdmirx->hdmirx_pwd_base + 0xB010, b000, 8, 0);
}

int hdmirx_frl_process(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t temp;
	hdmirx_frl_ctx *frl_ctx = NULL;
	disp_pr_info("frl irq coming\n");

	frl_ctx = hdmirx_frl_get_ctx();

	temp = inp32(hdmirx->hdmirx_pwd_base + 0x18);
	if (!(temp & 0x8)) {
		disp_pr_warn("not frl irq\n");
		return 0;
	}

	if (frl_get_interrupt(hdmirx, HDMIRX_FRL_INT_SOURCE_FLT) == true) {
		frl_clear_interrupt(hdmirx, HDMIRX_FRL_INT_SOURCE_FLT);
		hdmirx_frl_req_set(hdmirx);
		hdmirx_frl_clear_all(hdmirx);

		return 0;
	}

	if (frl_get_interrupt(hdmirx, HDMIRX_FRL_INT_FLT_UPDATE) == true) {
		frl_clear_interrupt(hdmirx, HDMIRX_FRL_INT_FLT_UPDATE);
		switch (frl_ctx->state) {
		case FRL_STATE_PATTEN_CHECK:
			hdmirx_frl_ltp_pattern_check(hdmirx);
			break;
		case FRL_STATE_GAP_CHECK:
			hdmirx_frl_gap_state_check(hdmirx);
			break;
		default:
			break;
		}
	}
	hdmirx_frl_clear_all(hdmirx);

	return 0;
}

int hdmirx_frl_req_set(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_frl_ctx *frl_ctx = NULL;
	frl_ctx = hdmirx_frl_get_ctx();

	disp_pr_info("hdmirx frl req set\n");

	set_reg(hdmirx->hdmirx_pwd_base + 0x8504, g_pattern, 4, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0x8504, g_pattern1, 4, 4);
	set_reg(hdmirx->hdmirx_pwd_base + 0x8508, g_pattern2, 4, 0);

	hdmirx_frl_flt_update(hdmirx);

	frl_set_state(hdmirx, FRL_STATE_PATTEN_CHECK);

	return 0;
}

void hdmirx_cmd_pattern(uint32_t ltp0, uint32_t ltp1, uint32_t ltp2)
{
	disp_pr_info("[hdmirx]g_pattern: 0x%x, 0x%x, 0x%x", ltp0, ltp1, ltp2);

	g_pattern = ltp0;
	g_pattern1 = ltp1;
	g_pattern2 = ltp2;
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_pattern);

void hdmirx_cmd_frl_cnt(uint32_t count)
{
	disp_pr_info("[hdmirx]frl cnt set");

	g_flag_cnt = count;
}
EXPORT_SYMBOL_GPL(hdmirx_cmd_frl_cnt);

int hdmirx_frl_flt_update(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t update;
	disp_pr_info("flt update\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0x8440, 0x1, 1, 5);

	update = inp32(hdmirx->hdmirx_pwd_base + 0x8440);
	disp_pr_info("flt update : 0x%x\n", update);
	return 0;
}

static void frl_set_ltp_fixreq_bylane(struct hdmirx_ctrl_st *hdmirx, frl_lane lane)
{
	hdmirx_frl_ctx *frl_ctx = NULL;
	uint8_t lane_count;
	disp_pr_info("fixreq bylane : 0x%x\n", lane);
	frl_ctx = hdmirx_frl_get_ctx();
	lane_count = 3;
	if (lane >= lane_count) {
		disp_pr_info("input lane %d is larger than max lane %u\n", lane, lane_count);
		return;
	}
	switch (lane) {
	case 0: /*  lane 0 */
		set_reg(hdmirx->hdmirx_pwd_base + 0x8504, 0xE, 4, 0);
		break;
	case 1: /*  lane 1 */
		set_reg(hdmirx->hdmirx_pwd_base + 0x8504, 0xE, 4, 4);
		break;
	case 2: /*  lane 2 */
		set_reg(hdmirx->hdmirx_pwd_base + 0x8508, 0xE, 4, 0);
		break;
	default:
		break;
	}
	hdmirx_frl_flt_update(hdmirx);
}

static void frl_pattern_timeout_proc(struct hdmirx_ctrl_st *hdmirx, uint8_t lane)
{
	hdmirx_frl_ctx *frl_ctx = NULL;
	frl_ctx = hdmirx_frl_get_ctx();
	if (frl_ctx->ffe >= 3) {
		frl_ctx->ffe = 0;
		set_reg(hdmirx->hdmirx_pwd_base + 0x8504, 0xFF, 32, 0);
		set_reg(hdmirx->hdmirx_pwd_base + 0x8508, 0xF, 32, 0);
		disp_pr_err("ffe : max\n");
		hdmirx_frl_flt_update(hdmirx);
		frl_set_state(hdmirx, FRL_STATE_IDEL);
		return;
	}
	frl_ctx->ffe++;
	disp_pr_info("ffe : 0x%x\n", frl_ctx->ffe);
	frl_set_ltp_fixreq_bylane(hdmirx, lane);
}

int hdmirx_frl_ltp_pattern_check(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t temp;
	uint32_t offset = 0;
	uint32_t lane_done = 0;
	uint flag = 100;

	frl_lane lane;
	hdmirx_frl_ctx *frl_ctx = NULL;
	hdmirx_frl_ltp_state state;

	frl_ctx = hdmirx_frl_get_ctx();
	disp_pr_info("ltp pattern check\n");

	while(flag){
		flag --;
		lane_done = 0;
		offset = 0;
		temp = inp32(hdmirx->hdmirx_pwd_base + 0xB0C4);
		disp_pr_info("B0C4 : 0x%x\n", temp);
		for (lane = FRL_LANE_0; lane < 3; lane++) {
			state = hdmirx_frl_pattern_switch(temp, offset);
			if (state == HDMIRX_FRL_LTP_DONE)
				lane_done++;
			else if (state == HDMIRX_FRL_LTP_TIMEOUT)
				frl_pattern_timeout_proc(hdmirx, lane);

			offset += 2;
		}
		if (lane_done == 3) {
			frl_ctx->ltp_state = HDMIRX_FRL_LTP_DONE;
			frl_set_state(hdmirx, FRL_STATE_GAP_CHECK);

			set_reg(hdmirx->hdmirx_pwd_base + 0x8504, 0x0, 32, 0);
			set_reg(hdmirx->hdmirx_pwd_base + 0x8508, 0x0, 32, 0);  /* 4 lane */

			hdmirx_frl_flt_update(hdmirx);
			break;
		}
		mdelay(2);
	}
	return 0;
}

/* pattern check */
hdmirx_frl_ltp_state hdmirx_frl_pattern_switch(uint32_t temp, uint32_t offset)
{
	uint32_t state;
	state = temp >> offset;
	state = state & 0x3;
	switch (state) {
	case 1:
		disp_pr_info("Lane %d waiting\n", offset / 2);
		return HDMIRX_FRL_LTP_CHECKING;
		break;
	case 2:
		disp_pr_info("Lane %d detection OK\n", offset / 2);
		return HDMIRX_FRL_LTP_DONE;
		break;
	case 3:
		disp_pr_info("Lane %d detection timeout\n", offset / 2);
		return HDMIRX_FRL_LTP_TIMEOUT;
		break;
	default:
		disp_pr_info("Lane %d not tested\n", offset / 2);
		return HDMIRX_FRL_LTP_IDEL;
		break;
	}
	return HDMIRX_FRL_LTP_MAX;
}

/* Check Gap state */
hdmirx_frl_gap_state frl_get_gap_state(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t state;

	state = inp32(hdmirx->hdmirx_pwd_base + 0xB0C8);
	if (state == 0)   /* 0 : HDMIRX_FRL_GAP_IDEL */
		return HDMIRX_FRL_GAP_IDEL;
	else if (state == 1)    /* 1 : HDMIRX_FRL_GAP_CHECKING */
		return HDMIRX_FRL_GAP_CHECKING;
	else if (state == 2)    /* 2 : HDMIRX_FRL_GAP_DONE */
		return HDMIRX_FRL_GAP_DONE;
	else if (state == 3)    /* 3 : HDMIRX_FRL_GAP_TIMEOUT */
		return HDMIRX_FRL_GAP_TIMEOUT;
	else
		return HDMIRX_FRL_GAP_MAX;
}

int hdmirx_frl_gap_state_check(struct hdmirx_ctrl_st *hdmirx)
{
	uint32_t t = 0;
	hdmirx_frl_ctx *frl_ctx = NULL;
	frl_ctx = hdmirx_frl_get_ctx();
	disp_pr_info("gap check\n");

	while (t < 10) {
		if (frl_get_gap_state(hdmirx) == HDMIRX_FRL_GAP_DONE) {
			frl_set_state(hdmirx,FRL_STATE_FRL_TRANS);
			frl_ctx->gap_state = HDMIRX_FRL_GAP_DONE;
			hdmirx_frl_start(hdmirx);
			hdmirx_frl_status_update(hdmirx);
			hdmirx_link_training_done(hdmirx);
			return 0;
		}
		t++;
		mdelay(20);
	}
	frl_ctx->gap_state = HDMIRX_FRL_GAP_TIMEOUT;
	return 0;
}

int hdmirx_frl_start(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("frl start\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0x8440, 0x1, 1, 4);
	return 0;
}

int hdmirx_frl_status_update(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("status update\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0x8440, 0x1, 1, 0);
	return 0;
}

int hdmirx_link_training_done(struct hdmirx_ctrl_st *hdmirx)
{
	disp_pr_info("frl training done\n");
	set_reg(hdmirx->hdmirx_pwd_base + 0xB028, 0x1, 1, 5);
	set_reg(hdmirx->hdmirx_pwd_base + 0xB028, 0x0, 1, 5);
	return 0;
}

void frl_set_state(struct hdmirx_ctrl_st *hdmirx,frl_state state)
{
	hdmirx_frl_ctx *frl_ctx = NULL;
	frl_ctx = hdmirx_frl_get_ctx();
	if (frl_ctx->state != state) {
		disp_pr_info( "[HDMI] frl from %s to %s\n",g_frl_state_string[frl_ctx->state], g_frl_state_string[state]);
		frl_ctx->state = state;
	}
	if (state == FRL_STATE_PATTEN_CHECK) {
		frl_ctx->ltp_state = HDMIRX_FRL_LTP_CHECKING;
	} else if (state == FRL_STATE_GAP_CHECK) {
		frl_ctx->gap_state = HDMIRX_FRL_GAP_CHECKING;
	} else if (state == FRL_STATE_IDEL) {
		disp_pr_info("set state clear\n");
		frl_clear_init(hdmirx);
	}
}

void frl_clear_init(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_frl_ctx *frl_ctx = NULL;
	errno_t err_ret;
	disp_pr_info("frl clear\n");
	frl_ctx = hdmirx_frl_get_ctx();

	err_ret = memset_s(frl_ctx, sizeof(hdmirx_frl_ctx), 0, sizeof(hdmirx_frl_ctx));
	if (err_ret != EOK)
		return;

	frl_ctx->state = FRL_STATE_IDEL;
	frl_ctx->rate = HDMIRX_FRL_RATE_TMDS;
	frl_ctx->ffe = 0;
	frl_ctx->max_ffe = 0;
	frl_ctx->ltp_state = HDMIRX_FRL_LTP_IDEL;
	frl_ctx->gap_state = HDMIRX_FRL_GAP_IDEL;
	frl_ctx->force_req = false;
}

void frl_init_ltp_req(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_frl_ctx *frl_ctx = NULL;

	frl_ctx = hdmirx_frl_get_ctx();

	frl_ctx->ltp_req[FRL_LANE_0] = LTP5;
	frl_ctx->ltp_req[FRL_LANE_1] = LTP6;
	frl_ctx->ltp_req[FRL_LANE_2] = LTP7;
	frl_ctx->ltp_req[FRL_LANE_3] = LTP8;

	set_reg(hdmirx->hdmirx_pwd_base + 0xB0B0, frl_ctx->ltp_req[FRL_LANE_0], 4, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xB0B0, frl_ctx->ltp_req[FRL_LANE_1], 4, 5);
	set_reg(hdmirx->hdmirx_pwd_base + 0xB0B4, frl_ctx->ltp_req[FRL_LANE_2], 4, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0xB0B4, frl_ctx->ltp_req[FRL_LANE_3], 4, 5);
}

bool frl_get_interrupt(struct hdmirx_ctrl_st *hdmirx, hdmirx_frl_intr_type type)
{
	uint8_t state = 0;
	uint32_t reg_mask;

	reg_mask = (uint32_t)type & FRL_IRQ_MASK;

	if ((type >= HDMIRX_FRL_INT_SOURCE_FLT) && (type <= HDMIRX_FRL_INT_SOURCE_CLR_TIMEOUT)) {
		state = inp32(hdmirx->hdmirx_pwd_base + 0xB000) & reg_mask;
		disp_pr_info("interrupt 1: 0x%x\n", state);
	} else if ((type >= HDMIRX_FRL_INT_RSCC) && (type <= HDMIRX_FRL_INT_FRL_START)) {
		state = inp32(hdmirx->hdmirx_pwd_base + 0xB004) & reg_mask;
		disp_pr_info("interrupt 2: 0x%x\n", state);
	} else if ((type >= HDMIRX_FRL_INT_STATUS_UPDATE) && (type <= HDMIRX_FRL_INT_RS_UNCORR_OVER)) {
		state = inp32(hdmirx->hdmirx_pwd_base + 0xB008) & reg_mask;
		disp_pr_info("interrupt 3: 0x%x\n", state);
	} else if ((type >= HDMIRX_FRL_INT_LN3_NORM_DATA_ERR_EXC) && (type <= HDMIRX_FRL_INT_LN0_LTP_DATA_ERR_EXC)) {
		state = inp32(hdmirx->hdmirx_pwd_base + 0xB00C) & reg_mask;
		disp_pr_info("interrupt 4: 0x%x\n", state);
	} else if ((type >= HDMIRX_FRL_INT_BK_PREAMBLE_ERR) && (type <= HDMIRX_FRL_INT_RC_ERR)) {
		state = inp32(hdmirx->hdmirx_pwd_base + 0xB010) & reg_mask;
		disp_pr_info("interrupt 5: 0x%x\n", state);
	}

	if (state == 0)
		return false;

	return true;
}

void frl_clear_interrupt(struct hdmirx_ctrl_st *hdmirx, hdmirx_frl_intr_type type)
{
	uint32_t reg_value;

	reg_value = (uint32_t)type & FRL_IRQ_MASK;

	if ((type >= HDMIRX_FRL_INT_SOURCE_FLT) && (type <= HDMIRX_FRL_INT_SOURCE_CLR_TIMEOUT)) {
		set_reg(hdmirx->hdmirx_pwd_base + 0xB000, reg_value, 8, 0);
		disp_pr_info("clear b000\n");
	} else if ((type >= HDMIRX_FRL_INT_RSCC) && (type <= HDMIRX_FRL_INT_FRL_START)) {
		set_reg(hdmirx->hdmirx_pwd_base + 0xB004, reg_value, 8, 0);
		disp_pr_info("clear b004\n");
	} else if ((type >= HDMIRX_FRL_INT_STATUS_UPDATE) && (type <= HDMIRX_FRL_INT_RS_UNCORR_OVER)) {
		set_reg(hdmirx->hdmirx_pwd_base + 0xB008, reg_value, 8, 0);
		disp_pr_info("clear b008\n");
	} else if ((type >= HDMIRX_FRL_INT_LN3_NORM_DATA_ERR_EXC) && (type <= HDMIRX_FRL_INT_LN0_LTP_DATA_ERR_EXC)) {
		set_reg(hdmirx->hdmirx_pwd_base + 0xB00C, reg_value, 8, 0);
		disp_pr_info("clear b00c\n");
	} else if ((type >= HDMIRX_FRL_INT_BK_PREAMBLE_ERR) && (type <= HDMIRX_FRL_INT_RC_ERR)) {
		set_reg(hdmirx->hdmirx_pwd_base + 0xB010, reg_value, 8, 0);
		disp_pr_info("clear b010\n");
	}
}
