#include <linux/printk.h>
#include <linux/slab.h>
#include "segment_hiof.h"
#include "cfg_table_hiof.h"
#include "cvdr_drv.h"
#include "hiof_drv.h"
#include "ipp_top_drv.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_HIOF

static int hiof_check_request_para(struct msg_req_hiof_request_t *req);
static void hiof_dump_request(struct msg_req_hiof_request_t *req);

static int seg_ipp_hiof_cfg_cvdr_rd_cfg(struct msg_req_hiof_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	struct cvdr_opt_fmt_t fmt;
	unsigned int line_stride = 0;
	loge_if_ret(memset_s(&fmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));
	fmt.pix_fmt = DF_FMT_INVALID;

	if (req->streams[BI_REF_Y].buffer) {
		fmt.id = IPP_VP_RD_HIOF_REF_Y;
		fmt.width = req->streams[BI_REF_Y].width;
		fmt.height = req->streams[BI_REF_Y].height;
		fmt.addr = req->streams[BI_REF_Y].buffer;
		line_stride = 0;
		fmt.line_size = fmt.width;
		fmt.pix_fmt = DF_1PF8;
		fmt.full_width = req->streams[BI_REF_Y].width;
		fmt.expand = 0;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
											&fmt, line_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_1PF8));
	}

	if (req->streams[BI_CUR_Y].buffer) {
		fmt.id = IPP_VP_RD_HIOF_CUR_Y;
		fmt.width = req->streams[BI_CUR_Y].width;
		fmt.height = req->streams[BI_CUR_Y].height;
		fmt.addr = req->streams[BI_CUR_Y].buffer;
		line_stride = 0;
		fmt.line_size = fmt.width;
		fmt.pix_fmt = DF_1PF8;
		fmt.full_width = req->streams[BI_CUR_Y].width;
		fmt.expand = 0;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
											&fmt, line_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_1PF8));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_hiof_cfg_cvdr_wr_cfg(struct msg_req_hiof_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	struct cvdr_opt_fmt_t fmt;
	unsigned int line_stride = 0;
	loge_if_ret(memset_s(&fmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));
	fmt.pix_fmt = DF_FMT_INVALID;

	if (req->streams[BO_SPARSE_MV_CONFI].buffer && (req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD ||
			req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR)) {
		fmt.id = IPP_VP_WR_HIOF_SPARSE_MD;
		fmt.width = req->streams[BO_SPARSE_MV_CONFI].width;
		fmt.height = req->streams[BO_SPARSE_MV_CONFI].height;
		fmt.addr = req->streams[BO_SPARSE_MV_CONFI].buffer;
		line_stride = 0;
		fmt.pix_fmt = DF_2PF14;
		fmt.full_width = req->streams[BO_SPARSE_MV_CONFI].width;
		fmt.expand = 1;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
											&fmt, line_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_2PF14));
	}

	if (req->streams[BO_DENSE_MV_CONFI].buffer && (req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_TNR ||
			req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR)) {
		fmt.id = IPP_VP_WR_HIOF_DENSE_TNR;
		fmt.width = req->streams[BO_DENSE_MV_CONFI].width;
		fmt.height = req->streams[BO_DENSE_MV_CONFI].height;
		fmt.addr = req->streams[BO_DENSE_MV_CONFI].buffer;
		line_stride = 0;
		fmt.pix_fmt = DF_2PF14;
		fmt.full_width = req->streams[BO_DENSE_MV_CONFI].width;
		fmt.expand = 1;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
											&fmt, line_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_2PF14));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_hiof_cfg_cvdr(struct msg_req_hiof_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	loge_if_ret(seg_ipp_hiof_cfg_cvdr_rd_cfg(req, p_cvdr_cfg));
	loge_if_ret(seg_ipp_hiof_cfg_cvdr_wr_cfg(req, p_cvdr_cfg));
	return ISP_IPP_OK;
}

