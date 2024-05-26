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

#ifndef DKMD_DPU_H
#define DKMD_DPU_H

enum {
	DISP_IOCTL_CREATE_FENCE = 0x10,
	DISP_IOCTL_PRESENT,
	DISP_IOCTL_IOBLANK,
	DISP_IOCTL_GET_PRODUCT_CONFIG,
};

#define DISP_IOCTL_MAGIC 'D'

#define DISP_CREATE_FENCE _IOWR(DISP_IOCTL_MAGIC, DISP_IOCTL_CREATE_FENCE, int)
#define DISP_PRESENT _IOW(DISP_IOCTL_MAGIC, DISP_IOCTL_PRESENT, struct disp_frame)
#define DISP_IOBLANK _IOW(DISP_IOCTL_MAGIC, DISP_IOCTL_IOBLANK, int)

/**
 * @brief this cmd was used to get product config items,
 *        those itmes may were defined at dts or panel,
 *        each online scene must have product config items.
 */
#define DISP_GET_PRODUCT_CONFIG _IOR(DISP_IOCTL_MAGIC, DISP_IOCTL_GET_PRODUCT_CONFIG, struct product_config)

struct disp_layer {
	int32_t share_fd;
	int32_t acquired_fence;
};

struct intra_frame_dvfs_info {
	uint32_t time_wait;
	uint32_t perf_level;
	uint32_t active_time;
	uint32_t idle_level;
};

#define DISP_LAYER_MAX_COUNT 32
struct disp_frame {
	int32_t scene_id;
	uint32_t cmdlist_id;
	uint32_t layer_count;

	struct disp_layer layer[DISP_LAYER_MAX_COUNT];

	/* for offline compose output buffer release signal */
	int32_t out_present_fence;

	uint32_t frame_index;

	struct intra_frame_dvfs_info dvfs_info;

	uint32_t resevered;
};

#define SCENE_SPLIT_MAX 4
enum split_mode {
	SCENE_SPLIT_MODE_NONE = 0,
	SCENE_SPLIT_MODE_V,
	SCENE_SPLIT_MODE_H,
};

// dimension
enum dim_id {
	DIMENSION_NORMAL = 0,
	DIMENSION_MEDIA,
	DIMENSION_LOW,
	DIMENSION_RESERVED,
	DIMENSION_ID_MAX = 4
};

enum fps_id {
	FPS_NORMAL = 0,
	FPS_LEVEL1,
	FPS_LEVEL2,
	FPS_LEVEL3,
	FPS_LEVEL4,
	FPS_LEVEL5,
	FPS_LEVEL6,
	FPS_LEVEL7,
	FPS_LEVEL_MAX,
};

struct rog_dim {
	int32_t width;
	int32_t height;
};

enum color_mode {
	COLOR_MODE_NATIVE = 0,
	COLOR_MODE_STANDARD_BT601_625 = 1,
	COLOR_MODE_STANDARD_BT601_625_UNADJUSTED = 2,
	COLOR_MODE_STANDARD_BT601_525 = 3,
	COLOR_MODE_STANDARD_BT601_525_UNADJUSTED = 4,
	COLOR_MODE_STANDARD_BT709 = 5,
	COLOR_MODE_DCI_P3 = 6,
	COLOR_MODE_SRGB = 7,
	COLOR_MODE_ADOBE_RGB = 8,
	COLOR_MODE_DISPLAY_P3 = 9,
	COLOR_MODE_BT2020 = 10,
	COLOR_MODE_BT2100_PQ = 11,
	COLOR_MODE_BT2100_HLG = 12,
};

#define COLOR_MODE_COUNT_MAX 5

union feature_bits {
	uint64_t value;
	struct {
		uint64_t enable_hdr10 : 1;
		uint64_t enable_dsc : 1;
		uint64_t enable_spr : 1;
		uint64_t reserved : 61;
	} bits;
};

enum driver_framework {
	DRV_FB = 0,
	DRV_CHR,
	DRV_DRM
};

// used for display's driver and device feature
union driver_bits {
	uint64_t value;
	struct {
		uint64_t drv_framework : 3;
		uint64_t is_pluggable  : 1;
		uint64_t reserved      : 60;
	} bits;
};

/**
 * @brief color_modes is must be enum color_mode's value
 *
 */
struct product_config {
	// fix information
	union driver_bits drv_feature; // driver and device feature

	uint16_t display_count;
	uint8_t split_mode;
	uint8_t split_count;
	uint16_t split_ratio[SCENE_SPLIT_MAX];
	union feature_bits feature_switch; // product feature

	// var information
	uint32_t dim_info_count;
	struct rog_dim dim_info[DIMENSION_ID_MAX];

	uint32_t fps_info_count;
	uint32_t fps_info[FPS_LEVEL_MAX];

	uint32_t color_mode_count;
	uint32_t color_modes[COLOR_MODE_COUNT_MAX];

	uint32_t dsc_out_width;
	uint32_t dsc_out_height;
};

#endif