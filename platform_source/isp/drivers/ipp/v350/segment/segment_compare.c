/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_compare.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/

#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "segment_compare.h"
#include "cfg_table_compare.h"
#include "cvdr_drv.h"
#include "compare_drv.h"
#include "cfg_table_cvdr.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "compare_drv_priv.h"
#include "compare_reg_offset.h"
#include "ipp_top_reg_offset.h"


#define LOG_TAG LOG_MODULE_COMPARE
#define IPP_COMPARE_CVDR_WIDTH 20

static void compare_request_dump(
	struct msg_req_compare_request_t *req);

static int cmp_update_cvdr_cfg_tab(
	struct msg_req_compare_request_t *compare_request,
	struct cfg_tab_cvdr_t *cmp_cvdr_cfg_tab)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int height;
	unsigned int addr;
	unsigned int stride = 0;
	enum pix_format_e format;
	height = compare_request->streams[BI_COMPARE_DESC_REF].height;
	addr = compare_request->streams[BI_COMPARE_DESC_REF].buffer;
	loge_if_ret(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (compare_request->streams[BI_COMPARE_DESC_REF].buffer) {
		cfmt.id = IPP_VP_RD_CMP;
		cfmt.width = IPP_COMPARE_CVDR_WIDTH;
		cfmt.line_size = cfmt.width;
		cfmt.height = height;
		cfmt.expand = 1;
		format = PIXEL_FMT_IPP_D64;
		stride   = cfmt.width / 2; // means : 20*8/16
		cfmt.addr = addr;
		loge_if_ret(cfg_tbl_cvdr_rd_cfg_d64(cmp_cvdr_cfg_tab, &cfmt,
											(align_up(CMP_IN_INDEX_NUM, CVDR_ALIGN_BYTES)), stride));
	}

	if (compare_request->streams[BI_COMPARE_DESC_CUR].buffer)
		loge_if_ret(cfg_tbl_cvdr_nr_rd_cfg(
						cmp_cvdr_cfg_tab, IPP_NR_RD_CMP));

	return ISP_IPP_OK;
}

