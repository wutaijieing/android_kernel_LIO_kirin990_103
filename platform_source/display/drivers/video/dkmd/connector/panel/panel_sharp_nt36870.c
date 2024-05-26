 /* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#include "dpu_conn_mgr.h"
#include "panel_dev.h"
#include "dsc_algorithm_manager.h"

#define MODE_3X_VIDEO 1

/*******************************************************************************
** Power ON/OFF Sequence(sleep mode to Normal mode) begin
*/
static char g_cmd_1[] = {
	0xFF,
	0xe0,
};
static char g_cmd_2[] = {
	0xfb,
	0x01,
};
static char g_cmd_3[] = {
	0x55,
	0x2f,
};
static char g_cmd_4[] = {
	0xFF,
	0x10,
};
static char g_cmd_5[] = {
	0x01,
	0x00,
};

static char g_cmd_6[] = {
	0xFF,
	0x10,
};
static char g_cmd_7[] = {
	0xfb,
	0x01,
};
static char g_cmd_8[] = {
	0xd7,
	0x01,
};
static char g_cmd_9[] = {
	0x2a,
	0x00, 0x00, 0x05, 0x9f,
};
static char g_cmd_10[] = {
	0x2b,
	0x00, 0x00, 0x09, 0xff,
};
static char g_cmd_11[] = {
	0xba,
	0x01,
};
static char g_cmd_12[] = {
	0xe5,
	0x01,
};
static char g_cmd_13[] = {
	0x35,
	0x00,
};
static char g_cmd_14[] = {
	0x44,
	0x09,
	0xfc,
};

static char g_cmd_16[] = {
	0xb0,
	0x01,
};
static char g_cmd_17[] = {
	0xFF,
	0xD0,
};
static char g_cmd_18[] = {
	0xFB,
	0x01,
};
static char g_cmd_19[] = {
	0x09,
	0xEE,
};
static char g_cmd_20[] = {
	0x28,
	0x50,
};
static char g_cmd_21[] = {
	0x53,
	0x88,
};
static char g_cmd_22[] = {
	0x54,
	0x08,
};
static char g_cmd_23[] = {
	0x1C,
	0x99,
};
static char g_cmd_24[] = {
	0x1D,
	0x09,
};
static char g_cmd_25[] = {
	0xFF,
	0x10,
};
static char g_cmd_26[] = {
	0xFB,
	0x01,
};
static char g_cmd_27[] = {
	0x51,
	0x0F,
	0xFF,
};
static char g_cmd_28[] = {
	0x53,
	0x2C,
};
static char g_cmd_29[] = {
	0x55,
	0x00,
};
static char g_cmd_30[] = {
	0x5E,
	0x00,
};
static char g_cmd_31[] = {
	0x61,
	0x00,
	0x00,
};
static char g_cmd_32[] = {
	0x63,
	0x24,
};
static char g_cmd_33[] = {
	0x65,
	0x01,
};

static char g_cmd_34[] = {
	0xFF,
	0xF0,

};
static char g_cmd_35[] = {
	0xFB,
	0x01,

};
static char g_cmd_36[] = {
	0xEF,
	0x80,
};
static char g_cmd_37[] = {
	0xFF,
	0x10,
};
static char g_cmd_38[] = {
	0xFB,
	0x01,
};

static char g_cmd_39[] = { // sleep out
	0x11,
	0x00,
};
static char g_cmd_40[] = {
	0x29,
	0x00,
};
static char g_cmd_41[] = {
	0xFF,
	0x2A,
};
static char g_cmd_42[] = {
	0xFB,
	0x01,
};
static char g_cmd_43[] = {
	0x42,
	0x4D,
};
static char g_cmd_44[] = {
	0xFF,
	0x23,
};

static char g_cmd_45[] = {
	0xFB,
	0x01,
};
static char g_cmd_46[] = {
	0x00,
	0x80,
};
static char g_cmd_47[] = {
	0x07,
	0x00,
};
static char g_cmd_48[] = {
	0x08,
	0x01,
};
static char g_cmd_49[] = {
	0x09,
	0x00,
};
static char g_cmd_50[] = {
	0xFF,
	0x26,
};
static char g_cmd_51[] = {
	0xFB,
	0x01,
};
static char g_cmd_52[] = {
	0x00,
	0xA1,
};
static char g_cmd_53[] = {
	0x03,
	0x02,
};
static char g_cmd_54[] = {
	0x04,
	0x20,
};
static char g_cmd_55[] = {
	0x01,
	0x10,
};
static char g_cmd_56[] = {
	0x07,
	0xD1,
};
static char g_cmd_57[] = {
	0x09,
	0x15,
};
static char g_cmd_58[] = {
	0x0C,
	0x03,
};
static char g_cmd_59[] = {
	0x0D,
	0x00,
};
static char g_cmd_60[] = {
	0x0E,
	0x01,
};
static char g_cmd_61[] = {
	0x0F,
	0x02,
};

