/*
 * lcd_kit_sysfs_qcom.c
 *
 * lcdkit sysfs function for lcd driver qcom platform
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include "lcd_kit_common.h"
#include "lcd_kit_drm_panel.h"
#include "lcd_kit_sysfs.h"
#include "lcd_kit_sysfs_qcom.h"
#include <linux/kernel.h>
#include "lcd_kit_adapt.h"
#ifdef LCD_FACTORY_MODE
#include "lcd_kit_factory.h"
#endif
#include "lcd_kit_utils.h"
/* marco define */
#ifndef strict_strtoul
#define strict_strtoul kstrtoul
#endif
#ifndef strict_strtol
#define strict_strtol kstrtol
#endif
#define PANEL_MAX 10
#define NIT_LENGTH 3

static int g_alpm_setting_ret = LCD_KIT_FAIL;

/* oem info */
static int oem_info_type = INVALID_TYPE;
static unsigned long g_display_nit = 0;
static unsigned int fold_panel_id;
static int lcd_get_2d_barcode(uint32_t panel_id, char *oem_data);
static int lcd_get_project_id(uint32_t panel_id, char *oem_data);
static struct oem_info_cmd oem_read_cmds[] = {
	{ PROJECT_ID_TYPE, lcd_get_project_id },
	{ BARCODE_2D_TYPE, lcd_get_2d_barcode }
};
extern unsigned int lcm_get_panel_state(uint32_t panel_id);
static ssize_t lcd_model_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (common_ops->get_panel_name)
		ret = common_ops->get_panel_name(panel_id, buf);
	return ret;
}

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	if (!buf) {
		LCD_KIT_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", is_mipi_cmd_panel(panel_id) ? 1 : 0);
}

#ifndef CONFIG_QCOM_MODULE_KO
static int __init early_parse_nit_cmdline(char *arg)
{
	int len;

	if (arg == NULL) {
		LCD_KIT_ERR("nit is null\n");
		return -EINVAL;
	}

	len = strlen(arg);
	if (len <= NIT_LENGTH)
		strict_strtoul(arg, 0, &g_display_nit);

	return LCD_KIT_OK;
}

early_param("display_nit", early_parse_nit_cmdline);
#endif

static ssize_t lcd_panel_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	char panel_type[PANEL_MAX] = {0};
	struct lcd_kit_bl_ops *bl_ops = NULL;
	char *bl_type = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (common_info->panel_type == LCD_TYPE)
		strncpy(panel_type, "LCD", strlen("LCD"));
	else if (common_info->panel_type == AMOLED_TYPE)
		strncpy(panel_type, "AMOLED", strlen("AMOLED"));
	else
		strncpy(panel_type, "INVALID", strlen("INVALID"));

	bl_ops = lcd_kit_get_bl_ops();
	if ((bl_ops != NULL) && (bl_ops->name != NULL))
		bl_type = bl_ops->name;

	ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,blmax_nit_actual:%d,blmax_nit_standard:%d,lcdtype:%s,bl_type:%s\n",
		common_info->bl_level_max, common_info->bl_level_min,
		g_display_nit, common_info->bl_max_nit,
		panel_type, bl_type);
	return ret;
}

static ssize_t lcd_fps_scence_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	char str[LCD_REG_LENGTH_MAX] = {0};
	char tmp[MAX_BUF] = {0};
	int i;
	int fps_rate;
	struct qcom_panel_info *pinfo = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (disp_info->fps.support) {
		ret = snprintf(str, sizeof(str), "current_fps:%d;default_fps:%d",
			disp_info->fps.current_fps, disp_info->fps.default_fps);
		strncat(str, ";support_fps_list:", strlen(";support_fps_list:"));
		for (i = 0; i < disp_info->fps.panel_support_fps_list.cnt; i++) {
			fps_rate = disp_info->fps.panel_support_fps_list.buf[i];
			if (i > 0)
				strncat(str, ",", strlen(","));
			ret += snprintf(tmp, sizeof(tmp), "%d", fps_rate);
			strncat(str, tmp, strlen(tmp));
		}
	} else {
		pinfo = lcm_get_panel_info(panel_id);

		ret = snprintf(str, sizeof(str), "lcd_fps=%d",
			disp_info->fps.default_fps);
	}
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str);
	LCD_KIT_INFO("%s\n", str);
	return ret;
}

static int lcd_fps_get_scence(int fps, int *scence)
{
	switch (fps) {
	case LCD_FPS_30:
		*scence = LCD_FPS_SCENCE_30;
		break;
	case LCD_FPS_45:
		*scence = LCD_FPS_SCENCE_45;
		break;
	case LCD_FPS_60:
		*scence = LCD_FPS_SCENCE_60;
		break;
	case LCD_FPS_90:
		*scence = LCD_FPS_SCENCE_90;
		break;
	case LCD_FPS_120:
		*scence = LCD_FPS_SCENCE_120;
		break;
	case LCD_FPS_180:
		*scence = LCD_FPS_SCENCE_180;
		break;
	default:
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}

static ssize_t lcd_fps_scence_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_fps_order_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	char str[LCD_REG_LENGTH_MAX] = {0};
	char tmp[MAX_BUF] = {0};
	int i;
	int fps_index;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buf || !attr) {
		LCD_KIT_ERR("lcd_fps_order_show buf NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!disp_info->fps.support) {
		ret = snprintf(buf, PAGE_SIZE, "0\n");
		return ret;
	} else {
		ret = snprintf(str, sizeof(str), "1,%d,%d", disp_info->fps.default_fps,
			disp_info->fps.panel_support_fps_list.cnt);
		for (i = 0; i < disp_info->fps.panel_support_fps_list.cnt; i++) {
			ret = lcd_fps_get_scence(disp_info->fps.panel_support_fps_list.buf[i],
				&fps_index);
			if (i == 0)
				strncat(str, ",", strlen(","));
			else
				strncat(str, ";", strlen(";"));
			ret += snprintf(tmp, sizeof(tmp), "%d:%d", fps_index, ORDER_DELAY);
			strncat(str, tmp, strlen(tmp));
		}
	}
	LCD_KIT_INFO("%s\n", str);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str);
	return ret;
}

static ssize_t lcd_alpm_function_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	return ret;
}

static ssize_t lcd_alpm_function_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_alpm_setting_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (!buf) {
		LCD_KIT_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_alpm_setting_ret);
}

static ssize_t lcd_alpm_setting_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int mode = 0;
	uint32_t panel_id = lcd_get_active_panel_id();

	g_alpm_setting_ret = LCD_KIT_FAIL;
	if (!buf) {
		LCD_KIT_ERR("lcd alpm setting store buf is null!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	if (!sscanf(buf, "%u", &mode)) {
		LCD_KIT_ERR("sscanf return invaild\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->alpm.support && (lcd_kit_alpm_setting(panel_id, mode) < 0))
		return LCD_KIT_FAIL;

	g_alpm_setting_ret = LCD_KIT_OK;
	return LCD_KIT_OK;
}

#ifdef LCD_FACTORY_MODE
static ssize_t lcd_inversion_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return lcd_kit_inversion_get_mode(buf);
}

static ssize_t lcd_inversion_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct qcom_panel_info *panel_info = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

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
	if (adapt_ops->mipi_tx == NULL) {
		LCD_KIT_ERR("mipi_tx is NULL\n");
		return LCD_KIT_FAIL;
	}
	val = simple_strtoul(buf, NULL, 0);
	if (!FACT_INFO->inversion.support) {
		LCD_KIT_ERR("not support inversion\n");
		return LCD_KIT_OK;
	}
	switch (val) {
	case COLUMN_MODE:
		ret = adapt_ops->mipi_tx(panel_info->display,
			&FACT_INFO->inversion.column_cmds);
		LCD_KIT_DEBUG("ret = %d", ret);
		break;
	case DOT_MODE:
		ret = adapt_ops->mipi_tx(panel_info->display,
			&FACT_INFO->inversion.dot_cmds);
		LCD_KIT_DEBUG("ret = %d", ret);
		break;
	default:
		return LCD_KIT_FAIL;
	}
	FACT_INFO->inversion.mode = (int)val;
	LCD_KIT_INFO("inversion.support = %d, inversion.mode = %d\n",
		FACT_INFO->inversion.support, FACT_INFO->inversion.mode);
	return LCD_KIT_OK;
}

