/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "dpu_fb.h"
#include "dpu_fb_panel.h"
#include "dpu_mipi_dsi.h"
#include "dpu_dpe_utils.h"

/*lint -e574 -e647 -e568 -e685 -e578*/
DEFINE_SEMAPHORE(dpu_fb_dts_resource_sem);

/* none, orise2x, orise3x, himax2x, rsp2x, rsp3x, vesa2x, vesa3x */
mipi_ifbc_division_t g_mipi_ifbc_division[MIPI_DPHY_NUM][IFBC_TYPE_MAX] = {
	/* single mipi */
	{
		/* none */
		{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* orise2x */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
		/* orise3x */
		{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_1, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
		/* himax2x */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_2, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
		/* rsp2x */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_3, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_OPEN, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
		/* rsp3x  [NOTE]reality: xres_div = 1.5, yres_div = 2, amended in "mipi_ifbc_get_rect" function */
		{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_4, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_OPEN, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
		/* vesa2x_1pipe */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
		/* vesa3x_1pipe */
		{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
		/* vesa2x_2pipe */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
		/* vesa3x_2pipe */
		{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
		/* vesa3.75x_2pipe */
		{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3},
		/* SPR+DSC1.2 need special calulation for compression.
		 * It calucate the div in get_hsize_after_spr_dsc().
		 */
		/* vesa2.66x_pipe depend on SPR */
		{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa2.66x_pipe depend on SPR */
		{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa2x_pipe depend on SPR */
		{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa2x_pipe depend on SPR */
		{XRES_DIV_1, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa3.75x_1pipe */
		{XRES_DIV_3, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_2, PXL0_DSI_GT_EN_3}
	},

	/* dual mipi */
	{
		/* none */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_1, PXL0_DSI_GT_EN_3},
		/* orise2x */
		{XRES_DIV_4, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_3, PXL0_DSI_GT_EN_3},
		/* orise3x */
		{XRES_DIV_6, YRES_DIV_1, IFBC_COMP_MODE_1, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_5, PXL0_DSI_GT_EN_3},
		/* himax2x */
		{XRES_DIV_4, YRES_DIV_1, IFBC_COMP_MODE_2, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_3, PXL0_DSI_GT_EN_3},
		/* rsp2x */
		{XRES_DIV_4, YRES_DIV_1, IFBC_COMP_MODE_3, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_OPEN, PXL0_DIVCFG_3, PXL0_DSI_GT_EN_3},
		/* rsp3x */
		{XRES_DIV_3, YRES_DIV_2, IFBC_COMP_MODE_4, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_OPEN, PXL0_DIVCFG_5, PXL0_DSI_GT_EN_3},
		/* vesa2x_1pipe */
		{XRES_DIV_4, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_3, PXL0_DSI_GT_EN_3},
		/* vesa3x_1pipe */
		{XRES_DIV_6, YRES_DIV_1, IFBC_COMP_MODE_5, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_5, PXL0_DSI_GT_EN_3},
		/* vesa2x_2pipe */
		{XRES_DIV_4, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_3, PXL0_DSI_GT_EN_3},
		/* vesa3x_2pipe */
		{XRES_DIV_6, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_5, 3},
		/* vesa3.75x_2pipe */
		{XRES_DIV_6, YRES_DIV_1, IFBC_COMP_MODE_6, PXL0_DIV2_GT_EN_OPEN,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_5, 3},
		/* SPR+DSC1.2 need special calulation for compression.
		 * It calucate the div in get_hsize_after_spr_dsc().
		 */
		/* vesa2.66x_pipe depend on SPR */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa2.66x_pipe depend on SPR */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa2x_pipe depend on SPR */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1},
		/* vesa2x_pipe depend on SPR */
		{XRES_DIV_2, YRES_DIV_1, IFBC_COMP_MODE_0, PXL0_DIV2_GT_EN_CLOSE,
			PXL0_DIV4_GT_EN_CLOSE, PXL0_DIVCFG_0, PXL0_DSI_GT_EN_1}
	}
};

static int gpio_cmds_tx_check_param(struct gpio_desc *cmds, int index)
{
	if ((cmds == NULL) || (cmds->label == NULL)) {
		DPU_FB_ERR("cmds or cmds->label is NULL! index=%d\n", index);
		return -1;
	}

	if (!gpio_is_valid(*(cmds->gpio))) {
		DPU_FB_ERR("gpio invalid, dtype=%d, lable=%s, gpio=%d!\n",
			cmds->dtype, cmds->label, *(cmds->gpio));
		return -1;
	}

	return 0;
}

static void cmds_tx_delay(int wait, int waittype)
{
	if (wait) {
		if (waittype == WAIT_TYPE_US)
			udelay(wait);
		else if (waittype == WAIT_TYPE_MS)
			mdelay(wait);
		else
			mdelay(wait * 1000);  /* ms */
	}
}

int gpio_cmds_tx(struct gpio_desc *cmds, int cnt)
{
	int ret;
	int i;

	struct gpio_desc *cm = cmds;

	for (i = 0; i < cnt; i++) {
		ret = gpio_cmds_tx_check_param(cm, i);
		if (ret)
			goto error;

		if (cm->dtype == DTYPE_GPIO_INPUT) {
			if (gpio_direction_input(*(cm->gpio)) != 0) {
				DPU_FB_ERR("failed to gpio_direction_input, lable=%s, gpio=%d!\n",
					cm->label, *(cm->gpio));
				ret = -1;
				goto error;
			}
		} else if (cm->dtype == DTYPE_GPIO_OUTPUT) {
			if (gpio_direction_output(*(cm->gpio), cm->value) != 0) {
				DPU_FB_ERR("failed to gpio_direction_output, label%s, gpio=%d!\n",
					cm->label, *(cm->gpio));
				ret = -1;
				goto error;
			}
		} else if (cm->dtype == DTYPE_GPIO_REQUEST) {
			if (gpio_request(*(cm->gpio), cm->label) != 0) {
				DPU_FB_ERR("failed to gpio_request, lable=%s, gpio=%d!\n",
					cm->label, *(cm->gpio));
				ret = -1;
				goto error;
			}
		} else if (cm->dtype == DTYPE_GPIO_FREE) {
			gpio_free(*(cm->gpio));
		} else {
			DPU_FB_ERR("dtype=%x NOT supported\n", cm->dtype);
			ret = -1;
			goto error;
		}

		cmds_tx_delay(cm->wait, cm->waittype);
		cm++;
	}

	return 0;

error:
	return ret;
}

int resource_cmds_tx(struct platform_device *pdev,
	struct resource_desc *cmds, int cnt)
{
	int ret = 0;
	struct resource *res = NULL;
	struct resource_desc *cm = NULL;
	int i;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return -EINVAL;
	}
	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if ((cm == NULL) || (cm->name == NULL)) {
			DPU_FB_ERR("cm or cm->name is NULL! index=%d\n", i);
			ret = -1;
			goto error;
		}

		res = platform_get_resource_byname(pdev, cm->flag, cm->name);
		if (!res) {
			DPU_FB_ERR("failed to get %s resource!\n", cm->name);
			ret = -1;
			goto error;
		}

		*(cm->value) = res->start;

		cm++;
	}

error:
	return ret;
}

int spi_cmds_tx(struct spi_device *spi, struct spi_cmd_desc *cmds, int cnt)
{
	void_unused(cmds);
	void_unused(spi);
	void_unused(cnt);
	return 0;
}

int vcc_cmds_tx(struct platform_device *pdev, struct vcc_desc *cmds, int cnt)
{
	struct vcc_desc *cm = NULL;
	int i;

	if (g_fpga_flag == 1)
		return 0;

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		dpu_check_and_return((!cm || !cm->id), -1, ERR, "cm or cm->id is NULL! index=%d\n", i);

		if (cm->dtype == DTYPE_VCC_GET) {
			dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");

			*(cm->regulator) = devm_regulator_get(&pdev->dev, cm->id);
			dpu_check_and_return(IS_ERR(*(cm->regulator)), -1,
				ERR, "failed to get %s regulator!\n", cm->id);
		} else if (cm->dtype == DTYPE_VCC_PUT) {
			if (!IS_ERR(*(cm->regulator)))
				devm_regulator_put(*(cm->regulator));
		} else if (cm->dtype == DTYPE_VCC_ENABLE) {
			if (!IS_ERR(*(cm->regulator)) &&
				regulator_enable(*(cm->regulator)) != 0) {
					DPU_FB_ERR("failed to enable %s regulator!\n", cm->id);
					return -1;
			}
		} else if (cm->dtype == DTYPE_VCC_DISABLE) {
			if (!IS_ERR(*(cm->regulator)) &&
				regulator_disable(*(cm->regulator)) != 0) {
					DPU_FB_ERR("failed to disable %s regulator!\n", cm->id);
					return -1;
			}
		} else if (cm->dtype == DTYPE_VCC_SET_VOLTAGE) {
			if (!IS_ERR(*(cm->regulator)) &&
				regulator_set_voltage(*(cm->regulator), cm->min_uV, cm->max_uV) != 0) {
					DPU_FB_ERR("failed to set %s regulator voltage!\n", cm->id);
					return -1;
			}
		} else {
			DPU_FB_ERR("dtype=%x NOT supported\n", cm->dtype);
			return -1;
		}

		cmds_tx_delay(cm->wait, cm->waittype);
		cm++;
	}

	return 0;
}

static int pinctrl_get_state_by_mode(struct pinctrl_cmd_desc *cmds, int index)
{
	if (cmds->mode == DTYPE_PINCTRL_STATE_DEFAULT) {
		cmds->pctrl_data->pinctrl_def = pinctrl_lookup_state(cmds->pctrl_data->p, PINCTRL_STATE_DEFAULT);
		if (IS_ERR(cmds->pctrl_data->pinctrl_def)) {
			DPU_FB_ERR("failed to get pinctrl_def, index=%d!\n", index);
			return -1;
		}
	} else if (cmds->mode == DTYPE_PINCTRL_STATE_IDLE) {
		cmds->pctrl_data->pinctrl_idle = pinctrl_lookup_state(cmds->pctrl_data->p, PINCTRL_STATE_IDLE);
		if (IS_ERR(cmds->pctrl_data->pinctrl_idle)) {
			DPU_FB_ERR("failed to get pinctrl_idle, index=%d!\n", index);
			return -1;
		}
	} else {
		DPU_FB_ERR("unknown pinctrl type to get!\n");
		return -1;
	}

	return 0;
}

static int pinctrl_set_state_by_mode(struct pinctrl_cmd_desc *cmds)
{
	int ret;

	if (cmds->mode == DTYPE_PINCTRL_STATE_DEFAULT) {
		if (cmds->pctrl_data->p && cmds->pctrl_data->pinctrl_def) {
			ret = pinctrl_select_state(cmds->pctrl_data->p, cmds->pctrl_data->pinctrl_def);
			if (ret) {
				DPU_FB_ERR("could not set this pin to default state!\n");
				return ret;
			}
		}
	} else if (cmds->mode == DTYPE_PINCTRL_STATE_IDLE) {
		if (cmds->pctrl_data->p && cmds->pctrl_data->pinctrl_idle) {
			ret = pinctrl_select_state(cmds->pctrl_data->p, cmds->pctrl_data->pinctrl_idle);
			if (ret) {
				DPU_FB_ERR("could not set this pin to idle state!\n");
				return ret;
			}
		}
	} else {
		ret = -1;
		DPU_FB_ERR("unknown pinctrl type to set!\n");
		return ret;
	}

	return 0;
}

int pinctrl_cmds_tx(struct platform_device *pdev, struct pinctrl_cmd_desc *cmds, int cnt)
{
	int ret;
	int i;
	struct pinctrl_cmd_desc *cm = NULL;

	if (g_fpga_flag == 1)
		return 0;

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (!cm) {
			DPU_FB_ERR("cm is NULL! index=%d\n", i);
			continue;
		}

		if (cm->dtype == DTYPE_PINCTRL_GET) {
			dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");

			cm->pctrl_data->p = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR(cm->pctrl_data->p)) {
				ret = -1;
				DPU_FB_ERR("failed to get p, index=%d!\n", i);
				goto err;
			}
		} else if (cm->dtype == DTYPE_PINCTRL_STATE_GET) {
			ret = pinctrl_get_state_by_mode(cm, i);
			if (ret)
				goto err;
		} else if (cm->dtype == DTYPE_PINCTRL_SET) {
			ret = pinctrl_set_state_by_mode(cm);
			if (ret) {
				ret = -1;
				goto err;
			}
		} else if (cm->dtype == DTYPE_PINCTRL_PUT) {
			if (cm->pctrl_data->p)
				pinctrl_put(cm->pctrl_data->p);
		} else {
			DPU_FB_ERR("not supported command type!\n");
			ret = -1;
			goto err;
		}

		cm++;
	}

	return 0;

err:
	return ret;
}

int panel_next_set_fastboot(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");

	pdata = (struct dpu_fb_panel_data *)pdev->dev.platform_data;
	if (pdata != NULL) {
		next_pdev = pdata->next;
		if (next_pdev != NULL) {
			next_pdata = (struct dpu_fb_panel_data *)next_pdev->dev.platform_data;
			if ((next_pdata) && (next_pdata->set_fastboot))
				ret = next_pdata->set_fastboot(next_pdev);
		}
	}

	return ret;
}

int panel_next_on(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->on))
			ret = next_pdata->on(next_pdev);
	}

	return ret;
}

