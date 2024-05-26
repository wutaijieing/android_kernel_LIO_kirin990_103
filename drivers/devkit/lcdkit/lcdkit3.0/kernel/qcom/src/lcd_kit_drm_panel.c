/*
 * lcd_kit_drm_panel.c
 *
 * lcdkit display function for lcd driver
 *
 * Copyright (c) 2019-2022 Huawei Technologies Co., Ltd.
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
#include <linux/hrtimer.h>
#include "lcd_kit_drm_panel.h"
#ifdef CONFIG_LOG_JANK
#include <huawei_platform/log/hwlog_kernel.h>
#endif
#include <linux/device.h>
#include <linux/backlight.h>
#ifdef CONFIG_QCOM_MODULE_KO
#include "../../common/displayengine/include/lcd_kit_displayengine.h"
#else
#include <lcd_kit_displayengine.h>
#endif
#include <huawei_platform/log/log_jank.h>
#include <linux/completion.h>

#define MAX_DBV 4095

#if defined CONFIG_HUAWEI_DSM
static struct dsm_dev dsm_lcd = {
	.name = "dsm_lcd",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *lcd_dclient = NULL;
#endif

unsigned int esd_recovery_level[MAX_ACTIVE_PANEL] = { 0 };
bool esd_recovery_status[MAX_ACTIVE_PANEL] = { false };
bool esd_recovery_backlight_status[MAX_ACTIVE_PANEL] = { false };
unsigned int fp_recovery_level = 0;
static struct qcom_panel_info lcd_kit_info[MAX_ACTIVE_PANEL] = {0};
static struct lcd_kit_disp_info g_lcd_kit_disp_info[MAX_ACTIVE_PANEL];
static struct poweric_detect_delay poweric_det_delay;
static uint32_t lcd_kit_product_type;
struct completion aod_lp1_done;
struct completion lcd_panel_init_done[MAX_ACTIVE_PANEL];
struct completion first_frame_done;

extern int dsi_panel_set_pinctrl_state(struct dsi_panel *panel, bool enable);
extern int dsi_panel_tx_cmd_set(struct dsi_panel *panel,
				enum dsi_cmd_set_type type);

int lcd_kit_panel_set_ddic_cmd(struct dsi_panel *panel, enum dsi_cmd_set_type type)
{
	int rc = 0;

	LCD_KIT_INFO("+ type:%d\n", type);

	if (!panel) {
		LCD_KIT_ERR("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);
	if (!panel->panel_initialized) {
		LCD_KIT_ERR("Panel not initialized\n");
		mutex_unlock(&panel->panel_lock);
		return LCD_KIT_FAIL;
	}

	rc = dsi_panel_tx_cmd_set(panel, type);
	if (rc)
		LCD_KIT_ERR("[%s] failed to send ddic cmds type:%d, rc=%d\n", panel->name, rc, type);

	mutex_unlock(&panel->panel_lock);
	return rc;
}

void lcd_kit_esd_recover_bl(struct dsi_panel *panel, u32 *bl_lvl)
{
	struct qcom_panel_info *panel_info = NULL;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (!esd_recovery_backlight_status[panel_id]) {
		LCD_KIT_DEBUG("no need recover esd bl\n");
		return;
	}
	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return;
	}
	if (panel_info->bl_after_frame) {
		*bl_lvl = 0;
		complete_all(&lcd_panel_init_done[panel_id]);
		DSI_INFO("panel-%u recovery done\n", panel_id);
	} else {
		DSI_INFO("panel-%u esd recovery backlight %u\n",
			panel_id, esd_recovery_level[panel_id]);
		*bl_lvl = esd_recovery_level[panel_id];
		esd_recovery_backlight_status[panel_id] = false;
		complete_all(&lcd_panel_init_done[panel_id]);
		DSI_INFO("panel-%u recovery done\n", panel_id);
	}
}

void lcd_kit_esd_recover_bl_after_frame(void)
{
	struct qcom_panel_info *pinfo = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return;
	}
	if (!pinfo->bl_after_frame) {
		LCD_KIT_DEBUG("no need set bl after frame\n");
		return;
	}
	if (esd_recovery_backlight_status[panel_id]) {
		esd_recovery_backlight_status[panel_id] = false;
		LCD_KIT_INFO("esd_recovery_level is %u\n", esd_recovery_level[panel_id]);
		dsi_panel_set_backlight(pinfo->display->panel, esd_recovery_level[panel_id]);
	}
}

void lcd_kit_record_frame_number(void)
{
	struct qcom_panel_info *pinfo = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null!\n");
		return;
	}

#ifndef CONFIG_QCOM_PLATFORM_8425
    LCD_KIT_DEBUG("frame_number is %d\n", pinfo->display->frame_number);
    if (pinfo->display->frame_number < MAX_COUNT_FRAME)
    	pinfo->display->frame_number++;
    if (pinfo->display->frame_number == FIRST_FRAME)
    	complete_all(&first_frame_done);
#endif
}

void lcd_kit_esd_recovery_enable(struct dsi_panel *panel)
{
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	esd_recovery_status[panel_id] = true;
	LCD_KIT_INFO("panel_%u esd status enable\n", panel_id);
	/* dual panel simultaneous scenario, panel dead trigger dual panel recovery */
	if (lcd_kit_get_product_type() == LCD_DUAL_PANEL_SIM_DISPLAY_TYPE) {
		panel_id = (panel_id == PRIMARY_PANEL) ? SECONDARY_PANEL : PRIMARY_PANEL;
		if (lcm_get_panel_state(panel_id) == LCD_POWER_STATE_ON) {
			esd_recovery_status[panel_id] = true;
			LCD_KIT_INFO("panel_%u esd status enable\n", panel_id);
		}
	}
}

void lcd_kit_esd_backlight_enable(struct dsi_panel *panel)
{
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (esd_recovery_status[panel_id]) {
		esd_recovery_backlight_status[panel_id] = true;
		esd_recovery_status[panel_id] = false;
		LCD_KIT_INFO("panel_%u esd recovery backlight enable\n", panel_id);
	}
}

void lcd_kit_get_esd_brightness(struct dsi_panel *panel, int *brightness)
{
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (esd_recovery_backlight_status[panel_id]) {
		*brightness = esd_recovery_level[panel_id];
		LCD_KIT_INFO("panel_%u esd brightness is %d\n", panel_id, *brightness);
	}
}

