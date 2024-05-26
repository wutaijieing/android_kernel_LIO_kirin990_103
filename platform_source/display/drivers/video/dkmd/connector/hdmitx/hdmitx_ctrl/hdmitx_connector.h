/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver connector header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */

#ifndef __HDMITX_CONNECTOR_H__
#define __HDMITX_CONNECTOR_H__

#include <linux/types.h>
#include <linux/mutex.h>

#include "hdmitx_modes.h"
#include "hdmitx_common.h"

struct hdmitx_timing_data;

#define INT_AON_NAME_LEN           20

#define CONNECTOR_TYPE_UNKNOWN     0
#define CONNECTOR_TYPE_VGA         1
#define CONNECTOR_TYPE_DVII        2
#define CONNECTOR_TYPE_DVID        3
#define CONNECTOR_TYPE_DVIA        4
#define CONNECTOR_TYPE_COMPOSITE   5
#define CONNECTOR_TYPE_SVIDEO      6
#define CONNECTOR_TYPE_LVDS        7
#define CONNECTOR_TYPE_COMPONENT   8
#define CONNECTOR_TYPE_9PINDIN     9
#define CONNECTOR_TYPE_DISPLAYPORT 10
#define CONNECTOR_TYPE_HDMITXA       11
#define CONNECTOR_TYPE_HDMITXB       12
#define CONNECTOR_TYPE_TV          13
#define CONNECTOR_TYPE_EDP         14
#define CONNECTOR_TYPE_VIRTUAL     15
#define CONNECTOR_TYPE_DSI         16
#define CONNECTOR_TYPE_DPI         17
#define CONNECTOR_TYPE_WRITEBACK   18

/* max tmds clock , in kHz */
#define HDMITX14_MAX_TMDS_CLK 340000
#define HDMITX20_MAX_TMDS_CLK 600000

/* max pixel clock , in kHz */
#define HDMITX14_MAX_PIXEL_CLK          340000
#define HDMITX20_MAX_PIXEL_CLK          600000
#define HDMITX21_8K30_MAX_PIXEL_CLK     1188000
#define HDMITX21_8K60_MAX_PIXEL_CLK     2376000
#define HDMITX21_8K120_MAX_PIXEL_CLK    4752000

#define HDMITX14_MAX_PIXEL_CLK_LEVEL        0
#define HDMITX20_MAX_PIXEL_CLK_LEVEL        1
#define HDMITX21_8K30_MAX_PIXEL_CLK_LEVEL   2
#define HDMITX21_8K60_MAX_PIXEL_CLK_LEVEL   3
#define HDMITX21_8K120_MAX_PIXEL_CLK_LEVEL  4

/* hdmi mode define */
#define HDMITX_MODE_DVI 0
#define HDMITX_MODE_14  1
#define HDMITX_MODE_20  2
#define HDMITX_MODE_21  3

/* EDID source type */
#define EDID_EMPTY        0
#define EDID_FROM_SINK    1
#define EDID_DEFAULT_DVI  2
#define EDID_DEFAULT_HDMITX 3
#define EDID_DEBUG        4

/* dsc caps max slice count define */
#define DSC_CAP_MAX_SLICE_CNT_0  0
#define DSC_CAP_MAX_SLICE_CNT_1  1
#define DSC_CAP_MAX_SLICE_CNT_2  2
#define DSC_CAP_MAX_SLICE_CNT_3  3
#define DSC_CAP_MAX_SLICE_CNT_4  4
#define DSC_CAP_MAX_SLICE_CNT_5  5
#define DSC_CAP_MAX_SLICE_CNT_6  6

struct eotf {
	bool traditional_sdr;
	bool traditional_hdr;
	bool st2084_hdr;
	bool hlg;
};

struct st_metadata {
	bool s_type1;
	u8 max_lum_cv;
	u8 aver_lum_cv;
	u8 min_lum_cv;
};

struct dy_metadata {
	bool d_type1_support;
	u8 d_type1_version;
	bool d_type2_support;
	u8 d_type2_version;
	bool d_type3_support;
	u8 d_type3_version;
	bool d_type4_support;
	u8 d_type4_version;
};

struct hdr_property {
	struct eotf eotf;
	struct st_metadata st_metadata; /* Static Metadata Type */
	struct dy_metadata dy_metadata; /* HDR Dynamic Metadata Type */
};

