/*
 * used for soundwire bandwidth self-adaptive arrangement
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SDW_BANDWIDTH_ADP_H__
#define __SDW_BANDWIDTH_ADP_H__

#include <linux/types.h>

#define DP_NUM 15

#define LANE0 0
#define LANE1 1

#define PORT_FLOW_MODE_NORMAL 0
#define PORT_FLOW_MODE_PUSH 1
#define PORT_FLOW_MODE_PULL 2

#define BLOCK_PACKING_MODE_PER_PORT 0
#define BLOCK_PACKING_MODE_PER_CHANNEL 1

struct soundwire_priv;

struct soundwire_payload {
	unsigned char dp_id;
	unsigned char channel_num;
	unsigned char channel_mask;
	unsigned char bits_per_sample;
	unsigned int sample_rate;
	unsigned int priority;

	// cannot be divided with no remainder by soundwire clock, 44.1K/88.2K/...
	bool special_sample_rate;
};

struct soundwire_dp {
	bool enable;
	unsigned char word_length;
	unsigned char channel_num;
	unsigned char channel_mask;
	unsigned char block_packing_mode;
	unsigned char port_flow_mode;
	unsigned char h_ctrl;
	unsigned char lane_ctrl;
	unsigned short sample_interval;
	int block_offset;
	int sub_block_offset;
};

struct soundwire_reg {
	unsigned char clk_div;
	struct soundwire_dp dp[DP_NUM];
};

int soundwire_bandwidth_adp_init(struct soundwire_priv *pdata);

int soundwire_bandwidth_adp_add(
	struct soundwire_payload *payloads, int payloads_num);

int soundwire_bandwidth_adp_del(
	struct soundwire_payload *payloads, int payloads_num);

void soundwire_bandwidth_adp_deinit(void);

bool soundwire_bandwidth_adp_is_active(void);

void soundwire_bandwidth_recover(void);

#endif /* __SOUNDWIRE_BANDWIDTH_ADP_H__ */
