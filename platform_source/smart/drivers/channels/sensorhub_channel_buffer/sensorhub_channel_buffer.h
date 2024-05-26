/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub sensorhub channel buffer header file.
 * Create: 2021-09-30
 */

/* INCLUDE FILES */

/* header file */
#ifndef __SENSORHUB_CHANNEL_BUFFER_H__
#define __SENSORHUB_CHANNEL_BUFFER_H__

#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/kfifo.h>
enum sensorhub_channel {
	CHANNEL_SENSOR_MGR = 0,
	CHANNEL_FLP,
	CHANNEL_BT,
	CHANNEL_SENSORHUB_MAX,
};

/**
 * @brief sensorhub msg buf descriptor. stored in sensorhub_channel_buf_node.read_kfifo
 *
 */
struct amf20_sensorhub_msg {
	u32 service_id;
	u32 command_id;
	u32 data_len;
	u8 buffer[0];
};

/**
 * @brief store the msg into buffer sent from sensorhub。the data format is like
 * sturct amf20_sensorhub_msg | payload. which buffer pointer to the payload
 *
 * @param channel  service_id
 * @param data  data pointer
 * @param data_len  data lenght
 * @return int  0: success, other: error code
 */
int store_msg_from_sensorhub(u32 service_id,
							 u32 command_id,
							 u8 *data,
							 u32 data_len);
/**
 * @brief read the msg of channel from buffer, and store it into user buffer
 *
 * @param channel channel number
 * @param user_buf user_buffer pointer
 * @param buf_len user_buffer length
 * @return int 0: success, other: error code
 */
int read_msg_from_sensorhub(u32 service_id,
							u32 *command_id,
							u8 *user_buf,
							u32 buf_len);

/**
 * @brief sensorhub channel kfifo buffer initialize
 *
 * @param channel channel number
 * @param channle_buf_len fifo buffer len
 * @return int 0: success, other: error code
 */
int sensorhub_channel_buf_init(u32 service_id, u32 channle_buf_len);

/**
 * @brief release channel buffer
 *
 * @param channel channel number
 * @return int 0: success, other: error code
 */
int sensorhub_channel_buf_release(u32 channel);

/**
 * @brief Get the avtive channel
 *
 * @param channel_array active service id save here
 * @param array_size channel_array size
 * @return int actual active channel num
 */
int get_active_channel(u32 *channel_array, u32 array_size);

#endif