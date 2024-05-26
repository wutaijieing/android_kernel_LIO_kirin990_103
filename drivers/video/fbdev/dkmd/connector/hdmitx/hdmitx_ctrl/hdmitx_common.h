/*
 * Copyright (c) CompanyNameMagicTag. 2020-2020. All rights reserved.
 * Description: hdmitx driver common header file.
 * Author: AuthorNameMagicTag
 * Create: 2020/03/20
 */

#ifndef __DRV_HDMITX_COMMON_H__
#define __DRV_HDMITX_COMMON_H__

#include <linux/types.h>

#define DISP_INTF_HDMITX0 0
#define DISP_INTF_HDMITX1 1

/* color depth */
#define HDMITX_BPC_24 0
#define HDMITX_BPC_30 1
#define HDMITX_BPC_36 2

/* AVI Byte1 B [1:0],Bar Data Present */
#define HDMITX_BAR_DATA_NO_PRESENT  0
#define HDMITX_BAR_DATA_VER_PRESENT 1
#define HDMITX_BAR_DATA_HOR_PRESENT 2
#define HDMITX_BAR_DATA_V_H_PRESENT 3

/* AVI Byte1 Y[2:0], RGB  or YCBCR .color format(pixel encoding format) */
#define HDMITX_COLOR_FORMAT_RGB      0
#define HDMITX_COLOR_FORMAT_YCBCR422 1
#define HDMITX_COLOR_FORMAT_YCBCR444 2
#define HDMITX_COLOR_FORMAT_YCBCR420 3

/*
 * AVI Byte2 R[3:0],Active Portion Aspect Ratio,
 * for ATSC .see CEA-861-G H.1 ATSC Active Format Description.
 */
#define HDMITX_ACT_ASP_RATIO_ATSC_G_16_9           4
#define HDMITX_ACT_ASP_RATIO_ATSC_SAME_PIC         8
#define HDMITX_ACT_ASP_RATIO_ATSC_4_3              9
#define HDMITX_ACT_ASP_RATIO_ATSC_16_9             10
#define HDMITX_ACT_ASP_RATIO_ATSC_14_9             11
#define HDMITX_ACT_ASP_RATIO_ATSC_4_3_CENTER_14_9  13
#define HDMITX_ACT_ASP_RATIO_ATSC_16_9_CENTER_14_9 14
#define HDMITX_ACT_ASP_RATIO_ATSC_16_9_CENTER_4_3  15

/*
 * AVI Byte2 R[3:0],Active Portion Aspect Ratio,
 * for DVB.see CEA-861-G H.2 DVB Active Format Description .
 */
#define HDMITX_ACT_ASP_RATIO_DVB_BOX_16_9         2
#define HDMITX_ACT_ASP_RATIO_DVB_BOX_14_9         3
#define HDMITX_ACT_ASP_RATIO_DVB_BOX_16_9_CENTER  4
#define HDMITX_ACT_ASP_RATIO_DVB_SAME_PIC         8
#define HDMITX_ACT_ASP_RATIO_DVB_CENTER_4_3       9
#define HDMITX_ACT_ASP_RATIO_DVB_CENTER_16_9      10
#define HDMITX_ACT_ASP_RATIO_DVB_CENTER_14_9      11
#define HDMITX_ACT_ASP_RATIO_DVB_4_3_CENTER_14_9  13
#define HDMITX_ACT_ASP_RATIO_DVB_16_9_CENTER_14_9 14
#define HDMITX_ACT_ASP_RATIO_DVB_16_9_CENTER_4_3  15

/* AVI Byte2 M[1:0],Picture Aspect Ratio */
#define HDMITX_PIC_ASPECT_RATIO_NO_DATA 0
#define HDMITX_PIC_ASPECT_RATIO_4_3     1
#define HDMITX_PIC_ASPECT_RATIO_16_9    2
#define HDMITX_PIC_ASPECT_RATIO_64_27   3
#define HDMITX_PIC_ASPECT_RATIO_256_135 4

/*
 * AVI Byte2 C[1:0],Colorimetry ;Byte3 EC[2:0]Extended Colorimetry
 * define [1:0] for C[1:0]; define [6:4] for EC[2:0]
 */
