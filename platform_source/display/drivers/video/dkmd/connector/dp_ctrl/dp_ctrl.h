/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#ifndef __DP_CTRL_H__
#define __DP_CTRL_H__

#include <linux/interrupt.h>
#include <linux/switch.h>
#include <linux/pci.h>
#include <drm/drm_dp_helper.h>
#include <drm/drm_fixed.h>
#include <drm/drm_dp_mst_helper.h>
#include "dpu_conn_mgr.h"
#include "dsc/dsc_algorithm.h"
#include "dp_hdr.h"

#define DPTX_RECEIVER_CAP_SIZE	(0x100)

#define DPTX_COMBOPHY_PARAM_NUM				20
#define DPTX_COMBOPHY_SSC_PPM_PARAM_NUM		12

#define DPTX_SDP_NUM		(0x10)
#define DPTX_SDP_LEN	(0x9)
#define DPTX_SDP_SIZE	(9 * 4)
#define DPTX_DEFAULT_EDID_BUFLEN	(128UL)

#define LSB_MASK 0x00FF
#define MSB_MASK 0xFF00
#define SHIFT_8BIT 8

#define DP_SYSFS_ATTRS_NUM	(10)

// TODO: stub useless interface
#define dp_imonitor_set_param(a, b)
#define dp_factory_mode_is_enable()  (0)
#define dp_factory_is_4k_60fps(a, b, c, d, e) (0)
#define dp_factory_link_cr_or_ch_eq_fail(a) (0)
#define dp_update_external_display_timming_info(a, b, c)

enum {
	DP_MODE = 0,
	DP_TV_MODE = 1,
	EDP_MODE = 2,
	HIDIF_MODE = 3,
	DP_MODE_MAX,
};

enum dp_tx_hpd_states {
	HPD_OFF,
	HPD_ON,
};

enum pixel_enc_type {
	RGB = 0,
	YCBCR420 = 1,
	YCBCR422 = 2,
	YCBCR444 = 3,
	YONLY = 4,
	RAW = 5
};

enum color_depth {
	COLOR_DEPTH_INVALID = 0,
	COLOR_DEPTH_6 = 6,
	COLOR_DEPTH_8 = 8,
	COLOR_DEPTH_10 = 10,
	COLOR_DEPTH_12 = 12,
	COLOR_DEPTH_16 = 16
};

enum pattern_mode {
	TILE = 0,
	RAMP = 1,
	CHESS = 2,
	COLRAMP = 3
};

enum dynamic_range_type {
	CEA = 1,
	VESA = 2
};

enum colorimetry_type {
	ITU601 = 1,
	ITU709 = 2
};

enum video_format_type {
	VCEA = 0,
	CVT = 1,
	DMT = 2
};

enum established_timings {
	DMT_640X480_60HZ,
	DMT_800X600_60HZ,
	DMT_1024X768_60HZ,
	NONE
};

enum iec_samp_freq_value {
	IEC_SAMP_FREQ_44K = 0,
	IEC_SAMP_FREQ_48K = 2,
	IEC_SAMP_FREQ_32K = 3,
	IEC_SAMP_FREQ_88K = 8,
	IEC_SAMP_FREQ_96K = 10,
	IEC_SAMP_FREQ_176K = 12,
	IEC_SAMP_FREQ_192K = 14,
};

enum iec_orig_samp_freq_value {
	IEC_ORIG_SAMP_FREQ_192K = 1,
	IEC_ORIG_SAMP_FREQ_176K = 3,
	IEC_ORIG_SAMP_FREQ_96K = 5,
	IEC_ORIG_SAMP_FREQ_88K = 7,
	IEC_ORIG_SAMP_FREQ_32K = 12,
	IEC_ORIG_SAMP_FREQ_48K = 13,
	IEC_ORIG_SAMP_FREQ_44K = 15,
};

enum dptx_hot_plug_type {
	HOT_PLUG_OUT = 0,
	HOT_PLUG_IN,
	HOT_PLUG_TEST,
	HOT_PLUG_IN_VR,
	HOT_PLUG_OUT_VR,
	HOT_PLUG_HDCP13_SUCCESS,
	HOT_PLUG_HDCP13_TIMEOUT,
	HOT_PLUG_HDCP13_FAIL,
	HOT_PLUG_HDCP13_POSTAUTH,
	HOT_PLUG_HDCP_ENABLE,
	HOT_PLUG_HDCP_DISABLE,
	HOT_PLUG_HDCP_OUT,
	HOT_PLUG_HDCP_CP_IRQ,
	HOT_PLUG_TEST_OUT,
	HOT_PLUG_MAINPANEL_UP,
	HOT_PLUG_MAINPANEL_DOWN,
};