struct scdc_property {
	bool present;     /* SCDC_Present */
	bool rr_capable;  /* RR_capable */
	bool lte_340mcsc; /* LTE_340Mcsc_Scramble */
	bool version;     /* source & sink version */
};

struct dsc_property {
	bool dsc_1p2;         /* DSC_1p2 */
	bool y420;            /* DSC_Native_420 */
	bool all_bpp;         /* DSC_ALL_bpp */
	bool dsc_16bpc;       /* DSC_16bpc */
	bool dsc_12bpc;       /* DSC_12bpc */
	bool dsc_10bpc;       /* DSC_10bpc */
	u8 dsc_max_rate;      /* DSC_Max_FRL_Rate */
	u8 max_slice;         /* DSC_MaxSlices */
	u8 total_chunk_bytes; /* DSC_TotalChunkBytes */
};

struct vrr_property {
	bool fva;              /* FVA */
	bool cnm_vrr;          /* CNMVRR */
	bool cinema_vrr;       /* CinemaVRR */
	bool m_delta;          /* MDelta */
	bool fapa_start_locat; /* FAPA_start_location */
	bool allm;             /* ALLM */
	u16 vrr_min;           /* VRRMin */
	u16 vrr_max;           /* VRRMax */
};

/* see HDMITX14b SPEC Table 8-16 HDMITX-LLC Vendor-Specific Data Block (HDMITX VSDB) */
struct color_depth {
	bool rgb_24; /* no define in VSDB.default true */
	bool rgb_30; /* DC_30bit */
	bool rgb_36; /* DC_36bit */
	bool rgb_48; /* DC_48bit */

	bool y444_24; /* no define in VSDB.if ycbcr444 is true ,set true. */
	bool y444_30; /* DC_30bit && DC_Y444 */
	bool y444_36; /* DC_36bit && DC_Y444 */
	bool y444_48; /* DC_36bit && DC_Y444 */

	bool y420_24; /* no define in HF-VSDB.if ycbcr420 is true,set true */
	bool y420_30; /* DC_420_30 */
	bool y420_36; /* DC_420_36 */
	bool y420_48; /* DC_420_48 */

	u32 bpc; /* Maximum bits per color channel. Used by HDMITX and DP outputs. */
};

/* see CTA-861-G Table 40 CEA Extension Version 2 (deprecated) */
struct color_format {
	/* no define in VSDB.default true */
	bool rgb;
	/* If byte 3 of bit 5 (YCBCR 4:4:4) = 0b1 , in CTA Extension Version 2/3,set true. */
	bool ycbcr444;
	/* If byte 3 of bit 4 (YCBCR 4:2:2) = 0b1 , in CTA Extension Version 2/3,set true. */
	bool ycbcr422;
	/* If a YCC420-VIC present, set true.see YCC420 Capability Map Data Block or YCC420 Video Data Block in CTA-861-G */
	bool ycbcr420;
};

/* see CTA-861-G Table 70 Colorimetry Data Block */
struct colorimetry {
	bool xvycc601;     /* Standard Definition Colorimetry based on IEC 61966-2-4 [5 ] */
	bool xvycc709;     /* High Definition Colorimetry based on IEC 61966-2-4 [5 ] */
	bool sycc601;      /* Colorimetry based on IEC 61966-2-1/Amendment 1 [34] */
	bool adobe_ycc601; /* Colorimetry based on IEC 61966-2-5 [32], Annex A */
	bool adobe_rgb;    /* Colorimetry based on IEC 61966-2-5 [32] */
	bool bt2020_cycc;  /* Colorimetry based on ITU-R BT.2020 [39]  Y'cC'bcC'rc */
	bool bt2020_ycc;   /* Colorimetry based on ITU-R BT.2020 [39]  Y'C'BC'R */
	bool bt2020_rgb;   /* Colorimetry based on ITU-R BT.2020 [39]  R'G'B' */
	bool dci_p3;       /* Colorimetry based on DCI-P3 [51][52] */
};

/* see CTG-861-G Table 74 */
struct quantization {
	/*
	 * @rgb_qs_selecable
	 * If the Sink declares a selectable RGB Quantization  Range (QS=1;rgb_qs==true) then it shall expect Limited Range
	 * pixel values if it receives Q=1 (in AVI) and it shall expect Full Range pixel values if it receives Q=2.
	 * For other values of Q,the Sink shall expect pixel values with the default range for the transmitted Video Format
	 * (Limited Range when receiving a CE Video Format and a Full Range when receiving an IT format).
	 */
	bool rgb_qs_selecable;

