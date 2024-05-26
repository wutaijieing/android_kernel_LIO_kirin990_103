/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_gf.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/

#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "segment_gf.h"
#include "cfg_table_gf.h"
#include "cvdr_drv.h"
#include "gf_drv.h"
#include "ipp_top_drv.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_GF
#define ALIGN_GF_WIDTH  16

static void gf_dump_request(struct msg_req_gf_request_t *req);
static int gf_calc_stripe_info(struct ipp_stream_t *stream,
							   struct ipp_stripe_info_t *p_stripe_info);

static int cfg_gf_cvdr_wr_port(struct msg_req_gf_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg,
							   struct ipp_stripe_info_t *gf_info,  unsigned int stripe_cnt)
{
	unsigned int total_bytes = 0;
	unsigned int gf_stride = 0;
	unsigned int stripe_width = gf_info->stripe_width[stripe_cnt];
	unsigned int overlap_left = gf_info->overlap_left[stripe_cnt];
	unsigned int overlap_right = gf_info->overlap_right[stripe_cnt];
	unsigned int addr_offset = gf_info->stripe_start_point[stripe_cnt] + overlap_left;
	struct cvdr_opt_fmt_t fmt;
	loge_if(memset_s(&fmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));
	fmt.pix_fmt = DF_FMT_INVALID;

	if (req->streams[BO_GF_LF_A].buffer) {
		fmt.id = IPP_VP_WR_GF_LF_A;
		fmt.width = stripe_width - overlap_left - overlap_right;
		fmt.height = req->streams[BO_GF_LF_A].height;
		fmt.addr = req->streams[BO_GF_LF_A].buffer + addr_offset;
		gf_stride = req->streams[BO_GF_LF_A].stride;
		fmt.pix_fmt = DF_2PF8;

		if (req->gf_hw_cfg.mode_cfg.mode_out == A_B) {
			fmt.addr = req->streams[BO_GF_LF_A].buffer + addr_offset * 2;
			total_bytes = req->streams[BO_GF_LF_A].height * req->streams[BO_GF_LF_A].width * 2;
			gf_stride = align_up(req->streams[BI_GF_GUI_I].width, CVDR_ALIGN_BYTES) * 2 / 16 - 1;
			loge_if_ret(dataflow_cvdr_wr_cfg_d32(p_cvdr_cfg,
												 &fmt, gf_stride, total_bytes));
		} else {
			loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
												&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_2PF8));
		}
	}

	if (req->streams[BO_GF_HF_B].buffer) {
		fmt.id        = IPP_VP_WR_GF_HF_B;
		fmt.width     = stripe_width - overlap_left - overlap_right;
		fmt.height    = req->streams[BO_GF_HF_B].height;
		fmt.addr      = req->streams[BO_GF_HF_B].buffer + addr_offset;
		gf_stride   = req->streams[BO_GF_HF_B].stride;
		fmt.pix_fmt   = DF_2PF8;

		if (req->gf_hw_cfg.mode_cfg.mode_out == A_B) {
			fmt.addr = req->streams[BO_GF_HF_B].buffer + addr_offset * 2;
			total_bytes = req->streams[BO_GF_HF_B].height * req->streams[BO_GF_LF_A].width * 2;
			gf_stride = align_up(req->streams[BI_GF_GUI_I].width, CVDR_ALIGN_BYTES) * 2 / 16 - 1;
			loge_if_ret(dataflow_cvdr_wr_cfg_d32(p_cvdr_cfg,
												 &fmt, gf_stride, total_bytes));
		} else {
			loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
												&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_2PF8));
		}
	}

	return ISP_IPP_OK;
}

static int cfg_gf_cvdr_rd_port(struct msg_req_gf_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg,
							   struct ipp_stripe_info_t *gf_info,  unsigned int stripe_cnt)
{
	unsigned int gf_stride = 0;
	unsigned int stripe_width = gf_info->stripe_width[stripe_cnt];
	struct cvdr_opt_fmt_t fmt;
	loge_if(memset_s(&fmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));
	fmt.pix_fmt = DF_FMT_INVALID;

