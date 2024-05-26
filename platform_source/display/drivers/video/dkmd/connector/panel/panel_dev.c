/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dkmd_rect.h"
#include "dpu_conn_mgr.h"
#include "panel_dev.h"
#include "dkmd_blpwm.h"
#include "dkmd_backlight.h"

#define LCD_POWER_STATUS 0x1c
/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
};

/*******************************************************************************
** LCD IOMUX
*/
static struct pinctrl_data panel_pctrl;

static struct pinctrl_cmd_desc lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &panel_pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &panel_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &panel_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &panel_pctrl, 0},
};

static struct pinctrl_cmd_desc lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &panel_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &panel_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_P5V5_ENABLE_NAME	"gpio_lcd_vsp_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_vsn_enable"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_BL_ENABLE_NAME	"gpio_lcd_bl_enable"
#define GPIO_LCD_TP1V8_NAME	"gpio_lcd_tp1v8"

static uint32_t gpio_lcd_vsp_enable;
static uint32_t gpio_lcd_vsn_enable;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_bl_enable;
static uint32_t gpio_lcd_tp1v8;

static struct gpio_desc lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_vsp_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_vsn_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
};

static struct gpio_desc lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_vsp_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_vsn_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 10,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc lcd_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_vsp_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_vsn_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_vsp_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_vsn_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_vsp_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_vsn_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

void panel_drv_data_setup(struct panel_drv_private *priv, struct device_node *np)
{
	gpio_lcd_vsp_enable = (uint32_t)of_get_named_gpio(np, "gpios", 0);
	gpio_lcd_vsn_enable = (uint32_t)of_get_named_gpio(np, "gpios", 1);
	gpio_lcd_reset = (uint32_t)of_get_named_gpio(np, "gpios", 2);
	gpio_lcd_bl_enable = (uint32_t)of_get_named_gpio(np, "gpios", 3);
	gpio_lcd_tp1v8 = (uint32_t)of_get_named_gpio(np, "gpios", 4);

	dpu_pr_info("used gpio:[vsp: %d, vsn: %d, rst: %d, bl_en: %d, tp1v8: %d]\n",
		gpio_lcd_vsp_enable, gpio_lcd_vsn_enable, gpio_lcd_reset, gpio_lcd_bl_enable, gpio_lcd_tp1v8);

	priv->gpio_request_cmds = lcd_gpio_request_cmds;
	priv->gpio_request_cmds_len = ARRAY_SIZE(lcd_gpio_request_cmds);
	priv->gpio_free_cmds = lcd_gpio_free_cmds;
	priv->gpio_free_cmds_len = ARRAY_SIZE(lcd_gpio_free_cmds);

	priv->gpio_normal_cmds = lcd_gpio_normal_cmds;
	priv->gpio_normal_cmds_len = ARRAY_SIZE(lcd_gpio_normal_cmds);
	priv->gpio_lowpower_cmds = lcd_gpio_lowpower_cmds;
	priv->gpio_lowpower_cmds_len = ARRAY_SIZE(lcd_gpio_lowpower_cmds);

	if (priv->connector_info.base.fpga_flag == 0) {
		priv->vcc_enable_cmds = lcd_vcc_enable_cmds;
		priv->vcc_enable_cmds_len = ARRAY_SIZE(lcd_vcc_enable_cmds);
		priv->vcc_disable_cmds = lcd_vcc_disable_cmds;
		priv->vcc_disable_cmds_len = ARRAY_SIZE(lcd_vcc_disable_cmds);

		priv->pinctrl_normal_cmds = lcd_pinctrl_normal_cmds;
		priv->pinctrl_normal_cmds_len = ARRAY_SIZE(lcd_pinctrl_normal_cmds);
		priv->pinctrl_lowpower_cmds = lcd_pinctrl_lowpower_cmds;
		priv->pinctrl_lowpower_cmds_len = ARRAY_SIZE(lcd_pinctrl_lowpower_cmds);

		priv->pinctrl_init_cmds = lcd_pinctrl_init_cmds;
		priv->pinctrl_init_cmds_len = ARRAY_SIZE(lcd_pinctrl_init_cmds);
		priv->pinctrl_finit_cmds = lcd_pinctrl_finit_cmds;
		priv->pinctrl_finit_cmds_len = ARRAY_SIZE(lcd_pinctrl_finit_cmds);

		priv->vcc_init_cmds = lcd_vcc_init_cmds;
		priv->vcc_init_cmds_len = (uint32_t)ARRAY_SIZE(lcd_vcc_init_cmds);
		priv->vcc_finit_cmds = lcd_vcc_finit_cmds;
		priv->vcc_finit_cmds_len = (uint32_t)ARRAY_SIZE(lcd_vcc_finit_cmds);
	}
}

