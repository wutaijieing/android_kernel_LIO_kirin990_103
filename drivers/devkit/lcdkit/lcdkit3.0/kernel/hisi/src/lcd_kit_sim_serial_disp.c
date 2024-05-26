/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "lcd_kit_sim_serial_disp.h"

static struct lcd_physical_setting *g_sim_serial_panel[MAX_SIM_SERIAL_PANEL];
static int g_panel_index = 0;
struct mutex g_active_panel_mutex;
static int g_pwr_ref_count = 0;
static struct hisi_fb_data_type *local_hisifb = NULL;
static struct lcd_kit_sysfs_ops g_sim_serial_sysfs_ops;
uint32_t g_alpm_setting = LCD_ALPM_SETTING_DEFAULT;

uint32_t lcd_kit_get_alpm_setting_status(void)
{
	return g_alpm_setting;
}

static bool lcd_kit_check_sim_serial_id(uint32_t panel_id)
{
	if (panel_id == AUXILIARY_PANEL_IDX || panel_id == MEDIACOMMON_PANEL_IDX)
		return false;
	return true;
}

static uint32_t lcd_kit_get_another_panel(uint32_t panel_id)
{
	if (panel_id > LCD_MAIN_PANEL)
		return LCD_MAIN_PANEL;
	else if (panel_id == LCD_MAIN_PANEL)
		return LCD_EXT_PANEL;
	return LCD_MAIN_PANEL;
}

static void lcd_kit_activate_panel_mutex(uint32_t panel_id)
{
	LCD_KIT_INFO("lcd folder type from %u change to %u.\n",
		PUBLIC_CFG->active_panel_id, panel_id);
	PUBLIC_CFG->active_panel_id = panel_id;
	mutex_lock(&g_active_panel_mutex);
}

static void lcd_kit_deactivate_panel_mutex(void)
{
	PUBLIC_CFG->active_panel_id = LCD_MAIN_PANEL;
	mutex_unlock(&g_active_panel_mutex);
}

static uint32_t lcd_kit_get_hisifd_index(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	if (pdev == NULL)
		return FB_INDEX_MAX;
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null\n");
		return FB_INDEX_MAX;
	}
	return hisifd->index;
}

int lcd_kit_single_power_refcount_on(struct platform_device *pdev,
	struct callback_data *data)
{
	struct hisi_fb_data_type *hisifd = NULL;
	if (pdev == NULL || data == NULL || data->func_id != LCD_KIT_FUNC_ON) {
		LCD_KIT_ERR("invalid data, return.\n");
		return LCD_KIT_FAIL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		LCD_KIT_ERR("invalid hisifd, return.\n");
		return LCD_KIT_FAIL;
	}
	LCD_KIT_DEBUG("enter, step %d.\n", hisifd->panel_info.lcd_init_step);
	if (hisifd->panel_info.lcd_init_step != LCD_INIT_POWER_ON ||
		hisifd->aod_function > 0)
		return LCD_KIT_OK;

	if (++g_pwr_ref_count > 1) {
		LCD_KIT_INFO("current ref count is %d, go to out\n", g_pwr_ref_count);
		hisifd->panel_info.lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
		return LCD_CB_BYPASS;
	}
	LCD_KIT_INFO("current ref count is %d.\n", g_pwr_ref_count);
	if (hisifd->index == LCD_EXT_PANEL && common_ops->panel_power_on &&
		hisifd->aod_function == 0) {
		lcd_kit_activate_panel_mutex(LCD_MAIN_PANEL);
		common_ops->panel_power_on((void *)hisifd);
		lcd_kit_deactivate_panel_mutex();
		hisifd->panel_info.lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
		return LCD_CB_BYPASS;
	}
	return LCD_KIT_OK;
}

int lcd_kit_single_power_refcount_off(struct platform_device *pdev,
	struct callback_data *data)
{
	struct hisi_fb_data_type *hisifd = NULL;
	if (pdev == NULL || data == NULL || data->func_id != LCD_KIT_FUNC_OFF) {
		LCD_KIT_ERR("invalid data, return.\n");
		return LCD_KIT_FAIL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		LCD_KIT_ERR("invalid hisifd, return.\n");
		return LCD_KIT_FAIL;
	}

	LCD_KIT_DEBUG("enter, step %d.\n", hisifd->panel_info.lcd_uninit_step);
	if (hisifd->panel_info.lcd_uninit_step != LCD_UNINIT_POWER_OFF ||
		hisifd->aod_function > 0)
		return LCD_KIT_OK;