static ssize_t lcd_check_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint8_t read_value[MAX_REG_READ_COUNT] = {0};
	int i;
	char *expect_ptr = NULL;
	unsigned int panel_state;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	struct qcom_panel_info *panel_info = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

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
	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return ret;
	}
	if (FACT_INFO->check_reg.support) {
		expect_ptr = (char *)FACT_INFO->check_reg.value.buf;
		ret = adapt_ops->mipi_rx(panel_info->display, read_value,
			MAX_REG_READ_COUNT, &FACT_INFO->check_reg.cmds);
		for (i = 0; i < FACT_INFO->check_reg.cmds.cmd_cnt; i++) {
			if ((char)read_value[i] != expect_ptr[i]) {
				ret = -1;
				LCD_KIT_ERR("read_value[%u] = 0x%x, but expect_ptr[%u] = 0x%x!\n",
					i, read_value[i], i, expect_ptr[i]);
				break;
			}
			LCD_KIT_INFO("read_value[%u] = 0x%x same with expect value!\n",
					 i, read_value[i]);
		}
		if (ret == 0)
			ret = snprintf(buf, PAGE_SIZE, "OK\n");
		else
			ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
		LCD_KIT_INFO("checkreg result:%s\n", buf);
	}
	return ret;
}

static ssize_t lcd_sleep_ctrl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	return lcd_kit_get_sleep_mode(panel_id, buf);
}

static ssize_t lcd_sleep_ctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long val = 0;
	struct qcom_panel_info *pinfo = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	pinfo = lcm_get_panel_info(panel_id);
	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	if (!pinfo->panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_FAIL;
	}

	ret = lcd_kit_set_sleep_mode(panel_id, val);
	if (ret)
		LCD_KIT_ERR("set sleep mode fail\n");
	return ret;
}
#endif
static ssize_t lcd_amoled_acl_ctrl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (common_ops->get_acl_mode)
		ret = common_ops->get_acl_mode(panel_id, buf);
	return ret;
}

static ssize_t lcd_amoled_acl_ctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_amoled_vr_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t lcd_amoled_vr_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_effect_color_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return LCD_KIT_OK;
}

static ssize_t lcd_effect_color_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

#ifdef LCD_FACTORY_MODE
static int lcd_kit_current_det(uint32_t panel_id)
{
	ssize_t current_check_result = 0;

	if (!FACT_INFO->current_det.support) {
		LCD_KIT_INFO("current detect is not support! return!\n");
		return LCD_KIT_OK;
	}
	return current_check_result;
}

static int lcd_kit_lv_det(uint32_t panel_id)
{
	ssize_t lv_check_result = 0;

	if (!FACT_INFO->lv_det.support) {
		LCD_KIT_INFO("current detect is not support! return!\n");
		return LCD_KIT_OK;
	}
	return lv_check_result;
}

static ssize_t lcd_test_config_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	ret = lcd_kit_get_test_config(panel_id, buf);
	if (ret < 0) {
		LCD_KIT_ERR("not find test item\n");
		return ret;
	}
	if (buf) {
		if (!strncmp(buf, "OVER_CURRENT_DETECTION",
			strlen("OVER_CURRENT_DETECTION"))) {
			ret = lcd_kit_current_det(panel_id);
			return snprintf(buf, PAGE_SIZE, "%d", ret);
		}
		if (!strncmp(buf, "OVER_VOLTAGE_DETECTION",
			strlen("OVER_VOLTAGE_DETECTION"))) {
			ret = lcd_kit_lv_det(panel_id);
			return snprintf(buf, PAGE_SIZE, "%d", ret);
		}
	}
	return ret;
}

static ssize_t lcd_test_config_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t panel_id = lcd_get_active_panel_id();

	if (lcd_kit_set_test_config(panel_id, buf) < 0)
		LCD_KIT_ERR("set_test_config failed\n");
	return count;
}

static ssize_t lcd_poweric_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	int i;
	char str[LCD_PMIC_LENGTH_MAX] = {0};
	char tmp[LCD_PMIC_LENGTH_MAX] = {0};
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!FACT_INFO->poweric_detect.poweric_num_length ||
		!FACT_INFO->poweric_detect.support) {
		ret += snprintf(tmp, sizeof(tmp), "poweric%d:%d", 1, 1);
		strncat(str, tmp, strlen(tmp));
		LCD_KIT_INFO("poweric_detect not support, default pass\n");
	} else {
		LCD_KIT_INFO("poweric num = %d\n", FACT_INFO->poweric_detect.poweric_num_length);
		for (i = 0; i < FACT_INFO->poweric_detect.poweric_num_length; i++) {
			LCD_KIT_INFO("i = %d, poweric_status = %d\n",
				i, FACT_INFO->poweric_detect.poweric_status[i]);
			ret += snprintf(tmp, sizeof(tmp), "poweric%d:%d",
				(i + 1), FACT_INFO->poweric_detect.poweric_status[i]);
			strncat(str, tmp, strlen(tmp));
			if (i != FACT_INFO->poweric_detect.poweric_num_length - 1)
				strncat(str, "\r\n", strlen("\r\n"));
		}
	}
	LCD_KIT_INFO("%s\n", str);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str);
	return ret;
}

void poweric_gpio_ctl(uint32_t panel_id, int num, int pull_type)
{
	int gpio_temp;

	gpio_temp = FACT_INFO->poweric_detect.gpio_list.buf[num];
	if (pull_type) {
		if (FACT_INFO->poweric_detect.gpio_val.buf[num])
			lcd_poweric_gpio_operator(panel_id, gpio_temp, GPIO_HIGH);
		else
			lcd_poweric_gpio_operator(panel_id, gpio_temp, GPIO_LOW);
	} else {
		if (FACT_INFO->poweric_detect.gpio_val.buf[num])
			lcd_poweric_gpio_operator(panel_id, gpio_temp, GPIO_LOW);
		else
			lcd_poweric_gpio_operator(panel_id, gpio_temp, GPIO_HIGH);
	}
}

int poweric_get_elvdd(uint32_t panel_id, int i)
{
	int ret;
	int32_t gpio_value;

	lcd_poweric_gpio_operator(panel_id, FACT_INFO->poweric_detect.detect_gpio,
		GPIO_REQ);
	ret = gpio_direction_input(FACT_INFO->poweric_detect.detect_gpio);
	if (ret) {
		gpio_free(FACT_INFO->poweric_detect.detect_gpio);
		LCD_KIT_ERR("poweric_gpio[%d] direction set fail\n",
			FACT_INFO->poweric_detect.detect_gpio);
		return LCD_KIT_FAIL;
	}
	gpio_value = gpio_get_value(FACT_INFO->poweric_detect.detect_gpio);
	LCD_KIT_INFO("poweric detect elvdd value is %d\n", gpio_value);

	if (gpio_value != FACT_INFO->poweric_detect.detect_val)
		FACT_INFO->poweric_detect.poweric_status[i] = POWERIC_ERR;
	else
		FACT_INFO->poweric_detect.poweric_status[i] = POWERIC_NOR;
	return LCD_KIT_OK;
}

int poweric_set_elvdd(uint32_t panel_id, int start, int end, int i)
{
	int j, k, ret, temp;
	int gpio_flag = 0;
	for (j = start; j < end; j++) {
		lcd_poweric_gpio_operator(panel_id, FACT_INFO->poweric_detect.gpio_list.buf[j],
			GPIO_REQ);
		ret = gpio_direction_output(FACT_INFO->poweric_detect.gpio_list.buf[j],
			OUTPUT_TYPE);
		if (ret) {
			gpio_flag = POWERIC_NOR;
			for (k = start; k <= j; k++) {
				temp = FACT_INFO->poweric_detect.gpio_list.buf[k];
				lcd_poweric_gpio_operator(panel_id, temp, GPIO_FREE);
			}
			LCD_KIT_ERR("poweric_gpio[%d] direction set fail\n",
				FACT_INFO->poweric_detect.gpio_list.buf[j]);
			FACT_INFO->poweric_detect.poweric_status[i] = POWERIC_ERR;
			break;
		}
		LCD_KIT_INFO("disp_info->poweric_detect.gpio_list.buf[j] = %d\n",
			FACT_INFO->poweric_detect.gpio_list.buf[j]);
		/* enable gpio */
		poweric_gpio_ctl(panel_id, j, PULL_TYPE_NOR);
		/* enable delay between gpio */
		msleep(10);
	}
	return gpio_flag;
}

