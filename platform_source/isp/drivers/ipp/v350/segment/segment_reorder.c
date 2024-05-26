/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_reorder.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <securec.h>
#include "segment_reorder.h"
#include "cfg_table_reorder.h"
#include "cvdr_drv.h"
#include "reorder_drv.h"
#include "cfg_table_cvdr.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "reorder_drv_priv.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_REORDER
#define IPP_REORDER_CVDR_WIDTH 18

static void reorder_request_dump(struct msg_req_reorder_request_t *req);

static int rdr_update_cvdr_cfg_tab(
	struct msg_req_reorder_request_t *reorder_request,
	struct cfg_tab_cvdr_t *reorder_cvdr_cfg_tab)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int stride = 0;
	loge_if_ret(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (reorder_request->streams[BI_RDR_FP].buffer) {
		cfmt.id = IPP_VP_RD_RDR;
		cfmt.width = IPP_REORDER_CVDR_WIDTH;
		cfmt.line_size = cfmt.width;
		cfmt.height = reorder_request->streams[BI_RDR_FP].height;
		stride = cfmt.width / 2;
		cfmt.addr = reorder_request->streams[BI_RDR_FP].buffer;
		loge_if_ret(cfg_tbl_cvdr_rd_cfg_d64(reorder_cvdr_cfg_tab,
											&cfmt, (align_up(144 * 4096, CVDR_ALIGN_BYTES)), stride));
	}

	if (reorder_request->reorder_first_4k_page)
		cfg_tbl_cvdr_nr_wr_cfg(reorder_cvdr_cfg_tab, IPP_NR_WR_RDR);

	return 0;
}

static int set_ipp_cfg_rdr(struct msg_req_reorder_request_t *reorder_request,
						   struct cfg_tab_reorder_t *reorder_cfg_tab)
{
	reorder_cfg_tab->reorder_ctrl_cfg.to_use = 1;
	reorder_cfg_tab->reorder_ctrl_cfg.reorder_en =
		reorder_request->reg_cfg.reorder_ctrl_cfg.reorder_en;
	reorder_cfg_tab->reorder_total_kpt_num.to_use = 1;
	reorder_cfg_tab->reorder_total_kpt_num.total_kpts =
		reorder_request->reg_cfg.reorder_ctrl_cfg.total_kpt;
	reorder_cfg_tab->reorder_block_cfg.to_use = 1;
	reorder_cfg_tab->reorder_block_cfg.blk_v_num =
		reorder_request->reg_cfg.reorder_block_cfg.blk_v_num;
	reorder_cfg_tab->reorder_block_cfg.blk_h_num =
		reorder_request->reg_cfg.reorder_block_cfg.blk_h_num;
	reorder_cfg_tab->reorder_block_cfg.blk_num =
		reorder_request->reg_cfg.reorder_block_cfg.blk_num;
	reorder_cfg_tab->reorder_prefetch_cfg.to_use = 1;
	reorder_cfg_tab->reorder_prefetch_cfg.prefetch_enable = 0;
	reorder_cfg_tab->reorder_prefetch_cfg.first_4k_page =
		reorder_request->reorder_first_4k_page >> 12;
	reorder_cfg_tab->reorder_kptnum_cfg.to_use = 1;
	loge_if_ret(memcpy_s(
					&(reorder_cfg_tab->reorder_kptnum_cfg.reorder_kpt_num[0]),
					RDR_KPT_NUM_RANGE_DS4 * sizeof(unsigned int),
					&(reorder_request->reg_cfg.reorder_kpt_num[0]),
					RDR_KPT_NUM_RANGE_DS4 * sizeof(unsigned int)));
	return ISP_IPP_OK;
}

static int seg_ipp_rdr_module_config(
	struct msg_req_reorder_request_t *rdr_request,
	struct seg_reorder_cfg_t *seg_reorder_cfg)
{
	int ret = 0;
	unsigned int stripe_cnt = 1;
	ret |= set_ipp_cfg_rdr(rdr_request,
						   &(seg_reorder_cfg[stripe_cnt - 1].reorder_cfg_tab));
	ret |= rdr_update_cvdr_cfg_tab(rdr_request,
								   &(seg_reorder_cfg[stripe_cnt - 1].reorder_cvdr_cfg_tab));
	if (ret != 0) {
		loge("Failed : rdr module config");
		return ISP_IPP_ERR;
	} else {
		return ISP_IPP_OK;
	}
}