int panel_next_off(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->off))
			ret = next_pdata->off(next_pdev);
	}

	return ret;
}

int panel_next_lp_ctrl(struct platform_device *pdev, bool lp_enter)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->lp_ctrl))
			ret = next_pdata->lp_ctrl(next_pdev, lp_enter);
	}

	return ret;
}

int panel_next_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->remove))
			ret = next_pdata->remove(next_pdev);
	}

	return ret;
}

int panel_next_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->set_backlight))
			ret = next_pdata->set_backlight(next_pdev, bl_level);
	}

	return ret;
}

int panel_next_vsync_ctrl(struct platform_device *pdev, int enable)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->vsync_ctrl))
			ret = next_pdata->vsync_ctrl(next_pdev, enable);
	}

	return ret;
}

int panel_next_lcd_fps_scence_handle(struct platform_device *pdev, uint32_t scence)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->lcd_fps_scence_handle))
			ret = next_pdata->lcd_fps_scence_handle(next_pdev, scence);
	}

	return ret;
}

int panel_next_lcd_fps_updt_handle(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->lcd_fps_updt_handle))
			ret = next_pdata->lcd_fps_updt_handle(next_pdev);
	}

	return ret;
}

int panel_next_esd_handle(struct platform_device *pdev)
{
	int ret = 0;
	struct dpu_fb_panel_data *pdata = NULL;
	struct dpu_fb_panel_data *next_pdata = NULL;
	struct platform_device *next_pdev = NULL;

	dpu_check_and_return(!pdev, -EINVAL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	next_pdev = pdata->next;
	if (next_pdev != NULL) {
		next_pdata = dev_get_platdata(&next_pdev->dev);
		if ((next_pdata) && (next_pdata->esd_handle))
			ret = next_pdata->esd_handle(next_pdev);
	}

	return ret;
}

bool is_mipi_cmd_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if (dpufd->panel_info.type & (PANEL_MIPI_CMD | PANEL_DUAL_MIPI_CMD))
		return true;

	return false;
}

