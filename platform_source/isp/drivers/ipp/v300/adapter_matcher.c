#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <securec.h>
#include <linux/delay.h>
#include "memory.h"
#include "matcher.h"
#include "cfg_table_matcher.h"
#include "cvdr_drv.h"
#include "cvdr_opt.h"
#include "reorder_drv.h"
#include "compare_drv.h"
#include "compare_reg_offset.h"
#include "ipp_top_reg_offset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#define LOG_TAG LOG_MODULE_MATCHER
#define CVDR_KEY_POINT_OVERFLOW  0x8000

static int matcher_request_dump(
	struct msg_req_matcher_t *req);

static int matcher_reorder_update_cvdr_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_cvdr_t *reorder_cfg_tab, unsigned int layer_num)
{
	if (matcher_request == NULL || reorder_cfg_tab == NULL) {
		E("pointer is NULL!!\n");
		return ISP_IPP_ERR;
	}

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
			(ALIGN_UP(80 * 8192, CVDR_ALIGN_BYTES)), stride);
	}

	if (req_rdr->streams[layer_num][BO_RDR_FP_BLOCK].buffer)
		cfg_tbl_cvdr_nr_wr_cfg(reorder_cfg_tab, IPP_NR_WR_RDR);

	return 0;
}

static void matcher_reorder_update_request_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_reorder_t *reorder_cfg_tab,
	unsigned int  layer_num,
	unsigned int  rdr_update_flag)
{
	if (matcher_request == NULL || reorder_cfg_tab == NULL) {
		E("pointer is NULL!!\n");
		return;
	}

	struct req_rdr_t *req_rdr = &(matcher_request->req_rdr);

	reorder_cfg_tab->reorder_ctrl_cfg.to_use = 1;
	reorder_cfg_tab->reorder_ctrl_cfg.reorder_en =
		req_rdr->ctrl_cfg[layer_num].reorder_en;
	reorder_cfg_tab->reorder_ctrl_cfg.descriptor_type =
		req_rdr->ctrl_cfg[layer_num].descriptor_type;
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
	reorder_cfg_tab->reorder_prefetch_cfg.first_32k_page =
		req_rdr->streams[layer_num][BO_RDR_FP_BLOCK].buffer >> 15;
	reorder_cfg_tab->reorder_kptnum_cfg.to_use = 1;

	reorder_cfg_tab->rdr_cascade_en = (rdr_update_flag == 0) ?
		CASCADE_DISABLE : CASCADE_ENABLE;
		
	loge_if(memcpy_s(
		&(reorder_cfg_tab->reorder_kptnum_cfg.reorder_kpt_num[0]),
		MATCHER_KPT_NUM * sizeof(unsigned int),
		&(req_rdr->reorder_kpt_num[layer_num][0]),
		MATCHER_KPT_NUM * sizeof(unsigned int)));
}

static int seg_src_cfg_rdr_cmdlst(
	struct msg_req_matcher_t *matcher_request,
	struct cmdlst_para_t *rdr_cmdlst_para,
	struct orb_schedule_flag_t * flag)
{
	if (matcher_request == NULL || rdr_cmdlst_para == NULL ||
		flag == NULL) {
		E("pointer is NULL!!\n");
		return ISP_IPP_ERR;
	}

	unsigned int i = 0;
	unsigned int stripe_cnt = matcher_request->rdr_pyramid_layer;
	unsigned int rdr_layer_stripe_num = 1;
	unsigned long long irq = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		rdr_cmdlst_para->cmd_stripe_info;
	stripe_cnt = stripe_cnt * rdr_layer_stripe_num;
	rdr_cmdlst_para->stripe_cnt = stripe_cnt;

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

