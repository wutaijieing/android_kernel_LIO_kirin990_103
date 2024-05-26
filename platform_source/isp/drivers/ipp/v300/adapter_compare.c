#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "compare.h"
#include "cfg_table_compare.h"
#include "cvdr_drv.h"
#include "compare_drv.h"
#include "config_table_cvdr.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "compare_drv_priv.h"
#include "compare_reg_offset.h"
#include "ipp_top_reg_offset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#define LOG_TAG LOG_MODULE_COMPARE
#define CVDR_KEY_POINT_OVERFLOW  0x8000

unsigned int g_ipp_compare_width[2] = {
	[IPP_FREAK_MODE] = 12,
	[IPP_BRIEF_MODE] = 8,
};

static int compare_request_dump(
	struct msg_req_compare_request_t *req);

static int cmp_update_cvdr_cfg_tab(
	struct msg_req_compare_request_t *compare_request,
	struct cfg_tab_cvdr_t *compare_cfg_tab)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int type;
	unsigned int height;
	unsigned int addr;

	height = compare_request->streams[BI_COMPARE_DESC_REF].height;
	type = compare_request->reg_cfg.compare_ctrl_cfg.descriptor_type;
	addr = compare_request->streams[BI_COMPARE_DESC_REF].buffer;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t),
		0, sizeof(struct cvdr_opt_fmt_t)));
	unsigned int stride = 0;
	enum pix_format_e format;

	if (compare_request->streams[BI_COMPARE_DESC_REF].buffer) {
		cfmt.id = IPP_VP_RD_CMP;
		cfmt.width = g_ipp_compare_width[type];
		cfmt.line_size = cfmt.width;
		cfmt.height = height;
		cfmt.expand = 1;
		format = PIXEL_FMT_IPP_D64;
		stride   = cfmt.width / 2;
		cfmt.addr = addr;
		loge_if_ret(cfg_tbl_cvdr_rd_cfg_d64(compare_cfg_tab, &cfmt,
		(ALIGN_UP(CMP_IN_INDEX_NUM, CVDR_ALIGN_BYTES)), stride));
	}

	if (compare_request->streams[BI_COMPARE_DESC_CUR].buffer)
		loge_if_ret(cfg_tbl_cvdr_nr_rd_cfg(
		compare_cfg_tab, IPP_NR_RD_CMP));

	return 0;
}

static int set_ipp_cfg_cmp(
	struct msg_req_compare_request_t *cmp_req,
	struct cfg_tab_compare_t *cmp_cfg_tab)
{
	unsigned int i = 0;

	cmp_cfg_tab->compare_ctrl_cfg.to_use   = 1;
	cmp_cfg_tab->compare_ctrl_cfg.compare_en =
		cmp_req->reg_cfg.compare_ctrl_cfg.compare_en;
	cmp_cfg_tab->compare_ctrl_cfg.descriptor_type =
		cmp_req->reg_cfg.compare_ctrl_cfg.descriptor_type;
	cmp_cfg_tab->compare_search_cfg.to_use = 1;
	cmp_cfg_tab->compare_search_cfg.v_radius =
		cmp_req->reg_cfg.compare_search_cfg.v_radius;
	cmp_cfg_tab->compare_search_cfg.h_radius =
		cmp_req->reg_cfg.compare_search_cfg.h_radius;
	cmp_cfg_tab->compare_search_cfg.dis_ratio =
		cmp_req->reg_cfg.compare_search_cfg.dis_ratio;
	cmp_cfg_tab->compare_search_cfg.dis_threshold =
		cmp_req->reg_cfg.compare_search_cfg.dis_threshold;
	cmp_cfg_tab->compare_offset_cfg.to_use = 1;
	cmp_cfg_tab->compare_offset_cfg.cenh_offset =
		cmp_req->reg_cfg.compare_offset_cfg.cenh_offset;
	cmp_cfg_tab->compare_offset_cfg.cenv_offset =
		cmp_req->reg_cfg.compare_offset_cfg.cenv_offset;
	cmp_cfg_tab->compare_stat_cfg.to_use   = 1;
	cmp_cfg_tab->compare_stat_cfg.stat_en =
		cmp_req->reg_cfg.compare_stat_cfg.stat_en;
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
	cmp_cfg_tab->compare_prefetch_cfg.first_32k_page =
		cmp_req->streams[BI_COMPARE_DESC_CUR].buffer >> 15;
	cmp_cfg_tab->compare_kptnum_cfg.to_use = 1;

