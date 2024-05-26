/*
 * lcd_kit_utils.c
 *
 * lcdkit utils function for lcd driver
 *
 * Copyright (c) 2018-2022 Huawei Technologies Co., Ltd.
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

#include "lcd_kit_utils.h"
#include <linux/time.h>
#include "lcd_kit_common.h"
#include "lcd_kit_drm_panel.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_core.h"
#include "lcd_kit_sysfs_qcom.h"
#include "sde/sde_connector.h"
#ifdef CONFIG_APP_INFO
#include <misc/app_info.h>
#endif
#ifdef LCD_FACTORY_MODE
#include "lcd_kit_factory.h"
#endif
#include <linux/thermal.h>

#define MAX_DELAY_TIME 200
#define SN_CODE_LENGTH 32
#define LCDKIT_DEFAULT_PANEL_NAME "AUO_OTM1901A 5.2' VIDEO TFT 1080 x 1920 DEFAULT"
static char panel_sn_buf[SN_CODE_LENGTH];
/* pcd add detect number */
static uint32_t error_num_pcd;
#if defined CONFIG_HUAWEI_DSM
extern struct dsm_client *lcd_dclient;
#endif
#define ALPM_SETTING_CASE_MASK 0xFFFF

#define SN_CODE_23_NUMB_SIZE 23
#define SN_CODE_2_NONE_SIZE 2

bool lcd_kit_support(void)
{
	struct device_node *lcdkit_np = NULL;
	const char *support_type = NULL;
	ssize_t ret;

	lcdkit_np = of_find_compatible_node(NULL, NULL,
		DTS_COMP_LCD_KIT_PANEL_TYPE);
	if (!lcdkit_np) {
		LCD_KIT_ERR("NOT FOUND device node!\n");
		return false;
	}
	ret = of_property_read_string(lcdkit_np, "support_lcd_type",
		&support_type);
	if (ret) {
		LCD_KIT_ERR("failed to get support_type\n");
		return false;
	}
	if (!strncmp(support_type, "LCD_KIT", strlen("LCD_KIT"))) {
		LCD_KIT_INFO("lcd_kit is support!\n");
		return true;
	}
	LCD_KIT_INFO("lcd_kit is not support!\n");
	return false;
}

static int lcd_kit_check_project_id(uint32_t panel_id)
{
	int i;

	for (i = 0; i < PROJECTID_LEN; i++) {
		if (isalnum((disp_info->project_id.id)[i]) == 0 &&
			isdigit((disp_info->project_id.id)[i]) == 0)
			return LCD_KIT_FAIL;
	}

	return LCD_KIT_OK;
}

int lcd_kit_read_project_id(uint32_t panel_id)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct qcom_panel_info *panel_info = NULL;

	panel_info = lcm_get_panel_info(panel_id);
	if (panel_info == NULL) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->mipi_rx == NULL) {
		LCD_KIT_ERR("mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!disp_info) {
		LCD_KIT_ERR("disp_info is null\n");
		return LCD_KIT_FAIL;
	}

	if (disp_info->project_id.support) {
		memset(disp_info->project_id.id, 0,
			sizeof(disp_info->project_id.id));
		ret = adapt_ops->mipi_rx(panel_info->display, disp_info->project_id.id,
			LCD_DDIC_INFO_LEN - 1, &disp_info->project_id.cmds);
		if (ret) {
			LCD_KIT_ERR("read project id fail\n");
			return LCD_KIT_FAIL;
		}
		if (lcd_kit_check_project_id(panel_id) == LCD_KIT_OK) {
			LCD_KIT_ERR("project id check fail\n");
			return LCD_KIT_FAIL;
		}

		LCD_KIT_INFO("read project id is %s\n",
			disp_info->project_id.id);
		return LCD_KIT_OK;
	} else {
		LCD_KIT_ERR("project id is not support!\n");
		return LCD_KIT_FAIL;
	}
}

static bool is_alpm_mode(struct dsi_display *disp)
{
	if ((disp->panel->power_mode == SDE_MODE_DPMS_LP1) ||
		(disp->panel->power_mode == SDE_MODE_DPMS_LP2))
		return true;
	return false;
}

static int alpm_mode_handle(uint32_t mode)
{
	int ret = LCD_KIT_FAIL;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct qcom_panel_info *pinfo = NULL;
	struct dsi_display *display = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("panel-%u: aod mode is %u\n", panel_id, mode);

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return LCD_KIT_FAIL;
	}
	display = pinfo->display;
	if (!display) {
		LCD_KIT_ERR("disp is null!\n");
		return LCD_KIT_FAIL;
	}

	switch (mode & ALPM_SETTING_CASE_MASK) {
	case ALPM_DISPLAY_OFF:
		ret = adapt_ops->mipi_tx(display, &disp_info->alpm.off_cmds);
		break;
	case ALPM_ON_HIGH_LIGHT:
		if (!is_alpm_mode(display)) {
			LCD_KIT_ERR("not in aod state, return!\n");
			break;
		}
		LCD_KIT_INFO("enter ALPM_ON_HIGH_LIGHT\n");
		ret = adapt_ops->mipi_tx(display, &disp_info->alpm.high_light_cmds);
		break;
	case ALPM_EXIT:
		ret = adapt_ops->mipi_tx(display, &disp_info->alpm.exit_cmds);
		break;
	case ALPM_ON_LOW_LIGHT:
		if (!is_alpm_mode(display)) {
			LCD_KIT_ERR("not in aod state, return!\n");
			break;
		}
		ret = adapt_ops->mipi_tx(display, &disp_info->alpm.low_light_cmds);
		break;
	case ALPM_SINGLE_CLOCK:
		ret = LCD_KIT_OK;
		break;
	case ALPM_DOUBLE_CLOCK:
		ret = LCD_KIT_OK;
		break;
	case ALPM_ON_MIDDLE_LIGHT:
		if (!is_alpm_mode(display)) {
			LCD_KIT_ERR("not in aod state, return!\n");
			break;
		}
		ret = adapt_ops->mipi_tx(display, &disp_info->alpm.middle_light_cmds);
		break;
	case ALPM_NO_LIGHT:
		if (!is_alpm_mode(display)) {
			LCD_KIT_ERR("not in aod state, return!\n");
			break;
		}
		ret = adapt_ops->mipi_tx(display, &disp_info->alpm.no_light_cmds);
		break;
	default:
		break;
	}
	return ret;
}

#define MAX_WAIT_FRAME_TIME 500

int lcd_kit_alpm_setting(uint32_t panel_id, uint32_t mode)
{
	int ret = LCD_KIT_FAIL;
	struct qcom_panel_info *pinfo = NULL;
	struct dsi_display *disp = NULL;

	LCD_KIT_INFO("+\n");
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return LCD_KIT_FAIL;
	}
	LCD_KIT_INFO("AOD mode is %u\n", mode);
	disp = pinfo->display;
	if (!disp) {
		LCD_KIT_ERR("disp is null!\n");
		return LCD_KIT_FAIL;
	}
	LCD_KIT_INFO("doze_wait_max_time is %u\n", disp_info->alpm.doze_time_delay);
	if (!is_alpm_mode(disp)) {
		reinit_completion(&aod_lp1_done);
		LCD_KIT_INFO("wait aod_lp1_done succ\n");
		if (!wait_for_completion_timeout(&aod_lp1_done,
			msecs_to_jiffies(disp_info->alpm.doze_time_delay)))
			LCD_KIT_ERR("wait aod lp1 mode time out!\n");
	}
#ifndef CONFIG_QCOM_MODULE_KO
	if ((disp->frame_number == 0) && (pinfo->bl_after_frame)) {
		reinit_completion(&first_frame_done);
		if (!wait_for_completion_timeout(&first_frame_done,
			msecs_to_jiffies(MAX_WAIT_FRAME_TIME)))
			LCD_KIT_ERR("wait first frame done time out!\n");
	}
#endif
	mutex_lock(&disp_info->drm_hw_lock);
	ret = alpm_mode_handle(mode);
	mutex_unlock(&disp_info->drm_hw_lock);
	LCD_KIT_INFO("-\n");
	return ret;
}

void lcd_kit_disp_on_check_delay(void)
{
	long delta_time_bl_to_panel_on;
	unsigned int delay_margin;
	struct timeval tv = {0};
	int max_delay_margin = MAX_DELAY_TIME;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (disp_info == NULL) {
		LCD_KIT_INFO("disp_info is NULL\n");
		return;
	}
	memset(&tv, 0, sizeof(struct timeval));
	do_gettimeofday(&tv);
	/* change s to us */
	delta_time_bl_to_panel_on = (tv.tv_sec - disp_info->quickly_sleep_out.panel_on_record_tv.tv_sec) *
		1000000 + tv.tv_usec - disp_info->quickly_sleep_out.panel_on_record_tv.tv_usec;
	/* change us to ms */
	delta_time_bl_to_panel_on /= 1000;
	if (delta_time_bl_to_panel_on >= disp_info->quickly_sleep_out.interval) {
		LCD_KIT_INFO("%lu > %d, no need delay\n",
			delta_time_bl_to_panel_on,
			disp_info->quickly_sleep_out.interval);
		disp_info->quickly_sleep_out.panel_on_tag = false;
		return;
	}
	delay_margin = disp_info->quickly_sleep_out.interval -
		delta_time_bl_to_panel_on;
	if (delay_margin > max_delay_margin) {
		LCD_KIT_INFO("something maybe error");
		disp_info->quickly_sleep_out.panel_on_tag = false;
		return;
	}
	msleep(delay_margin);
	LCD_KIT_INFO("backlight on delay %dms\n", delay_margin);
	disp_info->quickly_sleep_out.panel_on_tag = false;
}