	if (req->streams[BI_GF_GUI_I].buffer) {
		fmt.id        = IPP_VP_RD_GF_GUI_I;
		fmt.width     = stripe_width;
		fmt.height    = req->streams[BI_GF_GUI_I].height;
		fmt.addr      = req->streams[BI_GF_GUI_I].buffer + gf_info->stripe_start_point[stripe_cnt];
		fmt.line_size = fmt.width / 2;
		gf_stride   = req->streams[BI_GF_GUI_I].stride;
		fmt.pix_fmt   = DF_2PF8;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
											&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_2PF8));
	}

	if (req->streams[BI_GF_SRC_P].buffer) {
		fmt.id = IPP_VP_RD_GF_SRC_P;
		fmt.width = stripe_width;
		fmt.height = req->streams[BI_GF_SRC_P].height;
		fmt.addr = req->streams[BI_GF_SRC_P].buffer + gf_info->stripe_start_point[stripe_cnt];
		fmt.line_size = fmt.width / 2;
		gf_stride   = req->streams[BI_GF_SRC_P].stride;
		fmt.pix_fmt   = DF_2PF8;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
											&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_2PF8));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cfg_gf_cvdr(
	struct msg_req_gf_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg,
	struct ipp_stripe_info_t *gf_info,  unsigned int stripe_cnt)
{
	cfg_gf_cvdr_wr_port(req, p_cvdr_cfg, gf_info, stripe_cnt);
	cfg_gf_cvdr_rd_port(req, p_cvdr_cfg, gf_info, stripe_cnt);
	return ISP_IPP_OK;
}

static int set_ipp_cfg_gf(
	struct msg_req_gf_request_t *req, struct gf_config_table_t *cfg_tab,
	struct ipp_stripe_info_t *gf_stripe_info, unsigned int stripe_cnt)
{
	unsigned int gf_width = gf_stripe_info->stripe_width[stripe_cnt];
	unsigned int gf_overlap_l = gf_stripe_info->overlap_left[stripe_cnt];
	unsigned int gf_overlap_r = gf_stripe_info->overlap_right[stripe_cnt];
	cfg_tab->to_use = 1;
	cfg_tab->size_cfg.to_use = 1;
	cfg_tab->size_cfg.width = gf_stripe_info->stripe_width[stripe_cnt] - 1;
	cfg_tab->size_cfg.height = req->streams[BI_GF_GUI_I].height - 1;
	cfg_tab->mode_cfg.to_use = 1;
	cfg_tab->mode_cfg.mode_in = req->gf_hw_cfg.mode_cfg.mode_in;
	cfg_tab->mode_cfg.mode_out = req->gf_hw_cfg.mode_cfg.mode_out;
	cfg_tab->coeff_cfg.to_use = 1;
	cfg_tab->coeff_cfg.radius = req->gf_hw_cfg.coeff_cfg.radius;
	cfg_tab->coeff_cfg.eps = req->gf_hw_cfg.coeff_cfg.eps;
	cfg_tab->crop_cfg.to_use = 1;
	cfg_tab->crop_cfg.crop_left = gf_overlap_l;
	cfg_tab->crop_cfg.crop_right = gf_width - gf_overlap_r - 1;
	cfg_tab->crop_cfg.crop_top = 0;
	cfg_tab->crop_cfg.crop_bottom = gf_stripe_info->full_size.height - 1;
	return ISP_IPP_OK;
}

static int seg_ipp_gf_module_config(
	struct msg_req_gf_request_t *gf_request,
	struct seg_gf_cfg_t *seg_gf_cfg,
	struct ipp_stripe_info_t *gf_stripe_info)
{
	unsigned int i = 0;
	int ret = 0;

	for (i = 0; i < gf_stripe_info->stripe_cnt; i++) {
		ret |= set_ipp_cfg_gf(gf_request, &(seg_gf_cfg[i].gf_cfg_tab), gf_stripe_info, i);
		ret |= seg_ipp_cfg_gf_cvdr(gf_request, &(seg_gf_cfg[i].gf_cvdr_cfg_tab), gf_stripe_info, i);
	}