#define HDMITX_COLORIMETRY_NO_DATA                 0x00
#define HDMITX_COLORIMETRY_ITU601                  0x01
#define HDMITX_COLORIMETRY_ITU709                  0x02
#define HDMITX_COLORIMETRY_XVYCC_601               0x03
#define HDMITX_COLORIMETRY_XVYCC_709               0x13
#define HDMITX_COLORIMETRY_S_YCC_601               0x23
#define HDMITX_COLORIMETRY_ADOBE_YCC_601           0x33
#define HDMITX_COLORIMETRY_ADOBE_RGB               0x43
#define HDMITX_COLORIMETRY_2020_CONST_LUMINOUS     0x53
#define HDMITX_COLORIMETRY_2020_NON_CONST_LUMINOUS 0x63
#define HDMITX_COLORIMETRY_ADDITION_EXTENSION      0x73

/* AVI Byte3 Q[1:0] ,RGB Quantization Range */
#define HDMITX_RGB_QUANTIZEION_DEFAULT 0
#define HDMITX_RGB_QUANTIZEION_LIMITED 1
#define HDMITX_RGB_QUANTIZEION_FULL    2

/* AVI Byte5 Q[7:6] ,YCC Quantization Range */
#define HDMITX_YCC_QUANTIZEION_LIMITED 0
#define HDMITX_YCC_QUANTIZEION_FULL    1

/* AVI Byte5 CN[1:0],IT content type */
#define HDMITX_IT_CONTENT_GRAPHYICS 0
#define HDMITX_IT_CONTENT_PHOTO     1
#define HDMITX_IT_CONTENT_CINIMA    2
#define HDMITX_IT_CONTENT_GAME      3

/* AVI Byte5 PR[3:0],Pixel Repetition Factor */
#define HDMITX_PIXEL_REPEAT_NO_REPEAT 0
#define HDMITX_PIXEL_REPEAT_2_TIMES   1
#define HDMITX_PIXEL_REPEAT_3_TIMES   2
#define HDMITX_PIXEL_REPEAT_4_TIMES   3
#define HDMITX_PIXEL_REPEAT_5_TIMES   4
#define HDMITX_PIXEL_REPEAT_6_TIMES   5
#define HDMITX_PIXEL_REPEAT_7_TIMES   6
#define HDMITX_PIXEL_REPEAT_8_TIMES   7
#define HDMITX_PIXEL_REPEAT_9_TIMES   8
#define HDMITX_PIXEL_REPEAT_10_TIMES  9

/* 3D -- mode_3d */
#define HDMITX_3D_FRAME_PACKETING        0x00
#define HDMITX_3D_FIELD_ALTERNATIVE      0x01
#define HDMITX_3D_LINE_ALTERNATIVE       0x02
#define HDMITX_3D_SIDE_BY_SIDE_FULL      0x03
#define HDMITX_3D_L_DEPTH                0x04
#define HDMITX_3D_L_DEPTH_GRAPHICS_DEPTH 0x05
#define HDMITX_3D_TOP_AND_BOTTOM         0x06
#define HDMITX_3D_SIDE_BY_SIDE_HALF      0x08
#define HDMITX_3D_NONE                   0x09

/* hdmi_mode_type */
#define HDMITX_MODE_TYPE_IN  (1 << 0)
#define HDMITX_MODE_TYPE_OUT (1 << 1)
#define HDMITX_MODE_TYPE_HDR (1 << 2)
#define HDMITX_MODE_TYPE_VRR (1 << 3)

/* HDR mode type */
#define HDMITX_HDR_MODE_SDR            0x00
#define HDMITX_HDR_MODE_STATIC_TRD_SDR 0x10
#define HDMITX_HDR_MODE_STATIC_TRD_HDR 0x11
#define HDMITX_HDR_MODE_STATIC_ST2084  0x12
#define HDMITX_HDR_MODE_STATIC_HLG     0x13
#define HDMITX_HDR_MODE_DOLBY_V0       0x20
#define HDMITX_HDR_MODE_DOLBY_V1       0x21
#define HDMITX_HDR_MODE_DOLBY_V2       0x22
#define HDMITX_HDR_MODE_DYNAMIC_TYPE1  0x31
#define HDMITX_HDR_MODE_DYNAMIC_TYPE2  0x32
#define HDMITX_HDR_MODE_DYNAMIC_TYPE3  0x33
#define HDMITX_HDR_MODE_DYNAMIC_TYPE4  0x34

/* VRR mode type */
#define HDMITX_VRR_MODE_VRR  0
#define HDMITX_VRR_MODE_QMS  1
#define HDMITX_VRR_MODE_QFT  2
#define HDMITX_VRR_MODE_ALLM 3

