/*
 *
 * AP side track hook header file.
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

#ifndef __RDR_DFX_AP_HOOK_RATIO_EIGHT_CORE_H__
#define __RDR_DFX_AP_HOOK_RATIO_EIGHT_CORE_H__

#define IRQ_RECORD_RATIO {\
	{ 16, 0, 0, 0, 0, 0, 0, 0 },\
	{ 8, 8, 0, 0, 0, 0, 0, 0 },\
	{ 8, 4, 4, 0, 0, 0, 0, 0 },\
	{ 8, 4, 2, 2, 0, 0, 0, 0 },\
	{ 8, 4, 2, 1, 1, 0, 0, 0 },\
	{ 8, 4, 1, 1, 1, 1, 0, 0 },\
	{ 6, 4, 2, 1, 1, 1, 1, 0 },\
	{ 6, 4, 1, 1, 1, 1, 1, 1 }\
	}\

#define TASK_RECORD_RATIO {\
	{ 16, 0, 0, 0, 0, 0, 0, 0 },\
	{ 8, 8, 0, 0, 0, 0, 0, 0 },\
	{ 8, 4, 4, 0, 0, 0, 0, 0 },\
	{ 8, 4, 2, 2, 0, 0, 0, 0 },\
	{ 4, 4, 4, 2, 2, 0, 0, 0 },\
	{ 4, 4, 2, 2, 2, 2, 0, 0 },\
	{ 4, 4, 2, 2, 2, 1, 1, 0 },\
	{ 4, 2, 2, 2, 2, 2, 1, 1 }\
	}\

#define CPUIDLE_RECORD_RATIO {\
	{ 16, 0, 0, 0, 0, 0, 0, 0 },\
	{ 8, 8, 0, 0, 0, 0, 0, 0 },\
	{ 8, 4, 4, 0, 0, 0, 0, 0 },\
	{ 8, 4, 2, 2, 0, 0, 0, 0 },\
	{ 4, 4, 4, 2, 2, 0, 0, 0 },\
	{ 4, 4, 2, 2, 2, 2, 0, 0 },\
	{ 4, 4, 2, 2, 2, 1, 1, 0 },\
	{ 4, 3, 3, 2, 1, 1, 1, 1 }\
	}\

#define WORKER_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define MEM_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define ION_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define TIME_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define NET_TX_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define NET_RX_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define BLOCK_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define SCHED_RECORD_RATIO CPUIDLE_RECORD_RATIO
#define RCU_RECORD_RATIO CPUIDLE_RECORD_RATIO

#define BL31_RECORD_RATIO IRQ_RECORD_RATIO
#endif

