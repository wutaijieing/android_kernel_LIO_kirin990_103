/*
 * monitor_ap_m3_ipc.h
 *
 * monitor ap ipc
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#ifndef __MONITOR_AP_M3_IPC_H__
#define __MONITOR_AP_M3_IPC_H__

#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <linux/types.h>

#define OK		0
#define MSG_LEN		2
#define ACK_LEN		8
#define MAX_DDR_FREQ_NUM	32
#define DDR_INFO_LEN		2U
#define STR_SIZE		64

extern u32 g_dubai_ddr_info[DDR_INFO_LEN][MAX_DDR_FREQ_NUM];
extern char g_ddr_info_name[DDR_INFO_LEN][STR_SIZE];
int monitor_ipc_send(void);
int get_ddr_freq_num(void);

#endif