	if (--g_pwr_ref_count > 0) {
		LCD_KIT_INFO("current ref count is %d, go to out\n", g_pwr_ref_count);
		return LCD_CB_BYPASS;
	}
	LCD_KIT_INFO("current ref count is %d.\n", g_pwr_ref_count);
	if (hisifd->index == LCD_EXT_PANEL && common_ops->panel_power_off &&
		hisifd->aod_function == 0) {
		lcd_kit_activate_panel_mutex(LCD_MAIN_PANEL);
		common_ops->panel_power_off((void *)hisifd);
		lcd_kit_deactivate_panel_mutex();
	}
	g_pwr_ref_count = 0;
	return LCD_KIT_OK;
}

static uint32_t lcd_kit_sim_serial_cali_backlight(uint32_t panel_id,
	uint32_t bl)
{
	uint8_t *data;
	uint32_t offset;
	if (!lcd_kit_check_sim_serial_id(panel_id)) {
		LCD_KIT_ERR("no valid cali data\n");
		return bl;
	}
	if (!g_sim_serial_panel[panel_id] ||
		!g_sim_serial_panel[panel_id]->backlight_cali_map) {
		LCD_KIT_ERR("no cali bl map for fb%d\n", panel_id);
		return bl;
	}
	data = g_sim_serial_panel[panel_id]->backlight_cali_map;
	if (bl >= BL_MAX_LEVEL)
		return bl;

	return *(data + bl);
}


int lcd_kit_sim_backlight_postproc(struct platform_device *pdev,
	struct callback_data *data)
{
	int ret = LCD_KIT_OK;
	uint32_t index = FB_INDEX_MAX;
	uint32_t tmp_bl;
	uint32_t cali_bl;
	if (pdev == NULL || data == NULL || data->data == NULL ||
		data->func_id != LCD_KIT_FUNC_SET_BACKLIGHT) {
		LCD_KIT_ERR("invalid data, return.\n");
		return LCD_KIT_FAIL;
	}
	index = lcd_kit_get_another_panel(lcd_kit_get_hisifd_index(pdev));
	if (!lcd_kit_check_sim_serial_id(index) && hisifd_list[index] == NULL) {
		LCD_KIT_ERR("invalid hisifd, return.\n");
		return LCD_KIT_FAIL;
	}
	if (hisifd_list[index]->aod_function == LCD_SIM_ALPM_ON) {
		LCD_KIT_INFO("aod mode, do nothing.\n");
		return LCD_KIT_OK;
	}
	tmp_bl = *(uint32_t *)data->data;

	if (!hisifd_list[index]->panel_power_on ||
		!hisifd_list[index]->backlight.bl_updated) {
		hisifd_list[index]->bl_level = tmp_bl;
		LCD_KIT_INFO("panel is not ready for bl, do nothing.\n");
		return LCD_KIT_OK;
	}
	lcd_kit_activate_panel_mutex(index);
	hisifd_list[index]->bl_level = tmp_bl;
	cali_bl = lcd_kit_sim_serial_cali_backlight(index, tmp_bl);
	ret = lcd_kit_pdata()->set_backlight(hisifd_list[index]->pdev, cali_bl);
	hisifd_list[index]->backlight.bl_timestamp = ktime_get();
	hisifd_list[index]->backlight.bl_level_old = (int)tmp_bl;
	lcd_kit_deactivate_panel_mutex();
	if (ret < 0)
		return LCD_KIT_FAIL;
	return LCD_KIT_OK;
}

int lcd_kit_sim_backlight_preproc(struct platform_device *pdev,
	struct callback_data *data)
{
	uint32_t index = FB_INDEX_MAX;
	if (pdev == NULL || data == NULL || data->data == NULL ||
		data->func_id != LCD_KIT_FUNC_SET_BACKLIGHT) {
		LCD_KIT_ERR("invalid data, return.\n");
		return LCD_KIT_FAIL;
	}
	index = lcd_kit_get_another_panel(lcd_kit_get_hisifd_index(pdev));
	if (!lcd_kit_check_sim_serial_id(index) && hisifd_list[index] == NULL) {
		LCD_KIT_ERR("invalid hisifd, return.\n:");
		return LCD_KIT_FAIL;
	}