	/*
	 * @ycc_qy_selecable
	 * If the Sink declares a selectable YCC Quantization Range (QY=1;ycc_qy==true), then it shall expect Limited Range
	 * pixel values if it receives AVI YQ=0 and it shall expect Full Range pixel values if it receives AVI YQ=1.
	 * For other values of YQ,the Sink shall expect pixel values with the default range for the transmitted Video Format
	 * (Limited Range when receiving a CE Video Format and a Full Range when receiving an IT format)
	 */
	bool ycc_qy_selecable;
};

struct color_property {
	struct color_depth depth;         /* color depth */
	struct color_format format;       /* color format(Pixel Encoding format) */
	struct colorimetry colorimetry;   /* colorimetry */
	struct quantization quantization; /* quantization */
};

#define MAX_SAD_AUDIO_CNT (0x9 + 0xe)

/* Short Audio Descriptors.see CTA-861-G 7.5.2 Audio Data Block */
struct sad_fmt_audio {
	u8 fmt_code;      /* Audio Format Code */
	u8 ext_code;      /* Audio Coding Extension Type Code */
	/*
	 * Max Number of channels.Audio Format Code 1 to 14;
	 * Audio Coding Extension Type Codes 4 to 6,8 to 10;
	 * Audio Coding Extension Type 0x0D (L-PCM 3D Audio), bits MC4:MC0
	 */
	u32 max_channel;
	bool samp_32k;    /* 32   kHz */
	bool samp_44p1k;  /* 44.1 kHz */
	bool samp_48k;    /* 48   kHz */
	bool samp_88p2k;  /* 88.2 kHz */
	bool samp_96k;    /* 96   kHz */
	bool samp_176p4k; /* 176.4kHz */
	bool samp_192k;   /* 192 kHz */
	bool width_16; /* 16bit.Only to Audio Format Code = 1(L-PCM) & Audio Extension Type Code 13(L-PCM 3D Audio) */
	bool width_20; /* 20bit.Only to Audio Format Code = 1(L-PCM) & Audio Extension Type Code 13(L-PCM 3D Audio) */
	bool width_24; /* 24bit.Only to Audio Format Code = 1(L-PCM) & Audio Extension Type Code 13(L-PCM 3D Audio) */
	u32 max_bit_rate; /* Maximum bit rate in Hz.Only to Audio Format Codes 2 to 8 */
	u8 dependent;     /* Audio Format Code dependent value.Only to Audio Format Codes 9 to 13 */
	u8 profile;       /* Profile.Only to Audio Format Codes 14 (WMA pro) */
	bool len_1024_tl; /* 1024_TL.AAC audio frame lengths 1024_TL.Only to extension Type Codes 4 to 6 */
	bool len_960_tl;  /* 960_TL. AAC audio frame lengths 960_TL.Only to extension Type Codes 4 to 6 */
	bool mps_l;       /* MPS_L. Only to Extension Type Codes 8 to 10 */
};

struct audio_property {
	/*
	 * @basic :basic audio support.
	 * Basic Audio-Uncompressed, two channel, digital audio.
	 * e.g., 2 channel IEC 60958-3 [12] L-PCM, 32, 44.1, and 48 kHz sampling rates, 16 bits/sample.
	 */
	bool basic;
	/* @sad_count:Short Audio Descriptors audio format support total number. */
	u32 sad_count;
	/* @sad :see Short Audio Descriptors. */
	struct sad_fmt_audio sad[MAX_SAD_AUDIO_CNT];
};

struct audio_in {
	struct hdmitx_timing_mode mode;
	u32 bpc; /* 8 10 12 */
	u32 pixel_format;
	u32 audio_rate;
	u32 packet_type;
	u32 layout;
	u32 acat;
};

struct latency_property {
	bool latency_present; /* latency present */
	u32 p_video;          /* progressive  video_Latency,in milliseconds */
	u32 p_audio;          /* progresive Audio_Latency,in milliseconds */
	u32 i_video;          /* Interlaced_Video_Latency,in milliseconds */
	u32 i_audio;          /* Interlaced_Audio_Latency,in milliseconds */
};

