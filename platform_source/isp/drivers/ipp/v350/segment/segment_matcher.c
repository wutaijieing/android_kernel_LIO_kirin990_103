/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_matcher.c
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
#include <linux/delay.h>
#include "memory.h"
#include "segment_matcher.h"
#include "cfg_table_matcher.h"
#include "cvdr_drv.h"
#include "cvdr_opt.h"
#include "reorder_drv.h"
#include "compare_drv.h"
#include "compare_reg_offset.h"
#include "ipp_top_reg_offset.h"


#define LOG_TAG LOG_MODULE_MATCHER
static int matcher_request_dump(struct msg_req_matcher_t *req);

static int matcher_get_cmp_layer_stripe_cnt(struct msg_req_matcher_t *matcher_request)
{
	unsigned int cmp_layer_stripe_cnt = 1;

	if (matcher_request->req_cmp.streams[0][BO_CMP_MATCHED_INDEX_OUT].buffer)
		cmp_layer_stripe_cnt += 1;

	if (matcher_request->req_cmp.streams[0][BO_CMP_MATCHED_DIST_OUT].buffer)
		cmp_layer_stripe_cnt += 1;

	return cmp_layer_stripe_cnt;
}
static int matcher_reorder_update_cvdr_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_cvdr_t *reorder_cfg_tab, unsigned int layer_num)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int stride = 0;
	enum pix_format_e format;
	struct req_rdr_t *req_rdr = &(matcher_request->req_rdr);

	if (req_rdr->streams[layer_num][BI_RDR_FP].buffer) {
		cfmt.id = IPP_VP_RD_RDR;
		cfmt.width = req_rdr->streams[layer_num][BI_RDR_FP].width;
		cfmt.full_width = req_rdr->streams[layer_num][BI_RDR_FP].width;
		cfmt.line_size = req_rdr->streams[layer_num][BI_RDR_FP].width;
		cfmt.height = req_rdr->streams[layer_num][BI_RDR_FP].height;
		cfmt.expand = EXP_PIX;
		stride = req_rdr->streams[layer_num][BI_RDR_FP].width / 2;
		format = PIXEL_FMT_IPP_D64;
		cfmt.addr = req_rdr->streams[layer_num][BI_RDR_FP].buffer;
		cfg_tbl_cvdr_rd_cfg_d64(reorder_cfg_tab, &cfmt,
								(align_up(144 * 4096, CVDR_ALIGN_BYTES)), stride);
	}

	if (req_rdr->streams[layer_num][BO_RDR_FP_BLOCK].buffer)
		cfg_tbl_cvdr_nr_wr_cfg(reorder_cfg_tab, IPP_NR_WR_RDR);

	return 0;
}

static void matcher_reorder_update_request_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_reorder_t *reorder_cfg_tab, unsigned int layer_num)
{
	struct req_rdr_t *req_rdr = &(matcher_request->req_rdr);
	reorder_cfg_tab->reorder_ctrl_cfg.to_use = 1;
	reorder_cfg_tab->reorder_ctrl_cfg.reorder_en =
		req_rdr->ctrl_cfg[layer_num].reorder_en;
	reorder_cfg_tab->reorder_total_kpt_num.to_use = 1;
	reorder_cfg_tab->reorder_total_kpt_num.total_kpts =
		req_rdr->ctrl_cfg[layer_num].total_kpt;
	reorder_cfg_tab->reorder_block_cfg.to_use = 1;
	reorder_cfg_tab->reorder_block_cfg.blk_v_num =
		req_rdr->block_cfg[layer_num].blk_v_num;
	reorder_cfg_tab->reorder_block_cfg.blk_h_num =
		req_rdr->block_cfg[layer_num].blk_h_num;
	reorder_cfg_tab->reorder_block_cfg.blk_num =
		req_rdr->block_cfg[layer_num].blk_num;
	reorder_cfg_tab->reorder_prefetch_cfg.to_use = 1;
	reorder_cfg_tab->reorder_prefetch_cfg.prefetch_enable = 0;
	reorder_cfg_tab->reorder_prefetch_cfg.first_4k_page =
		req_rdr->streams[layer_num][BO_RDR_FP_BLOCK].buffer >> 12;
	reorder_cfg_tab->reorder_kptnum_cfg.to_use = 1;
	loge_if(memcpy_s(
				&(reorder_cfg_tab->reorder_kptnum_cfg.reorder_kpt_num[0]),
				RDR_KPT_NUM_RANGE_DS4 * sizeof(unsigned int),
				&(req_rdr->reorder_kpt_num[layer_num][0]),
				RDR_KPT_NUM_RANGE_DS4 * sizeof(unsigned int)));
}