/* ----------------------- function pointor setup ------------------------ */

static int32_t panel_dev_lcd_power_on(struct dkmd_connector_info *pinfo)
{
	int32_t ret;
	struct panel_drv_private *priv = to_panel_private(pinfo);
	struct dkmd_bl_ctrl *bl_ctrl = NULL;

	if (!priv) {
		dpu_pr_err("get panel drv private err!\n");
		return -EINVAL;
	}

	bl_ctrl = &priv->bl_ctrl;
	if (bl_ctrl->bl_ops.on)
		bl_ctrl->bl_ops.on(bl_ctrl->private_data);

	dpu_pr_info("enter!\n");
	if (priv->vcc_enable_cmds && (priv->vcc_enable_cmds_len > 0)) {
		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_enable_cmds, priv->vcc_enable_cmds_len);
		if (ret)
			dpu_pr_warn("vcc enable cmds handle fail!\n");
	}

	if (priv->pinctrl_normal_cmds && (priv->pinctrl_normal_cmds_len > 0)) {
		ret = peri_pinctrl_cmds_tx(priv->pdev, priv->pinctrl_normal_cmds, priv->pinctrl_normal_cmds_len);
		if (ret)
			dpu_pr_warn("pinctrl normal cmds handle fail!\n");
	}

	if (priv->gpio_normal_cmds && (priv->gpio_normal_cmds_len > 0)) {
		ret = peri_gpio_cmds_tx(priv->gpio_normal_cmds, priv->gpio_normal_cmds_len);
		if (ret)
			dpu_pr_warn("gpio normal cmds handle fail!\n");
	}
	dpu_pr_info("exit!\n");

	return 0;
}

static int32_t panel_dev_lcd_power_off(struct dkmd_connector_info *pinfo)
{
	int32_t ret = 0;
	struct panel_drv_private *priv = to_panel_private(pinfo);
	struct dkmd_bl_ctrl *bl_ctrl = NULL;

	if (!priv) {
		dpu_pr_info("get panel drv private err!\n");
		return -EINVAL;
	}

	bl_ctrl = &priv->bl_ctrl;
	dkmd_backlight_cancel(bl_ctrl);
	if (bl_ctrl->bl_ops.off)
		bl_ctrl->bl_ops.off(bl_ctrl->private_data);

	dpu_pr_info("enter!\n");
	if (priv->vcc_disable_cmds && (priv->vcc_disable_cmds_len > 0)) {
		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_disable_cmds, priv->vcc_disable_cmds_len);
		if (ret)
			dpu_pr_info("vcc disable cmds handle fail!\n");
	}

	if (priv->pinctrl_lowpower_cmds && (priv->pinctrl_lowpower_cmds_len > 0)) {
		ret = peri_pinctrl_cmds_tx(priv->pdev, priv->pinctrl_lowpower_cmds, priv->pinctrl_lowpower_cmds_len);
		if (ret)
			dpu_pr_info("pinctrl lowpower handle fail!\n");
	}

	if (priv->gpio_lowpower_cmds && (priv->gpio_lowpower_cmds_len > 0)) {
		ret = peri_gpio_cmds_tx(priv->gpio_lowpower_cmds, priv->gpio_lowpower_cmds_len);
		if (ret)
			dpu_pr_info("gpio lowpower handle fail!\n");
	}
	dpu_pr_info("exit!\n");

	return 0;
}

