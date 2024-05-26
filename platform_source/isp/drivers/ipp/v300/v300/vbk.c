
#include <linux/printk.h>
#include "ipp.h"
#include "vbk_drv.h"
#include "vbk_drv_priv.h"
#include "vbk_reg_offset.h"
#include "vbk_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_VBK_DRV

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

static int vbk_basic_config(
struct vbk_dev_t *dev, struct vbk_ctrl_cfg_t *vbk_basic_cfg)
{
	union U_VBK_CFG temp_vbk_cfg;

	if ((dev == NULL) || (vbk_basic_cfg == NULL))
		return ISP_IPP_ERR;

	temp_vbk_cfg.u32 = 0;
	temp_vbk_cfg.bits.op_mode = vbk_basic_cfg->op_mode;
	temp_vbk_cfg.bits.y_ds16_gauss_en = vbk_basic_cfg->y_ds16_gauss_en;
	temp_vbk_cfg.bits.uv_ds16_gauss_en = vbk_basic_cfg->uv_ds16_gauss_en;
	temp_vbk_cfg.bits.sigma_gauss_en = vbk_basic_cfg->sigma_gauss_en;
	temp_vbk_cfg.bits.sigma2alpha_en = vbk_basic_cfg->sigma2alpha_en;
	temp_vbk_cfg.bits.in_width_ds4 = vbk_basic_cfg->in_width_ds4;
	temp_vbk_cfg.bits.in_height_ds4 = vbk_basic_cfg->in_height_ds4;
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_CFG_REG,
		temp_vbk_cfg.u32);
	return ISP_IPP_OK;
}

static int vbk_y_gauss_config(
	struct vbk_dev_t *dev, struct vbk_y_sigma_gauss_coeff_t *gauss_coeff)
{
	union U_VBK_Y_DS16_GAUSS_COEFF0_CFG temp_gauss_coeff0;
	union U_VBK_Y_DS16_GAUSS_COEFF1_CFG temp_gauss_coeff1;

	if ((dev == NULL) || (gauss_coeff == NULL))
		return ISP_IPP_ERR;

	temp_gauss_coeff0.u32 = 0;
	temp_gauss_coeff1.u32 = 0;
	temp_gauss_coeff0.bits.y_ds16_g00 = gauss_coeff->g00;
	temp_gauss_coeff0.bits.y_ds16_g01 = gauss_coeff->g01;
	temp_gauss_coeff0.bits.y_ds16_g02 = gauss_coeff->g02;
	temp_gauss_coeff0.bits.y_ds16_g03 = gauss_coeff->g03;
	temp_gauss_coeff1.bits.y_ds16_g04 = gauss_coeff->g04;
	temp_gauss_coeff1.bits.y_ds16_gauss_inv = gauss_coeff->inv;
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_Y_DS16_GAUSS_COEFF0_CFG_REG,
		temp_gauss_coeff0.u32);
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_Y_DS16_GAUSS_COEFF1_CFG_REG,
		temp_gauss_coeff1.u32);
	return ISP_IPP_OK;
}

static int vbk_sigma_gauss_config(
	struct vbk_dev_t *dev, struct vbk_y_sigma_gauss_coeff_t *gauss_coeff)
{
	union U_VBK_SIGMA_GAUSS_COEFF0_CFG temp_gauss_coeff0;
	union U_VBK_SIGMA_GAUSS_COEFF1_CFG temp_gauss_coeff1;

	if ((dev == NULL) || (gauss_coeff == NULL))
		return ISP_IPP_ERR;

	temp_gauss_coeff0.u32 = 0;
	temp_gauss_coeff1.u32 = 0;
	temp_gauss_coeff0.bits.sigma_g00 = gauss_coeff->g00;
	temp_gauss_coeff0.bits.sigma_g01 = gauss_coeff->g01;
	temp_gauss_coeff0.bits.sigma_g02 = gauss_coeff->g02;
	temp_gauss_coeff0.bits.sigma_g03 = gauss_coeff->g03;
	temp_gauss_coeff1.bits.sigma_g04 = gauss_coeff->g04;
	temp_gauss_coeff1.bits.sigma_gauss_inv = gauss_coeff->inv;
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_SIGMA_GAUSS_COEFF0_CFG_REG,
		temp_gauss_coeff0.u32);
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_SIGMA_GAUSS_COEFF1_CFG_REG,
		temp_gauss_coeff1.u32);
	return ISP_IPP_OK;
}

static int vbk_uv_gauss_config(
	struct vbk_dev_t *dev, struct vbk_uv_gauss_coeff_t *gauss_coeff)
{
	union U_VBK_UV_DS16_GAUSS_COEFF0_CFG temp_gauss_coeff0;
	union U_VBK_UV_DS16_GAUSS_COEFF1_CFG temp_gauss_coeff1;