struct chromaticity {
	u16 red_x;
	u16 red_y;
	u16 green_x;
	u16 green_y;
	u16 blue_x;
	u16 blue_y;
	u16 white_x;
	u16 white_y;
};

/*
 * struct drm_hdmi_info - runtime information about the connected HDMITX sink
 * Describes if a given display supports advanced HDMITX 2.0 features.
 * This information is available in CEA-861-F extension blocks (like HF-VSDB).
 */
struct drm_hdmitx_info {
	/* @y420_cmdb_map: bitmap of SVD index, to extraxt vcb modes */
	u64 y420_cmdb_map;
};

/*
 * struct hdmitx_display_info - runtime data about the connected sink
 * Describes a given display (e.g. CRT or flat panel) and its limitations. For
 * fixed display sinks like built-in panels there's not much difference between
 * this and &struct drm_connector. But for sinks with a real cable this
 * structure is meant to describe all the things at the other end of the cable.
 * For sinks which provide an EDID this can be filled out by calling drm_add_edid_modes.
 */
struct hdmitx_display_info {
	/* Physical width in cm */
	u32 width_cm;

	/* Physical height in cm */
	u32 height_cm;

	/* Dual-link DVI sink */
	bool dvi_dual;

	/* Does the sink support the HDMITX infoframe? */
	bool has_hdmi_infoframe;

	/* @cea_rev: CEA revision of the HDMITX sink */
	u8 cea_rev;

	bool force_max_tmds; /* when dvi,force max_tmds_clk to 340MHz */

	u32 max_tmds_clock; /* Max_TMDS_Charater_Rate,in kHz. */

	struct drm_hdmitx_info hdmi;

	u8 ffe_level; /* source's FFE level.default 3.no define in edid. */

	u8 max_frl_rate; /* Max_FRL_Rate */
};

struct base_property {
	u8 version;
	u8 revision;
	u8 ext_block_num;
	// struct vendor_info vendor;
	struct chromaticity chromaticity;
};

/*
 * see Table 3.19 - Standard Timings in VESA ENHANCED
 * EXTENDED DISPLAY IDENTIFICATION DATA STANDARD (September 25, 2006 ).
 */
struct standard_timing {
	u32 h_active;
	u32 v_active;
	u32 refresh_rate;
};

#define MAX_Y420_ONLY_CNT 128
#define MAX_Y420_CMDB_CNT 128
#define MAX_VIC_CNT       0xff
#define MAX_STD_CNT       32
#define MAX_DTD_CNT       32
#define MAX_ESTAB_CNT     32

struct timing_property {
	u32 native_vic;
	u32 total_cnt;              /* total vic count, fillter the same vic in vesa & svd & y420's vic */
	u32 total_vic[MAX_VIC_CNT]; /* total vic, fillter the same vic in vesa & svd & y420's vic */

	u32 vesa_cnt;
	u32 svd_cnt;
	u32 y420_only_cnt;
	u32 y420cmdb_cnt;
	u32 svr_cnt;
	u32 estabblished_cnt;
	u32 dtds_cnt;
	u32 std_cnt;

	u32 svd_vic[MAX_VIC_CNT];
	/* Established Timing or Standard Timings or  Detailed Timing  conver to huanglong's self-define vic */
	u32 vesa_vic[MAX_VIC_CNT];
	/*
	 * vic which can support ycbcr420output only (not normal RGB/YCBCR444/422 outputs).
	 * There are total 107 VICs defined by CEA-861-F spec, so the size is 128 bits to mapupto 128 VICs;
	 */
	u32 y420_only_vic[MAX_Y420_ONLY_CNT];
	/*
	 * YCBCR4:2:0 Capability Map Data Block.modes which can support ycbcr420 output also,along with normal HDMI outputs
	 * There are total 107 VICs defined by CEA-861-F spec, so the size is 128 bits to map upto 128 VICs;
	 */
	u32 y420_cmdb_vic[MAX_Y420_CMDB_CNT];
	/* Short Video Reference .see CTA-861-G Table 83  Video Format Preference Data Block */
	u32 svr_vic[MAX_VIC_CNT];
	/*
	 * Established Timings or Manufacturer's Timings.
	 * see VESA ENHANCED EXTENDED DISPLAY IDENTIFICATION DATA STANDARD
	 * (September 25, 2006) Table 3.18 - Established Timings I & II
	 */
	u32 estabblished[MAX_ESTAB_CNT];
	struct standard_timing std[MAX_STD_CNT]; /* Standard Timings */
};