bool is_mipi_cmd_panel_ext(struct dpu_panel_info *pinfo)
{
	if (!pinfo) {
		DPU_FB_ERR("pinfo is NULL\n");
		return false;
	}

	if (pinfo->type & (PANEL_MIPI_CMD | PANEL_DUAL_MIPI_CMD))
		return true;

	return false;
}

bool is_mipi_video_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if (dpufd->panel_info.type & (PANEL_MIPI_VIDEO | PANEL_DUAL_MIPI_VIDEO | PANEL_RGB2MIPI))
		return true;

	return false;
}

bool is_mipi_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if (dpufd->panel_info.type & (PANEL_MIPI_VIDEO | PANEL_MIPI_CMD |
		PANEL_DUAL_MIPI_VIDEO | PANEL_DUAL_MIPI_CMD))
		return true;

	return false;
}

bool is_dual_mipi_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if (dpufd->panel_info.type & (PANEL_DUAL_MIPI_VIDEO | PANEL_DUAL_MIPI_CMD))
		return true;

	return false;
}

bool need_config_dsi0(struct dpu_fb_data_type *dpufd)
{
	if ((dpufd->index == PRIMARY_PANEL_IDX) &&
		(is_dual_mipi_panel(dpufd) || is_dsi0_pipe_switch_connector(dpufd)))
		return true;

	return false;
}