uint32_t lcd_kit_get_current_brightness(uint32_t panel_id)
{
	if (panel_id > MAX_ACTIVE_PANEL) {
		LCD_KIT_ERR("panel_id is wrong %u\n", panel_id);
		return 0;
	}
	return esd_recovery_level[panel_id];
}

int lcd_kit_drm_notifier_register(uint32_t panel_id, struct notifier_block *nb)
{
	struct qcom_panel_info *panel_info = NULL;
	struct drm_panel *drm_panel = NULL;
#ifndef CONFIG_QCOM_PLATFORM_8425
	int ret;
#endif

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!panel_info->display || !panel_info->display->panel) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	drm_panel = &(panel_info->display->panel->drm_panel);
	if (!drm_panel) {
		LCD_KIT_ERR("panel is NULL\n");
		return LCD_KIT_FAIL;
	}
#ifndef CONFIG_QCOM_PLATFORM_8425
	ret = drm_panel_notifier_register(drm_panel, nb);
	return ret;
#else
	return 0;
#endif
}

int lcd_kit_drm_notifier_unregister(uint32_t panel_id, struct notifier_block *nb)
{
	struct qcom_panel_info *panel_info = NULL;
	struct drm_panel *drm_panel = NULL;
#ifndef CONFIG_QCOM_PLATFORM_8425
	int ret;
#endif

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (!panel_info->display || !panel_info->display->panel) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	drm_panel = &(panel_info->display->panel->drm_panel);
	if (!drm_panel) {
		LCD_KIT_ERR("panel is NULL\n");
		return LCD_KIT_FAIL;
	}
#ifndef CONFIG_QCOM_PLATFORM_8425
	ret = drm_panel_notifier_unregister(drm_panel, nb);
	return ret;
#else
	return 0;
#endif
}

struct lcd_kit_disp_info *lcd_kit_get_disp_info(uint32_t panel_id)
{
	if (panel_id >= MAX_ACTIVE_PANEL)
		return &g_lcd_kit_disp_info[PRIMARY_PANEL];
	return &g_lcd_kit_disp_info[panel_id];
}

#if defined CONFIG_HUAWEI_DSM
struct dsm_client *lcd_kit_get_lcd_dsm_client(void)
{
	return lcd_dclient;
}
#endif

struct qcom_panel_info *lcm_get_panel_info(uint32_t panel_id)
{
	if (panel_id >= MAX_ACTIVE_PANEL)
		return &lcd_kit_info[PRIMARY_PANEL];
	return &lcd_kit_info[panel_id];
}

uint32_t lcd_kit_get_product_type(void)
{
	return lcd_kit_product_type;
}

uint32_t lcd_kit_get_current_panel_id(struct dsi_panel *panel)
{
	if (lcd_kit_product_type == LCD_SINGLE_PANEL_TYPE)
		return PRIMARY_PANEL;
	if (!strcmp(panel->type, "primary"))
		return PRIMARY_PANEL;
	else if (!strcmp(panel->type, "secondary"))
		return SECONDARY_PANEL;
	return PRIMARY_PANEL;
}

uint32_t lcd_get_active_panel_id(void)
{
	u32 panel_state;

	if (lcd_kit_product_type == LCD_SINGLE_PANEL_TYPE)
		return PRIMARY_PANEL;

	panel_state = lcm_get_panel_state(PRIMARY_PANEL);
	if (panel_state == LCD_POWER_STATE_ON)
		return PRIMARY_PANEL;

	panel_state = lcm_get_panel_state(SECONDARY_PANEL);
	if (panel_state == LCD_POWER_STATE_ON)
		return SECONDARY_PANEL;

	return PRIMARY_PANEL;
}

int is_mipi_cmd_panel(uint32_t panel_id)
{
	if (panel_id >= MAX_ACTIVE_PANEL) {
		LCD_KIT_INFO("wrong panel_id %d!\n", panel_id);
		return 0;
	}
	if (lcd_kit_info[panel_id].panel_dsi_mode == 0)
		return 1;
	return 0;
}

static int32_t poweric_cmd_detect(uint32_t panel_id, uint8_t *out, int out_len)
{
	uint8_t read_value = 0;
	int32_t ret;
	struct qcom_panel_info *panel_info = NULL;

	panel_info = lcm_get_panel_info(panel_id);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	ret = lcd_kit_dsi_cmds_rx(panel_info->display, &read_value,
		out_len, &disp_info->elvdd_detect.cmds);
	if (ret) {
		LCD_KIT_ERR("mipi rx failed!\n");
		return LCD_KIT_OK;
	}
	if ((read_value & disp_info->elvdd_detect.exp_value_mask) !=
		disp_info->elvdd_detect.exp_value)
		ret = LCD_KIT_FAIL;
	*out = read_value;

	LCD_KIT_INFO("read_value = 0x%x, exp_value = 0x%x, mask = 0x%x\n",
		read_value,
		disp_info->elvdd_detect.exp_value,
		disp_info->elvdd_detect.exp_value_mask);
	return ret;
}

static int32_t poweric_gpio_detect(uint32_t panel_id, int32_t *out)
{
	int32_t gpio_value;
	int32_t ret;
	uint32_t detect_gpio;
	struct qcom_panel_info *plcd_kit_info = NULL;

	detect_gpio = disp_info->elvdd_detect.detect_gpio;
	if (!gpio_is_valid(detect_gpio)) {
		LCD_KIT_ERR("gpio invalid, gpio=%d!\n", detect_gpio);
		return LCD_KIT_OK;
	}
	plcd_kit_info = lcm_get_panel_info(panel_id);
	if (plcd_kit_info)
		detect_gpio += plcd_kit_info->gpio_offset;
	ret = gpio_request(detect_gpio, "lcm_dsi_elvdd");
	if (ret != 0) {
		LCD_KIT_ERR("pcd_gpio %d request fail!ret = %d\n",
			detect_gpio, ret);
		return LCD_KIT_OK;
	}
	ret = gpio_direction_input(detect_gpio);
	if (ret != 0) {
		gpio_free(detect_gpio);
		LCD_KIT_ERR("pcd_gpio %d direction set fail!\n", detect_gpio);
		return LCD_KIT_OK;
	}
	gpio_value = gpio_get_value(detect_gpio);
	gpio_free(detect_gpio);
	if (gpio_value != disp_info->elvdd_detect.exp_value)
		ret = LCD_KIT_FAIL;
	*out = gpio_value;
	LCD_KIT_INFO("pcd_gpio %d elvdd value = %d  exp_value = 0x%x\n",
		detect_gpio, gpio_value, disp_info->elvdd_detect.exp_value);
	return ret;
}

