/*******************************************************************
 * Copyright    Copyright (c) 2014- Hisilicon Technologies CO., Ltd.
 * File name    reorder_drv.c
 * Description:
 *
 * Date        2020-04-10
 ******************************************************************/
#include <linux/printk.h>
#include <linux/string.h>
#include "ipp.h"
#include "reorder_drv.h"
#include "reorder_drv_priv.h"
#include "reorder_reg_offset.h"
#include "reorder_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_REORDER_DRV

static int reorder_ctrl_config(
	struct reorder_dev_t *dev, struct reorder_ctrl_cfg_t  *ctrl)
{
	union u_reorder_cfg temp;

	if (dev == NULL || ctrl == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.reorder_en = ctrl->reorder_en;
	macro_cmdlst_set_reg(dev->base_addr + REORDER_REORDER_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int reorder_block_config(
	struct reorder_dev_t *dev, struct reorder_block_cfg_t  *block_cfg)
{
	union u_reorder_block_cfg temp;

	if (dev == NULL || block_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.blk_v_num = block_cfg->blk_v_num;
	temp.bits.blk_h_num = block_cfg->blk_h_num;
	temp.bits.blk_num = block_cfg->blk_num;
	macro_cmdlst_set_reg(dev->base_addr + REORDER_BLOCK_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_reorder_set_total_kpts(
	struct reorder_dev_t *dev, struct cfg_tab_reorder_t *tab_reorder)
{
	union u_reorder_total_kpt_num temp;

	if (dev == NULL || tab_reorder == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	tab_reorder->rdr_total_num_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("rdr_total_num_addr = 0x%x", tab_reorder->rdr_total_num_addr);
	macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
	macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
	temp.bits.total_kpt_num = tab_reorder->reorder_total_kpt_num.total_kpts;
	macro_cmdlst_set_reg(dev->base_addr  + REORDER_TOTAL_KPT_NUM_REG, temp.u32);
	return ISP_IPP_OK;
}

static int reorder_prefetch_config(
	struct reorder_dev_t *dev, struct reorder_prefetch_cfg_t *prefetch_cfg)
{
	union u_reorder_prefetch_cfg temp;

	if (dev == NULL || prefetch_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.prefetch_enable = 0; // Tie 0 in V350
	temp.bits.first_4k_page = prefetch_cfg->first_4k_page;
	macro_cmdlst_set_reg(dev->base_addr + REORDER_PREFETCH_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int reorder_kpt_num_config(
	struct reorder_dev_t *dev, struct cfg_tab_reorder_t *tab_reorder)
{
	unsigned int i = 0;
	union u_kpt_number temp;
	struct reorder_kptnum_cfg_t *kptnum_cfg = NULL;

	if (dev == NULL || tab_reorder == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	kptnum_cfg = &(tab_reorder->reorder_kptnum_cfg);
	temp.u32 = 0;
	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	macro_cmdlst_set_reg_burst_data_align(RDR_KPT_NUM_RANGE_DS4, CVDR_ALIGN_BYTES);
	tab_reorder->rdr_kpt_num_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("rdr_kpt_num_addr = 0x%x", tab_reorder->rdr_kpt_num_addr);
	macro_cmdlst_set_reg_incr(dev->base_addr + REORDER_KPT_NUMBER_0_REG, RDR_KPT_NUM_RANGE_DS4, 0, 0);

	for (i = 0; i < RDR_KPT_NUM_RANGE_DS4; i++) {
		temp.bits.kpt_num = kptnum_cfg->reorder_kpt_num[i];
		macro_cmdlst_set_reg_data(temp.u32);
	}

	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	return ISP_IPP_OK;
}

static int reorder_do_config(struct reorder_dev_t *dev,
							 struct cfg_tab_reorder_t *tab_reorder)
{
	if (dev == NULL || tab_reorder == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	if (tab_reorder->reorder_block_cfg.to_use) {
		tab_reorder->reorder_block_cfg.to_use = 0;
		loge_if_ret(reorder_block_config(dev, &(tab_reorder->reorder_block_cfg)));
	}

	if (tab_reorder->reorder_total_kpt_num.to_use) {
		tab_reorder->reorder_total_kpt_num.to_use = 0;
		loge_if_ret(ipp_reorder_set_total_kpts(dev, tab_reorder));
	}

	if (tab_reorder->reorder_prefetch_cfg.to_use) {
		tab_reorder->reorder_prefetch_cfg.to_use = 0;
		loge_if_ret(reorder_prefetch_config(dev, &(tab_reorder->reorder_prefetch_cfg)));
	}

	if (tab_reorder->reorder_kptnum_cfg.to_use) {
		tab_reorder->reorder_kptnum_cfg.to_use = 0;
		loge_if_ret(reorder_kpt_num_config(dev, tab_reorder));
	}

	if (tab_reorder->reorder_ctrl_cfg.to_use) {
		tab_reorder->reorder_ctrl_cfg.to_use = 0;
		loge_if_ret(reorder_ctrl_config(dev, &(tab_reorder->reorder_ctrl_cfg)));
	}

	return ISP_IPP_OK;
}

int reorder_prepare_cmd(struct reorder_dev_t *dev,
						struct cmd_buf_t *cmd_buf, struct cfg_tab_reorder_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(reorder_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct reorder_ops_t reorder_ops = {
	.prepare_cmd = reorder_prepare_cmd,
};

struct reorder_dev_t g_reorder_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_REORDER,
		.ops = &reorder_ops,
	},
};

/* ********************************* END ********************************* */