static int set_ipp_cfg_cmp(
	struct msg_req_compare_request_t *cmp_req,
	struct cfg_tab_compare_t *cmp_cfg_tab)
{
	unsigned int i = 0;
	cmp_cfg_tab->compare_ctrl_cfg.to_use   = 1;
	cmp_cfg_tab->compare_ctrl_cfg.compare_en =
		cmp_req->reg_cfg.compare_ctrl_cfg.compare_en;
	cmp_cfg_tab->compare_search_cfg.to_use = 1;
	cmp_cfg_tab->compare_search_cfg.v_radius =
		cmp_req->reg_cfg.compare_search_cfg.v_radius;
	cmp_cfg_tab->compare_search_cfg.h_radius =
		cmp_req->reg_cfg.compare_search_cfg.h_radius;
	cmp_cfg_tab->compare_search_cfg.dis_ratio =
		cmp_req->reg_cfg.compare_search_cfg.dis_ratio;
	cmp_cfg_tab->compare_search_cfg.dis_threshold = cmp_req->reg_cfg.compare_search_cfg.dis_threshold;
	cmp_cfg_tab->compare_offset_cfg.to_use = 1;
	cmp_cfg_tab->compare_offset_cfg.cenh_offset = cmp_req->reg_cfg.compare_offset_cfg.cenh_offset;
	cmp_cfg_tab->compare_offset_cfg.cenv_offset = cmp_req->reg_cfg.compare_offset_cfg.cenv_offset;
	cmp_cfg_tab->compare_stat_cfg.to_use   = 1;
	cmp_cfg_tab->compare_stat_cfg.stat_en = cmp_req->reg_cfg.compare_stat_cfg.stat_en;
	cmp_cfg_tab->compare_stat_cfg.max3_ratio =
		cmp_req->reg_cfg.compare_stat_cfg.max3_ratio;
	cmp_cfg_tab->compare_stat_cfg.bin_factor =
		cmp_req->reg_cfg.compare_stat_cfg.bin_factor;
	cmp_cfg_tab->compare_stat_cfg.bin_num =
		cmp_req->reg_cfg.compare_stat_cfg.bin_num;
	cmp_cfg_tab->compare_total_kptnum_cfg.to_use   = 1;
	cmp_cfg_tab->compare_total_kptnum_cfg.total_kptnum   =
		cmp_req->reg_cfg.compare_total_kptnum_cfg.total_kptnum;
	cmp_cfg_tab->compare_block_cfg.to_use  = 1;
	cmp_cfg_tab->compare_block_cfg.blk_v_num =
		cmp_req->reg_cfg.compare_block_cfg.blk_v_num;
	cmp_cfg_tab->compare_block_cfg.blk_h_num =
		cmp_req->reg_cfg.compare_block_cfg.blk_h_num;
	cmp_cfg_tab->compare_block_cfg.blk_num =
		cmp_req->reg_cfg.compare_block_cfg.blk_num;
	cmp_cfg_tab->compare_prefetch_cfg.to_use = 1;
	cmp_cfg_tab->compare_prefetch_cfg.prefetch_enable = 0;
	cmp_cfg_tab->compare_prefetch_cfg.first_4k_page =
		cmp_req->streams[BI_COMPARE_DESC_CUR].buffer >> 12;
	cmp_cfg_tab->compare_kptnum_cfg.to_use = 1;

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4; i++) {
		cmp_cfg_tab->compare_kptnum_cfg.compare_ref_kpt_num[i] =
			cmp_req->reg_cfg.compare_kptnum_cfg.compare_ref_kpt_num[i];
		cmp_cfg_tab->compare_kptnum_cfg.compare_cur_kpt_num[i] =
			cmp_req->reg_cfg.compare_kptnum_cfg.compare_cur_kpt_num[i];
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cmp_module_config(
	struct msg_req_compare_request_t *cmp_request,
	struct seg_compare_cfg_t *seg_compare_cfg)
{
	int ret = 0;
	unsigned int stripe_cnt = 1;
	ret |= set_ipp_cfg_cmp(cmp_request, &(seg_compare_cfg[stripe_cnt - 1].compare_cfg_tab));
	ret |= cmp_update_cvdr_cfg_tab(cmp_request, &(seg_compare_cfg[stripe_cnt - 1].compare_cvdr_cfg_tab));
	if (ret != 0) {
		loge(" Failed : cmp module config");
		return ISP_IPP_ERR;
	} else {
		return ISP_IPP_OK;
	}
}

static int seg_cmp_cfg_cmdlst_stripe_irq(struct cmdlst_stripe_info_t *cmdlst_stripe, unsigned int stripe_cnt)
{
	unsigned long long irq = 0;
	unsigned int i = 0;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i != 0) {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_IRQ_DONE);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_FP);
		}

		logi("cmp_stripe=%d, irq=0x%llx", i, irq);
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_CMP;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int seg_cmp_cfg_cmdlst_wr_buf(struct msg_req_compare_request_t *cmp_request,
									 struct schedule_cmdlst_link_t *cmd_link_entry, unsigned int i)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;

	if (cmp_request->compare_matched_kpt) {
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = cmp_request->compare_matched_kpt;
		logd("1, cmdlst_wr_cfg.buff_wr_addr = 0x%x", cmdlst_wr_cfg.buff_wr_addr);
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_MATCH_POINTS_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}

	if (cmp_request->compare_index) {
		cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = cmp_request->compare_index;
		logd("2, cmdlst_wr_cfg.buff_wr_addr = 0x%x", cmdlst_wr_cfg.buff_wr_addr);
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_INDEX_0_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}

	if (cmp_request->compare_matched_dist) {
		cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = cmp_request->compare_matched_dist;
		logd("3, cmdlst_wr_cfg.buff_wr_addr = 0x%x", cmdlst_wr_cfg.buff_wr_addr);
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_DISTANCE_0_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}

#if IPP_CMP_UT_TEST
	if (cmp_request->compare_index) { // only for UT
		cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = cmp_request->compare_index + (CMP_INDEX_RANGE_MAX * 4);
		logd("2', cmdlst_wr_cfg.buff_wr_addr = 0x%x", cmdlst_wr_cfg.buff_wr_addr);
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_INDEX_0_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}

	if (cmp_request->compare_matched_dist) { // only for UT
		cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = cmp_request->compare_matched_dist + (CMP_INDEX_RANGE_MAX * 4);
		logd("3', cmdlst_wr_cfg.buff_wr_addr = 0x%x", cmdlst_wr_cfg.buff_wr_addr);
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_DISTANCE_0_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}
#endif
	return i;
}