void poweric_reverse_gpio(uint32_t panel_id, int start, int end)
{
	int j;
	for (j = start; j < end; j++) {
		poweric_gpio_ctl(panel_id, j, PULL_TYPE_REV);
		/* disable delay between gpio */
		msleep(10);
	}
}

void poweric_free_gpio(uint32_t panel_id, int start, int end)
{
	int j;
	for (j = start; j < end; j++)
		lcd_poweric_gpio_operator(panel_id,
			FACT_INFO->poweric_detect.gpio_list.buf[j],
			GPIO_FREE);
}
void poweric_gpio_detect_dbc(uint32_t panel_id)
{
	int i, gpio_temp;
	int32_t gpio_value;
	int gpio_pos = 0;
	int gpio_flag, temp;

	temp = FACT_INFO->poweric_detect.poweric_num;
	FACT_INFO->poweric_detect.poweric_num_length = temp;
	LCD_KIT_INFO("poweric_num = %d\n",
		FACT_INFO->poweric_detect.poweric_num);

	for (i = 0; i < FACT_INFO->poweric_detect.poweric_num; i++) {
		temp = FACT_INFO->poweric_detect.detect_gpio;
		gpio_temp = gpio_pos;
		gpio_pos += FACT_INFO->poweric_detect.gpio_num_list.buf[i];
		/* operate gpio to detect elvdd */
		gpio_flag = poweric_set_elvdd(panel_id, gpio_temp, gpio_pos, i);
		if (!gpio_flag) {
			LCD_KIT_INFO("detect_gpio = %d\n", temp);
			if (poweric_get_elvdd(panel_id, i))
				LCD_KIT_INFO("get ELVDD fail %d\n", temp);
			/* reverse operate gpio to recovery elvdd */
			poweric_reverse_gpio(panel_id, gpio_temp, gpio_pos);
			/* delay before obtaining ELVDD */
			msleep(1000);
			gpio_value = gpio_get_value(
				FACT_INFO->poweric_detect.detect_gpio);
			lcd_poweric_gpio_operator(panel_id,
				FACT_INFO->poweric_detect.detect_gpio,
				GPIO_FREE);
			/* free gpio */
			poweric_free_gpio(panel_id, gpio_temp, gpio_pos);
			LCD_KIT_INFO("second detect elvdd value is %d\n", gpio_value);
		} else {
			gpio_flag = POWERIC_ERR;
		}
	}
}

static ssize_t lcd_poweric_det_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	if (!buf) {
		LCD_KIT_ERR("lcd poweric det store buf NULL Pointer\n");
		return LCD_KIT_OK;
	}
	FACT_INFO->poweric_detect.poweric_num_length = 0;
	if (FACT_INFO->poweric_detect.support) {
		LCD_KIT_INFO("poweric detect is support\n");
		poweric_gpio_detect_dbc(panel_id);
	} else {
		LCD_KIT_ERR("poweric detect is not support\n");
	}
	return count;
}

static void lcd_enable_checksum(uint32_t panel_id)
{
	struct qcom_panel_info *panel_info = NULL;
	/* disable esd */
	lcd_esd_enable(panel_id, 0);

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return;
	}
	FACT_INFO->checksum.status = LCD_KIT_CHECKSUM_START;
	if (!FACT_INFO->checksum.special_support)
		lcd_kit_dsi_cmds_tx(panel_info->display, &FACT_INFO->checksum.enable_cmds);
	FACT_INFO->checksum.pic_index = INVALID_INDEX;
	LCD_KIT_INFO("Enable checksum\n");
}

static int lcd_kit_checksum_set(uint32_t panel_id, int pic_index)
{
	int ret = LCD_KIT_OK;
	struct qcom_panel_info *panel_info = NULL;

	if (FACT_INFO->checksum.status == LCD_KIT_CHECKSUM_END) {
		if (pic_index == TEST_PIC_0) {
			LCD_KIT_INFO("start gram checksum\n");
			lcd_enable_checksum(panel_id);
			return ret;
		}
		LCD_KIT_INFO("pic index error\n");
		return LCD_KIT_FAIL;
	}
	if (pic_index == TEST_PIC_2)
		FACT_INFO->checksum.check_count++;
	if (FACT_INFO->checksum.check_count == CHECKSUM_CHECKCOUNT) {
		LCD_KIT_INFO("check 5 times, set checksum end\n");
		FACT_INFO->checksum.status = LCD_KIT_CHECKSUM_END;
		FACT_INFO->checksum.check_count = 0;
	}
	switch (pic_index) {
	case TEST_PIC_0:
	case TEST_PIC_1:
	case TEST_PIC_2:
		panel_info = lcm_get_panel_info(panel_id);
		if (!panel_info) {
			LCD_KIT_ERR("panel_info is NULL\n");
			return LCD_KIT_FAIL;
		}
		if (FACT_INFO->checksum.special_support)
			lcd_kit_dsi_cmds_tx(panel_info->display,
				&FACT_INFO->checksum.enable_cmds);
		LCD_KIT_INFO("set pic [%d]\n", pic_index);
		FACT_INFO->checksum.pic_index = pic_index;
		break;
	default:
		LCD_KIT_INFO("set pic [%d] unknown\n", pic_index);
		FACT_INFO->checksum.pic_index = INVALID_INDEX;
		break;
	}
	return ret;
}

static int lcd_checksum_compare(uint8_t *read_value, uint32_t *value,
	int len)
{
	int i;
	int err_no = 0;
	uint8_t tmp;

	for (i = 0; i < len; i++) {
		tmp = (uint8_t)value[i];
		if (read_value[i] != tmp) {
			LCD_KIT_ERR("gram check error\n");
			LCD_KIT_ERR("read_value[%d]:0x%x\n", i, read_value[i]);
			LCD_KIT_ERR("expect_value[%d]:0x%x\n", i, tmp);
			err_no++;
		}
	}
	return err_no;
}

static int lcd_check_checksum(uint32_t panel_id)
{
	int ret;
	int err_cnt;
	int err_cnt_ext;
	int check_cnt;
	uint8_t read_value[LCD_KIT_CHECKSUM_SIZE + 1] = {0};
	uint32_t *checksum = NULL;
	uint32_t *checksum_ext = NULL;
	uint32_t pic_index;
	struct qcom_panel_info *panel_info = NULL;

	if (FACT_INFO->checksum.value.arry_data == NULL) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	pic_index = FACT_INFO->checksum.pic_index;
	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_kit_dsi_cmds_rx(panel_info->display, read_value, LCD_KIT_CHECKSUM_SIZE,
		&FACT_INFO->checksum.checksum_cmds);
	if (ret) {
		LCD_KIT_ERR("checksum read dsi0 error\n");
		return LCD_KIT_FAIL;
	}
	check_cnt = FACT_INFO->checksum.value.arry_data[pic_index].cnt;
	if (check_cnt > LCD_KIT_CHECKSUM_SIZE) {
		LCD_KIT_ERR("checksum count is larger than checksum size\n");
		return LCD_KIT_FAIL;
	}
	checksum = FACT_INFO->checksum.value.arry_data[pic_index].buf;
	err_cnt = lcd_checksum_compare(read_value, checksum, check_cnt);
	if (err_cnt) {
		LCD_KIT_ERR("err_cnt:%d\n", err_cnt);
		ret = LCD_KIT_FAIL;
	}
	if (FACT_INFO->checksum.value_ext.arry_data) {
		LCD_KIT_INFO("checksum value ext find, do checksum again\n");
		checksum_ext = FACT_INFO->checksum.value_ext.arry_data[pic_index].buf;
		err_cnt_ext = lcd_checksum_compare(read_value, checksum_ext, check_cnt);
		if (!err_cnt_ext) {
			LCD_KIT_INFO("ext checksum pass, ret is LCD_KIT_OK\n");
			ret = LCD_KIT_OK;
		} else {
			LCD_KIT_ERR("ext checksum failed, err_cnt_ext:%d\n", err_cnt_ext);
			if (!err_cnt)
				LCD_KIT_INFO("primary checksum pass, ret is LCD_KIT_OK\n");
		}
	}
	return ret;
}

