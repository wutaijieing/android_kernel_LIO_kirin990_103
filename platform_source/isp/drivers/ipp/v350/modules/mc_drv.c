/*******************************************************************
 * Copyright    Copyright (c) 2014- Hisilicon Technologies CO., Ltd.
 * File name    mc_drv.c
 * Description:
 * Date         2020-04-10
 ******************************************************************/
#include <linux/printk.h>
#include "ipp.h"
#include "mc_drv.h"

#define LOG_TAG LOG_MODULE_MC_DRV

static int ipp_mc_ctrl_config(
	struct mc_dev_t *dev, struct mc_ctrl_cfg_t *config_table)
{
	union u_mc_cfg temp;

	if (dev == NULL || config_table == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	if (config_table->svd_iterations > SVD_ITERATIONS_MAX ||
		config_table->svd_iterations < SVD_ITERATIONS_MIN) {
		loge("Failed:config_table->svd_iterations=%d error",
			 config_table->svd_iterations);
		return ISP_IPP_ERR;
	}

	if (config_table->max_iterations > H_ITERATIONS_MAX ||
		config_table->max_iterations < H_ITERATIONS_MIN) {
		loge("Failed:config_table->max_iterations=%d error",
			 config_table->max_iterations);
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mc_en = config_table->mc_en; // start mc
	temp.bits.cfg_mode = config_table->cfg_mode;
	temp.bits.h_cal_en = config_table->h_cal_en;
	temp.bits.push_inliers_en = config_table->push_inliers_en;
	temp.bits.flag_10_11 = config_table->flag_10_11;
	temp.bits.max_iterations = config_table->max_iterations;
	temp.bits.svd_iterations = config_table->svd_iterations;
	macro_cmdlst_set_reg(dev->base_addr + MC_MC_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_mc_inlier_thld_config(
	struct mc_dev_t *dev, struct mc_inlier_thld_t *config_table)
{
	union u_threshold_cfg temp;

	if (dev == NULL || config_table == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.inlier_th = config_table->inlier_th;
	macro_cmdlst_set_reg(dev->base_addr + MC_THRESHOLD_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_mc_kpt_num_config(
	struct mc_dev_t *dev, struct cfg_tab_mc_t *tab_mc)
{
	union u_match_points temp;

	if (dev == NULL || tab_mc == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	tab_mc->mc_match_points_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("mc_match_points_addr = 0x%x", tab_mc->mc_match_points_addr);
	macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
	macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
	temp.bits.matched_kpts = tab_mc->mc_kpt_num_cfg.matched_kpts;
	macro_cmdlst_set_reg(dev->base_addr + MC_MATCH_POINTS_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_mc_prefetch_config(
	struct mc_dev_t *dev, struct mc_prefech_t *config_table)
{
	union u_ref_prefetch_cfg ref;

	union u_cur_prefetch_cfg cur;

	if (dev == NULL || config_table == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	ref.u32 = 0;

	ref.bits.ref_first_4k_page = config_table->ref_first_page;

	ref.bits.ref_prefetch_enable = 0; // configured to 0 in V350 because no prefetch

	macro_cmdlst_set_reg(dev->base_addr  + MC_REF_PREFETCH_CFG_REG, ref.u32);

	cur.u32 = 0;

	cur.bits.cur_first_4k_page = config_table->cur_first_page;

	cur.bits.cur_prefetch_enable = 0; // configured to 0 in V350 because no prefetch

	macro_cmdlst_set_reg(dev->base_addr  + MC_CUR_PREFETCH_CFG_REG, cur.u32);

	return ISP_IPP_OK;
}

static int ipp_mc_index_pairs_config(
	struct mc_dev_t *dev, struct cfg_tab_mc_t *tab_mc)
{
	unsigned int i = 0;
	unsigned int mc_reg_size = 0;
	unsigned int mc_data_size = 0;
	union u_index_cfg temp;
	if (dev == NULL || tab_mc == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	mc_data_size = tab_mc->mc_kpt_num_cfg.matched_kpts;
	macro_cmdlst_set_reg(dev->base_addr + MC_TABLE_CLEAR_CFG_REG, 0);
	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	macro_cmdlst_set_reg_burst_data_align(MC_KPT_NUM_MAX, CVDR_ALIGN_BYTES);
	tab_mc->mc_index_addr = dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	logd("mc_index_addr = 0x%x, mc_data_size = %d", tab_mc->mc_index_addr, mc_data_size);
	logd("tab_mc->mc_cascade_en = %d", tab_mc->mc_cascade_en);

	if (tab_mc->mc_cascade_en == CASCADE_ENABLE) {
		mc_data_size = MC_KPT_NUM_MAX;

		for (i = 0; i < (mc_data_size / CMDLST_BURST_MAX_SIZE + 1); i++) {
			mc_reg_size = ((mc_data_size - i * CMDLST_BURST_MAX_SIZE) >
						   CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE :
						  (mc_data_size - i * CMDLST_BURST_MAX_SIZE);
			macro_cmdlst_set_reg_incr(dev->base_addr + MC_INDEX_CFG_0_REG, mc_reg_size, 1, 0);
			macro_cmdlst_set_addr_offset(mc_reg_size);
		}
	} else if (tab_mc->mc_cascade_en == CASCADE_DISABLE) {
		for (i = 0; i < mc_data_size; i++) {
			temp.u32 = 0;
			temp.bits.cfg_cur_index = tab_mc->mc_index_pairs_cfg.cfg_cur_index[i];
			temp.bits.cfg_ref_index = tab_mc->mc_index_pairs_cfg.cfg_ref_index[i];
			temp.bits.cfg_best_dist = tab_mc->mc_index_pairs_cfg.cfg_best_dist[i];
			macro_cmdlst_set_reg(dev->base_addr  + MC_INDEX_CFG_0_REG, temp.u32);
		}
	}

	macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
	macro_cmdlst_set_reg(dev->base_addr  + MC_TABLE_CLEAR_CFG_REG, 0);
	return ISP_IPP_OK;
}

static int ipp_mc_coord_pairs_config(
	struct mc_dev_t *dev,
	struct mc_coord_pairs_t *config_table,
	unsigned int kpt_num)
{
	unsigned int i = 0;
	union u_coordinate_cfg temp;

	if (dev == NULL || config_table == NULL || (kpt_num > MC_KPT_NUM_MAX)) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	macro_cmdlst_set_reg(dev->base_addr + MC_TABLE_CLEAR_CFG_REG, 0);

	for (i = 0; i < kpt_num; i++) {
		temp.u32 = 0;
		temp.bits.cfg_coordinate_x = config_table->cur_cfg_coordinate_x[i];
		temp.bits.cfg_coordinate_y = config_table->cur_cfg_coordinate_y[i];
		macro_cmdlst_set_reg(dev->base_addr + MC_COORDINATE_CFG_0_REG, temp.u32);
		temp.bits.cfg_coordinate_x = config_table->ref_cfg_coordinate_x[i];
		temp.bits.cfg_coordinate_y = config_table->ref_cfg_coordinate_y[i];
		macro_cmdlst_set_reg(dev->base_addr + MC_COORDINATE_CFG_0_REG, temp.u32);
	}

	macro_cmdlst_set_reg(dev->base_addr + MC_TABLE_CLEAR_CFG_REG, 0);
	return ISP_IPP_OK;
}

static int mc_do_config(struct mc_dev_t *dev, struct cfg_tab_mc_t *tab_mc)
{
	if ((dev == NULL) || (tab_mc == NULL))
		return ISP_IPP_ERR;

	if (tab_mc->to_use) {
		tab_mc->to_use = 0;

		if (tab_mc->mc_kpt_num_cfg.to_use) {
			tab_mc->mc_kpt_num_cfg.to_use = 0;
			loge_if_ret(ipp_mc_kpt_num_config(dev, tab_mc));
		}

		if (tab_mc->mc_prefech_cfg.to_use) {
			tab_mc->mc_prefech_cfg.to_use = 0;
			loge_if_ret(ipp_mc_prefetch_config(dev, &(tab_mc->mc_prefech_cfg)));
		}

		if (tab_mc->mc_inlier_thld_cfg.to_use) {
			tab_mc->mc_inlier_thld_cfg.to_use = 0;
			loge_if_ret(ipp_mc_inlier_thld_config(dev, &(tab_mc->mc_inlier_thld_cfg)));
		}

		if ((tab_mc->mc_ctrl_cfg.cfg_mode == CFG_MODE_INDEX_PAIRS) && tab_mc->mc_index_pairs_cfg.to_use) {
			tab_mc->mc_index_pairs_cfg.to_use = 0;
			loge_if_ret(ipp_mc_index_pairs_config(dev, tab_mc));
		}

		if ((tab_mc->mc_ctrl_cfg.cfg_mode == CFG_MODE_COOR_PAIRS) && tab_mc->mc_coord_pairs_cfg.to_use) {
			tab_mc->mc_coord_pairs_cfg.to_use = 0;
			loge_if_ret(ipp_mc_coord_pairs_config(dev,
												  &(tab_mc->mc_coord_pairs_cfg), tab_mc->mc_kpt_num_cfg.matched_kpts));
		}
	}

	return ISP_IPP_OK;
}

int mc_prepare_cmd(struct mc_dev_t *dev,
				   struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(mc_do_config(dev, table));
	return ISP_IPP_OK;
}

int mc_enable_cmd(struct mc_dev_t *dev,
				  struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;

	if (table->mc_ctrl_cfg.to_use) {
		table->mc_ctrl_cfg.to_use = 0;
		loge_if_ret(ipp_mc_ctrl_config(dev, &(table->mc_ctrl_cfg)));
	}

	return ISP_IPP_OK;
}

int mc_clear_table(struct mc_dev_t *dev, struct cmd_buf_t *cmd_buf)
{
	if (dev == NULL || cmd_buf == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	macro_cmdlst_set_reg(dev->base_addr + MC_TABLE_CLEAR_CFG_REG, 1);
	return ISP_IPP_OK;
}


static struct mc_ops_t mc_ops = {
	.prepare_cmd   = mc_prepare_cmd,
};

struct mc_dev_t g_mc_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_MC,
		.ops = &mc_ops,
	},
};

/* ********************************* END ********************************* */