/* aud_codec */
#define HDMITX_AUDIO_CODE_TYPE_PCM     1
#define HDMITX_AUDIO_CODE_TYPE_AC3     2
#define HDMITX_AUDIO_CODE_TYPE_MPEG1   3
#define HDMITX_AUDIO_CODE_TYPE_MP3     4
#define HDMITX_AUDIO_CODE_TYPE_MPEG2   5
#define HDMITX_AUDIO_CODE_TYPE_AAC_LC  6
#define HDMITX_AUDIO_CODE_TYPE_DTS     7
#define HDMITX_AUDIO_CODE_TYPE_ATRAC   8
#define HDMITX_AUDIO_CODE_TYPE_DSD     9
#define HDMITX_AUDIO_CODE_TYPE_EAC3    10
#define HDMITX_AUDIO_CODE_TYPE_DTS_HD  11
#define HDMITX_AUDIO_CODE_TYPE_MAT     12
#define HDMITX_AUDIO_CODE_TYPE_DST     13
#define HDMITX_AUDIO_CODE_TYPE_WMA_PRO 14

/* aud_sample_size */
#define HDMITX_AUDIO_SAMP_SIZE_UNKNOW 0
#define HDMITX_AUDIO_SAMP_SIZE_8      8
#define HDMITX_AUDIO_SAMP_SIZE_16     16
#define HDMITX_AUDIO_SAMP_SIZE_18     18
#define HDMITX_AUDIO_SAMP_SIZE_20     20
#define HDMITX_AUDIO_SAMP_SIZE_24     24
#define HDMITX_AUDIO_SAMP_SIZE_32     32

/* aud_input_type */
#define HDMITX_AUDIO_SAMPLE_RATE_UNKNOW 0
#define HDMITX_AUDIO_SAMPLE_RATE_8K     8000
#define HDMITX_AUDIO_SAMPLE_RATE_11K    11025
#define HDMITX_AUDIO_SAMPLE_RATE_12K    12000
#define HDMITX_AUDIO_SAMPLE_RATE_16K    16000
#define HDMITX_AUDIO_SAMPLE_RATE_22K    22050
#define HDMITX_AUDIO_SAMPLE_RATE_24K    24000
#define HDMITX_AUDIO_SAMPLE_RATE_32K    32000
#define HDMITX_AUDIO_SAMPLE_RATE_44K    44100
#define HDMITX_AUDIO_SAMPLE_RATE_48K    48000
#define HDMITX_AUDIO_SAMPLE_RATE_88K    88200
#define HDMITX_AUDIO_SAMPLE_RATE_96K    96000
#define HDMITX_AUDIO_SAMPLE_RATE_176K   176400
#define HDMITX_AUDIO_SAMPLE_RATE_192K   192000
#define HDMITX_AUDIO_SAMPLE_RATE_768K   768000

/* aud_input_type */
#define HDMITX_AUDIO_INPUT_TYPE_I2S   1
#define HDMITX_AUDIO_INPUT_TYPE_SPDIF 2
#define HDMITX_AUDIO_INPUT_TYPE_HBR   3

/* aud_channels */
#define HDMITX_AUDIO_CHANNEL_2CH 2
#define HDMITX_AUDIO_CHANNEL_3CH 3
#define HDMITX_AUDIO_CHANNEL_4CH 4
#define HDMITX_AUDIO_CHANNEL_5CH 5
#define HDMITX_AUDIO_CHANNEL_6CH 6
#define HDMITX_AUDIO_CHANNEL_7CH 7
#define HDMITX_AUDIO_CHANNEL_8CH 8

/* vo set mode */
#define TIMING_SET_MODE (1 << 0)
#define HDR_SET_MODE    (1 << 1)
#define VRR_SET_MODE    (1 << 2)

struct color_info {
	u32 colorimetry;
	u32 color_format;
	u8 rgb_quantization;
	u8 ycc_quantization;
};

struct hdmitx_avi_data {
	u32 vic; /* AVI Byte4 VIC,Video ID Code */

	u8 scan_info;               /* AVI Byte1 S [1:0],Scan information	 */
	bool active_aspect_present; /* AVI Byte1 A[0], Active Format Information Present */
	u8 bar_present;             /* AVI Byte1 B [1:0],Bar Data Present */

	u32 picture_aspect_ratio; /* AVI Byte2 M[1:0],Picture Aspect Ratio */
	u32 active_aspect_ratio;  /* AVI Byte2 R[3:0],Active Portion Aspect Ratio.support in both DVB & ATSC mode */

	u8 picture_scal;       /* AVI Byte3 SC[1:0],Non-Uniform Picture Scaling */
	bool it_content_valid; /* AVI Byte3 ITC,IT content */