	for (i = 0; i < MATCHER_KPT_NUM; i++) {
		cmp_cfg_tab->compare_kptnum_cfg.compare_ref_kpt_num[i] =
		cmp_req->reg_cfg.compare_kptnum_cfg.compare_ref_kpt_num[i];
		cmp_cfg_tab->compare_kptnum_cfg.compare_cur_kpt_num[i] =
		cmp_req->reg_cfg.compare_kptnum_cfg.compare_cur_kpt_num[i];
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cmp_module_config(
	struct msg_req_compare_request_t *cmp_request,
	struct cfg_tab_compare_t *cmp_cfg_tab,
	struct cfg_tab_cvdr_t *cmp_cvdr_cfg_tab)
{
	int ret;

    ret = set_ipp_cfg_cmp(cmp_request, &cmp_cfg_tab[0]);
	if (ret < 0) {
		E("failed : set_ipp_cfg_cmp");
		return ISP_IPP_ERR;
	}
	ret = cmp_update_cvdr_cfg_tab(cmp_request, &cmp_cvdr_cfg_tab[0]);
	if (ret < 0) {
		E("failed : cmp_update_cvdr_cfg_tab");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cfg_cmp_cmdlst_para(
	struct msg_req_compare_request_t *cmp_request,
	struct cfg_tab_compare_t *cmp_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;

	memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t),
		0, sizeof(struct cmdlst_wr_cfg_t));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;
	stripe_cnt = (cmp_request->compare_matched_kpt == 0) ?
		(stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (cmp_request->compare_index == 0) ?
		(stripe_cnt) : (stripe_cnt + 1);

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i != 0) {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
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

		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_CMP;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_CMP_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		cvdr_prepare_nr_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[i].cmd_buf,
			&cvdr_cfg_tab[0]);
		compare_prepare_cmd(
			&g_compare_devs[0],
			&cmd_link_entry[i].cmd_buf,
			&cmp_cfg_tab[0]);
		cvdr_prepare_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[i++].cmd_buf,
			&cvdr_cfg_tab[0]);

		if (cmp_request->compare_matched_kpt) {
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr =
				cmp_request->compare_matched_kpt;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_COMPARE +
					COMPARE_MATCH_POINTS_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (cmp_request->compare_index) {
			cmdlst_wr_cfg.data_size = CMP_INDEX_NUM_MAX;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = cmp_request->compare_index;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_COMPARE + COMPARE_INDEX_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}
	}

    return ISP_IPP_OK;
}

int compare_request_handler(
	struct msg_req_compare_request_t *compare_request)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret;

	D("+\n");

	if (compare_request == NULL) {
		E("failed : compare_request is null");
		return -1;
	}

	compare_request_dump(compare_request);
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
        return ISP_IPP_ERR;
	}

	struct cfg_tab_cvdr_t *compare_cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_COMPARE, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CMDLST_PARA_COMPARE);
		return ISP_IPP_ERR;
	}

	struct cmdlst_para_t *compare_cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;

	loge_if_ret(seg_ipp_cmp_module_config(
		compare_request, compare_cfg_tab, compare_cvdr_cfg_tab));
	loge_if_ret(seg_ipp_cfg_cmp_cmdlst_para(
		compare_request, compare_cfg_tab,
		compare_cvdr_cfg_tab, compare_cmdlst_para));
	loge_if_ret(df_sched_start(compare_cmdlst_para));
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_COMPARE);
	D("-\n");
	return ISP_IPP_OK;
}