/* EDID Dolby capability in VSVDB version 0 */
struct dolby_v0 {
	bool y422_36bit; /* support(true) or not support(false) a YUV422-12Bit dolby singal */
	bool is_2160p60; /* capable of processing a max timming 3840X2160p60(true) /3840X2160p30(false) */
	bool global_dimming; /* support(true) or not support(false) global dimming. */
	/*
	 * white point chromaticity coordinate x,
	 * bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N), only support when u8VSVDBVersion = 0.
	 */
	u16 white_x;
	/*
	 * white point chromaticity coordinate y,
	 * bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N), only support when u8VSVDBVersion = 0.
	 */
	u16 white_y;
	/* the major version of display management implemented. only support when u8VSVDBVersion = 0 */
	u16 dm_major_ver;
	/* the minor version of display management implemented. only support when u8VSVDBVersion = 0. */
	u8 dm_minor_ver;
	u16 target_min_pq; /* Perceptual quantization(PQ)-encoded value of minimum display luminance */
	u16 target_max_pq; /* PQ-encoded value of maximum display luminance */
	u16 red_x; /* red primary chromaticity coordinate x   ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 red_y; /* red primary chromaticity coordinate y   ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 green_x; /* green primary chromaticity coordinate x ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 green_y; /* green primary chromaticity coordinate y ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 blue_x; /* blue primary chromaticity coordinate x  ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 blue_y; /* blue primary chromaticity coordinate y  ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
};

/* EDID Dolby capability in VSVDB version 1 */
struct dolby_v1 {
	/*
	 * support(true) or not support(false) a YUV422-12Bit dolby singal;
	 * For low-latency dolbyvision this flag is ingnored
	 */
	bool y422_36bit;
	/*
	 * capable of processing a max timming 3840X2160p60(true) /3840X2160p30(false);
	 * For low-latency dolbyvision this bit maybe ingnored,relay on supported video format from the E-EDID
	 */
	bool is_2160p60;
	bool global_dimming; /* support(true) or not support(false) global dimming. */
	/*
	 * this bit is valid only u8VSVDBVersion = 1.
	 * 0: Dolby Vision HDMITX sink's colorimetry is close to Rec.709,
	 * 1: EDR HDMITX sink's colorimetry is close to P3, if Byte[9] to Byte[14] are present, ignores this bit.
	 */
	bool colorimetry;
	/*
	 * 0:based on display management v2.x; 1:based on the video and blending pipeline v3.x;
	 * 2-7: reserved. only support when u8VSVDBVersion = 1.
	 */
	u8 dm_version;
	/*
	 * 0:supports only standard DolbyVison;
	 * 1: Supports low latency with 12-bit YCC422 interface using
	 * the HDMITX native 12-bit YCC422 pixel encoding and standard Dolby Vision interface; 2-3:reserved
	 */
	u8 low_latency;
	u16 target_min_lum; /* minimum display luminance = (100+50*CV)cd/m2, where CV is the value */
	u16 target_max_lum; /* maximum display luminance = (CV/127)^2cd/m2, where CV is the value */
	u16 red_x; /* red primary chromaticity coordinate x,bit[11:0]valid.Real value=SUM OF bit[N]*2^-(12-N) */
	u16 red_y; /* red primary chromaticity coordinate y,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 green_x; /* green primary chromaticity coordinate x,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 green_y; /* green primary chromaticity coordinate y,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 blue_x; /* blue primary chromaticity coordinate x,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 blue_y; /* blue primary chromaticity coordinate y,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
};

/* EDID Dolby capability in VSVDB version 2 */
struct dolby_v2 {
	/* support(true) or not support(false) a YUV422-12Bit dolby singal */
	bool y422_36bit;
	/* supports Backlight Control */
	bool back_light_ctrl;
	/* support(true) or not support(false) global dimming. */
	bool global_dimming;
	/*
	 * 0:based on display management v2.x;
	 * 1:based on the video and blending pipeline v3.x;
	 * 2-7: reserved. only support when u8VSVDBVersion = 1.
	 */
	u8 dm_version;
	/* minimum luminance level 0:25cd/m2 1:50cd/m2 2:75cd/m2 3:100cd/m2 */
	u8 back_lt_min_lum;
	/*
	 * 0:support only "low latency with YUV422"
	 * 1:support both "low latency with YUV422" and YUV444/RGB_10/12bit
	 * 2:support both "standard DolbyVision" and "low latency with YUV422"
	 * 3:support "standard DolbyVision" "low latency YUV422 YUV444/RGB_10/12bit"
	 */
	u8 interface;
	/* 0:not support 1:support YUV444/RGB444_10bit 2:support YUV444/RGB444_12bit 3:reserved */
	u8 y444_rgb_30b36b;
	/* minimum display luminance, in the PQ-encoded value= 2055+u16TargetMaxPQv2*65. */
	u16 target_min_pq_v2;
	/*
	 * maximum display luminance, in the PQ-encoded value= u16TargetMinPQv2*20.
	 * A code value of 31 is approximately equivalent to 1cd/m2
	 */
	u16 target_max_pq_v2;
	/* red primary chromaticity coordinate x   ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 red_x;
	/* red primary chromaticity coordinate y   ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 red_y;
	/* green primary chromaticity coordinate x ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 green_x;
	/* green primary chromaticity coordinate y ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 green_y;
	/* blue primary chromaticity coordinate x  ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 blue_x;
	/* blue primary chromaticity coordinate y  ,bit[11:0]valid.Real value =SUM OF bit[N]*2^-(12-N) */
	u16 blue_y;
};

struct dolby_property {
	bool support_v0;
	bool support_v1;
	bool support_v2;
	struct dolby_v0 v0;
	struct dolby_v1 v1;
	struct dolby_v2 v2;
};

struct cuva_property {
	u8 system_start_code;
	u8 version_code;
	u32 display_max_lum;
	u32 display_min_lum;
	bool monitor_mode_support;
	bool rx_mode_support;
};

enum hdmitx_connector_status {
	HPD_DETECTING = 0,
	HPD_PLUGIN = 1,
	HPD_PLUGOUT = 2,
	HPD_DET_FAIL = 3
};

struct hdmitx_connector {
	char *name;
	bool user_dvi_mode;
	struct mutex mutex;
	/*
	 * struct hdmi_available_mode
	 * Modes available on this connector (from fill_modes() + user).
	 * Protected by &hdmitx_connector.mutex.
	 */
	struct list_head valid_modes;

