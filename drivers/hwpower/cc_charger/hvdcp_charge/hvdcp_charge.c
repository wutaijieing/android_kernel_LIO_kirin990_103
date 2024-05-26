// SPDX-License-Identifier: GPL-2.0
/*
 * hvdcp_charge.c
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

#include <chipset_common/hwpower/hvdcp_charge/hvdcp_charge.h>
#include <chipset_common/hwpower/adapter/adapter_detect.h>
#include <chipset_common/hwpower/charger/charger_common_interface.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <huawei_platform/power/direct_charger/direct_charger.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>

#define HWLOG_TAG hvdcp_charge
HWLOG_REGIST();

static unsigned int g_hvdcp_rt_current_thld;
static unsigned int g_hvdcp_rt_time;
static bool g_hvdcp_rt_result;
static bool g_hvdcp_charging_flag;
static unsigned int g_hvdcp_charging_stage = HVDCP_STAGE_DEFAUTL;
static unsigned int g_hvdcp_master_error_cnt;
static unsigned int g_hvdcp_vboost_retry_cnt;
static unsigned int g_hvdcp_adapter_retry_cnt;
static unsigned int g_hvdcp_adapter_enable_cnt;
static unsigned int g_hvdcp_adapter_detect_cnt;

static const char * const g_hvdcp_charging_stage_str[HVDCP_STAGE_END] = {
	[HVDCP_STAGE_DEFAUTL] = "hvdcp_stage_default",
	[HVDCP_STAGE_SUPPORT_DETECT] = "hvdcp_stage_support_detect",
	[HVDCP_STAGE_ADAPTER_DETECT] = "hvdcp_stage_adapter_detect",
	[HVDCP_STAGE_ADAPTER_ENABLE] = "hvdcp_stage_adapter_enable",
	[HVDCP_STAGE_SUCCESS] = "hvdcp_stage_success",
	[HVDCP_STAGE_CHARGE_DONE] = "hvdcp_stage_charge_done",
	[HVDCP_STAGE_RESET_ADAPTER] = "hvdcp_stage_reset_adapter",
	[HVDCP_STAGE_ERROR] = "hvdcp_stage_error",
};

unsigned int hvdcp_get_rt_current_thld(void)
{
	return g_hvdcp_rt_current_thld;
}

void hvdcp_set_rt_current_thld(unsigned int curr)
{
	g_hvdcp_rt_current_thld = curr;
	hwlog_info("set_rt_current_thld: curr=%u\n", curr);
}

unsigned int hvdcp_get_rt_time(void)
{
	return g_hvdcp_rt_time;
}

void hvdcp_set_rt_time(unsigned int time)
{
	g_hvdcp_rt_time = time;
	hwlog_info("set_rt_time: time=%u\n", time);
}

bool hvdcp_get_rt_result(void)
{
	return g_hvdcp_rt_result;
}

void hvdcp_set_rt_result(bool result)
{
	g_hvdcp_rt_result = result;
	hwlog_info("set_rt_result: result=%d\n", result);
}

bool hvdcp_get_charging_flag(void)
{
	return g_hvdcp_charging_flag;
}

void hvdcp_set_charging_flag(bool flag)
{
	g_hvdcp_charging_flag = flag;
	hwlog_info("set_charging_flag: flag=%d\n", flag);
}

unsigned int hvdcp_get_charging_stage(void)
{
	return g_hvdcp_charging_stage;
}

const char *hvdcp_get_charging_stage_string(unsigned int stage)
{
	if ((stage >= HVDCP_STAGE_BEGIN) && (stage < HVDCP_STAGE_END))
		return g_hvdcp_charging_stage_str[stage];

	return "illegal charging_stage";
}

void hvdcp_set_charging_stage(unsigned int stage)
{
	g_hvdcp_charging_stage = stage;
	hwlog_info("set_charging_stage: stage=%u\n", stage);
}

unsigned int hvdcp_get_adapter_enable_count(void)
{
	return g_hvdcp_adapter_enable_cnt;
}

void hvdcp_set_adapter_enable_count(unsigned int cnt)
{
	g_hvdcp_adapter_enable_cnt = cnt;
	hwlog_info("set_adapter_enable_count: cnt=%u\n", cnt);
}

unsigned int hvdcp_get_adapter_detect_count(void)
{
	return g_hvdcp_adapter_detect_cnt;
}

void hvdcp_set_adapter_detect_count(unsigned int cnt)
{
	g_hvdcp_adapter_detect_cnt = cnt;
	hwlog_info("set_adapter_detect_count: cnt=%u\n", cnt);
}

unsigned int hvdcp_get_adapter_retry_count(void)
{
	return g_hvdcp_adapter_retry_cnt;
}

void hvdcp_set_adapter_retry_count(unsigned int cnt)
{
	g_hvdcp_adapter_retry_cnt = cnt;
	hwlog_info("set_adapter_retry_count: cnt=%u\n", cnt);
}

unsigned int hvdcp_get_vboost_retry_count(void)
{
	return g_hvdcp_vboost_retry_cnt;
}

void hvdcp_set_vboost_retry_count(unsigned int cnt)
{
	g_hvdcp_vboost_retry_cnt = cnt;
	hwlog_info("set_vboost_retry_count: cnt=%u\n", cnt);
}

int hvdcp_set_adapter_voltage(int volt)
{
	int ret = adapter_set_output_voltage(ADAPTER_PROTOCOL_FCP, volt);

	hwlog_info("set_adapter_voltage: volt=%d ret=%d\n", volt, ret);
	return ret;
}

void hvdcp_reset_flags(void)
{
	hvdcp_set_adapter_enable_count(0);
	hvdcp_set_adapter_detect_count(0);
	hvdcp_set_adapter_retry_count(0);
	hvdcp_set_vboost_retry_count(0);
}

int hvdcp_reset_adapter(void)
{
	return adapter_soft_reset_slave(ADAPTER_PROTOCOL_FCP);
}

int hvdcp_reset_master(void)
{
	return adapter_soft_reset_master(ADAPTER_PROTOCOL_FCP);
}

int hvdcp_reset_operate(unsigned int type)
{
	int ret;
	unsigned int chg_state = charge_get_charging_state();

	/* check not power good state of charger */
	if (chg_state & CHAGRE_STATE_NOT_PG) {
		hwlog_err("charger not power good, no need reset\n");
		return -EINVAL;
	}

	hwlog_info("reset_operate: type=%u\n", type);
	switch (type) {
	case HVDCP_RESET_ADAPTER:
		ret = hvdcp_reset_adapter();
		break;
	case HVDCP_RESET_MASTER:
		ret = hvdcp_reset_master();
		/* should delay 2000ms to wait reset success */
		(void)power_msleep(DT_MSLEEP_2S, 0, NULL);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int hvdcp_decrease_adapter_voltage_to_5v(void)
{
	int i;
	int ret;
	int vbus;
	int adp_type = ADAPTER_TYPE_UNKNOWN;

	adapter_get_adp_type(ADAPTER_PROTOCOL_SCP, &adp_type);
	hwlog_info("adp_type=%d\n", adp_type);

	/* 65w adapter not support reset, set output voltage to 5v */
	if ((adp_type == ADAPTER_TYPE_20V3P25A_MAX) ||
		(adp_type == ADAPTER_TYPE_20V3P25A)) {
		ret = hvdcp_set_adapter_voltage(ADAPTER_5V * POWER_MV_PER_V);
		if (ret)
			goto reset_adapter;

		/* wait for a period of time for the voltage to decrease */
		for (i = 0; i < (DT_MSLEEP_1S / DT_MSLEEP_50MS); i++)
			(void)power_msleep(DT_MSLEEP_50MS, 0, NULL);

		vbus = charge_get_vbus();
		if ((vbus > HVDCP_RESET_VOLT_UPPER_LIMIT) ||
			(vbus < HVDCP_RESET_VOLT_LOWER_LIMIT)) {
			hwlog_info("adapter vbus=%d out of range\n", vbus);
			goto reset_adapter;
		}

		return 0;
	}

reset_adapter:
	return hvdcp_reset_adapter();
}

bool hvdcp_check_charger_type(int type)
{
	bool ret = false;

	switch (type) {
	case CHARGER_TYPE_STANDARD:
	case CHARGER_TYPE_FCP:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}

	hwlog_info("check_charger_type: type=%d ret=%d\n", type, ret);
	return ret;
}

void hvdcp_check_adapter_status(void)
{
	int status;
	unsigned int dmd_no;
	char buf[POWER_DSM_BUF_SIZE_0128] = { 0 };
	unsigned int chg_state = charge_get_charging_state();

	/* check usb is on or not, if not, can not detect the adapter status */
	if (chg_state & CHAGRE_STATE_NOT_PG) {
		hwlog_info("charger not power good, no need check adapter status\n");
		return;
	}

	status = adapter_get_slave_status(ADAPTER_PROTOCOL_FCP);
	hwlog_info("check_adapter_status: status=%d\n", status);
	switch (status) {
	case ADAPTER_OUTPUT_UVP:
		dmd_no = ERROR_ADAPTER_OVLT;
		snprintf(buf, sizeof(buf), "hvdcp adapter voltage over high\n");
		break;
	case ADAPTER_OUTPUT_OCP:
		dmd_no = ERROR_ADAPTER_OCCURRENT;
		snprintf(buf, sizeof(buf), "hvdcp adapter current over high\n");
		break;
	case ADAPTER_OUTPUT_OTP:
		dmd_no = ERROR_ADAPTER_OTEMP;
		snprintf(buf, sizeof(buf), "hvdcp adapter temp over high\n");
		break;
	default:
		return;
	}

	hwlog_info("%s\n", buf);
	power_dsm_report_dmd(POWER_DSM_FCP_CHARGE, dmd_no, buf);
}

void hvdcp_check_master_status(void)
{
	int status;
	char buf[POWER_DSM_BUF_SIZE_0128] = { 0 };
	unsigned int chg_state = charge_get_charging_state();

	/* check usb is on or not, if not, can not detect the master status */
	if (chg_state & CHAGRE_STATE_NOT_PG) {
		hwlog_info("charger not power good, no need check master status\n");
		return;
	}

	status = adapter_get_master_status(ADAPTER_PROTOCOL_FCP);
	if (status)
		g_hvdcp_master_error_cnt++;
	else
		g_hvdcp_master_error_cnt = 0;

	hwlog_info("check_master_status: status=%d error_cnt=%u\n",
		status, g_hvdcp_master_error_cnt);

	if (g_hvdcp_master_error_cnt >= HVDCP_MAX_MASTER_ERROR_CNT) {
		g_hvdcp_master_error_cnt = 0;
		snprintf(buf, sizeof(buf), "hvdcp adapter connect fail\n");
		hwlog_info("%s\n", buf);
		power_dsm_report_dmd(POWER_DSM_FCP_CHARGE, POWER_DSM_ERROR_SWITCH_ATTACH, buf);
	}
}

bool hvdcp_check_running_current(int cur)
{
	int ichg;

	if (!power_cmdline_is_factory_mode())
		return false;

	if (!hvdcp_get_charging_flag())
		return false;

	/* record current test result on running mode */
	ichg = -power_platform_get_battery_current();
	hwlog_info("check_running_current: ichg=%d, cur=%d\n", ichg, cur);
	return (ichg >= cur) ? true : false;
}

bool hvdcp_check_adapter_retry_count(void)
{
	unsigned int cnt = hvdcp_get_adapter_retry_count();

	hvdcp_set_adapter_retry_count(++cnt);
	hwlog_info("adapter_retry cnt=%u, max_cnt=%d\n", cnt, HVDCP_MAX_ADAPTER_RETRY_CNT);

	return (cnt <= HVDCP_MAX_ADAPTER_RETRY_CNT) ? true : false;
}

bool hvdcp_check_adapter_enable_count(void)
{
	char buf[POWER_DSM_BUF_SIZE_0128] = { 0 };
	unsigned int cnt = hvdcp_get_adapter_enable_count();

	if (hvdcp_get_charging_stage() == HVDCP_STAGE_ADAPTER_ENABLE)
		++cnt;
	else
		cnt = 0;

	hvdcp_set_adapter_enable_count(cnt);
	hwlog_info("adapter_enable cnt=%u, max_cnt=%d\n", cnt, HVDCP_MAX_ADAPTER_ENABLE_CNT);

	if (cnt >= HVDCP_MAX_ADAPTER_ENABLE_CNT) {
		snprintf(buf, sizeof(buf), "hvdcp enable fail, vbus=%d\n", charge_get_vbus());
		hwlog_info("%s\n", buf);
#ifdef CONFIG_DIRECT_CHARGER
		if (!direct_charge_is_failed())
			power_dsm_report_dmd(POWER_DSM_FCP_CHARGE, POWER_DSM_ERROR_FCP_OUTPUT, buf);
#else
		power_dsm_report_dmd(POWER_DSM_FCP_CHARGE, POWER_DSM_ERROR_FCP_OUTPUT, buf);
#endif
	}

	return (cnt < HVDCP_MAX_ADAPTER_ENABLE_CNT) ? true : false;
}

bool hvdcp_check_adapter_detect_count(void)
{
	char buf[POWER_DSM_BUF_SIZE_0128] = { 0 };
	unsigned int cnt = hvdcp_get_adapter_detect_count();

	if (hvdcp_get_charging_stage() == HVDCP_STAGE_ADAPTER_DETECT)
		++cnt;
	else
		cnt = 0;

	hvdcp_set_adapter_detect_count(cnt);
	hwlog_info("adapter_detect cnt=%u, max_cnt=%d\n", cnt, HVDCP_MAX_ADAPTER_DETECT_CNT);

	if (cnt >= HVDCP_MAX_ADAPTER_DETECT_CNT) {
		snprintf(buf, sizeof(buf), "hvdcp detect fail, vbus=%d\n", charge_get_vbus());
		hwlog_info("%s\n", buf);
		power_dsm_report_dmd(POWER_DSM_FCP_CHARGE, POWER_DSM_ERROR_FCP_DETECT, buf);
	}

	return (cnt < HVDCP_MAX_ADAPTER_DETECT_CNT) ? true : false;
}

#ifndef CONFIG_DIRECT_CHARGER
int hvdcp_detect_adapter(void)
{
	int ret;
	int adp_mode = ADAPTER_SUPPORT_UNDEFINED;

	ret = adapter_detect_ping_fcp_type(&adp_mode);
	hwlog_info("detect_adapter: adp_mode=%x ret=%d\n", adp_mode, ret);

	if (ret == ADAPTER_DETECT_FAIL) {
		hwlog_err("detect adapter fail\n");
		return ADAPTER_DETECT_FAIL;
	}

	if (ret == ADAPTER_DETECT_OTHER) {
		hwlog_err("detect adapter other\n");
		return ADAPTER_DETECT_OTHER;
	}

	return ADAPTER_DETECT_SUCC;
}
#else
int hvdcp_detect_adapter(void)
{
	int ret;
	int adp_mode = ADAPTER_SUPPORT_UNDEFINED;
	int dc_adp_mode = ADAPTER_SUPPORT_UNDEFINED;
	unsigned int cnt;

	ret = adapter_detect_ping_fcp_type(&adp_mode);
	hwlog_info("detect_adapter: adp_mode=%x ret=%d\n", adp_mode, ret);

	if (!direct_charge_is_failed()) {
		hvdcp_set_vboost_retry_count(0);

		if (ret == ADAPTER_DETECT_FAIL) {
			hwlog_err("detect adapter fail\n");
			return ADAPTER_DETECT_FAIL;
		}

		if (ret == ADAPTER_DETECT_OTHER) {
			hwlog_err("detect adapter other\n");
			return ADAPTER_DETECT_OTHER;
		}

		(void)adapter_get_support_mode(ADAPTER_PROTOCOL_SCP, &dc_adp_mode);
		hwlog_info("detect_adapter: dc_adp_mode=%x ret=%d\n", dc_adp_mode, ret);
		if ((dc_adp_mode & ADAPTER_SUPPORT_LVC) || (dc_adp_mode & ADAPTER_SUPPORT_SC))
			return ADAPTER_DETECT_OTHER;
	}

	cnt = hvdcp_get_vboost_retry_count();
	hwlog_info("vboost_retry cnt=%u, max_cnt=%d\n", cnt, HVDCP_MAX_VBOOST_RETRY_CNT);
	if (cnt >= HVDCP_MAX_VBOOST_RETRY_CNT)
		return ADAPTER_DETECT_OTHER;
	hvdcp_set_vboost_retry_count(++cnt);

	if (ret || (adp_mode != ADAPTER_SUPPORT_HV))
		return ADAPTER_DETECT_OTHER;
	return ADAPTER_DETECT_SUCC;
}
#endif /* CONFIG_DIRECT_CHARGER */

bool hvdcp_exit_charging(void)
{
	hwlog_info("exit_charging\n");

	/* step1: set mivr to 4.6v when exit hvdcp charging */
	(void)charge_set_mivr(HVDCP_MIVR_SETTING);

	/* step2: set adapter output voltage to 5v */
	if (hvdcp_decrease_adapter_voltage_to_5v())
		return false;

	/* step3: set charger input voltage to 5v */
	(void)charge_set_vbus_vset(ADAPTER_5V);
	return true;
}