static int seg_ipp_hiof_cfg_hiof(struct msg_req_hiof_request_t *req, struct cfg_tab_hiof_t *cfg_tab)
{
	cfg_tab->hiof_ctrl_cfg.to_use = 1;
	cfg_tab->hiof_ctrl_cfg.mode = req->hiof_cfg.hiof_ctrl_cfg.mode;
	cfg_tab->hiof_ctrl_cfg.spena = req->hiof_cfg.hiof_ctrl_cfg.spena;
	cfg_tab->hiof_ctrl_cfg.iter_num = req->hiof_cfg.hiof_ctrl_cfg.iter_num;
	cfg_tab->hiof_ctrl_cfg.iter_check_en = req->hiof_cfg.hiof_ctrl_cfg.iter_check_en;
	cfg_tab->hiof_size_cfg.to_use = 1;
	cfg_tab->hiof_size_cfg.width = req->hiof_cfg.hiof_size_cfg.width;
	cfg_tab->hiof_size_cfg.height = req->hiof_cfg.hiof_size_cfg.height;
	cfg_tab->hiof_coeff_cfg.to_use = 1;
	cfg_tab->hiof_coeff_cfg.coeff0 = req->hiof_cfg.hiof_coeff_cfg.coeff0;
	cfg_tab->hiof_coeff_cfg.coeff1 = req->hiof_cfg.hiof_coeff_cfg.coeff1;
	cfg_tab->hiof_threshold_cfg.to_use = 1;
	cfg_tab->hiof_threshold_cfg.flat_thr = req->hiof_cfg.hiof_threshold_cfg.flat_thr;
	cfg_tab->hiof_threshold_cfg.ismotion_thr = req->hiof_cfg.hiof_threshold_cfg.ismotion_thr;
	cfg_tab->hiof_threshold_cfg.confilv_thr = req->hiof_cfg.hiof_threshold_cfg.confilv_thr;
	cfg_tab->hiof_threshold_cfg.att_max = req->hiof_cfg.hiof_threshold_cfg.att_max;
	return ISP_IPP_OK;
}