static char g_cmd_62[] = {
	0x10,
	0x03,
};
static char g_cmd_63[] = {
	0x11,
	0x02,
};
static char g_cmd_64[] = {
	0x12,
	0x80,
};

static char g_cmd_65[] = {
	0x19,
	0x35,
};

static char g_cmd_66[] = {
	0x1A,
	0xF0,
};
static char g_cmd_67[] = {
	0x1B,
	0x00,
};
static char g_cmd_68[] = {
	0x1C,
	0xFA,
};

static char g_cmd_69[] = {
	0x1D,
	0x2E,
};
static char g_cmd_70[] = {
	0x1E,
	0x2B,
};
static char g_cmd_71[] = {
	0x23,
	0x01,
};
static char g_cmd_72[] = {
	0x24,
	0x00,
};
static char g_cmd_73[] = {
	0x25,
	0x01,
};
static char g_cmd_74[] = {
	0x26,
	0x05,
};
static char g_cmd_75[] = {
	0x27,
	0x06,
};
static char g_cmd_76[] = {
	0x28,
	0x05,
};
static char g_cmd_77[] = {
	0x29,
	0x00,
};
static char g_cmd_78[] = {
	0x2F,
	0x00,
};

static char g_cmd_79[] = {
	0x33,
	0x6B,
};

static char g_cmd_80[] = {
	0x34,
	0xE0,
};

static char g_cmd_81[] = {
	0x35,
	0x00,
};

static char g_cmd_82[] = {
	0x37,
	0x5C,
};

static char g_cmd_83[] = {
	0x38,
	0xCF,
};

static char g_cmd_84[] = {
	0x6F,
	0x00,
};

static char g_cmd_85[] = {
	0x70,
	0x00,
};

static char g_cmd_86[] = {
	0x71,
	0x40,
};

static char g_cmd_87[] = {
	0x72,
	0x00,
};

static char g_cmd_88[] = {
	0x78,
	0x00,
};

static char g_cmd_89[] = {
	0x80,
	0x05,
};

static char g_cmd_90[] = {
	0x81,
	0x1B,
};

static char g_cmd_91[] = {
	0x82,
	0x00,
};

static char g_cmd_92[] = {
	0x83,
	0x09,
};

static char g_cmd_93[] = {
	0x84,
	0x05,
};

static char g_cmd_94[] = {
	0x85,
	0x03,

};

static char g_cmd_95[] = {
	0x86,
	0x05,
};

static char g_cmd_96[] = {
	0x87,
	0x03,
};

static char g_cmd_97[] = {
	0x88,
	0x10,
};

static char g_cmd_98[] = {
	0x89,
	0x00,
};

static char g_cmd_99[] = {
	0x8A,
	0x12,
};

static char g_cmd_100[] = {
	0x8B,
	0x00,
};

static char g_cmd_101[] = {
	0x8C,
	0x33,
};

static char g_cmd_102[] = {
	0x8D,
	0x42,
};
static char g_cmd_103[] = {
	0x8E,
	0x24,
};

static char g_cmd_104[] = {
	0x8F,
	0x55,
};

static char g_cmd_105[] = {
	0x90,
	0x66,
};

static char g_cmd_106[] = {
	0x91,
	0x77,
};

static char g_cmd_107[] = {
	0x93,
	0x00,
};

static char g_cmd_108[] = {
	0x99,
	0x24,
};

static char g_cmd_109[] = {
	0x9A,
	0x81,
};

static char g_cmd_110[] = {
	0x9B,
	0x01,
};

static char g_cmd_111[] = {
	0x9C,
	0x01,
};

static char g_cmd_112[] = {
	0x9D,
	0x01,
};

static char g_cmd_113[] = {
	0x9E,
	0x01,
};

static char g_cmd_114[] = {
	0xC3,
	0x00,
};

static char g_cmd_115[] = {
	0xC8,
	0x0A,
};

static char g_cmd_116[] = {
	0xC9,
	0x00,
};

static char g_cmd_117[] = {
	0xD0,
	0xD7,
};

static char g_cmd_118[] = {
	0xD1,
	0xC1,
};

static char g_cmd_119[] = {
	0xC4,
	0x00,
};

static char g_cmd_120[] = {
	0xC5,
	0x01,
};

static char g_cmd_121[] = {
	0xCC,
	0x00,
};

static char g_cmd_122[] = {
	0xD4,
	0xBA,
};
static char g_cmd_123[] = {
	0xD5,
	0x17,
};

static char g_cmd_124[] = {
	0xFF,
	0x26,
};

static char g_cmd_125[] = {
	0xFB,
	0x01,
};

static char g_cmd_126[] = {
	0xDB,
	0x03,
};

static char g_cmd_127[] = {
	0xDC,
	0x00,
};

static char g_cmd_128[] = {
	0xDD,
	0x01,
};