static int lcd_kit_checksum_check(uint32_t panel_id)
{
	int ret;
	uint32_t pic_index;
	struct qcom_panel_info *panel_info = NULL;

	pic_index = FACT_INFO->checksum.pic_index;
	LCD_KIT_INFO("panel_id %u, checksum pic num:%d\n", panel_id, pic_index);
	if (pic_index > TEST_PIC_2) {
		LCD_KIT_ERR("checksum pic num unknown:%d\n", pic_index);
		return LCD_KIT_FAIL;
	}
	ret = lcd_check_checksum(panel_id);
	if (ret)
		LCD_KIT_ERR("checksum error\n");
	if (ret && FACT_INFO->checksum.pic_index == TEST_PIC_2)
		FACT_INFO->checksum.status = LCD_KIT_CHECKSUM_END;

	if (FACT_INFO->checksum.status == LCD_KIT_CHECKSUM_END) {
		panel_info = lcm_get_panel_info(panel_id);
		if (!panel_info) {
			LCD_KIT_ERR("panel_info is NULL\n");
			return LCD_KIT_FAIL;
		}
		lcd_kit_dsi_cmds_tx(panel_info->display, &FACT_INFO->checksum.disable_cmds);
		LCD_KIT_INFO("gram checksum end, disable checksum\n");
		/* enable esd */
		lcd_esd_enable(panel_id, 1);
	}
	return ret;
}

static ssize_t lcd_gram_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	int checksum_result;
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (FACT_INFO->checksum.support) {
		down(&disp_info->blank_sem);
		panel_state = lcm_get_panel_state(panel_id);
		if (panel_state) {
			checksum_result = lcd_kit_checksum_check(panel_id);
			ret = snprintf(buf, PAGE_SIZE, "%d\n", checksum_result);
		}
		up(&disp_info->blank_sem);
	}
	return ret;
}

static ssize_t lcd_gram_check_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long val = 0;
	int index;
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;
	LCD_KIT_INFO("panel_id %u, val=%ld\n", panel_id, val);
	if (FACT_INFO->checksum.support) {
		down(&disp_info->blank_sem);
		panel_state = lcm_get_panel_state(panel_id);
		if (panel_state) {
			index = val - INDEX_START;
			ret = lcd_kit_checksum_set(panel_id, index);
		}
		up(&disp_info->blank_sem);
	}
	return count;
}

static ssize_t lcd_fpc_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	int i;
	char str[LCD_RSP_STR_LENGTH_MAX] = {0};
	int gpio_val;
	uint32_t gpio_num;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!FACT_INFO->fpc_detect.support) {
		LCD_KIT_INFO("fpc detect not support, default pass\n");
		(void)snprintf(str, sizeof(str), "fpc check %s", "ok");
	} else {
#ifndef CONFIG_QCOM_MODULE_KO
		if (FACT_INFO->fpc_detect.pinctrl_cfg)
			lcd_kit_set_pinctrl_status("fpc_active");
#endif
		for (i = 0; i < FACT_INFO->fpc_detect.det_gpio_list.cnt; i++) {
			gpio_num = FACT_INFO->fpc_detect.det_gpio_list.buf[i];
			gpio_val = lcd_kit_get_gpio_value(gpio_num, "lcd_fpc_detect");
			if (gpio_val != FACT_INFO->fpc_detect.exp_gpio_val_list.buf[i]) {
				LCD_KIT_ERR("gpio_num = %u, gpio_val = %d\n",
					gpio_num, gpio_val);
				(void)snprintf(str, sizeof(str), "fpc check %s", "fail");
				return snprintf(buf, PAGE_SIZE, "%s\n", str);
			}
			LCD_KIT_INFO("gpio_num = %u, gpio_val = %d\n", gpio_num, gpio_val);
		}
		ret = snprintf(str, sizeof(str), "fpc check %s", "ok");
	}
	LCD_KIT_INFO("%s\n", str);
	return snprintf(buf, PAGE_SIZE, "%s\n", str);
}
#endif

static ssize_t lcd_reg_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return LCD_KIT_OK;
}

static ssize_t lcd_reg_read_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_gamma_dynamic_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_frame_count_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t lcd_frame_update_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t lcd_frame_update_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t lcd_mipi_clk_upt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return LCD_KIT_OK;
}

static ssize_t lcd_mipi_clk_upt_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return LCD_KIT_OK;
}

static ssize_t lcd_func_switch_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t panel_id;
	struct qcom_panel_info *pinfo = NULL;
	struct dsi_display *disp = NULL;

	LCD_KIT_INFO("enter\n");
	panel_id = lcd_get_active_panel_id();
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return LCD_KIT_FAIL;
	}
	disp = pinfo->display;
	if (!disp || !disp->panel || !disp->panel->cur_mode ||
		!disp->panel->cur_mode->priv_info) {
		LCD_KIT_ERR("null pointer!\n");
		return LCD_KIT_FAIL;
	}
	return snprintf(buf, PAGE_SIZE,
			"dirty_region_upt=%d\n"
			"esd_enable=%d\n"
			"fps_updt_support=%d\n",
			disp->panel->cur_mode->priv_info->roi_caps.enabled,
			disp->panel->esd_config.esd_enabled,
			disp->panel->dms_mode);
}

static void parse_esd_enable(struct dsi_display *disp, const char *command)
{
	static int esd_status_mode = ESD_MODE_MAX;

	if (strncmp("esd_enable:", command, strlen("esd_enable:")))
		return;
	if (command[strlen("esd_enable:")] == '0') {
		disp->panel->esd_config.esd_enabled = false;
		esd_status_mode = disp->panel->esd_config.status_mode;
		disp->panel->esd_config.status_mode = ESD_MODE_MAX;
	} else {
		disp->panel->esd_config.esd_enabled = true;
		if (disp->panel->esd_config.status_mode == ESD_MODE_MAX)
			disp->panel->esd_config.status_mode = esd_status_mode;
	}
}

static void lcd_kit_parse_switch_cmd(struct dsi_display *disp,
	const char *command)
{
	parse_esd_enable(disp, command);
}

static ssize_t lcd_func_switch_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	char command[MAX_BUF] = {0};
	uint32_t panel_id;
	struct qcom_panel_info *pinfo = NULL;
	struct dsi_display *disp = NULL;

	LCD_KIT_INFO("enter\n");
	panel_id = lcd_get_active_panel_id();
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return LCD_KIT_FAIL;
	}
	disp = pinfo->display;
	if (!buf) {
		LCD_KIT_ERR("NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	if (!sscanf(buf, "%s", command)) {
		LCD_KIT_ERR("bad command(%s)\n", command);
		return count;
	}
	lcd_kit_parse_switch_cmd(disp, command);
	return count;
}