static int seg_src_cfg_rdr_cmdlst(
	struct msg_req_matcher_t *matcher_request,
	struct cmdlst_para_t *rdr_cmdlst_para)
{
	unsigned int i = 0;
	unsigned long long irq = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe = rdr_cmdlst_para->cmd_stripe_info;
	unsigned int cmp_layer_stripe_cnt = matcher_get_cmp_layer_stripe_cnt(matcher_request);
	rdr_cmdlst_para->stripe_cnt = matcher_request->rdr_pyramid_layer * STRIPE_NUM_EACH_RDR;

	for (i = 0; i < rdr_cmdlst_para->stripe_cnt; i++) {
		irq = 0;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_RDR;
		cmdlst_stripe[i].en_link = 0;
		cmdlst_stripe[i].ch_link = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
		irq = irq | (((unsigned long long)(1)) << IPP_RDR_CVDR_VP_RD_EOF_CMDSLT);
		irq = irq | (((unsigned long long)(1)) << IPP_RDR_IRQ_DONE);
		irq = irq | (((unsigned long long)(1)) << IPP_RDR_CVDR_VP_RD_EOF_FP);
		cmdlst_stripe[i].hw_priority = CMD_PRIO_LOW;
		cmdlst_stripe[i].irq_mode = irq;

		if ((matcher_request->work_mode & (1 << 1)) == 0) { // rdr0 + rdr1 -> cmp0,  rdr0 + rdr2 => cmp1, ...
			cmdlst_stripe[i].en_link		 = 1;

			if (i > 0) {
				cmdlst_stripe[i].ch_link = IPP_CMP_CHANNEL_ID;
				cmdlst_stripe[i].ch_link_act_nbr = cmp_layer_stripe_cnt;
			}
		} else { // rdr0, cmp0, cmp1, ...
			cmdlst_stripe[i].en_link = 1;
			cmdlst_stripe[i].ch_link = IPP_CMP_CHANNEL_ID;
			cmdlst_stripe[i].ch_link_act_nbr = matcher_request->cmp_pyramid_layer * cmp_layer_stripe_cnt;
			logd("rdr module release cmp token ch_link_act_nbr = %d", cmdlst_stripe[i].ch_link_act_nbr);
		}
	}

	cmdlst_stripe[rdr_cmdlst_para->stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int matcher_compare_update_cvdr_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_cvdr_t *compare_cfg_tab, unsigned int layer_num)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int stride = 0;
	enum pix_format_e format;
	struct req_cmp_t *req_cmp = &(matcher_request->req_cmp);

	if (req_cmp->streams[layer_num][BI_CMP_REF_FP].buffer) {
		cfmt.id = IPP_VP_RD_CMP;
		cfmt.width = req_cmp->streams[layer_num][BI_CMP_REF_FP].width;
		cfmt.full_width = req_cmp->streams[layer_num][BI_CMP_REF_FP].width;
		cfmt.line_size = req_cmp->streams[layer_num][BI_CMP_REF_FP].width;
		cfmt.height = req_cmp->streams[layer_num][BI_CMP_REF_FP].height;
		cfmt.expand = 1;
		format = PIXEL_FMT_IPP_D64;
		stride   = cfmt.width / 2; // means: 20*8/16
		cfmt.addr = req_cmp->streams[layer_num][BI_CMP_REF_FP].buffer;
		cfg_tbl_cvdr_rd_cfg_d64(compare_cfg_tab, &cfmt,
								(align_up(CMP_IN_INDEX_NUM,
										  CVDR_ALIGN_BYTES)), stride);
	}

	if (req_cmp->streams[layer_num][BI_CMP_CUR_FP].buffer)
		cfg_tbl_cvdr_nr_rd_cfg(compare_cfg_tab, IPP_NR_RD_CMP);

	return 0;
}