	if (hisifd_list[index]->backlight.bl_updated == 0) {
		if (index == LCD_EXT_PANEL && hisifd_list[index]->aod_function == 0)
			hisifd_list[index]->bl_level = *(uint32_t *)data->data;
		LCD_KIT_INFO("wait another panel, return\n");
		return LCD_CB_BYPASS;
	}
	return LCD_KIT_OK;
}

struct lcd_callback lcd_kit_sim_serial_pwr_ref_count_on[] = {
	{ .func = lcd_kit_single_power_refcount_on, },
	{ .func = lcd_kit_single_power_refcount_on, },
};
struct lcd_callback lcd_kit_sim_serial_pwr_ref_count_off[] = {
	{ .func = lcd_kit_single_power_refcount_off, },
	{ .func = lcd_kit_single_power_refcount_off, },
};
struct lcd_callback lcd_kit_sim_backlight_preproc_cb[] = {
	 { .func = lcd_kit_sim_backlight_preproc, },
	 { .func = lcd_kit_sim_backlight_preproc, },
};
struct lcd_callback lcd_kit_sim_backlight_postproc_cb[] = {
	 { .func = lcd_kit_sim_backlight_postproc, },
	 { .func = lcd_kit_sim_backlight_postproc, },
};

static void lcd_kit_sim_serial_init_cb(uint32_t panel_id)
{
	if (disp_info->sim_serial_panel.pwr_ref_count_support) {
		lcd_kit_regist_preproc(panel_id, LCD_KIT_FUNC_ON,
			&lcd_kit_sim_serial_pwr_ref_count_on[panel_id]);
		lcd_kit_regist_preproc(panel_id, LCD_KIT_FUNC_OFF,
			&lcd_kit_sim_serial_pwr_ref_count_off[panel_id]);
	}
	if (!get_boot_into_recovery_flag() &&
		!get_pd_charge_flag() &&
		disp_info->sim_serial_panel.sim_backlight_support) {
		lcd_kit_regist_preproc(panel_id, LCD_KIT_FUNC_SET_BACKLIGHT,
			&lcd_kit_sim_backlight_preproc_cb[panel_id]);
		lcd_kit_regist_postproc(panel_id, LCD_KIT_FUNC_SET_BACKLIGHT,
			&lcd_kit_sim_backlight_postproc_cb[panel_id]);
	}
}

static void sim_serial_get_backlight_cali_data(struct device_node *np,
	uint32_t index)
{
	uint8_t *dts_data = NULL;
	uint32_t blen = 0;
	dts_data = (uint8_t *)of_get_property(np, "lcd-kit,light-cali-map", &blen);
	if (!dts_data || blen != BL_CALI_DATA_LEN ||
		dts_data[BL_MAX_LEVEL] == 0) {
		LCD_KIT_ERR("get light cali data fail\n");
		return;
	}
	g_sim_serial_panel[index]->backlight_cali_map = dts_data;
	LCD_KIT_INFO("cali addr %p, len %d\n", dts_data, blen);
}

static int lcd_kit_register_sim_serial_panel(uint32_t index,
	struct device_node *np)
{
	uint32_t i;
	if (index > LCD_EXT_PANEL) {
		LCD_KIT_ERR("invalid panel.\n");
		return LCD_KIT_FAIL;
	}

	g_sim_serial_panel[index] = kzalloc(sizeof(struct lcd_physical_setting),
		GFP_KERNEL);
	if (g_sim_serial_panel[index] == NULL) {
		LCD_KIT_ERR("register sim serial panel %d failed.\n", index);
		return LCD_KIT_FAIL;
	}
	for (i = 0; i < LCD_KIT_FUNC_MAX; i++) {
		INIT_LIST_HEAD(&g_sim_serial_panel[index]->preproc_callback_head[i]);
		INIT_LIST_HEAD(&g_sim_serial_panel[index]->postproc_callback_head[i]);
	}
#if defined(CONFIG_DPU_FB_AP_AOD)
	g_sim_serial_panel[index]->alpm_mode = LCD_ALPM_SETTING_DEFAULT;
#endif

	sim_serial_get_backlight_cali_data(np, index);
	return LCD_KIT_OK;
}

static void lcd_kit_unregister_physical_panel(uint32_t panel_id)
{
	kfree(g_sim_serial_panel[panel_id]);
	g_sim_serial_panel[panel_id] = NULL;
}