static int lcd_get_project_id(uint32_t panel_id, char *oem_data)
{
	struct lcd_kit_ops *lcd_ops = NULL;
	char project_id[PROJECT_ID_LENGTH] = {0};
	int ret = 0;
	int i;

	lcd_ops = lcd_kit_get_ops();
	if (!lcd_ops) {
		LCD_KIT_ERR("lcd_ops is null\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->project_id.support) {
		if (lcd_ops->get_project_id)
			ret = lcd_ops->get_project_id(project_id);
		oem_data[0] = PROJECT_ID_TYPE;
		oem_data[1] = OEM_INFO_BLOCK_NUM;
		for (i = 0; i < PROJECT_ID_LENGTH; i++)
			/* 0 type 1 block number after 2 for data */
			oem_data[i + 2] = project_id[i];
	}
	return ret;
}

static int lcd_get_2d_barcode(uint32_t panel_id, char *oem_data)
{
	struct lcd_kit_ops *lcd_ops = NULL;
	char read_value[OEM_INFO_SIZE_MAX] = {0};
	int ret = 0;

	lcd_ops = lcd_kit_get_ops();
	if (!lcd_ops) {
		LCD_KIT_ERR("lcd_ops is null\n");
		return LCD_KIT_FAIL;
	}

	if (disp_info->oeminfo.barcode_2d.support) {
		if (lcd_ops->get_2d_barcode)
			ret = lcd_ops->get_2d_barcode(panel_id, read_value);
		oem_data[0] = BARCODE_2D_TYPE;
		oem_data[1] = BARCODE_BLOCK_NUM +
			disp_info->oeminfo.barcode_2d.number_offset;
		strncat(oem_data, read_value, strlen(read_value));
	}
	return ret;
}

static ssize_t lcd_oem_info_show(struct device* dev,
	struct device_attribute* attr, char *buf)
{
	int i;
	int ret;
	int length;
	char oem_info_data[OEM_INFO_SIZE_MAX] = {0};
	char str_oem[OEM_INFO_SIZE_MAX + 1] = {0};
	char str_tmp[OEM_INFO_SIZE_MAX + 1] = {0};
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_FAIL;
	}

	if (!disp_info->oeminfo.support) {
		LCD_KIT_ERR("oem info is not support\n");
		return LCD_KIT_FAIL;
	}

	if (oem_info_type == INVALID_TYPE) {
		LCD_KIT_ERR("first write ddic_oem_info, then read\n");
		return LCD_KIT_FAIL;
	}

	length = sizeof(oem_read_cmds) / sizeof(oem_read_cmds[0]);
	for (i = 0; i < length; i++) {
		if (oem_info_type == oem_read_cmds[i].type) {
			LCD_KIT_INFO("cmd = %d\n", oem_info_type);
			if (oem_read_cmds[i].func)
				(*oem_read_cmds[i].func)(panel_id, oem_info_data);
		}
	}

	/* parse data */
	memset(str_oem, 0, sizeof(str_oem));
	for (i = 0; i < oem_info_data[1]; i++) {
		memset(str_tmp, 0, sizeof(str_tmp));
		snprintf(str_tmp, sizeof(str_tmp),
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
			oem_info_data[0 + i * BARCODE_BLOCK_LEN],
			oem_info_data[1 + i * BARCODE_BLOCK_LEN],
			oem_info_data[2 + i * BARCODE_BLOCK_LEN],
			oem_info_data[3 + i * BARCODE_BLOCK_LEN],
			oem_info_data[4 + i * BARCODE_BLOCK_LEN],
			oem_info_data[5 + i * BARCODE_BLOCK_LEN],
			oem_info_data[6 + i * BARCODE_BLOCK_LEN],
			oem_info_data[7 + i * BARCODE_BLOCK_LEN],
			oem_info_data[8 + i * BARCODE_BLOCK_LEN],
			oem_info_data[9 + i * BARCODE_BLOCK_LEN],
			oem_info_data[10 + i * BARCODE_BLOCK_LEN],
			oem_info_data[11 + i * BARCODE_BLOCK_LEN],
			oem_info_data[12 + i * BARCODE_BLOCK_LEN],
			oem_info_data[13 + i * BARCODE_BLOCK_LEN],
			oem_info_data[14 + i * BARCODE_BLOCK_LEN],
			oem_info_data[15 + i * BARCODE_BLOCK_LEN]);

		strncat(str_oem, str_tmp, strlen(str_tmp));
	}

	LCD_KIT_INFO("str_oem = %s\n", str_oem);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str_oem);
	return ret;
}

static ssize_t lcd_oem_info_store(struct device *dev,
	struct device_attribute *attr, const char *buff, size_t count)
{
	char *cur = NULL;
	char *token = NULL;
	char oem_info[OEM_INFO_SIZE_MAX] = {0};
	unsigned int panel_state;
	int i = 0;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buff) {
		LCD_KIT_ERR("buff is NULL\n");
		return count;
	}

	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_FAIL;
	}

	if (!disp_info->oeminfo.support) {
		LCD_KIT_ERR("oem info is not support\n");
		return LCD_KIT_FAIL;
	}

	if (strlen(buff) < OEM_INFO_SIZE_MAX) {
		cur = (char *)buff;
		token = strsep(&cur, ",");
		while (token) {
			oem_info[i++] = (unsigned char)simple_strtol(token, NULL, 0);
			token = strsep(&cur, ",");
		}
	} else {
		memcpy(oem_info, "INVALID", strlen("INVALID") + 1);
		LCD_KIT_ERR("invalid cmd\n");
	}

	LCD_KIT_INFO("write Type=0x%x , data len=%d\n", oem_info[0], oem_info[DATA_INDEX]);

	oem_info_type = oem_info[0];
	/* if the data length is 0, then just store the type */
	if (oem_info[DATA_INDEX] == 0) {
		LCD_KIT_INFO("Just store type:0x%x and then finished\n", oem_info[0]);
		return count;
	}

	LCD_KIT_INFO("oem_info = %s\n", oem_info);
	return count;
}

static ssize_t lcd_kit_cabc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (common_ops->get_cabc_mode)
		ret = common_ops->get_cabc_mode(panel_id, buf);
	return ret;
}

static ssize_t lcd_kit_cabc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = LCD_KIT_OK;
	unsigned long mode = 0;
	struct qcom_panel_info *panel_info = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	panel_info = lcm_get_panel_info(panel_id);
	if (panel_info == NULL) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}

	ret = strict_strtoul(buf, 0, &mode);
	if (ret) {
		LCD_KIT_ERR("invalid data!\n");
		return ret;
	}

	if (common_ops->set_cabc_mode) {
		mutex_lock(&COMMON_LOCK->mipi_lock);
		common_ops->set_cabc_mode(panel_id, panel_info->display, mode);
		mutex_unlock(&COMMON_LOCK->mipi_lock);
	}
	return ret;
}

static ssize_t lcd_panel_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	struct qcom_panel_info *pinfo = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buf || !attr) {
		LCD_KIT_ERR("buf is NULL\n");
		return LCD_KIT_FAIL;
	}
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	if (!pinfo->panel_version.support) {
		ret = snprintf(buf, PAGE_SIZE, "VER:NA\n");
		return ret;
	}
	ret = snprintf(buf, PAGE_SIZE, "%s\n", pinfo->panel_version.lcd_panel_version);
	return ret;
}

static ssize_t lcd_panel_sncode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i;
	ssize_t ret;
	struct qcom_panel_info *pinfo = NULL;
	char str_oem[OEM_INFO_SIZE_MAX] = {0};
	char str_tmp[OEM_INFO_SIZE_MAX] = {0};
	uint32_t panel_id = fold_panel_id;

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}

	pinfo = lcm_get_panel_info(panel_id);
	if (pinfo == NULL) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	for(i = 0; i < sizeof(pinfo->sn_code); i++) {
		memset(str_tmp, 0, sizeof(str_tmp));
		ret = snprintf(str_tmp, sizeof(str_tmp), "%d,", pinfo->sn_code[i]);
		if (ret < 0) {
			LCD_KIT_ERR("snprintf fail\n");
			return LCD_KIT_FAIL;
		}
		strncat(str_oem, str_tmp, strlen(str_tmp));
	}
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str_oem);
	LCD_KIT_INFO("lcd_panel_sncode_show str_oem = %s\n", str_oem);
	if (ret < 0) {
		LCD_KIT_ERR("snprintf fail\n");
		return LCD_KIT_FAIL;
	}
	return ret;
}

static ssize_t lcd_sys_panel_sncode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int val = 0;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}

	fold_panel_id = val;
	LCD_KIT_INFO("panel id is %u\n", val);
	return ret;
}

static ssize_t lcd_pre_camera_position_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t len;
	uint32_t panel_id = lcd_get_active_panel_id();
	if (!buf) {
		LCD_KIT_ERR("buf is NULL Pointer\n");
		return LCD_KIT_FAIL;
	}

	len = snprintf(buf, PAGE_SIZE, "end_y:%u\n",
		common_info->p_cam_position.end_y);
	if (len < 0) {
		LCD_KIT_ERR("snprintf fail\n");
		return LCD_KIT_FAIL;
	}
	return len;
}

static ssize_t lcd_finger_unlock_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	uint32_t panel_id;
	struct qcom_panel_info *pinfo = NULL;

	panel_id = lcd_get_active_panel_id();
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", pinfo->finger_unlock_state ? 1 : 0);
}