static void report_poweric_err(uint32_t panel_id, uint8_t cmd_val, int32_t gpio_val)
{
#if defined CONFIG_HUAWEI_DSM
	int8_t record_buf[DMD_RECORD_BUF_LEN] = {'\0'};
	int32_t recordtime = 0;
	int32_t ret;

	ret = snprintf(record_buf, DMD_RECORD_BUF_LEN,
		"elvdd: detect_type = 0x%x, cmd_val = 0x%x, gpio_val = %d\n",
		disp_info->elvdd_detect.detect_type, cmd_val, gpio_val);
	if (ret < 0)
		LCD_KIT_ERR("snprintf happened error!\n");
	(void)lcd_dsm_client_record(lcd_dclient, record_buf,
		DSM_LCD_STATUS_ERROR_NO,
		REC_DMD_NO_LIMIT,
		&recordtime);
#endif
}

static void del_poweric_timer(uint32_t panel_id)
{
	LCD_KIT_INFO("+\n");
	if (disp_info->elvdd_detect.is_start_delay_timer == false)
		return;
	LCD_KIT_INFO("delete elvdd detect delay timer and wq\n");
	cancel_work_sync(&(poweric_det_delay.wq));
	hrtimer_cancel(&poweric_det_delay.timer);
	disp_info->elvdd_detect.is_start_delay_timer = false;
	LCD_KIT_INFO("-\n");
}

static void poweric_wq_handler(struct work_struct *work)
{
	static int32_t retry_times = 0;
	int32_t ret = 0;
	uint8_t cmd_val = 0;
	int32_t gpio_val = 0;
	struct poweric_detect_delay *detect = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	LCD_KIT_INFO("+\n");
	detect = container_of(work, struct poweric_detect_delay, wq);
	if (detect == NULL) {
		LCD_KIT_ERR("detect is NULL\n");
		return;
	}
	if (disp_info->elvdd_detect.detect_type == ELVDD_MIPI_CHECK_MODE)
		ret = poweric_cmd_detect(panel_id, &cmd_val, 1);
	else if (disp_info->elvdd_detect.detect_type == ELVDD_GPIO_CHECK_MODE)
		ret = poweric_gpio_detect(panel_id, &gpio_val);
	if (ret) {
		LCD_KIT_ERR("detect poweric abnormal, recovery lcd\n");
		report_poweric_err(panel_id, cmd_val, gpio_val);
		if (retry_times >= RECOVERY_TIMES) {
			LCD_KIT_WARNING("not need recovery, recovery num:%d\n",
				retry_times);
			retry_times = 0;
			del_poweric_timer(panel_id);
			return;
		}
		lcd_kit_recovery_display(panel_id, LCD_ASYNC_MODE);
		retry_times++;
		del_poweric_timer(panel_id);
		return;
	}
	retry_times = 0;
	del_poweric_timer(panel_id);
	LCD_KIT_INFO("detect poweric normal\n");
}

static enum hrtimer_restart poweric_timer_fun(struct hrtimer *arg)
{
	struct poweric_detect_delay *detect =
		(struct poweric_detect_delay *)arg;
	LCD_KIT_INFO("+\n");
	schedule_work(&(detect->wq));
	return HRTIMER_NORESTART;
}

void poweric_detect_delay(uint32_t panel_id)
{
	LCD_KIT_INFO("+\n");
	if (disp_info->elvdd_detect.is_start_delay_timer == false) {
		LCD_KIT_INFO("init elvdd detect delay timer\n");
		INIT_WORK(&(poweric_det_delay.wq), poweric_wq_handler);
		hrtimer_init(&poweric_det_delay.timer,
			CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		LCD_KIT_INFO("disp_info->elvdd_detect.delay = %d\n", disp_info->elvdd_detect.delay);
		poweric_det_delay.timer._softexpires = jiffies +
			disp_info->elvdd_detect.delay * HZ / 1000;
		poweric_det_delay.timer.function = poweric_timer_fun;
		hrtimer_start(&poweric_det_delay.timer,
			ktime_set(disp_info->elvdd_detect.delay / 1000, // chang ms to s
			(disp_info->elvdd_detect.delay % 1000) * 1000000), // change ms to ns
			HRTIMER_MODE_REL);
		disp_info->elvdd_detect.is_start_delay_timer = true;
	}
	LCD_KIT_INFO("-\n");
}

static void lcd_kit_poweric_detect(uint32_t panel_id)
{
	static int32_t retry_times;
	int32_t ret = 0;
	uint8_t cmd_val = 0;
	int32_t gpio_val = 0;

	if (!disp_info->elvdd_detect.support) {
		LCD_KIT_INFO("elvdd begin\n");
		return;
	}
	if (disp_info->elvdd_detect.delay) {
		LCD_KIT_INFO("elvdd delay\n");
		poweric_detect_delay(panel_id);
		return;
	}
	if (disp_info->elvdd_detect.detect_type == ELVDD_MIPI_CHECK_MODE)
		ret = poweric_cmd_detect(panel_id, &cmd_val, 1);
	else if (disp_info->elvdd_detect.detect_type == ELVDD_GPIO_CHECK_MODE)
		ret = poweric_gpio_detect(panel_id, &gpio_val);
	if (ret) {
		LCD_KIT_ERR("detect poweric abnormal, recovery lcd\n");
		report_poweric_err(panel_id, cmd_val, gpio_val);
		if (retry_times >= RECOVERY_TIMES) {
			LCD_KIT_WARNING("not need recovery, recovery num:%d\n",
				retry_times);
			retry_times = 0;
			return;
		}
		lcd_kit_recovery_display(panel_id, LCD_ASYNC_MODE);
		retry_times++;
		return;
	}
	retry_times = 0;
	LCD_KIT_INFO("detect poweric normal\n");
}

static bool lcd_kit_first_screenon(uint32_t last_bl_level, uint32_t bl_level)
{
	bool ret = false;

	if (last_bl_level == 0 && bl_level != 0) {
		LCD_KIT_INFO("first_screenon, bl_level = %d", bl_level);
		ret = true;
		boost_5v_power_enable();
	} else {
		ret = false;
	}
	last_bl_level = bl_level;
	return ret;
}

int  lcd_kit_bl_ic_set_backlight(unsigned int bl_level)
{
	struct lcd_kit_bl_ops *bl_ops = NULL;
#ifdef CONFIG_LOG_JANK
	static uint32_t jank_last_bl_level;
	if ((jank_last_bl_level == 0) && (bl_level != 0)) {
		LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON, "LCD_BACKLIGHT_ON,%u", bl_level);
		jank_last_bl_level = bl_level;
	} else if ((bl_level == 0) && (jank_last_bl_level != 0)) {
		LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_OFF, "LCD_BACKLIGHT_OFF");
		jank_last_bl_level = bl_level;
	}