static char g_cmd_129[] = {
	0xDE,
	0x02,
};

static char g_cmd_130[] = {
	0xDF,
	0x03,
};

static char g_cmd_131[] = {
	0xE0,
	0x02,
};

static char g_cmd_132[] = {
	0xE1,
	0x80,
};

static char g_cmd_133[] = {
	0xE8,
	0x6B,
};

static char g_cmd_134[] = {
	0xE9,
	0xE0,
};

static char g_cmd_135[] = {
	0xEC,
	0x2E,
};

static char g_cmd_136[] = {
	0xED,
	0x2B,
};

#ifdef FPGA_FLAG
static char g_cmd_f0[] = {
	0xFF,
	0xF0,
};

static char g_cmd_fb[] = {
	0xFB,
	0x01,
};

static char g_cmd_d7[] = {
	0xD7,
	0x04,
};

static char g_cmd_9c[] = {
	0x9C,
	0x00,
};

static char g_cmd_2d[] = {
	0x2D,
	0x10,
};
#endif

static char g_cmd_137[] = {
	0xFF,
	0x10,
};

static char g_cmd_138[] = {
	0xFB,
	0x01,
};

static char g_cmd_pg25_1[] = {
	0xff,
	0x25,
};

static char g_cmd_pg25_2[] = {
	0xfb,
	0x01,
};

static char g_cmd_pg25_3[] = {
	0x05,
	0x00,
};

static char g_cmd_pg25_4[] = {
	0x0a,
	0x80,
};

static char g_cmd_pg25_5[] = {
	0x0b,
	0x8C,
};

static char g_cmd_pg25_6[] = {
	0x0c,
	0x01,
};

static char g_cmd_pg24_1[] = {
	0xff,
	0x24,
};

static char g_cmd_pg24_2[] = {
	0xfb,
	0x01,
};

// 0xC6-en vid drop
static char g_cmd_pg24_3[] = {
	0xC2,
	0xC6,
};

static char g_cmd_pg24_4[] = {
	0xC1,
	0x13,
};

static char g_compression_select[] = {
	0xC0,
	0x03,
};

static char g_vesa3x_10bit_video_slice8l_c3[] = {
	0xC3,
	0xAB,0x2A,0x00,0x08,0x02,0x00,0x02,0x20,0x01,0x04,0x00,0x0E,0x0D,0xB7,0x0B,0xAB,
};
static char g_vesa3x_10bit_video_slice8l_c4[] = {
	0xC4,
	0x0C,0xF0,
};

static char g_vesa3x_8bit_slice8l_c1[] = {
	0xC1,
	0x89,0x28,0x00,0x08,0x02,0x00,0x02,0x68,0x00,0xD5,0x00,0x0A,0x0D,0xB7,0x09,0x89,
};

static char g_vesa3x_8bit_slice8l_c2[] = {
	0xC2,
	0x10,0xF0,
};

/* vesa3_75x_slice8L_C1:
 *	0xC1,
 *	0xAB,0x28,0x00,0x08,0x02,0x00,0x02,0x68,0x00,0xD5,0x00,0x0A,0x0D,0xB7,0x09,0x89,
 *
 * vesa3_75x_slice8L_C2:
 *	0xC2,
 *	0x10,0xF0,
 */
static char g_te_output[] = {
	0xB3,
	0x21,
};
static char g_frm_delay[] = {
	0xB4,
	0x21,
};

static char g_video_mode[] = {
	0xBB,
	0x13,
};

/* Param[1:2] = (VBP+VSA)[0:7]  VFP[0:7]
 * tmp == 2:0x03,0x0A,0x0A,
 * tmp == 3:0x03,0x32,0x1e,
 * tmp == 4:0x03,0x58,0x1C,
 */
static char g_video_control[] = {
	0x3B,
	0x03,0x58,0x1C,
};

#ifdef MODE_3X_CMD
static char g_cmd_mode[] = {
	0xBB,
	0x10,
};

static char g_get_compression_mode_8bit[] = {
	0x03,
	0x00,
};
#endif

static char g_get_compression_mode_10bit[] = {
	0x03,
	0x10,
};

static char g_pwm_out_0x51[] = {
	0x51,
	0xFE,
};

