/** @file
 * Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_DSS_CONTEXT_H_
#define HISI_DSS_CONTEXT_H_

enum intf_timing_code {
	TIMING_480P,   // 720 x 480
	TIMING_1080P,  // 1920 x 1080
	TIMING_MAX,
};

enum dpu_power_state {
	STATE_OFF = 0,
	STATE_ON = 1,
};

enum dss_displaysink_id {
	DISPLAY_SINK_NONE = 0,
	DISPLAY_SINK_DSI0,
	DISPLAY_SINK_DPTX0,
	DISPLAY_SINK_DPTX1,
	DISPLAY_SINK_DDR,
	DISPLAY_SINK_MAX,
};

enum dss_inputsource_id {
	INPUT_SOURCE_NONE = 0,
	INPUT_SOURCE_DDR,
	INPUT_SOURCE_DPRX0,
	INPUT_SOURCE_DPRX1,
	INPUT_SOURCE_HDMIIN,
	INPUT_SOURCE_MAX,
};

enum dss_intf_type {
	INTF_DISPLAY_SINK,
	INTF_INPUT_SOURCE,
};

enum dss_psr_type {
	PSR_NONE,
	PSR_VER1,
	PSR_VER2, // SU update
};

enum dss_framerate_type {
	FRAMERATE_FIXED,
	FRAMERATE_ADAPTIVE, // VRR
};

enum dss_pipeline_state {
	STATE_000, // RX(off), TX(off), DPU(off)
	STATE_001, // RX(off), TX(off), DPU(on)
	STATE_011, // RX(off), TX(on), DPU(on)
	STATE_100, // RX(on), TX(off), DPU(off)
	STATE_101, // RX(on), TX(off), DPU(on)
	STATE_111, // RX(on), TX(off), DPU(on)
	STATE_MAX,
};

enum dss_notify_event {
	INPUTSOURCE_DISCONNECTED,
	INPUTSOURCE_CONNECTED,

	DISPLAYSINK_DISCONNECTED,
	DISPLAYSINK_CONNECTED,
	DISPLAYSINK_VIDEO_ACTIVATED,

	CONTEXT_READY,

	RECONNECT_INPUTSOURCE,

	NOTIFY_EVENT_MAX,
};

typedef struct dpu_dispintf_info {
	uint16_t timing_code;

	uint16_t format;

	uint16_t xres;
	uint16_t yres;
	uint16_t vporch;
	uint16_t hporch;

	uint16_t framerate;

	uint8_t psr;
	uint8_t vrr;
} dss_intfinfo_t;

typedef void* rx_context_t;
typedef void* tx_context_t;

typedef void (*notify_dsscontext_func)(uint32_t dev_id, uint32_t msg_id);

typedef void (*enable_inputsource_func)(rx_context_t rx_ctx, notify_dsscontext_func event_notify);
typedef void (*disable_inputsource_func)(rx_context_t rx_ctx);
typedef void (*enable_pipeline_func)(rx_context_t rx_ctx, bool on);
typedef void (*get_rxinfo_func)(rx_context_t rx_ctx, dss_intfinfo_t *info);

typedef void (*enable_displaysink_func)(tx_context_t tx_ctx, uint32_t inputsource, notify_dsscontext_func event_notify);
typedef void (*disable_displaysink_func)(tx_context_t tx_ctx);
typedef void (*get_txinfo_func)(tx_context_t tx_ctx, dss_intfinfo_t *info);

typedef struct dss_inputsource {
	rx_context_t rx_ctx;

	enable_inputsource_func enable_inputsource;
	disable_inputsource_func disable_inputsource;
	get_rxinfo_func get_information;
	enable_pipeline_func enable_pipeline;
} dss_inputsource_t;

typedef struct dss_displaysink {
	tx_context_t tx_ctx;

	enable_displaysink_func enable_displaysink;
	disable_displaysink_func disable_displaysink;
	get_txinfo_func get_information;
} dss_displaysink_t;

void dss_register_displaysink(uint32_t id, dss_displaysink_t *intf);
void dss_register_inputsource(uint32_t id, dss_inputsource_t *intf);

#endif