bool need_config_dsi1(struct dpu_fb_data_type *dpufd)
{
	if (is_dual_mipi_panel(dpufd) ||
		(dpufd->index == EXTERNAL_PANEL_IDX) ||
		(is_dsi1_pipe_switch_connector(dpufd)))
		return true;

	return false;
}

bool is_dual_mipi_panel_ext(struct dpu_panel_info *pinfo)
{
	if (!pinfo) {
		DPU_FB_ERR("pinfo is NULL\n");
		return false;
	}

	if (pinfo->type & (PANEL_DUAL_MIPI_VIDEO | PANEL_DUAL_MIPI_CMD))
		return true;

	return false;
}

bool is_dpu_writeback_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if (dpufd->panel_info.type & PANEL_WRITEBACK)
		return true;

	return false;
}

bool is_ifbc_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if (dpufd->panel_info.ifbc_type != IFBC_TYPE_NONE)
		return true;

	return false;
}

bool is_ifbc_vesa_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if ((dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA2X_SINGLE) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA3X_SINGLE) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA2X_DUAL) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA3X_DUAL) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA3_75X_DUAL) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA4X_SINGLE_SPR) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA4X_DUAL_SPR) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA3_75X_SINGLE) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA2X_SINGLE_SPR) ||
		(dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA2X_DUAL_SPR) ||
		dpufd->panel_info.dynamic_dsc_support)
		return true;

	return false;
}