static int seg_ipp_hiof_module_config(
	struct msg_req_hiof_request_t *hiof_req,
	struct seg_hiof_cfg_t *seg_hiof_cfg)
{
	int ret = 0;
	struct cfg_tab_hiof_t  *hiof_cfg = &seg_hiof_cfg->hiof_cfg;
	struct cfg_tab_cvdr_t *cvdr_cfg = &seg_hiof_cfg->cvdr_cfg;
	ret |= seg_ipp_hiof_cfg_hiof(hiof_req, hiof_cfg);
	ret |= seg_ipp_hiof_cfg_cvdr(hiof_req, cvdr_cfg);
	if (ret != 0) {
		loge(" Failed : hiof module config");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_hiof_cfg_cmdlst(
	struct msg_req_hiof_request_t *hiof_req,
	struct seg_hiof_cfg_t *seg_hiof_cfg,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1; // because hiof not support stripe.
	unsigned long long irq = 0;
	struct cfg_tab_hiof_t  *hiof_cfg = &seg_hiof_cfg->hiof_cfg;
	struct cfg_tab_cvdr_t *cvdr_cfg = &seg_hiof_cfg->cvdr_cfg;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (hiof_req->streams[BI_REF_Y].buffer)
			irq = irq | (((unsigned long long)(1)) << IPP_HIOF_CVDR_VP_RD_EOF_REF_FRAME);

		if (hiof_req->streams[BI_CUR_Y].buffer)
			irq = irq | (((unsigned long long)(1)) << IPP_HIOF_CVDR_VP_RD_EOF_CUR_FRAME);

		if (hiof_req->streams[BO_SPARSE_MV_CONFI].buffer && (hiof_req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD ||
				hiof_req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR))
			irq = irq | (((unsigned long long)(1)) << IPP_HIOF_CVDR_VP_WR_EOF_MD);

		if (hiof_req->streams[BO_DENSE_MV_CONFI].buffer && (hiof_req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_TNR ||
				hiof_req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR))
			irq = irq | (((unsigned long long)(1)) << IPP_HIOF_CVDR_VP_WR_EOF_TNR);

		irq = irq | (((unsigned long long)(1)) << IPP_HIOF_CVDR_VP_RD_EOF_CMDLST);
		irq = irq | (((unsigned long long)(1)) << IPP_HIOF_DONE);
		logi("hiof_stripe=%d, irq=0x%llx", i, irq);
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_HIOF;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_HIOF_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	cmd_link_entry = cmdlst_para->cmd_entry;
	cvdr_prepare_vpwr_cmd(&g_cvdr_devs[0],
						  &cmd_link_entry[0].cmd_buf, cvdr_cfg);
	hiof_prepare_cmd(&g_hiof_devs[0],
					 &cmd_link_entry[0].cmd_buf, hiof_cfg);
	cvdr_prepare_vprd_cmd(&g_cvdr_devs[0],
						  &cmd_link_entry[0].cmd_buf, cvdr_cfg);
	cvdr_prepare_cmd(&g_cvdr_devs[0],
					 &cmd_link_entry[0].cmd_buf, cvdr_cfg);
	return ISP_IPP_OK;
}

static int seg_ipp_hiof_get_cpe_mem(struct seg_hiof_cfg_t **seg_hiof_cfg, struct cmdlst_para_t **cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1. cfg_tbl
	ret = cpe_mem_get(MEM_ID_HIOF_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge("Failed : hiof_mem_get.%d", MEM_ID_HIOF_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_hiof_cfg = (struct seg_hiof_cfg_t *)(uintptr_t)va; // CVDR cfg move to seg_hiof_cfg
	ret = memset_s(*seg_hiof_cfg, sizeof(struct seg_hiof_cfg_t), 0, sizeof(struct seg_hiof_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_hiof_cfg error!");
		goto hiof_get_cpe_mem_failed;
	}

	// 2. cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_HIOF, &va, &da);
	if (ret != 0) {
		loge(" Failed : hiof_mem_get.%d", MEM_ID_CMDLST_PARA_HIOF);
		goto hiof_get_cpe_mem_failed;
	}

	*cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s cmdlst_para error!");
		goto hiof_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
hiof_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_HIOF_CFG_TAB);
	return ret;
}

int hiof_request_handler(struct msg_req_hiof_request_t *req)
{
	int ret = 0;
	struct seg_hiof_cfg_t *seg_hiof_cfg = NULL; // CVDR cfg move to seg_hiof_cfg
	struct cmdlst_para_t *cmdlst_para = NULL;
	dataflow_check_para(req);
#if IPP_UT_DEBUG
	set_dump_register_init(UT_REG_ADDR, IPP_MAX_REG_OFFSET, 0);
#endif
	loge_if_ret(hiof_check_request_para(req));
	hiof_dump_request(req);
	ret = seg_ipp_hiof_get_cpe_mem(&seg_hiof_cfg, &cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_hiof_get_cpe_mem");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_hiof_module_config(req, seg_hiof_cfg);
	if (ret != 0) {
		loge(" Failed : seg_ipp_hiof_module_config");
		goto SEG_HIOF_BUFF_FERR;
	}

	ret = seg_ipp_hiof_cfg_cmdlst(req, seg_hiof_cfg, cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_hiof_cfg_cmdlst");
		goto SEG_HIOF_BUFF_FERR;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0)
		loge(" Failed : df_sched_start");

SEG_HIOF_BUFF_FERR:
	cpe_mem_free(MEM_ID_HIOF_CFG_TAB);
	return ret;
}

static int hiof_check_request_para(struct msg_req_hiof_request_t *req)
{
	if ((req->hiof_cfg.hiof_size_cfg.width > HIOF_MAX_IMG_W_SIZE) ||
		(req->hiof_cfg.hiof_size_cfg.width < HIOF_MIN_IMG_W_SIZE) ||
		(req->hiof_cfg.hiof_size_cfg.width % ALIGN_4_PIX != 0)) {
		loge(" Failed : hiof_size_cfg.width %d is error", req->hiof_cfg.hiof_size_cfg.width);
		return ISP_IPP_ERR;
	}

	if ((req->hiof_cfg.hiof_size_cfg.height > HIOF_MAX_IMG_H_SIZE) ||
		(req->hiof_cfg.hiof_size_cfg.height < HIOF_MIN_IMG_H_SIZE) ||
		(req->hiof_cfg.hiof_size_cfg.height % ALIGN_4_PIX != 0)) {
		loge(" Failed : hiof_size_cfg.height %d is error", req->hiof_cfg.hiof_size_cfg.height);
		return ISP_IPP_ERR;
	}

	if ((req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD) || (req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR)) {
		if (req->streams[BO_SPARSE_MV_CONFI].buffer == 0) {
			loge(" Failed : Hiof works in the %d mode, but BO_SPARSE_MV_CONFI buf is NULL", req->hiof_cfg.hiof_ctrl_cfg.mode);
			return ISP_IPP_ERR;
		}
	}

	if ((req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_TNR) || (req->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR)) {
		if (req->streams[BO_DENSE_MV_CONFI].buffer == 0) {
			loge(" Failed : Hiof works in the %d mode, but BO_DENSE_MV_CONFI buf is NULL", req->hiof_cfg.hiof_ctrl_cfg.mode);
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

static void hiof_dump_request(struct msg_req_hiof_request_t *req)
{
	int i = 0;
	char *stream_name[] = {
		"BI_REF_Y",
		"BI_CUR_Y",
		"BO_SPARSE_MV_CONFI",
		"BO_DENSE_MV_CONFI",
	};
	logi("sizeof(struct msg_req_hiof_request_t) = 0x%x",
		 (int)(sizeof(struct msg_req_hiof_request_t)));
	logi("frame_number = %d", req->frame_number);
	logi("hiof_rate_value = %d", req->hiof_rate_value);

	for (i = 0; i < HIOF_STREAM_MAX; i++) {
		logi("streams[%s].width = %d", stream_name[i], req->streams[i].width);
		logi("streams[%s].height = %d", stream_name[i], req->streams[i].height);
		logi("streams[%s].format = %d", stream_name[i], req->streams[i].format);
		logi("streams[%s].stride = %d", stream_name[i], req->streams[i].stride);
		logi("streams[%s].buffer = 0x%x", stream_name[i], req->streams[i].buffer);
	}

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logi("HIOF CFG DUMP:");
	logi("hiof_size_cfg: width = %d, height = %d",
		 req->hiof_cfg.hiof_size_cfg.width,
		 req->hiof_cfg.hiof_size_cfg.height);
	logi("hiof_ctrl_cfg: mode = %d, spena = %d, iter_num = %d, iter_check_en = %d",
		 req->hiof_cfg.hiof_ctrl_cfg.mode,
		 req->hiof_cfg.hiof_ctrl_cfg.spena,
		 req->hiof_cfg.hiof_ctrl_cfg.iter_num,
		 req->hiof_cfg.hiof_ctrl_cfg.iter_check_en);
	logi("hiof_coeff_cfg: coeff0 = %d, coeff1 = %d",
		 req->hiof_cfg.hiof_coeff_cfg.coeff0,
		 req->hiof_cfg.hiof_coeff_cfg.coeff1);
	logi("hiof_threshold_cfg: flat_thr = %d, ismotion_thr = %d, confilv_thr = %d, att_max = %d",
		 req->hiof_cfg.hiof_threshold_cfg.flat_thr,
		 req->hiof_cfg.hiof_threshold_cfg.ismotion_thr,
		 req->hiof_cfg.hiof_threshold_cfg.confilv_thr,
		 req->hiof_cfg.hiof_threshold_cfg.att_max);
#endif
	return;
}