static struct dsi_cmd_desc g_lcd_display_on_cmds[] = {
	// bl_type: BL_SET_BY_BLPWM or BL_SET_BY_SH_BLPWM
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_pwm_out_0x51), g_pwm_out_0x51 },

	// lcd_display_init_cmds_part1
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_1), g_cmd_1},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_2), g_cmd_2},
	{DTYPE_DCS_LWRITE, 0,1, WAIT_TYPE_MS, sizeof(g_cmd_3), g_cmd_3},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_4), g_cmd_4},
	{DTYPE_DCS_WRITE, 0,20, WAIT_TYPE_MS, sizeof(g_cmd_5), g_cmd_5},

	// disable_auto_vbpvfp_en_nodrop_cmds
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg25_1), g_cmd_pg25_1},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg25_2), g_cmd_pg25_2},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg25_3), g_cmd_pg25_3},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg25_4), g_cmd_pg25_4},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg25_5), g_cmd_pg25_5},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg25_6), g_cmd_pg25_6},

	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg24_1), g_cmd_pg24_1},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg24_2), g_cmd_pg24_2},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg24_3), g_cmd_pg24_3},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_pg24_4), g_cmd_pg24_4},

	// lcd_display_init_cmds_part2
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_6), g_cmd_6},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_7), g_cmd_7},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_8), g_cmd_8},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_9), g_cmd_9},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_10), g_cmd_10},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_11), g_cmd_11},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_12), g_cmd_12},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_13), g_cmd_13},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_14), g_cmd_14},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_16), g_cmd_16},

	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_compression_select), g_compression_select},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_video_control), g_video_control},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_te_output), g_te_output},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_frm_delay), g_frm_delay},

	// lcd_display_init_vesa3x_para
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_vesa3x_8bit_slice8l_c1), g_vesa3x_8bit_slice8l_c1},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_vesa3x_8bit_slice8l_c2), g_vesa3x_8bit_slice8l_c2},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_vesa3x_10bit_video_slice8l_c3), g_vesa3x_10bit_video_slice8l_c3},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_vesa3x_10bit_video_slice8l_c4), g_vesa3x_10bit_video_slice8l_c4},

#ifdef MODE_3X_VIDEO
	// lcd_display_init_10bit_video_3x_dsc_cmds
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_video_mode), g_video_mode},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_get_compression_mode_10bit), g_get_compression_mode_10bit},
#else // MODE_3X_CMD
	// lcd_display_init_8bit_cmd_3x_dsc_cmds
	{ DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_mode), g_cmd_mode },
	{ DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_get_compression_mode_8bit), g_get_compression_mode_8bit },
#endif

	// lcd_display_init_cmds_part3
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_17), g_cmd_17},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_18), g_cmd_18},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_19), g_cmd_19},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_20), g_cmd_20},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_21), g_cmd_21},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_22), g_cmd_22},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_23), g_cmd_23},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_24), g_cmd_24},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_25), g_cmd_25},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_26), g_cmd_26},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_27), g_cmd_27},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_28), g_cmd_28},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_29), g_cmd_29},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_30), g_cmd_30},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_31), g_cmd_31},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_32), g_cmd_32},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_33), g_cmd_33},
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_34), g_cmd_34},
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_35), g_cmd_35},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_36), g_cmd_36},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_37), g_cmd_37},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_38), g_cmd_38},
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_MS, sizeof(g_cmd_39), g_cmd_39},
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_US, sizeof(g_cmd_40), g_cmd_40},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_41), g_cmd_41},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_42), g_cmd_42},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_43), g_cmd_43},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_44), g_cmd_44},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_45), g_cmd_45},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_46), g_cmd_46},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_47), g_cmd_47},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_48), g_cmd_48},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_49), g_cmd_49},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_50), g_cmd_50},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_51), g_cmd_51},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_52), g_cmd_52},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_53), g_cmd_53},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_54), g_cmd_54},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_55), g_cmd_55},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_56), g_cmd_56},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_57), g_cmd_57},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_58), g_cmd_58},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_59), g_cmd_59},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_60), g_cmd_60},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_61), g_cmd_61},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_62), g_cmd_62},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_63), g_cmd_63},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_64), g_cmd_64},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_65), g_cmd_65},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_66), g_cmd_66},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_67), g_cmd_67},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_68), g_cmd_68},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_69), g_cmd_69},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_70), g_cmd_70},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_71), g_cmd_71},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_72), g_cmd_72},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_73), g_cmd_73},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_74), g_cmd_74},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_75), g_cmd_75},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_76), g_cmd_76},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_77), g_cmd_77},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_78), g_cmd_78},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_79), g_cmd_79},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_80), g_cmd_80},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_81), g_cmd_81},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_82), g_cmd_82},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_83), g_cmd_83},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_84), g_cmd_84},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_85), g_cmd_85},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_86), g_cmd_86},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_87), g_cmd_87},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_88), g_cmd_88},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_89), g_cmd_89},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_90), g_cmd_90},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_91), g_cmd_91},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_92), g_cmd_92},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_93), g_cmd_93},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_94), g_cmd_94},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_95), g_cmd_95},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_96), g_cmd_96},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_97), g_cmd_97},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_98), g_cmd_98},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_99), g_cmd_99},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_100), g_cmd_100},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_101), g_cmd_101},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_102), g_cmd_102},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_103), g_cmd_103},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_104), g_cmd_104},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_105), g_cmd_105},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_106), g_cmd_106},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_107), g_cmd_107},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_108), g_cmd_108},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_109), g_cmd_109},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_110), g_cmd_110},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_111), g_cmd_111},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_112), g_cmd_112},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_113), g_cmd_113},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_114), g_cmd_114},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_115), g_cmd_115},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_116), g_cmd_116},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_117), g_cmd_117},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_118), g_cmd_118},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_119), g_cmd_119},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_120), g_cmd_120},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_121), g_cmd_121},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_122), g_cmd_122},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_123), g_cmd_123},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_124), g_cmd_124},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_125), g_cmd_125},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_126), g_cmd_126},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_127), g_cmd_127},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_128), g_cmd_128},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_129), g_cmd_129},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_130), g_cmd_130},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_131), g_cmd_131},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_132), g_cmd_132},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_133), g_cmd_133},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_134), g_cmd_134},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_135), g_cmd_135},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_136), g_cmd_136},