bool is_single_slice_dsc_panel(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if ((dpufd->panel_info.ifbc_type == IFBC_MODE_VESA3X_SINGLE) ||
		(dpufd->panel_info.ifbc_type == IFBC_MODE_VESA3_75X_SINGLE))
		return true;

	return false;
}

bool is_dsi0_pipe_switch_connector(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if ((dpufd->panel_info.mipi.dsi_te_type == DSI0_TE0_TYPE) &&
		(g_dsi_pipe_switch_connector == PIPE_SWITCH_CONNECT_DSI0))
		return true;

	return false;
}

bool is_dsi1_pipe_switch_connector(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return false;
	}

	if ((g_dsi_pipe_switch_connector == PIPE_SWITCH_CONNECT_DSI1) ||
		(dpufd->panel_info.mipi.dsi_te_type == DSI1_TE1_TYPE))
		return true;

	return false;
}

bool mipi_panel_check_reg(struct dpu_fb_data_type *dpufd, uint32_t *read_value, int buf_len)
{
	int ret;
	char __iomem *mipi_dsi_base = NULL;
	/* mipi reg default value */
	char lcd_reg_05[] = {0x05};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0e[] = {0x0e};
	char lcd_reg_0f[] = {0x0f};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_05), lcd_reg_05},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0f), lcd_reg_0f},
	};

	if (dpufd->index == PRIMARY_PANEL_IDX)
		mipi_dsi_base = get_mipi_dsi_base(dpufd);
	else if (dpufd->index == EXTERNAL_PANEL_IDX)
		mipi_dsi_base = dpufd->mipi_dsi1_base;

	ret = mipi_dsi_cmds_rx_with_check_fifo(read_value, lcd_check_reg, buf_len, mipi_dsi_base);
	if (ret) {
		DPU_FB_ERR("Read error number: %d\n", ret);
		return false;
	}

	return true;
}

