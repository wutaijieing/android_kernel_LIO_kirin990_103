/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#ifndef __DKMD_CONNECTOR_H__
#define __DKMD_CONNECTOR_H__

#include "dkmd_object.h"

#define FPGA_EVENT_TIMEOUT_MS 10000
#define ASIC_EVENT_TIMEOUT_MS 300

enum {
	DSI_VIDEO_MODE,
	DSI_CMD_MODE,
};

enum {
	DSI_1_LANES = 0,
	DSI_2_LANES,
	DSI_3_LANES,
	DSI_4_LANES,
};

/* Now all the way to send display up to choose two connector,
 * such as cphy 1+1 or dual-mipi or dual-dp
 */
enum {
	PRIMARY_CONNECT_CHN_IDX  = 0,
	EXTERNAL_CONNECT_CHN_IDX = 1,
	MAX_CONNECT_CHN_NUM  = 2,
};

enum pipe_ops_cmd {
	SETUP_ISR = 0,
	ENABLE_ISR = 1,
	DISABLE_ISR = 2,
	DO_CLEAR = 3,   // do clear when underflow

	SET_FASTBOOT = 4,
	INIT_SPR = 5,
	INIT_DSC = 6,
	SET_BACKLIGHT = 7,

	PIPE_OPS_CMD_MAX,
};

/* this structure do not contain pointers */
struct dkmd_connector_info {
	struct dkmd_object_info base;

	uint32_t product_type;
	uint32_t bpp;
	uint32_t orientation;
	uint32_t bgr_fmt;
	uint32_t vsync_ctrl_type;
	uint32_t ifbc_type;

	uint8_t color_temperature_support;
	uint8_t lcd_uninit_step_support;
	uint8_t esd_enable;
	uint8_t frc_enable;
	uint8_t dirty_region_updt_support;
	uint8_t colorbar_enable; /* need config frm_valid_dbg bit[29]=0 */

	uint32_t active_flag;
	uint32_t sw_post_chn_num;
	uint32_t sw_post_chn_idx[MAX_CONNECT_CHN_NUM];

	/* pointer to connector manager device, get connector manager interface through it */
	struct platform_device *conn_device;

	void (*enable_ldi)(struct dkmd_connector_info *pinfo);
	void (*disable_ldi)(struct dkmd_connector_info *pinfo);
};

struct dkmd_conn_handle_data {
	struct dkmd_connector_info *conn_info;

	int32_t (*on_func)(struct dkmd_connector_info *pinfo);
	int32_t (*off_func)(struct dkmd_connector_info *pinfo);
	int32_t (*ops_handle_func)(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value);
};

/**
 * @brief Composer exposed outside the registered interface
 *
 * @param pinfo A pointer to a device to be registered
 * @return int32_t 0: success other: fail
 */
int register_connector(struct dkmd_connector_info *conn_info);
int unregister_connector(struct dkmd_connector_info *pinfo);
void connector_device_shutdown(struct dkmd_connector_info *pinfo);

int register_composer(struct dkmd_connector_info *pinfo);
void unregister_composer(struct dkmd_connector_info *pinfo);
void composer_device_shutdown(struct dkmd_connector_info *pinfo);

/*
 * used for connector to active vsync for exit ulps, such as set backlight
 * need_active_vsync = true, means exit ulps,
 * need_active_vsync = false, means would enter ulps
 */
void composer_active_vsync(struct dkmd_connector_info *pinfo, bool need_active_vsync);

#endif
