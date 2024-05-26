/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_mc.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/

#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "segment_mc.h"
#include "cfg_table_mc.h"
#include "cvdr_drv.h"
#include "cfg_table_cvdr.h"
#include "mc_drv.h"
#include "ipp_top_drv.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_MC

static int mc_update_cvdr_cfg_tab(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	if (mc_request->mc_buff_cfg.ref_first_page &&
		mc_request->mc_buff_cfg.cur_first_page)
		loge_if_ret(cfg_tbl_cvdr_nr_rd_cfg(p_cvdr_cfg, IPP_NR_RD_MC));

	return ISP_IPP_OK;
}

static int set_mc_index_pairs_cfg_tbl(struct msg_req_mc_request_t *req,
									  struct cfg_tab_mc_t *mc_cfg_tab)
{
	if (mc_cfg_tab->mc_ctrl_cfg.cfg_mode == CFG_MODE_INDEX_PAIRS) {
		mc_cfg_tab->mc_prefech_cfg.to_use = 1;
		mc_cfg_tab->mc_prefech_cfg.ref_prefetch_enable = 0;
		mc_cfg_tab->mc_prefech_cfg.cur_prefetch_enable = 0;
		mc_cfg_tab->mc_prefech_cfg.ref_first_page =
			req->mc_buff_cfg.ref_first_page >> 12;
		mc_cfg_tab->mc_prefech_cfg.cur_first_page =
			req->mc_buff_cfg.cur_first_page >> 12;
		mc_cfg_tab->mc_index_pairs_cfg.to_use = 1;
		mc_cfg_tab->mc_coord_pairs_cfg.to_use = 0;
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_index_pairs_cfg.cfg_best_dist[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_index_pairs_cfg.cfg_best_dist[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_index_pairs_cfg.cfg_ref_index[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_index_pairs_cfg.cfg_ref_index[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_index_pairs_cfg.cfg_cur_index[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_index_pairs_cfg.cfg_cur_index[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
	}

	return ISP_IPP_OK;
}

static int set_mc_coord_pairs_cfg_tbl(struct msg_req_mc_request_t *req,
									  struct cfg_tab_mc_t *mc_cfg_tab)
{
	if (mc_cfg_tab->mc_ctrl_cfg.cfg_mode == CFG_MODE_COOR_PAIRS) {
		mc_cfg_tab->mc_prefech_cfg.to_use = 0;
		mc_cfg_tab->mc_index_pairs_cfg.to_use = 0;
		mc_cfg_tab->mc_coord_pairs_cfg.to_use = 1;
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_coord_pairs_cfg.cur_cfg_coordinate_y[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_y[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_coord_pairs_cfg.cur_cfg_coordinate_x[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_x[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_coord_pairs_cfg.ref_cfg_coordinate_y[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_coord_pairs_cfg.ref_cfg_coordinate_y[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if_ret(memcpy_s(
						&mc_cfg_tab->mc_coord_pairs_cfg.ref_cfg_coordinate_x[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int),
						&req->mc_hw_config.mc_coord_pairs_cfg.ref_cfg_coordinate_x[0],
						MC_KPT_NUM_MAX * sizeof(unsigned int)));
	}

	return ISP_IPP_OK;
}

static int set_ipp_cfg_mc(struct msg_req_mc_request_t *req,
						  struct cfg_tab_mc_t *mc_cfg_tab)
{
	mc_cfg_tab->to_use = 1;
	mc_cfg_tab->mc_ctrl_cfg.to_use = 1;
	mc_cfg_tab->mc_ctrl_cfg.max_iterations =
		req->mc_hw_config.mc_ctrl_cfg.max_iterations;
	mc_cfg_tab->mc_ctrl_cfg.svd_iterations =
		req->mc_hw_config.mc_ctrl_cfg.svd_iterations;
	mc_cfg_tab->mc_ctrl_cfg.flag_10_11 =
		req->mc_hw_config.mc_ctrl_cfg.flag_10_11;
	mc_cfg_tab->mc_ctrl_cfg.push_inliers_en =
		req->mc_hw_config.mc_ctrl_cfg.push_inliers_en;
	mc_cfg_tab->mc_ctrl_cfg.h_cal_en =
		req->mc_hw_config.mc_ctrl_cfg.h_cal_en;
	mc_cfg_tab->mc_ctrl_cfg.cfg_mode =
		req->mc_hw_config.mc_ctrl_cfg.cfg_mode;
	mc_cfg_tab->mc_ctrl_cfg.mc_en =
		req->mc_hw_config.mc_ctrl_cfg.mc_en;
	mc_cfg_tab->mc_inlier_thld_cfg.to_use = 1;
	mc_cfg_tab->mc_inlier_thld_cfg.inlier_th =
		req->mc_hw_config.mc_inlier_thld_cfg.inlier_th;
	mc_cfg_tab->mc_kpt_num_cfg.to_use = 1;
	mc_cfg_tab->mc_kpt_num_cfg.matched_kpts =
		req->mc_hw_config.mc_kpt_num_cfg.matched_kpts;
	loge_if_ret(set_mc_index_pairs_cfg_tbl(req, mc_cfg_tab));
	loge_if_ret(set_mc_coord_pairs_cfg_tbl(req, mc_cfg_tab));
	return ISP_IPP_OK;
}

static int seg_ipp_mc_module_config(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_mc_t *mc_cfg_tab,
	struct cfg_tab_cvdr_t *mc_cvdr_cfg_tab)
{
	int ret = 0;
	ret |= set_ipp_cfg_mc(mc_request, &mc_cfg_tab[0]);
	ret |= mc_update_cvdr_cfg_tab(mc_request, &mc_cvdr_cfg_tab[0]);
	if (ret != 0) {
		loge(" Failed : mc module config");
		return ISP_IPP_ERR;
	} else {
		return ISP_IPP_OK;
	}
}

static int seg_mc_config_cmdlst_buf(struct msg_req_mc_request_t *mc_request,
									struct cfg_tab_mc_t *mc_cfg_tab,
									struct cfg_tab_cvdr_t *cvdr_cfg_tab,
									struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;
	unsigned int i = 0;
	loge_if_ret(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.read_mode = 0;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0],
									 &cmd_link_entry[i].cmd_buf, &cvdr_cfg_tab[0]));
		loge_if_ret(mc_prepare_cmd(&g_mc_devs[0],
								   &cmd_link_entry[i].cmd_buf, &mc_cfg_tab[0]));
		loge_if_ret(mc_enable_cmd(&g_mc_devs[0],
								  &cmd_link_entry[i++].cmd_buf, &mc_cfg_tab[0]));

		if (mc_request->mc_buff_cfg.inlier_num_buff) {
			cmdlst_wr_cfg.is_incr = 0;
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = mc_request->mc_buff_cfg.inlier_num_buff;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_MC + MC_INLIER_NUMBER_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (mc_request->mc_buff_cfg.h_matrix_buff) {
			cmdlst_wr_cfg.is_incr = 1;
			cmdlst_wr_cfg.data_size = MC_H_MATRIX_NUM;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr =
				mc_request->mc_buff_cfg.h_matrix_buff;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_MC + MC_H_MATRIX_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
							&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (mc_request->mc_buff_cfg.coordinate_pairs_buff) {
			cmdlst_wr_cfg.is_incr = 0;
			cmdlst_wr_cfg.data_size = MC_COORDINATE_PAIRS;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = mc_request->mc_buff_cfg.coordinate_pairs_buff;
			mc_clear_table(&g_mc_devs[0], &cmd_link_entry[i].cmd_buf);
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_MC + MC_COORDINATE_PAIRS_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
							&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cfg_mc_cmdlst(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_mc_t *mc_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;
	stripe_cnt = (mc_request->mc_buff_cfg.inlier_num_buff == 0) ?
				 (stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (mc_request->mc_buff_cfg.h_matrix_buff == 0) ?
				 (stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (mc_request->mc_buff_cfg.coordinate_pairs_buff == 0) ?
				 (stripe_cnt) : (stripe_cnt + 1);

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i != 0) {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_MC_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_MC_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_MC_IRQ_DONE);
			irq = irq | (((unsigned long long)(1)) << IPP_MC_CVDR_VP_RD_EOF_CMDLST);
		}

		logi("mc_stripe=%d, mc irq=0x%llx", i, irq);
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_MC;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_MC_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	loge_if_ret(seg_mc_config_cmdlst_buf(mc_request, mc_cfg_tab, cvdr_cfg_tab, cmdlst_para));
	return ISP_IPP_OK;
}

static int seg_ipp_mc_get_cpe_mem(struct seg_mc_cfg_t **seg_mc_cfg, struct cmdlst_para_t **cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1. get cfg_tbl
	ret = cpe_mem_get(MEM_ID_CFG_TAB_MC, &va, &da);
	if (ret != 0) {
		loge(" Failed : get MEM_ID_CFG_TAB_MC %d", MEM_ID_CFG_TAB_MC);
		return ISP_IPP_ERR;
	}

	*seg_mc_cfg = (struct seg_mc_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_mc_cfg, sizeof(struct seg_mc_cfg_t), 0, sizeof(struct seg_mc_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_mc_cfg_t error!");
		goto mc_get_cpe_mem_failed;
	}

	// 2. get cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_MC, &va, &da);
	if (ret != 0) {
		loge(" Failed : get MEM_ID_CMDLST_PARA_MC %d", MEM_ID_CMDLST_PARA_MC);
		goto mc_get_cpe_mem_failed;
	}

	*cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s cmdlst_para_t error!");
		goto mc_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
mc_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_CFG_TAB_MC);
	return ISP_IPP_ERR;
}

void mc_dump_request(struct msg_req_mc_request_t *req)
{
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	int i = 0;
#endif
	logi("size of msg_req_mc_request_t = 0x%lx",
		 sizeof(struct msg_req_mc_request_t));
	logi("frame_number = %d",
		 req->frame_number);
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logi("MC CFG DUMP:");
	logi("mc_buff_cfg.ref_first_page = 0x%x", req->mc_buff_cfg.ref_first_page);
	logi("mc_buff_cfg.cur_first_page = 0x%x", req->mc_buff_cfg.cur_first_page);
	logi("mc_buff_cfg.inlier_num_buff = 0x%x", req->mc_buff_cfg.inlier_num_buff);
	logi("mc_buff_cfg.h_matrix_buff = 0x%x", req->mc_buff_cfg.h_matrix_buff);
	logi("mc_buff_cfg.coordinate_pairs_buff = 0x%x", req->mc_buff_cfg.coordinate_pairs_buff);
	logi("mc_ctrl_cfg: push_inliers_en= %d", req->mc_hw_config.mc_ctrl_cfg.push_inliers_en);
	logi("mc_ctrl_cfg: svd_iterations= %d", req->mc_hw_config.mc_ctrl_cfg.svd_iterations);
	logi("mc_ctrl_cfg: max_iterations= %d", req->mc_hw_config.mc_ctrl_cfg.max_iterations);
	logi("mc_ctrl_cfg: flag_10_11= %d", req->mc_hw_config.mc_ctrl_cfg.flag_10_11);
	logi("mc_ctrl_cfg: h_cal_en= %d", req->mc_hw_config.mc_ctrl_cfg.h_cal_en);
	logi("mc_ctrl_cfg: cfg_mode= %d", req->mc_hw_config.mc_ctrl_cfg.cfg_mode);
	logi("mc_ctrl_cfg: mc_en= %d", req->mc_hw_config.mc_ctrl_cfg.mc_en);
	logi("mc_inlier_thld_cfg: inlier_th= %d", req->mc_hw_config.mc_inlier_thld_cfg.inlier_th);
	logi("mc_kpt_num_cfg: matched_kpts= %d", req->mc_hw_config.mc_kpt_num_cfg.matched_kpts);

	for (i = 0; i < 4; i++)
		logi("cfg_ref_index[%d]=%d, cfg_cur_index[%d]=%d",
			 i, req->mc_hw_config.mc_index_pairs_cfg.cfg_ref_index[i],
			 i, req->mc_hw_config.mc_index_pairs_cfg.cfg_cur_index[i]);

	for (i = 0; i < 4; i++)
		logi("cur_cfg_coordinate_x[%d]=%d, cur_cfg_coordinate_y[%d]=%d",
			 i, req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_x[i],
			 i, req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_y[i]);

#endif
	return;
}

int mc_request_handler(struct msg_req_mc_request_t *mc_request)
{
	unsigned int mc_mode;
	int ret;
	struct seg_mc_cfg_t *seg_mc_cfg = NULL;
	struct cmdlst_para_t *cmdlst_para = NULL;
	unsigned int mc_token_nbr_en = 0;
	unsigned int nrt_channel  = 1;
	unsigned int cmdlst_channel_value = (mc_token_nbr_en << 7) | (nrt_channel << 8);
	dataflow_check_para(mc_request);
	mc_mode = mc_request->mc_hw_config.mc_ctrl_cfg.cfg_mode;

	if ((mc_mode == CFG_MODE_INDEX_PAIRS) &&
		(mc_request->mc_buff_cfg.cur_first_page == 0
		 || mc_request->mc_buff_cfg.ref_first_page == 0)) {
		loge(" Failed : mc_addr is incorrect");
		return ISP_IPP_ERR;
	}

	mc_dump_request(mc_request);
	hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_MC_CHANNEL_ID * 0x80, cmdlst_channel_value);
	ret = seg_ipp_mc_get_cpe_mem(&seg_mc_cfg, &cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_mc_get_cpe_mem");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_mc_module_config(mc_request, &(seg_mc_cfg->mc_cfg_tab),
								   &(seg_mc_cfg->mc_cvdr_cfg_tab));
	if (ret != 0) {
		loge("Failed : seg_ipp_mc_module_config");
		goto SEG_MC_BUFF_FERR;
	}

	ret = seg_ipp_cfg_mc_cmdlst(mc_request, &(seg_mc_cfg->mc_cfg_tab),
								&(seg_mc_cfg->mc_cvdr_cfg_tab), cmdlst_para);
	if (ret != 0) {
		loge("Failed : seg_ipp_cfg_mc_cmdlst");
		goto SEG_MC_BUFF_FERR;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0)
		loge(" Failed : df_sched_start");

SEG_MC_BUFF_FERR:
	cpe_mem_free(MEM_ID_CFG_TAB_MC);
	return ret;
}