#ifdef FPGA_FLAG
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_f0), g_cmd_f0},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_fb), g_cmd_fb},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_d7), g_cmd_d7},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_9c), g_cmd_9c},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_2d), g_cmd_2d},
#endif

	// lcd_display_init_cmds_part5
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_137), g_cmd_137},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US, sizeof(g_cmd_138), g_cmd_138},
};

static char g_display_off[] = {
	0x28,
};
static char g_sleep_in[] = {
	0x10,
};

static struct dsi_cmd_desc g_lcd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 60, WAIT_TYPE_MS, sizeof(g_display_off), g_display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_sleep_in), g_sleep_in},
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME "lcdio-vcc"
#define VCC_LCDANALOG_NAME "lcdanalog-vcc"

static struct regulator *g_vcc_lcdio;
static struct regulator *g_vcc_lcdanalog;

static struct vcc_desc g_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &g_vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &g_vcc_lcdio, 1850000, 1850000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc g_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc g_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc g_lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
};

/*******************************************************************************
** LCD IOMUX
*/
static struct pinctrl_data g_pctrl;

static struct pinctrl_cmd_desc g_lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &g_pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &g_pctrl, 0},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_P5V8_ENABLE_NAME "gpio_lcd_p5v8_enable"
#define GPIO_LCD_N5V8_ENABLE_NAME "gpio_lcd_n5v8_enable"
#define GPIO_LCD_RESET_NAME "gpio_lcd_reset"
#define GPIO_LCD_BL_ENABLE_NAME "gpio_lcd_bl_enable"
#define GPIO_LCD_TP1V8_NAME "gpio_lcd_1v8"

static uint32_t g_gpio_lcd_p5v8_enable;
static uint32_t g_gpio_lcd_n5v8_enable;
static uint32_t g_gpio_lcd_reset;
static uint32_t g_gpio_lcd_bl_enable;
static uint32_t g_gpio_lcd_1v8;

#define GPIO_LCD_P5V5_ENABLE_NAME "gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_TE0_NAME "gpio_lcd_te0"
static uint32_t g_gpio_lcd_p5v5_enable;
static uint32_t g_gpio_lcd_n5v5_enable;
static uint32_t g_gpio_lcd_te0;

static struct gpio_desc g_fpga_lcd_gpio_request_cmds[] = {
	/* AVDD_5.8V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_P5V8_ENABLE_NAME, &g_gpio_lcd_p5v8_enable, 0},
	/* AVEE_-5.8V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_N5V8_ENABLE_NAME, &g_gpio_lcd_n5v8_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_TP1V8_NAME, &g_gpio_lcd_1v8, 0},
};

static struct gpio_desc g_fpga_lcd_gpio_on_cmds[] = {
	/* lcd_1.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10, GPIO_LCD_TP1V8_NAME, &g_gpio_lcd_1v8, 1},
	/* AVDD_5.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10, GPIO_LCD_P5V8_ENABLE_NAME, &g_gpio_lcd_p5v8_enable, 1},
	/* AVEE_-5.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_N5V8_ENABLE_NAME, &g_gpio_lcd_n5v8_enable, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 1},
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 1},
};

static struct gpio_desc g_fpga_lcd_gpio_off_cmds[] = {
	/* backlight disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 0},
	/* AVEE_-5.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3, GPIO_LCD_N5V8_ENABLE_NAME, &g_gpio_lcd_n5v8_enable, 0},
	/* AVDD_5.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3, GPIO_LCD_P5V8_ENABLE_NAME, &g_gpio_lcd_p5v8_enable, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
	/* lcd_1.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0, GPIO_LCD_TP1V8_NAME, &g_gpio_lcd_1v8, 0},

	/* backlight enable input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 0},
	/* AVEE_-5.8V input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0, GPIO_LCD_N5V8_ENABLE_NAME, &g_gpio_lcd_n5v8_enable, 0},
	/* AVDD_5.8V input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0, GPIO_LCD_P5V8_ENABLE_NAME, &g_gpio_lcd_p5v8_enable, 0},
	/* reset input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
	/* lcd_1.8V */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0, GPIO_LCD_TP1V8_NAME, &g_gpio_lcd_1v8, 0},

};