static int seg_ipp_cfg_rdr_cmdlst_para(
	struct msg_req_reorder_request_t *rdr_request,
	struct seg_reorder_cfg_t *seg_reorder_cfg,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;
		cmdlst_stripe[i].resource_share  = 1 <<
										   IPP_CMD_RES_SHARE_RDR;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_RDR_CVDR_VP_RD_EOF_CMDSLT);
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_RDR_IRQ_DONE);
		irq = irq | (((unsigned long long)(1)) <<
					 IPP_RDR_CVDR_VP_RD_EOF_FP);
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].irq_mode        = irq;
		logi("rdr_stripe=%d, irq=0x%llx", i, irq);
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_RDR_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		loge_if_ret(cvdr_prepare_nr_cmd(&g_cvdr_devs[0],
										&cmd_link_entry[0].cmd_buf, &(seg_reorder_cfg[0].reorder_cvdr_cfg_tab)));
		loge_if_ret(reorder_prepare_cmd(&g_reorder_devs[0],
										&cmd_link_entry[0].cmd_buf, &(seg_reorder_cfg[0].reorder_cfg_tab)));
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0],
									 &cmd_link_entry[0].cmd_buf, &(seg_reorder_cfg[0].reorder_cvdr_cfg_tab)));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_rdr_get_cpe_mem(struct seg_reorder_cfg_t **seg_reorder_cfg,
								   struct cmdlst_para_t **reorder_cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1.cfg tbl buffer
	ret = cpe_mem_get(MEM_ID_REORDER_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_REORDER_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_reorder_cfg = (struct seg_reorder_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_reorder_cfg, sizeof(struct seg_reorder_cfg_t), 0, sizeof(struct seg_reorder_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_reorder_cfg error!");
		goto reorder_get_cpe_mem_failed;
	}

	// 2. cmdlst para buffer
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_REORDER, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_REORDER);
		goto reorder_get_cpe_mem_failed;
	}

	*reorder_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*reorder_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s reorder_cmdlst_para error!");
		goto reorder_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
reorder_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	return ISP_IPP_ERR;
}

int reorder_request_handler(
	struct msg_req_reorder_request_t *reorder_request)
{
	int ret = 0;
	struct seg_reorder_cfg_t *seg_reorder_cfg = NULL;
	struct cmdlst_para_t *reorder_cmdlst_para = NULL;
	dataflow_check_para(reorder_request);
	logd("+");
	reorder_request_dump(reorder_request);
#if IPP_UT_DEBUG
	set_dump_register_init(UT_REG_ADDR, IPP_MAX_REG_OFFSET, 0);
#endif
	ret = seg_ipp_rdr_get_cpe_mem(&seg_reorder_cfg, &reorder_cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_rdr_get_cpe_mem");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_rdr_module_config(reorder_request, seg_reorder_cfg);
	if (ret != 0) {
		loge(" Failed : seg_ipp_rdr_module_config");
		goto SEG_REORDER_BUFF_FREE;
	}

	ret = seg_ipp_cfg_rdr_cmdlst_para(reorder_request, seg_reorder_cfg, reorder_cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_cfg_rdr_cmdlst_para");
		goto SEG_REORDER_BUFF_FREE;
	}

	ret = df_sched_start(reorder_cmdlst_para);
	if (ret != 0)
		loge(" Failed : df_sched_start");

	logd("-");
SEG_REORDER_BUFF_FREE:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	return ret;
}

static void reorder_request_dump(struct msg_req_reorder_request_t *req)
{
	unsigned int i = 0;
	logi("sizeof(struct msg_req_reorder_request_t) = 0x%x",
		 (int)(sizeof(struct msg_req_reorder_request_t)));
	logi("frame_number = %d", req->frame_number);

	for (i = 0; i < RDR_STREAM_MAX; i++) {
		logi("streams[%d].width = %d", i, req->streams[i].width);
		logi("streams[%d].height = %d", i, req->streams[i].height);
		logi("streams[%d].stride = %d", i, req->streams[i].stride);
		logi("streams[%d].buf = 0x%x", i, req->streams[i].buffer);
		logi("streams[%d].format = %d", i, req->streams[i].format);
	}

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logi("RDR CFG DUMP:");
	logi("req->reorder_ctrl_cfg.reorder_en = %d",
		 req->reg_cfg.reorder_ctrl_cfg.reorder_en);
	logi("req->reorder_ctrl_cfg.total_kpt = %d",
		 req->reg_cfg.reorder_ctrl_cfg.total_kpt);
	logi("req->reorder_block_cfg.blk_v_num = %d",
		 req->reg_cfg.reorder_block_cfg.blk_v_num);
	logi("req->reorder_block_cfg.blk_h_num = %d",
		 req->reg_cfg.reorder_block_cfg.blk_h_num);
	logi("req->reorder_block_cfg.blk_num = %d",
		 req->reg_cfg.reorder_block_cfg.blk_num);
	logi("req->reorder_prefetch_cfg.first_4k_page = 0x%x",
		 req->reorder_first_4k_page);
	logi("req->reg_cfg.reorder_kptnum_cfg.reorder_kpt_num =");

	for (i = 0; i < RDR_KPT_NUM_RANGE_DS4 / 4 - 1; i++)
		logi("0x%08x  0x%08x  0x%08x  0x%08x",
			 req->reg_cfg.reorder_kpt_num[0 + 4 * i],
			 req->reg_cfg.reorder_kpt_num[1 + 4 * i],
			 req->reg_cfg.reorder_kpt_num[2 + 4 * i],
			 req->reg_cfg.reorder_kpt_num[3 + 4 * i]);

#endif
	return;
}