static int compare_request_dump(
	struct msg_req_compare_request_t *req)
{
	unsigned int i = 0;
	struct compare_req_kptnum_cfg_t *kptnum_cfg = NULL;
	struct compare_req_stat_cfg_t *stat_cfg = NULL;
	struct compare_req_offset_cfg_t *offset_cfg = NULL;
	struct compare_req_search_cfg_t *search_cfg = NULL;
	struct compare_req_block_cfg_t *block_cfg = NULL;
	struct compare_req_ctrl_cfg_t *ctrl_cfg = NULL;

	kptnum_cfg = &req->reg_cfg.compare_kptnum_cfg;
	stat_cfg = &req->reg_cfg.compare_stat_cfg;
	offset_cfg = &req->reg_cfg.compare_offset_cfg;
	search_cfg = &req->reg_cfg.compare_search_cfg;
	block_cfg = &req->reg_cfg.compare_block_cfg;
	ctrl_cfg = &req->reg_cfg.compare_ctrl_cfg;
	I("size = %d\n", (int)(sizeof(struct msg_req_compare_request_t)));
	I("frame_number = %d\n", req->frame_number);

	for (i = 0; i < COMPARE_STREAM_MAX; i++) {
		D("streams[%d].width = %d\n",  i, req->streams[i].width);
		D("streams[%d].height = %d\n", i, req->streams[i].height);
		D("streams[%d].stride = %d\n", i, req->streams[i].stride);
		D("streams[%d].buf = 0x%x\n",  i, req->streams[i].buffer);
		D("streams[%d].format = %d\n", i, req->streams[i].format);
	}

	D("compare_ctrl_cfg.compare_en = %d\n", ctrl_cfg->compare_en);
	D("compare_ctrl_cfg.descriptor_type = %d\n", ctrl_cfg->descriptor_type);
	D("compare_block_cfg.blk_v_num = %d\n", block_cfg->blk_v_num);
	D("compare_block_cfg.blk_h_num = %d\n", block_cfg->blk_h_num);
	D("compare_block_cfg.blk_num = %d\n", block_cfg->blk_num);
	D("compare_search_cfg.v_radius = %d\n", search_cfg->v_radius);
	D("compare_search_cfg.h_radius = %d\n", search_cfg->h_radius);
	D("compare_search_cfg.dis_ratio = %d\n", search_cfg->dis_ratio);
	D("compare_search_cfg.dis_threshold = %d\n", search_cfg->dis_threshold);
	D("compare_offset_cfg.cenh_offset = %d\n", offset_cfg->cenh_offset);
	D("compare_offset_cfg.cenv_offset = %d\n", offset_cfg->cenv_offset);
	D("compare_stat_cfg.stat_en = %d\n", stat_cfg->stat_en);
	D("compare_stat_cfg.max3_ratio = %d\n", stat_cfg->max3_ratio);
	D("compare_stat_cfg.bin_factor = %d\n", stat_cfg->bin_factor);
	D("compare_stat_cfg.bin_num = %d\n", stat_cfg->bin_num);
	D("compare_index = %d\n", req->compare_index);
	D("compare_matched_kpt = %d\n", req->compare_matched_kpt);
	D("reg_cfg.compare_kptnum_cfg.compare_ref_kpt_num = ");

	for (i = 0; i < 94 / 4 - 1; i++)
		D("0x%08x  0x%08x  0x%08x  0x%08x\n",
		kptnum_cfg->compare_ref_kpt_num[0 + 4 * i],
		kptnum_cfg->compare_ref_kpt_num[1 + 4 * i],
		kptnum_cfg->compare_ref_kpt_num[2 + 4 * i],
		kptnum_cfg->compare_ref_kpt_num[3 + 4 * i]);

	for (i = 0; i < 94 / 4 - 1; i++)
		D("0x%08x  0x%08x  0x%08x  0x%08x\n",
		kptnum_cfg->compare_cur_kpt_num[0 + 4 * i],
		kptnum_cfg->compare_cur_kpt_num[1 + 4 * i],
		kptnum_cfg->compare_cur_kpt_num[2 + 4 * i],
		kptnum_cfg->compare_cur_kpt_num[3 + 4 * i]);

	return 0;
}

#pragma GCC diagnostic pop