static int lcd_kit_exec_preproc_cb(struct platform_device *pdev,
	uint32_t func_id, struct callback_data *data)
{
	uint32_t index = FB_INDEX_MAX;
	struct list_head *callback_head = NULL;
	struct lcd_callback *cur = NULL;
	uint32_t ret = LCD_KIT_OK;

	LCD_KIT_INFO("enter.\n");
	if (pdev == NULL || data == NULL) {
		LCD_KIT_ERR("invaild data.\n");
		return LCD_KIT_FAIL;
	}

	index = lcd_kit_get_hisifd_index(pdev);
	if (!lcd_kit_check_sim_serial_id(index) ||
		g_sim_serial_panel[index] == NULL ||
		func_id >= LCD_KIT_FUNC_MAX) {
		LCD_KIT_ERR("invalid device fb%d[%p].\n", index,
			g_sim_serial_panel[index]);
		return LCD_KIT_FAIL;
	}

	callback_head = &g_sim_serial_panel[index]->preproc_callback_head[func_id];
	if (callback_head == NULL) {
		LCD_KIT_ERR("fb%d's callback func has not been initialized.\n", index);
		return LCD_KIT_FAIL;
	}
	list_for_each_entry(cur, callback_head, list) {
		if (cur->func != NULL)
			ret |= (uint32_t)(cur->func(pdev, data));
	}

	if (ret & LCD_CB_BYPASS)
		return LCD_CB_BYPASS;
	else if (ret & LCD_KIT_FAIL)
		return LCD_KIT_FAIL;
	return LCD_KIT_OK;
}

static int lcd_kit_exec_postproc_cb(struct platform_device *pdev,
	uint32_t func_id, struct callback_data *data)
{
	uint32_t index = FB_INDEX_MAX;
	struct list_head *callback_head;
	struct lcd_callback *cur = NULL;

	LCD_KIT_INFO("enter.\n");
	if (pdev == NULL || data == NULL) {
		LCD_KIT_ERR("invaild data.\n");
		return LCD_KIT_FAIL;
	}

	index = lcd_kit_get_hisifd_index(pdev);
	if (!lcd_kit_check_sim_serial_id(index) ||
		g_sim_serial_panel[index] == NULL ||
		func_id >= LCD_KIT_FUNC_MAX) {
		LCD_KIT_ERR("invalid device fb%d[%p].\n", index,
			g_sim_serial_panel[index]);
		return LCD_KIT_FAIL;
	}

	callback_head = &g_sim_serial_panel[index]->postproc_callback_head[func_id];
	if (callback_head == NULL) {
		LCD_KIT_ERR("fb%d's callback func has not been initialized.\n", index);
		return LCD_KIT_FAIL;
	}
	list_for_each_entry(cur, callback_head, list) {
		if (cur->func == NULL) {
			LCD_KIT_ERR("invalid postproc func, %p.\n", cur);
			return LCD_KIT_FAIL;
		}
		cur->func(pdev, data);
	}
	return LCD_KIT_OK;
}

int lcd_kit_regist_preproc(uint32_t panel_id, uint32_t func_id,
	struct lcd_callback *cb)
{
	if (cb == NULL || cb->func == NULL ||
		!lcd_kit_check_sim_serial_id(panel_id) ||
		func_id >= LCD_KIT_FUNC_MAX) {
		LCD_KIT_ERR("invalid param, regist preproc to fb%d faild.\n", panel_id);
		return LCD_KIT_FAIL;
	}
	if (g_sim_serial_panel[panel_id] == NULL) {
		LCD_KIT_ERR("fb%d is invalid.\n", panel_id);
		return LCD_KIT_FAIL;
	}
	list_add_tail(&cb->list,
		&g_sim_serial_panel[panel_id]->preproc_callback_head[func_id]);
	LCD_KIT_INFO("success regist preproc to fb%d", panel_id);
	return LCD_KIT_OK;
}

int lcd_kit_regist_postproc(uint32_t panel_id, uint32_t func_id,
	struct lcd_callback *cb)
{
	if (cb == NULL || cb->func == NULL ||
		!lcd_kit_check_sim_serial_id(panel_id) ||
		func_id >= LCD_KIT_FUNC_MAX) {
		LCD_KIT_ERR("invalid param, regist postproc to fb%d faild.\n",
			panel_id);
		return LCD_KIT_FAIL;
	}
	if (g_sim_serial_panel[panel_id] == NULL) {
		LCD_KIT_ERR("fb%d is invalid.\n", panel_id);
		return LCD_KIT_FAIL;
	}
	list_add_tail(&cb->list,
		&g_sim_serial_panel[panel_id]->postproc_callback_head[func_id]);
	LCD_KIT_INFO("success regist postproc to fb%d", panel_id);
	return LCD_KIT_OK;
}