		if ((matcher_request->work_mode & (1 << 1)) == 0) {
			cmdlst_stripe[i].en_link		 = 1;

			if (i > 0) {
				cmdlst_stripe[i].ch_link =
					IPP_CMP_CHANNEL_ID;
				cmdlst_stripe[i].ch_link_act_nbr = (flag->mc_en_flag == 0) ?
					STRIPE_NUM_EACH_CMP : STRIPE_NUM_EACH_CMP + 2;
			}
		} else {
			cmdlst_stripe[i].en_link = 1;
			cmdlst_stripe[i].ch_link = IPP_CMP_CHANNEL_ID;
			cmdlst_stripe[i].ch_link_act_nbr = (flag->mc_en_flag == 0) ?
				matcher_request->cmp_pyramid_layer *
					STRIPE_NUM_EACH_CMP :
					STRIPE_NUM_EACH_CMP + 2;		
		}
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int matcher_compare_update_cvdr_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_cvdr_t *compare_cfg_tab, unsigned int layer_num)
{
	if (matcher_request == NULL || compare_cfg_tab == NULL) {
		E("pointer is NULL!!\n");
		return ISP_IPP_ERR;
	}

	struct cvdr_opt_fmt_t cfmt;
	unsigned int stride = 0;
	enum pix_format_e format;
	struct req_cmp_t *req_cmp = &(matcher_request->req_cmp);

	if (req_cmp->streams[layer_num][BI_CMP_REF_FP].buffer) {
		cfmt.id = IPP_VP_RD_CMP;
		cfmt.width =
			req_cmp->streams[layer_num][BI_CMP_REF_FP].width;
		cfmt.full_width =
			req_cmp->streams[layer_num][BI_CMP_REF_FP].width;
		cfmt.line_size =
			req_cmp->streams[layer_num][BI_CMP_REF_FP].width;
		cfmt.height =
			req_cmp->streams[layer_num][BI_CMP_REF_FP].height;
		cfmt.expand = 1;
		format = PIXEL_FMT_IPP_D64;
		stride   = cfmt.width / 2;
		cfmt.addr = req_cmp->streams[layer_num][BI_CMP_REF_FP].buffer;
		cfg_tbl_cvdr_rd_cfg_d64(compare_cfg_tab, &cfmt,
			(ALIGN_UP(CMP_IN_INDEX_NUM,
				CVDR_ALIGN_BYTES)), stride);
	}

	if (req_cmp->streams[layer_num][BI_CMP_CUR_FP].buffer)
		cfg_tbl_cvdr_nr_rd_cfg(compare_cfg_tab, IPP_NR_RD_CMP);

	return 0;
}

static void matcher_compare_update_request_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_compare_t *compare_cfg_tab,
	unsigned int layer_num,
	unsigned int cmp_update_flag)
{
	if (matcher_request == NULL || compare_cfg_tab == NULL) {
		E("pointer is NULL!!\n");
		return;
	}

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
	compare_cfg_tab->compare_ctrl_cfg.compare_en =
		req_cmp->ctrl_cfg[layer_num].compare_en;
	compare_cfg_tab->compare_ctrl_cfg.descriptor_type =
		req_cmp->ctrl_cfg[layer_num].descriptor_type;
	compare_cfg_tab->compare_total_kptnum_cfg.total_kptnum =
		req_cmp->total_kptnum_cfg[layer_num].total_kptnum;
	compare_cfg_tab->compare_block_cfg.blk_v_num =
		req_cmp->block_cfg[layer_num].blk_v_num;
	compare_cfg_tab->compare_block_cfg.blk_h_num =
		req_cmp->block_cfg[layer_num].blk_h_num;
	compare_cfg_tab->compare_block_cfg.blk_num =
		req_cmp->block_cfg[layer_num].blk_num;
	compare_cfg_tab->compare_search_cfg.v_radius =
		req_cmp->search_cfg[layer_num].v_radius;
	compare_cfg_tab->compare_search_cfg.h_radius =
		req_cmp->search_cfg[layer_num].h_radius;
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
	compare_cfg_tab->compare_prefetch_cfg.first_32k_page =
		req_cmp->streams[layer_num][BI_CMP_CUR_FP].buffer >> 15;

	compare_cfg_tab->cmp_cascade_en = (cmp_update_flag == 0) ?
		CASCADE_DISABLE : CASCADE_ENABLE;

	for (i = 0; i < MATCHER_KPT_NUM; i++) {
		compare_cfg_tab->compare_kptnum_cfg.compare_ref_kpt_num[i] =
			req_cmp->kptnum_cfg[layer_num].compare_ref_kpt_num[i];
		compare_cfg_tab->compare_kptnum_cfg.compare_cur_kpt_num[i] =
			req_cmp->kptnum_cfg[layer_num].compare_cur_kpt_num[i];
	}
}

