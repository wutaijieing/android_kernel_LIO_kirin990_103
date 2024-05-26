
#include <linux/printk.h>
#include "ipp.h"
#include "gf_drv.h"
#include "gf_drv_priv.h"
#include "gf_reg_offset.h"
#include "gf_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_GF_DRV

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

static int gf_size_config(struct gf_dev_t *dev, struct gf_size_cfg_t *ctrl)
{
	union U_GF_IMAGE_SIZE temp;

	if (dev == NULL || ctrl == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.width = ctrl->width;
	temp.bits.height = ctrl->height;
	CMDLST_SET_REG(dev->base_addr  + GF_IMAGE_SIZE_REG, temp.u32);
	return ISP_IPP_OK;
}

static int gf_set_mode(struct gf_dev_t *dev, struct gf_mode_cfg_t *mode_cfg)
{
	union U_GF_MODE temp;

	if (dev == NULL || mode_cfg == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mode_in = mode_cfg->mode_in;
	temp.bits.mode_out = mode_cfg->mode_out;
	CMDLST_SET_REG(dev->base_addr  + GF_MODE_REG, temp.u32);

	return ISP_IPP_OK;
}

static int gf_set_crop(struct gf_dev_t *dev, struct gf_crop_cfg_t *crop_cfg)
{
	union U_GF_CROP_H temp_h;
	union U_GF_CROP_V temp_v;

	if (dev == NULL || crop_cfg == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_h.u32 = 0;
	temp_v.u32 = 0;
	temp_h.bits.crop_right = crop_cfg->crop_right;
	temp_h.bits.crop_left = crop_cfg->crop_left;
	temp_v.bits.crop_bottom = crop_cfg->crop_bottom;
	temp_v.bits.crop_top = crop_cfg->crop_top;
	CMDLST_SET_REG(dev->base_addr  + GF_CROP_H_REG, temp_h.u32);
	CMDLST_SET_REG(dev->base_addr  + GF_CROP_V_REG, temp_v.u32);
	return ISP_IPP_OK;
}

static int gf_set_filter_coeff(struct gf_dev_t *dev,
	struct gf_filter_coeff_cfg_t *coeff_cfg)
{
	union U_GF_FILTER_COEFF temp;

	if (dev == NULL || coeff_cfg == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.radius = coeff_cfg->radius;
	temp.bits.eps = coeff_cfg->eps;
	CMDLST_SET_REG(dev->base_addr  + GF_FILTER_COEFF_REG, temp.u32);
	return ISP_IPP_OK;
}

static int gf_do_config(struct gf_dev_t *dev,
	struct gf_config_table_t *cfg_tab)
{
	if (dev == NULL || cfg_tab == NULL)
		return ISP_IPP_ERR;

	if (cfg_tab->to_use) {
		if (cfg_tab->size_cfg.to_use)
			loge_if_ret(gf_size_config(dev,
			&(cfg_tab->size_cfg)));

		if (cfg_tab->mode_cfg.to_use)
			loge_if_ret(gf_set_mode(dev,
			&(cfg_tab->mode_cfg)));

		if (cfg_tab->crop_cfg.to_use)
			loge_if_ret(gf_set_crop(dev,
			&(cfg_tab->crop_cfg)));

		if (cfg_tab->coeff_cfg.to_use)
			loge_if_ret(gf_set_filter_coeff(dev,
			&(cfg_tab->coeff_cfg)));
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

#pragma GCC diagnostic pop
/* ********************************end***************************** */