void lcd_kit_disp_on_record_time(uint32_t panel_id)
{
	if (disp_info == NULL) {
		LCD_KIT_INFO("disp_info is NULL\n");
		return;
	}
	do_gettimeofday(&disp_info->quickly_sleep_out.panel_on_record_tv);
	LCD_KIT_INFO("display on at %lu seconds %lu mil seconds\n",
		disp_info->quickly_sleep_out.panel_on_record_tv.tv_sec,
		disp_info->quickly_sleep_out.panel_on_record_tv.tv_usec);
	disp_info->quickly_sleep_out.panel_on_tag = true;
}

static void parse_panel_version(struct device_node *np, struct qcom_panel_info *pinfo)
{
	u32 i;
	char *name[VERSION_NUM_MAX] = {NULL};

	lcd_kit_parse_arrays_data(np, "lcd-kit,panel-version-value",
		&pinfo->panel_version.value, pinfo->panel_version.value_number);
	pinfo->panel_version.version_number = pinfo->panel_version.value.cnt;
	LCD_KIT_INFO("Panel version value_num=%d version_num = %d\n",
		pinfo->panel_version.value_number, pinfo->panel_version.version_number);
	if (pinfo->panel_version.version_number == 0)
		return;
	lcd_kit_parse_string_array(np, "lcd-kit,panel-version",
		(const char **)&name[0], pinfo->panel_version.version_number);
	for (i = 0; i < pinfo->panel_version.version_number; i++) {
		strncpy(pinfo->panel_version.lcd_version_name[i],
			name[i], LCD_PANEL_VERSION_SIZE - 1);
		LCD_KIT_INFO("Panel version name[%d] = %s\n", i, name[i]);
	}
}

static void panel_version_cat(struct device_node *np, struct qcom_panel_info *pinfo,
	u32 i)
{
	memcpy(pinfo->panel_version.lcd_panel_version, " VER:", strlen(" VER:") + 1);
	strncat(pinfo->panel_version.lcd_panel_version,
		pinfo->panel_version.lcd_version_name[i],
		strlen(pinfo->panel_version.lcd_version_name[i]));
	LCD_KIT_INFO("Panel version is %s\n",
		pinfo->panel_version.lcd_panel_version);
}

static void panel_version_compare(struct device_node *np, struct qcom_panel_info *pinfo)
{
	u32 i;
	u32 j;

	for (i = 0; i < pinfo->panel_version.version_number; i++) {
		for (j = 0; j < pinfo->panel_version.value_number; j++) {
			LCD_KIT_INFO("read_value[%d]:0x%x\n", j,
				pinfo->panel_version.read_value[j]);
			LCD_KIT_INFO("expected_value[%d].buf[%d]:0x%x\n", i, j,
				pinfo->panel_version.value.arry_data[i].buf[j]);
			if (pinfo->panel_version.read_value[j] !=
				pinfo->panel_version.value.arry_data[i].buf[j])
				break;
			if (j == ((int)pinfo->panel_version.value_number - 1)) {
				panel_version_cat(np, pinfo, i);
				return;
			}
		}
	}
}

static void lcd_kit_panel_version_init(uint32_t panel_id,
	struct device_node *np, struct qcom_panel_info *pinfo)
{
	lcd_kit_parse_u32(np, "lcd-kit,panel-version-support",
		&pinfo->panel_version.support, 0);
	if (!pinfo->panel_version.support) {
		LCD_KIT_INFO("not support panel version\n");
		return;
	}
	parse_panel_version(np, pinfo);
	panel_version_compare(np, pinfo);
}

static int parse_panel0_version_cmdline(void)
{
	char *pstr = NULL;
	char *dstr = NULL;
	uint32_t panel_version_len;
	struct qcom_panel_info *pinfo = lcm_get_panel_info(PRIMARY_PANEL);
	int i = 0;

	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is NULL\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO("enter\n");
	pstr = strstr(saved_command_line, "panel0_version=");
	if (!pstr) {
		LCD_KIT_ERR("No fastboot panel0_version info\n");
		return -EINVAL;
	}
	pstr += strlen("panel0_version=");
	dstr = strstr(pstr, " ");
	if (!dstr) {
		pr_err("No find the panel0_version end\n");
		return -EINVAL;
	}
	panel_version_len = (unsigned long)(dstr - pstr);
	LCD_KIT_INFO("panel_version_len1 is %d\n", panel_version_len);
	if (panel_version_len > sizeof(pinfo->panel_version.read_value)) {
		panel_version_len = sizeof(pinfo->panel_version.read_value);
		LCD_KIT_INFO("panel_version_len2 is %d\n", panel_version_len);
	}
	for (i = 0; i < panel_version_len; i++) {
		LCD_KIT_INFO("read_value1[%d]:0x%x\n", i, pinfo->panel_version.read_value[i]);
	}
	memcpy(pinfo->panel_version.read_value, pstr, panel_version_len);
	for (i = 0; i < panel_version_len; i++) {
		LCD_KIT_INFO("read_value2[%d]:0x%x\n", i, pinfo->panel_version.read_value[i]);
	}
	pinfo->panel_version.value_number = panel_version_len;

	return LCD_KIT_OK;
}

static void lcd_kit_sn_reprocess(uint32_t panel_id, char *buff, unsigned int len)
{
	int i;
	int j = 0;

	if (!common_info->sn_code.reprocess_support) {
		LCD_KIT_ERR("no need reprocess\n");
		return;
	}

	if (!buff) {
		LCD_KIT_ERR("buff is NULL\n");
		return;
	}

	if (len < SN_CODE_23_NUMB_SIZE) {
		LCD_KIT_ERR("len is error\n");
		return;
	}

	for (i = 0; i < SN_CODE_23_NUMB_SIZE && j < SN_CODE_2_NONE_SIZE; i++) {
		LCD_KIT_INFO("sn read:%d-%d\n", i, buff[i]);
		if (buff[i] == 0) {
			buff[i] = '0';
			LCD_KIT_INFO("sn reprocess:%d-%d\n", i, buff[i]);
			j++;
		}
	}
	return;
}

void lcd_kit_pinfo_init(uint32_t panel_id,
	struct device_node *np, struct qcom_panel_info *pinfo)
{
	uint32_t left_time_to_te_us = 0;
	uint32_t right_time_to_te_us = 0;
	uint32_t te_interval_us = 0;

	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return;
	}
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-type", &pinfo->bl_set_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-min", &pinfo->bl_min, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-max", &pinfo->bl_max, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-def", &pinfo->bl_default, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-cmd-type", &pinfo->type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-lcm-type", &pinfo->panel_lcm_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ldi-dsi-mode", &pinfo->panel_dsi_mode, 0);
	/* project id */
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,project-id-support",
		&disp_info->project_id.support, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-gpio-offset", &pinfo->gpio_offset, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,left-time-to-te-us", &left_time_to_te_us, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,right-time-to-te-us", &right_time_to_te_us, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,te-interval-time-us", &te_interval_us, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mask-delay-time-before-fp",
		&pinfo->mask_delay.delay_time_before_fp, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mask-delay-time-after-fp",
		&pinfo->mask_delay.delay_time_after_fp, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,finger-unlock-state-support",
		&pinfo->finger_unlock_state_support, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,finger-handle-esd-support",
		&pinfo->finger_handle_esd_support, 0);
	pinfo->finger_unlock_state = ON_FINGER_UNLOCK;
	pinfo->mask_delay.left_time_to_te_us = left_time_to_te_us;
	pinfo->mask_delay.right_time_to_te_us = right_time_to_te_us;
	pinfo->mask_delay.te_interval_us = te_interval_us;
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,bl-after-frame", &pinfo->bl_after_frame, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,cmd-async-support", &pinfo->cmd_async_support, 0);
	pinfo->cmd_async = 0;

	if (disp_info->project_id.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,project-id-cmds",
			"lcd-kit,project-id-cmds-state",
			&disp_info->project_id.cmds);

		disp_info->project_id.default_project_id = (char*)of_get_property(np,
			"lcd-kit,default-project-id", NULL);
	}
	/* get SN code from Cmdline */
	if (common_info && common_info->sn_code.support) {
		if (strlen(panel_sn_buf) != 0) {
			memcpy(pinfo->sn_code, panel_sn_buf, sizeof(panel_sn_buf));
		}
	}

	/* oem information */
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,oem-info-support",
		&disp_info->oeminfo.support, 0);

	if (disp_info->oeminfo.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,oem-barcode-2d-support",
			&disp_info->oeminfo.barcode_2d.support, 0);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,oem-barcode-2d-num-offset",
			&disp_info->oeminfo.barcode_2d.number_offset, 0);
		if (disp_info->oeminfo.barcode_2d.support) {
			lcd_kit_parse_dcs_cmds(np, "lcd-kit,barcode-2d-cmds",
				"lcd-kit,barcode-2d-cmds-state",
				&disp_info->oeminfo.barcode_2d.cmds);
			disp_info->oeminfo.barcode_2d.flags = false;
		}
	}
/* get panel version from uefi */
	if (!parse_panel0_version_cmdline())
		lcd_kit_panel_version_init(panel_id, np, pinfo);
}