	if (ret != 0) {
		loge(" Failed : gf module config");
		return ISP_IPP_ERR;
	} else {
		return ISP_IPP_OK;
	}
}

static int seg_ipp_cfg_gf_cmdlst(
	struct msg_req_gf_request_t *gf_request,
	struct seg_gf_cfg_t *seg_gf_cfg,
	struct ipp_stripe_info_t *gf_stripe_info,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe = cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = gf_stripe_info->stripe_cnt;
	unsigned long long irq = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (gf_request->gf_hw_cfg.mode_cfg.mode_in == SINGLE_INPUT) {
			irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_RD_EOF_GUI_I);
		} else {
			irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_RD_EOF_GUI_I);
			irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_RD_EOF_SRC_P);
		}

		if (gf_request->gf_hw_cfg.mode_cfg.mode_out == LF_ONLY) {
			irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_WR_EOF_LF_A);
		} else { // mode_out is A_B
			irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_WR_EOF_LF_A);
			irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_WR_EOF_HF_B);
		}

		irq = irq | (((unsigned long long)(1)) << IPP_GF_CVDR_VP_RD_EOF_CMDLST);
		irq = irq | (((unsigned long long)(1)) << IPP_GF_DONE);
		logi("gf_stripe=%d, irq=0x%llx", i, irq);
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_GF;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_GF_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < gf_stripe_info->stripe_cnt; i++) {
		gf_prepare_cmd(&g_gf_devs[0], &cmd_link_entry[i].cmd_buf,
					   &(seg_gf_cfg[i].gf_cfg_tab));
		cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i].cmd_buf,
						 &(seg_gf_cfg[i].gf_cvdr_cfg_tab));
	}

	return ISP_IPP_OK;
}

static int mcf_convert_gf(
	struct msg_req_mcf_request_t *ctrl_mcf,
	struct msg_req_gf_request_t *ctrl_gf)
{
	struct adjust_y_t  *adjust_y = NULL;
	adjust_y = &(ctrl_mcf->u.y_lf_gf_mode_param.adjust_y);
	ctrl_gf->frame_number = ctrl_mcf->frame_number;
	ctrl_gf->gf_hw_cfg.mode_cfg.mode_in = ctrl_mcf->u.y_lf_gf_mode_param.input_mode;
	ctrl_gf->gf_hw_cfg.mode_cfg.mode_out = ctrl_mcf->u.y_lf_gf_mode_param.output_mode;
	ctrl_gf->gf_hw_cfg.coeff_cfg.radius = adjust_y->adjust_col_gf.y_gf_radius;
	ctrl_gf->gf_hw_cfg.coeff_cfg.eps = adjust_y->adjust_col_gf.y_gf_eps;
	ctrl_gf->streams[BI_GF_GUI_I].width = ctrl_mcf->streams[BI_MONO].width;
	ctrl_gf->streams[BI_GF_GUI_I].height = ctrl_mcf->streams[BI_MONO].height;
	ctrl_gf->streams[BI_GF_GUI_I].stride = ctrl_mcf->streams[BI_MONO].stride;
	ctrl_gf->streams[BI_GF_GUI_I].buffer = ctrl_mcf->streams[BI_MONO].buffer;
	ctrl_gf->streams[BI_GF_GUI_I].format = ctrl_mcf->streams[BI_MONO].format;
	ctrl_gf->streams[BI_GF_SRC_P].width = ctrl_mcf->streams[BI_WARP_COL_Y_DS4].width;
	ctrl_gf->streams[BI_GF_SRC_P].height = ctrl_mcf->streams[BI_WARP_COL_Y_DS4].height;
	ctrl_gf->streams[BI_GF_SRC_P].stride = ctrl_mcf->streams[BI_WARP_COL_Y_DS4].stride;
	ctrl_gf->streams[BI_GF_SRC_P].buffer = ctrl_mcf->streams[BI_WARP_COL_Y_DS4].buffer;
	ctrl_gf->streams[BI_GF_SRC_P].format = ctrl_mcf->streams[BI_WARP_COL_Y_DS4].format;
	ctrl_gf->streams[BO_GF_LF_A].width = ctrl_mcf->streams[BO_RESULT_Y].width;
	ctrl_gf->streams[BO_GF_LF_A].height = ctrl_mcf->streams[BO_RESULT_Y].height;
	ctrl_gf->streams[BO_GF_LF_A].stride = ctrl_mcf->streams[BO_RESULT_Y].stride;
	ctrl_gf->streams[BO_GF_LF_A].buffer = ctrl_mcf->streams[BO_RESULT_Y].buffer;
	ctrl_gf->streams[BO_GF_LF_A].format = ctrl_mcf->streams[BO_RESULT_Y].format;
	ctrl_gf->streams[BO_GF_HF_B].width = ctrl_mcf->streams[BO_RESULT_UV].width;
	ctrl_gf->streams[BO_GF_HF_B].height = ctrl_mcf->streams[BO_RESULT_UV].height;
	ctrl_gf->streams[BO_GF_HF_B].stride = ctrl_mcf->streams[BO_RESULT_UV].stride;
	ctrl_gf->streams[BO_GF_HF_B].buffer = ctrl_mcf->streams[BO_RESULT_UV].buffer;
	ctrl_gf->streams[BO_GF_HF_B].format = ctrl_mcf->streams[BO_RESULT_UV].format;
	return ISP_IPP_OK;
}