static int32_t lcd_initialize_code(struct panel_drv_private *priv,
	struct dkmd_connector_info *pinfo, const void *value)
{
	int32_t ret = 0;
	char __iomem *dsi_base = (char __iomem *)value;

	if (priv->disp_on_cmds && (priv->disp_on_cmds_len > 0)) {
		ret = mipi_dsi_cmds_tx(priv->disp_on_cmds, priv->disp_on_cmds_len, dsi_base);
		dpu_pr_info("disp on cmds handle %d!\n", ret);
	}

	return 0;
}

/* lcd_check_status need be visited externelly, so get dsi_base internally */
static int32_t lcd_check_status(struct panel_drv_private *priv,
	struct dkmd_connector_info *pinfo, const void *value)
{
	uint32_t status = 0;
	uint32_t try_times = 0;
	char __iomem *dsi_base = NULL;
	struct dpu_connector *connector = NULL;

	(void)value;
	(void)priv;
	dpu_check_and_return(!pinfo, -EINVAL, err, "pinfo is NULL!");
	connector = get_primary_connector(pinfo);
	dpu_check_and_return(!connector, -EINVAL, err, "connector is NULL!");

	dsi_base = connector->connector_base;
	outp32(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), 0x0A06);
	status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
	/* wait for 5ms until reading status timeout */
	while (status & 0x10) {
		udelay(5);
		if (++try_times > 100) {
			dpu_pr_info("Read lcd power status timeout!\n");
			break;
		}
		outp32(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), 0x0A06);
		status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
	}
	status = inp32(DPU_DSI_APB_WR_LP_PLD_DATA_ADDR(dsi_base));
	dpu_pr_info("Power State = %#x!\n", status);
	return ((status & LCD_POWER_STATUS) == LCD_POWER_STATUS) ? 0 : (-1);
}

static int32_t lcd_uninitialize_code(struct panel_drv_private *priv,
	struct dkmd_connector_info *pinfo, const void *value)
{
	int32_t ret = 0;
	char __iomem *dsi_base = (char __iomem *)value;

	if (priv->disp_off_cmds && (priv->disp_off_cmds_len > 0)) {
		ret = mipi_dsi_cmds_tx(priv->disp_off_cmds, priv->disp_off_cmds_len, dsi_base);
		dpu_pr_info("display off cmds handle ret=%d!\n", ret);
	}

	return 0;
}

static int32_t lcd_set_fastboot(struct panel_drv_private *priv,
	struct dkmd_connector_info *pinfo, const void *value)
{
	int ret;

	dpu_pr_info("enter!\n");

	if (priv->pinctrl_normal_cmds && (priv->pinctrl_normal_cmds_len > 0)) {
		ret = peri_pinctrl_cmds_tx(priv->pdev, priv->pinctrl_normal_cmds, priv->pinctrl_normal_cmds_len);
		if (ret)
			dpu_pr_warn("pinctrl normal cmds handle fail!\n");
	}

	if (priv->gpio_request_cmds && (priv->gpio_request_cmds_len > 0)) {
		ret = peri_gpio_cmds_tx(priv->gpio_request_cmds, priv->gpio_request_cmds_len);
		if (ret)
			dpu_pr_info("gpio cmds request err: %d!\n", ret);
	}

	dpu_pr_info("exit!\n");
	return 0;
}