	/*
	 * These are modes added by probing with DDC or the BIOS, before
	 * filtering is applied. Used by the probe helpers. Protected by
	 * &drm_mode_config.mutex.
	 */
	struct list_head probed_modes;

	s32 probed_mode_valid_cnt;

	s32 probed_mode_cnt;

	u32 detail_vic_base;

	s32 edid_src_type;

#define MAX_ELD_BYTES 128
	/* EDID-like data, if present */
	u8 eld[MAX_ELD_BYTES];

	struct edid *edid_raw;

	s32 edid_size;

	struct hdmitx_display_info display_info;

	/* base property of the connected sink */
	struct base_property base;

	/* timing property of the connected sink */
	struct timing_property timing;

	/* color property of the connected sink */
	struct color_property color;

	/* vrr property of the connected sink */
	struct vrr_property vrr;

	/* hdr property of the connected sink */
	struct hdr_property hdr;

	/* dolby property of the connected sink */
	struct dolby_property dolby;

	/* cuva hdr property of the connected sink */
	struct cuva_property cuva;

	/* frl property of the connected sink */
	struct scdc_property scdc;

	/* dsc property of the connected sink */
	struct dsc_property dsc;

	/* audio property of the connected sink */
	struct audio_property audio;

	/* latency property of the connected sink */
	struct latency_property latency;

	struct source_capability src_cap;

	/* current compatibility info of the connected sink */
	// struct compat_info cur_compat_info;
};

struct hdmitx_ctrl;
bool drv_hdmitx_connector_is_scdc_present(struct hdmitx_ctrl *hdmitx);

int hdmitx_connector_init(struct hdmitx_ctrl *hdmitx);
int hdmitx_handle_hotplug(struct hdmitx_ctrl *hdmitx);
int hdmitx_handle_unhotplug(struct hdmitx_ctrl *hdmitx);


#endif /* __DRV_HDMITX_CONNECTOR_H__ */