static int seg_ipp_cfg_cmp_cmdlst_para(
	struct msg_req_compare_request_t *cmp_request,
	struct seg_compare_cfg_t *seg_compare_cfg,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe = cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;
	stripe_cnt = (cmp_request->compare_matched_kpt == 0) ? (stripe_cnt) : (stripe_cnt + 1);
#if IPP_CMP_UT_TEST
	stripe_cnt = (cmp_request->compare_index == 0) ? (stripe_cnt) : (stripe_cnt + 2);
	stripe_cnt = (cmp_request->compare_matched_dist == 0) ? (stripe_cnt) : (stripe_cnt + 2);
#else
	stripe_cnt = (cmp_request->compare_index == 0) ? (stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (cmp_request->compare_matched_dist == 0) ? (stripe_cnt) : (stripe_cnt + 1);
#endif
	logd("cmp's stripe_cnt = %d", stripe_cnt);
	seg_cmp_cfg_cmdlst_stripe_irq(cmdlst_stripe, stripe_cnt);
	cmdlst_para->channel_id = IPP_CMP_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		loge_if_ret(cvdr_prepare_nr_cmd(&g_cvdr_devs[0], &cmd_link_entry[i].cmd_buf,
										&(seg_compare_cfg[0].compare_cvdr_cfg_tab)));
		loge_if_ret(compare_prepare_cmd(&g_compare_devs[0], &cmd_link_entry[i].cmd_buf,
										&(seg_compare_cfg[0].compare_cfg_tab)));
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i++].cmd_buf,
									 &(seg_compare_cfg[0].compare_cvdr_cfg_tab)));
		i = seg_cmp_cfg_cmdlst_wr_buf(cmp_request, cmd_link_entry, i);
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cmp_get_cpe_mem(struct seg_compare_cfg_t **seg_compare_cfg,
								   struct cmdlst_para_t **compare_cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1. cfg tbl
	ret = cpe_mem_get(MEM_ID_COMPARE_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge(" Failed : cpe_mem_get %d", MEM_ID_COMPARE_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_compare_cfg = (struct seg_compare_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_compare_cfg, sizeof(struct seg_compare_cfg_t), 0, sizeof(struct seg_compare_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_compare_cfg error!");
		goto compare_get_cpe_mem_failed;
	}

	// 2. cmdlst para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_COMPARE, &va, &da);
	if (ret != 0) {
		loge(" Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_COMPARE);
		goto compare_get_cpe_mem_failed;
	}

	*compare_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*compare_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s compare_cmdlst_para error!");
		goto compare_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
compare_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	return ISP_IPP_ERR;
}

int compare_request_handler(
	struct msg_req_compare_request_t *compare_request)
{
	struct seg_compare_cfg_t *seg_compare_cfg = NULL;
	struct cmdlst_para_t *compare_cmdlst_para = NULL;
	int ret = 0;
	dataflow_check_para(compare_request);
	logd("+");
	compare_request_dump(compare_request);
#if IPP_UT_DEBUG
	set_dump_register_init(UT_REG_ADDR, IPP_MAX_REG_OFFSET, 0);
#endif
	ret = seg_ipp_cmp_get_cpe_mem(&seg_compare_cfg, &compare_cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_cmp_get_cpe_mem");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_cmp_module_config(compare_request, seg_compare_cfg);
	if (ret != 0) {
		loge(" Failed : seg_ipp_cmp_module_config");
		goto SEG_COMPARE_BUFF_FREE;
	}

	ret = seg_ipp_cfg_cmp_cmdlst_para(compare_request, seg_compare_cfg, compare_cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_cfg_cmp_cmdlst_para");
		goto SEG_COMPARE_BUFF_FREE;
	}

	ret = df_sched_start(compare_cmdlst_para);
	if (ret != 0)
		loge(" Failed : df_sched_start");

	logd("-");
SEG_COMPARE_BUFF_FREE:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	return ret;
}

static void compare_request_dump_stream(struct msg_req_compare_request_t *req)
{
	unsigned int i = 0;

	for (i = 0; i < COMPARE_STREAM_MAX; i++) {
		logi("streams[%d].width = %d", i, req->streams[i].width);
		logi("streams[%d].height = %d", i, req->streams[i].height);
		logi("streams[%d].stride = %d", i, req->streams[i].stride);
		logi("streams[%d].buf = 0x%x", i, req->streams[i].buffer);
		logi("streams[%d].format = %d", i, req->streams[i].format);
	}

	return;
}

static void compare_request_dump(struct msg_req_compare_request_t *req)
{
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	unsigned int i = 0;
	struct compare_req_kptnum_cfg_t *kptnum_cfg = &req->reg_cfg.compare_kptnum_cfg;
	struct compare_req_stat_cfg_t *stat_cfg = &req->reg_cfg.compare_stat_cfg;
	struct compare_req_offset_cfg_t *offset_cfg = &req->reg_cfg.compare_offset_cfg;
	struct compare_req_search_cfg_t *search_cfg = &req->reg_cfg.compare_search_cfg;
	struct compare_req_block_cfg_t *block_cfg = &req->reg_cfg.compare_block_cfg;
	struct compare_req_ctrl_cfg_t *ctrl_cfg = &req->reg_cfg.compare_ctrl_cfg;
#endif

	logi("sizeof(struct msg_req_compare_request_t) = 0x%x",
		 (int)(sizeof(struct msg_req_compare_request_t)));
	logi("frame_number = %d", req->frame_number);
	compare_request_dump_stream(req);
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logi("CMP CFG DUMP:");
	logi("compare_ctrl_cfg.compare_en = %d", ctrl_cfg->compare_en);
	logi("compare_block_cfg.blk_v_num = %d", block_cfg->blk_v_num);
	logi("compare_block_cfg.blk_h_num = %d", block_cfg->blk_h_num);
	logi("compare_block_cfg.blk_num = %d", block_cfg->blk_num);
	logi("compare_search_cfg.v_radius = %d", search_cfg->v_radius);
	logi("compare_search_cfg.h_radius = %d", search_cfg->h_radius);
	logi("compare_search_cfg.dis_ratio = %d", search_cfg->dis_ratio);
	logi("compare_search_cfg.dis_threshold = %d", search_cfg->dis_threshold);
	logi("compare_offset_cfg.cenh_offset = %d", offset_cfg->cenh_offset);
	logi("compare_offset_cfg.cenv_offset = %d", offset_cfg->cenv_offset);
	logi("compare_stat_cfg.stat_en = %d", stat_cfg->stat_en);
	logi("compare_stat_cfg.max3_ratio = %d", stat_cfg->max3_ratio);
	logi("compare_stat_cfg.bin_factor = %d", stat_cfg->bin_factor);
	logi("compare_stat_cfg.bin_num = %d", stat_cfg->bin_num);
	logi("compare_total_kptnum_cfg.total_kptnum = %d",
		 req->reg_cfg.compare_total_kptnum_cfg.total_kptnum);
	logi("compare_index = 0x%x", req->compare_index);
	logi("compare_matched_kpt = 0x%x", req->compare_matched_kpt);
	logi("compare_matched_dist = 0x%x", req->compare_matched_dist);
	logi("compare_ref_kpt_num =");

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4 / 4 - 1; i++)
		logi("0x%08x  0x%08x  0x%08x  0x%08x",
			 kptnum_cfg->compare_ref_kpt_num[0 + 4 * i], kptnum_cfg->compare_ref_kpt_num[1 + 4 * i],
			 kptnum_cfg->compare_ref_kpt_num[2 + 4 * i], kptnum_cfg->compare_ref_kpt_num[3 + 4 * i]);

	logi("compare_cur_kpt_num = ");

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4 / 4 - 1; i++)
		logi("0x%08x  0x%08x  0x%08x  0x%08x",
			 kptnum_cfg->compare_cur_kpt_num[0 + 4 * i], kptnum_cfg->compare_cur_kpt_num[1 + 4 * i],
			 kptnum_cfg->compare_cur_kpt_num[2 + 4 * i], kptnum_cfg->compare_cur_kpt_num[3 + 4 * i]);

#endif
}