/*lint -save -e573*/
int mipi_ifbc_get_rect(struct dpu_fb_data_type *dpufd, struct dss_rect *rect)
{
	uint32_t ifbc_type;
	uint32_t mipi_idx;
	uint32_t xres_div;
	uint32_t yres_div;
	struct panel_dsc_info *panel_dsc_info = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");
	dpu_check_and_return(!rect, -EINVAL, ERR, "rect is NULL\n");

	ifbc_type = dpufd->panel_info.ifbc_type;
	panel_dsc_info = &(dpufd->panel_info.panel_dsc_info);
	dpu_check_and_return((ifbc_type >= IFBC_TYPE_MAX), -EINVAL, ERR, "ifbc_type is invalid\n");

	mipi_idx = is_dual_mipi_panel(dpufd) ? 1 : 0;

	xres_div = g_mipi_ifbc_division[mipi_idx][ifbc_type].xres_div;
	yres_div = g_mipi_ifbc_division[mipi_idx][ifbc_type].yres_div;

	if ((rect->w % xres_div) > 0)
		DPU_FB_INFO("fb%d, xres: %d is not division_h: %d pixel aligned!\n", dpufd->index, rect->w, xres_div);

	if ((rect->h % yres_div) > 0)
		DPU_FB_ERR("fb%d, yres: %d is not division_v: %d pixel aligned!\n", dpufd->index, rect->h, yres_div);

	/* [NOTE] rsp3x && single_mipi CMD mode amended xres_div = 1.5, */
	/* yres_div = 2 ,VIDEO mode amended xres_div = 3, yres_div = 1 */
	if ((mipi_idx == 0) && (ifbc_type == IFBC_TYPE_RSP3X)) {
		rect->w *= 2;
		rect->h /= 2;
	}
	dpu_check_and_return((xres_div == 0), -EINVAL, ERR, "xres_div is 0\n");
	if ((dpufd->panel_info.mode_switch_to == MODE_10BIT_VIDEO_3X)
		&& (dpufd->panel_info.ifbc_type == IFBC_TYPE_VESA3X_DUAL)) {
		rect->w = rect->w * 30 / 24 / xres_div;
	} else {
		if ((rect->w % xres_div) > 0)
			rect->w = ((panel_dsc_info->dsc_info.chunk_size + panel_dsc_info->dsc_insert_byte_num)
			* (panel_dsc_info->dual_dsc_en + 1)) / xres_div;
		else
			rect->w /= xres_div;
	}

	rect->h /= yres_div;

	return 0;
}

/*lint -restore*/
void dpufb_snd_cmd_before_frame(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	if (!dpufd->panel_info.snd_cmd_before_frame_support)
		return;

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	dpu_check_and_no_retval(!pdata, ERR, "pdata is NULL\n");

	if (pdata->snd_cmd_before_frame != NULL)
		pdata->snd_cmd_before_frame(dpufd->pdev);
}

static void dpu_fb_lcdc_rgb_resource_ready(uint32_t bl_type, bool *flag)
{
	if ((g_dts_resouce_ready & DTS_FB_RESOURCE_INIT_READY) && (g_dts_resouce_ready & DTS_SPI_READY)) {
		if (bl_type & (BL_SET_BY_PWM | BL_SET_BY_BLPWM)) {
			if (g_dts_resouce_ready & DTS_PWM_READY)
				*flag = false;
		} else {
			*flag = false;
		}
	}
}

static void dpu_fb_mipi_resource_ready(uint32_t bl_type, bool *flag)
{
	if (g_dts_resouce_ready & DTS_FB_RESOURCE_INIT_READY) {
		if (bl_type & (BL_SET_BY_PWM | BL_SET_BY_BLPWM)) {
			if (g_dts_resouce_ready & DTS_PWM_READY)
				*flag = false;
		} else {
			*flag = false;
		}
	}
}

bool dpu_fb_device_probe_defer(uint32_t panel_type, uint32_t bl_type)
{
	bool flag = true;

	down(&dpu_fb_dts_resource_sem);

	switch (panel_type) {
	case PANEL_NO:
		if (g_dts_resouce_ready & DTS_FB_RESOURCE_INIT_READY)
			flag = false;
		break;
	case PANEL_LCDC:
	case PANEL_MIPI2RGB:
	case PANEL_RGB2MIPI:
		dpu_fb_lcdc_rgb_resource_ready(bl_type, &flag);
		break;
	case PANEL_MIPI_VIDEO:
	case PANEL_MIPI_CMD:
	case PANEL_DUAL_MIPI_VIDEO:
	case PANEL_DUAL_MIPI_CMD:
		dpu_fb_mipi_resource_ready(bl_type, &flag);
		break;
	case PANEL_HDMI:
	case PANEL_DP:
		if (g_dts_resouce_ready & DTS_PANEL_PRIMARY_READY)
			flag = false;
		break;
	case PANEL_OFFLINECOMPOSER:
		if (g_dts_resouce_ready & DTS_PANEL_EXTERNAL_READY)
			flag = false;
		break;
	case PANEL_WRITEBACK:
		if (g_dts_resouce_ready & DTS_PANEL_OFFLINECOMPOSER_READY)
			flag = false;
		break;
	case PANEL_MEDIACOMMON:
		if (g_dts_resouce_ready & DTS_PANEL_OFFLINECOMPOSER_READY)
			flag = false;
		break;
	default:
		DPU_FB_ERR("not support this panel type: %d\n", panel_type);
		break;
	}

	up(&dpu_fb_dts_resource_sem);

	return flag;
}

