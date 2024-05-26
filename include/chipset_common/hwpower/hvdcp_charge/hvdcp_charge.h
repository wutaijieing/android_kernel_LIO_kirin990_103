/* SPDX-License-Identifier: GPL-2.0 */
/*
 * hvdcp_charge.h
 *
 * high voltage dcp charge module
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#ifndef _HVDCP_CHARGE_H_
#define _HVDCP_CHARGE_H_

#include <linux/errno.h>
#include <linux/types.h>

#define HVDCP_MAX_MASTER_ERROR_CNT   4
#define HVDCP_MAX_VBOOST_RETRY_CNT   2
#define HVDCP_MAX_ADAPTER_RETRY_CNT  3
#define HVDCP_MAX_ADAPTER_ENABLE_CNT 4
#define HVDCP_MAX_ADAPTER_DETECT_CNT 4
#define HVDCP_RESET_VOLT_LOWER_LIMIT 4500
#define HVDCP_RESET_VOLT_UPPER_LIMIT 5500

#define HVDCP_MIVR_SETTING           4600000

enum hvdcp_stage_type {
	HVDCP_STAGE_BEGIN,
	HVDCP_STAGE_DEFAUTL = HVDCP_STAGE_BEGIN,
	HVDCP_STAGE_SUPPORT_DETECT,
	HVDCP_STAGE_ADAPTER_DETECT,
	HVDCP_STAGE_ADAPTER_ENABLE,
	HVDCP_STAGE_SUCCESS,
	HVDCP_STAGE_CHARGE_DONE,
	HVDCP_STAGE_RESET_ADAPTER,
	HVDCP_STAGE_ERROR,
	HVDCP_STAGE_END,
};

enum hvdcp_reset_type {
	HVDCP_RESET_BEGIN,
	HVDCP_RESET_ADAPTER = HVDCP_RESET_BEGIN,
	HVDCP_RESET_MASTER,
	HVDCP_RESET_END,
};

#if (defined(CONFIG_HUAWEI_CHARGER_AP) || defined(CONFIG_FCP_CHARGER))
unsigned int hvdcp_get_rt_current_thld(void);
void hvdcp_set_rt_current_thld(unsigned int curr);
unsigned int hvdcp_get_rt_time(void);
void hvdcp_set_rt_time(unsigned int time);
bool hvdcp_get_rt_result(void);
void hvdcp_set_rt_result(bool result);
bool hvdcp_get_charging_flag(void);
void hvdcp_set_charging_flag(bool flag);
unsigned int hvdcp_get_charging_stage(void);
const char *hvdcp_get_charging_stage_string(unsigned int stage);
void hvdcp_set_charging_stage(unsigned int stage);
unsigned int hvdcp_get_adapter_enable_count(void);
void hvdcp_set_adapter_enable_count(unsigned int cnt);
unsigned int hvdcp_get_adapter_detect_count(void);
void hvdcp_set_adapter_detect_count(unsigned int cnt);
unsigned int hvdcp_get_adapter_retry_count(void);
void hvdcp_set_adapter_retry_count(unsigned int cnt);
unsigned int hvdcp_get_vboost_retry_count(void);
void hvdcp_set_vboost_retry_count(unsigned int cnt);
int hvdcp_set_adapter_voltage(int volt);
void hvdcp_reset_flags(void);
int hvdcp_reset_adapter(void);
int hvdcp_reset_master(void);
int hvdcp_reset_operate(unsigned int type);
int hvdcp_decrease_adapter_voltage_to_5v(void);
bool hvdcp_check_charger_type(int type);
void hvdcp_check_adapter_status(void);
void hvdcp_check_master_status(void);
bool hvdcp_check_running_current(int cur);
bool hvdcp_check_adapter_retry_count(void);
bool hvdcp_check_adapter_enable_count(void);
bool hvdcp_check_adapter_detect_count(void);
int hvdcp_detect_adapter(void);
bool hvdcp_exit_charging(void);
#else
static inline unsigned int hvdcp_get_rt_current_thld(void)
{
	return 0;
}

static inline void hvdcp_set_rt_current_thld(unsigned int curr)
{
}

static inline unsigned int hvdcp_get_rt_time(void)
{
	return 0;
}

static inline void hvdcp_set_rt_time(unsigned int time)
{
}

static inline bool hvdcp_get_rt_result(void)
{
	return false;
}

static inline void hvdcp_set_rt_result(bool result)
{
}

static inline bool hvdcp_get_charging_flag(void)
{
	return false;
}

static inline void hvdcp_set_charging_flag(bool flag)
{
}

static inline unsigned int hvdcp_get_charging_stage(void)
{
	return HVDCP_STAGE_DEFAUTL;
}

static inline const char *hvdcp_get_charging_stage_string(unsigned int stage)
{
	return "illegal stage_status";
}

static inline void hvdcp_set_charging_stage(unsigned int stage)
{
}

static inline unsigned int hvdcp_get_adapter_enable_count(void)
{
	return 0;
}

static inline void hvdcp_set_adapter_enable_count(unsigned int cnt)
{
}

static inline unsigned int hvdcp_get_adapter_detect_count(void)
{
	return 0;
}

static inline void hvdcp_set_adapter_detect_count(unsigned int cnt)
{
}

static inline unsigned int hvdcp_get_adapter_retry_count(void)
{
	return 0;
}

static inline void hvdcp_set_adapter_retry_count(unsigned int cnt)
{
}

static inline unsigned int hvdcp_get_vboost_retry_count(void)
{
	return 0;
}

static inline void hvdcp_set_vboost_retry_count(unsigned int cnt)
{
}

static inline int hvdcp_set_adapter_voltage(int volt)
{
	return -EINVAL;
}

static inline void hvdcp_reset_flags(void)
{
}

static inline int hvdcp_reset_adapter(void)
{
	return -EINVAL;
}

static inline int hvdcp_reset_master(void)
{
	return -EINVAL;
}

static inline int hvdcp_reset_operate(unsigned int type)
{
	return -EINVAL;
}

static inline int hvdcp_decrease_adapter_voltage_to_5v(void)
{
	return -EINVAL;
}

static inline bool hvdcp_check_charger_type(int type)
{
	return false;
}

static inline void hvdcp_check_adapter_status(void)
{
}

static inline void hvdcp_check_master_status(void)
{
}

static inline bool hvdcp_check_running_current(int cur)
{
	return false;
}

static inline bool hvdcp_check_adapter_retry_count(void)
{
	return false;
}

static inline bool hvdcp_check_adapter_enable_count(void)
{
	return false;
}

static inline bool hvdcp_check_adapter_detect_count(void)
{
	return false;
}

static inline int hvdcp_detect_adapter(void)
{
	return 0;
}

static inline bool hvdcp_exit_charging(void)
{
	return false;
}
#endif /* CONFIG_HUAWEI_CHARGER_AP || CONFIG_FCP_CHARGER */

#endif /* _HVDCP_CHARGE_H_ */
