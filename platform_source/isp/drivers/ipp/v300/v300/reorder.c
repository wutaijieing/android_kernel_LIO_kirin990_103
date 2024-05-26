
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
	union U_REORDER_CFG temp;

	if (dev == NULL || ctrl == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.reorder_en = ctrl->reorder_en;
	temp.bits.descriptor_type = ctrl->descriptor_type;
	CMDLST_SET_REG(dev->base_addr + REORDER_REORDER_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int reorder_block_config(
	struct reorder_dev_t *dev, struct reorder_block_cfg_t  *block_cfg)
{
	union U_BLOCK_CFG temp;

	if (dev == NULL || block_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.blk_v_num = block_cfg->blk_v_num;
	temp.bits.blk_h_num = block_cfg->blk_h_num;
	temp.bits.blk_num = block_cfg->blk_num;
	CMDLST_SET_REG(dev->base_addr + REORDER_BLOCK_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int ipp_reorder_set_total_kpts(
	struct reorder_dev_t *dev, struct cfg_tab_reorder_t *tab_reorder)
{
	union U_TOTAL_KPT_NUMBER temp;

	if (dev == NULL || tab_reorder == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	CMDLST_SET_ADDR_ALIGN(2, CVDR_ALIGN_BYTES);
	tab_reorder->rdr_total_num_addr =
		dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	I("reorder_kpt_addr = 0x%x", tab_reorder->rdr_total_num_addr);
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	temp.bits.total_kpt_num = tab_reorder->reorder_total_kpt_num.total_kpts;
	CMDLST_SET_REG(dev->base_addr  + REORDER_TOTAL_KPT_NUM, temp.u32);
	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	return ISP_IPP_OK;
}

static int reorder_prefetch_config(
	struct reorder_dev_t *dev, struct reorder_prefetch_cfg_t *prefetch_cfg)
{
	union U_PREFETCH_CFG temp;

	if (dev == NULL || prefetch_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.prefetch_enable = prefetch_cfg->prefetch_enable;
	temp.bits.first_32k_page = prefetch_cfg->first_32k_page;
	CMDLST_SET_REG(dev->base_addr + REORDER_PREFETCH_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int reorder_kpt_num_config(
	struct reorder_dev_t *dev, struct cfg_tab_reorder_t *tab_reorder)
{
	unsigned int i = 0;
	union U_KPT_NUMBER temp;
	struct reorder_kptnum_cfg_t *kptnum_cfg = NULL;

	if (dev == NULL || tab_reorder == NULL) {
		E("params is NULL!!");
		return ISP_IPP_ERR;
	}

	kptnum_cfg = &(tab_reorder->reorder_kptnum_cfg);
	temp.u32 = 0;
	CMDLST_SET_ADDR_ALIGN(REORDER_KPT_NUM, CVDR_ALIGN_BYTES);
	CMDLST_SET_REG_INCR(dev->base_addr + REORDER_KPT_NUMBER_0_REG,
		REORDER_KPT_NUM, 0, 0);
	tab_reorder->rdr_kpt_num_addr =
		dev->cmd_buf->start_addr_isp_map + dev->cmd_buf->data_size;
	I("reorder_kpt_addr = 0x%x", tab_reorder->rdr_kpt_num_addr);

	if(tab_reorder->rdr_cascade_en == CASCADE_ENABLE) {
		CMDLST_SET_ADDR_OFFSET(REORDER_KPT_NUM);
	} else if (tab_reorder->rdr_cascade_en == CASCADE_DISABLE) {
		for (i = 0; i < REORDER_KPT_NUM; i++) {
			temp.bits.kpt_num = kptnum_cfg->reorder_kpt_num[i];
			CMDLST_SET_REG_DATA(temp.u32);
		}
	}

	CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	return ISP_IPP_OK;
}

static int reorder_do_config(struct reorder_dev_t *dev,
	struct cfg_tab_reorder_t *tab_reorder)
{
	if (dev == NULL || tab_reorder == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	if (tab_reorder->reorder_block_cfg.to_use) {
		loge_if_ret(reorder_block_config(dev,
			&(tab_reorder->reorder_block_cfg)));
		tab_reorder->reorder_block_cfg.to_use = 0;
	}

	if (tab_reorder->reorder_total_kpt_num.to_use) {
		loge_if_ret(ipp_reorder_set_total_kpts(dev, tab_reorder));
		tab_reorder->reorder_total_kpt_num.to_use = 0;
	}

	if (tab_reorder->reorder_prefetch_cfg.to_use) {
		loge_if_ret(reorder_prefetch_config(dev,
			&(tab_reorder->reorder_prefetch_cfg)));
		tab_reorder->reorder_prefetch_cfg.to_use = 0;
	}

	if (tab_reorder->reorder_kptnum_cfg.to_use) {
		loge_if_ret(reorder_kpt_num_config(dev, tab_reorder));
		tab_reorder->reorder_kptnum_cfg.to_use = 0;
	}

	if (tab_reorder->reorder_ctrl_cfg.to_use) {
		loge_if_ret(reorder_ctrl_config(dev,
			&(tab_reorder->reorder_ctrl_cfg)));
		tab_reorder->reorder_ctrl_cfg.to_use = 0;
	}

	return ISP_IPP_OK;
}


int reorder_prepare_cmd(struct reorder_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_reorder_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	dev->cmd_buf = cmd_buf;
	loge_if_ret(reorder_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct reorder_ops_t reorder_ops = {
	.prepare_cmd   = reorder_prepare_cmd,
};

struct reorder_dev_t g_reorder_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_REORDER,
		.ops = &reorder_ops,
	},
};

/* ********************************* END ********************************* */
