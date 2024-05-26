/*******************************************************************
 * Copyright    Copyright (c) 2014- Hisilicon Technologies CO., Ltd.
 * File name    hiof_drv.c
 * Description:
 *
 * Date        2020-04-10
 ******************************************************************/
#include <linux/printk.h>
#include <linux/string.h>
#include "ipp.h"
#include "hiof_drv.h"
#include "hiof_drv_priv.h"
#include "hiof_reg_offset.h"
#include "hiof_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_HIOF_DRV

static int hiof_ctrl_config(
	struct hiof_dev_t *dev, struct hiof_ctrl_cfg_t  *ctrl)
{
	union u_hiof_cfg temp;

	if (dev == NULL || ctrl == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mode = ctrl->mode;
	temp.bits.spena = ctrl->spena;
	temp.bits.iter_num = ctrl->iter_num;
	temp.bits.iter_check_en = ctrl->iter_check_en;
	macro_cmdlst_set_reg(dev->base_addr + HIOF_HIOF_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int hiof_size_config(
	struct hiof_dev_t *dev, struct hiof_size_cfg_t  *size_cfg)
{
	union u_size_cfg temp;

	if (dev == NULL || size_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.width = size_cfg->width;
	temp.bits.height = size_cfg->height;
	macro_cmdlst_set_reg(dev->base_addr + HIOF_SIZE_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int hiof_coeff_config(
	struct hiof_dev_t *dev, struct hiof_coeff_cfg_t *coeff_cfg)
{
	union u_coeff_cfg temp;

	if (dev == NULL || coeff_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.coeff0 = coeff_cfg->coeff0;
	temp.bits.coeff1 = coeff_cfg->coeff1;
	macro_cmdlst_set_reg(dev->base_addr + HIOF_COEFF_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int hiof_threshold_config(
	struct hiof_dev_t *dev, struct hiof_threshold_cfg_t *threshold_cfg)
{
	union u_hiof_threshold_cfg temp;

	if (dev == NULL || threshold_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.flat_thr = threshold_cfg->flat_thr;
	temp.bits.ismotion_thr = threshold_cfg->ismotion_thr;
	temp.bits.confilv_thr = threshold_cfg->confilv_thr;
	temp.bits.att_max = threshold_cfg->att_max;
	macro_cmdlst_set_reg(dev->base_addr + HIOF_THRESHOLD_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int hiof_do_config(struct hiof_dev_t *dev,
						  struct cfg_tab_hiof_t *tab_hiof)
{
	if (dev == NULL || tab_hiof == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	if (tab_hiof->hiof_threshold_cfg.to_use) {
		tab_hiof->hiof_threshold_cfg.to_use = 0;
		loge_if_ret(hiof_threshold_config(dev, &(tab_hiof->hiof_threshold_cfg)));
	}

	if (tab_hiof->hiof_coeff_cfg.to_use) {
		tab_hiof->hiof_coeff_cfg.to_use = 0;
		loge_if_ret(hiof_coeff_config(dev, &(tab_hiof->hiof_coeff_cfg)));
	}

	if (tab_hiof->hiof_size_cfg.to_use) {
		tab_hiof->hiof_size_cfg.to_use = 0;
		loge_if_ret(hiof_size_config(dev, &(tab_hiof->hiof_size_cfg)));
	}

	if (tab_hiof->hiof_ctrl_cfg.to_use) {
		tab_hiof->hiof_ctrl_cfg.to_use = 0;
		loge_if_ret(hiof_ctrl_config(dev, &(tab_hiof->hiof_ctrl_cfg)));
	}

	return ISP_IPP_OK;
}

int hiof_prepare_cmd(struct hiof_dev_t *dev,
					 struct cmd_buf_t *cmd_buf, struct cfg_tab_hiof_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(hiof_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct hiof_ops_t hiof_ops = {
	.prepare_cmd   = hiof_prepare_cmd,
};

struct hiof_dev_t g_hiof_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_HIOF,
		.ops = &hiof_ops,
	},
};

/* ********************************* END ********************************* */
