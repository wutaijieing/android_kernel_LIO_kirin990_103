/*
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

#ifndef __DP_IRQ_H__
#define __DP_IRQ_H__

#include "dp_ctrl.h"

struct dp_ctrl;

#define MAX_EXT_BLOCKS 3
#define DP_DMD_REPORT_SIZE 900

int handle_hotplug(struct dp_ctrl *dptx);
int handle_hotunplug(struct dp_ctrl *dptx);
void dptx_link_params_reset(struct dp_ctrl *dptx);

struct test_dtd {
	uint32_t h_total;
	uint32_t v_total;
	uint32_t h_start;
	uint32_t v_start;
	uint32_t h_sync_width;
	uint32_t v_sync_width;
	uint32_t h_width;
	uint32_t v_width;
	uint32_t h_sync_pol;
	uint32_t v_sync_pol;
	uint32_t refresh_rate;
	enum video_format_type video_format;
	uint8_t vmode;
};

static const struct test_dtd test_timing[] = {
	/*
	 * h_total, v_total, h_start, v_start, h_sync_width, v_sync_width,
	 * h_width, v_width, h_sync_pol, v_sync_pol, refresh_rate
	 */
	{1056, 628, 216, 27, 128, 4, 800, 600, 0, 0, 0, DMT, 9}, /* DMT-9 */
	{1088, 517, 224, 31, 112, 8, 848, 480, 0, 0, 0, DMT, 14}, /* DMT-14 */
	{1344, 806, 296, 35, 136, 6, 1024, 768, 0, 0, 0, DMT, 16}, /* DMT-16 */
	{1440, 790, 112, 19, 32, 7, 1280, 768, 0, 0, 0, DMT, 22}, /* DMT-22 */
	{1664, 798, 320, 27, 128, 7, 1280, 768, 0, 0, 0, DMT, 23}, /* DMT-23 */
	{1440, 823, 112, 20, 32, 6, 1280, 800, 0, 0, 0, DMT, 27}, /* DMT-27 */
	{1800, 1000, 424, 39, 112, 3, 1280, 960, 0, 0, 0, DMT, 32}, /* DMT-32 */
	{1688, 1066, 360, 41, 112, 3, 1280, 1024, 0, 0, 0, DMT, 35}, /* DMT-35 */
	{1792, 795, 368, 24, 112, 6, 1360, 768, 0, 0, 0, DMT, 39}, /* DMT-39 */
	{1560, 1080, 112, 27, 32, 4, 1400, 1050, 0, 0, 0, DMT, 41}, /* DMT-41 */
	{2160, 1250, 496, 49, 192, 3, 1600, 1200, 0, 0, 0, DMT, 51}, /* DMT-51 */
	{2448, 1394, 528, 49, 200, 3, 1792, 1344, 0, 0, 0, DMT, 62}, /* DMT-62 */
	{2600, 1500, 552, 59, 208, 3, 1920, 1440, 0, 0, 0, DMT, 73}, /* DMT-73 */
	{2200, 1125, 192, 41, 44, 5, 1920, 1080, 0, 0, 0, DMT, 82}, /* DMT-82/VCEA-63 */
	{800, 525, 144, 35, 96, 2, 640, 480, 0, 0, 0, VCEA, 1}, /* VCEA-1 */
	{1650, 750, 260, 25, 40, 5, 1280, 720, 0, 0, 0, VCEA, 4}, /* VCEA-4 */
	{1680, 831, 328, 28, 128, 6, 1280, 800, 0, 0, 0, CVT, 28}, /* CVT-28 */
	{1760, 1235, 112, 32, 32, 4, 1600, 1200, 0, 0, 0, CVT, 40}, /* CVT-40 */
	{2208, 1580, 112, 41, 32, 4, 2048, 1536, 0, 0, 0, CVT, 41}, /* CVT-41 */
};

int handle_sink_request(struct dp_ctrl *dptx);
int dptx_link_get_device_edid(struct dp_ctrl *dptx, bool *bsafemode, uint32_t *edid_info_size);

#endif /* DP_IRQ_H */