enum dptx_sdp_config_type {
	SDP_CONFIG_TYPE_DSC,
	SDP_CONFIG_TYPE_HDR,
	SDP_CONFIG_TYPE_AUDIO,
};

/**
 * struct dptx_link - The link state.
 * @status: Holds the sink status register values.
 * @trained: True if the link is successfully trained.
 * @rate: The current rate that the link is trained at.
 * @lanes: The current number of lanes that the link is trained at.
 * @preemp_level: The pre-emphasis level used for each lane.
 * @vswing_level: The vswing level used for each lane.
 */
struct dptx_link {
	uint8_t status[DP_LINK_STATUS_SIZE];
	bool trained;
	uint8_t rate;
	uint8_t lanes;
	uint8_t preemp_level[4];
	uint8_t vswing_level[4];
};

/**
 * struct dptx_aux - The aux state
 * @sts: The AUXSTS register contents.
 * @data: The AUX data register contents.
 * @event: Indicates an AUX event ocurred.
 * @abort: Indicates that the AUX transfer should be aborted.
 */
struct dptx_aux {
	uint32_t sts;
	uint32_t data[4];
	atomic_t event;
	atomic_t abort;
};

struct sdp_header {
	uint8_t hb0;
	uint8_t hb1;
	uint8_t hb2;
	uint8_t hb3;
} __packed;

struct sdp_full_data {
	uint8_t en;
	uint32_t payload[9];
	uint8_t blanking;
	uint8_t cont;
} __packed;

enum _master_hdcp_op_type_ {
	DSS_HDCP13_ENABLE = 1,
	DSS_HDCP22_ENABLE,
	DSS_HDCP13_ENCRYPT_ENABLE,
	DSS_HDCP_OBS_SET,
	DSS_HDCP_INT_CLR,
	DSS_HDCP_INT_MASK,
	DSS_HDCP_CP_IRQ,
	DSS_HDCP_DPC_SEC_EN,
	DSS_HDCP_ENC_MODE_EN,
	DSS_HDCP_DP_ENABLE,
	HDCP_OP_SECURITY_MAX,
};

enum audio_sample_freq {
	SAMPLE_FREQ_32 = 0,
	SAMPLE_FREQ_44_1 = 1,
	SAMPLE_FREQ_48 = 2,
	SAMPLE_FREQ_88_2 = 3,
	SAMPLE_FREQ_96 = 4,
	SAMPLE_FREQ_176_4 = 5,
	SAMPLE_FREQ_192 = 6
};

struct audio_short_desc {
	uint8_t max_num_of_channels;
	enum audio_sample_freq max_sampling_freq;
	uint8_t max_bit_per_sample;
};

struct audio_params {
	uint8_t iec_channel_numcl0;
	uint8_t iec_channel_numcr0;
	uint8_t use_lut;
	uint8_t iec_samp_freq;
	uint8_t iec_word_length;
	uint8_t iec_orig_samp_freq;
	uint8_t data_width;
	uint8_t num_channels;
	uint8_t inf_type;
	uint8_t mute;
	uint8_t ats_ver;
};

struct dtd {
	uint64_t pixel_clock;
	uint16_t pixel_repetition_input;
	uint16_t h_active;
	uint16_t h_blanking;
	uint16_t h_image_size;
	uint16_t h_sync_offset;
	uint16_t h_sync_pulse_width;
	uint8_t h_sync_polarity;
	uint16_t v_active;
	uint16_t v_blanking;
	uint16_t v_image_size;
	uint16_t v_sync_offset;
	uint16_t v_sync_pulse_width;
	uint8_t v_sync_polarity;
	uint8_t interlaced; /* 1 for interlaced, 0 progressive */
};

struct dp_dsc_info {
	uint32_t lsteer_xmit_delay;
	uint32_t wait_cnt_int;
	uint32_t wait_cnt_frac;
	uint32_t encoders;
	uint32_t h_active_new;
	struct dsc_info dsc_info;
};