static ssize_t lcd_finger_unlock_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;
	unsigned int panel_id;
	struct qcom_panel_info *pinfo = NULL;
	int ret;

	panel_id = lcd_get_active_panel_id();
	if (!buf) {
		LCD_KIT_ERR("buf is null!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	LCD_KIT_INFO("finger unlock state is %u\n", val);
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	pinfo->finger_unlock_state = val;
	return LCD_KIT_OK;
}

#ifdef LCD_FACTORY_MODE
static int lcd_judge_ddic_lv_detect(uint32_t panel_id, unsigned int pic_index, int loop)
{
	int i;
	unsigned char expect_value;
	int ret;
	unsigned int *detect_value = NULL;
	unsigned char read_value[MAX_REG_READ_COUNT] = {0};
	struct lcd_kit_array_data congfig_data;
	struct qcom_panel_info *panel_info = NULL;

	LCD_KIT_INFO("+\n");
	congfig_data = FACT_INFO->ddic_lv_detect.value[pic_index];
	detect_value = congfig_data.buf;
	if (!FACT_INFO->ddic_lv_detect.rd_cmds[pic_index].cmds) {
		LCD_KIT_INFO("read ddic lv detect cmd is NULL\n");
		return LCD_KIT_FAIL;
	}
	/* delay 2s, or ddic reg value invalid */
	ssleep(2);
	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_kit_dsi_cmds_rx(panel_info->display, read_value,
		MAX_REG_READ_COUNT - 1, &FACT_INFO->ddic_lv_detect.rd_cmds[pic_index]);
	if (ret) {
		LCD_KIT_INFO("read ddic lv detect cmd error\n");
		return ret;
	}
	for (i = 0; i < congfig_data.cnt; i++) {
		/* obtains the value of the first byte */
		expect_value = detect_value[i] & 0xFF;
		LCD_KIT_INFO("pic: %u, read_val:%d = 0x%x, expect_val = 0x%x\n",
			pic_index, i, read_value[i], expect_value);
		if ((i < VAL_NUM) && (loop < DETECT_LOOPS))
			FACT_INFO->ddic_lv_detect.reg_val[loop][pic_index][i] =
				read_value[i];
		if (read_value[i] != expect_value) {
			FACT_INFO->ddic_lv_detect.err_flag[pic_index]++;
			LCD_KIT_ERR("pic: %u, read_val:%d = 0x%x, "
				"but expect_val = 0x%x\n",
				pic_index, i, read_value[i], expect_value);
			return LCD_KIT_FAIL;
		}
	}
	return ret;
}

static int lcd_ddic_lv_detect_test(uint32_t panel_id)
{
	int i;
	int ret;
	static int count;
	static int loop_num;
	int err_record = 0;
	unsigned int pic_index;
	unsigned int *error_flag = NULL;

	LCD_KIT_INFO("+\n");
	pic_index = FACT_INFO->ddic_lv_detect.pic_index;
	error_flag = FACT_INFO->ddic_lv_detect.err_flag;
	if (pic_index >= DETECT_NUM) {
		LCD_KIT_ERR("pic number not support\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_judge_ddic_lv_detect(panel_id, pic_index, loop_num);
	if (ret)
		LCD_KIT_ERR("ddic lv detect judge fail\n");
	count++;
	if (count >= DETECT_NUM) {
		loop_num++;
		/* set initial value */
		count = 0;
	}
	LCD_KIT_INFO("count = %d, loop_num = %d\n", count, loop_num);
	if (loop_num >= DETECT_LOOPS) {
		for (i = 0; i < DETECT_NUM; i++) {
			LCD_KIT_INFO("pic : %d, err_num = %d\n",
				i, error_flag[i]);
			if (error_flag[i] > ERR_THRESHOLD)
				err_record++;
			/* set initial value */
			FACT_INFO->ddic_lv_detect.err_flag[i] = 0;
		}
		if (err_record >= i)
			lcd_kit_ddic_lv_detect_dmd_report(
				FACT_INFO->ddic_lv_detect.reg_val);
		/* set initial value */
		loop_num = 0;
	}
	LCD_KIT_INFO("-\n");
	return ret;
}

static int lcd_ddic_lv_detect_set(uint32_t panel_id, unsigned int pic_index)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_dsi_panel_cmds *enter_cmds = NULL;
	struct qcom_panel_info *panel_info = NULL;

	LCD_KIT_INFO("+\n");
	pic_index = pic_index - DET_START;
	enter_cmds = FACT_INFO->ddic_lv_detect.enter_cmds;
	if ((pic_index >= DET1_INDEX) && (pic_index < DETECT_NUM)) {
		/* disable esd */
		lcd_esd_enable(panel_id, DISABLE);
		if (!enter_cmds[pic_index].cmds) {
			LCD_KIT_ERR("send ddic lv detect cmd is NULL\n");
			return LCD_KIT_FAIL;
		}
		panel_info = lcm_get_panel_info(panel_id);
		if (!panel_info) {
			LCD_KIT_ERR("panel_info is NULL\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_dsi_cmds_tx(panel_info->display, &enter_cmds[pic_index]);
		if (ret) {
			LCD_KIT_ERR("send ddic lv detect cmd error\n");
			return ret;
		}
		LCD_KIT_INFO("set picture : %u\n", pic_index);
		FACT_INFO->ddic_lv_detect.pic_index = pic_index;
	} else {
		LCD_KIT_INFO("set picture : %u unknown\n", pic_index);
		FACT_INFO->ddic_lv_detect.pic_index = INVALID_INDEX;
	}
	return ret;
}

static ssize_t lcd_ddic_lv_detect_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int result;
	ssize_t ret = LCD_KIT_OK;
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("+\n");
	panel_state = lcm_get_panel_state(panel_id);
	if (FACT_INFO->ddic_lv_detect.support) {
		down(&disp_info->blank_sem);
		if (panel_state) {
			result = lcd_ddic_lv_detect_test(panel_id);
			ret = snprintf(buf, PAGE_SIZE, "%d\n", result);
		}
		up(&disp_info->blank_sem);
		lcd_kit_recovery_display(panel_id, LCD_SYNC_MODE);
		/* enable esd */
		lcd_esd_enable(panel_id, ENABLE);
	}
	return ret;
}

static ssize_t lcd_ddic_lv_detect_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	unsigned int index = 0;
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("+\n");
	panel_state = lcm_get_panel_state(panel_id);
	if (!FACT_INFO->ddic_lv_detect.support) {
		LCD_KIT_ERR("ddic lv detect is not support\n");
		return LCD_KIT_FAIL;
	}
	if (!dev) {
		LCD_KIT_ERR("dev is null\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	/* decimal parsing */
	ret = kstrtouint(buf, 10, &index);
	if (ret) {
		LCD_KIT_ERR("strict_strtoul fail\n");
		return ret;
	}
	LCD_KIT_INFO("picture index=%u\n", index);
	down(&disp_info->blank_sem);
	if (panel_state)
		ret = lcd_ddic_lv_detect_set(panel_id, index);
	up(&disp_info->blank_sem);
	return ret;
}

static int lcd_hor_line_test(uint32_t panel_id)
{
	int ret = LCD_KIT_OK;
	unsigned int panel_state;
	int count = HOR_LINE_TEST_TIMES;
	struct qcom_panel_info *panel_info = NULL;

	LCD_KIT_INFO("horizontal line test start\n");
	LCD_KIT_INFO("g_fact_info.hor_line.duration = %d\n",
			FACT_INFO->hor_line.duration);

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	panel_state = lcm_get_panel_state(panel_id);

	/* disable esd check */
	lcd_esd_enable(panel_id, DISABLE);
	while (count--) {
		mdelay(30);
		/* test avdd */
		down(&disp_info->blank_sem);
		if (!panel_state) {
			LCD_KIT_ERR("panel is power off\n");
			up(&disp_info->blank_sem);
			return LCD_KIT_FAIL;
		}
		if (FACT_INFO->hor_line.hl_cmds.cmds) {
			ret = lcd_kit_dsi_cmds_tx(panel_info->display,
				&FACT_INFO->hor_line.hl_cmds);
			if (ret)
				LCD_KIT_ERR("send avdd cmd error\n");
		}
		up(&disp_info->blank_sem);
		msleep(FACT_INFO->hor_line.duration * MILLISEC_TIME);
		/* recovery display */
		lcd_kit_recovery_display(panel_id, LCD_SYNC_MODE);
	}
	/* enable esd */
	lcd_esd_enable(panel_id, ENABLE);
	LCD_KIT_INFO("horizontal line test end\n");
	return ret;
}

static ssize_t lcd_general_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("+\n");
	if (FACT_INFO->hor_line.support)
		ret = lcd_hor_line_test(panel_id);

	if (ret == LCD_KIT_OK)
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");

	return ret;
}

static void lcd_vtc_line_set_bias_voltage(uint32_t panel_id)
{
	struct lcd_kit_bias_ops *bias_ops = NULL;
	int ret;

	bias_ops = lcd_kit_get_bias_ops();
	if (!bias_ops) {
		LCD_KIT_ERR("can not get bias_ops\n");
		return;
	}
	/* set bias voltage */
	if ((bias_ops->set_vtc_bias_voltage) &&
		(FACT_INFO->vtc_line.vtc_vsp > 0) &&
		(FACT_INFO->vtc_line.vtc_vsn > 0)) {
		ret = bias_ops->set_vtc_bias_voltage(
			FACT_INFO->vtc_line.vtc_vsp,
			FACT_INFO->vtc_line.vtc_vsn, true);
		if (ret != LCD_KIT_OK)
			LCD_KIT_ERR("set vtc bias voltage error\n");
		if (bias_ops->set_bias_is_need_disable())
			lcd_kit_recovery_display(panel_id, LCD_SYNC_MODE);
	}
}

static void lcd_vtc_line_resume_bias_voltage(uint32_t panel_id)
{
	struct lcd_kit_bias_ops *bias_ops = NULL;
	int ret;

	bias_ops = lcd_kit_get_bias_ops();
	if (!bias_ops) {
		LCD_KIT_ERR("can not get bias_ops\n");
		return;
	}
	/* set bias voltage */
	if ((bias_ops->set_vtc_bias_voltage) &&
		(bias_ops->set_bias_is_need_disable())) {
		/* buf[2]:set bias voltage value */
		ret = bias_ops->set_vtc_bias_voltage(power_hdl->lcd_vsp.buf[2],
			power_hdl->lcd_vsn.buf[2], false);
		if (ret != LCD_KIT_OK)
			LCD_KIT_ERR("set vtc bias voltage error\n");
	}
}

static int lcd_vtc_line_test(uint32_t panel_id, unsigned long pic_index)
{
	int ret = LCD_KIT_OK;
	struct qcom_panel_info *panel_info = NULL;

	LCD_KIT_INFO("+\n");
	switch (pic_index) {
	case PIC1_INDEX:
		/* disable esd */
		lcd_esd_enable(panel_id, DISABLE);
		/* lcd panel set bias */
		lcd_vtc_line_set_bias_voltage(panel_id);
		/* hardware reset */
		if (!FACT_INFO->vtc_line.vtc_no_reset)
			lcd_hardware_reset(panel_id);
		mdelay(20);
		panel_info = lcm_get_panel_info(panel_id);
		if (!panel_info) {
			LCD_KIT_ERR("panel_info is NULL\n");
			return LCD_KIT_FAIL;
		}
		if (FACT_INFO->vtc_line.vtc_cmds.cmds) {
			ret = lcd_kit_dsi_cmds_tx(panel_info->display,
				&FACT_INFO->vtc_line.vtc_cmds);
			if (ret)
				LCD_KIT_ERR("send vtc cmd error\n");
		}
		break;
	case PIC2_INDEX:
	case PIC3_INDEX:
	case PIC4_INDEX:
		LCD_KIT_INFO("picture:%lu display\n", pic_index);
		break;
	case PIC5_INDEX:
		/* lcd panel resume bias */
		lcd_vtc_line_resume_bias_voltage(panel_id);
		lcd_kit_recovery_display(panel_id, LCD_SYNC_MODE);
		/* enable esd */
		lcd_esd_enable(panel_id, ENABLE);
		break;
	default:
		LCD_KIT_ERR("pic number not support\n");
		break;
	}
	return ret;
}

static ssize_t lcd_vertical_line_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("+\n");
	if (FACT_INFO->vtc_line.support)
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	return ret;
}

static ssize_t lcd_vertical_line_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long index = 0;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("+\n");
	if (!dev) {
		LCD_KIT_ERR("dev NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("buf NULL Pointer!\n");
		return LCD_KIT_FAIL;
	}
	ret = kstrtoul(buf, 10, &index);
	if (ret) {
		LCD_KIT_ERR("strict_strtoul fail!\n");
		return ret;
	}
	LCD_KIT_INFO("index=%ld\n", index);
	if (FACT_INFO->vtc_line.support) {
		ret = lcd_vtc_line_test(panel_id, index);
		if (ret)
			LCD_KIT_ERR("vtc line test fail\n");
	}
	return count;
}

static ssize_t lcd_fps_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	char str[LCD_REG_LENGTH_MAX] = {0};
	uint32_t panel_id = lcd_get_active_panel_id();

	if (disp_info->fps.support) {
		ret = snprintf(str, sizeof(str), "FPS60:%s;FPS90:%s;FPS120:%s;",
			disp_info->fps.fps_60_cmd, disp_info->fps.fps_90_cmd,
			disp_info->fps.fps_120_cmd);
	}
	ret = snprintf(buf, PAGE_SIZE, "%s\n", str);
	LCD_KIT_INFO("%s\n", str);
	return ret;
}
#endif

static int lcd_check_support(int index)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	struct qcom_panel_info *pinfo = NULL;

	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	switch (index) {
	case LCD_MODEL_INDEX:
	case LCD_TYPE_INDEX:
	case PANEL_INFO_INDEX:
		return SYSFS_SUPPORT;
	case VOLTAGE_ENABLE_INDEX:
		return SYSFS_NOT_SUPPORT;
	case ACL_INDEX:
		return common_info->acl.support;
	case VR_INDEX:
		return common_info->vr.support;
	case SUPPORT_MODE_INDEX:
		return common_info->effect_color.support;
	case GAMMA_DYNAMIC_INDEX:
		return disp_info->gamma_cal.support;
	case FRAME_COUNT_INDEX:
		return disp_info->vr_support;
	case FRAME_UPDATE_INDEX:
		return disp_info->vr_support;
	case MIPI_DSI_CLK_UPT_INDEX:
	case FPS_SCENCE_INDEX:
	case FPS_ORDER_INDEX:
		return SYSFS_SUPPORT;
	case ALPM_FUNCTION_INDEX:
		return disp_info->alpm.support;
	case ALPM_SETTING_INDEX:
		return disp_info->alpm.support;
	case FUNC_SWITCH_INDEX:
		return SYSFS_SUPPORT;
	case REG_READ_INDEX:
		return disp_info->gamma_cal.support;
	case DDIC_OEM_INDEX:
		return disp_info->oeminfo.support;
	case BL_MODE_INDEX:
		return SYSFS_NOT_SUPPORT;
	case BL_SUPPORT_MODE_INDEX:
		return SYSFS_NOT_SUPPORT;
	case PANEL_SNCODE_INDEX:
		return common_info->sn_code.support;
	case FINGER_UNLOCK_STATE:
		return SYSFS_SUPPORT;
	case PANEL_VERSION_INDEX:
		return pinfo->panel_version.support;
	case LCD_CABC_MODE:
		return common_info->cabc.support;
	case PRE_CAMERA_POSITION:
		return common_info->p_cam_position.support;
	default:
		return SYSFS_NOT_SUPPORT;
	}
}