#endif
	bl_ops = lcd_kit_get_bl_ops();
	if (!bl_ops) {
		LCD_KIT_INFO("bl_ops is null!\n");
		return LCD_KIT_FAIL;
	}
	if (bl_ops->set_backlight) {
		if (bl_level > 0)
			esd_recovery_level[PRIMARY_PANEL] = bl_level;
		bl_ops->set_backlight(bl_level);
	}
	return LCD_KIT_OK;
}

int lcd_kit_dsi_panel_update_backlight(struct dsi_panel *panel,
	unsigned int level)
{
	ssize_t ret = 0;
	struct mipi_dsi_device *dsi = NULL;
	unsigned char bl_tb_short[] = { 0xFF };
	unsigned char bl_tb_long[] = { 0xFF, 0xFF };
	static uint32_t jank_last_bl_level;
	bool first_screenon = false;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);
	int tuned_level = level;
	unsigned long mode_flags = 0;

	LCD_KIT_INFO("+, in_level [%u]", level);
	first_screenon = lcd_kit_first_screenon(jank_last_bl_level, level);
	if (level > 0)
		esd_recovery_level[panel_id] = level;
	if ((jank_last_bl_level == 0) && (level != 0)) {
		LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON, "LCD_BACKLIGHT_ON,%u", level);
		jank_last_bl_level = level;
	} else if ((level == 0) && (jank_last_bl_level != 0)) {
		LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_OFF, "LCD_BACKLIGHT_OFF");
		jank_last_bl_level = level;
	}
	if (first_screenon)
		lcd_kit_poweric_detect(panel_id);

	if (!panel || (level > 0xffff)) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}
#ifndef QCOM_PLATFORM_6225
	tuned_level = display_engine_brightness_get_mapped_level(level, panel_id);
#else
	tuned_level = level;
#endif
	fp_recovery_level = tuned_level;
	if (common_info->hbm.hbm_if_fp_is_using == 1) {
		LCD_KIT_INFO("fingerprint is using, backlight can not set!\n");
		return -EINVAL;
	}
	if (common_info->backlight.bl_cmd.cmds == NULL) {
		LCD_KIT_ERR("invalid cmds\n");
		return -EINVAL;
	}
	if (panel->power_mode == SDE_MODE_DPMS_LP1) {
		LCD_KIT_ERR("in aod state, return!\n");
		return ret;
	}
	dsi = &panel->mipi_device;
	if (common_info->backlight.bl_cmd.cmds == NULL) {
		LCD_KIT_ERR("invalid cmds\n");
		return -EINVAL;
	}
#ifndef QCOM_PLATFORM_6225
	display_engine_compensation_set_dbv(level, tuned_level, panel_id);
#endif
	switch (common_info->backlight.order) {
	case BL_BIG_ENDIAN:
		if (common_info->backlight.bl_max <= 0xFF) {
			common_info->backlight.bl_cmd.cmds[0].payload[1] = tuned_level;
		} else {
			/* change bl level to dsi cmds */
			common_info->backlight.bl_cmd.cmds[0].payload[1] =
				((tuned_level >> 8) & 0xFF) | common_info->backlight.write_offset;
			common_info->backlight.bl_cmd.cmds[0].payload[2] = tuned_level & 0xFF;
		}
		break;
	case BL_LITTLE_ENDIAN:
		if (common_info->backlight.bl_max <= 0xFF) {
			common_info->backlight.bl_cmd.cmds[0].payload[1] = tuned_level;
		} else {
			/* change bl level to dsi cmds */
			common_info->backlight.bl_cmd.cmds[0].payload[1] = tuned_level & 0xFF;
			common_info->backlight.bl_cmd.cmds[0].payload[2] =
				((tuned_level >> 8) & 0xFF) | common_info->backlight.write_offset;
		}
		break;
	default:
		LCD_KIT_ERR("not support order\n");
		break;
	}
	if (!common_info->backlight.bl_cmd.link_state) {
		mode_flags = dsi->mode_flags;
		dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	}
	if(common_info->backlight.bl_max <= 0xFF) {
		bl_tb_short[0] = common_info->backlight.bl_cmd.cmds[0].payload[1];
		ret = mipi_dsi_dcs_write(dsi,
			common_info->backlight.bl_cmd.cmds[0].payload[0],
			bl_tb_short, sizeof(bl_tb_short));
	} else {
		bl_tb_long[0] = common_info->backlight.bl_cmd.cmds[0].payload[1];
		bl_tb_long[1] = common_info->backlight.bl_cmd.cmds[0].payload[2];
		ret = mipi_dsi_dcs_write(dsi,
			common_info->backlight.bl_cmd.cmds[0].payload[0],
			bl_tb_long, sizeof(bl_tb_long));
	}
	if (common_info->backlight.need_sync)
		ret = mipi_dsi_dcs_write(dsi, 0x6C, NULL, 0);
	if (!common_info->backlight.bl_cmd.link_state)
		dsi->mode_flags = mode_flags;
	if (ret < 0)
		LCD_KIT_ERR("failed to update dcs backlight:%d\n", tuned_level);
	LCD_KIT_INFO("-, tuned_level [%u]", tuned_level);
	return ret;
}

