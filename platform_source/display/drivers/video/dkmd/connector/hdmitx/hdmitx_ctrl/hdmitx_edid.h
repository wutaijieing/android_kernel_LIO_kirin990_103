/*
 * Copyright (c) CompanyNameMagicTag. 2019-2019. All rights reserved.
 * Description: hdmi driver edid header file
 * Author: AuthorNameMagicTag
 * Create: 2019-06-22
 */
#ifndef __HDMITX_EDID_H__
#define __HDMITX_EDID_H__

#include <linux/types.h>

struct hdmitx_connector;

#define DATA_BLOCK_VENDOR_SPECIFIC 0x7f

#define PRODUCT_TYPE_EXTENSION    0x00
#define PRODUCT_TYPE_TEST         0x01
#define PRODUCT_TYPE_PANEL        0x02
#define PRODUCT_TYPE_MONITOR      0x03
#define PRODUCT_TYPE_TV           0x04
#define PRODUCT_TYPE_REPEATER     0x05
#define PRODUCT_TYPE_DIRECT_DRIVE 0x06

#define EDID_LENGTH 128
#define DDC_ADDR    0x50
#define DDC_ADDR2   0x52 /* can hide DisplayID */

#define CEA_EXTEND       0x02
#define VTB_EXTEND       0x10
#define DI_EXTEND        0x40
#define LS_EXTEND        0x50
#define MI_EXTEND        0x60
#define DISPLAYID_EXT 0x70

struct est_timings {
	u8 t1;
	u8 t2;
	u8 mfg_rsvd;
} __attribute__((packed));

/* 00 = 16:10, 01 = 4:3, 10 = 5:4, 11 = 16:9 */
#define EDID_TIMING_ASPECT_RATIO_SHIFT 6
#define EDID_TIMING_ASPECT_RATIO_MASK  (0x3 << EDID_TIMING_ASPECT_RATIO_SHIFT)

/* need to add 60 */
#define EDID_TIMING_VF_REQ_SHIFT 0
#define EDID_TIMING_VF_REQ_MASK  (0x3f << EDID_TIMING_VF_REQ_SHIFT)

struct std_timing {
	u8 hsize; /* (hsize *8) + 248 */
	u8 vfreq_aspect;
} __attribute__((packed));

#define HDMITX_EDID_PT_HSYNC_POSITIVE (1 << 1)
#define HDMITX_EDID_PT_VSYNC_POSITIVE (1 << 2)
#define HDMITX_EDID_PT_SEPARATE_SYNC  (3 << 3)
#define HDMITX_EDID_PT_STEREO         (1 << 5)
#define HDMITX_EDID_PT_INTERLACED     (1 << 7)

/* If detailed data is pixel timing */
struct detailed_pixel_timing {
	u8 hactive_lo;
	u8 hblank_lo;
	u8 hactive_hblank_hi;
	u8 vactive_lo;
	u8 vblank_lo;
	u8 vactive_vblank_hi;
	u8 hsync_offset_lo;
	u8 hsync_pulse_width_lo;
	u8 vsync_offset_pulse_width_lo;
	u8 hsync_vsync_offset_pulse_width_hi;
	u8 width_mm_lo;
	u8 height_mm_lo;
	u8 width_height_mm_hi;
	u8 hborder;
	u8 vborder;
	u8 misc;
} __attribute__((packed));

/* If it's not pixel timing, it'll be one of the below */
struct detailed_data_string {
	u8 str[13]; /* detail data string len is 13 */
} __attribute__((packed));

struct detailed_data_monitor_range {
	u8 min_vfreq;
	u8 max_vfreq;
	u8 min_hfreq_khz;
	u8 max_hfreq_khz;
	u8 pixel_clock_mhz; /* need to multiply by 10 */
	u8 flags;
	union {
		struct {
			u8 reserved;
			u8 hfreq_start_khz; /* need to multiply by 2 */
			u8 c;               /* need to divide by 2 */
			u16 m;
			u8 k;
			u8 j; /* need to divide by 2 */
		} __attribute__((packed)) gtf2;
		struct {
			u8 version;
			u8 data1; /* high 6 bits: extra clock resolution */
			u8 data2; /* plus low 2 of above: max hactive */
			u8 supported_aspects;
			u8 flags; /* preferred aspect and blanking support */
			u8 supported_scalings;
			u8 preferred_refresh;
		} __attribute__((packed)) cvt;
	} formula;
} __attribute__((packed));

struct detailed_data_wpindex {
	u8 white_yx_lo; /* Lower 2 bits each */
	u8 white_x_hi;
	u8 white_y_hi;
	u8 gamma; /* need to divide by 100 then add 1 */
} __attribute__((packed));

