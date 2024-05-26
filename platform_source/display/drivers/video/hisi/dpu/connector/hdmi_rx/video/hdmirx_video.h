/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HDMIRX_VIDEO_H
#define HDMIRX_VIDEO_H

#include "hdmirx_struct.h"
#include "hdmirx_common.h"

typedef enum video_timing_idx_en {
	TIMING_1_640X480P,
	TIMING_2_3_720X480P,
	TIMING_4_69_1280X720P,
	TIMING_5_1920X1080I,             /* 3 */
	TIMING_6_7_720_1440X480I,        /* 4 */
	TIMING_8_9_720_1440X240P_1,      /* 5 */
	TIMING_8_9_720_1440X240P_2,      /* 6 */
	TIMING_10_11_2880X480I,          /* 7 */
	TIMING_12_13_2880X240P_1,        /* 8 */
	TIMING_12_13_2880X240P_2,        /* 9 */
	TIMING_14_15_1440X480P,          /* 10 */
	TIMING_16_76_1920X1080P,         /* 11 */
	TIMING_17_18_720X576P,           /* 12 */
	TIMING_19_68_1280X720P,          /* 13 */
	TIMING_20_1920X1080I,            /* 14 */
	TIMING_21_22_720_1440X576I,      /* 15 */
	TIMING_23_24_720_1440X288P_1,    /* 16 */
	TIMING_23_24_720_1440X288P_2,    /* 17 */
	TIMING_23_24_720_1440X288P_3,    /* 18 */
	TIMING_25_26_2880X576I,          /* 19 */
	TIMING_27_28_2880X288P_1,        /* 20 */
	TIMING_27_28_2880X288P_2,        /* 21 */
	TIMING_27_28_2880X288P_3,        /* 22 */
	TIMING_29_30_1440X576P,          /* 23 */
	TIMING_31_75_1920X1080P,         /* 24 */
	TIMING_32_72_1920X1080P,         /* 25 */
	TIMING_33_73_1920X1080P,         /* 26 */
	TIMING_34_74_1920X1080P,         /* 27 */
	TIMING_35_36_2880X480P,          /* 28 */
	TIMING_37_38_2880X576P,          /* 29 */
	TIMING_39_1920X1080I_1250_TOTAL, /* 30 */
	TIMING_40_1920X1080I,            /* 31 */
	TIMING_41_70_1280X720P,          /* 32 */
	TIMING_42_43_720X576P,           /* 33 */
	TIMING_44_45_720_1440X576I,      /* 34 */
	TIMING_46_1920X1080I,            /* 35 */
	TIMING_47_71_1280X720P,          /* 36 */
	TIMING_48_49_720X480P,           /* 37 */
	TIMING_50_51_720_1440X480I,      /* 38 */
	TIMING_52_53_720X576P,           /* 39 */
	TIMING_54_55_720_1440X576I,      /* 40 */
	TIMING_56_57_720X480P,           /* 41 */
	TIMING_58_59_720_1440X480I,      /* 42 */
	TIMING_60_65_1280X720_24,        /* 43 */
	TIMING_61_66_1280X720_25,        /* 44 */
	TIMING_62_67_1280X720_30,        /* 45 */
	TIMING_63_78_1920X1080P_120,     /* 46 */
	TIMING_64_77_1920X1080P_100,     /* 47 */
	TIMING_79_1680X720P_24,          /* 48 */
	TIMING_80_1680X720P_25,          /* 49 */
	TIMING_81_1680X720P_30,          /* 50 */
	TIMING_82_1680X720P_50,          /* 51 */
	TIMING_83_1680X720P_60,          /* 52 */
	TIMING_84_1680X720P_100,         /* 53 */
	TIMING_85_1680X720P_120,         /* 54 */
	TIMING_86_2560X1080P_24,         /* 55 */
	TIMING_87_2560X1080P_25,         /* 56 */
	TIMING_88_2560X1080P_30,         /* 57 */
	TIMING_89_2560X1080P_50,         /* 58 */
	TIMING_90_2560X1080P_60,         /* 59 */
	TIMING_91_2560X1080P_100,        /* 60 */
	TIMING_92_2560X1080P_120,        /* 61 */
	TIMING_93_103_3840X2160P_24,     /* 62 */
	TIMING_94_104_3840X2160P_25,     /* 63 */
	TIMING_95_105_3840X2160P_30,     /* 64 */
	TIMING_96_106_3840X2160P_50,     /* 65 */
	TIMING_97_107_3840X2160P_60,     /* 66 */
	TIMING_98_4096X2160P_24,         /* 67 */
	TIMING_99_4096X2160P_25,         /* 68 */
	TIMING_100_4096X2160P_30,        /* 69 */
	TIMING_101_4096X2160P_50,        /* 70 */
	TIMING_102_4096X2160P_60,        /* 71 */
	TIMING_117_119_3840X2160P_100,   /* 72 */
	TIMING_118_120_3840X2160P_120,   /* 73 */
	TIMING_218_4096X2160P_100,       /* 74 */
	TIMING_219_4096X2160P_120,       /* 75 */
	TIMING_194_7680X4320P_24,        /* 76 */
	TIMING_195_7680X4320P_25,        /* 77 */
	TIMING_196_204_7680X4320P_30,        /* 78 */
	TIMING_198_206_7680X4320P_50,        /* 79 */
	TIMING_199_207_7680X4320P_60,        /* 80 */
	TIMING_200_208_7680X4320P_100,       /* 81 */
	TIMING_201_209_7680X4320P_120,       /* 82 */
	TIMING_MAX,
	TIMING_PC_MODE = 0x200,
	TIMING_NOT_SUPPORT,
	TIMING_NOSIGNAL
} video_timing_idx;

typedef struct cfg_timing_info_struct {
	uint32_t hactive_count;
	uint32_t htotal_count;
	uint32_t hfront_count;
	uint32_t hback_count;

	uint32_t vactive_count;
	uint32_t vtotal_count;
	uint32_t vfront_count;
	uint32_t vback_count;

	uint32_t hsync_count;
	uint32_t vsync_count;
	uint32_t fps_count;
} cfg_timing_info_st;

typedef struct hdmirx_timing_info_struct {
	video_timing_idx video_idx;
	uint32_t cea861_vic;
	uint32_t hdmi3d_structure;
	uint32_t hdmi3d_ext_data;
	uint32_t pix_freq; /* pixel frequency in 10k_hz units */
	uint32_t vactive;
	uint32_t hactive;
	uint32_t frame_rate;
	uint32_t htotal;
	uint32_t vtotal;
} hdmirx_timing_info_st;

typedef struct {
	hdmirx_timing_info_st hdmirx_timing_info;
	hdmirx_input_width input_width;
} hdmirx_video_ctx;

void hdmirx_video_detect_enable(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_video_detect_process(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_video_detect_irq_mask(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_video_start_display(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_video_stop_display(struct hdmirx_ctrl_st *hdmirx);
unsigned int hdmirx_video_get_hblank(void);
unsigned int hdmirx_video_get_vblank(void);
unsigned int hdmirx_video_get_htotal(void);

#endif