struct video_params {
	enum pixel_enc_type pix_enc;
	enum pattern_mode pattern_mode;
	struct dtd mdtd;
	uint8_t mode;
	enum color_depth bpc;
	enum colorimetry_type colorimetry;
	enum dynamic_range_type dynamic_range;
	uint8_t aver_bytes_per_tu;
	uint8_t aver_bytes_per_tu_frac;
	uint8_t init_threshold;
	uint32_t refresh_rate;
	uint8_t video_format;
	uint8_t m_fps;

	struct dp_dsc_info dp_dsc_info;
};

/* edid info */
struct timing_info {
	struct list_head list_node;

	uint8_t vsync_polarity;
	uint8_t hsync_polarity;

	uint64_t pixel_clock;
	uint16_t hactive_pixels;
	uint16_t hblanking;
	uint16_t hsync_offset;
	uint16_t hsync_pulse_width;
	uint16_t hborder;
	uint16_t hsize;
	uint16_t vactive_pixels;
	uint16_t vblanking;
	uint16_t vsync_offset;
	uint16_t vsync_pulse_width;
	uint16_t vborder;
	uint16_t vsize;
	uint16_t input_type;
	uint16_t interlaced;
	uint16_t sync_scheme;
	uint16_t scheme_detail;
};

struct ext_timing {
	uint16_t ext_format_code;
	uint16_t ext_hpixels;
	uint16_t ext_vpixels;
	uint16_t ext_vfreq;
};

struct edid_video {
	uint8_t main_vcount;
	uint8_t ext_vcount;

	uint16_t max_hpixels;
	uint16_t max_vpixels;
	uint16_t max_himage_size;
	uint16_t max_vimage_size;
	uint16_t max_vfreq;
	uint16_t min_vfreq;
	uint16_t max_hfreq;
	uint16_t min_hfreq;
	uint16_t max_pixel_clock;

	struct list_head *dptx_timing_list;
	struct ext_timing *ext_timing;
	char *dp_monitor_descriptor;
};

struct edid_audio_info {
	uint16_t format;
	uint16_t channels;
	uint16_t sampling;
	uint16_t bitrate;
};

struct edid_audio {
	struct edid_audio_info *spec;
	uint8_t basic_audio;
	uint8_t ext_speaker;
	uint8_t ext_acount;
};

struct edid_information {
	struct edid_video video;
	struct edid_audio audio;
};

/**
 * struct dp_ctrl - The representation of the DP TX core
 * @mutex:
 * @base: Base address of the registers
 * @irq: IRQ number
 * @version: Contents of the IP_VERSION register
 * @max_rate: The maximum rate that the controller supports
 * @max_lanes: The maximum lane count that the controller supports
 * @dev: The struct device
 * @root: The debugfs root
 * @regset: The debugfs regset
 * @vparams: The video params to use
 * @aparams: The audio params to use
 * @waitq: The waitq
 * @shutdown: Signals that the driver is shutting down and that all
 *            operations should be aborted.
 * @c_connect: Signals that a HOT_PLUG or HOT_UNPLUG has occurred.
 * @sink_request: Signals the a HPD_IRQ has occurred.
 * @rx_caps: The sink's receiver capabilities.
 * @edid: The sink's EDID.
 * @aux: AUX channel state for performing an AUX transfer.
 * @link: The current link state.
 * @multipixel: Controls multipixel configuration. 0-Single, 1-Dual, 2-Quad.
 */
struct dp_ctrl {
	struct mutex dptx_mutex; /* generic mutex for dptx */

	struct {
		uint8_t multipixel;
		uint8_t streams;
		bool gen2phy;
		bool dsc;
	} hwparams;

	void __iomem *base;
	uint32_t irq;

	uint8_t mode;
	uint8_t id;

	uint32_t version;
	uint8_t bstatus;
	uint8_t streams;
	uint8_t multipixel;
	uint8_t max_rate;
	uint8_t max_lanes;
	uint8_t dsc_decoders;
	uint8_t dsc_ifbc_type;

	/* mst */
	uint8_t rad_port;
	uint8_t port[2];
	uint16_t pbn;
	int active_mst_links;

	int vcp_id;

	bool mst;
	bool fec;
	bool dsc;
	bool ycbcr420;
	bool cr_fail;
	bool ssc_en;
	bool efm_en;
	bool need_rad;
	bool logic_port;

	bool dummy_dtds_present;