struct detailed_data_color_point {
	u8 windex1;
	u8 wpindex1[3]; /* array size is 3 */
	u8 windex2;
	u8 wpindex2[3]; /* array size is 3 */
} __attribute__((packed));

struct cvt_timing {
	u8 code[3]; /* array size is 3 */
} __attribute__((packed));

struct detailed_non_pixel {
	u8 pad1;
	u8 type;
	/*
	*  ff=serial, fe=string, fd=monitor range, fc=monitor name
	*  fb=color point data, fa=standard timing data,
	*  f9=undefined, f8=mfg. reserved
	*/
	u8 pad2;
	union {
		struct detailed_data_string str;
		struct detailed_data_monitor_range range;
		struct detailed_data_wpindex color;
		struct std_timing timings[6]; /* array size is 6 */
		struct cvt_timing cvt[4]; /* array size is 4 */
	} data;
} __attribute__((packed));

#define EDID_DETAILED_EST_TIMINGS     0xf7
#define EDID_DETAILED_CVT_3BYTE       0xf8
#define EDID_DETAILED_COLOR_MGMT_DATA 0xf9
#define EDID_DETAILED_STD_MODES       0xfa
#define EDID_DETAILED_MONITOR_CPDATA  0xfb
#define EDID_DETAILED_MONITOR_NAME    0xfc
#define EDID_DETAILED_MONITOR_RANGE   0xfd
#define EDID_DETAILED_MONITOR_STRING  0xfe
#define EDID_DETAILED_MONITOR_SERIAL  0xff

struct detailed_timing {
	u16 pixel_clock; /* pixel_clock * 10KHz */
	union {
		struct detailed_pixel_timing pixel_data;
		struct detailed_non_pixel other_data;
	} data;
} __attribute__((packed));

#define HDMITX_EDID_INPUT_SERRATION_VSYNC (1 << 0)
#define HDMITX_EDID_INPUT_SYNC_ON_GREEN   (1 << 1)
#define HDMITX_EDID_INPUT_COMPOSITE_SYNC  (1 << 2)
#define HDMITX_EDID_INPUT_SEPARATE_SYNCS  (1 << 3)
#define HDMITX_EDID_INPUT_BLANK_TO_BLACK  (1 << 4)
#define HDMITX_EDID_INPUT_VIDEO_LEVEL     (3 << 5)
#define HDMITX_EDID_INPUT_DIGITAL         (1 << 7)
#define HDMITX_EDID_DIGITAL_DEPTH_MASK    (7 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_UNDEF   (0 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_6       (1 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_8       (2 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_10      (3 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_12      (4 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_14      (5 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_16      (6 << 4)
#define HDMITX_EDID_DIGITAL_DEPTH_RSVD    (7 << 4)
#define HDMITX_EDID_DIGITAL_TYPE_UNDEF    (0)
#define HDMITX_EDID_DIGITAL_TYPE_DVI      (1)
#define HDMITX_EDID_DIGITAL_TYPE_HDMITX_A   (2)
#define HDMITX_EDID_DIGITAL_TYPE_HDMITX_B   (3)
#define HDMITX_EDID_DIGITAL_TYPE_MDDI     (4)
#define HDMITX_EDID_DIGITAL_TYPE_DP       (5)

#define HDMITX_EDID_FEATURE_DEFAULT_GTF      (1 << 0)
#define HDMITX_EDID_FEATURE_PREFERRED_TIMING (1 << 1)
#define HDMITX_EDID_FEATURE_STANDARD_COLOR   (1 << 2)
/* If analog */
/* 00:mono, 01:rgb, 10:non-rgb, 11:unknown */
#define HDMITX_EDID_FEATURE_DISPLAY_TYPE (3 << 3)
/* If digital */
#define HDMITX_EDID_FEATURE_COLOR_MASK   (3 << 3)
#define HDMITX_EDID_FEATURE_RGB          (0 << 3)
#define HDMITX_EDID_FEATURE_RGB_YCRCB444 (1 << 3)
#define HDMITX_EDID_FEATURE_RGB_YCRCB422 (2 << 3)
#define HDMITX_EDID_FEATURE_RGB_YCRCB    (3 << 3) /* both 4:4:4 and 4:2:2 */

#define HDMITX_EDID_FEATURE_PM_ACTIVE_OFF (1 << 5)
#define HDMITX_EDID_FEATURE_PM_SUSPEND    (1 << 6)
#define HDMITX_EDID_FEATURE_PM_STANDBY    (1 << 7)

#define HDMITX_EDID_HDMITX_DC_48   (1 << 6)
#define HDMITX_EDID_HDMITX_DC_36   (1 << 5)
#define HDMITX_EDID_HDMITX_DC_30   (1 << 4)
#define HDMITX_EDID_HDMITX_DC_Y444 (1 << 3)