void lcd_kit_parse_effect(uint32_t panel_id, struct device_node *np)
{
	/* rgbw */
	lcd_kit_parse_u32(np, "lcd-kit,rgbw-support",
		&disp_info->rgbw.support, 0);
	if (disp_info->rgbw.support) {
		lcd_kit_parse_u32(np, "lcd-kit,rgbw-bl-max",
			&disp_info->rgbw.rgbw_bl_max, 0);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode1-cmds",
			"lcd-kit,rgbw-mode1-cmds-state",
			&disp_info->rgbw.mode1_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode2-cmds",
			"lcd-kit,rgbw-mode2-cmds-state",
			&disp_info->rgbw.mode2_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode3-cmds",
			"lcd-kit,rgbw-mode3-cmds-state",
			&disp_info->rgbw.mode3_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-mode4-cmds",
			"lcd-kit,rgbw-mode4-cmds-state",
			&disp_info->rgbw.mode4_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-backlight-cmds",
			"lcd-kit,rgbw-backlight-cmds-state",
			&disp_info->rgbw.backlight_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,rgbw-pixel-gain-limit-cmds",
			"lcd-kit,rgbw-pixel-gain-limit-cmds-state",
			&disp_info->rgbw.pixel_gain_limit_cmds);
	}
}

void lcd_kit_parse_alpm(uint32_t panel_id, struct device_node *np)
{
	lcd_kit_parse_u32(np, "lcd-kit,doze-time-delay",
		&disp_info->alpm.doze_time_delay, 500);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-enter-aod-cmds",
		"lcd-kit,panel-enter-aod-cmds-state",
		&disp_info->alpm.off_cmds);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-exit-aod-cmds",
		"lcd-kit,panel-exit-aod-cmds-state",
		&disp_info->alpm.exit_cmds);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-aod-high-brightness-cmds",
		"lcd-kit,panel-aod-high-brightness-cmds-state",
		&disp_info->alpm.high_light_cmds);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-aod-middle-brightness-cmds",
		"lcd-kit,panel-aod-middle-brightness-cmds-state",
		&disp_info->alpm.middle_light_cmds);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-aod-low-brightness-cmds",
		"lcd-kit,panel-aod-low-brightness-cmds-state",
		&disp_info->alpm.low_light_cmds);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-exit-aod-cmds",
		"lcd-kit,panel-exit-aod-cmds-state",
		&disp_info->alpm.exit_cmds);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-aod-no-brightness-cmds",
		"lcd-kit,panel-aod-no-brightness-cmds-state",
		&disp_info->alpm.no_light_cmds);
}
static void lcd_kit_parse_dt_pcd(uint32_t panel_id, struct device_node *np)
{
	/* pcd errflag */
	lcd_kit_parse_u32(np, "lcd-kit,pcd-errflag-check-support",
		&disp_info->pcd_errflag.pcd_errflag_check_support, 0);
	lcd_kit_parse_u32(np, "lcd-kit,gpio-pcd",
		&disp_info->pcd_errflag.gpio_pcd, 0);
	lcd_kit_parse_u32(np, "lcd-kit,gpio-errflag",
		&disp_info->pcd_errflag.gpio_errflag, 0);
	lcd_kit_parse_u32(np, "lcd-kit,pcd-cmds-support",
		&disp_info->pcd_errflag.pcd_support, 0);
	if (disp_info->pcd_errflag.pcd_support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,start-pcd-check-cmds",
			"lcd-kit,start-pcd-check-cmds-state",
			&disp_info->pcd_errflag.start_pcd_check_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,pcd-read-cmds",
			"lcd-kit,pcd-read-cmds-state",
			&disp_info->pcd_errflag.read_pcd_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,switch-page-cmds",
			"lcd-kit,switch-page-cmds-state",
			&disp_info->pcd_errflag.switch_page_cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,pcd-check-reg-value",
			&disp_info->pcd_errflag.pcd_value);
		lcd_kit_parse_u32(np,
			"lcd-kit,pcd-check-reg-value-compare-mode",
			&disp_info->pcd_errflag.pcd_value_compare_mode, 0);
		lcd_kit_parse_u32(np, "lcd-kit,exp-pcd-mask",
			&disp_info->pcd_errflag.exp_pcd_mask, 0);
		lcd_kit_parse_u32(np, "lcd-kit,pcd-det-num",
			&disp_info->pcd_errflag.pcd_det_num, 1);
	}
	lcd_kit_parse_u32(np, "lcd-kit,errflag-cmds-support",
		&disp_info->pcd_errflag.errflag_support, 0);
	if (disp_info->pcd_errflag.errflag_support)
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,errflag-read-cmds",
			"lcd-kit,errflag-read-cmds-state",
			&disp_info->pcd_errflag.read_errflag_cmds);
}

void lcd_kit_parse_util(uint32_t panel_id, struct device_node *np)
{
	if (np == NULL) {
		LCD_KIT_ERR("np is null\n");
		return;
	}
	/* quickly sleep out */
	lcd_kit_parse_u32(np, "lcd-kit,quickly-sleep-out-support",
		&disp_info->quickly_sleep_out.support, 0);
	if (disp_info->quickly_sleep_out.support)
		lcd_kit_parse_u32(np,
			"lcd-kit,quickly-sleep-out-interval",
			&disp_info->quickly_sleep_out.interval, 0);

	/* alpm */
	lcd_kit_parse_u32(np, "lcd-kit,ap-aod-support",
		&disp_info->alpm.support, 0);
	if (disp_info->alpm.support)
		lcd_kit_parse_alpm(panel_id, np);
	/* fps */
	lcd_kit_parse_u32(np, "lcd-kit,fps-support",
		&disp_info->fps.support, 0);
	/* default fps */
	lcd_kit_parse_u32(np, "lcd-kit,default-fps",
		&disp_info->fps.default_fps, 0);
	if (disp_info->fps.support) {
		disp_info->fps.panel_support_fps_list.cnt = SEQ_NUM;
		lcd_kit_parse_array_data(np, "lcd-kit,panel-support-fps-list",
			&disp_info->fps.panel_support_fps_list);
		disp_info->fps.fps_60_cmd = (char *)of_get_property(np,
			"lcd-kit,fps-60-cmd", NULL);
		disp_info->fps.fps_90_cmd = (char *)of_get_property(np,
			"lcd-kit,fps-90-cmd", NULL);
		disp_info->fps.fps_120_cmd = (char *)of_get_property(np,
			"lcd-kit,fps-120-cmd", NULL);
	}
	/* elvdd detect */
	lcd_kit_parse_u32(np, "lcd-kit,elvdd-detect-support",
		&disp_info->elvdd_detect.support, 0);
	if (disp_info->elvdd_detect.support) {
		lcd_kit_parse_u32(np, "lcd-kit,elvdd-detect-type",
			&disp_info->elvdd_detect.detect_type, 0);
		if (disp_info->elvdd_detect.detect_type ==
			ELVDD_MIPI_CHECK_MODE)
			lcd_kit_parse_dcs_cmds(np, "lcd-kit,elvdd-detect-cmds",
				"lcd-kit,elvdd-detect-cmds-state",
				&disp_info->elvdd_detect.cmds);
		else
			lcd_kit_parse_u32(np,
				"lcd-kit,elvdd-detect-gpio",
				&disp_info->elvdd_detect.detect_gpio, 0);
		lcd_kit_parse_u32(np,
			"lcd-kit,elvdd-detect-value",
			&disp_info->elvdd_detect.exp_value, 0);
		lcd_kit_parse_u32(np,
			"lcd-kit,elvdd-value-mask",
			&disp_info->elvdd_detect.exp_value_mask, 0);
		lcd_kit_parse_u32(np,
			"lcd-kit,elvdd-delay",
			&disp_info->elvdd_detect.delay, 0);
	}
	lcd_kit_parse_u32(np, "lcd-kit,boost-5v-enable",
		&disp_info->boost_5v_enable, 0);
}

void lcd_kit_parse_dt(uint32_t panel_id, struct device_node *np)
{
	if (!np) {
		LCD_KIT_ERR("np is null\n");
		return;
	}
	/* parse effect info */
	lcd_kit_parse_effect(panel_id, np);
	/* parse normal function */
	lcd_kit_parse_util(panel_id, np);
	/* parse pcd errflag test */
	lcd_kit_parse_dt_pcd(panel_id, np);
}

int lcd_kit_get_bl_max_nit_from_dts(uint32_t panel_id)
{
	int ret;
	struct device_node *np = NULL;

	if (common_info->blmaxnit.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC) {
		np = of_find_compatible_node(NULL, NULL,
			DTS_COMP_LCD_KIT_PANEL_TYPE);
		if (!np) {
			LCD_KIT_ERR("NOT FOUND device node %s!\n",
				DTS_COMP_LCD_KIT_PANEL_TYPE);
			ret = -1;
			return ret;
		}
		OF_PROPERTY_READ_U32_RETURN(np, "panel_ddic_max_brightness",
			&common_info->actual_bl_max_nit);
		LCD_KIT_INFO("max nit is %d\n", common_info->actual_bl_max_nit);
	}
	return LCD_KIT_OK;
}

static int lcd_kit_get_project_id(char *buff)
{
	uint32_t panel_id = lcd_get_active_panel_id();

	if (buff == NULL) {
		LCD_KIT_ERR("buff is null\n");
		return LCD_KIT_FAIL;
	}

	if (!disp_info) {
		LCD_KIT_ERR("disp_info is null\n");
		return LCD_KIT_FAIL;
	}

	/* use read project id */
	if (disp_info->project_id.support &&
		(strlen(disp_info->project_id.id) > 0)) {
		strncpy(buff, disp_info->project_id.id,
			strlen(disp_info->project_id.id));
		LCD_KIT_INFO("use read project id is %s\n",
			disp_info->project_id.id);
		return LCD_KIT_OK;
	}

	/* use default project id */
	if (disp_info->project_id.support &&
		disp_info->project_id.default_project_id) {
		strncpy(buff, disp_info->project_id.default_project_id,
			PROJECTID_LEN);
		LCD_KIT_INFO("use default project id:%s\n",
			disp_info->project_id.default_project_id);
		return LCD_KIT_OK;
	}

	LCD_KIT_ERR("not support get project id\n");
	return LCD_KIT_FAIL;
}