	bool dptx_underflow_clear;
	bool dptx_vr;
	bool dptx_enable;
	bool dptx_plug_type;
	bool dptx_detect_inited;
	bool same_source;
	bool video_transfer_enable;
	bool power_saving_mode;

	struct device *dev;
	struct switch_dev sdev;
	struct switch_dev dp_switch;
	struct dpu_connector *connector;

	struct video_params vparams;
	struct audio_params aparams;
	struct audio_short_desc audio_desc;
	struct hdr_metadata hdr_metadata;
	struct hdr_infoframe hdr_infoframe;

	struct edid_information edid_info;
	struct drm_dp_mst_topology_mgr mst_mgr;

	wait_queue_head_t dptxq;
	wait_queue_head_t waitq;

	atomic_t shutdown;
	atomic_t c_connect;
	atomic_t sink_request;

	uint8_t rx_caps[DPTX_RECEIVER_CAP_SIZE];

	uint8_t *edid;
	uint32_t edid_try_count;
	uint32_t edid_try_delay; /* unit: ms */

	struct sdp_full_data sdp_list[DPTX_SDP_NUM];
	struct dptx_aux aux;
	struct dptx_link link;

	uint8_t detect_times;
	uint8_t current_link_rate;
	uint8_t current_link_lanes;
	uint32_t hpd_state;
	uint32_t user_mode;
	uint32_t combophy_pree_swing[DPTX_COMBOPHY_PARAM_NUM];
	uint32_t combophy_ssc_ppm[DPTX_COMBOPHY_SSC_PPM_PARAM_NUM];
	uint32_t max_edid_timing_hactive;
	enum video_format_type user_mode_format;
	enum established_timings selected_est_timing;