/* YCBCR 420 deep color modes */
#define HDMITX_EDID_YCBCR420_DC_48   (1 << 2)
#define HDMITX_EDID_YCBCR420_DC_36   (1 << 1)
#define HDMITX_EDID_YCBCR420_DC_30   (1 << 0)
#define HDMITX_EDID_YCBCR420_DC_MASK (HDMITX_EDID_YCBCR420_DC_48 | \
	  HDMITX_EDID_YCBCR420_DC_36 | \
	  HDMITX_EDID_YCBCR420_DC_30)

#define HDMITX_IEEE_OUI       0x000c03
#define HDMITX_FORUM_IEEE_OUI 0xc45dd8
#define HDMITX_DOLBY_IEEE_OUI 0x00d046
#define HDMITX_CUVA_IEEE_OUI  0x047503

#define EDID_DOLBY_VSVDB_VERSION_0_LEN 0x19
#define EDID_DOLBY_VSVDB_VERSION_1_LEN 0x0e
#define EDID_DOLBY_VSVDB_VERSION_2_LEN 0x0b
#define EDID_DOLBY_VSVDB_VERSION_0     0x00
#define EDID_DOLBY_VSVDB_VERSION_1     0x01
#define EDID_DOLBY_VSVDB_VERSION_2     0x02

#define EDID_DOLBY_LOWER_2BIT_MASK   0x03
#define EDID_DOLBY_LOWER_3BIT_MASK   0x07
#define EDID_DOLBY_LOWER_NIBBLE_MASK 0x0F
#define EDID_DOLBY_LOWER_5BIT_MASK   0x1F
#define EDID_DOLBY_LOWER_7BIT_MASK   0x7F

/* AUDIO DATA BLOCK */
#define EDID_AUDIO_FORMAT_MASK   (0xf << 3)
#define EDID_AUDIO_CHANNEL_MASK  (0x7 << 0)
#define EDID_AUDIO_EXT_TYPE_CODE (0x1f << 3)

#define BIT0_MASK 0x01
#define BIT1_MASK 0x02
#define BIT2_MASK 0x04
#define BIT3_MASK 0x08
#define BIT4_MASK 0x10
#define BIT5_MASK 0x20
#define BIT6_MASK 0x40
#define BIT7_MASK 0x80

#define MODE_TYPE_DETAILED_TIMINGE    (1 << 0)
#define MODE_TYPE_ESTABLISHED_TIMINGE (1 << 1)
#define MODE_TYPE_STD_TIMINGE         (1 << 2)
#define MODE_TYPE_VSDB_4K             (1 << 3)
#define MODE_TYPE_VSDB_3D             (1 << 4)
#define MODE_TYPE_VDB                 (1 << 5)
#define MODE_TYPE_VDB_Y420CMDB        (1 << 6)
#define MODE_TYPE_Y420VDB             (1 << 7)

struct edid {
	u8 header[8]; /* array size is 8 */
	/* Vendor & product info */
	u8 mfg_id[2]; /* array size is 2 */
	u8 prod_code[2]; /* array size is 2 */
	u32 serial;
	u8 mfg_week;
	u8 mfg_year;
	/* EDID version */
	u8 version;
	u8 revision;
	/* Display info: */
	u8 input;
	u8 width_cm;
	u8 height_cm;
	u8 gamma; /* real value = (gamma+100)/100 ,range [1.0~3.54] */
	u8 features;
	/* Color characteristics */
	u8 red_green_lo;
	u8 black_white_lo;
	u8 red_x;
	u8 red_y;
	u8 green_x;
	u8 green_y;
	u8 blue_x;
	u8 blue_y;
	u8 white_x;
	u8 white_y;
	/* Est. timings and mfg rsvd timings */
	struct est_timings established_timings;
	/* Standard timings 1-8 */
	struct std_timing standard_timings[8];
	/* Detailing timings 1-4 */
	struct detailed_timing detailed_timings[4];
	/* Number of 128 byte ext. blocks */
	u8 extensions;
	/* Checksum */
	u8 checksum;
} __attribute__((packed));

/* Short Audio Descriptor */
struct cea_sad {
	u8 format;
	u8 channels; /* max number of channels - 1 */
	u8 freq;
	u8 byte2; /* meaning depends on format */
};

/* use for edid vsdb */
struct edid_vsdb_info {
	u32 diff_len;
	u8 video_len;
	u32 offset;
};

bool drv_hdmitx_edid_data_is_zero(const u8 *in_edid, u32 length);
bool drv_hdmitx_edid_block_is_valid(u8 *raw_edid, u32 block, bool print_bad_edid);
u32 drv_hdmitx_edid_add_modes(struct hdmitx_connector *connector, const struct edid *edid);

#endif