static void matcher_compare_update_request_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_compare_t *compare_cfg_tab, unsigned int layer_num)
{
	unsigned int i = 0;
	struct req_cmp_t *req_cmp = &(matcher_request->req_cmp);
	compare_cfg_tab->compare_ctrl_cfg.to_use   = 1;
	compare_cfg_tab->compare_search_cfg.to_use = 1;
	compare_cfg_tab->compare_stat_cfg.to_use   = 1;
	compare_cfg_tab->compare_block_cfg.to_use  = 1;
	compare_cfg_tab->compare_prefetch_cfg.to_use = 1;
	compare_cfg_tab->compare_kptnum_cfg.to_use = 1;
	compare_cfg_tab->compare_total_kptnum_cfg.to_use = 1;
	compare_cfg_tab->compare_offset_cfg.to_use = 1;
	compare_cfg_tab->compare_ctrl_cfg.compare_en = req_cmp->ctrl_cfg[layer_num].compare_en;
	compare_cfg_tab->compare_total_kptnum_cfg.total_kptnum = req_cmp->total_kptnum_cfg[layer_num].total_kptnum;
	compare_cfg_tab->compare_block_cfg.blk_v_num = req_cmp->block_cfg[layer_num].blk_v_num;
	compare_cfg_tab->compare_block_cfg.blk_h_num = req_cmp->block_cfg[layer_num].blk_h_num;
	compare_cfg_tab->compare_block_cfg.blk_num = req_cmp->block_cfg[layer_num].blk_num;
	compare_cfg_tab->compare_search_cfg.v_radius = req_cmp->search_cfg[layer_num].v_radius;
	compare_cfg_tab->compare_search_cfg.h_radius = req_cmp->search_cfg[layer_num].h_radius;
	compare_cfg_tab->compare_search_cfg.dis_ratio =
		req_cmp->search_cfg[layer_num].dis_ratio;
	compare_cfg_tab->compare_search_cfg.dis_threshold =
		req_cmp->search_cfg[layer_num].dis_threshold;
	compare_cfg_tab->compare_offset_cfg.cenh_offset =
		req_cmp->offset_cfg[layer_num].cenh_offset;
	compare_cfg_tab->compare_offset_cfg.cenv_offset =
		req_cmp->offset_cfg[layer_num].cenv_offset;
	compare_cfg_tab->compare_stat_cfg.stat_en =
		req_cmp->stat_cfg[layer_num].stat_en;
	compare_cfg_tab->compare_stat_cfg.max3_ratio =
		req_cmp->stat_cfg[layer_num].max3_ratio;
	compare_cfg_tab->compare_stat_cfg.bin_factor =
		req_cmp->stat_cfg[layer_num].bin_factor;
	compare_cfg_tab->compare_stat_cfg.bin_num =
		req_cmp->stat_cfg[layer_num].bin_num;
	compare_cfg_tab->compare_prefetch_cfg.prefetch_enable = 0;
	compare_cfg_tab->compare_prefetch_cfg.first_4k_page =
		req_cmp->streams[layer_num][BI_CMP_CUR_FP].buffer >> 12;

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4; i++) {
		compare_cfg_tab->compare_kptnum_cfg.compare_ref_kpt_num[i] =
			req_cmp->kptnum_cfg[layer_num].compare_ref_kpt_num[i];
		compare_cfg_tab->compare_kptnum_cfg.compare_cur_kpt_num[i] =
			req_cmp->kptnum_cfg[layer_num].compare_cur_kpt_num[i];
	}
}

static int seg_src_cfg_cmp_cmdlst(struct msg_req_matcher_t *matcher_request,
								  struct cmdlst_para_t *cmp_cmdlst_para)
{
	unsigned int i = 0;
	unsigned int cmp_layer_stripe_cnt = matcher_get_cmp_layer_stripe_cnt(matcher_request);
	unsigned int stripe_cnt = matcher_request->cmp_pyramid_layer * cmp_layer_stripe_cnt;
	unsigned long long irq = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe = cmp_cmdlst_para->cmd_stripe_info;
	cmp_cmdlst_para->stripe_cnt = stripe_cnt;

	if (cmp_layer_stripe_cnt == 0)
		return ISP_IPP_ERR;

