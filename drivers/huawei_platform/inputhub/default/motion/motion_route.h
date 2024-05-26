/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: motion route header file
 * Author: linjianpeng <linjianpeng1@huawei.com>
 * Create: 2020-05-25
 */

#ifndef __MOTION_ROUTE_H__
#define __MOTION_ROUTE_H__

#include "motion_detect.h"

#define ACTIVITY_DATA_LENGTH 3
#define AUDIO_SPEAKER_ON    3
#define AUDIO_SPEAKER_OFF   2
#define MOTION_FINGER_UP    6
#define MOTION_FINGER_DOWN  5
#define APK_CONFIG_DATA_LEN 16
#define RET_FAIL (-1)
#define RET_SUCC 0
struct motion_fall_down_data {
	uint8_t fall_status;
	uint8_t suspected_cnt;
	int8_t confidence;
	uint8_t impact_force;
	int32_t still_time;
	int32_t fall_height;
};

struct pkt_motion_data {
	int8_t motion_type;
	uint8_t motion_result;
	int8_t motion_status;
	uint8_t data_len;
	int32_t data[ACTIVITY_DATA_LENGTH];
};

struct pkt_step_counter_motion_req {
	pkt_common_data_t data_hd;
	struct pkt_motion_data mt;
};

struct motion_sf_data_t {
	uint8_t spk_value;
	uint8_t finger_value;
};

int inputhub_process_motion_report(const struct pkt_header *head);
bool is_motion_data_report(const struct pkt_header *head);
void process_step_counter_report(const struct pkt_header *head);
void motion_speaker_status_change(unsigned long value);
void motion_finger_status_change(unsigned long value);

#endif
