#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <securec.h>
#include "reorder.h"
#include "cfg_table_reorder.h"
#include "cvdr_drv.h"
#include "reorder_drv.h"
#include "config_table_cvdr.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "reorder_drv_priv.h"
#include "ipp_top_reg_offset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#define LOG_TAG LOG_MODULE_REORDER
#define CVDR_KEY_POINT_OVERFLOW  0x8000

unsigned int g_ipp_reorder_width[2] = {
	[IPP_FREAK_MODE] = 10,
	[IPP_BRIEF_MODE] = 6,
};

static int reorder_request_dump(struct msg_req_reorder_request_t *req);

static int rdr_update_cvdr_cfg_tab(
	struct msg_req_reorder_request_t *reorder_request,
	struct cfg_tab_cvdr_t *reorder_cfg_tab)
{
	struct cvdr_opt_fmt_t cfmt;
	unsigned int stride = 0;
	unsigned int des_type;

	des_type = reorder_request->reg_cfg.reorder_ctrl_cfg.descriptor_type;
	if (reorder_request->streams[BI_RDR_FP].buffer) {
		cfmt.id = IPP_VP_RD_RDR;
		cfmt.width = g_ipp_reorder_width[des_type];
		cfmt.line_size = cfmt.width;
		cfmt.height = reorder_request->streams[BI_RDR_FP].height;
		stride = cfmt.width / 2;
		cfmt.addr = reorder_request->streams[BI_RDR_FP].buffer;
		cfg_tbl_cvdr_rd_cfg_d64(reorder_cfg_tab,
			&cfmt, (ALIGN_UP(80 * 4096, CVDR_ALIGN_BYTES)), stride);
	}

	if (reorder_request->reorder_first_32k_page)
		cfg_tbl_cvdr_nr_wr_cfg(reorder_cfg_tab, IPP_NR_WR_RDR);

	return 0;
}

static int set_ipp_cfg_rdr(struct msg_req_reorder_request_t *reorder_request,
	struct cfg_tab_reorder_t *reorder_cfg_tab)
{
	reorder_cfg_tab->reorder_ctrl_cfg.to_use = 1;
	reorder_cfg_tab->reorder_ctrl_cfg.reorder_en =
		reorder_request->reg_cfg.reorder_ctrl_cfg.reorder_en;
	reorder_cfg_tab->reorder_ctrl_cfg.descriptor_type =
		reorder_request->reg_cfg.reorder_ctrl_cfg.descriptor_type;
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
	reorder_cfg_tab->reorder_prefetch_cfg.first_32k_page =
		reorder_request->reorder_first_32k_page >> 15;
	reorder_cfg_tab->reorder_kptnum_cfg.to_use = 1;
	loge_if(memcpy_s(
		&(reorder_cfg_tab->reorder_kptnum_cfg.reorder_kpt_num[0]),
		REORDER_KPT_NUM * sizeof(unsigned int),
		&(reorder_request->reg_cfg.reorder_kpt_num[0]),
		REORDER_KPT_NUM * sizeof(unsigned int)));
	return ISP_IPP_OK;
}

static int seg_ipp_rdr_module_config(
	struct msg_req_reorder_request_t *rdr_request,
	struct cfg_tab_reorder_t *rdr_cfg_tab,
	struct cfg_tab_cvdr_t *rdr_cvdr_cfg_tab)
{
	int ret = 0;

	ret = set_ipp_cfg_rdr(rdr_request, &rdr_cfg_tab[0]);
	if (ret < 0) {
		E("failed : set_ipp_cfg_rdr");
		return ISP_IPP_ERR;
	}
	ret = rdr_update_cvdr_cfg_tab(rdr_request,&rdr_cvdr_cfg_tab[0]);
	if (ret < 0) {
		E("failed : rdr_update_cvdr_cfg_tab");
		return ISP_IPP_ERR;
	}
	return ISP_IPP_OK;
}