static void lcd_kit_set_thp_proximity_state(uint32_t panel_id, int power_state)
{
	if (!common_info->thp_proximity.support) {
		LCD_KIT_INFO("thp_proximity not support!\n");
		return;
	}
	common_info->thp_proximity.panel_power_state = power_state;
}

static void lcd_kit_set_thp_proximity_sem(uint32_t panel_id, bool sem_lock)
{
	if (!common_info->thp_proximity.support) {
		LCD_KIT_INFO("thp_proximity not support!\n");
		return;
	}
	if (sem_lock == true)
		down(&disp_info->thp_second_poweroff_sem);
	else
		up(&disp_info->thp_second_poweroff_sem);
}

void lcm_set_panel_state(uint32_t panel_id, unsigned int state)
{
	if (panel_id >= MAX_ACTIVE_PANEL) {
		LCD_KIT_INFO("wrong panel_id %d!\n", panel_id);
		return;
	}
	lcd_kit_info[panel_id].panel_state = state;
}

unsigned int lcm_get_panel_state(uint32_t panel_id)
{
	if (panel_id >= MAX_ACTIVE_PANEL) {
		LCD_KIT_INFO("wrong panel_id %d!\n", panel_id);
		return LCD_POWER_STATE_OFF;
	}
	return lcd_kit_info[panel_id].panel_state;
}

unsigned int lcm_get_panel_backlight_max_level(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	return lcd_kit_info[panel_id].bl_max;
}

int lcm_rgbw_mode_set_param(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int ret = LCD_KIT_OK;
	struct display_engine_ddic_rgbw_param *param = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (dev == NULL) {
		LCD_KIT_ERR("dev is null\n");
		return LCD_KIT_FAIL;
	}
	if (data == NULL) {
		LCD_KIT_ERR("data is null\n");
		return LCD_KIT_FAIL;
	}
	param = (struct display_engine_ddic_rgbw_param *)data;
	memcpy(&g_lcd_kit_disp_info[panel_id].ddic_rgbw_param, param, sizeof(*param));
	ret = lcd_kit_rgbw_set_handle(panel_id);
	if (ret < 0)
		LCD_KIT_ERR("set rgbw fail\n");
	return ret;
}

int lcm_rgbw_mode_get_param(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int ret = LCD_KIT_OK;
	struct display_engine_ddic_rgbw_param *param = NULL;

	if (dev == NULL) {
		LCD_KIT_ERR("dev is null\n");
		return LCD_KIT_FAIL;
	}
	if (data == NULL) {
		LCD_KIT_ERR("data is null\n");
		return LCD_KIT_FAIL;
	}
	param = (struct display_engine_ddic_rgbw_param *)data;
	param->ddic_panel_id = g_lcd_kit_disp_info[PRIMARY_PANEL].ddic_rgbw_param.ddic_panel_id;
	param->ddic_rgbw_mode = g_lcd_kit_disp_info[PRIMARY_PANEL].ddic_rgbw_param.ddic_rgbw_mode;
	param->ddic_rgbw_backlight = g_lcd_kit_disp_info[PRIMARY_PANEL].ddic_rgbw_param.ddic_rgbw_backlight;
	param->pixel_gain_limit = g_lcd_kit_disp_info[PRIMARY_PANEL].ddic_rgbw_param.pixel_gain_limit;

	LCD_KIT_INFO("get RGBW parameters success\n");
	return ret;
}

int lcm_display_engine_get_panel_info(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int ret = LCD_KIT_OK;
	struct display_engine_panel_info_param *param = NULL;

	if (dev == NULL) {
		LCD_KIT_ERR("dev is null\n");
		return LCD_KIT_FAIL;
	}
	if (data == NULL) {
		LCD_KIT_ERR("data is null\n");
		return LCD_KIT_FAIL;
	}
	param = (struct display_engine_panel_info_param *)data;
	param->width = lcd_kit_info[PRIMARY_PANEL].xres;
	param->height = lcd_kit_info[PRIMARY_PANEL].yres;
	param->maxluminance = lcd_kit_info[PRIMARY_PANEL].maxluminance;
	param->minluminance = lcd_kit_info[PRIMARY_PANEL].minluminance;
	param->maxbacklight = lcd_kit_info[PRIMARY_PANEL].bl_max;
	param->minbacklight = lcd_kit_info[PRIMARY_PANEL].bl_min;

	LCD_KIT_INFO("get panel info parameters success\n");
	return ret;
}

int lcm_display_engine_init(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int ret = LCD_KIT_OK;
	struct display_engine *param = NULL;

	if (dev == NULL) {
		LCD_KIT_ERR("dev is null\n");
		return LCD_KIT_FAIL;
	}
	if (data == NULL) {
		LCD_KIT_ERR("data is null\n");
		return LCD_KIT_FAIL;
	}
	param = (struct display_engine *)data;
	/* 0:no support  1:support */
	if (g_lcd_kit_disp_info[PRIMARY_PANEL].rgbw.support == 0)
		param->ddic_rgbw_support = 0;
	else
		param->ddic_rgbw_support = 1;

	LCD_KIT_INFO("display engine init success\n");
	return ret;
}

static int lcd_kit_dsi_panel_pre_power_on(struct dsi_panel *panel)
{
	int ret = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return ret;
	}
	ret = dsi_panel_set_pinctrl_state(panel, true);
	if (ret)
		LCD_KIT_ERR("[%s] failed to set pinctrl, rc=%d\n", panel->name, ret);
	if (common_ops->panel_pre_power_on)
		ret = common_ops->panel_pre_power_on(panel_id, panel);

	return ret;
}

static int lcd_kit_dsi_panel_power_on(struct dsi_panel *panel)
{
	int ret = 0;
	char *panel_name = NULL;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return ret;
	}
	lcd_kit_set_thp_proximity_sem(panel_id, true);
	ret = dsi_panel_set_pinctrl_state(panel, true);
	if (ret)
		LCD_KIT_ERR("[%s] failed to set pinctrl, rc=%d\n", panel->name, ret);

	panel_name = common_info->panel_model != NULL ?
		common_info->panel_model : common_info->panel_name;
	if (panel_name)
		LCD_KIT_INFO("lcd_name is %s\n", panel_name);

	if (common_ops->panel_power_on)
		ret = common_ops->panel_power_on(panel_id, panel);
	lcm_set_panel_state(panel_id, LCD_POWER_STATE_ON);
	lcd_kit_set_thp_proximity_state(panel_id, POWER_ON);
	lcd_kit_set_thp_proximity_sem(panel_id, false);
	return ret;
}

