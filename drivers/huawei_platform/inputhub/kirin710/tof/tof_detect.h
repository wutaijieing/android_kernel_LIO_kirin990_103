/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: tof detect head file
 */

#ifndef __TOF_DETECT_H__
#define __TOF_DETECT_H__

#include "sensor_detect.h"

#define VI5300_CALIB_SHOW_BUF_SIZE 64
#define VL53LX_XTALK_HISTO_BINS 12
#define VL53LX_MAX_BIN_SEQUENCE_LENGTH 6
#define VL53LX_HISTOGRAM_BUFFER_SIZE 24
#define VL53LX_NVM_PEAK_RATE_MAP_SAMPLES 25
#define VL53LX_BIN_REC_SIZE 6
#define VL53LX_XTALK_SHAPE_BIN_SIZE 4
#define VL53LX_XTALK_REMOVED_BIN_SIZE 4
#define VL53L3_CALIB_DATA_PKG_SIZE 96
#define VL53L3_CALIB_DATA_PKG_OFFSET 40
#define VL53L3_CALIB_DATA_OFFSET 192
#define VL53L3_CALIB_SHOW_BUF_SIZE 256
#define VL53L3_CALIB_DATA_BUF_NUM 3
#define DOUBLE_TOF_CALIB_DATA_BUF_NUM (VL53L3_CALIB_DATA_BUF_NUM * 2)
#define CALIB_DATA_MAX_BUF_SIZE 128
#define VL53L3_CALIB_DATA_SIZE 256
#define RSV452_NV_NUM 452
#define RSV453_NV_NUM 453
#define RSV454_NV_NUM 454
#define RSV455_NV_NUM 455
#define RSV456_NV_NUM 456
#define RSV457_NV_NUM 457

typedef enum {
	CALIB_DATA_PKG_FIRST_PART,
	CALIB_DATA_PKG_SECOND_PART,
	CALIB_DATA_PKG_THIRD_PART,
	CALIB_DATA_PKG_FORTH_PART,
	CALIB_DATA_PKG_FIFTH_PART,
	CALIB_DATA_PKG_SIXTH_PART,
} tof_vl53l3_calib_pkg_num;

struct tof_calibrate_nv_info {
	int nv_number;
	int length;
	char *nv_name;
	int bias;
};

static const struct tof_calibrate_nv_info tof_nv_info[] = {
	{ RSV452_NV_NUM, VL53L3_CALIB_DATA_PKG_SIZE, "RSV452", 0 },
	{ RSV453_NV_NUM, VL53L3_CALIB_DATA_PKG_SIZE, "RSV453", VL53L3_CALIB_DATA_PKG_SIZE },
	{ RSV454_NV_NUM, VL53L3_CALIB_DATA_PKG_OFFSET, "RSV454", VL53L3_CALIB_DATA_OFFSET },
	{ RSV455_NV_NUM, VL53L3_CALIB_DATA_PKG_SIZE, "RSV455", 0 },
	{ RSV456_NV_NUM, VL53L3_CALIB_DATA_PKG_SIZE, "RSV456", VL53L3_CALIB_DATA_PKG_SIZE },
	{ RSV457_NV_NUM, VL53L3_CALIB_DATA_PKG_OFFSET, "RSV457", VL53L3_CALIB_DATA_OFFSET },
};

struct tof_vi5300_calibrate_data {
	int32_t sensor_type;
	int8_t xtalk_calib;
	uint16_t xtalk_peak;
	uint16_t xtalk_check;
	int16_t offset_calib;
	uint16_t offset_check;
};

struct tof_vl53l3_calibrate_cmd_to_mcu {
	uint8_t tag;
	uint8_t pkg_num;
	int32_t calibrate_mode;
};

struct tof_vl53l3_calibrate_data {
	int32_t sensor_type;
	uint8_t global_config_spad_enables_ref_0;
	uint8_t global_config_spad_enables_ref_1;
	uint8_t global_config_spad_enables_ref_2;
	uint8_t global_config_spad_enables_ref_3;
	uint8_t global_config_spad_enables_ref_4;
	uint8_t global_config_spad_enables_ref_5;
	uint8_t global_config_ref_en_start_select;
	uint8_t ref_spad_man_num_requested_ref_spads;
	uint8_t ref_spad_man_ref_location;
	uint16_t ref_spad_char_total_rate_target_mcps;
	uint32_t algo_crosstalk_compensation_plane_offset_kcps;
	int16_t algo_crosstalk_compensation_x_plane_gradient_kcps;
	int16_t algo_crosstalk_compensation_y_plane_gradient_kcps;
	uint32_t algo_xtalk_cpo_histomerge_kcps[VL53LX_BIN_REC_SIZE];
	uint32_t xtalk_shape_bin_data[VL53LX_XTALK_HISTO_BINS];
	uint16_t phasecal_result_reference_phase;
	uint8_t  phasecal_result_vcsel_start;
	uint8_t  cal_config_vcsel_start;
	uint16_t vcsel_width;
	uint16_t vl53lx_p_015;
	uint16_t zero_distance_phase;
	uint8_t cfg_device_state;
	uint8_t rd_device_state;
	uint8_t vl53lx_p_019;
	uint8_t vl53lx_p_020;
	uint8_t vl53lx_p_021;
	uint8_t xtalk_hist_removed_bin_seq[VL53LX_MAX_BIN_SEQUENCE_LENGTH];
	uint8_t xtalk_hist_removed_bin_rep[VL53LX_MAX_BIN_SEQUENCE_LENGTH];
	uint16_t xtalk_check;
	uint32_t xtalk_hist_removed_bin_data[VL53LX_HISTOGRAM_BUFFER_SIZE];
	int16_t algo_part_to_part_range_offset_mm;
	int16_t mm_config_inner_offset_mm;
	int16_t mm_config_outer_offset_mm;
	uint16_t offset_check;
};

struct vl53l3_calib_data_pkg {
	int32_t sensor_type;
	uint8_t pkg_num;
	uint8_t tag;
	uint8_t length;
	uint8_t buffer[VL53L3_CALIB_DATA_PKG_SIZE];
};

struct tof_dts_data {
	uint32_t bus_num;
	uint32_t dev_addr;
	uint32_t chip_id_reg;
	uint32_t chip_id_val;
	GPIO_NUM_TYPE gpio_xshut;
	GPIO_NUM_TYPE gpio_irq;
	uint16_t poll_interval;
	uint8_t enable;
};

struct tof_platform_data_ext {
	struct sensor_combo_cfg cfg;
	struct tof_dts_data vi5300;
	struct tof_dts_data vl53l3;
};

void read_tof_data_from_dts_ext(enum sensor_detect_list s_id, pkt_sys_dynload_req_t *dyn_req,
	struct device_node *dn, int32_t *tof_sensor_flag);
int32_t tof_sensor_detect(struct device_node *dn, struct sensor_detect_manager *sm, int32_t index);
void tof_detect_init(struct sensor_detect_manager *sm, uint32_t len);
void tof_vl53l3_calib_data_dump(const struct tof_vl53l3_calibrate_data *data,
	int __attribute__((unused)) len);

#endif