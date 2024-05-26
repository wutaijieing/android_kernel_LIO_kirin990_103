/* *****************************************************************
 *   Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 *   File name    cmdlst_drv.c
 *   Description:
 *
 *   Date         2020-04-15 16:28:10
 *******************************************************************/

#define LOG_TAG LOG_MODULE_CMDLST_DRV

#include "linux/printk.h"

#include "cmdlst_drv.h"
#include "cmdlst_drv_priv.h"
#include "cmdlst_reg_offset.h"
#include "cvdr_ipp_reg_offset.h"


static int cmdlst_set_config(struct cmdlst_dev_t *dev,
							 struct cmdlst_cfg_t *cfg)
{
	union u_cfg tmp_cfg;
	tmp_cfg.u32 = hispcpe_reg_get(CMDLIST_REG, CMDLST_CFG_REG);
	tmp_cfg.bits.slowdown_nrt_channel = cfg->slowdown_nrt_channel;
	hispcpe_reg_set(CMDLIST_REG, CMDLST_CFG_REG, tmp_cfg.u32);
	return ISP_IPP_OK;
}

static int cmdlst_set_sw_ch(struct cmdlst_dev_t *dev,
							struct cmdlst_ch_cfg_t *cfg, unsigned int channel_id)
{
	union u_ch_cfg tmp_cfg;
	tmp_cfg.u32 = hispcpe_reg_get(CMDLIST_REG,
								  CMDLST_CH_CFG_0_REG + CH_CFG_OFFSET * channel_id);
	tmp_cfg.bits.active_token_nbr = cfg->active_token_nbr;
	tmp_cfg.bits.active_token_nbr_enable = cfg->active_token_nbr_en;
	tmp_cfg.bits.nrt_channel = cfg->nrt_channel;
	hispcpe_reg_set(CMDLIST_REG,
					CMDLST_CH_CFG_0_REG + CH_CFG_OFFSET * channel_id, tmp_cfg.u32);
	return ISP_IPP_OK;
}

static int cmdlst_set_sw_ch_mngr(struct cmdlst_dev_t *dev,
								 struct cmdlst_sw_ch_mngr_cfg_t *cfg, unsigned int channel_id)
{
	union u_sw_ch_mngr tmp_cfg;
	tmp_cfg.u32 = hispcpe_reg_get(CMDLIST_REG,
								  CMDLST_SW_CH_MNGR_0_REG + CH_CFG_OFFSET * channel_id);
	tmp_cfg.bits.sw_task = cfg->sw_task;
	tmp_cfg.bits.sw_resource = cfg->sw_resource;
	tmp_cfg.bits.sw_priority = cfg->sw_priority;
	hispcpe_reg_set(CMDLIST_REG,
					CMDLST_SW_CH_MNGR_0_REG + CH_CFG_OFFSET * channel_id, tmp_cfg.u32);
	return ISP_IPP_OK;
}

int cmdlst_set_vp_rd(struct cmdlst_dev_t *dev,
					 struct cmdlst_vp_rd_cfg_t *cfg)
{
	loge_if_ret(cfg == NULL);
	logd("buffer addr = 0x%x, size=0x%x, cfg->vp_rd_id=%d", cfg->rd_addr, cfg->rd_size, cfg->vp_rd_id);
	// same with isp
	hispcpe_reg_set(CMDLIST_REG, CMDLST_START_ADDR_REG, JPG_CMDLST_OFFSET);
	hispcpe_reg_set(CMDLIST_REG, CMDLST_END_ADDR_REG, JPG_CMDLST_OFFSET + 0xC80);
	hispcpe_reg_set(CMDLIST_REG, CMDLST_SW_CVDR_RD_ADDR_0_REG + CH_CFG_OFFSET * cfg->vp_rd_id,
					((0x3 << 24) | ((JPG_CVDR_OFFSET + CVDR_IPP_CVDR_IPP_VP_RD_CFG_0_REG) & 0x3FFFFC)));
	hispcpe_reg_set(CVDR_REG, CVDR_IPP_CVDR_IPP_VP_RD_IF_CFG_0_REG, 0x80000000);
	hispcpe_reg_set(CMDLIST_REG, CMDLST_SW_CVDR_RD_DATA_0_0_REG + CH_CFG_OFFSET * cfg->vp_rd_id,
					(((cfg->rd_addr + cfg->rd_size) >> 2) & 0xffffE000) | 0x000001BE); // 0x000001BE  6DU|expand|D64 format
	hispcpe_reg_set(CMDLIST_REG, CMDLST_SW_CVDR_RD_DATA_1_0_REG + CH_CFG_OFFSET * cfg->vp_rd_id,
					((cfg->rd_size >> 3) - 1)); // D64 = 8byte
	hispcpe_reg_set(CMDLIST_REG, CMDLST_SW_CVDR_RD_DATA_2_0_REG + CH_CFG_OFFSET * cfg->vp_rd_id, 0x0);
	hispcpe_reg_set(CMDLIST_REG, CMDLST_SW_CVDR_RD_DATA_3_0_REG + CH_CFG_OFFSET * cfg->vp_rd_id, cfg->rd_addr >> 2);
	return ISP_IPP_OK;
}

int cmdlst_do_config(struct cmdlst_dev_t *dev, struct cfg_tab_cmdlst_t *config_table)
{
	loge_if_ret(cmdlst_set_config(dev, &config_table->cfg));
	loge_if_ret(cmdlst_set_sw_ch_mngr(dev,
									  &config_table->sw_ch_mngr_cfg, config_table->vp_rd_cfg.vp_rd_id));
	loge_if_ret(cmdlst_set_vp_rd(dev, &config_table->vp_rd_cfg));
	return ISP_IPP_OK;
}

static int cmdlst_set_branch(struct cmdlst_dev_t *dev, int ch_id)
{
	hispcpe_reg_set(CMDLIST_REG,
					CMDLST_SW_BRANCH_0_REG + CH_CFG_OFFSET * ch_id, 0x00000001);
	return ISP_IPP_OK;
}

int cmdlst_get_state(struct cmdlst_dev_t *dev, struct cmdlst_state_t *st)
{
	// last_task id(frame num in isp)no use for ipp
	st->ch_state = hispcpe_reg_get(CMDLIST_REG,
								   CMDLST_DEBUG_CHANNEL_0_REG + CH_CFG_OFFSET * st->ch_id);
	st->last_exec = hispcpe_reg_get(CMDLIST_REG,
									CMDLST_LAST_EXEC_PTR_0_REG + CH_CFG_OFFSET * st->ch_id);
	return ISP_IPP_OK;
}

int cmdlst_set_vp_wr_flush(
	struct cmdlst_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	int ch_id)
{
	dev->cmd_buf = cmd_buf;
	macro_cmdlst_set_reg(dev->base_addr +
						 CMDLST_VP_WR_FLUSH_0_REG + CH_CFG_OFFSET * ch_id, 0x00000001);
	return ISP_IPP_OK;
}


static struct cmdlst_ops_t cmdlst_ops = {
	.set_config = cmdlst_set_config,
	.set_sw_ch  = cmdlst_set_sw_ch,
	.set_sw_ch_mngr  = cmdlst_set_sw_ch_mngr,
	.set_vp_rd = cmdlst_set_vp_rd,
	// .set_token_cfg   = cmdlst_set_token_cfg,
	.do_config = cmdlst_do_config,
	.set_branch = cmdlst_set_branch,
	.get_state = cmdlst_get_state,
};

struct cmdlst_dev_t g_cmdlst_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_CMDLST,
		.ops = &cmdlst_ops,
	},
};

/* ********************************* END ********************************* */