static int lcd_kit_dsi_panel_on_lp(struct dsi_panel *panel)
{
	int ret = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return ret;
	}

	if (common_ops->panel_on_lp)
		ret = common_ops->panel_on_lp(panel_id, panel);

	return ret;
}

static int lcd_kit_dsi_panel_on_hs(struct dsi_panel *panel)
{
	int ret = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return ret;
	}

	if (common_ops->panel_on_hs)
		ret = common_ops->panel_on_hs(panel_id, panel);

	return ret;
}

static int lcd_kit_dsi_panel_power_off(struct dsi_panel *panel)
{
	int ret = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return ret;
	}
	lcd_kit_set_thp_proximity_sem(panel_id, true);
	if (common_ops->panel_power_off)
		common_ops->panel_power_off(panel_id, panel);
	ret = dsi_panel_set_pinctrl_state(panel, false);
	if (ret)
		LCD_KIT_ERR("[%s] failed set pinctrl state, rc=%d\n", panel->name,
			ret);
	lcm_set_panel_state(panel_id, LCD_POWER_STATE_OFF);
	lcd_kit_set_thp_proximity_state(panel_id, POWER_OFF);
	lcd_kit_set_thp_proximity_sem(panel_id, false);
#ifdef CONFIG_LOG_JANK
	LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "LCD_POWER_OFF");
#endif
	return ret;
}

static int lcd_kit_dsi_panel_off_lp(struct dsi_panel *panel)
{
	int ret = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return ret;
	}

	if (common_ops->panel_off_lp)
		ret = common_ops->panel_off_lp(panel_id, panel);

	return ret;
}

static int lcd_kit_dsi_panel_off_hs(struct dsi_panel *panel)
{
	int ret = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return LCD_KIT_FAIL;
	}

	if (common_ops->panel_off_hs)
		ret = common_ops->panel_off_hs(panel_id, panel);
	return ret;
}

int lcd_kit_panel_pre_prepare(struct dsi_panel *panel)
{
	int rc = 0;

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);
	rc = lcd_kit_dsi_panel_pre_power_on(panel);
	if (rc) {
		LCD_KIT_ERR("[%s] panel power on failed, rc=%d\n", panel->name, rc);
		goto error;
	}

	/* If LP11_INIT is set, panel will be powered up during prepare() */
	if (panel->lp11_init)
		goto error;
	rc = lcd_kit_dsi_panel_power_on(panel);
	if (rc) {
		LCD_KIT_ERR("[%s] panel power on failed, rc=%d\n", panel->name, rc);
		goto error;
	}

error:
	mutex_unlock(&panel->panel_lock);
	LCD_KIT_INFO("exit\n");
	return rc;
}

int lcd_kit_panel_prepare(struct dsi_panel *panel)
{
	int rc = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (panel->lp11_init) {
		rc = lcd_kit_dsi_panel_power_on(panel);
		if (rc) {
			LCD_KIT_ERR("[%s] panel power on failed, rc=%d\n",
			       panel->name, rc);
			goto error;
		}
	}

	if (common_info->three_stage_poweron) {
		rc = lcd_kit_dsi_panel_on_lp(panel);
		if (rc != LCD_KIT_OK)
			LCD_KIT_ERR("panel on lp failed\n");
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_PRE_ON);
	if (rc) {
		LCD_KIT_ERR("[%s] failed to send DSI_CMD_SET_PRE_ON cmds, rc=%d\n",
		       panel->name, rc);
		goto error;
	}
#ifdef LCD_KIT_DEBUG_ENABLE
	lcd_kit_dbg_init();
	LCD_KIT_INFO("enter lcd_kit_dbg_init\n");
#endif
error:
	mutex_unlock(&panel->panel_lock);
	LCD_KIT_INFO("exit\n");
	return rc;
}

int lcd_kit_panel_enable(struct dsi_panel *panel)
{
	int rc = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);
	lcd_kit_proxmity_proc(panel_id, LCD_RESET_HIGH);
	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ON);
	if (rc)
		LCD_KIT_ERR("[%s] failed to send DSI_CMD_SET_ON cmds, rc=%d\n",
		       panel->name, rc);
	else
		panel->panel_initialized = true;
	/* record panel on time */
	if (disp_info->quickly_sleep_out.support)
		lcd_kit_disp_on_record_time(panel_id);

	if (common_info->three_stage_poweron) {
		rc = lcd_kit_dsi_panel_on_hs(panel);
		if (rc != LCD_KIT_OK)
			LCD_KIT_ERR("panel on hs failed\n");
	} else {
		rc = lcd_kit_dsi_panel_on_lp(panel);
		if (rc != LCD_KIT_OK)
			LCD_KIT_ERR("panel on lp failed\n");
	}

	mutex_unlock(&panel->panel_lock);
	lcd_kit_start_check_hrtimer(panel_id);
#ifdef CONFIG_LOG_JANK
	LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "LCD_POWER_ON");
#endif
#ifndef QCOM_PLATFORM_6225
	display_engine_panel_on(panel_id);
#endif
	LCD_KIT_INFO("exit\n");
	return rc;
}

void lcd_kit_panel_post_enable(struct dsi_display *display)
{
	u32 panel_id;
	struct lcd_kit_check_reg_dsm *check_reg_dsm = NULL;

	if (lcd_panel_sncode_store(display) != 0)
		LCD_KIT_ERR("lcd panel sncode store failed!\n");

	panel_id = lcd_kit_get_current_panel_id(display->panel);

	if ((display->panel->power_mode == SDE_MODE_DPMS_LP1) ||
		(display->panel->power_mode == SDE_MODE_DPMS_LP2))
		check_reg_dsm = &common_info->check_aod_reg_on;
	else
		check_reg_dsm = &common_info->check_reg_on;

	(void)lcd_kit_check_reg_report_dsm(panel_id, display,
		check_reg_dsm);

#ifndef QCOM_PLATFORM_6225
	display_engine_hbm_gamma_read(panel_id, display);
#endif
}