	for (i = 0; i < stripe_cnt; i++) {
		logi("cmp stripe_cnt = %d, the current stripe i = %d", stripe_cnt, i);
		irq = 0;

		if (i % cmp_layer_stripe_cnt != 0) {  // cmdlst read matched out
			cmdlst_stripe[i].hw_priority  = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_IRQ_DONE);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_FP);
		}

		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_CMP;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int compare_set_cmdlst_wr_buf(struct msg_req_matcher_t *matcher_request,
									 struct schedule_cmdlst_link_t *cmd_link_entry,
									 unsigned int cur_layer, unsigned int *idx)
{
	unsigned int i = *idx;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;

	if (matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_INDEX_OUT].buffer) {
		cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_INDEX_0_REG;
		cmdlst_wr_cfg.buff_wr_addr = matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_INDEX_OUT].buffer;
		// from v300, alg need 0x4f0 match_points reg val, so add 0x10 incr;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf_cmp(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg, 0x10));
	}

	if (matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_DIST_OUT].buffer) {
		cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_DISTANCE_0_REG;
		cmdlst_wr_cfg.buff_wr_addr = matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_DIST_OUT].buffer;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}

	*idx = i;
	return ISP_IPP_OK;
}

static int seg_src_set_cmp_cmdlst_para(
	struct cmdlst_para_t *cmdlst_para,
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_compare_t *compare_cfg_tab,
	struct cfg_tab_cvdr_t *compare_cvdr_cfg_tab)
{
	unsigned int i = 0;
	unsigned int cur_layer = 0;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	unsigned int cmp_layer_stripe_num = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry = (struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	loge_if_ret(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;
	cmp_layer_stripe_num = matcher_get_cmp_layer_stripe_cnt(matcher_request);;

	if (cmp_layer_stripe_num == 0)
		return ISP_IPP_ERR;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		cur_layer = i / cmp_layer_stripe_num;
		logi("cmp stripe_cnt = %d, cur_layer= %d, i = %d", cmdlst_para->stripe_cnt, cur_layer, i);
		loge_if_ret(cvdr_prepare_nr_cmd(&g_cvdr_devs[0], &cmd_link_entry[i].cmd_buf,
										&compare_cvdr_cfg_tab[cur_layer]));
		loge_if_ret(compare_prepare_cmd(&g_compare_devs[0], &cmd_link_entry[i].cmd_buf,
										&compare_cfg_tab[cur_layer]));
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i++].cmd_buf,
									 &compare_cvdr_cfg_tab[cur_layer]));
		loge_if_ret(compare_set_cmdlst_wr_buf(matcher_request, cmd_link_entry, cur_layer, &i));
	}

	return ISP_IPP_OK;
}

void cmdlst_channel_cfg_init(
	unsigned int chan_id,
	unsigned int token_nbr_en,
	unsigned int nrt_channel)
{
	unsigned int cmdlst_channel_value =
		(token_nbr_en << 7) | (nrt_channel << 8);
	hispcpe_reg_set(CMDLIST_REG, 0x80 +
					chan_id * 0x80, cmdlst_channel_value);
}

static int orb_matcher_check_para(
	struct msg_req_matcher_t *matcher_req)
{
	unsigned int i = 0;

	if (matcher_req->cmp_pyramid_layer > 8
		|| matcher_req->rdr_pyramid_layer > 8 ||
		(matcher_req->cmp_pyramid_layer == 0 &&
		 matcher_req->rdr_pyramid_layer == 0)) {
		loge(" Failed : matcher_pyramid_layer  is incorrect");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < matcher_req->rdr_pyramid_layer; i++) {
		if (matcher_req->req_rdr.streams[i][BI_RDR_FP].buffer == 0
			|| matcher_req->req_rdr.streams[i][BO_RDR_FP_BLOCK].buffer == 0) {
			loge(" Failed: rdr_addr  is incorrect");
			return ISP_IPP_ERR;
		}
	}