int lcd_kit_get_online_status(uint32_t panel_id)
{
	int status = LCD_ONLINE;

	if (!strncmp(disp_info->compatible, LCD_KIT_DEFAULT_PANEL,
		strlen(disp_info->compatible)))
		/* panel is online */
		status = LCD_OFFLINE;
	LCD_KIT_INFO("status = %d\n", status);
	return status;
}

static int lcd_get_2d_barcode(uint32_t panel_id, char *buff)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct qcom_panel_info *panel_info = NULL;

	panel_info = lcm_get_panel_info(panel_id);
	if (panel_info == NULL) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->mipi_rx == NULL) {
		LCD_KIT_ERR("mipi_rx is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!buff) {
		LCD_KIT_ERR("buff is NULL\n");
		return LCD_KIT_FAIL;
	}

	if (!disp_info) {
		LCD_KIT_ERR("disp_info is null\n");
		return LCD_KIT_FAIL;
	}

	if (!disp_info->oeminfo.support) {
		LCD_KIT_ERR("oem info is not support\n");
		return LCD_KIT_FAIL;
	}
	ret = adapt_ops->mipi_rx(panel_info->display, buff,
		LCD_KIT_SN_CODE_LENGTH, &disp_info->oeminfo.barcode_2d.cmds);
	lcd_kit_sn_reprocess(panel_id, buff, LCD_KIT_SN_CODE_LENGTH);
	LCD_KIT_INFO("do lcd_kit_sn_reprocess\n");

	if (ret) {
		LCD_KIT_ERR("get 2d barcode fail\n");
		return LCD_KIT_FAIL;
	} else {
		LCD_KIT_INFO("get 2d barcode success! %s\n", buff);
		return LCD_KIT_OK;
	}
}

void lcd_kit_recovery_display(uint32_t panel_id, uint32_t sync_mode)
{
	struct qcom_panel_info *pinfo = NULL;
	struct dsi_display *disp = NULL;
	struct sde_connector *conn = NULL;

	LCD_KIT_INFO("sync_mode %u\n", sync_mode);
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return;
	}
	LCD_KIT_INFO("recovery display\n");
	disp = pinfo->display;
	if (!disp) {
		LCD_KIT_ERR("disp is null!\n");
		return;
	}
	conn = to_sde_connector(disp->drm_conn);
#ifndef CONFIG_QCOM_MODULE_KO
	_sde_connector_report_panel_dead(conn, false);
#endif
	lcd_kit_esd_recovery_enable(disp->panel);
	if (sync_mode == LCD_SYNC_MODE) {
		reinit_completion(&lcd_panel_init_done[panel_id]);
		/* wait 2s for panel init done */
		if (!wait_for_completion_timeout(&lcd_panel_init_done[panel_id],
			msecs_to_jiffies(2000)))
			LCD_KIT_ERR("panel-%u wait panel init time out!\n", panel_id);
		LCD_KIT_INFO("panel-%u init done!\n", panel_id);
	}
	LCD_KIT_INFO("-\n");
}

void lcd_esd_enable(uint32_t panel_id, int enable)
{
	LCD_KIT_INFO("esd_enable = %d\n", enable);
}

void lcd_kit_ddic_lv_detect_dmd_report(
	u8 reg_val[DETECT_LOOPS][DETECT_NUM][VAL_NUM])
{
	int i;
	int ret;
	unsigned int len;
	char err_info[DMD_DET_ERR_LEN] = {0};

	LCD_KIT_INFO("+\n");

	if (!reg_val) {
		LCD_KIT_ERR("reg_val is NULL\n");
		return;
	}
	for (i = 0; i < DETECT_LOOPS; i++) {
		len = strlen(err_info);
		if (len >= DMD_DET_ERR_LEN) {
			LCD_KIT_ERR("strlen error\n");
			return;
		}
		ret = snprintf(err_info + len, DMD_DET_ERR_LEN - len,
			"%d: %x %x, %x %x, %x %x, %x %x ",
			i + DET_START,
			reg_val[i][DET1_INDEX][VAL_1],
			reg_val[i][DET1_INDEX][VAL_0],
			reg_val[i][DET2_INDEX][VAL_1],
			reg_val[i][DET2_INDEX][VAL_0],
			reg_val[i][DET3_INDEX][VAL_1],
			reg_val[i][DET3_INDEX][VAL_0],
			reg_val[i][DET4_INDEX][VAL_1],
			reg_val[i][DET4_INDEX][VAL_0]);
		if (ret < 0) {
			LCD_KIT_ERR("snprintf error\n");
			return;
		}
	}
#if defined(CONFIG_HUAWEI_DSM)
	if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
		dsm_client_record(lcd_dclient, err_info);
		dsm_client_notify(lcd_dclient, DSM_LCD_DDIC_LV_DETECT_ERROR_NO);
	}
#endif
}

