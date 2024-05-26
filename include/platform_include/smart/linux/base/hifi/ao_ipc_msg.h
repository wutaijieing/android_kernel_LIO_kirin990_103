/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: soc_dsp ao ipc msg head file
 * Create: 2020-9-18
 */

#ifndef _AO_IPC_MSF_H_
#define _AO_IPC_MSF_H_

#include "typedef.h"

enum ao_ipc_msg_tag {
	TAG_PS = 0x5,
	TAG_SCENE_DENOISE = 0x99,
};

enum hifi2sensor_hub_msg {
	HIFI_SEND_TO_IOM3_CMD = 7,
	IPC_TO_SENSORHUB_CMD_SCENE_DENOISE_OPEN = 0x61,
	IPC_TO_SENSORHUB_CMD_SCENE_DENOISE_DATA_READY = 0x62,
	IPC_TO_SENSORHUB_CMD_SCENE_DENOISE_CLOSE = 0x63,
	IPC_TO_SENSORHUB_CMD_SCENE_DENOISE_CHECK_BUFFER = 0x65,
};

enum sensor_hub2hifi_msg {
	AO_IPC_LIGHT_MSG_TYPE = 0,
	AO_IPC_SEND_TO_SENSERHUB_ULTRA_STATUS_TYPE,

	/* add for ultra mtp */
	AP_IPC_START_MTP,
	AP_IPC_UPDATE_MTP_CALIB_VAL,
	AP_IPC_SEND_MTP_STATUS,
	IPC_FROM_SENSORHUB_CMD_SCENE_DENOISE_GET_ADDR = 0x60,
	IPC_FROM_SENSORHUB_CMD_SCENE_DENOISE_GET_PARAM = 0x64,
	IPC_FROM_SENSORHUB_CMD_SCENE_DENOISE_BUFFER_READY = 0x66,
};

enum ultrasonic_hifi_ctrl_msg {
	/* add for ultra proximity */
	IPC_TO_SENSORHUB_ULTRA_DATA_FLG = 0,
	IPC_TO_SENSORHUB_ULTRA_OPEN_FLG,
	IPC_TO_SENSORHUB_ULTRA_CLOSE_FLG,
	IPC_TO_SENSORHUB_ULTRA_CLOSE_LIGHT,
	IPC_TO_SENSORHUB_FLG_IDX,

	/* add for ultra mtp */
	IPC_TO_SENSORHUB_MTP_DATA = 0x20,
	IPC_TO_SENSORHUB_MTP_OPEN,
	IPC_TO_SENSORHUB_MTP_CLOSE,

	IPC_TO_SENSORHUB_MTP_ANALOG_OPEN,
	IPC_TO_SENSORHUB_MTP_ANALOG_CLOSE,
	IPC_TO_SENSORHUB_MTP_START_FAIL,
	IPC_TO_SENSORHUB_MTP_UPDATE_CALIB_VAL_FAIL,
	IPC_TO_SENSORHUB_MTP_NOTHING_FLG
};

/* msg header */
struct ao_ipc_msg_header {
	uint8_t tag;
	uint8_t cmd;
	uint8_t resp : 1;
	uint8_t rsv : 3;
	uint8_t core : 4;
	uint8_t partial_order;
	uint16_t tranid;
	uint16_t length; /* size of ao ipc para */
};

/* ultrasonic msg body */
struct ps_sound_para_t {
	uint32_t sh_ctrl;
	uint32_t hifi_ctrl;
	uint32_t sound_data_0_3;
	uint32_t sound_data_4_7;
	uint32_t sound_data_8_11;
	uint32_t sound_data_12_15;
};

/* ultrasonic msg */
struct pkt_ps_sound_req_t {
	struct ao_ipc_msg_header hd;
	struct ps_sound_para_t para;
};

/* scene_denoise msg body */
struct audio_scene_denoise_ao_ipc_param {
	uint32_t batch_num;
	uint32_t size;
};

/* scene_denoise msg */
struct audio_scene_denoise_ao_ipc_pkt {
	struct ao_ipc_msg_header hd;
	struct audio_scene_denoise_ao_ipc_param para;
};

#endif