#ifdef LCD_FACTORY_MODE
static ssize_t lcd_amoled_gpio_pcd_errflag(uint32_t panel_id, struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_kit_gpio_pcd_errflag_check(panel_id);
	ret = snprintf(buf, PAGE_SIZE, "%d\n", ret);
	return ret;
}

static ssize_t lcd_amoled_cmds_pcd_errflag(uint32_t panel_id, struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	int check_result;
	unsigned int panel_state;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	panel_state = lcm_get_panel_state(panel_id);
	if (panel_state) {
		if (disp_info->pcd_errflag.pcd_support ||
			disp_info->pcd_errflag.errflag_support) {
			check_result = lcd_kit_check_pcd_errflag_check_fac(panel_id);
			ret = snprintf(buf, PAGE_SIZE, "%d\n", check_result);
			LCD_KIT_INFO("pcd_errflag, the check_result = %d\n",
				check_result);
		}
	}
	return ret;
}

int lcd_kit_pcd_detect(uint32_t panel_id, uint32_t val)
{
	int ret = LCD_KIT_OK;
	static uint32_t pcd_mode;
	struct qcom_panel_info *panel_info = NULL;

	if (!FACT_INFO->pcd_errflag_check.pcd_errflag_check_support) {
		LCD_KIT_ERR("pcd errflag not support!\n");
		return LCD_KIT_OK;
	}
	if (pcd_mode == val) {
		LCD_KIT_ERR("pcd detect already\n");
		return LCD_KIT_OK;
	}
	pcd_mode = val;

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (pcd_mode == LCD_KIT_PCD_DETECT_OPEN)
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->pcd_errflag_check.pcd_detect_open_cmds);
	else if (pcd_mode == LCD_KIT_PCD_DETECT_CLOSE)
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->pcd_errflag_check.pcd_detect_close_cmds);
	return ret;
}
static ssize_t lcd_amoled_pcd_errflag_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	if (disp_info->pcd_errflag.pcd_support ||
		disp_info->pcd_errflag.errflag_support)
		ret = lcd_amoled_cmds_pcd_errflag(panel_id, dev, attr, buf);
	else
		ret = lcd_amoled_gpio_pcd_errflag(panel_id, dev, attr, buf);
	return ret;
}