int lcd_kit_rgbw_set_mode(uint32_t panel_id, struct dsi_display *display, int mode)
{
	int ret = LCD_KIT_OK;
	static int old_rgbw_mode;
	int rgbw_mode;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (display == NULL) {
		LCD_KIT_INFO("display is NULL\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->mipi_tx == NULL) {
		LCD_KIT_ERR("mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info == NULL) {
		LCD_KIT_ERR("disp_info is null\n");
		return LCD_KIT_FAIL;
	}
	rgbw_mode = disp_info->ddic_rgbw_param.ddic_rgbw_mode;
	if (rgbw_mode != old_rgbw_mode) {
		switch (mode) {
		case RGBW_SET1_MODE:
			ret = adapt_ops->mipi_tx(display, &disp_info->rgbw.mode1_cmds);
			break;
		case RGBW_SET2_MODE:
			ret = adapt_ops->mipi_tx(display, &disp_info->rgbw.mode2_cmds);
			break;
		case RGBW_SET3_MODE:
			ret = adapt_ops->mipi_tx(display, &disp_info->rgbw.mode3_cmds);
			break;
		case RGBW_SET4_MODE:
			ret = adapt_ops->mipi_tx(display, &disp_info->rgbw.mode4_cmds);
			break;
		default:
			LCD_KIT_ERR("mode err: %d\n", disp_info->ddic_rgbw_param.ddic_rgbw_mode);
			ret = LCD_KIT_FAIL;
			break;
		}
	}
	LCD_KIT_DEBUG("rgbw_mode=%d,rgbw_mode_old=%d!\n", rgbw_mode, old_rgbw_mode);
	old_rgbw_mode = rgbw_mode;
	return ret;
}

int lcd_kit_rgbw_set_backlight(uint32_t panel_id, struct dsi_display *display, int bl_level)
{
	int ret;
	unsigned int level;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (display == NULL) {
		LCD_KIT_INFO("display is NULL\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->mipi_tx == NULL) {
		LCD_KIT_ERR("mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (bl_level < 0)
		bl_level = 0;
	level = (unsigned int)bl_level;
	/* change bl level to dsi cmds */
	disp_info->rgbw.backlight_cmds.cmds[0].payload[1] = (level >> 8) & 0xff;
	disp_info->rgbw.backlight_cmds.cmds[0].payload[2] = level & 0xff;
	ret = adapt_ops->mipi_tx(display, &disp_info->rgbw.backlight_cmds);
	return ret;
}

static int lcd_kit_rgbw_pix_gain(uint32_t panel_id, struct dsi_display *display)
{
	unsigned int pix_gain;
	static unsigned int pix_gain_old;
	int rgbw_mode;
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	if (display == NULL) {
		LCD_KIT_INFO("display is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->rgbw.pixel_gain_limit_cmds.cmds == NULL) {
		LCD_KIT_INFO("RGBW not support pixel_gain_limit\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->mipi_tx == NULL) {
		LCD_KIT_ERR("mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	rgbw_mode = disp_info->ddic_rgbw_param.ddic_rgbw_mode;
	pix_gain = (unsigned int)disp_info->ddic_rgbw_param.pixel_gain_limit;
	if ((pix_gain != pix_gain_old) && (rgbw_mode == RGBW_SET4_MODE)) {
		disp_info->rgbw.pixel_gain_limit_cmds.cmds[0].payload[1] = pix_gain;
		ret = adapt_ops->mipi_tx(display, &disp_info->rgbw.pixel_gain_limit_cmds);
		LCD_KIT_DEBUG("RGBW pixel_gain=%u,pix_gain_old=%u\n",
			pix_gain, pix_gain_old);
		pix_gain_old = pix_gain;
	}
	return ret;
}

int lcd_kit_rgbw_set_handle(uint32_t panel_id)
{
	int ret;
	static int old_rgbw_backlight;
	int rgbw_backlight;
	int rgbw_bl_level;
	struct qcom_panel_info *panel_info = NULL;

	if (disp_info == NULL) {
		LCD_KIT_ERR("disp_info is null\n");
		return LCD_KIT_FAIL;
	}
	panel_info = lcm_get_panel_info(panel_id);
	if (panel_info == NULL) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	/* set mode */
	ret = lcd_kit_rgbw_set_mode(panel_id, panel_info->display,
		disp_info->ddic_rgbw_param.ddic_rgbw_mode);
	if (ret < 0) {
		LCD_KIT_ERR("RGBW set mode fail\n");
		return LCD_KIT_FAIL;
	}

	/* set backlight */
	rgbw_backlight = disp_info->ddic_rgbw_param.ddic_rgbw_backlight;
	if (disp_info->rgbw.backlight_cmds.cmds &&
		(panel_info->bl_current != 0) &&
		(rgbw_backlight != old_rgbw_backlight)) {
		rgbw_bl_level = rgbw_backlight * disp_info->rgbw.rgbw_bl_max /
			panel_info->bl_max;
		ret = lcd_kit_rgbw_set_backlight(panel_id, panel_info->display, rgbw_bl_level);
		if (ret < 0) {
			LCD_KIT_ERR("RGBW set backlight fail\n");
			return LCD_KIT_FAIL;
		}
	}
	old_rgbw_backlight = rgbw_backlight;

	/* set gain */
	ret = lcd_kit_rgbw_pix_gain(panel_id, panel_info->display);
	if (ret) {
		LCD_KIT_INFO("RGBW set pix_gain fail\n");
		return LCD_KIT_FAIL;
	}
	return ret;
}

int lcd_kit_get_status_by_type(int type, int *status)
{
	int ret;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (status == NULL) {
		LCD_KIT_ERR("status is null\n");
		return LCD_KIT_FAIL;
	}
	switch (type) {
	case LCD_ONLINE_TYPE:
		*status = lcd_kit_get_online_status(panel_id);
		ret = LCD_KIT_OK;
		break;
	case PT_STATION_TYPE:
#ifdef LCD_FACTORY_MODE
		*status = lcd_kit_get_pt_station_status(panel_id);
#endif
		ret = LCD_KIT_OK;
		break;
	default:
		LCD_KIT_ERR("not support type\n");
		ret = LCD_KIT_FAIL;
		break;
	}
	return ret;
}

void lcd_kit_set_bl_cmd(uint32_t panel_id, uint32_t level)
{
	if (common_info->backlight.order != BL_BIG_ENDIAN &&
		common_info->backlight.order != BL_LITTLE_ENDIAN) {
		LCD_KIT_ERR("not support order\n");
		return;
	}
	if (common_info->backlight.order == BL_BIG_ENDIAN) {
		if (common_info->backlight.bl_max <= 0xFF) {
			common_info->backlight.bl_cmd.cmds[0].payload[1] = level;
			return;
		}
		/* change bl level to dsi cmds */
		common_info->backlight.bl_cmd.cmds[0].payload[1] =
			(level >> 8) & 0xFF;
		common_info->backlight.bl_cmd.cmds[0].payload[2] = level & 0xFF;
		return;
	}
	if (common_info->backlight.bl_max <= 0xFF) {
		common_info->backlight.bl_cmd.cmds[0].payload[1] = level;
		return;
	}
	/* change bl level to dsi cmds */
	common_info->backlight.bl_cmd.cmds[0].payload[1] = level & 0xFF;
	common_info->backlight.bl_cmd.cmds[0].payload[2] = (level >> 8) & 0xFF;
}

static int set_mipi_bl_level(uint32_t panel_id, struct hbm_type_cfg hbm_source, uint32_t level)
{
	lcd_kit_set_bl_cmd(panel_id, level);

	return LCD_KIT_OK;
}

int lcd_kit_mipi_set_backlight(struct hbm_type_cfg hbm_source, uint32_t level)
{
	uint32_t panel_id = lcd_get_active_panel_id();

	if (set_mipi_bl_level(panel_id, hbm_source, level) == LCD_KIT_FAIL)
		return LCD_KIT_FAIL;
	return LCD_KIT_OK;
}

static int lcd_create_sysfs(struct kobject *obj)
{
	int rc = LCD_KIT_FAIL;

	if (obj == NULL) {
		LCD_KIT_ERR("create sysfs fail, obj is NULL\n");
		return rc;
	}

	rc = lcd_kit_create_sysfs(obj);
	if (rc) {
		LCD_KIT_ERR("create sysfs fail\n");
		return rc;
	}
#ifdef LCD_FACTORY_MODE
	rc = lcd_create_fact_sysfs(obj);
	if (rc) {
		LCD_KIT_ERR("create fact sysfs fail\n");
		return rc;
	}
#endif
	return rc;
}

static int lcd_kit_proximity_power_off(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("[Proximity_feature] lcd_kit_proximity_power_off enter!\n");
	if (!common_info->thp_proximity.support) {
		LCD_KIT_INFO("[Proximity_feature] thp_proximity not support exit!\n");
		return LCD_KIT_FAIL;
	}
	if (lcd_kit_get_pt_mode(panel_id)) {
		LCD_KIT_INFO("[Proximity_feature] pt test mode exit!\n");
		return LCD_KIT_FAIL;
	}
	down(&disp_info->thp_second_poweroff_sem);
	if (common_info->thp_proximity.panel_power_state == POWER_ON) {
		LCD_KIT_INFO("[Proximity_feature] power state is on exit!\n");
		up(&disp_info->thp_second_poweroff_sem);
		return LCD_KIT_FAIL;
	}
	if (common_info->thp_proximity.panel_power_state == POWER_TS_SUSPEND) {
		LCD_KIT_INFO("[Proximity_feature] power off suspend state exit!\n");
		up(&disp_info->thp_second_poweroff_sem);
		return LCD_KIT_OK;
	}
	if (common_info->thp_proximity.work_status == TP_PROXMITY_DISABLE) {
		LCD_KIT_INFO("[Proximity_feature] thp_proximity has been disabled exit!\n");
		up(&disp_info->thp_second_poweroff_sem);
		return LCD_KIT_FAIL;
	}
	common_info->thp_proximity.work_status = TP_PROXMITY_DISABLE;
	if (common_ops->panel_only_power_off)
		common_ops->panel_only_power_off(panel_id, NULL);
	up(&disp_info->thp_second_poweroff_sem);
	LCD_KIT_INFO("[Proximity_feature] lcd_kit_proximity_power_off exit!\n");
	return LCD_KIT_OK;
}

static uint32_t lcd_kit_get_light_dimming_type(uint32_t panel_id)
{
	uint32_t cur_brightness;
	uint32_t light_pwm_threshold = common_info->set_power.light_pwm_threshold;
	uint32_t light_dc_threshold = common_info->set_power.light_dc_threshold;

	cur_brightness = lcd_kit_get_current_brightness(panel_id);
	if (cur_brightness == 0)
		cur_brightness = common_info->bl_default_level;

	if ((light_pwm_threshold == 0) || (light_dc_threshold == 0))
		return LCD_LIGHT_IGNORE_DIMMING;

	if (cur_brightness <= light_pwm_threshold) {
		LCD_KIT_INFO("pwm dimming:brightness %u, pwm_threshold %u!\n",
			cur_brightness, light_pwm_threshold);
		return LCD_LIGHT_PWM_DIMMING;
	}

	if (cur_brightness > light_dc_threshold) {
		LCD_KIT_INFO("dc dimming:brightness %u, dc_threshold %u!\n",
			cur_brightness, light_dc_threshold);
		return LCD_LIGHT_DC_DIMMING;
	}

	return LCD_LIGHT_IGNORE_DIMMING;
}

static void lcd_kit_set_power_by_thermal_handle(uint32_t panel_id)
{
	int ret;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct thermal_zone_device *thermal_zone = NULL;
	unsigned int cur_thermal = 0;
	struct dsi_display *display = NULL;
	struct qcom_panel_info *pinfo = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return;
	}

	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return;
	}
	display = pinfo->display;
	if (!display) {
		LCD_KIT_ERR("disp is null!\n");
		return;
	}

	thermal_zone = thermal_zone_get_zone_by_name("shell_front");
	if (IS_ERR_OR_NULL(thermal_zone)) {
		LCD_KIT_ERR("Failed getting thermal zone!\n");
		return;
	}
	ret = thermal_zone_get_temp(thermal_zone, &cur_thermal);
	if (ret) {
		LCD_KIT_ERR("get_temperature fail!!");
		return;
	}

	LCD_KIT_INFO("current thermal is %u, power state %u",
		cur_thermal, common_info->set_power.state);

	if (common_info->set_power.state == LCD_THERMAL_STATE_DEFAULT) {
		if ((cur_thermal > common_info->set_power.thermal_high_threshold) &&
			(lcd_kit_get_light_dimming_type(panel_id) == LCD_LIGHT_PWM_DIMMING)) {
#ifndef CONFIG_QCOM_MODULE_KO
			(void)dsi_panel_set_high_voltage(display->panel);
#endif
			common_info->set_power.state = LCD_THERMAL_STATE_HIGH;
			LCD_KIT_INFO("thermal switch high mode\n");
		}
	}
	if (common_info->set_power.state == LCD_THERMAL_STATE_HIGH) {
		if ((cur_thermal < common_info->set_power.thermal_low_threshold) ||
			(lcd_kit_get_light_dimming_type(panel_id) == LCD_LIGHT_DC_DIMMING)) {
#ifndef CONFIG_QCOM_MODULE_KO
			(void)dsi_panel_set_low_voltage(display->panel);
#endif
			common_info->set_power.state = LCD_THERMAL_STATE_DEFAULT;
			LCD_KIT_INFO("thermal switch low mode\n");
		}
	}
}

static int lcd_kit_set_power_by_thermal(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	struct qcom_panel_info *panel_info = NULL;
	unsigned int panel_state;

	panel_info = lcm_get_panel_info(panel_id);
	if (panel_info == NULL) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!common_info->set_power.support)
		return LCD_KIT_OK;

	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_OK;
	}

	if (!panel_info->display || !panel_info->display->panel) {
		LCD_KIT_ERR("display is null!\n");
		return LCD_KIT_FAIL;
	}

	if (panel_info->display->panel->power_mode != SDE_MODE_DPMS_ON &&
		panel_info->display->panel->power_mode != SDE_MODE_DPMS_OFF) {
		LCD_KIT_INFO("not in on mode\n");
		return LCD_KIT_OK;
	}

	down(&disp_info->blank_sem);
	lcd_kit_set_power_by_thermal_handle(panel_id);
	up(&disp_info->blank_sem);

	return LCD_KIT_OK;
}

#ifndef CONFIG_QCOM_MODULE_KO
static int __init early_parse_panel_sn_cmdline(char *arg)
{
	int len;

	memset(panel_sn_buf, 0, sizeof(panel_sn_buf));
	if (arg != NULL) {
		len = strlen(arg);
		if (len > sizeof(panel_sn_buf))
			len = sizeof(panel_sn_buf);
		memcpy(panel_sn_buf, arg, len);
		return LCD_KIT_OK;
	} else {
		LCD_KIT_ERR("arg is NULL\n");
		return LCD_KIT_FAIL;
	}
}

static int __init early_parse_panel0_sn_cmdline(char *arg)
{
	uint32_t panel_sn_len;
	struct qcom_panel_info *pinfo = lcm_get_panel_info(PRIMARY_PANEL);

	if ((arg == NULL) || (pinfo == NULL)) {
		LCD_KIT_ERR("pinfo is NULL\n");
		return LCD_KIT_FAIL;
	}

	panel_sn_len = strlen(arg);
	if (panel_sn_len > sizeof(pinfo->sn_code))
		panel_sn_len = sizeof(pinfo->sn_code);
	memcpy(pinfo->sn_code, arg, panel_sn_len);
	pinfo->sn_code_length = panel_sn_len;
	LCD_KIT_INFO("panel sn len is %u\n", panel_sn_len);
	return LCD_KIT_OK;
}

static int __init early_parse_panel1_sn_cmdline(char *arg)
{
	uint32_t panel_sn_len;
	struct qcom_panel_info *pinfo = lcm_get_panel_info(SECONDARY_PANEL);

	if ((arg == NULL) || (pinfo == NULL)) {
		LCD_KIT_ERR("pinfo is NULL\n");
		return LCD_KIT_FAIL;
	}

	panel_sn_len = strlen(arg);
	if (panel_sn_len > sizeof(pinfo->sn_code))
		panel_sn_len = sizeof(pinfo->sn_code);
	memcpy(pinfo->sn_code, arg, panel_sn_len);
	pinfo->sn_code_length = panel_sn_len;
	LCD_KIT_INFO("panel sn len is %u\n", panel_sn_len);
	return LCD_KIT_OK;
}

static int __init early_parse_project_id_cmdline(char *arg)
{
	uint32_t project_id_len;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (arg == NULL) {
		LCD_KIT_ERR("input is null\n");
		return LCD_KIT_FAIL;
	}

	if (!disp_info) {
		LCD_KIT_ERR("disp_info is null\n");
		return LCD_KIT_FAIL;
	}

	project_id_len = strlen(arg);
	if (project_id_len >= PROJECTID_LEN)
		project_id_len = PROJECTID_LEN;
	memcpy(disp_info->project_id.id, arg, project_id_len);
	LCD_KIT_INFO("project id len is %u\n", project_id_len);
	return LCD_KIT_OK;
}
early_param("panel_sn", early_parse_panel_sn_cmdline);
early_param("panel0_sn", early_parse_panel0_sn_cmdline);
early_param("panel1_sn", early_parse_panel1_sn_cmdline);
early_param("projectid", early_parse_project_id_cmdline);
#endif

static int lcd_kit_get_sn_code(uint32_t panel_id)
{
	int ret;
	struct qcom_panel_info *pinfo = NULL;
	struct lcd_kit_panel_ops *panel_ops = NULL;
	char read_value[OEM_INFO_SIZE_MAX + 1] = {0};
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->mipi_rx == NULL) {
		LCD_KIT_ERR("mipi_rx is NULL\n");
		return LCD_KIT_FAIL;
	}
	pinfo = lcm_get_panel_info(panel_id);
	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	if (common_info && common_info->sn_code.support) {
		panel_ops = lcd_kit_panel_get_ops();
		if (panel_ops && panel_ops->lcd_get_2d_barcode) {
			ret = panel_ops->lcd_get_2d_barcode(panel_id, read_value);
			if (ret != 0) {
				LCD_KIT_ERR("get sn_code error!\n");
				return LCD_KIT_FAIL;
			}
			memcpy(pinfo->sn_code, read_value + 2, LCD_KIT_SN_CODE_LENGTH);
			pinfo->sn_code_length = LCD_KIT_SN_CODE_LENGTH;
			return LCD_KIT_OK;
		}
		if (disp_info && disp_info->oeminfo.barcode_2d.support) {
			if (strlen(pinfo->sn_code) != 0) {
				LCD_KIT_INFO("have got sn_code in cmdline: %s\n", pinfo->sn_code);
				pinfo->sn_code_length = SN_CODE_LENGTH;
				return LCD_KIT_OK;
			}
			common_info->close_dual_dsi_sn = 1;
			ret = adapt_ops->mipi_rx(pinfo->display, read_value,
				LCD_KIT_SN_CODE_LENGTH,
				&disp_info->oeminfo.barcode_2d.cmds);
			lcd_kit_sn_reprocess(panel_id, read_value, LCD_KIT_SN_CODE_LENGTH);
			LCD_KIT_INFO("do lcd_kit_sn_reprocess\n");
			if (ret != 0) {
				LCD_KIT_ERR("get sn_code error!\n");
				return LCD_KIT_FAIL;
			}
			memcpy(pinfo->sn_code, read_value, LCD_KIT_SN_CODE_LENGTH);
			pinfo->sn_code_length = LCD_KIT_SN_CODE_LENGTH;
			return LCD_KIT_OK;
		}
	}
	return LCD_KIT_OK;
}

int lcd_panel_sncode_store(struct dsi_display *display)
{
	int ret;
	struct dsi_panel *panel = NULL;
	uint32_t panel_id;

	if (!display || !display->panel) {
		LCD_KIT_ERR("display is null!\n");
		return LCD_KIT_FAIL;
	}

	panel = display->panel;
	panel_id = lcd_kit_get_current_panel_id(panel);
	if (disp_info->oeminfo.barcode_2d.flags == true) {
		LCD_KIT_ERR("panel_sncode got it!\n");
		return LCD_KIT_OK;
	}

	/* get sn code */
	ret = lcd_kit_get_sn_code(panel_id);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("get sn code failed!\n");
		return LCD_KIT_FAIL;
	}
	disp_info->oeminfo.barcode_2d.flags = true;
	return LCD_KIT_OK;
}

static int fold_switch_handler(uint32_t panel_id, struct qcom_panel_info *panel_info)
{
	static unsigned int time = 0;
	unsigned int fold_lp_time;
	unsigned int fold_nolp_time;
	int ret = LCD_KIT_OK;

	LCD_KIT_INFO("time = %d\n", time);
	LCD_KIT_INFO("crtc_h = %d\n", common_info->fold_info.crtc_h);
	LCD_KIT_INFO("fold_status = %d\n", common_info->fold_info.fold_status);
	if (common_info->fold_info.crtc_h != common_info->fold_info.fold_height) {
		LCD_KIT_INFO("not in fold\n");
#ifndef CONFIG_QCOM_MODULE_KO
		if (common_info->fold_info.fold_status == FOLD_STATUS) {
			LCD_KIT_INFO("switch to unfold\n");
			ret = dsi_panel_switch_to_unfold(panel_info->display->panel);
		}
#endif
		return LCD_KIT_OK;
	}
	fold_lp_time = common_info->fold_info.fold_lp_time;
	fold_nolp_time = common_info->fold_info.fold_nolp_time;
	time = time % (fold_lp_time + fold_nolp_time);
	LCD_KIT_INFO("time2 = %d\n", time);
	if (time < fold_lp_time) {
		if (common_info->fold_info.fold_status == UNFOLD_STATUS) {
#ifndef CONFIG_QCOM_MODULE_KO
			ret = dsi_panel_switch_to_fold(panel_info->display->panel);
			common_info->fold_info.fold_status = FOLD_STATUS;
#endif
			LCD_KIT_INFO("switch to fold\n");
		}
	} else {
#ifndef CONFIG_QCOM_MODULE_KO
		ret = dsi_panel_switch_to_unfold(panel_info->display->panel);
		common_info->fold_info.fold_status = UNFOLD_STATUS;
#endif
		LCD_KIT_INFO("switch to unfold\n");
	}
	time++;
	return ret;
}

static int lcd_kit_fold_switch(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	struct qcom_panel_info *panel_info = NULL;
	int ret;
	unsigned int panel_state;

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info || !panel_info->display || !panel_info->display->panel) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!common_info->check_thread.fold_support) {
		LCD_KIT_INFO("not support fold thread\n");
		return LCD_KIT_OK;
	}
	if (panel_info->display->panel->power_mode != SDE_MODE_DPMS_ON &&
		panel_info->display->panel->power_mode != SDE_MODE_DPMS_OFF) {
		LCD_KIT_INFO("not in on mode\n");
		return LCD_KIT_OK;
	}
	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_INFO("panel_state is power off\n");
		return LCD_KIT_OK;
	}
	ret = fold_switch_handler(panel_id, panel_info);
	return ret;
}