	if ((dev == NULL) || (gauss_coeff == NULL))
		return ISP_IPP_ERR;

	temp_gauss_coeff0.u32 = 0;
	temp_gauss_coeff1.u32 = 0;
	temp_gauss_coeff0.bits.uv_ds16_g00 = gauss_coeff->g00;
	temp_gauss_coeff0.bits.uv_ds16_g01 = gauss_coeff->g01;
	temp_gauss_coeff0.bits.uv_ds16_g02 = gauss_coeff->g02;
	temp_gauss_coeff1.bits.uv_ds16_gauss_inv = gauss_coeff->inv;
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_UV_DS16_GAUSS_COEFF1_CFG_REG,
		temp_gauss_coeff1.u32);
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_UV_DS16_GAUSS_COEFF0_CFG_REG,
		temp_gauss_coeff0.u32);
	return ISP_IPP_OK;
}

static int vbk_addnoise_config(
	struct vbk_dev_t *dev, struct vbk_addnoise_t *addnoise)
{
	union U_VBK_ADDNOISE_CFG temp;

	if ((dev == NULL) || (addnoise == NULL))
		return ISP_IPP_ERR;

	temp.u32 = 0;
	temp.bits.addnoise_th = addnoise->addnoise_th;
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_ADDNOISE_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int vbk_sigma2alpha_config(
	struct vbk_dev_t *dev, struct vbk_sigma2alpha_t *sigma2alpha)
{
	unsigned int i = 0;
	union U_VBK_SIGMA2ALPHA_LUT temp;

	if ((dev == NULL) || (sigma2alpha == NULL))
		return ISP_IPP_ERR;

	temp.u32 = 0;

	for (i = 0; i < SIGMA2ALPHA_NUM; i++) {
		temp.bits.sigma2alpha = sigma2alpha->sigma2alpha[i];
		CMDLST_SET_REG(dev->base_addr +
			VBK_VBK_SIGMA2ALPHA_LUT_0_REG + 4 * i, temp.u32);
	}

	return ISP_IPP_OK;
}

static int vbk_foremask_config(
	struct vbk_dev_t *dev, struct vbk_foremask_cfg_t *foremask_cfg)
{
	union U_VBK_FOREMASK_CFG temp;

	if ((dev == NULL) || (foremask_cfg == NULL))
		return ISP_IPP_ERR;

	unsigned int filter_en = foremask_cfg->foremask_weighted_filter_en;
	unsigned int radius = foremask_cfg->foremask_dilation_radius;