static struct gpio_desc g_fpga_lcd_gpio_free_cmds[] = {
	/* backlight enable */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 0},
	/* AVEE_-5.8V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_N5V8_ENABLE_NAME, &g_gpio_lcd_n5v8_enable, 0},
	/* AVDD_5.8V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_P5V8_ENABLE_NAME, &g_gpio_lcd_p5v8_enable, 0},
	/* lcd_1.8V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_TP1V8_NAME, &g_gpio_lcd_1v8, 0},
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
};


static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_P5V5_ENABLE_NAME, &g_gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_N5V5_ENABLE_NAME, &g_gpio_lcd_n5v5_enable, 0},

	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
};

static struct gpio_desc g_asic_lcd_gpio_normal_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_LCD_P5V5_ENABLE_NAME, &g_gpio_lcd_p5v5_enable, 1},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_N5V5_ENABLE_NAME, &g_gpio_lcd_n5v5_enable, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 1},

	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 1},
};

static struct gpio_desc g_asic_lcd_gpio_free_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_P5V5_ENABLE_NAME, &g_gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_N5V5_ENABLE_NAME, &g_gpio_lcd_n5v5_enable, 0},

	/* backlight enable */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_BL_ENABLE_NAME, &g_gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_RESET_NAME, &g_gpio_lcd_reset, 0},
};

/*******************************************************************************
 */
static void panel_drv_private_data_setup(struct panel_drv_private *priv, struct device_node *np)
{
	if (priv->connector_info.base.fpga_flag == 1) {
		g_gpio_lcd_p5v8_enable = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_lcd_n5v8_enable = (uint32_t)of_get_named_gpio(np, "gpios", 2);
		g_gpio_lcd_reset = (uint32_t)of_get_named_gpio(np, "gpios", 3);
		g_gpio_lcd_bl_enable = (uint32_t)of_get_named_gpio(np, "gpios", 4);
		g_gpio_lcd_1v8 = (uint32_t)of_get_named_gpio(np, "gpios", 0);

		dpu_pr_info("used gpio:[p5v8: %d, n5v8: %d, rst: %d, bl: %d, 1v8: %d]\n",
			g_gpio_lcd_p5v8_enable, g_gpio_lcd_n5v8_enable, g_gpio_lcd_reset, g_gpio_lcd_bl_enable, g_gpio_lcd_1v8);

		priv->gpio_request_cmds = g_fpga_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_fpga_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_free_cmds);

		priv->gpio_normal_cmds = g_fpga_lcd_gpio_on_cmds;
		priv->gpio_normal_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_on_cmds);
		priv->gpio_lowpower_cmds = g_fpga_lcd_gpio_off_cmds;
		priv->gpio_lowpower_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_off_cmds);
	} else {
		g_gpio_lcd_p5v5_enable = (uint32_t)of_get_named_gpio(np, "gpios", 0);
		g_gpio_lcd_n5v5_enable = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_lcd_reset = (uint32_t)of_get_named_gpio(np, "gpios", 2);
		g_gpio_lcd_bl_enable = (uint32_t)of_get_named_gpio(np, "gpios", 3);
		g_gpio_lcd_te0 = (uint32_t)of_get_named_gpio(np, "gpios", 4);

		dpu_pr_info("used gpio:[p5v5: %d, n5v5: %d, rst: %d, bl: %d, te0: %d]\n",
			g_gpio_lcd_p5v5_enable, g_gpio_lcd_n5v5_enable, g_gpio_lcd_reset, g_gpio_lcd_bl_enable, g_gpio_lcd_te0);

		priv->gpio_request_cmds = asic_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = ARRAY_SIZE(asic_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_asic_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_free_cmds);

		priv->gpio_normal_cmds = g_asic_lcd_gpio_normal_cmds;
		priv->gpio_normal_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_normal_cmds);
		priv->gpio_lowpower_cmds = NULL;
		priv->gpio_lowpower_cmds_len = 0;

		priv->vcc_init_cmds = g_lcd_vcc_init_cmds;
		priv->vcc_init_cmds_len = ARRAY_SIZE(g_lcd_vcc_init_cmds);
		priv->vcc_finit_cmds = g_lcd_vcc_finit_cmds;
		priv->vcc_finit_cmds_len = ARRAY_SIZE(g_lcd_vcc_finit_cmds);

		priv->vcc_enable_cmds = g_lcd_vcc_enable_cmds;
		priv->vcc_enable_cmds_len = ARRAY_SIZE(g_lcd_vcc_enable_cmds);
		priv->vcc_disable_cmds = g_lcd_vcc_disable_cmds;
		priv->vcc_disable_cmds_len = ARRAY_SIZE(g_lcd_vcc_disable_cmds);

		priv->pinctrl_init_cmds = g_lcd_pinctrl_init_cmds;
		priv->pinctrl_init_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_init_cmds);
		priv->pinctrl_finit_cmds = g_lcd_pinctrl_finit_cmds;
		priv->pinctrl_finit_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_finit_cmds);

		priv->pinctrl_normal_cmds = g_lcd_pinctrl_normal_cmds;
		priv->pinctrl_normal_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_normal_cmds);
		priv->pinctrl_lowpower_cmds = NULL;
		priv->pinctrl_lowpower_cmds_len = 0;
	}
}