static int lcd_kit_ddic_cmd_vint2_on(uint32_t panel_id, int async)
{
	struct qcom_panel_info *pinfo = NULL;
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return 0;
	}

	if (!common_info->process_ddic_vint2_support) {
		LCD_KIT_INFO("not support ddic vint2\n");
		return 0;
	}

	LCD_KIT_INFO("async:%d\n", async);

	if (async == LCD_KIT_DDIC_CMD_ASYNC) {
		queue_work(pinfo->cmd_workqueue, &pinfo->vint2_on_cmd_work);
	} else {
		(void)lcd_kit_panel_set_ddic_cmd(pinfo->display->panel, DSI_CMD_SET_DDIC_VINT2_ON);
	}
	return 0;
}

static int lcd_kit_ddic_cmd_vint2_off(uint32_t panel_id, int async)
{
	struct qcom_panel_info *pinfo = NULL;
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return 0;
	}

	if (!common_info->process_ddic_vint2_support) {
		LCD_KIT_INFO("not support ddic vint2\n");
		return 0;
	}

	LCD_KIT_INFO("async:%d\n", async);

	if (async == LCD_KIT_DDIC_CMD_ASYNC) {
		queue_work(pinfo->cmd_workqueue, &pinfo->vint2_off_cmd_work);
	} else {
		(void)lcd_kit_panel_set_ddic_cmd(pinfo->display->panel, DSI_CMD_SET_DDIC_VINT2_OFF);
	}
	return 0;
}