	u8 it_content_type; /* AVI Byte5 CN[1:0],IT content type */
	u8 pixel_repeat;    /* AVI Byte5 PR[3:0],Pixel Repetition Factor	 */

	u16 top_bar;    /* AVI Byte7-6 ETB15-ETB00 (Line Number of End of Top Bar) */
	u16 bottom_bar; /* AVI Byte9-8 SBB15-SBB00 (Line Number of Start of Bottom Bar) */
	u16 left_bar;   /* AVI Byte11-10 ELB15-ELB08 (Pixel Number of End of Left Bar) */
	u16 right_bar;  /* AVI Byte13-12 SRB07-SRB00 (Pixel Number of Start of Right Bar)	 */

	u32 color_depth; /* in color depth */
	struct color_info color; /* AVI Byte 1 Y[2:0] and Byte 5 YQ[1:0] */
	u32 fva_factor;
};

struct hdmitx_hdr_data {
	u32 hdr_mode_type;
};

struct hdmitx_vrr_data {
	u32 vrr_mode_type;
	u32 fva_factor;
};

struct dpu_video_info {
	struct hdmitx_hdr_data hdr_data;
	struct hdmitx_vrr_data vrr_data;
	struct hdmitx_avi_data avi_data;

	u8 mode_3d; /* VS PB5[7:4],3D_Structure */
	u32 reserved;
};

struct src_hdmitx {
	/* tmds */
	bool dvi_support;
	bool hdmi_support;
	u32 max_tmds_clock; /* in kHz */
	u32 max_pixel_clock;

	/* csc */
	bool rgb2yuv;
	bool yuv2rgb;
	bool ycbcr420;
	bool ycbcr422;
	bool ycbcr444;
	bool dither_support;

	/* scdc */
	bool scdc_present;
	bool scdc_lte_340mcsc;

	/* color depth */
	bool bpc_30;
	bool bpc_36;
	bool bpc_48;

	/* frl */
	u8 max_frl_rate;
	u8 ffe_levels;

	/* dsc */
	bool dsc_support;
	u8 max_dsc_frl_rate;
	bool native_y420;            /* DSC support native 420 */
	bool dsc_10bpc;              /* DSC support 10bpc */
	bool dsc_12bpc;              /* DSC support 12bpc */
	u8 max_silce_count;          /* DSC max slice count */
	u32 max_pixel_clk_per_slice; /* DSC max pixel clk per slice */
};

struct hdmitx_hpd_cfg {
	bool fillter_en;
	u32 high_reshold; /* in ms */
	u32 low_reshold;  /* in ms */
};

struct source_capability {
	bool hdcp14_support;
	bool hdcp2x_support;
	bool cec_support;
	struct src_hdmitx hdmi;
	struct hdmitx_hpd_cfg hpd;
};

struct hdmitx_debug_msg {
	u32 sub_cmd;
	void *data;
};

/* hdcp mode */
#define HDCP_MODE_AUTO 0
#define HDCP_MODE_1X   1
#define HDCP_MODE_2X   2
#define HDCP_MODE_BUTT 3

#define HDCP_DEFAULT_REAUTH_TIMES 0xffffffff

enum hdcp_err_code {
	HDCP_ERR_UNDO = 0x00,      /* do not start hdcp. */
	HDCP_ERR_NONE,             /* no authentication error. */
	HDCP_ERR_PLUG_OUT,         /* cable plug out. */
	HDCP_ERR_NO_SIGNAL,        /* signal output disable. */
	HDCP_ERR_NO_KEY,           /* do not load key. */
	HDCP_ERR_INVALID_KEY,      /* key is invalid. */
	HDCP_ERR_DDC,              /* DDC link error. */
	HDCP_ERR_ON_SRM,           /* on revocation list. */
	HDCP_ERR_NO_CAP,           /* no hdcp capability. */
	HDCP_ERR_TYPE1_NO_2X_CAP,  /* no hdcp2x capability on stream type 1. */
	HDCP_ERR_TYPE1_ON_1X_MODE, /* set only hdcp1x mode on stream type 1. */
	HDCP_1X_NO_CAP = 0x10,     /* no hdcp1x capability. */
	HDCP_1X_BCAP_FAIL,         /* read BCAP failed. */
	HDCP_1X_BSKV_FAIL,         /* read BKSV failed. */
	HDCP_1X_INTEGRITY_FAIL_R0, /* R0(R0') integrety failure. */
	HDCP_1X_WATCHDOG_TIMEOUT,  /* Repeater watch dog timeout. */
	HDCP_1X_VI_CHCECK_FAIL,    /* Vi(Vi') check failed. */
	HDCP_1X_EXCEEDED_TOPOLOGY, /* Exceeded toplogy. */
	HDCP_1X_INTEGRITY_FAIL_RI, /* Ri(Ri') integrety failure. */
	HDCP_2X_NO_CAP = 0x20,     /* no hdcp2x capability. */
	HDCP_2X_SIGNATURE_FAIL,    /* signature error. */
	HDCP_2X_MISMATCH_H,        /* mismatch  H(H'). */
	HDCP_2X_AKE_TIMEOUT,       /* AKE timeout. */
	HDCP_2X_LOCALITY_FAIL,     /* locality check failed. */
	HDCP_2X_REAUTH_REQ,        /* REAUTH_REQ request. */
	HDCP_2X_WATCHDOG_TIMEOUT,  /* watchdog timeout. */
	HDCP_2X_V_MISMATCH,        /* mismatch  V(V'). */
	HDCP_2X_ROLL_OVER,         /* no roll-over of seq_num_V. */
	HDCP_2X_EXCEEDED_TOPOLOGY  /* Exceeded toplogy. */
};