	temp.u32 = 0;
	temp.bits.foremask_coeff = foremask_cfg->foremask_coeff;
	temp.bits.foremask_th = foremask_cfg->foremask_th;
	temp.bits.foremask_weighted_filter_en = filter_en;
	temp.bits.foremask_dilation_radius = radius;
	CMDLST_SET_REG(dev->base_addr + VBK_VBK_FOREMASK_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int vbk_y_crop_config(
	struct vbk_dev_t *dev, struct vbk_crop_cfg_t *y_crop_cfg)
{
	union U_VBK_Y_CROP_H temp_h;
	union U_VBK_Y_CROP_V temp_v;

	if ((dev == NULL) || (y_crop_cfg == NULL))
		return ISP_IPP_OK;

	temp_h.u32 = 0;
	temp_v.u32 = 0;
	temp_h.bits.y_crop_right = y_crop_cfg->crop_right;
	temp_h.bits.y_crop_left = y_crop_cfg->crop_left;
	temp_v.bits.y_crop_bottom = y_crop_cfg->crop_bottom;
	temp_v.bits.y_crop_top = y_crop_cfg->crop_top;
	CMDLST_SET_REG(dev->base_addr  + VBK_VBK_Y_CROP_H_REG, temp_h.u32);
	CMDLST_SET_REG(dev->base_addr  + VBK_VBK_Y_CROP_V_REG, temp_v.u32);
	return ISP_IPP_OK;
}

static int vbk_uv_crop_config(
	struct vbk_dev_t *dev, struct vbk_crop_cfg_t *uv_crop_cfg)
{
	union U_VBK_UV_CROP_H temp_h;
	union U_VBK_UV_CROP_V temp_v;

	if ((dev == NULL) || (uv_crop_cfg == NULL))
		return ISP_IPP_OK;

	temp_h.u32 = 0;
	temp_v.u32 = 0;
	temp_h.bits.uv_crop_right = uv_crop_cfg->crop_right;
	temp_h.bits.uv_crop_left = uv_crop_cfg->crop_left;
	temp_v.bits.uv_crop_bottom = uv_crop_cfg->crop_bottom;
	temp_v.bits.uv_crop_top = uv_crop_cfg->crop_top;
	CMDLST_SET_REG(dev->base_addr  + VBK_VBK_UV_CROP_H_REG, temp_h.u32);
	CMDLST_SET_REG(dev->base_addr  + VBK_VBK_UV_CROP_V_REG, temp_v.u32);
	return ISP_IPP_OK;
}

static int vbk_tf_crop_config(
	struct vbk_dev_t *dev, struct vbk_crop_cfg_t *tf_crop_cfg)
{
	union U_VBK_TF_CROP_H temp_h;
	union U_VBK_TF_CROP_V temp_v;

	if ((dev == NULL) || (tf_crop_cfg == NULL))
		return ISP_IPP_OK;

	temp_h.u32 = 0;
	temp_v.u32 = 0;
	temp_h.bits.tf_crop_right = tf_crop_cfg->crop_right;
	temp_h.bits.tf_crop_left = tf_crop_cfg->crop_left;
	temp_v.bits.tf_crop_bottom = tf_crop_cfg->crop_bottom;
	temp_v.bits.tf_crop_top = tf_crop_cfg->crop_top;
	CMDLST_SET_REG(dev->base_addr  + VBK_VBK_TF_CROP_H_REG, temp_h.u32);
	CMDLST_SET_REG(dev->base_addr  + VBK_VBK_TF_CROP_V_REG, temp_v.u32);
	return ISP_IPP_OK;
}

static int vbk_do_config(
	struct vbk_dev_t *dev, struct vbk_config_table_t *cfg_tab)
{
	if ((dev == NULL) || (cfg_tab == NULL))
		return ISP_IPP_ERR;

	if (cfg_tab->to_use == 1) {
		cfg_tab->to_use = 0;

		if (cfg_tab->vbk_ctrl.to_use == 1) {
			cfg_tab->vbk_ctrl.to_use = 0;
			vbk_basic_config(dev,
				&cfg_tab->vbk_ctrl);
		}

		if (cfg_tab->yds16_gauss_coeff.to_use == 1) {
			cfg_tab->yds16_gauss_coeff.to_use = 0;
			vbk_y_gauss_config(dev,
				&cfg_tab->yds16_gauss_coeff);
		}

		if (cfg_tab->uvds16_gauss_coeff.to_use == 1) {
			cfg_tab->uvds16_gauss_coeff.to_use = 0;
			vbk_uv_gauss_config(dev,
				&cfg_tab->uvds16_gauss_coeff);
		}

		if (cfg_tab->sigma_gauss_coeff.to_use == 1) {
			cfg_tab->sigma_gauss_coeff.to_use = 0;
			vbk_sigma_gauss_config(dev,
				&cfg_tab->sigma_gauss_coeff);
		}

		if (cfg_tab->sigma2alpha.to_use == 1) {
			cfg_tab->sigma2alpha.to_use = 0;
			vbk_sigma2alpha_config(dev,
				&cfg_tab->sigma2alpha);
		}

		if (cfg_tab->addnoise.to_use == 1) {
			cfg_tab->addnoise.to_use = 0;
			vbk_addnoise_config(dev,
				&cfg_tab->addnoise);
		}

		if (cfg_tab->foremask_cfg.to_use == 1) {
			cfg_tab->foremask_cfg.to_use = 0;
			vbk_foremask_config(dev,
				&cfg_tab->foremask_cfg);
		}

		if (cfg_tab->y_crop_cfg.to_use == 1)
			loge_if_ret(vbk_y_crop_config(dev,
				&cfg_tab->y_crop_cfg));

		if (cfg_tab->uv_crop_cfg.to_use == 1)
			loge_if_ret(vbk_uv_crop_config(dev,
				&cfg_tab->uv_crop_cfg));

		if (cfg_tab->tf_crop_cfg.to_use == 1)
			loge_if_ret(vbk_tf_crop_config(dev,
				&cfg_tab->tf_crop_cfg));
	}

	return ISP_IPP_OK;
}

int vbk_prepare_cmd(struct vbk_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct vbk_config_table_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL) {
		E("failed : null pointer");
		ISP_IPP_ERR;
	}

	dev->cmd_buf = cmd_buf;
	loge_if_ret(vbk_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct vbk_ops_t vbk_ops = {
	.prepare_cmd   = vbk_prepare_cmd,
};

struct vbk_dev_t g_vbk_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_VBK,
		.ops = &vbk_ops,
	},
};

#pragma GCC diagnostic pop

/* ********************************* END ********************************* */