struct lcd_kit_ddic_cmd_group {
	enum dsi_cmd_set_type cmd_type;
	int (*cmd_func)(uint32_t panel_id, int async);
};

struct lcd_kit_ddic_cmd_group g_lcd_kit_ddic_cmd_table[] = {
	{ DSI_CMD_SET_DDIC_VINT2_ON,
		lcd_kit_ddic_cmd_vint2_on },
	{ DSI_CMD_SET_DDIC_VINT2_OFF,
		lcd_kit_ddic_cmd_vint2_off },
};

static void lcd_kit_ddic_cmd_vint2_on_cmd_work(struct work_struct *work)
{
	struct qcom_panel_info *panel_info = container_of(work,
		struct qcom_panel_info, vint2_on_cmd_work);
	(void)lcd_kit_panel_set_ddic_cmd(panel_info->display->panel, DSI_CMD_SET_DDIC_VINT2_ON);
}

static void lcd_kit_ddic_cmd_vint2_off_cmd_work(struct work_struct *work)
{
	struct qcom_panel_info *panel_info = container_of(work,
		struct qcom_panel_info, vint2_off_cmd_work);
	(void)lcd_kit_panel_set_ddic_cmd(panel_info->display->panel, DSI_CMD_SET_DDIC_VINT2_OFF);
}

static int lcd_kit_ddic_cmd(uint32_t panel_id, int type, int async)
{
	int i;

	LCD_KIT_INFO("+\n");

	for (i = 0; i < ARRAY_SIZE(g_lcd_kit_ddic_cmd_table); ++i)
		if (type == g_lcd_kit_ddic_cmd_table[i].cmd_type)
			return g_lcd_kit_ddic_cmd_table[i].cmd_func(panel_id, async);
	LCD_KIT_ERR("type unknown, type = %d\n", type);

	return -EINVAL;
}

struct lcd_kit_ops g_lcd_ops = {
	.lcd_kit_support = lcd_kit_support,
	.get_project_id = lcd_kit_get_project_id,
	.create_sysfs = lcd_create_sysfs,
	.read_project_id = lcd_kit_read_project_id,
	.get_2d_barcode = lcd_get_2d_barcode,
	.get_status_by_type = lcd_kit_get_status_by_type,
	.proximity_power_off = lcd_kit_proximity_power_off,
#ifdef LCD_FACTORY_MODE
	.get_pt_station_status = lcd_kit_get_pt_station_status,
#endif
	.set_power_by_thermal = lcd_kit_set_power_by_thermal,
	.get_sn_code = lcd_kit_get_sn_code,
	.fold_switch = lcd_kit_fold_switch,
	.ddic_cmd = lcd_kit_ddic_cmd,
};

#ifdef CONFIG_APP_INFO
static void lcd_kit_app_info_set(uint32_t panel_id)
{
	int ret;
	static const char *panel_model = NULL;
	static const char *info_node = "lcd type";

	panel_model = common_info->panel_name;
	if (panel_model == NULL) {
		LCD_KIT_ERR("panel_name is NULL\n");
		panel_model = "default lcd";
	} else {
		if (!strcmp(panel_model, LCDKIT_DEFAULT_PANEL_NAME))
			panel_model = "default lcd";
	}
	LCD_KIT_INFO("Panel Name = %s\n", panel_model);
	ret = app_info_set(info_node, panel_model);
	if (ret)
		LCD_KIT_ERR("app_info_set:panel dt parse failed\n");
}
#endif

static int lcd_kit_gpio_pcd_errflag_read(const int gpio_no, int *read_value)
{
	if (!gpio_no) {
		/* only pcd check or only errflag check */
		LCD_KIT_INFO("gpio_no is 0\n");
		return LCD_KIT_OK;
	}

	if (gpio_request(gpio_no, GPIO_PCD_ERRFLAG_NAME)) {
		LCD_KIT_ERR("pcd_errflag_gpio[%d] request fail!\n", gpio_no);
		return LCD_KIT_FAIL;
	}
	if (gpio_direction_input(gpio_no)) {
		gpio_free(gpio_no);
		LCD_KIT_ERR("pcd_errflag_gpio[%d] direction set fail!\n",
			gpio_no);
		return LCD_KIT_FAIL;
	}
	*read_value = gpio_get_value(gpio_no);
	gpio_free(gpio_no);
	return LCD_KIT_OK;
}

int lcd_kit_gpio_pcd_errflag_check(uint32_t panel_id)
{
	int pcd_gpio = disp_info->pcd_errflag.gpio_pcd;
	int errflag_gpio = disp_info->pcd_errflag.gpio_errflag;
	int pcd_gpio_value = 0;
	int errflag_gpio_value = 0;

	if (!disp_info->pcd_errflag.pcd_errflag_check_support) {
		LCD_KIT_INFO("no support pcd_errflag check, default pass\n");
		return PCD_ERRFLAG_SUCCESS;
	}

	if (!pcd_gpio && !errflag_gpio) {
		LCD_KIT_INFO("pcd_errflag gpio is 0, default pass\n");
		return PCD_ERRFLAG_SUCCESS;
	}

	if (lcd_kit_gpio_pcd_errflag_read(pcd_gpio, &pcd_gpio_value))
		return PCD_ERRFLAG_SUCCESS;

	if (lcd_kit_gpio_pcd_errflag_read(errflag_gpio,
		&errflag_gpio_value))
		return PCD_ERRFLAG_SUCCESS;

	LCD_KIT_INFO("pcd_gpio[%d]=%d, errflag_gpio[%d]=%d\n",
		pcd_gpio, pcd_gpio_value,
		errflag_gpio, errflag_gpio_value);
	if ((pcd_gpio_value == GPIO_HIGH_PCDERRFLAG) &&
		(errflag_gpio_value == GPIO_LOW_PCDERRFLAG))
		return PCD_FAIL;
	else if ((pcd_gpio_value == GPIO_LOW_PCDERRFLAG) &&
		(errflag_gpio_value == GPIO_HIGH_PCDERRFLAG))
		return ERRFLAG_FAIL;
	else if ((pcd_gpio_value == GPIO_HIGH_PCDERRFLAG) &&
		(errflag_gpio_value == GPIO_HIGH_PCDERRFLAG))
		return PCD_ERRFLAG_FAIL;

	return PCD_ERRFLAG_SUCCESS;
}

static void lcd_dmd_report_err(uint32_t err_no, const char *info, int info_len)
{
	if (!info) {
		LCD_KIT_ERR("info is NULL Pointer\n");
		return;
	}
#if defined(CONFIG_HUAWEI_DSM)
	if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
		dsm_client_record(lcd_dclient, info);
		dsm_client_notify(lcd_dclient, err_no);
	}
#endif
}