static int32_t lcd_set_display_region(struct panel_drv_private *priv,
	struct dkmd_connector_info *pinfo, const void *value)
{
	static char lcd_disp_x[] = {
		0x2A,
		0x00, 0x00,0x04,0x37
	};
	static char lcd_disp_y[] = {
		0x2B,
		0x00, 0x00,0x07,0x7F
	};
	static struct dsi_cmd_desc set_display_address[] = {
		{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
			sizeof(lcd_disp_x), lcd_disp_x},
		{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
			sizeof(lcd_disp_y), lcd_disp_y},
	};
	struct dkmd_rect *dirty = (struct dkmd_rect *)value;

	if (!dirty) {
		dpu_pr_info("dirty rect is null!\n");
		return -EINVAL;
	}

	dpu_pr_info("dirty_region : %d,%d, %d,%d !\n", dirty->x, dirty->y, dirty->w, dirty->h);

	lcd_disp_x[1] = ((uint32_t)(dirty->x) >> 8) & 0xff;
	lcd_disp_x[2] = ((uint32_t)(dirty->x)) & 0xff;
	lcd_disp_x[3] = ((uint32_t)((uint32_t)dirty->x + dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = ((uint32_t)((uint32_t)dirty->x + dirty->w - 1)) & 0xff;
	lcd_disp_y[1] = ((uint32_t)(dirty->y) >> 8) & 0xff;
	lcd_disp_y[2] = ((uint32_t)(dirty->y)) & 0xff;
	lcd_disp_y[3] = ((uint32_t)((uint32_t)dirty->y + dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = ((uint32_t)((uint32_t)dirty->y + dirty->h - 1)) & 0xff;

	mipi_dsi_cmds_tx(set_display_address,
		ARRAY_SIZE(set_display_address), get_primary_connector(pinfo)->connector_base);

	return 0;
}

static int32_t lcd_set_backlight(struct panel_drv_private *priv,
	struct dkmd_connector_info *pinfo, const void *value)
{
	dkmd_backlight_update(&priv->bl_ctrl, false);
	return 0;
}

struct panel_ops_func_map {
	uint32_t ops_cmd_id;
	int32_t (*handle_func)(struct panel_drv_private *priv,
		struct dkmd_connector_info *pinfo, const void *value);
};

struct panel_ops_func_map panel_ops_func_table[] = {
	{SET_FASTBOOT, lcd_set_fastboot},
	{LCD_SEND_INITIAL_LP_CMD, lcd_initialize_code},
	{LCD_SEND_UNINITIAL_LP_CMD, lcd_uninitialize_code},
	{CHECK_LCD_STATUS, lcd_check_status},
	{SET_BACKLIGHT, lcd_set_backlight},
	{LCD_SET_DISPLAY_REGION, lcd_set_display_region},
};

static int32_t panel_dev_ops_handle(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value)
{
	int32_t i;
	struct panel_ops_func_map *ops_handle = NULL;
	struct panel_drv_private *priv = to_panel_private(pinfo);

	dpu_pr_debug("ops_cmd_id = %d!\n", ops_cmd_id);
	if (!priv) {
		dpu_pr_info("get panel drv private err!\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(panel_ops_func_table); i++) {
		ops_handle = &(panel_ops_func_table[i]);
		if ((ops_cmd_id == ops_handle->ops_cmd_id) && ops_handle->handle_func)
			return ops_handle->handle_func(priv, pinfo, value);
	}

	return 0;
}

void panel_dev_data_setup(struct panel_drv_private *priv)
{
	struct dkmd_conn_handle_data *pdata = NULL;

	pdata = devm_kzalloc(&priv->pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dpu_pr_err("alloc panel device data failed!\n");
		return;
	}
	pdata->conn_info = &priv->connector_info;
	pdata->on_func = panel_dev_lcd_power_on;
	pdata->off_func = panel_dev_lcd_power_off;
	pdata->ops_handle_func = panel_dev_ops_handle;

	/* add panel handle data to platform device */
	if (platform_device_add_data(priv->pdev, pdata, sizeof(*pdata)) != 0) {
		dpu_pr_err("add dsi device data failed!\n");
		return;
	}
}

MODULE_LICENSE("GPL");