void dpu_fb_device_set_status0(uint32_t status)
{
	down(&dpu_fb_dts_resource_sem);
	g_dts_resouce_ready |= status;
	up(&dpu_fb_dts_resource_sem);
}

int dpu_fb_device_set_status1(struct dpu_fb_data_type *dpufd)
{
	int ret = 0;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	down(&dpu_fb_dts_resource_sem);

	switch (dpufd->panel_info.type) {
	case PANEL_LCDC:
	case PANEL_MIPI_VIDEO:
	case PANEL_MIPI_CMD:
	case PANEL_DUAL_MIPI_VIDEO:
	case PANEL_DUAL_MIPI_CMD:
	case PANEL_DP:
	case PANEL_MIPI2RGB:
	case PANEL_RGB2MIPI:
	case PANEL_HDMI:
		if (dpufd->index == PRIMARY_PANEL_IDX)
			g_dts_resouce_ready |= DTS_PANEL_PRIMARY_READY;
		else if (dpufd->index == EXTERNAL_PANEL_IDX)
			g_dts_resouce_ready |= DTS_PANEL_EXTERNAL_READY;
		else
			DPU_FB_ERR("not support fb%d\n", dpufd->index);
		break;
	case PANEL_OFFLINECOMPOSER:
		g_dts_resouce_ready |= DTS_PANEL_OFFLINECOMPOSER_READY;
		break;
	case PANEL_WRITEBACK:
		g_dts_resouce_ready |= DTS_PANEL_WRITEBACK_READY;
		break;
	case PANEL_MEDIACOMMON:
		g_dts_resouce_ready |= DTS_PANEL_MEDIACOMMON_READY;
		break;
	default:
		DPU_FB_ERR("not support this panel type: %d\n", dpufd->panel_info.type);
		ret = -1;
		break;
	}

	up(&dpu_fb_dts_resource_sem);

	return ret;
}

struct platform_device *dpu_fb_device_alloc(struct dpu_fb_panel_data *pdata,
	uint32_t type, uint32_t id)
{
	struct platform_device *this_dev = NULL;
	char dev_name[32] = {0};  /* device name buf len */

	if (!pdata) {
		DPU_FB_ERR("pdata is NULL");
		return NULL;
	}

	switch (type) {
	case PANEL_MIPI_VIDEO:
	case PANEL_MIPI_CMD:
	case PANEL_DUAL_MIPI_VIDEO:
	case PANEL_DUAL_MIPI_CMD:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_MIPIDSI);
		break;
	case PANEL_DP:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_DP);
		break;
	case PANEL_NO:
	case PANEL_LCDC:
	case PANEL_HDMI:
	case PANEL_OFFLINECOMPOSER:
	case PANEL_WRITEBACK:
	case PANEL_MEDIACOMMON:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_DSS_DPE);
		break;
	case PANEL_RGB2MIPI:
		snprintf(dev_name, sizeof(dev_name), DEV_NAME_RGB2MIPI);
		break;
	default:
		DPU_FB_ERR("invalid panel type = %d!\n", type);
		return NULL;
	}

	if (pdata != NULL)
		pdata->next = NULL;

	this_dev = platform_device_alloc(dev_name, (((uint32_t)type << 16) | (uint32_t)id));
	if (this_dev != NULL) {
		if (platform_device_add_data(this_dev, pdata, sizeof(struct dpu_fb_panel_data))) {
			DPU_FB_ERR("failed to platform_device_add_data!\n");
			platform_device_put(this_dev);
			this_dev = NULL;
			return this_dev;
		}
	}

	return this_dev;
}
/*lint +e574 +e647 +e568 +e685 +e578*/