static int seg_src_cfg_cmp_cmdlst(
	struct msg_req_matcher_t *matcher_request,
	struct cmdlst_para_t *cmp_cmdlst_para,
	struct orb_schedule_flag_t * flag)
{
	if (matcher_request == NULL || cmp_cmdlst_para == NULL
		|| flag == NULL) {
		E("pointer is NULL!!\n");
		return ISP_IPP_ERR;
	}

	unsigned int i = 0;
	unsigned int mc_token_num =
		flag->mc_token_num;
	
	if (matcher_request->cmp_pyramid_layer == 0) {
		D("@@@@ matcher_request->cmp_pyramid_layer = %d\n",
			matcher_request->cmp_pyramid_layer);
		return ISP_IPP_OK;
	}

	unsigned int stripe_cnt =
		matcher_request->cmp_pyramid_layer * STRIPE_NUM_EACH_CMP;
	stripe_cnt = (flag->mc_en_flag == 0) ?
		stripe_cnt : (stripe_cnt + 2);
	unsigned long long irq = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmp_cmdlst_para->cmd_stripe_info;
	cmp_cmdlst_para->stripe_cnt = stripe_cnt;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i % STRIPE_NUM_EACH_CMP != 0 ||
			(i!= 0 && flag->mc_en_flag == 1)) {
			cmdlst_stripe[i].hw_priority  = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) <<
				IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) <<
				IPP_CMP_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) <<
				IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) <<
				IPP_CMP_IRQ_DONE);
			irq = irq | (((unsigned long long)(1)) <<
				IPP_CMP_CVDR_VP_RD_EOF_FP);
		}

		cmdlst_stripe[i].resource_share  = 1 <<
			IPP_CMD_RES_SHARE_CMP;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		
		cmdlst_stripe[i].ch_link = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].ch_link = (flag->mc_en_flag == 0) ?
			0 : IPP_MC_CHANNEL_ID;
	cmdlst_stripe[stripe_cnt - 1].ch_link_act_nbr = (flag->mc_en_flag == 0) ?
			0: mc_token_num;
	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int seg_src_set_cmp_cmdlst_para(
	struct cmdlst_para_t *cmdlst_para,
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_compare_t *compare_cfg_tab,
	struct cfg_tab_cvdr_t *compare_cvdr_cfg_tab,
	struct orb_schedule_flag_t * flag)
{
	if (cmdlst_para == NULL || matcher_request == NULL ||
	compare_cfg_tab == NULL || compare_cvdr_cfg_tab == NULL) {
		E("pointer is NULL!!\n");
		return ISP_IPP_ERR;
	}

	unsigned int i = 0;
	unsigned int cur_layer = 0;
	unsigned int cmp_update_flag =  flag->matcher_update_flag;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;

	memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t),
		0, sizeof(struct cmdlst_wr_cfg_t));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	unsigned int cmp_layer_stripe_num = STRIPE_NUM_EACH_CMP;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		cur_layer = i / cmp_layer_stripe_num;
		cvdr_prepare_nr_cmd(
			&g_cvdr_devs[0],
			&cmd_link_entry[i].cmd_buf,
			&compare_cvdr_cfg_tab[cur_layer]);
		compare_prepare_cmd(
			&g_compare_devs[0],
			&cmd_link_entry[i].cmd_buf,
			&compare_cfg_tab[cur_layer]);
		cvdr_prepare_cmd(
			&g_cvdr_devs[0],
			&cmd_link_entry[i++].cmd_buf,
			&compare_cvdr_cfg_tab[cur_layer]);
		if(cmp_update_flag == 1){
			flag->cmp_kpt_num_addr_cur =
				compare_cfg_tab[cur_layer].cmp_cur_kpt_addr;
			flag->cmp_kpt_num_addr_ref =
				compare_cfg_tab[cur_layer].cmp_ref_kpt_addr;
			flag->cmp_total_num_addr =
				compare_cfg_tab[cur_layer].cmp_total_kpt_addr;
			flag->cmp_frame_height_addr =
				compare_cvdr_cfg_tab[cur_layer].cmp_cvdr_frame_size_addr;
		}

		if (STRIPE_NUM_EACH_CMP > 1) {
			if (matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_OUT].buffer) {
				cmdlst_wr_cfg.data_size =
					CPMPARE_OUT_REG_NUM;
				cmdlst_wr_cfg.is_wstrb = 0;
				cmdlst_wr_cfg.reg_rd_addr =
					IPP_BASE_ADDR_COMPARE
					+ COMPARE_INDEX_0_REG;
				if(flag->mc_en_flag == 0) {
					cmdlst_wr_cfg.buff_wr_addr =
					matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_OUT].buffer;
					loge_if_ret(seg_ipp_set_cmdlst_wr_buf_cmp(
					&cmd_link_entry[i++].cmd_buf,
					&cmdlst_wr_cfg, 0x10));
				} else if (flag->mc_en_flag == 1) {
					cmdlst_wr_cfg.read_mode = 0;
					cmdlst_wr_cfg.buff_wr_addr =
						matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_OUT].buffer;
					loge_if_ret(seg_ipp_set_cmdlst_wr_buf_cmp(
					&cmd_link_entry[i++].cmd_buf,
					&cmdlst_wr_cfg,0x10));
			
					cmdlst_wr_cfg.data_size = 1;
				        cmdlst_wr_cfg.reg_rd_addr =
					IPP_BASE_ADDR_COMPARE
					+ COMPARE_MATCH_POINTS_REG;
					cmdlst_wr_cfg.is_wstrb = 1;
					cmdlst_wr_cfg.read_mode = 0;
					cmdlst_wr_cfg.buff_wr_addr =
						flag->mc_matched_num_addr;
					loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
					&cmd_link_entry[i++].cmd_buf,
					&cmdlst_wr_cfg));

					
					cmdlst_wr_cfg.data_size =
					MC_OUT_REG_NUM;
				        cmdlst_wr_cfg.is_wstrb = 1;
				        cmdlst_wr_cfg.reg_rd_addr =
					IPP_BASE_ADDR_COMPARE
					+ COMPARE_INDEX_0_REG;
					cmdlst_wr_cfg.read_mode = 1;
					cmdlst_wr_cfg.buff_wr_addr =
						flag->mc_index_addr;
					loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
					&cmd_link_entry[i++].cmd_buf,
					&cmdlst_wr_cfg));
				}
			}
		}
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
	struct msg_req_matcher_t *matcher_req) {

	unsigned int i = 0;
	if(matcher_req->cmp_pyramid_layer > 8
	|| matcher_req->rdr_pyramid_layer > 8 ||
	(matcher_req->cmp_pyramid_layer == 0 &&
	matcher_req->rdr_pyramid_layer == 0)) {
		E(" failed : matcher_pyramid_layer  is incorrect");
		return ISP_IPP_ERR;
	}

	for(i = 0; i < matcher_req->rdr_pyramid_layer;i++) {
		if(matcher_req->req_rdr.streams[i][BI_RDR_FP].buffer == 0
		|| matcher_req->req_rdr.streams[i][BO_RDR_FP_BLOCK].buffer == 0) {
			E(" failed : rdr_addr  is incorrect");
			return ISP_IPP_ERR;
		}
	}

	for(i = 0; i < matcher_req->cmp_pyramid_layer;i++) {
		if(matcher_req->req_cmp.streams[i][BI_CMP_REF_FP].buffer == 0
		|| matcher_req->req_cmp.streams[i][BI_CMP_CUR_FP].buffer == 0) {
			E(" failed : cmp_addr  is incorrect");
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

int rdr_request_handler(
	struct msg_req_matcher_t *matcher_request,
	struct orb_schedule_flag_t * flag) {

	int ret = 0;
	unsigned int i = 0;
	unsigned long long va = 0;
	unsigned int da = 0;
	unsigned int rdr_update_flag =
		flag->matcher_update_flag;
	if (matcher_request->rdr_pyramid_layer == 0)
		return ISP_IPP_OK;
	
	ret = cpe_mem_get(MEM_ID_REORDER_CFG_TAB, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_REORDER_CFG_TAB);
		return ISP_IPP_ERR;
	}

	struct cfg_tab_reorder_t *reorder_cfg_tab =
		(struct cfg_tab_reorder_t *)(uintptr_t)va;
	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_REORDER, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CVDR_CFG_TAB_REORDER);
		goto fail_rdr_cfg_tab;
	}

	struct cfg_tab_cvdr_t *reorder_cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_REORDER, &va, &da);
	if (ret != 0) {
		E("Failed : cpe_mem_get %d\n", MEM_ID_CMDLST_PARA_REORDER);
		goto fail_rdr_cvdr_tab;
	}

	struct cmdlst_para_t *reorder_cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;
	reorder_cmdlst_para->channel_id = IPP_RDR_CHANNEL_ID;
	reorder_cmdlst_para->stripe_cnt =
		matcher_request->rdr_pyramid_layer;
	ret = seg_src_cfg_rdr_cmdlst(matcher_request,
		reorder_cmdlst_para, flag);
	if (ret != 0) {
		E("Failed : seg_src_cfg_rdr_cmdlst \n");
		goto fail_rdr_cvdr_tab;
	}

	for (i = 0; i < reorder_cmdlst_para->stripe_cnt; i++) {
		matcher_reorder_update_request_cfg_tab(
			matcher_request, &reorder_cfg_tab[i],
			i,rdr_update_flag);
		ret = matcher_reorder_update_cvdr_cfg_tab(
			matcher_request, &reorder_cvdr_cfg_tab[i], i);
		if (ret != 0) {
			E("Failed : matcher_reorder_update_cvdr_cfg_tab \n");
			goto fail_rdr_cvdr_tab;
		}
	}

	ret = df_sched_prepare(reorder_cmdlst_para);
	if (ret != 0) {
		E("Failed : df_sched_prepare \n");
		goto fail_rdr_cvdr_tab;
	}

	struct schedule_cmdlst_link_t *rdr_cmd_link_entry =
	(struct schedule_cmdlst_link_t *)reorder_cmdlst_para->cmd_entry;

	for (i = 0; i < reorder_cmdlst_para->stripe_cnt; i++) {
		ret = cvdr_prepare_nr_cmd(
			&g_cvdr_devs[0],
			&rdr_cmd_link_entry[i].cmd_buf,
			&reorder_cvdr_cfg_tab[i]);
		if (ret != 0) {
			E("Failed : cvdr_prepare_nr_cmd\n");
			goto fail_rdr_cvdr_tab;
		}

		ret = reorder_prepare_cmd(
			&g_reorder_devs[0],
			&rdr_cmd_link_entry[i].cmd_buf,
			&reorder_cfg_tab[i]);
		if (ret != 0) {
			E("Failed : reorder_prepare_cmd\n");
			goto fail_rdr_cvdr_tab;
		}

		ret = cvdr_prepare_cmd(
			&g_cvdr_devs[0],
			&rdr_cmd_link_entry[i].cmd_buf,
			&reorder_cvdr_cfg_tab[i]);
		if (ret != 0) {
			E("Failed : cvdr_prepare_cmd\n");
			goto fail_rdr_cvdr_tab;
		}

		if(rdr_update_flag == 1 && i == 0){
			flag->rdr_kpt_num_addr_cur =
				reorder_cfg_tab[i].rdr_kpt_num_addr;
			flag->rdr_total_num_addr_cur =
				reorder_cfg_tab[i].rdr_total_num_addr; 
			flag->rdr_frame_height_addr_cur =
				reorder_cvdr_cfg_tab[i].rdr_cvdr_frame_size_addr; 
		}else if (rdr_update_flag == 1 && i == 1){
			flag->rdr_kpt_num_addr_ref =
					reorder_cfg_tab[i].rdr_kpt_num_addr;
			flag->rdr_total_num_addr_ref =
					reorder_cfg_tab[i].rdr_total_num_addr;
			flag->rdr_frame_height_addr_ref =
					reorder_cvdr_cfg_tab[i].rdr_cvdr_frame_size_addr; 
		}
	}

	ret = df_sched_start(reorder_cmdlst_para);
