// ******************************************************
// Copyright    Copyright (c) 2014- Hisilicon Technologies CO., Ltd.
// File name    compare_drv.c
// Description:
//
// Date        2020-04-10
// ******************************************************
#include <linux/printk.h>
#include <linux/string.h>
#include "ipp.h"
#include "compare_drv.h"
#include "compare_drv_priv.h"
#include "compare_reg_offset.h"
#include "compare_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_COMPARE_DRV

static int compare_ctrl_config(
	struct compare_dev_t *dev, struct compare_ctrl_cfg_t  *ctrl)
{
	union u_compare_cfg temp;

	if (dev == NULL || ctrl == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.compare_en = ctrl->compare_en;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_COMPARE_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_block_config(
	struct compare_dev_t *dev, struct compare_block_cfg_t  *block_cfg)
{
	union u_compare_block_cfg temp;

	if (dev == NULL || block_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.blk_v_num = block_cfg->blk_v_num;
	temp.bits.blk_h_num = block_cfg->blk_h_num;
	temp.bits.blk_num = block_cfg->blk_num;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_BLOCK_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_search_config(
	struct compare_dev_t *dev, struct compare_search_cfg_t *search_cfg)
{
	union u_search_cfg temp;

	if (dev == NULL || search_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.v_radius = search_cfg->v_radius;
	temp.bits.h_radius = search_cfg->h_radius;
	temp.bits.dis_ratio = search_cfg->dis_ratio;
	temp.bits.dis_threshold = search_cfg->dis_threshold;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_SEARCH_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_offset_config(
	struct compare_dev_t *dev, struct compare_offset_cfg_t *offset_cfg)
{
	union u_offset_cfg temp;

	if (dev == NULL || offset_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.cenh_offset = offset_cfg->cenh_offset;
	temp.bits.cenv_offset = offset_cfg->cenv_offset;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_OFFSET_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_stat_config(
	struct compare_dev_t *dev, struct compare_stat_cfg_t *stat_cfg)
{
	union u_stat_cfg temp;

	if (dev == NULL || stat_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.stat_en = stat_cfg->stat_en;
	temp.bits.max3_ratio = stat_cfg->max3_ratio;
	temp.bits.bin_factor = stat_cfg->bin_factor;
	temp.bits.bin_num = stat_cfg->bin_num;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_STAT_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_prefetch_config(
	struct compare_dev_t *dev, struct compare_prefetch_cfg_t *prefetch_cfg)
{
	union u_compare_prefetch_cfg temp;

	if (dev == NULL || prefetch_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.prefetch_enable = 0; // Tie 0 in V350
	temp.bits.first_4k_page = prefetch_cfg->first_4k_page;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_PREFETCH_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_kpt_num_config(
	struct compare_dev_t *dev, struct cfg_tab_compare_t *tab_compare)
{
	unsigned int i = 0;
	union u_ref_kpt_number temp_ref;
	union u_cur_kpt_number temp_cur;
	struct compare_kptnum_cfg_t *kptnum_cfg = NULL;

	if (dev == NULL || tab_compare == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	kptnum_cfg = &(tab_compare->compare_kptnum_cfg);
	temp_ref.u32 = 0;
	temp_cur.u32 = 0;
	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	macro_cmdlst_set_reg_burst_data_align(CMP_KPT_NUM_RANGE_DS4, CVDR_ALIGN_BYTES);
	tab_compare->cmp_ref_kpt_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("cmp_ref_kpt_addr = 0x%x", tab_compare->cmp_ref_kpt_addr);
	macro_cmdlst_set_reg_incr(dev->base_addr + COMPARE_REF_KPT_NUMBER_0_REG,
							  CMP_KPT_NUM_RANGE_DS4, 0, 0);

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4; i++) {
		temp_ref.bits.ref_kpt_num = kptnum_cfg->compare_ref_kpt_num[i];
		macro_cmdlst_set_reg_data(temp_ref.u32);
	}

	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	macro_cmdlst_set_reg_burst_data_align(CMP_KPT_NUM_RANGE_DS4, CVDR_ALIGN_BYTES);
	tab_compare->cmp_cur_kpt_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("cmp_cur_kpt_addr = 0x%x", tab_compare->cmp_cur_kpt_addr);
	macro_cmdlst_set_reg_incr(dev->base_addr + COMPARE_CUR_KPT_NUMBER_0_REG,
							  CMP_KPT_NUM_RANGE_DS4, 0, 0);

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4; i++) {
		temp_cur.bits.cur_kpt_num = kptnum_cfg->compare_cur_kpt_num[i];
		macro_cmdlst_set_reg_data(temp_cur.u32);
	}

	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	return ISP_IPP_OK;
}

static int compare_stat_total_kptnum(
	struct compare_dev_t *dev,
	struct cfg_tab_compare_t *tab_compare)
{
	union u_compare_total_kpt_num temp;

	if (dev == NULL || tab_compare == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	tab_compare->cmp_total_kpt_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("cmp_total_kpt_addr = 0x%x", tab_compare->cmp_total_kpt_addr);
	macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
	macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
	temp.bits.total_kpt_num = tab_compare->compare_total_kptnum_cfg.total_kptnum;
	macro_cmdlst_set_reg(dev->base_addr + COMPARE_TOTAL_KPT_NUM_REG, temp.u32);
	return ISP_IPP_OK;
}

static int compare_do_config(struct compare_dev_t *dev, struct cfg_tab_compare_t *tab_compare)
{
	if (dev == NULL || tab_compare == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	if (tab_compare->compare_block_cfg.to_use) {
		tab_compare->compare_block_cfg.to_use = 0;
		loge_if_ret(compare_block_config(dev, &(tab_compare->compare_block_cfg)));
	}

	if (tab_compare->compare_search_cfg.to_use) {
		tab_compare->compare_search_cfg.to_use = 0;
		loge_if_ret(compare_search_config(dev, &(tab_compare->compare_search_cfg)));
	}

	if (tab_compare->compare_offset_cfg.to_use) {
		tab_compare->compare_offset_cfg.to_use = 0;
		loge_if_ret(compare_offset_config(dev, &(tab_compare->compare_offset_cfg)));
	}

	if (tab_compare->compare_stat_cfg.to_use) {
		tab_compare->compare_stat_cfg.to_use = 0;
		loge_if_ret(compare_stat_config(dev, &(tab_compare->compare_stat_cfg)));
	}

	if (tab_compare->compare_total_kptnum_cfg.to_use) {
		tab_compare->compare_total_kptnum_cfg.to_use = 0;
		loge_if_ret(compare_stat_total_kptnum(dev, tab_compare));
	}

	if (tab_compare->compare_prefetch_cfg.to_use) {
		tab_compare->compare_prefetch_cfg.to_use = 0;
		loge_if_ret(compare_prefetch_config(dev, &(tab_compare->compare_prefetch_cfg)));
	}

	if (tab_compare->compare_kptnum_cfg.to_use) {
		tab_compare->compare_kptnum_cfg.to_use = 0;
		loge_if_ret(compare_kpt_num_config(dev, tab_compare));
	}

	if (tab_compare->compare_ctrl_cfg.to_use) {
		tab_compare->compare_ctrl_cfg.to_use = 0;
		loge_if_ret(compare_ctrl_config(dev, &(tab_compare->compare_ctrl_cfg)));
	}

	return ISP_IPP_OK;
}

int compare_prepare_cmd(struct compare_dev_t *dev,
						struct cmd_buf_t *cmd_buf, struct cfg_tab_compare_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(compare_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct compare_ops_t compare_ops = {
	.prepare_cmd   = compare_prepare_cmd,
};

struct compare_dev_t g_compare_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_COMPARE,
		.ops = &compare_ops,
	},
};

/* ********************************* END ********************************* */