static void dsc_config_initial(struct dkmd_connector_info *pinfo, struct dsc_calc_info *dsc)
{
	struct input_dsc_info input_dsc_info;
	struct dsc_algorithm_manager *pt = get_dsc_algorithm_manager_instance();

	dpu_pr_info("+\n");
	dpu_check_and_no_retval((pt == NULL) || (pinfo == NULL), err, "[DSC] NULL Pointer");

	pinfo->ifbc_type = IFBC_TYPE_VESA3X_DUAL;

	input_dsc_info.dsc_version = DSC_VERSION_V_1_1;
	input_dsc_info.format = DSC_RGB;
	input_dsc_info.pic_width = pinfo->base.xres;
	input_dsc_info.pic_height = pinfo->base.yres;
	input_dsc_info.slice_width = input_dsc_info.pic_width / 2;

	// slice height is same with DDIC
	input_dsc_info.slice_height = 7 + 1;
	if (pinfo->base.type == PANEL_MIPI_CMD) {
		input_dsc_info.dsc_bpp = DSC_8BPP;
		input_dsc_info.dsc_bpc = DSC_8BPC;
		input_dsc_info.linebuf_depth = LINEBUF_DEPTH_9;
	} else { // video
		input_dsc_info.dsc_bpp = DSC_10BPP;
		input_dsc_info.dsc_bpc = DSC_10BPC;
		input_dsc_info.linebuf_depth = LINEBUF_DEPTH_11;
	}
	input_dsc_info.block_pred_enable = 1;
	input_dsc_info.gen_rc_params = DSC_NOT_GENERATE_RC_PARAMETERS;
	pt->vesa_dsc_info_calc(&input_dsc_info, &(dsc->dsc_info), NULL);
	dsc->dsc_en = 1;

	if (pinfo->base.type == PANEL_MIPI_CMD) {
		// DSC_RC_RANGE_PARAM5, 0x2af62b34
		dsc->dsc_info.rc_range[10].min_qp = (0x2af62b34 >> 27) & 0x1F;
		dsc->dsc_info.rc_range[10].max_qp = (0x2af62b34 >> 22) & 0x1F;
		dsc->dsc_info.rc_range[10].offset = (0x2af62b34 >> 16) & 0x3F;
		dsc->dsc_info.rc_range[11].min_qp = (0x2af62b34 >> 11) & 0x1F;
		dsc->dsc_info.rc_range[11].max_qp = (0x2af62b34 >> 6) & 0x1F;
		dsc->dsc_info.rc_range[11].offset = (0x2af62b34 >> 0) & 0x3F;
		// DSC_RC_RANGE_PARAM6, 0x2b743b74
		dsc->dsc_info.rc_range[12].min_qp = (0x2b743b74 >> 27) & 0x1F;
		dsc->dsc_info.rc_range[12].max_qp = (0x2b743b74 >> 22) & 0x1F;
		dsc->dsc_info.rc_range[12].offset = (0x2b743b74 >> 16) & 0x3F;
		dsc->dsc_info.rc_range[13].min_qp = (0x2b743b74 >> 11) & 0x1F;
		dsc->dsc_info.rc_range[13].max_qp = (0x2b743b74 >> 6) & 0x1F;
		dsc->dsc_info.rc_range[13].offset = (0x2b743b74 >> 0) & 0x3F;
		// DSC_RC_RANGE_PARAM7, 0x6bf40000
		dsc->dsc_info.rc_range[14].min_qp = (0x6bf40000 >> 27) & 0x1F;
		dsc->dsc_info.rc_range[14].max_qp = (0x6bf40000 >> 22) & 0x1F;
		dsc->dsc_info.rc_range[14].offset = (0x6bf40000 >> 16) & 0x3F;
	}
	dpu_pr_info("-\n");
}

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct dkmd_connector_info *pinfo, struct mipi_panel_info *mipi)
{
	if (pinfo->base.fpga_flag == 1) {
		mipi->hsa = 7;
		mipi->hbp = 12;
		mipi->dpi_hsize = 812;
		mipi->hline_time = 840;
		mipi->vsa = 10;
		mipi->vbp = 58;
		mipi->vfp = 80;

		mipi->pxl_clk_rate = 20 * 1000000UL;
		mipi->dsi_bit_clk = 240;
	} else {
		if (pinfo->base.type == PANEL_MIPI_CMD) {
			mipi->hsa = 40;
			mipi->hbp = 40;
			mipi->dpi_hsize = 570;
			mipi->hline_time = 718;
			mipi->vsa = 18;
			mipi->vbp = 70;
			mipi->vfp = 28;

			mipi->pxl_clk_rate = 332 * 1000000UL;
			mipi->dsi_bit_clk = 850;
		} else { // PANEL_MIPI_VIDEO
			mipi->hsa = 15;
			mipi->hbp = 15;
			mipi->dpi_hsize = 458;
			mipi->hline_time = 718;
			mipi->vsa = 18;
			mipi->vbp = 70;
			mipi->vfp = 28;

			mipi->pxl_clk_rate = 332 * 1000000UL;
			mipi->dsi_bit_clk = 850;
		}
	}

	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;

	// IFBC_TYPE_NONE: pxl_clk_rate_div = 1, VESA3X or VESA3_75X: pxl_clk_rate_div = 3
	mipi->pxl_clk_rate_div = 3;
	mipi->dsi_bit_clk_upt_support = 0;

	mipi->clk_post_adjust = 215;
	mipi->lane_nums = DSI_2_LANES;
	mipi->color_mode = DSI_24BITS_1;

	mipi->dsi_version = DSI_1_1_VERSION;
	mipi->phy_mode = CPHY_MODE;

	mipi->vc = 0;
	mipi->max_tx_esc_clk = 10 * 1000000;
	mipi->burst_mode = DSI_BURST_SYNC_PULSES_1;
	mipi->non_continue_en = 1;
}
/* dirty region initialized value from panel spec */
static void lcd_init_dirty_region(struct panel_drv_private *priv)
{
	priv->dirty_region_info.left_align = -1;
	priv->dirty_region_info.right_align = -1;
	priv->dirty_region_info.top_align = 32;
	priv->dirty_region_info.bottom_align = 32;
	priv->dirty_region_info.w_align = -1;
	priv->dirty_region_info.h_align = -1;
	priv->dirty_region_info.w_min = 1440;
	priv->dirty_region_info.h_min = 32;
	priv->dirty_region_info.top_start = -1;
	priv->dirty_region_info.bottom_start = -1;
}