fail_rdr_cvdr_tab:
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_REORDER);
fail_rdr_cfg_tab:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	D("-\n");
	return ret;
}

int cmp_request_handler(
	struct msg_req_matcher_t *matcher_request,
		struct orb_schedule_flag_t * flag) {

	int ret = 0;
	unsigned int i = 0;
	unsigned long long va = 0;
	unsigned int da = 0;
	unsigned int cmp_update_flag =
		flag->matcher_update_flag;
	struct cmdlst_para_t *compare_cmdlst_para = NULL;

	if (matcher_request->cmp_pyramid_layer == 0) 
		return ISP_IPP_OK;

	ret = cpe_mem_get(MEM_ID_COMPARE_CFG_TAB, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_COMPARE_CFG_TAB);
		return ISP_IPP_ERR;
	}

	struct cfg_tab_compare_t *compare_cfg_tab =
		(struct cfg_tab_compare_t *)(uintptr_t)va;
	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_COMPARE, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CVDR_CFG_TAB_COMPARE);
		goto  fail_cmp_cfg_tab;
	}

	struct cfg_tab_cvdr_t *compare_cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_COMPARE, &va, &da);
	if (ret != 0) {
		E("Failed : cpe_mem_get %d\n", MEM_ID_CMDLST_PARA_COMPARE);
		goto fail_cmp_cvdr_tab;
	}

	compare_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	compare_cmdlst_para->channel_id = IPP_CMP_CHANNEL_ID;

	for (i = 0; i < matcher_request->cmp_pyramid_layer; i++) {
		matcher_compare_update_request_cfg_tab(
			matcher_request, &compare_cfg_tab[i],
			i, cmp_update_flag);
		ret = matcher_compare_update_cvdr_cfg_tab(
			matcher_request, &compare_cvdr_cfg_tab[i], i);
		if (ret != 0) {
			E("Failed : cmp_update_cvdr_cfg_tab\n");
			goto fail_cmp_cvdr_tab;
		}
	}

	ret = seg_src_cfg_cmp_cmdlst(matcher_request,
		compare_cmdlst_para, flag);
	if (ret != 0) {
		E("Failed : seg_src_cfg_cmp_cmdlst\n");
		goto fail_cmp_cvdr_tab;
	}

	ret = df_sched_prepare(compare_cmdlst_para);
	if (ret != 0) {
		E("Failed : df_sched_prepare\n");
		goto fail_cmp_cvdr_tab;
	}

	ret = seg_src_set_cmp_cmdlst_para(compare_cmdlst_para,
	matcher_request, compare_cfg_tab, compare_cvdr_cfg_tab, flag);
	if (ret != 0) {
		E("Failed : seg_src_set_cmp_cmdlst_para\n");
		goto fail_cmp_cvdr_tab;
	}

	ret = df_sched_start(compare_cmdlst_para);