static int lcd_kit_sim_serial_set_fastboot(struct platform_device* pdev)
{
	uint32_t index = FB_INDEX_MAX;
	int ret = LCD_KIT_OK;
	struct callback_data cb_data = { LCD_KIT_FUNC_SET_FASTBOOT, NULL };
	index = lcd_kit_get_hisifd_index(pdev);
	if (index >= FB_INDEX_MAX) {
		LCD_KIT_ERR("fb%d is invalid\n", index);
		return LCD_KIT_OK;
	}
	if (lcd_kit_exec_preproc_cb(pdev, LCD_KIT_FUNC_SET_FASTBOOT,
		&cb_data) == LCD_CB_BYPASS) {
		LCD_KIT_INFO("set fastboot preproc go to out.\n");
		return LCD_KIT_OK;
	}
	g_pwr_ref_count++;
	lcd_kit_activate_panel_mutex(index);
	LCD_KIT_INFO("exec func set fastboot on fb%d", index);
	ret = lcd_kit_pdata()->set_fastboot(pdev);
	lcd_kit_deactivate_panel_mutex();
	lcd_kit_exec_postproc_cb(pdev, LCD_KIT_FUNC_SET_FASTBOOT, &cb_data);
	return ret;
}

static int lcd_kit_sim_serial_on(struct platform_device* pdev)
{
	uint32_t index = FB_INDEX_MAX;
	int ret = LCD_KIT_OK;
	struct callback_data cb_data = { LCD_KIT_FUNC_ON, NULL };
	index = lcd_kit_get_hisifd_index(pdev);
	if (index >= FB_INDEX_MAX) {
		LCD_KIT_ERR("on func fb%d is invalid\n", index);
		return LCD_KIT_OK;
	}
	if (lcd_kit_exec_preproc_cb(pdev, LCD_KIT_FUNC_ON,
		&cb_data) == LCD_CB_BYPASS) {
		LCD_KIT_INFO("on func preproc go to out.\n");
		return LCD_KIT_OK;
	}
	lcd_kit_activate_panel_mutex(index);
	LCD_KIT_INFO("exec func on func for fb%d", index);
	ret = lcd_kit_pdata()->on(pdev);
	lcd_kit_deactivate_panel_mutex();
	lcd_kit_exec_postproc_cb(pdev, LCD_KIT_FUNC_ON, &cb_data);
	return ret;
}

static int lcd_kit_sim_serial_off(struct platform_device* pdev)
{
	uint32_t index = FB_INDEX_MAX;
	int ret = LCD_KIT_OK;
	struct callback_data cb_data = { LCD_KIT_FUNC_OFF, NULL };
	index = lcd_kit_get_hisifd_index(pdev);
	if (index >= FB_INDEX_MAX) {
		LCD_KIT_ERR("off func fb%d is invalid\n", index);
		return LCD_KIT_OK;
	}
	if (lcd_kit_exec_preproc_cb(pdev, LCD_KIT_FUNC_OFF,
		&cb_data) == LCD_CB_BYPASS) {
		LCD_KIT_INFO("off func preproc go to out.\n");
		return LCD_KIT_OK;
	}
	lcd_kit_activate_panel_mutex(index);
	LCD_KIT_INFO("exec func off func on fb%d", index);
	ret = lcd_kit_pdata()->off(pdev);
	lcd_kit_deactivate_panel_mutex();
	lcd_kit_exec_postproc_cb(pdev, LCD_KIT_FUNC_OFF, &cb_data);
	return ret;
}