static ssize_t lcd_amoled_pcd_errflag_store(struct device *dev,
	struct device_attribute *attr, const char *buf)
{
	int ret;
	unsigned long val = 0;
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		LCD_KIT_ERR("invalid parameter!\n");
		return ret;
	}
	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_FAIL;
	}
	return lcd_kit_pcd_detect(panel_id, val);
}

static ssize_t lcd_oneside_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (FACT_INFO->oneside_mode.support ||
		FACT_INFO->color_aod_det.support)
		ret = snprintf(buf, PAGE_SIZE, "%d\n",
			FACT_INFO->oneside_mode.mode);

	return ret;
}

static int lcd_kit_oneside_setting(uint32_t panel_id, uint32_t mode)
{
	int ret = LCD_KIT_OK;
	struct qcom_panel_info *panel_info = NULL;

	LCD_KIT_INFO("oneside driver mode is %u\n", mode);
	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	FACT_INFO->oneside_mode.mode = (int)mode;
	switch (mode) {
	case ONESIDE_DRIVER_LEFT:
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->oneside_mode.left_cmds);
		break;
	case ONESIDE_DRIVER_RIGHT:
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->oneside_mode.right_cmds);
		break;
	case ONESIDE_MODE_EXIT:
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->oneside_mode.exit_cmds);
		break;
	default:
		break;
	}
	return ret;
}

static int lcd_kit_aod_detect_setting(uint32_t panel_id, uint32_t mode)
{
	int ret = LCD_KIT_OK;
	struct qcom_panel_info *panel_info = NULL;

	LCD_KIT_INFO("panel_id %u, color aod detect mode is %u\n", panel_id, mode);
	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	FACT_INFO->oneside_mode.mode = (int)mode;
	if (mode == COLOR_AOD_DETECT_ENTER)
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->color_aod_det.enter_cmds);
	else if (mode == COLOR_AOD_DETECT_EXIT)
		ret = lcd_kit_dsi_cmds_tx(panel_info->display,
			&FACT_INFO->color_aod_det.exit_cmds);
	else
		LCD_KIT_ERR("color aod detect mode not support\n");
	return ret;
}

static ssize_t lcd_oneside_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	unsigned int mode = 0;
	unsigned int panel_state;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!buf) {
		LCD_KIT_ERR("oneside setting store buf null!\n");
		return LCD_KIT_FAIL;
	}
	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_ERR("panel is power off!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	ret = sscanf(buf, "%u", &mode);
	if (!ret) {
		LCD_KIT_ERR("sscanf return invaild:%zd\n", ret);
		return LCD_KIT_FAIL;
	}
	if (FACT_INFO->oneside_mode.support)
		lcd_kit_oneside_setting(panel_id, mode);
	if (FACT_INFO->color_aod_det.support)
		lcd_kit_aod_detect_setting(panel_id, mode);
	return count;
}
#endif

struct lcd_kit_sysfs_ops g_lcd_sysfs_ops = {
	.check_support = lcd_check_support,
	.model_show = lcd_model_show,
	.type_show = lcd_type_show,
	.panel_info_show = lcd_panel_info_show,
	.amoled_acl_ctrl_show = lcd_amoled_acl_ctrl_show,
	.amoled_acl_ctrl_store = lcd_amoled_acl_ctrl_store,
	.amoled_vr_mode_show = lcd_amoled_vr_mode_show,
	.amoled_vr_mode_store = lcd_amoled_vr_mode_store,
	.effect_color_mode_show = lcd_effect_color_mode_show,
	.effect_color_mode_store = lcd_effect_color_mode_store,
	.reg_read_show = lcd_reg_read_show,
	.reg_read_store = lcd_reg_read_store,
	.gamma_dynamic_store = lcd_gamma_dynamic_store,
	.frame_count_show = lcd_frame_count_show,
	.frame_update_show = lcd_frame_update_show,
	.frame_update_store = lcd_frame_update_store,
	.mipi_dsi_clk_upt_show = lcd_mipi_clk_upt_show,
	.mipi_dsi_clk_upt_store = lcd_mipi_clk_upt_store,
	.fps_scence_show = lcd_fps_scence_show,
	.fps_scence_store = lcd_fps_scence_store,
	.fps_order_show = lcd_fps_order_show,
	.alpm_function_show = lcd_alpm_function_show,
	.alpm_function_store = lcd_alpm_function_store,
	.alpm_setting_show = lcd_alpm_setting_show,
	.alpm_setting_store = lcd_alpm_setting_store,
	.func_switch_show = lcd_func_switch_show,
	.func_switch_store = lcd_func_switch_store,
	.ddic_oem_info_show = lcd_oem_info_show,
	.ddic_oem_info_store = lcd_oem_info_store,
	.panel_sncode_show = lcd_panel_sncode_show,
	.panel_sncode_store = lcd_sys_panel_sncode_store,
	.finger_unlock_show = lcd_finger_unlock_show,
	.finger_unlock_store = lcd_finger_unlock_store,
	.panel_version_show = lcd_panel_version_show,
	.lcd_cabc_mode_show = lcd_kit_cabc_show,
	.lcd_cabc_mode_store = lcd_kit_cabc_store,
#ifdef LCD_FACTORY_MODE
	.ddic_lv_detect_test_show = lcd_ddic_lv_detect_test_show,
	.ddic_lv_detect_test_store = lcd_ddic_lv_detect_test_store,
#endif
	.pre_camera_position_show = lcd_pre_camera_position_show,
};

#ifdef LCD_FACTORY_MODE
struct lcd_fact_ops g_fact_ops = {
	.inversion_mode_show = lcd_inversion_mode_show,
	.inversion_mode_store = lcd_inversion_mode_store,
	.general_test_show = lcd_general_test_show,
	.vtc_line_test_show = lcd_vertical_line_test_show,
	.vtc_line_test_store = lcd_vertical_line_test_store,
	.check_reg_show = lcd_check_reg_show,
	.sleep_ctrl_show = lcd_sleep_ctrl_show,
	.sleep_ctrl_store = lcd_sleep_ctrl_store,
	.poweric_det_show = lcd_poweric_det_show,
	.poweric_det_store = lcd_poweric_det_store,
	.test_config_show = lcd_test_config_show,
	.test_config_store = lcd_test_config_store,
	.pcd_errflag_check_show = lcd_amoled_pcd_errflag_show,
	.pcd_errflag_check_store = lcd_amoled_pcd_errflag_store,
	.oneside_mode_show = lcd_oneside_mode_show,
	.oneside_mode_store = lcd_oneside_mode_store,
	.gram_check_show = lcd_gram_check_show,
	.gram_check_store = lcd_gram_check_store,
	.fpc_det_show = lcd_fpc_det_show,
	.fps_cmd_show = lcd_fps_cmd_show,
};
#endif

int lcd_kit_sysfs_init(void)
{
#ifdef LCD_FACTORY_MODE
	lcd_fact_ops_register(&g_fact_ops);
#endif
	lcd_kit_sysfs_ops_register(&g_lcd_sysfs_ops);
	return LCD_KIT_OK;
}