fail_cmp_cvdr_tab:
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_COMPARE);
fail_cmp_cfg_tab:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	D("-\n");

	return ret;
}

int matcher_request_handler(
	struct msg_req_matcher_t *matcher_request,
	struct orb_schedule_flag_t * flag)
{
	int ret = 0;

	D("+\n");

	if (matcher_request == NULL ||
		flag == NULL) {
		E("failed : matcher_request is null");
		return ISP_IPP_ERR;
	}

	ret = orb_matcher_check_para(matcher_request);
	if (ret != 0) {
		E(" Failed : orb_matcher_check_para");
		return ISP_IPP_ERR;
	}

	matcher_request_dump(matcher_request);
	unsigned int rdr_token_nbr_en = flag->matcher_en_flag;
	unsigned int cmp_token_nbr_en =
		(matcher_request->rdr_pyramid_layer == 0) ? (0) : (1);
	unsigned int nrt_channel  = 1;

	cmdlst_channel_cfg_init(IPP_RDR_CHANNEL_ID,
		rdr_token_nbr_en, nrt_channel);
	cmdlst_channel_cfg_init(IPP_CMP_CHANNEL_ID,
		cmp_token_nbr_en, nrt_channel);
	
	loge_if_ret(cmp_request_handler(matcher_request,flag));
	loge_if_ret(rdr_request_handler(matcher_request,flag));