int lcd_kit_start_pcd_check(struct dsi_display *display)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct dsi_panel *panel = NULL;
	uint32_t panel_id;

	if (!display || !display->panel) {
		LCD_KIT_ERR("display is null!\n");
		return LCD_KIT_FAIL;
	}

	panel = display->panel;
	panel_id = lcd_kit_get_current_panel_id(panel);
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (!adapt_ops->mipi_tx) {
		LCD_KIT_ERR("mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->pcd_errflag.pcd_support)
		ret = adapt_ops->mipi_tx(display,
			&disp_info->pcd_errflag.start_pcd_check_cmds);
	return ret;
}

static int lcd_kit_judge_pcd_dmd(uint32_t panel_id, uint8_t *read_val,
	uint32_t *expect_val, int cnt, uint32_t compare_mode)
{
	int i;
	uint32_t exp_pcd_value_mask;
	exp_pcd_value_mask = disp_info->pcd_errflag.exp_pcd_mask;
	if (read_val == NULL || expect_val == NULL) {
		LCD_KIT_ERR("read_val or expect_val is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (compare_mode == PCD_COMPARE_MODE_EQUAL) {
		for (i = 0; i < cnt; i++) {
			if ((uint32_t)read_val[i] != expect_val[i])
				return LCD_KIT_FAIL;
		}
	} else if (compare_mode == PCD_COMPARE_MODE_BIGGER) {
		if ((uint32_t)read_val[0] < expect_val[0])
			return LCD_KIT_FAIL;
	} else if (compare_mode == PCD_COMPARE_MODE_MASK) {
		if (((uint32_t)read_val[0] & exp_pcd_value_mask) == \
			expect_val[0])
			return LCD_KIT_FAIL;
	} else if (compare_mode == PCD_COMPARE_MODE_MULTI) {
		if (((uint32_t)read_val[0] >= expect_val[0]) && \
			((uint32_t)read_val[0] <= expect_val[1])) /* array index */
			return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

#define PCD_READ_LEN 3
static void lcd_kit_pcd_dmd_report(uint8_t *pcd_read_val, uint32_t val_len)
{
	int ret;
	char err_info[DMD_ERR_INFO_LEN] = {0};

	if (val_len < PCD_READ_LEN) {
		LCD_KIT_ERR("val len err\n");
		return;
	}
	if (!pcd_read_val) {
		LCD_KIT_ERR("pcd_read_val is NULL\n");
		return;
	}
	ret = snprintf(err_info, DMD_ERR_INFO_LEN,
		"PCD REG Value is 0x%x 0x%x 0x%x\n",
		pcd_read_val[0], pcd_read_val[1], pcd_read_val[2]);
	if (ret < 0) {
		LCD_KIT_ERR("snprintf error\n");
		return;
	}
	lcd_dmd_report_err(DSM_LCD_PANEL_CRACK_ERROR_NO, err_info,
		 DMD_ERR_INFO_LEN);
}

void pcd_test(uint32_t panel_id, uint8_t *result, uint8_t *pcd_read_val)
{
	error_num_pcd++;
	LCD_KIT_INFO("enter pcd_test, pcd_num = %d\n", error_num_pcd);
	if (error_num_pcd >= disp_info->pcd_errflag.pcd_det_num) {
		LCD_KIT_INFO("pcd detect num = %d\n", error_num_pcd);
		lcd_kit_pcd_dmd_report(pcd_read_val, LCD_KIT_PCD_SIZE);
		*result |= PCD_FAIL;
		error_num_pcd = 0;
	}
}

int lcd_kit_pcd_compare_result(uint32_t panel_id, uint8_t *read_val, int ret)
{
	uint32_t *expect_value = NULL;
	uint32_t expect_value_cnt;
	uint8_t result = PCD_ERRFLAG_SUCCESS;

	expect_value = disp_info->pcd_errflag.pcd_value.buf;
	expect_value_cnt = disp_info->pcd_errflag.pcd_value.cnt;
	if (ret == LCD_KIT_OK) {
		if (lcd_kit_judge_pcd_dmd(panel_id, read_val, expect_value,
			expect_value_cnt,
			disp_info->pcd_errflag.pcd_value_compare_mode) == \
			LCD_KIT_OK) {
			lcd_kit_pcd_dmd_report(read_val, LCD_KIT_PCD_SIZE);
			result |= PCD_FAIL;
		}
	} else {
		LCD_KIT_ERR("read pcd err\n");
	}
	LCD_KIT_INFO("pcd REG read result is 0x%x 0x%x 0x%x\n",
		read_val[0], read_val[1], read_val[2]);
	LCD_KIT_INFO("pcd check result is %d\n", result);
	return (int)result;
}

int lcd_kit_check_pcd_errflag_check(struct dsi_display *display)
{
	uint8_t result = PCD_ERRFLAG_SUCCESS;
	int ret;
	uint8_t read_pcd[LCD_KIT_PCD_SIZE] = {0};
	uint8_t read_errflag[LCD_KIT_ERRFLAG_SIZE] = {0};
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	int i;
	struct dsi_panel *panel = NULL;
	uint32_t panel_id;

	if (!display || !display->panel) {
		LCD_KIT_ERR("display is null!\n");
		return LCD_KIT_FAIL;
	}

	panel = display->panel;
	panel_id= lcd_kit_get_current_panel_id(panel);

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (!adapt_ops->mipi_rx || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("mipi_rx or mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}

	if (disp_info->pcd_errflag.pcd_support) {
		ret = adapt_ops->mipi_tx(display,
			&disp_info->pcd_errflag.switch_page_cmds);
		if (ret)
			LCD_KIT_ERR("mipi_tx fail\n");
		/* init code */
		ret = adapt_ops->mipi_rx(display, read_pcd,
			LCD_KIT_PCD_SIZE - 1,
			&disp_info->pcd_errflag.read_pcd_cmds);
		if (ret) {
			LCD_KIT_INFO("mipi_rx fail\n");
			return ret;
		}
		result = lcd_kit_pcd_compare_result(panel_id, read_pcd, ret);
	}
	/* Reserve interface, redevelop when needed */
	if (disp_info->pcd_errflag.errflag_support) {
		ret = adapt_ops->mipi_rx(display, read_errflag,
			LCD_KIT_ERRFLAG_SIZE - 1,
			&disp_info->pcd_errflag.read_errflag_cmds);
		if (ret) {
			LCD_KIT_INFO("mipi_rx fail\n");
			return ret;
		}
		for (i = 0; i < LCD_KIT_ERRFLAG_SIZE; i++) {
			if (read_errflag[i] != 0) {
				result |= ERRFLAG_FAIL;
				break;
			}
		}
	}
	return (int)result;
}

int lcd_kit_check_pcd_errflag_check_fac(uint32_t panel_id)
{
	uint8_t result = PCD_ERRFLAG_SUCCESS;
	int ret;
	uint8_t read_pcd[LCD_KIT_PCD_SIZE] = {0};
	uint8_t read_errflag[LCD_KIT_ERRFLAG_SIZE] = {0};
	uint32_t *expect_value = NULL;
	uint32_t expect_value_cnt;
	struct qcom_panel_info *panel_info = NULL;
	int i;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not get adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (!adapt_ops->mipi_rx || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("mipi_rx or mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}

	if (disp_info->pcd_errflag.pcd_support) {
		ret = adapt_ops->mipi_tx(panel_info->display,
			&disp_info->pcd_errflag.switch_page_cmds);
		if (ret)
			LCD_KIT_ERR("mipi_tx fail\n");
		ret = adapt_ops->mipi_rx(panel_info->display, read_pcd,
			LCD_KIT_PCD_SIZE - 1,
			&disp_info->pcd_errflag.read_pcd_cmds);
		expect_value = disp_info->pcd_errflag.pcd_value.buf;
		expect_value_cnt = disp_info->pcd_errflag.pcd_value.cnt;
		if (ret == LCD_KIT_OK) {
			if (lcd_kit_judge_pcd_dmd(panel_id, read_pcd, expect_value,
				expect_value_cnt,
				disp_info->pcd_errflag.pcd_value_compare_mode) \
				== LCD_KIT_OK) {
				pcd_test(panel_id, &result, read_pcd);
			} else {
				LCD_KIT_INFO("detect num=%d\n", error_num_pcd);
				error_num_pcd = 0;
			}
		} else {
			LCD_KIT_ERR("read pcd err\n");
		}
		LCD_KIT_INFO("pcd REG read result is 0x%x 0x%x 0x%x\n",
			read_pcd[0], read_pcd[1], read_pcd[2]);
		LCD_KIT_INFO("pcd check result is %d\n", result);
	}
	/* Reserve interface, redevelop when needed */
	if (disp_info->pcd_errflag.errflag_support) {
		adapt_ops->mipi_rx(panel_info->display, read_errflag,
			LCD_KIT_ERRFLAG_SIZE - 1,
			&disp_info->pcd_errflag.read_errflag_cmds);
		for (i = 0; i < LCD_KIT_ERRFLAG_SIZE; i++) {
			if (read_errflag[i] != 0) {
				result |= ERRFLAG_FAIL;
				break;
			}
		}
	}
	return (int)result;
}

int lcd_kit_get_gpio_value(uint32_t gpio_num, const char *gpio_name)
{
	int gpio_value;
	int ret;

	ret = gpio_request(gpio_num, gpio_name);
	if (ret != 0) {
		LCD_KIT_ERR("gpio-%u request fail!\n", gpio_num);
		return LCD_KIT_FAIL;
	}
	ret = gpio_direction_input(gpio_num);
	if (ret != 0) {
		gpio_free(gpio_num);
		LCD_KIT_ERR("gpio-%u direction set fail!\n", gpio_num);
		return LCD_KIT_FAIL;
	}
	gpio_value = gpio_get_value(gpio_num);
	gpio_free(gpio_num);

	LCD_KIT_INFO("gpio-%u value:%d\n", gpio_num, gpio_value);
	return gpio_value;
}

/* workqueue init */
static int lcd_kit_cmd_workqueue_init(uint32_t panel_id)
{
	struct qcom_panel_info *panel_info = lcm_get_panel_info(panel_id);

	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL!\n");
		return LCD_KIT_FAIL;
	}

	panel_info->cmd_workqueue =
		create_singlethread_workqueue("cmd");
	if (!panel_info->cmd_workqueue) {
		LCD_KIT_ERR("workqueue is NULL!\n");
		return LCD_KIT_FAIL;
	}

	INIT_WORK(&panel_info->vint2_on_cmd_work,
		lcd_kit_ddic_cmd_vint2_on_cmd_work);
	INIT_WORK(&panel_info->vint2_off_cmd_work,
		lcd_kit_ddic_cmd_vint2_off_cmd_work);
	LCD_KIT_INFO("workqueue inited\n");
	return LCD_KIT_OK;
}

int lcd_kit_utils_init(uint32_t panel_id,
	struct device_node *np, struct qcom_panel_info *pinfo)
{
	/* init sem */
	sema_init(&disp_info->blank_sem, 1);
	sema_init(&disp_info->lcd_kit_sem, 1);
	sema_init(&disp_info->thp_second_poweroff_sem, 1);
	/* init mipi lock */
	mutex_init(&disp_info->mipi_lock);
	/* init power mode lock */
	mutex_init(&disp_info->power_mode_lock);
	/* parse display dts */
	lcd_kit_parse_dt(panel_id, np);
#ifdef LCD_FACTORY_MODE
	lcd_kit_fact_init(panel_id, np);
#endif
	/* init qcom pinfo */
	lcd_kit_pinfo_init(panel_id, np, pinfo);
	lcd_kit_ops_register(&g_lcd_ops);
#ifdef CONFIG_APP_INFO
	lcd_kit_app_info_set(panel_id);
#endif
	lcd_kit_cmd_workqueue_init(panel_id);
	return LCD_KIT_OK;
}
