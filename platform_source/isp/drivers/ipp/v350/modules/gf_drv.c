/*********************************************************************
 * Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
 * File name    hispipp_gf.c
 * Description:
 *
 * Date         2020-04-11 20:37:22
 ***********************************************************************/
#include <linux/printk.h>
#include "ipp.h"
#include "gf_drv.h"
#include "gf_drv_priv.h"
#include "gf_reg_offset.h"
#include "gf_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_GF_DRV

static int gf_size_config(struct gf_dev_t *dev, struct gf_size_cfg_t *ctrl)
{
	union u_gf_image_size temp;

	if (dev == NULL || ctrl == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.width = ctrl->width;
	temp.bits.height = ctrl->height;
	macro_cmdlst_set_reg(dev->base_addr  + GF_IMAGE_SIZE_REG, temp.u32);
	return ISP_IPP_OK;
}

static int gf_set_mode(struct gf_dev_t *dev, struct gf_mode_cfg_t *mode_cfg)
{
	union u_gf_mode temp;

	if (dev == NULL || mode_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mode_in = mode_cfg->mode_in;
	temp.bits.mode_out = mode_cfg->mode_out;
	macro_cmdlst_set_reg(dev->base_addr  + GF_MODE_REG, temp.u32);
	return ISP_IPP_OK;
}

static int gf_set_crop(struct gf_dev_t *dev, struct gf_crop_cfg_t *crop_cfg)
{
	union u_gf_crop_h temp_h;
	union u_gf_crop_v temp_v;

	if (dev == NULL || crop_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_h.u32 = 0;
	temp_v.u32 = 0;
	temp_h.bits.crop_right = crop_cfg->crop_right;
	temp_h.bits.crop_left = crop_cfg->crop_left;
	temp_v.bits.crop_bottom = crop_cfg->crop_bottom;
	temp_v.bits.crop_top = crop_cfg->crop_top;
	macro_cmdlst_set_reg(dev->base_addr  + GF_CROP_H_REG, temp_h.u32);
	macro_cmdlst_set_reg(dev->base_addr  + GF_CROP_V_REG, temp_v.u32);
	return ISP_IPP_OK;
}

static int gf_set_filter_coeff(struct gf_dev_t *dev, struct gf_filter_coeff_cfg_t *coeff_cfg)
{
	union u_gf_filter_coeff temp;

	if (dev == NULL || coeff_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.radius = coeff_cfg->radius;
	temp.bits.eps = coeff_cfg->eps;
	macro_cmdlst_set_reg(dev->base_addr  + GF_FILTER_COEFF_REG, temp.u32);
	return ISP_IPP_OK;
}

static int gf_do_config(struct gf_dev_t *dev, struct gf_config_table_t *cfg_tab)
{
	if (dev == NULL || cfg_tab == NULL)
		return ISP_IPP_ERR;

	if (cfg_tab->to_use) {
		cfg_tab->to_use = 0;

		if (cfg_tab->size_cfg.to_use) {
			cfg_tab->size_cfg.to_use = 0;
			loge_if_ret(gf_size_config(dev, &(cfg_tab->size_cfg)));
		}

		if (cfg_tab->mode_cfg.to_use) {
			cfg_tab->mode_cfg.to_use = 0;
			loge_if_ret(gf_set_mode(dev, &(cfg_tab->mode_cfg)));
		}

		if (cfg_tab->crop_cfg.to_use) {
			cfg_tab->crop_cfg.to_use = 0;
			loge_if_ret(gf_set_crop(dev, &(cfg_tab->crop_cfg)));
		}

		if (cfg_tab->coeff_cfg.to_use) {
			cfg_tab->coeff_cfg.to_use = 0;
			loge_if_ret(gf_set_filter_coeff(dev, &(cfg_tab->coeff_cfg)));
		}
	}

	return ISP_IPP_OK;
}

int gf_prepare_cmd(struct gf_dev_t *dev,
				   struct cmd_buf_t *cmd_buf,
				   struct gf_config_table_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(gf_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct gf_ops_t gf_ops = {
	.prepare_cmd   = gf_prepare_cmd,
};

struct gf_dev_t g_gf_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_GF,
		.ops = &gf_ops,
	},
};

/* ********************************end***************************** */