	D("-\n");
	return ISP_IPP_OK;
}

int matcher_eof_handler(
	struct msg_req_matcher_t *matcher_request)
{
	int ret;
	int res = ISP_IPP_OK;

	if (matcher_request == NULL) {
		E("matcher_request is NULL!!\n");
		return ISP_IPP_ERR;
	}

	if (matcher_request->rdr_pyramid_layer != 0) {
		ret = ipp_eop_handler(CMD_EOF_RDR_MODE);
		if (ret != 0) {
			res = ISP_IPP_ERR;
			E("failed : ipp_eop_handler_rdr");
		}
	}

	if (matcher_request->cmp_pyramid_layer != 0) {
		ret = ipp_eop_handler(CMD_EOF_CMP_MODE);
		if (ret != 0) {
			res = ISP_IPP_ERR;
			E("failed : ipp_eop_handler_cmp");
		}
	}

	return res;
}

static int matcher_request_dump(
	struct msg_req_matcher_t *req)
{
#if FLAG_LOG_DEBUG
	unsigned int i = 0;
	unsigned int j = 0;

	if (req == NULL) {
		E("req is NULL!!\n");
		return ISP_IPP_ERR;
	}

	D("size_of_matcher_req = %d\n",
		(int)(sizeof(struct msg_req_matcher_t)));
	D("req->frame_number      = %d\n", req->frame_number);
	I("req->rdr_pyramid_layer = %d\n", req->rdr_pyramid_layer);
	D("req->cmp_pyramid_layer = %d\n", req->cmp_pyramid_layer);
	D("req->work_mode         = %d\n", req->work_mode);

	for (i = 0; i < req->rdr_pyramid_layer; i++) {
		for (j = 0; j < RDR_STREAM_MAX; j++) {
			D(" req->req_rdr.streams[%d][%d].width  = %d\n",
				i, j, req->req_rdr.streams[i][j].width);
			D(" req->req_rdr.streams[%d][%d].height = %d\n",
				i, j, req->req_rdr.streams[i][j].height);
			D(" req->req_rdr.streams[%d][%d].stride = %d\n",
				i, j, req->req_rdr.streams[i][j].stride);
			D(" req->req_rdr.streams[%d][%d].buffer    = 0x%08x\n",
				i, j, req->req_rdr.streams[i][j].buffer);
			D(" req->req_rdr.streams[%d][%d].format = %d\n",
				i, j, req->req_rdr.streams[i][j].format);
		}

		D(" req->req_rdr.ctrl_cfg[%d].reorder_en      = %d\n",
			i, req->req_rdr.ctrl_cfg[i].reorder_en);
		D(" req->req_rdr.ctrl_cfg[%d].descriptor_type = %d\n",
			i, req->req_rdr.ctrl_cfg[i].descriptor_type);
		D(" req->req_rdr.ctrl_cfg[%d].total_kpt       = %d\n",
			i, req->req_rdr.ctrl_cfg[i].total_kpt);
		D(" req->req_rdr.block_cfg[%d].blk_v_num = %d\n",
			i, req->req_rdr.block_cfg[i].blk_v_num);
		D(" req->req_rdr.block_cfg[%d].blk_h_num = %d\n",
			i, req->req_rdr.block_cfg[i].blk_h_num);
		D(" req->req_rdr.block_cfg[%d].blk_num   = %d\n",
			i, req->req_rdr.block_cfg[i].blk_num);

		for (j = 0; j < MATCHER_KPT_NUM; j++)
			D(" req->req_rdr.reorder_kpt_num[%d][%d] = 0x%08x\n",
				i, j, req->req_rdr.reorder_kpt_num[i][j]);
	}

