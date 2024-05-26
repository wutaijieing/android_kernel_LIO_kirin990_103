/*********************************************************************
 * Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
 * File name    hispipp_orb_enh.c
 * Description:
 *
 * Date         2020-04-11 20:37:22
 ***********************************************************************/
#include <linux/printk.h>
#include "ipp.h"
#include "orb_enh_drv.h"
#include "orb_enh_drv_priv.h"
#include "orb_enh_reg_offset.h"
#include "orb_enh_offset_field.h"

#define LOG_TAG LOG_MODULE_ORB_ENH_DRV

static int orb_enh_set_ctrl(
	struct orb_enh_dev_t  *dev, struct orb_enh_ctrl_cfg_t *ctrl)
{
	union u_enable_cfg temp;

	if (dev == NULL || ctrl == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.enh_en = ctrl->enh_en;
	temp.bits.gsblur_en = ctrl->gsblur_en;
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_ENABLE_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int orb_enh_set_compute_hist(
	struct orb_enh_dev_t *dev,
	struct orb_enh_cal_hist_cfg_t  *cal_hist_cfg)
{
	union u_blk_num  temp_num;
	union u_blk_size temp_size;
	union u_extend_cfg temp_extend;
	union u_hist_climit temp_climit;

	if (dev == NULL || cal_hist_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_num.u32 = 0;
	temp_size.u32 = 0;
	temp_extend.u32 = 0;
	temp_climit.u32 = 0;
	temp_num.bits.blknumy = cal_hist_cfg->blknumy;
	temp_num.bits.blknumx = cal_hist_cfg->blknumx;
	temp_size.bits.blk_ysize = cal_hist_cfg->blk_ysize;
	temp_size.bits.blk_xsize = cal_hist_cfg->blk_xsize;
	temp_extend.bits.edgeex_y = cal_hist_cfg->edgeex_y;
	temp_extend.bits.edgeex_x = cal_hist_cfg->edgeex_x;
	temp_climit.bits.climit = cal_hist_cfg->climit;
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_BLK_NUM_REG, temp_num.u32);
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_BLK_SIZE_REG, temp_size.u32);
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_EXTEND_CFG_REG, temp_extend.u32);
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_HIST_CLIMIT_REG, temp_climit.u32);
	return ISP_IPP_OK;
}

static int orb_enh_set_adjust_hist(
	struct orb_enh_dev_t *dev,
	struct orb_enh_adjust_hist_cfg_t *adjust_hist_cfg)
{
	union u_lut_scale temp;

	if (dev == NULL || adjust_hist_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.lutscale = adjust_hist_cfg->lutscale;
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_LUT_SCALE_REG, temp.u32);
	return ISP_IPP_OK;
}

static int orb_enh_set_adjust_image(
	struct orb_enh_dev_t *dev,
	struct orb_enh_adjust_image_cfg_t *adjust_image_cfg)
{
	union u_inv_blk_size_y temp_y;
	union u_inv_blk_size_x temp_x;

	if (dev == NULL || adjust_image_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_y.u32 = 0;
	temp_x.u32 = 0;
	temp_y.bits.inv_ysize = adjust_image_cfg->inv_ysize;
	temp_x.bits.inv_xsize = adjust_image_cfg->inv_xsize;
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_INV_BLK_SIZE_Y_REG, temp_y.u32);
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_INV_BLK_SIZE_X_REG, temp_x.u32);
	return ISP_IPP_OK;
}

static int orb_enh_set_gsblur_coef(
	struct orb_enh_dev_t *dev,
	struct orb_enh_gsblur_coef_cfg_t *gsblur_coef)

{
	union u_gsblur_coef_01 temp_01;
	union u_gsblur_coef_23 temp_23;

	if (dev == NULL || gsblur_coef == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_01.u32 = 0;
	temp_23.u32 = 0;
	temp_01.bits.gauss_coeff_0 = gsblur_coef->coeff_gauss_0;
	temp_01.bits.gauss_coeff_1 = gsblur_coef->coeff_gauss_1;
	temp_23.bits.gauss_coeff_2 = gsblur_coef->coeff_gauss_2;
	temp_23.bits.gauss_coeff_3 = gsblur_coef->coeff_gauss_3;
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_GSBLUR_COEF_01_REG,
						 temp_01.u32);
	macro_cmdlst_set_reg(dev->base_addr + ORB_ENH_GSBLUR_COEF_23_REG,
						 temp_23.u32);
	return ISP_IPP_OK;
}

static int orb_enh_do_config(
	struct orb_enh_dev_t *dev, struct cfg_tab_orb_enh_t *tab_orb_enh)
{
	if (dev == NULL || tab_orb_enh == NULL)
		return ISP_IPP_ERR;

	if (tab_orb_enh->to_use) {
		tab_orb_enh->to_use = 0;

		if (tab_orb_enh->cal_hist_cfg.to_use) {
			tab_orb_enh->cal_hist_cfg.to_use = 0;
			loge_if_ret(orb_enh_set_compute_hist(dev, &(tab_orb_enh->cal_hist_cfg)));
		}

		if (tab_orb_enh->adjust_image_cfg.to_use) {
			tab_orb_enh->adjust_image_cfg.to_use = 0;
			loge_if_ret(orb_enh_set_adjust_image(dev, &(tab_orb_enh->adjust_image_cfg)));
		}

		if (tab_orb_enh->adjust_hist_cfg.to_use) {
			tab_orb_enh->adjust_hist_cfg.to_use = 0;
			loge_if_ret(orb_enh_set_adjust_hist(dev, &(tab_orb_enh->adjust_hist_cfg)));
		}

		if (tab_orb_enh->gsblur_coef_cfg.to_use) {
			tab_orb_enh->gsblur_coef_cfg.to_use = 0;
			loge_if_ret(orb_enh_set_gsblur_coef(dev, &(tab_orb_enh->gsblur_coef_cfg)));
		}

		if (tab_orb_enh->ctrl_cfg.to_use) {
			tab_orb_enh->ctrl_cfg.to_use = 0;
			loge_if_ret(orb_enh_set_ctrl(dev, &(tab_orb_enh->ctrl_cfg)));
		}
	}

	return ISP_IPP_OK;
}

int orb_enh_prepare_cmd(
	struct orb_enh_dev_t *dev, struct cmd_buf_t *cmd_buf,
	struct cfg_tab_orb_enh_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(orb_enh_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct orb_enh_ops_t orb_enh_ops = {
	.prepare_cmd   = orb_enh_prepare_cmd,
};

struct orb_enh_dev_t g_orb_enh_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_ORB_ENH,
		.ops = &orb_enh_ops,
	},
};

/* ********************************end***************************** */