static int lcd_kit_sim_serial_set_backlight(struct platform_device *pdev,
	uint32_t bl_level)
{
	uint32_t index = FB_INDEX_MAX;
	int ret = LCD_KIT_OK;
	struct callback_data cb_data = { LCD_KIT_FUNC_SET_BACKLIGHT, &bl_level };
	uint32_t cali_bl;
	index = lcd_kit_get_hisifd_index(pdev);
	if (index >= FB_INDEX_MAX) {
		LCD_KIT_ERR("set backlight fb%d is invalid\n", index);
		return LCD_KIT_OK;
	}
	if (lcd_kit_exec_preproc_cb(pdev, LCD_KIT_FUNC_SET_BACKLIGHT,
		&cb_data) == LCD_CB_BYPASS) {
		LCD_KIT_INFO("set backlight preproc go to out.\n");
		return LCD_KIT_OK;
	}
	lcd_kit_activate_panel_mutex(index);
	LCD_KIT_INFO("exec func set backlight func on fb%d", index);
	cali_bl = lcd_kit_sim_serial_cali_backlight(index, bl_level);
	ret = lcd_kit_pdata()->set_backlight(pdev, cali_bl);
	lcd_kit_deactivate_panel_mutex();
	lcd_kit_exec_postproc_cb(pdev, LCD_KIT_FUNC_SET_BACKLIGHT, &cb_data);
	return ret;
}

static void lcd_kit_sim_serial_override_pdata(struct hisi_fb_panel_data *pdata)
{
	(void)memset(pdata, 0, sizeof(*pdata));
	pdata->set_fastboot = lcd_kit_sim_serial_set_fastboot;
	pdata->on = lcd_kit_sim_serial_on;
	pdata->off = lcd_kit_sim_serial_off;
	pdata->set_backlight = lcd_kit_sim_serial_set_backlight;
}

#if defined(CONFIG_DPU_FB_AP_AOD)
static int lcd_kit_sim_serial_alpm_setting(struct hisi_fb_data_type *hisifd,
	uint32_t mode)
{
	if (hisifd == NULL || !lcd_kit_check_sim_serial_id(hisifd->index)) {
		LCD_KIT_ERR("invaild param.\n");
		return LCD_KIT_FAIL;
	}

	lcd_kit_activate_panel_mutex(hisifd->index);
	switch (mode) {
	case ALPM_DISPLAY_OFF:
		hisifd->aod_mode = LCD_SIM_ALPM_ON;
		break;
	case ALPM_ON_MIDDLE_LIGHT:
		lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.off_cmds);
		break;
	case ALPM_EXIT:
		lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.exit_cmds);
		hisifd->aod_mode = 0;
		break;
	case ALPM_CLOSE_TE:
		lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.close_te_cmds);
		break;
	case ALPM_OPEN_TE:
		lcd_kit_dsi_cmds_tx(hisifd, &disp_info->alpm.open_te_cmds);
		break;
	default:
		LCD_KIT_ERR("invalid mode %u\n", mode);
		break;
	}
	g_sim_serial_panel[hisifd->index]->alpm_mode = mode;
	lcd_kit_deactivate_panel_mutex();
	return LCD_KIT_OK;
}

static ssize_t lcd_sim_serial_alpm_setting_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;
	unsigned int mode = 0;

	hisifd = dev_get_hisifd(dev);
	if (!hisifd || !buf || strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("invalid argument\n");
		return LCD_KIT_FAIL;
	}
	ret = sscanf(buf, "%u", &mode);
	if (!ret) {
		LCD_KIT_ERR("sscanf return invaild:%zd\n", ret);
		return LCD_KIT_FAIL;
	}
	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		LCD_KIT_ERR("fb%d is not ready\n", hisifd->index);
		up(&hisifd->blank_sem);
		return LCD_KIT_FAIL;
	}

	hisifb_activate_vsync(hisifd);
	lcd_kit_sim_serial_alpm_setting(hisifd, mode);
	hisifb_deactivate_vsync(hisifd);
	up(&hisifd->blank_sem);
	return count;
}

static ssize_t lcd_sim_serial_alpm_function_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (!hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("lcd alpm function store buf null Pointer!\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) >= MAX_BUF) {
		LCD_KIT_ERR("buf overflow!\n");
		return LCD_KIT_FAIL;
	}
	ret = sscanf(buf, "%u", &hisifd->aod_function);
	if (!ret) {
		LCD_KIT_ERR("sscanf return invaild:%zd\n", ret);
		return LCD_KIT_FAIL;
	}
	hisifd->ap_aod_skip_frame_flag = hisifd->aod_function;
	g_alpm_setting = hisifd->aod_function;
	LCD_KIT_INFO("fb%d aod func %d\n",
			hisifd->index, hisifd->aod_function);
	return count;
}