static int check_mcf_req_para(struct msg_req_mcf_request_t *mcf_request)
{
	if (mcf_request->mcf_cfg.mode != MCF_Y_GF_MODE) {
		loge("Failed : mcf only support Y_GF");
		return ISP_IPP_ERR;
	}

	if (mcf_request->streams[BI_MONO].width > IPP_SIZE_MAX) {
		loge("Failed : mcf size[%d] overflow ",
			 mcf_request->streams[BI_MONO].width);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_gf_get_cpe_mem(struct seg_gf_cfg_t **seg_gf_cfg, struct cmdlst_para_t **cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret;
	// 1.cfg_tbl
	ret = cpe_mem_get(MEM_ID_GF_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge("Failed : gf_mem_get.%d", MEM_ID_GF_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_gf_cfg = (struct seg_gf_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_gf_cfg, sizeof(struct seg_gf_cfg_t), 0, sizeof(struct seg_gf_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_gf_cfg error!");
		goto gf_get_cpe_mem_failed;
	}

	// 2. cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_GF, &va, &da);
	if (ret != 0) {
		loge(" Failed : gf_mem_get.%d", MEM_ID_CMDLST_PARA_GF);
		goto gf_get_cpe_mem_failed;
	}

	*cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s cmdlst_para_t error!");
		goto gf_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
gf_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_GF_CFG_TAB);
	return ISP_IPP_ERR;
}

int gf_request_handler(struct msg_req_mcf_request_t *mcf_request)
{
	int ret = 0;
	struct seg_gf_cfg_t *seg_gf_cfg = NULL;
	struct cmdlst_para_t *cmdlst_para = NULL;
	struct ipp_stripe_info_t *gf_stripe_info = NULL;
	struct msg_req_gf_request_t *gf_request = NULL;
	dataflow_check_para(mcf_request);
	loge_if_ret(check_mcf_req_para(mcf_request));
	gf_request = (struct msg_req_gf_request_t *)vmalloc(sizeof(struct msg_req_gf_request_t));
	loge_if_null(gf_request);
	ret = mcf_convert_gf(mcf_request, gf_request);
	if (ret != 0) {
		loge("Failed : mcf_convert_gf");
		goto fail_gf_mem;
	}

	gf_dump_request(gf_request);
	gf_stripe_info = (struct ipp_stripe_info_t *)vmalloc(sizeof(struct ipp_stripe_info_t));
	if (gf_stripe_info == NULL) {
		loge(" Failed to vmalloc gf_stripe_info");
		goto fail_gf_mem;
	}

	loge_if(memset_s(gf_stripe_info, sizeof(struct ipp_stripe_info_t), 0, sizeof(struct ipp_stripe_info_t)));
	gf_calc_stripe_info(&gf_request->streams[BI_GF_GUI_I], gf_stripe_info);
	ret = seg_ipp_gf_get_cpe_mem(&seg_gf_cfg, &cmdlst_para);
	if (ret != 0) {
		loge("Failed : seg_ipp_gf_get_cpe_mem fail");
		goto fail_get_mem;
	}

	ret = seg_ipp_gf_module_config(gf_request, seg_gf_cfg, gf_stripe_info);
	if (ret != 0) {
		loge("Failed : seg_ipp_gf_module_config error");
		goto seg_gf_request_fail;
	}

	ret = seg_ipp_cfg_gf_cmdlst(gf_request, seg_gf_cfg, gf_stripe_info, cmdlst_para);
	if (ret != 0) {
		loge("Failed : seg_ipp_cfg_gf_cmdlst error");
		goto seg_gf_request_fail;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0) loge("Failed : df_sched_start error");

seg_gf_request_fail:
	cpe_mem_free(MEM_ID_GF_CFG_TAB);
fail_get_mem:
	vfree(gf_stripe_info);
	gf_stripe_info = NULL;
fail_gf_mem:
	vfree(gf_request);
	gf_request = NULL;
	return ret;
}

static void gf_dump_request(struct msg_req_gf_request_t *req)
{
	int i = 0;
	char *stream_name[] = {
		"BI_GF_GUI_I",
		"BI_GF_SRC_P",
		"BO_GF_LF_A",
		"BO_GF_HF_B",
	};
	logi("sizeof(struct msg_req_gf_request_t) = 0x%x", (int)(sizeof(struct msg_req_gf_request_t)));
	logi("frame_number = %d", req->frame_number);

	for (i = 0; i < GF_STREAM_MAX; i++) {
		logi("streams[%s].width = %d", stream_name[i], req->streams[i].width);
		logi("streams[%s].height = %d", stream_name[i], req->streams[i].height);
		logi("streams[%s].stride = %d", stream_name[i], req->streams[i].stride);
		logi("streams[%s].format = %d", stream_name[i], req->streams[i].format);
		logi("streams[%s].buffer = 0x%x", stream_name[i], req->streams[i].buffer);
	}

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	logi("GF CFG DUMP:");
	logi("[GF_MODE] input mode = %d, out mode = %d",
		 req->gf_hw_cfg.mode_cfg.mode_in,
		 req->gf_hw_cfg.mode_cfg.mode_out);
	logi("[GF_FILTER+COEFF] radius = %d, eps = %d",
		 req->gf_hw_cfg.coeff_cfg.radius,
		 req->gf_hw_cfg.coeff_cfg.eps);
#endif
	return;
}

static int gf_calc_stripe_info(
	struct ipp_stream_t *stream,
	struct ipp_stripe_info_t *p_stripe_info)
{
	const unsigned int overlap = 32;
	const unsigned int max_stripe_width = 512;
	struct df_size_constrain_t  p_size_constrain = {0};
	unsigned int align_width = align_up(stream->width, ALIGN_GF_WIDTH);
	const unsigned int constrain_cnt = 1;
	p_size_constrain.hinc      = 1 * 65536;
	p_size_constrain.pix_align = overlap;
	p_size_constrain.out_width = 8912;
	logi("overlap =%d ,align_width =%d,max_stripe_width = %d ",
		 overlap, align_width, max_stripe_width);
	df_size_split_stripe(constrain_cnt, &p_size_constrain,
						 p_stripe_info, overlap, align_width, max_stripe_width);
	p_stripe_info->full_size.width = stream->stride;
	p_stripe_info->full_size.height = stream->height;
	df_size_dump_stripe_info(p_stripe_info, "gf_stripe");
	return ISP_IPP_OK;
}