static int seg_ipp_cfg_rdr_cmdlst_para(
	struct msg_req_reorder_request_t *rdr_request,
	struct cfg_tab_reorder_t *rdr_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;

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
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_RDR_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		cvdr_prepare_nr_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[0].cmd_buf, &cvdr_cfg_tab[0]);
		reorder_prepare_cmd(&g_reorder_devs[0],
			&cmd_link_entry[0].cmd_buf, &rdr_cfg_tab[0]);
		cvdr_prepare_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[0].cmd_buf, &cvdr_cfg_tab[0]);
	}

	return ISP_IPP_OK;
}

int reorder_request_handler(
	struct msg_req_reorder_request_t *reorder_request)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret;

	D("+\n");

	if (reorder_request == NULL) {
		E(" failed : reorder_request is null");
		return -1;
	}

	reorder_request_dump(reorder_request);
	ret = cpe_mem_get(MEM_ID_REORDER_CFG_TAB, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d", MEM_ID_REORDER_CFG_TAB);
		return ISP_IPP_ERR;
	}

	struct cfg_tab_reorder_t *reorder_cfg_tab =
		(struct cfg_tab_reorder_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_REORDER, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d", MEM_ID_CVDR_CFG_TAB_REORDER);
		return ISP_IPP_ERR;
	}

	struct cfg_tab_cvdr_t *reorder_cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_REORDER, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CMDLST_PARA_REORDER);
		return ISP_IPP_ERR;
	}

	struct cmdlst_para_t *reorder_cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;

	loge_if_ret(seg_ipp_rdr_module_config(reorder_request,
		reorder_cfg_tab, reorder_cvdr_cfg_tab));
	loge_if_ret(seg_ipp_cfg_rdr_cmdlst_para(reorder_request,
		reorder_cfg_tab, reorder_cvdr_cfg_tab, reorder_cmdlst_para));
	loge_if_ret(df_sched_start(reorder_cmdlst_para));
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_REORDER);
	D("-\n");
	return ISP_IPP_OK;
}

static int reorder_request_dump(struct msg_req_reorder_request_t *req)
{
	unsigned int i = 0;

	I("size_of_reorder_req = %d\n",
		(int)(sizeof(struct msg_req_reorder_request_t)));
	I("frame_number = %d\n", req->frame_number);

	for (i = 0; i < RDR_STREAM_MAX; i++) {
		D("streams[%d].width = %d\n",  i, req->streams[i].width);
		D("streams[%d].height = %d\n", i, req->streams[i].height);
		D("streams[%d].stride = %d\n", i, req->streams[i].stride);
		D("streams[%d].buf = 0x%x\n",  i, req->streams[i].buffer);
		D("streams[%d].format = %d\n", i, req->streams[i].format);
	}

	D("req->reorder_ctrl_cfg.reorder_en = %d\n",
		req->reg_cfg.reorder_ctrl_cfg.reorder_en);
	D("req->reorder_ctrl_cfg.descriptor_type = %d\n",
		req->reg_cfg.reorder_ctrl_cfg.descriptor_type);
	D("req->reorder_ctrl_cfg.total_kpt = %d\n",
		req->reg_cfg.reorder_ctrl_cfg.total_kpt);
	D("req->reorder_block_cfg.blk_v_num = %d\n",
		req->reg_cfg.reorder_block_cfg.blk_v_num);
	D("req->reorder_block_cfg.blk_h_num = %d\n",
		req->reg_cfg.reorder_block_cfg.blk_h_num);
	D("req->reorder_block_cfg.blk_num = %d\n",
		req->reg_cfg.reorder_block_cfg.blk_num);
	D("req->reorder_prefetch_cfg.first_32k_page = 0x%x\n",
		req->reorder_first_32k_page);
	D("req->reg_cfg.reorder_kptnum_cfg.reorder_kpt_num = ");

	for (i = 0; i < 94 / 4 - 1; i++)
		D("0x%08x  0x%08x  0x%08x  0x%08x\n",
		req->reg_cfg.reorder_kpt_num[0 + 4 * i],
		req->reg_cfg.reorder_kpt_num[1 + 4 * i],
		req->reg_cfg.reorder_kpt_num[2 + 4 * i],
		req->reg_cfg.reorder_kpt_num[3 + 4 * i]);

	return 0;
}