int lcd_kit_panel_pre_disable(struct dsi_panel *panel)
{
	int rc = 0;

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	rc = lcd_kit_dsi_panel_off_hs(panel);
	if (rc != LCD_KIT_OK)
		LCD_KIT_INFO("panel off hs failed\n");

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_PRE_OFF);
	if (rc) {
		LCD_KIT_ERR("[%s] failed to send DSI_CMD_SET_PRE_OFF cmds, rc=%d\n",
		       panel->name, rc);
		goto error;
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int lcd_kit_panel_disable(struct dsi_panel *panel)
{
	int rc = 0;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}

	lcd_kit_stop_check_hrtimer(panel_id);

	mutex_lock(&panel->panel_lock);

	/* Avoid sending panel off commands when ESD recovery is underway */
	if (!atomic_read(&panel->esd_recovery_pending)) {
		/*
		 * Need to set IBB/AB regulator mode to STANDBY,
		 * if panel is going off from AOD mode.
		 */
		if (dsi_panel_is_type_oled(panel) &&
			(panel->power_mode == SDE_MODE_DPMS_LP1 ||
			panel->power_mode == SDE_MODE_DPMS_LP2))
			dsi_pwr_panel_regulator_mode_set(&panel->power_info,
				"ibb", REGULATOR_MODE_STANDBY);
		rc = lcd_kit_dsi_panel_off_lp(panel);
		if (rc != LCD_KIT_OK)
			LCD_KIT_ERR("panel off lp failed\n");
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_OFF);
		if (rc) {
			/*
			 * Sending panel off commands may fail when  DSI
			 * controller is in a bad state. These failures can be
			 * ignored since controller will go for full reset on
			 * subsequent display enable anyway.
			 */
			pr_warn_ratelimited("[%s] failed to send DSI_CMD_SET_OFF cmds, rc=%d\n",
					panel->name, rc);
			rc = 0;
		}
	}
	panel->panel_initialized = false;
	panel->power_mode = SDE_MODE_DPMS_OFF;

	mutex_unlock(&panel->panel_lock);
	LCD_KIT_INFO("exit\n");
	return rc;
}

int lcd_kit_panel_unprepare(struct dsi_panel *panel)
{
	int rc = 0;

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);
	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_POST_OFF);
	if (rc)
		LCD_KIT_ERR("[%s] failed to send DSI_CMD_SET_POST_OFF cmds, rc=%d\n",
		       panel->name, rc);
	mutex_unlock(&panel->panel_lock);
#ifdef CONFIG_LOG_JANK
	LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "LCD_POWER_OFF");
#endif
	LCD_KIT_INFO("exit\n");
	return rc;
}

int lcd_kit_panel_post_unprepare(struct dsi_panel *panel)
{
	int rc = 0;

	LCD_KIT_INFO("enter\n");
	if (!panel) {
		LCD_KIT_ERR("invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);
	rc = lcd_kit_dsi_panel_power_off(panel);
	if (rc)
		LCD_KIT_ERR("[%s] panel power_Off failed, rc=%d\n",
		       panel->name, rc);
	mutex_unlock(&panel->panel_lock);
	LCD_KIT_INFO("exit\n");
	return rc;
}

static void lcd_kit_completion_init(void)
{
	int i;
	static bool completion_init_flag = false;

	if (completion_init_flag)
		return;

	init_completion(&aod_lp1_done);
	init_completion(&first_frame_done);
	for (i = 0; i< MAX_ACTIVE_PANEL; i++)
		init_completion(&lcd_panel_init_done[i]);
	completion_init_flag = true;
}

int lcd_kit_init(struct dsi_panel *panel)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_kit_get_current_panel_id(panel);
	struct device_node *np = NULL;

	LCD_KIT_INFO("enter\n");
	if (!lcd_kit_support()) {
		LCD_KIT_INFO("not lcd_kit driver and return\n");
		return ret;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_KIT_PANEL_TYPE);
	if (np == NULL) {
		LCD_KIT_ERR("not find device node %s\n", DTS_COMP_LCD_KIT_PANEL_TYPE);
		return LCD_KIT_FAIL;
	}

	OF_PROPERTY_READ_U32_RETURN(np, "product_id", &disp_info->product_id);
	OF_PROPERTY_READ_U32_RETURN(np, "product_type", &lcd_kit_product_type);
	LCD_KIT_INFO("product_id = %d\n", disp_info->product_id);
	LCD_KIT_INFO("product_type = %d\n", lcd_kit_product_type);

	if (panel == NULL) {
		LCD_KIT_ERR("panel is null\n");
		return LCD_KIT_FAIL;
	}

	if (panel->panel_of_node == NULL) {
		LCD_KIT_ERR("not found device node\n");
		return LCD_KIT_FAIL;
	}
#if defined CONFIG_HUAWEI_DSM
	if (lcd_dclient == NULL)
		lcd_dclient = dsm_register_client(&dsm_lcd);
#endif
	/* completion init */
	lcd_kit_completion_init();
	/* adapt init */
	lcd_kit_adapt_init();
	/* common init */
	if (common_ops->common_init)
		common_ops->common_init(panel_id, panel->panel_of_node);
	/* utils init */
	lcd_kit_utils_init(panel_id, panel->panel_of_node, &lcd_kit_info[panel_id]);
	/* init fnode */
	lcd_kit_sysfs_init();
	/* power init */
	lcd_kit_power_init(panel_id, panel);
	/* init panel ops */
	lcd_kit_panel_init(panel_id);
	/* get lcd max brightness */
	lcd_kit_get_bl_max_nit_from_dts(panel_id);
	lcm_set_panel_state(panel_id, LCD_POWER_STATE_ON);
	lcd_kit_set_thp_proximity_state(panel_id, POWER_ON);
#ifndef QCOM_PLATFORM_6225
	display_engine_panel_on(panel_id);
#endif
	LCD_KIT_INFO("exit\n");
	return ret;
}

int panel_drm_hbm_set(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int mmi_level = -1;
	int backlight_type = -1;
	int ret = LCD_KIT_OK;
	struct qcom_panel_info *panel_info = NULL;

	if (!data) {
		LCD_KIT_ERR("data is NULL\n");
		return LCD_KIT_FAIL;
	}
	panel_info = lcm_get_panel_info(PRIMARY_PANEL);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	mmi_level = *(int *)data;
	if (mmi_level == 0) {
		backlight_type = BACKLIGHT_LOW_LEVEL;
		mmi_level = fp_recovery_level;
#ifndef QCOM_PLATFORM_6225
		display_engine_brightness_set_alpha_bypass(false);
#endif
	} else {
		backlight_type = BACKLIGHT_HIGH_LEVEL;
#ifndef QCOM_PLATFORM_6225
		display_engine_brightness_set_alpha_bypass(true);
#endif
	}
	LCD_KIT_INFO("mmi_level %d, type %d\n", mmi_level, backlight_type);
#ifndef QCOM_PLATFORM_6225
	display_engine_set_fp_backlight(PRIMARY_PANEL, panel_info->display, mmi_level,
		backlight_type);
#endif
	return ret;
}

void lcd_kit_set_fold_info(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();

	if (!common_info->fold_info.support)
		return;
	common_info->fold_info.crtc_h = 0;
}

void lcd_kit_crtc_info_store(struct drm_plane *plane)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	unsigned int panel_state;

	if (!common_info->fold_info.support)
		return;
	if (plane == NULL || plane->state == NULL) {
		LCD_KIT_ERR("NULL pointer\n");
		return;
	}
	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_INFO("panel_state is power off\n");
		return;
	}
	if ((plane->state->crtc_y + plane->state->crtc_h) > common_info->fold_info.crtc_h)
		common_info->fold_info.crtc_h = (plane->state->crtc_y + plane->state->crtc_h);
}

