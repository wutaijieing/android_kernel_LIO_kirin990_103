/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: ddr config
 * Create: 2021.11.16
 */

#ifndef __DDR_CONFIG_H__
#define __DDR_CONFIG_H__

#include <linux/types.h>
#include "platform_include/smart/linux/iomcu_boot.h"

#define VERSION_LENGTH	16

struct log_buff_t {
	u32 mutex;
	u16 index;
	u16 pingpang;
	u32 buff;
	u32 ddr_log_buff_cnt;
	u32 ddr_log_buff_index;
	u32 ddr_log_buff_last_update_index;
};

struct wrong_wakeup_msg_t {
	u32 flag;
	u32 reserved1;
	u64 time;
	u32 irq0;
	u32 irq1;
	u32 recvfromapmsg[4];
	u32 recvfromlpmsg[4];
	u32 sendtoapmsg[4];
	u32 sendtolpmsg[4];
	u32 recvfromapmsgmode;
	u32 recvfromlpmsgmode;
	u32 sendtoapmsgmode;
	u32 sendtolpmsgmode;
};

struct coul_core_info {
	int c_offset_a;
	int c_offset_b;
	int r_coul_mohm;
	int reserved;
};

struct bright_data {
	uint32_t mipi_data;
	uint32_t bright_data;
	uint64_t time_stamp;
};

struct read_data_als_ud {
	float rdata;
	float gdata;
	float bdata;
	float irdata;
};

struct als_ud_config {
	u8 screen_status;
	u8 reserved[7]; /* 7 is reserved nums */
	u64 als_rgb_pa;
	struct bright_data bright_data_input;
	struct read_data_als_ud read_data_history;
};

struct config_on_ddr {
	struct dump_config dump_config;
	struct log_buff_t logbuff_cb_backup;
	struct wrong_wakeup_msg_t wrong_wakeup_msg;
	u32 log_level;
	float gyro_offset[3];
	u8 modem_open_app_state;
	u8 hifi_open_sound_msg;
	u8 igs_hardware_bypass;
	u8 igs_screen_status;
	u8 reserved1[4];
	u64 reserved;
	struct timestamp_kernel timestamp_base_send;
	struct timestamp_iomcu_base timestamp_base_iomcu;
	struct coul_core_info coul_info;
	struct bbox_config bbox_config;
	struct als_ud_config als_ud_config;
	u32 te_irq_tcs3701;
	u32 old_dc_flag;
	u32 lcd_freq;
	u8 phone_type_info[2];
	u32 sc_flag;
	u32 is_pll_on;
	u32 fall_down_support;
	char sensorhub_version[VERSION_LENGTH];
};

#endif