	int sysfs_index;
	struct attribute *sysfs_attrs[DP_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;

	struct hrtimer dptx_hrtimer;
	struct workqueue_struct *dptx_check_wq;
	struct work_struct dptx_check_work;

	/*
	 * aux read/write
	 */
	int (*aux_rw)(struct dp_ctrl *dptx, bool rw, bool i2c, bool mot,
		bool addr_only, uint32_t addr, uint8_t *bytes, uint32_t len);

	/*
	 * irq
	 */
	void (*dptx_hpd_handler)(struct dp_ctrl *dptx, bool plugin, uint8_t dp_lanes);
	void (*dptx_hpd_irq_handler)(struct dp_ctrl *dptx);

	/*
	 * link
	 */
	int (*handle_hotplug)(struct dp_ctrl *dptx);
	int (*handle_hotunplug)(struct dp_ctrl *dptx);
	int (*dptx_power_handler)(struct dp_ctrl *dptx, bool ublank);

	/*
	 * dsc
	 */
	int (*dptx_slice_height_limit)(struct dp_ctrl *dptx, uint32_t pic_height);
	int (*dptx_line_buffer_depth_limit)(uint8_t line_buf_depth);
	void (*dptx_dsc_check_rx_cap)(struct dp_ctrl *dptx);
	void (*dptx_dsc_para_init)(struct dp_ctrl *dptx);
	void (*dptx_dsc_sdp_manul_send)(struct dp_ctrl *dptx, bool enable);
	void (*dptx_dsc_cfg)(struct dp_ctrl *dptx);

	/*
	 * link training
	 */
	int (*dptx_link_set_lane_status)(struct dp_ctrl *dptx); /* set lane num/rate/ssc ... */
	void (*dptx_link_set_lane_after_training)(struct dp_ctrl *dptx);
	void (*dptx_phy_set_pattern)(struct dp_ctrl *dptx, uint32_t pattern);
	void (*dptx_link_set_preemp_vswing)(struct dp_ctrl *dptx);

	/*
	 * core config
	 */
	int (*dp_dis_reset)(struct dp_ctrl *dptx, bool benable);
	int (*dptx_core_on)(struct dp_ctrl *dptx);
	void (*dptx_core_off)(struct dp_ctrl *dptx);
	void (*dptx_free_lanes)(struct dp_ctrl *dptx);
	void (*dptx_link_core_reset)(struct dp_ctrl *dptx);
	void (*dptx_default_params_from_core)(struct dp_ctrl *dptx); /* get default params decided by controller */
	bool (*dptx_sink_device_connected)(struct dp_ctrl *dptx);

	/*
	 * hdr infoframe
	 */
	void (*dptx_hdr_infoframe_set_reg)(struct dp_ctrl *dptx, uint8_t enable);

	/*
	 * stream config
	 */
	void (*dptx_video_core_config)(struct dp_ctrl *dptx, int stream); /* only video param config */
	void (*dptx_video_ts_change)(struct dp_ctrl *dptx, int stream); /* tu change config */
	void (*dptx_sdp_config)(struct dp_ctrl *dptx, int stream, enum dptx_sdp_config_type sdp_type);
	int (*dptx_video_ts_calculate)(struct dp_ctrl *dptx, int lane_num, int rate,
		int bpc, int encoding, int pixel_clock);
	/* include video/tu config and feature config like dsc */
	int (*dptx_link_timing_config)(struct dp_ctrl *dptx);
	/* disable stream when hotunplug */
	void (*dptx_link_close_stream)(struct dp_ctrl *dptx);
	/* enable/disable fec */
	int (*dptx_fec_enable)(struct dp_ctrl *dptx, bool fec_enable);

	/*
	 * audio config
	 */
	void (*dptx_audio_config)(struct dp_ctrl *dptx);

	/*
	 * media enable/disable
	 */
	void (*dptx_enable_default_video_stream)(struct dp_ctrl *dptx, int stream); /* stream enable */
	void (*dptx_disable_default_video_stream)(struct dp_ctrl *dptx, int stream); /* stream disable */
	/* enable/disable ldi and audio channel */
	int (*dptx_triger_media_transfer)(struct dp_ctrl *dptx, bool benable);
	int (*dptx_resolution_switch)(struct dp_ctrl *dptx, enum dptx_hot_plug_type etype);
	/* enable/disable phy transmitter on per lane */
	void (*dptx_phy_enable_xmit)(struct dp_ctrl *dptx, uint32_t lane, bool enable);

	/*
	 * phy config
	 */
	void (*dptx_combophy_set_preemphasis_swing)(struct dp_ctrl *dptx,
		uint32_t lane, uint32_t sw_level, uint32_t pe_level);
	int (*dptx_change_physetting_hbr2)(struct dp_ctrl *dptx); /* V510 Hardware patch */
	void (*dptx_combophy_set_rate)(struct dp_ctrl *dptx, int rate);
	/* V510 Hardware patch, phy set before and after set ssc */
	void (*dptx_combophy_set_ssc_addtions)(uint8_t link_rate, bool before);
	void (*dptx_combophy_set_ssc_dis)(struct dp_ctrl *dptx, bool ssc_disable);
	void (*dptx_aux_disreset)(struct dp_ctrl *dptx, bool enable);
	void (*dptx_combophy_set_lanes_power)(bool bopen);

	/*
	 * for test
	 */
	void (*dptx_audio_num_ch_change)(struct dp_ctrl *dptx);
	void (*dptx_video_timing_change)(struct dp_ctrl *dptx, int stream);
	void (*dptx_video_bpc_change)(struct dp_ctrl *dptx, int stream);
	void (*dptx_audio_infoframe_sdp_send)(struct dp_ctrl *dptx);
	int (*handle_test_link_phy_pattern)(struct dp_ctrl *dptx);
	void (*dptx_phy_set_ssc)(struct dp_ctrl *dptx, bool bswitchphy);
};

static inline uint32_t dptx_readl(struct dp_ctrl *dp, uint32_t offset)
{
	uint32_t data = readl(dp->base + offset);

	return data;
}

static inline void dptx_writel(struct dp_ctrl *dp, uint32_t offset, uint32_t data)
{
	writel(data, dp->base + offset);
}

static inline void dptx_notify(struct dp_ctrl *dptx)
{
	wake_up_interruptible(&dptx->waitq);
}

static inline void dptx_notify_shutdown(struct dp_ctrl *dptx)
{
	atomic_set(&dptx->shutdown, 1);
	wake_up_interruptible(&dptx->waitq);
}

/*
 * Wait functions
 */
#define dptx_wait(_dptx, _cond, _timeout)				\
	({								\
		int __retval;						\
		__retval = wait_event_interruptible_timeout(		\
			_dptx->waitq,					\
			((_cond) || (atomic_read(&_dptx->shutdown))),	\
			msecs_to_jiffies(_timeout));			\
		if (atomic_read(&_dptx->shutdown)) {			\
			__retval = -ESHUTDOWN;				\
		}							\
		else if (!__retval) {					\
			__retval = -ETIMEDOUT;				\
		}							\
		__retval;						\
	})

#endif