static int32_t panel_of_device_setup(struct panel_drv_private *priv)
{
	int32_t ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;
	struct device_node *np = priv->pdev->dev.of_node;

	dpu_pr_info("enter!\n");

	/* Inheritance based processing */
	panel_base_of_device_setup(priv);
	panel_drv_private_data_setup(priv, np);

#ifdef MODE_3X_VIDEO
	pinfo->base.type = PANEL_MIPI_VIDEO;
#else
	pinfo->base.type = PANEL_MIPI_CMD;
#endif
	dpu_pr_info("panel_type=%#x", pinfo->base.type);
	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 1440; // FIXME: Modified for new panel device
	pinfo->base.yres = 2560; // FIXME: Modified for new panel device

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 75; // FIXME: Modified for new panel device
	pinfo->base.height = 133; // FIXME: Modified for new panel device

	// TODO: caculate fps by mipi timing para
	pinfo->base.fps = 60;

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	mipi_lcd_init_dsi_param(pinfo, &get_primary_connector(pinfo)->mipi);

	dsc_config_initial(pinfo, &get_primary_connector(pinfo)->dsc);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 12;
	priv->bl_info.bl_max = 2047;
	priv->bl_info.bl_default = 1000;

	lcd_init_dirty_region(priv);

	priv->disp_on_cmds = g_lcd_display_on_cmds;
	priv->disp_on_cmds_len = ARRAY_SIZE(g_lcd_display_on_cmds);
	priv->disp_off_cmds = g_lcd_display_off_cmds;
	priv->disp_off_cmds_len = ARRAY_SIZE(g_lcd_display_off_cmds);

	if (pinfo->base.fpga_flag == 0) {
		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_init_cmds, priv->vcc_init_cmds_len);
		if (ret != 0)
			dpu_pr_info("vcc init failed!\n");

		ret = peri_pinctrl_cmds_tx(priv->pdev, priv->pinctrl_init_cmds, priv->pinctrl_init_cmds_len);
		if (ret != 0)
			dpu_pr_info("pinctrl init failed\n");

		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_enable_cmds, priv->vcc_enable_cmds_len);
		if (ret)
			dpu_pr_warn("vcc enable cmds handle fail!\n");
	}

	dpu_pr_info("exit!\n");

	return 0;
}

static void panel_of_device_release(struct panel_drv_private *priv)
{
	int32_t ret = 0;

	panel_base_of_device_release(priv);

	if (priv->gpio_free_cmds && (priv->gpio_free_cmds_len > 0)) {
		ret = peri_gpio_cmds_tx(priv->gpio_free_cmds, priv->gpio_free_cmds_len);
		if (ret)
			dpu_pr_info("gpio free handle err!\n");
	}

	dpu_pr_info("exit!\n");
}

panel_device_match_data(nt36870_panel_info, PANEL_NT36870_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");