enum debug_ext_cmd_list {
	DEBUG_CMD_AVMUTE,
	DEBUG_CMD_CBAR,
	DEBUG_CMD_SCDC,
	DEBUG_CMD_HDMITX_MODE,
	DEBUG_CMD_RC,
	DEBUG_CMD_DE,
	DEBUG_CMD_NULL_PACKET,
	DEBUG_CMD_FIFO_THRESHOLD,
	DEBUG_CMD_FORCE_OUT,
	DEBUG_CMD_SOFTWARE_RESET,
	DEBUG_CMD_SEND_INFOFRAME,
	DEBUG_CMD_SET_VDP_AVI_PATH,
	DEBUG_CMD_DITHER,
	DEBUG_CMD_TRAINING
};

enum sub_cmd_num {
	CMD_NUM_0,
	CMD_NUM_1,
	CMD_NUM_2,
	CMD_NUM_3,
	CMD_NUM_4,
	CMD_NUM_5,
	CMD_NUM_6,
	CMD_NUM_7,
	CMD_NUM_8,
	CMD_NUM_9,
	CMD_NUM_MAX
};

/* 3D video capibility definations */
#define HDMITX_3D_BZ_NONE                  0
#define HDMITX_3D_BZ_FRAME_PACKING         (1 << 1)
#define HDMITX_3D_BZ_FIELD_ALTERNATIVE     (1 << 2)
#define HDMITX_3D_BZ_LINE_ALTERNATIVE      (1 << 3)
#define HDMITX_3D_BZ_SIDE_BY_SIDE_FULL     (1 << 4)
#define HDMITX_3D_BZ_L_DEPTH               (1 << 5)
#define HDMITX_3D_BZ_L_DEPTH_GFX_GFX_DEPTH (1 << 6)
#define HDMITX_3D_BZ_TOP_AND_BOTTOM        (1 << 7)
#define HDMITX_3D_BZ_SIDE_BY_SIDE_HALF     (1 << 8)

/* hdmitx_detail_timing flag */
#define HDMITX_TIMING_FLAG_NATIVE          (1 << 0)
#define HDMITX_TIMING_FLAG_ALTERNATE_RATE  (1 << 1)

#define COMPAT_AVMUTE_MAX_DELAY_TIME 2000
#define COMPAT_VIC_BITMASK               0xFFF
#define COMPAT_VIC_TYPE_BITMASK          (0xF << 12)
#define COMPAT_VIC_TYPE_NO_Y420          (0 << 12)
#define COMPAT_VIC_TYPE_Y420_ALSO        (1 << 12)
#define COMPAT_VIC_TYPE_Y420_ONLY        (2 << 12)
#define COMPAT_SINK_MAX_RESET_DELAY_TIME 2000

/*
 * Result is undefined of below situations :
 * The divisor is unsigned type value, and the divisor is a negative number or
 * the divisor is a negative number, and the divisor is unsigned type value
 * Please ensure that divisor is not 0.
 */
#define hdmitx_div_round_closest(x, divisor) \
	((((x) > 0) == ((divisor) > 0)) ? (((x) + ((divisor) / 2)) / (divisor)) : (((x) - ((divisor) / 2)) / (divisor)))

#define hdmitx_div_round_up(n, d) (((n) + (d) - 1) / (d))

#define hdmitx_array_size(x) (sizeof(x) / sizeof((x)[0]))
#endif /* __DRV_HDMITX_COMMON_H__ */