	for (i = 0; i < matcher_req->cmp_pyramid_layer; i++) {
		if (matcher_req->req_cmp.streams[i][BI_CMP_REF_FP].buffer == 0
			|| matcher_req->req_cmp.streams[i][BI_CMP_CUR_FP].buffer == 0) {
			loge(" Failed: cmp_addr  is incorrect");
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

static int seg_matcher_config_rdr_cmdlst_buf(struct cmdlst_para_t *reorder_cmdlst_para,
		struct seg_matcher_rdr_cfg_t *seg_reorder_cfg)
{
	struct schedule_cmdlst_link_t *rdr_cmd_link_entry = (struct schedule_cmdlst_link_t *)reorder_cmdlst_para->cmd_entry;
	unsigned int i = 0;

	for (i = 0; i < reorder_cmdlst_para->stripe_cnt; i++) {
		loge_if_ret(cvdr_prepare_nr_cmd(
						&g_cvdr_devs[0],
						&rdr_cmd_link_entry[i].cmd_buf,
						&(seg_reorder_cfg->reorder_cvdr_cfg_tab[i])));
		loge_if_ret(reorder_prepare_cmd(
						&g_reorder_devs[0],
						&rdr_cmd_link_entry[i].cmd_buf,
						&(seg_reorder_cfg->reorder_cfg_tab[i])));
		loge_if_ret(cvdr_prepare_cmd(
						&g_cvdr_devs[0],
						&rdr_cmd_link_entry[i].cmd_buf,
						&(seg_reorder_cfg->reorder_cvdr_cfg_tab[i])));
	}

	return ISP_IPP_OK;
}

static int seg_matcher_rdr_get_cpe_mem(struct seg_matcher_rdr_cfg_t **seg_reorder_cfg,
									   struct cmdlst_para_t **reorder_cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1.cfg_tbl
	ret = cpe_mem_get(MEM_ID_REORDER_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_REORDER_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_reorder_cfg = (struct seg_matcher_rdr_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_reorder_cfg, sizeof(struct seg_matcher_rdr_cfg_t), 0, sizeof(struct seg_matcher_rdr_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_reorder_cfg error!");
		goto matcher_rdr_get_cpe_mem_failed;
	}

	// 2.cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_REORDER, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_REORDER);
		goto matcher_rdr_get_cpe_mem_failed;
	}

	*reorder_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*reorder_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s reorder_cmdlst_para error!");
		goto matcher_rdr_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
matcher_rdr_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	return ISP_IPP_ERR;
}

int rdr_request_handler(struct msg_req_matcher_t *matcher_request)
{
	int ret = 0;
	unsigned int i = 0;
	struct seg_matcher_rdr_cfg_t *seg_reorder_cfg = NULL;
	struct cmdlst_para_t *reorder_cmdlst_para = NULL;

	if (matcher_request->rdr_pyramid_layer == 0)
		return ISP_IPP_OK;

	loge_if_ret(seg_matcher_rdr_get_cpe_mem(&seg_reorder_cfg, &reorder_cmdlst_para));
	reorder_cmdlst_para->channel_id = IPP_RDR_CHANNEL_ID;
	reorder_cmdlst_para->stripe_cnt = matcher_request->rdr_pyramid_layer;
	loge_if(seg_src_cfg_rdr_cmdlst(matcher_request, reorder_cmdlst_para));

	for (i = 0; i < reorder_cmdlst_para->stripe_cnt; i++) {
		matcher_reorder_update_request_cfg_tab(matcher_request,
											   &(seg_reorder_cfg->reorder_cfg_tab[i]), i);
		loge_if(matcher_reorder_update_cvdr_cfg_tab(matcher_request,
				&(seg_reorder_cfg->reorder_cvdr_cfg_tab[i]), i));
	}

	ret = df_sched_prepare(reorder_cmdlst_para);
	if (ret != 0) {
		loge("Failed : df_sched_prepare");
		goto SEG_MATCHER_RDR_BUFF_FERR;
	}

	ret = seg_matcher_config_rdr_cmdlst_buf(reorder_cmdlst_para, seg_reorder_cfg);
	if (ret != 0) {
		loge("Failed : seg_matcher_config_rdr_cmdlst_buf");
		goto SEG_MATCHER_RDR_BUFF_FERR;
	}

	ret = df_sched_start(reorder_cmdlst_para);
	if (ret != 0) loge("Failed : df_sched_start");

SEG_MATCHER_RDR_BUFF_FERR:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	return ret;
}

static int seg_matcher_cmp_get_cpe_mem(struct seg_matcher_cmp_cfg_t **seg_compare_cfg,
									   struct cmdlst_para_t **compare_cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1. cfg_tbl
	ret = cpe_mem_get(MEM_ID_COMPARE_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge(" Failed : cpe_mem_get %d", MEM_ID_COMPARE_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_compare_cfg = (struct seg_matcher_cmp_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_compare_cfg, sizeof(struct seg_matcher_cmp_cfg_t), 0, sizeof(struct seg_matcher_cmp_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_compare_cfg error!");
		goto matcher_cmp_get_cpe_mem_failed;
	}

	// 2.cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_COMPARE, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_COMPARE);
		goto matcher_cmp_get_cpe_mem_failed;
	}

	*compare_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*compare_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s compare_cmdlst_para error!");
		goto matcher_cmp_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
matcher_cmp_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	return ISP_IPP_ERR;
}

int cmp_request_handler(struct msg_req_matcher_t *matcher_request)
{
	int ret = 0;
	unsigned int i = 0;
	struct seg_matcher_cmp_cfg_t *seg_compare_cfg = NULL;
	struct cmdlst_para_t *compare_cmdlst_para = NULL;

	if (matcher_request->cmp_pyramid_layer == 0)
		return ISP_IPP_OK;

	loge_if_ret(seg_matcher_cmp_get_cpe_mem(&seg_compare_cfg, &compare_cmdlst_para));
	compare_cmdlst_para->channel_id = IPP_CMP_CHANNEL_ID;

	for (i = 0; i < matcher_request->cmp_pyramid_layer; i++) {
		matcher_compare_update_request_cfg_tab(matcher_request,
											   &(seg_compare_cfg->compare_cfg_tab[i]), i);
		loge_if(matcher_compare_update_cvdr_cfg_tab(matcher_request,
				&(seg_compare_cfg->compare_cvdr_cfg_tab[i]), i));
	}

	loge_if(seg_src_cfg_cmp_cmdlst(matcher_request, compare_cmdlst_para));
	ret = df_sched_prepare(compare_cmdlst_para);
	if (ret != 0) {
		loge("Failed : df_sched_prepare");
		goto SEG_MATCHER_CMP_BUFF_FERR;
	}

	ret = seg_src_set_cmp_cmdlst_para(compare_cmdlst_para, matcher_request,
									  &(seg_compare_cfg->compare_cfg_tab[0]),
									  &(seg_compare_cfg->compare_cvdr_cfg_tab[0]));
	if (ret != 0) {
		loge("Failed : seg_src_set_cmp_cmdlst_para");
		goto SEG_MATCHER_CMP_BUFF_FERR;
	}

	ret = df_sched_start(compare_cmdlst_para);
	if (ret != 0)
		loge("Failed : df_sched_start");

SEG_MATCHER_CMP_BUFF_FERR:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	return ret;
}

int matcher_request_handler(struct msg_req_matcher_t *matcher_request)
{
	int ret = 0;
	unsigned int rdr_token_nbr_en = 0;
	unsigned int cmp_token_nbr_en = 0;
	unsigned int nrt_channel  = 1;
	logd("+");

	if (matcher_request == NULL) {
		loge("Failed : matcher_request is null");
		return ISP_IPP_ERR;
	}

	ret = orb_matcher_check_para(matcher_request);
	if (ret != 0) {
		loge(" Failed : orb_matcher_check_para");
		return ISP_IPP_ERR;
	}

	matcher_request_dump(matcher_request);
	cmp_token_nbr_en = (matcher_request->rdr_pyramid_layer == 0) ? (0) : (1);
	cmdlst_channel_cfg_init(IPP_RDR_CHANNEL_ID, rdr_token_nbr_en, nrt_channel);
	cmdlst_channel_cfg_init(IPP_CMP_CHANNEL_ID, cmp_token_nbr_en, nrt_channel);
	loge_if_ret(cmp_request_handler(matcher_request));
	loge_if_ret(rdr_request_handler(matcher_request));
	logd("-");
	return ISP_IPP_OK;
}

int matcher_eof_handler(
	struct msg_req_matcher_t *matcher_request)
{
	if (matcher_request->rdr_pyramid_layer != 0)
		loge_if_ret(ipp_eop_handler(CMD_EOF_RDR_MODE));

	if (matcher_request->cmp_pyramid_layer != 0)
		loge_if_ret(ipp_eop_handler(CMD_EOF_CMP_MODE));

	return ISP_IPP_OK;
}

static int matcher_request_dump(
	struct msg_req_matcher_t *req)
{
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logd("size_of_matcher_req = %d", (int)(sizeof(struct msg_req_matcher_t)));
	logd("req->frame_number      = %d", req->frame_number);
	logi("req->rdr_pyramid_layer = %d", req->rdr_pyramid_layer);
	logd("req->cmp_pyramid_layer = %d", req->cmp_pyramid_layer);
	logd("req->work_mode         = %d", req->work_mode);
	logd(" req->matcher_rate_value      = %d", req->matcher_rate_value);
#else
	logi("req->rdr_pyramid_layer = %d", req->rdr_pyramid_layer);
#endif
	return 0;
}

void rdr_request_dump(struct msg_req_matcher_t *req_matcher)
{
	unsigned int i = 0;
	unsigned int j = 0;
	char *stream_name[] = {
		"BI_RDR_FP",
		"BO_RDR_FP_BLOCK",
	};
	struct req_rdr_t *req = &req_matcher->req_rdr;
	logi("req_matcher->frame_number = %d", req_matcher->frame_number);
	logi("req_matcher->rdr_pyramid_layer = %d", req_matcher->rdr_pyramid_layer);

	for (i = 0; i < req_matcher->rdr_pyramid_layer; i++) {
		for (j = 0; j < RDR_STREAM_MAX; j++) {
			if (req->streams[i][j].width != 0) {
				logi("streams[%d][%s].width = %d", i, stream_name[j], req->streams[i][j].width);
				logi("streams[%d][%s].height = %d", i, stream_name[j], req->streams[i][j].height);
				logi("streams[%d][%s].stride = %d", i, stream_name[j], req->streams[i][j].stride);
				logi("streams[%d][%s].buf = 0x%x", i, stream_name[j], req->streams[i][j].buffer);
				logi("streams[%d][%s].format = %d", i, stream_name[j], req->streams[i][j].format);
			}
		}

		logi("RDR CFG DUMP:");
		logi("req->ctrl_cfg.reorder_en = %d", req->ctrl_cfg[i].reorder_en);
		logi("req->ctrl_cfg.total_kpt = %d", req->ctrl_cfg[i].total_kpt);
		logi("req->block_cfg.blk_v_num = %d", req->block_cfg[i].blk_v_num);
		logi("req->block_cfg.blk_h_num = %d", req->block_cfg[i].blk_h_num);
		logi("req->block_cfg.blk_num = %d", req->block_cfg[i].blk_num);
	}
}

void cmp_request_dump(struct msg_req_matcher_t *req_matcher)
{
	unsigned int i = 0;
	unsigned int j = 0;
	char *stream_name[] = {
		"BI_COMPARE_DESC_REF",
		"BI_COMPARE_DESC_CUR",
	};
	struct req_cmp_t *req = &req_matcher->req_cmp;
	logi("req_matcher->cmp_pyramid_layer = %d", req_matcher->cmp_pyramid_layer);

	for (i = 0; i < req_matcher->cmp_pyramid_layer; i++) {
		for (j = 0; j < CMP_STREAM_MAX; j++) {
			if (req->streams[i][j].width != 0) {
				logi("streams[%d][%s].width = %d", i, stream_name[j], req->streams[i][j].width);
				logi("streams[%d][%s].height = %d", i, stream_name[j], req->streams[i][j].height);
				logi("streams[%d][%s].stride = %d", i, stream_name[j], req->streams[i][j].stride);
				logi("streams[%d][%s].buf = 0x%x", i, stream_name[j], req->streams[i][j].buffer);
				logi("streams[%d][%s].format = %d", i, stream_name[j], req->streams[i][j].format);
			}
		}

		logi("CMP CFG DUMP:");
		logi("compare_ctrl_cfg.compare_en = %d", req->ctrl_cfg[i].compare_en);
		logi("compare_block_cfg.blk_v_num = %d", req->block_cfg[i].blk_v_num);
		logi("compare_block_cfg.blk_h_num = %d", req->block_cfg[i].blk_h_num);
		logi("compare_block_cfg.blk_num = %d", req->block_cfg[i].blk_num);
		logi("compare_search_cfg.v_radius = %d", req->search_cfg[i].v_radius);
		logi("compare_search_cfg.h_radius = %d", req->search_cfg[i].h_radius);
		logi("compare_search_cfg.dis_ratio = %d", req->search_cfg[i].dis_ratio);
		logi("compare_search_cfg.dis_threshold = %d", req->search_cfg[i].dis_threshold);
		logi("compare_offset_cfg.cenh_offset = %d", req->offset_cfg[i].cenh_offset);
		logi("ompare_offset_cfg.cenv_offset = %d", req->offset_cfg[i].cenv_offset);
		logi("compare_stat_cfg.stat_en = %d", req->stat_cfg[i].stat_en);
		logi("compare_stat_cfg.max3_ratio = %d", req->stat_cfg[i].max3_ratio);
		logi("compare_stat_cfg.bin_factor = %d", req->stat_cfg[i].bin_factor);
		logi("compare_stat_cfg.bin_num = %d", req->stat_cfg[i].bin_num);
	}
}
// end