void lcd_kit_fold_lp_handle(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();
	struct qcom_panel_info *pinfo = NULL;
	struct dsi_display *display = NULL;
	unsigned int panel_state;

	if (!common_info->fold_info.support)
		return;
	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is NULL\n");
		return;
	}
	display = pinfo->display;
	if (!display) {
		LCD_KIT_ERR("display is NULL\n");
		return;
	}
	panel_state = lcm_get_panel_state(panel_id);
	if (!panel_state) {
		LCD_KIT_INFO("panel_state is power off\n");
		return;
	}
	if (!display->panel) {
		LCD_KIT_ERR("display->panel is NULL\n");
		return;
	}
	if (display->panel->power_mode != SDE_MODE_DPMS_ON &&
		display->panel->power_mode != SDE_MODE_DPMS_OFF) {
		LCD_KIT_DEBUG("not in on mode\n");
		return;
	}
#ifndef CONFIG_QCOM_PLATFORM_8425
	if (common_info->fold_info.crtc_h == 0)
		common_info->fold_info.crtc_h = common_info->fold_info.last_crtc_h;
	if (common_info->fold_info.last_crtc_h > common_info->fold_info.fold_height &&
		common_info->fold_info.crtc_h == common_info->fold_info.fold_height) {
		LCD_KIT_INFO("switch to fold\n");
		common_info->fold_info.fold_status = FOLD_STATUS;
		(void)dsi_panel_switch_to_fold(display->panel);
	}
	if (common_info->fold_info.last_crtc_h <= common_info->fold_info.fold_height &&
		common_info->fold_info.crtc_h > common_info->fold_info.fold_height) {
		LCD_KIT_INFO("switch to unfold\n");
		common_info->fold_info.fold_status = UNFOLD_STATUS;
		(void)dsi_panel_switch_to_unfold(display->panel);
	}
	common_info->fold_info.last_crtc_h = common_info->fold_info.crtc_h;
	if ((common_info->fold_info.crtc_h == common_info->fold_info.fold_height) &&
		pinfo->display->frame_number == FIRST_FRAME &&
		((pinfo->display->panel->power_mode == SDE_MODE_DPMS_ON) ||
		(pinfo->display->panel->power_mode == SDE_MODE_DPMS_LP1))) {
		LCD_KIT_INFO("first frame switch to fold\n");
		common_info->fold_info.fold_status = FOLD_STATUS;
		dsi_panel_switch_to_fold(pinfo->display->panel);
	}
#endif
}

int panel_drm_hbm_set_for_fingerprint(bool is_enable)
{
	int ret = LCD_KIT_OK;
	int backlight_type = -1;
	struct qcom_panel_info *panel_info = NULL;
#ifndef QCOM_PLATFORM_6225
#ifdef CONFIG_QCOM_PLATFORM_8425
	int hbm_level = display_engine_get_hbm_level(0);
#else
	int hbm_level = display_engine_get_hbm_level(DISPLAY_ENGINE_PANEL_INNER);
#endif
	if (hbm_level < 0) {
		LCD_KIT_ERR("invalid hbm_level, hbm_level %d\n", hbm_level);
		return LCD_KIT_FAIL;
	}
#else
	int hbm_level = 0;
#endif
	panel_info = lcm_get_panel_info(PRIMARY_PANEL);
	if (!panel_info) {
		LCD_KIT_ERR("panel_info is NULL\n");
		return LCD_KIT_FAIL;
	}
	if (panel_info->cmd_async_support)
		panel_info->cmd_async = true;
	if (is_enable) {
		backlight_type = BACKLIGHT_HIGH_LEVEL;
	} else {
		backlight_type = BACKLIGHT_LOW_LEVEL;
		hbm_level = fp_recovery_level;
	}
	LCD_KIT_INFO("queue work hbm_level %d, type %d\n", hbm_level, backlight_type);
#ifndef QCOM_PLATFORM_6225
	ret = display_engine_fp_backlight_set_sync(PRIMARY_PANEL, hbm_level, backlight_type);
#endif
	LCD_KIT_INFO("queue work display_engine_set_fp_backlight done\n");
	if (panel_info->cmd_async_support)
		panel_info->cmd_async = false;
	return ret;
}

void lcd_kit_msg_set(struct mipi_dsi_msg *msg)
{
	struct qcom_panel_info *pinfo = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	pinfo = lcm_get_panel_info(panel_id);
	if (!pinfo) {
		LCD_KIT_INFO("pinfo is null!\n");
		return;
	}
	if (pinfo->cmd_async_support && pinfo->cmd_async) {
		msg->flags |= MIPI_DSI_MSG_ASYNC_OVERRIDE;
		LCD_KIT_INFO("cmd async!\n");
	}
}