	for (i = 0; i < req->cmp_pyramid_layer; i++) {
		for (j = 0; j < CMP_STREAM_MAX; j++) {
			D(" req->req_cmp.streams[%d][%d].width  = %d\n",
				i, j, req->req_cmp.streams[i][j].width);
			D(" req->req_cmp.streams[%d][%d].height = %d\n",
				i, j, req->req_cmp.streams[i][j].height);
			D(" req->req_cmp.streams[%d][%d].stride = %d\n",
				i, j, req->req_cmp.streams[i][j].stride);
			D(" req->req_cmp.streams[%d][%d].buffer    = 0x%08x\n",
				i, j, req->req_cmp.streams[i][j].buffer);
			D(" req->req_cmp.streams[%d][%d].format = %d\n",
				i, j, req->req_cmp.streams[i][j].format);
		}

		D(" req->req_cmp.ctrl_cfg[%d].compare_en      = %d\n",
			i, req->req_cmp.ctrl_cfg[i].compare_en);
		D(" req->req_cmp.ctrl_cfg[%d].descriptor_type = %d\n",
			i, req->req_cmp.ctrl_cfg[i].descriptor_type);
		D(" req->req_cmp.block_cfg[%d].blk_v_num    = %d\n",
			i, req->req_cmp.block_cfg[i].blk_v_num);
		D(" req->req_cmp.block_cfg[%d].blk_h_num    = %d\n",
			i, req->req_cmp.block_cfg[i].blk_h_num);
		D(" req->req_cmp.block_cfg[%d].blk_num      = %d\n",
			i, req->req_cmp.block_cfg[i].blk_num);
		D(" req->req_cmp.search_cfg[%d].v_radius      = %d\n",
			i, req->req_cmp.search_cfg[i].v_radius);
		D(" req->req_cmp.search_cfg[%d].h_radius      = %d\n",
			i, req->req_cmp.search_cfg[i].h_radius);
		D(" req->req_cmp.search_cfg[%d].dis_ratio     = %d\n",
			i, req->req_cmp.search_cfg[i].dis_ratio);
		D(" req->req_cmp.search_cfg[%d].dis_threshold = %d\n",
			i, req->req_cmp.search_cfg[i].dis_threshold);
		D(" req->req_cmp.stat_cfg[%d].stat_en      = %d\n",
			i, req->req_cmp.stat_cfg[i].stat_en);
		D(" req->req_cmp.stat_cfg[%d].max3_ratio   = %d\n",
			i, req->req_cmp.stat_cfg[i].max3_ratio);
		D(" req->req_cmp.stat_cfg[%d].bin_factor   = %d\n",
			i, req->req_cmp.stat_cfg[i].bin_factor);
		D(" req->req_cmp.stat_cfg[%d].bin_num      = %d\n",
			i, req->req_cmp.stat_cfg[i].bin_num);

	}

	D(" req->matcher_rate_value      = %d\n",
		req->matcher_rate_value);
#else
	I("req->rdr_pyramid_layer = %d\n",
	req->rdr_pyramid_layer);
#endif
	return 0;
}

