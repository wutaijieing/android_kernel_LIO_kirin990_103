
#include <linux/printk.h>
#include "ipp.h"
#include "mc_drv.h"

#define LOG_TAG LOG_MODULE_MC_DRV

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

static int ipp_mc_ctrl_config(
	struct mc_dev_t *dev, struct mc_ctrl_cfg_t *config_table)
{
	union U_MC_CFG temp;

	if (dev == NULL || config_table == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	if (config_table->svd_iterations > SVD_ITERATIONS_MAX ||
		config_table->svd_iterations < SVD_ITERATIONS_MIN) {
		E("config_table->svd_iterations=%d error",
			config_table->svd_iterations);
		return ISP_IPP_ERR;
	}

	if (config_table->max_iterations > H_ITERATIONS_MAX ||
		config_table->max_iterations < H_ITERATIONS_MIN) {
		E("config_table->max_iterations=%d error",
			config_table->max_iterations);
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mc_en = config_table->mc_en; // start mc
	temp.bits.cfg_mode = config_table->cfg_mode;
	temp.bits.h_cal_en = config_table->h_cal_en;
	temp.bits.push_inliers_en = config_table->push_inliers_en;
	temp.bits.bf_mode = config_table->bf_mode;
	temp.bits.flag_10_11 = config_table->flag_10_11;
	temp.bits.max_iterations = config_table->max_iterations;
	temp.bits.svd_iterations = config_table->svd_iterations;
	CMDLST_SET_REG(dev->base_addr + MC_MC_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_mc_inlier_thld_config(
		struct mc_dev_t *dev, struct mc_inlier_thld_t *config_table)
{
	union U_THRESHOLD_CFG temp;

	if (dev == NULL || config_table == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.inlier_th = config_table->inlier_th;
	CMDLST_SET_REG(dev->base_addr  +
		MC_THRESHOLD_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_mc_kpt_num_config(
	struct mc_dev_t *dev, struct cfg_tab_mc_t *tab_mc)
{
	union U_MATCH_POINTS points;

	if (dev == NULL || tab_mc == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	points.u32 = 0;
	CMDLST_SET_ADDR_ALIGN(2, CVDR_ALIGN_BYTES);
	tab_mc->mc_match_points_addr =
		dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	I("mc_match_points_addr = 0x%x", tab_mc->mc_match_points_addr);
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	points.bits.matched_kpts = tab_mc->mc_kpt_num_cfg.matched_kpts;
	CMDLST_SET_REG(dev->base_addr + MC_MATCH_POINTS_REG, points.u32);
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	return ISP_IPP_OK;
}

static int ipp_mc_prefetch_config(
		struct mc_dev_t *dev, struct mc_prefech_t *config_table)
{
	union U_REF_PREFETCH_CFG ref;

	if (dev == NULL || config_table == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	ref.u32 = 0;
	ref.bits.ref_first_32k_page = config_table->ref_first_32k_page;
	ref.bits.ref_prefetch_enable = config_table->ref_prefetch_enable;
	CMDLST_SET_REG(dev->base_addr  + MC_REF_PREFETCH_CFG_REG, ref.u32);
	union U_CUR_PREFETCH_CFG cur;

	cur.u32 = 0;
	cur.bits.cur_first_32k_page = config_table->cur_first_32k_page;
	cur.bits.cur_prefetch_enable = config_table->cur_prefetch_enable;
	CMDLST_SET_REG(dev->base_addr  + MC_CUR_PREFETCH_CFG_REG, cur.u32);
	return ISP_IPP_OK;
}

static int ipp_mc_index_pairs_config(
		struct mc_dev_t *dev, struct cfg_tab_mc_t *tab_mc)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int mc_reg_size = 0;
	unsigned int mc_data_size = KPT_NUM_MAX;
	unsigned int mc_index_cnt = 0;
	union U_INDEX_CFG temp;

	if (dev == NULL || tab_mc == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	CMDLST_SET_REG(dev->base_addr  +
		MC_TABLE_CLEAR_CFG_REG, 0);
	CMDLST_SET_ADDR_ALIGN(2, CVDR_ALIGN_BYTES);
	tab_mc->mc_index_addr =
		dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	I("mc_index_addr = 0x%x", tab_mc->mc_index_addr);
	
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);

	I("tab_mc->mc_cascade_en = %d", tab_mc->mc_cascade_en);
	if(tab_mc->mc_cascade_en == CASCADE_ENABLE) {
		mc_data_size = KPT_NUM_MAX;
		for (i = 0; i < (mc_data_size / CMDLST_BURST_MAX_SIZE + 1); i++) {
				mc_reg_size = ((mc_data_size - i * CMDLST_BURST_MAX_SIZE) >
					CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE :
					(mc_data_size - i * CMDLST_BURST_MAX_SIZE);
				CMDLST_SET_REG_INCR(dev->base_addr +
					MC_INDEX_CFG_0_REG, mc_reg_size, 1, 0);
		
				CMDLST_SET_ADDR_OFFSET(mc_reg_size);
			}
	}else if (tab_mc->mc_cascade_en == CASCADE_DISABLE) {
		mc_data_size = tab_mc->mc_kpt_num_cfg.matched_kpts;
		for (i = 0; i < (mc_data_size / CMDLST_BURST_MAX_SIZE + 1); i++) {
				mc_reg_size = ((mc_data_size - i * CMDLST_BURST_MAX_SIZE) >
					CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE :
					(mc_data_size - i * CMDLST_BURST_MAX_SIZE);
				CMDLST_SET_REG_INCR(dev->base_addr +
					MC_INDEX_CFG_0_REG, mc_reg_size, 1, 0);
		
				for (j = 0; j < mc_reg_size; j++) {
					temp.u32 = 0;
					temp.bits.cfg_cur_index =
					tab_mc->mc_index_pairs_cfg.cfg_cur_index[mc_index_cnt];
					temp.bits.cfg_ref_index =
					tab_mc->mc_index_pairs_cfg.cfg_ref_index[mc_index_cnt];
					temp.bits.cfg_best_dist =
					tab_mc->mc_index_pairs_cfg.cfg_best_dist[mc_index_cnt];
		
					mc_index_cnt++;
					CMDLST_SET_REG_DATA(temp.u32);
				}
			}
	}
	
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);

	CMDLST_SET_REG(dev->base_addr  +
		MC_TABLE_CLEAR_CFG_REG, 0);

	return ISP_IPP_OK;
}

static int ipp_mc_coord_pairs_config(
	struct mc_dev_t *dev,
	struct mc_coord_pairs_t *config_table,
	unsigned int kpt_num)
{
	unsigned int i = 0;
	union U_COORDINATE_CFG temp;

	if (dev == NULL || config_table == NULL || (kpt_num > KPT_NUM_MAX)) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	CMDLST_SET_REG(dev->base_addr +
		MC_TABLE_CLEAR_CFG_REG, 0);

	for (i = 0; i < kpt_num; i++) {
		temp.u32 = 0;
		temp.bits.cfg_coordinate_x =
			config_table->cur_cfg_coordinate_x[i];
		temp.bits.cfg_coordinate_y =
			config_table->cur_cfg_coordinate_y[i];
		CMDLST_SET_REG(dev->base_addr  +
			MC_COORDINATE_CFG_0_REG, temp.u32);
		temp.bits.cfg_coordinate_x =
			config_table->ref_cfg_coordinate_x[i];
		temp.bits.cfg_coordinate_y =
			config_table->ref_cfg_coordinate_y[i];
		CMDLST_SET_REG(dev->base_addr  +
			MC_COORDINATE_CFG_0_REG, temp.u32);
	}

	return ISP_IPP_OK;
}

static int mc_do_config(struct mc_dev_t *dev, struct cfg_tab_mc_t *tab_mc)
{
	if ((dev == NULL) || (tab_mc == NULL))
		return ISP_IPP_ERR;

	if (tab_mc->to_use) {
		tab_mc->to_use = 0;

		if (tab_mc->mc_kpt_num_cfg.to_use) {
			loge_if_ret(ipp_mc_kpt_num_config(dev, tab_mc));
			tab_mc->mc_kpt_num_cfg.to_use = 0;
		}

		if (tab_mc->mc_prefech_cfg.to_use) {
			loge_if_ret(ipp_mc_prefetch_config(dev,
				&(tab_mc->mc_prefech_cfg)));
			tab_mc->mc_prefech_cfg.to_use = 0;
		}

		if (tab_mc->mc_inlier_thld_cfg.to_use) {
			loge_if_ret(ipp_mc_inlier_thld_config(dev,
				&(tab_mc->mc_inlier_thld_cfg)));
			tab_mc->mc_inlier_thld_cfg.to_use = 0;
		}

		if ((tab_mc->mc_ctrl_cfg.cfg_mode == CFG_MODE_INDEX_PAIRS)
			&& tab_mc->mc_index_pairs_cfg.to_use) {
			loge_if_ret(ipp_mc_index_pairs_config(dev, tab_mc));
			tab_mc->mc_index_pairs_cfg.to_use = 0;
		}

		if ((tab_mc->mc_ctrl_cfg.cfg_mode == CFG_MODE_COOR_PAIRS)
			&& tab_mc->mc_coord_pairs_cfg.to_use) {
			loge_if_ret(ipp_mc_coord_pairs_config(dev,
				&(tab_mc->mc_coord_pairs_cfg),
				tab_mc->mc_kpt_num_cfg.matched_kpts));
			tab_mc->mc_coord_pairs_cfg.to_use = 0;
		}

	}

	return ISP_IPP_OK;
}

int mc_prepare_cmd(struct mc_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table)
{
	if(dev == NULL || cmd_buf == NULL || table == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	dev->cmd_buf = cmd_buf;
	loge_if_ret(mc_do_config(dev, table));
	return ISP_IPP_OK;
}

int mc_enable_cmd(struct mc_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table)
{
	if (dev == NULL ||cmd_buf == NULL || table == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	dev->cmd_buf = cmd_buf;
	if (table->mc_ctrl_cfg.to_use) {
		loge_if_ret(ipp_mc_ctrl_config(dev,
			&(table->mc_ctrl_cfg)));
		table->mc_ctrl_cfg.to_use = 0;
	}

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

#pragma GCC diagnostic pop

/* ********************************* END ********************************* */