static ssize_t lcd_sim_serial_alpm_function_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	int panel_power_status = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (!hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("lcd alpm function show buf is null!\n");
		return LCD_KIT_FAIL;
	}

	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on)
		panel_power_status = LCD_SIM_ALPM_ON;

	ret = snprintf(buf, PAGE_SIZE, "%d", panel_power_status);
	LCD_KIT_INFO("fb%d panel power status %d!\n",
		hisifd->index, panel_power_status);
	up(&hisifd->blank_sem);

	return ret;
}

static ssize_t lcd_sim_serial_alpm_setting_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (!hisifd || !lcd_kit_check_sim_serial_id(hisifd->index)
		|| !g_sim_serial_panel[hisifd->index]) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("lcd alpm setting show buf is null!\n");
		return LCD_KIT_FAIL;
	}
	ret = snprintf(buf, PAGE_SIZE, "%d",
		g_sim_serial_panel[hisifd->index]->alpm_mode);
	return ret;
}
#endif

static ssize_t lcd_sim_serial_model_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = LCD_KIT_OK;

	if ((!runmode_is_factory()) && (common_info->panel_model != NULL))
		return snprintf(buf, PAGE_SIZE, "%s\n", common_info->panel_model);

	if (common_ops->get_panel_name)
		ret = common_ops->get_panel_name(buf);
	return ret;
}

static int lcd_kit_sim_serial_check_support(int index)
{
#if defined(CONFIG_DPU_FB_AP_AOD)
	if (index == ALPM_SETTING_INDEX || index == ALPM_FUNCTION_INDEX)
		return disp_info->alpm.support;
#endif
	if (index == LCD_MODEL_INDEX || index == LCD_TYPE_INDEX)
		return SYSFS_SUPPORT;
	return 0;
}

static ssize_t lcd_sim_serial_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = dev_get_hisifd(dev);
	if (!hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", is_mipi_cmd_panel(hisifd) ? 1 : 0);
}

static int lcd_kit_sim_serial_sysfs_init(uint32_t index)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret;

	if (!lcd_kit_check_sim_serial_id(index)) {
		LCD_KIT_ERR("invalid index %d, return\n", index);
		return LCD_KIT_FAIL;
	}

	hisifd = hisifd_list[index];
	if (!hisifd) {
		LCD_KIT_ERR("hisifd is null\n");
		return LCD_KIT_FAIL;
	}
	if (lcd_kit_get_sysfs_ops() == NULL) {
		g_sim_serial_sysfs_ops.check_support = lcd_kit_sim_serial_check_support;
#if defined(CONFIG_DPU_FB_AP_AOD)
		g_sim_serial_sysfs_ops.alpm_setting_store = lcd_sim_serial_alpm_setting_store;
		g_sim_serial_sysfs_ops.alpm_function_store =
			lcd_sim_serial_alpm_function_store;
		g_sim_serial_sysfs_ops.alpm_setting_show = lcd_sim_serial_alpm_setting_show;
		g_sim_serial_sysfs_ops.alpm_function_show = lcd_sim_serial_alpm_function_show;
#endif
		g_sim_serial_sysfs_ops.model_show = lcd_sim_serial_model_show;
		g_sim_serial_sysfs_ops.type_show = lcd_sim_serial_type_show;
		lcd_kit_sysfs_ops_register(&g_sim_serial_sysfs_ops);
	}
	ret = lcd_kit_create_sysfs(&hisifd->fbi->dev->kobj);
	if (ret)
		LCD_KIT_ERR("create fs node fail\n");
	return LCD_KIT_OK;
}

static void lcd_kit_sim_serial_panel_probe_sub(struct platform_device *pdev,
	struct device_node *np, struct hisi_panel_info *pinfo)
{
	lcd_kit_adapt_init();
	if (common_ops->common_init)
		common_ops->common_init(np);
	lcd_kit_utils_init(np, pinfo);
	factory_init(pinfo);
	lcd_kit_power_init(pdev);
	lcd_kit_panel_init();
}

static int lcd_kit_sim_serial_panel_probe(struct platform_device *pdev)
{
	struct hisi_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	struct hisi_fb_panel_data pdata;

	LCD_KIT_INFO("enter. panel index %d.\n", g_panel_index);

	np = pdev->dev.of_node;
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node\n");
		return LCD_KIT_FAIL;
	}

	PUBLIC_CFG->active_panel_id = (unsigned int)g_panel_index;
	disp_info->compatible = (char *)of_get_property(np, "compatible", NULL);

	pinfo = lcd_kit_get_pinfo(g_panel_index);
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return LCD_KIT_FAIL;
	}
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->disp_panel_id = g_panel_index;
	/* init adapt, common, factory, utils... */
	lcd_kit_sim_serial_panel_probe_sub(pdev, np, pinfo);
	if (hisi_fb_device_probe_defer(pinfo->type, pinfo->bl_set_type))
		goto err_probe_defer;

	pdev->id = g_panel_index + 1; /* dev id should start from 1 */
	lcd_kit_sim_serial_override_pdata(&pdata);
	pdata.panel_info = pinfo;
	if (platform_device_add_data(pdev, &pdata, sizeof(pdata))) {
		LCD_KIT_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}
	hisi_fb_add_device(pdev);

	if (lcd_kit_register_sim_serial_panel(g_panel_index, np) < 0)
		goto err_register_phy_panel;
	lcd_kit_sim_serial_init_cb(g_panel_index);

#ifdef LCD_FACTORY_MODE
	lcd_factory_init(np);
#endif
	lcd_kit_sim_serial_sysfs_init(g_panel_index);
	lcd_kit_register_power_key_notify();
	g_panel_index++;
	return LCD_KIT_OK;

err_register_phy_panel:
	lcd_kit_unregister_physical_panel(g_panel_index);
err_device_put:
	platform_device_put(pdev);
err_probe_defer:
	return -EPROBE_DEFER;
}

static struct of_device_id lcd_kit_sim_serial_panel_match_table[] = {
	{
		.compatible = "auo_otm1901a_5p2_1080p_video",
		.data = NULL,
	},
	{
		.compatible = "lcd_ext_panel_default",
		.data = NULL,
	},
	{},
};

static struct platform_driver lcd_kit_sim_serial_panel_driver = {
	.probe = lcd_kit_sim_serial_panel_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "lcd_kit_ext_mipi_panel",
		.of_match_table = lcd_kit_sim_serial_panel_match_table,
	},
};

static void lcd_kit_overlay_match_table(struct device_node *np)
{
	char *panel_compatible_tmp;
	size_t len;
	uint32_t i;
	struct of_device_id *match_table = lcd_kit_sim_serial_panel_match_table;
	char *property_names[] = {
		"lcd_panel_type",
		"ext_lcd_panel_type",
	};

	for (i = 0; i < ARRAY_SIZE(property_names); i++) {
		panel_compatible_tmp = (char *)of_get_property(np,
			property_names[i], NULL);
		if (panel_compatible_tmp == NULL)
			continue;
		LCD_KIT_INFO("get available panel %s\n", panel_compatible_tmp);
		len = strlen(panel_compatible_tmp);
		memset((char *)match_table->compatible, 0, LCD_KIT_PANEL_COMP_LENGTH);
		strncpy((char *)match_table->compatible, panel_compatible_tmp,
			len > (LCD_KIT_PANEL_COMP_LENGTH - 1) ?
			(LCD_KIT_PANEL_COMP_LENGTH - 1) : len);
		match_table++;
	}
}

int lcd_kit_sim_serial_panel_init(void)
{
	int ret = LCD_KIT_OK;
	struct device_node *np = NULL;
	unsigned int product_type = 0;
	uint32_t is_serial_panel = 0;

	LCD_KIT_INFO("sim serial panel init enter.\n");
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_KIT_PANEL_TYPE);
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node %s!\n",
			DTS_COMP_LCD_KIT_PANEL_TYPE);
		return LCD_KIT_FAIL;
	}
	OF_PROPERTY_READ_U32_RETURN(np, "product_type",
		&product_type);
	OF_PROPERTY_READ_U32_RETURN(np, "is_serial_panel",
		&is_serial_panel);
	if (product_type != LCD_DUAL_PANEL_SIM_DISPLAY_TYPE ||
		is_serial_panel == 0)
		return LCD_KIT_FAIL;

	lcd_kit_overlay_match_table(np);
	mutex_init(&g_active_panel_mutex);
	ret = platform_driver_register(&lcd_kit_sim_serial_panel_driver);
	if (ret != LCD_KIT_OK) {
		LCD_KIT_ERR("Driver_register failed, error = %d!\n", ret);
		return LCD_KIT_FAIL;
	}

	LCD_KIT_INFO("sim serial panel init finish.\n");
	return LCD_KIT_OK;
}
