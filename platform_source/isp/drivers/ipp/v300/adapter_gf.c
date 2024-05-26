#include <linux/printk.h>
#include <linux/slab.h>
#include "gf.h"
#include "cfg_table_gf.h"
#include "cvdr_drv.h"
#include "gf_drv.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_GF

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
/*lint -e429  -e613  -e668*/
#define ALIGN_GF_WIDTH  16

static void gf_dump_request(struct msg_req_gf_request_t *req);
static void gf_calc_stripe_info(struct ipp_stream_t *stream,
	struct ipp_stripe_info_t *p_stripe_info);

static int seg_ipp_cfg_gf_cvdr(
	struct msg_req_gf_request_t *req, struct cfg_tab_cvdr_t *p_cvdr_cfg,
	struct ipp_stripe_info_t *gf_info,  unsigned int stripe_cnt)
{
	struct cvdr_opt_fmt_t fmt;

	loge_if(memset_s(&fmt, sizeof(struct cvdr_opt_fmt_t),
		0, sizeof(struct cvdr_opt_fmt_t)));

	fmt.pix_fmt = DF_FMT_INVALID;
	unsigned int total_bytes = 0;
	unsigned int gf_stride = 0;
	unsigned int stripe_width = gf_info->stripe_width[stripe_cnt];
	unsigned int start_point = gf_info->stripe_start_point[stripe_cnt];
	unsigned int overlap_left = gf_info->overlap_left[stripe_cnt];
	unsigned int overlap_right = gf_info->overlap_right[stripe_cnt];
	unsigned int addr_offset = start_point + overlap_left;

	if (req->streams[BO_GF_LF_A].buffer) {
		fmt.id = IPP_VP_WR_GF_LF_A;
		fmt.width = stripe_width - overlap_left - overlap_right;
		fmt.height = req->streams[BO_GF_LF_A].height;
		fmt.addr = req->streams[BO_GF_LF_A].buffer + addr_offset;
		gf_stride = req->streams[BO_GF_LF_A].stride;
		fmt.pix_fmt = DF_2PF8;

		if (req->gf_hw_cfg.mode_cfg.mode_out == A_B) {
			fmt.addr = req->streams[BO_GF_LF_A].buffer +
						addr_offset * 2;
			total_bytes = req->streams[BO_GF_LF_A].height *
					req->streams[BO_GF_LF_A].width * 2;
			gf_stride = ALIGN_UP(req->streams[BI_GF_GUI_I].width,
				CVDR_ALIGN_BYTES) * 2 / 16 - 1;
			loge_if_ret(dataflow_cvdr_wr_cfg_d32(p_cvdr_cfg,
				&fmt, gf_stride, total_bytes));
		} else {
			loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
				&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_Y8));
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
			fmt.addr = req->streams[BO_GF_HF_B].buffer +
				addr_offset * 2;
			total_bytes = req->streams[BO_GF_HF_B].height *
				req->streams[BO_GF_LF_A].width * 2;
			gf_stride = ALIGN_UP(req->streams[BI_GF_GUI_I].width,
				CVDR_ALIGN_BYTES) * 2 / 16 - 1;
			loge_if_ret(dataflow_cvdr_wr_cfg_d32(p_cvdr_cfg,
				&fmt, gf_stride, total_bytes));
		} else {
			loge_if_ret(dataflow_cvdr_wr_cfg_vp(p_cvdr_cfg,
				&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_Y8));
		}
	}

	if (req->streams[BI_GF_GUI_I].buffer) {
		fmt.id        = IPP_VP_RD_GF_GUI_I;
		fmt.width     = stripe_width;
		fmt.height    = req->streams[BI_GF_GUI_I].height;
		fmt.addr      = req->streams[BI_GF_GUI_I].buffer + start_point;
		fmt.line_size = fmt.width / 2;
		gf_stride   = req->streams[BI_GF_GUI_I].stride;
		fmt.pix_fmt   = DF_2PF8;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
			&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_Y8));
	}

	if (req->streams[BI_GF_SRC_P].buffer) {
		fmt.id = IPP_VP_RD_GF_SRC_P;
		fmt.width = stripe_width;
		fmt.height = req->streams[BI_GF_SRC_P].height;
		fmt.addr = req->streams[BI_GF_SRC_P].buffer + start_point;
		fmt.line_size = fmt.width / 2;
		gf_stride   = req->streams[BI_GF_SRC_P].stride;
		fmt.pix_fmt   = DF_2PF8;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(p_cvdr_cfg,
			&fmt, gf_stride, ISP_IPP_CLK, PIXEL_FMT_IPP_Y8));
	}

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
	struct gf_config_table_t *gf_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct ipp_stripe_info_t *gf_stripe_info)
{
	unsigned int i = 0;
	int ret = 0;
	unsigned int cnt = gf_stripe_info->stripe_cnt;

	if (cnt == 0 || cnt > MAX_GF_STRIPE_NUM) {
		E("failed : stripe cnt error");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < gf_stripe_info->stripe_cnt; i++) {
		ret = set_ipp_cfg_gf(gf_request,&gf_cfg_tab[i], gf_stripe_info, i);
		if (ret < 0) {
			E("failed : set_ipp_cfg_gf");
			return ISP_IPP_ERR;
		}

		ret = seg_ipp_cfg_gf_cvdr(gf_request,
			&cvdr_cfg_tab[i],gf_stripe_info, i);
		if (ret < 0) {
			E("failed : seg_ipp_cfg_gf_cvdr");
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

static int seg_ipp_cfg_gf_cmdlst(
	struct msg_req_gf_request_t *gf_request,
	struct gf_config_table_t *gf_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct ipp_stripe_info_t *gf_stripe_info,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned long long irq = 0;

	unsigned int stripe_cnt = gf_stripe_info->stripe_cnt;

	if (stripe_cnt == 0 || stripe_cnt > MAX_GF_STRIPE_NUM) {
		E("failed : stripe cnt error");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (gf_request->gf_hw_cfg.mode_cfg.mode_in == SINGLE_INPUT) {
			irq = irq | (((unsigned long long)(1)) <<
				IPP_GF_CVDR_VP_RD_EOF_GUI_I);
		} else {
			irq = irq | (((unsigned long long)(1)) <<
				IPP_GF_CVDR_VP_RD_EOF_GUI_I);
			irq = irq | (((unsigned long long)(1)) <<
				IPP_GF_CVDR_VP_RD_EOF_SRC_P);
		}

		if (gf_request->gf_hw_cfg.mode_cfg.mode_out == LF_ONLY) {
			irq = irq | (((unsigned long long)(1)) <<
				IPP_GF_CVDR_VP_WR_EOF_LF_A);
		} else  if (gf_request->gf_hw_cfg.mode_cfg.mode_out == A_B) {
			irq = irq | (((unsigned long long)(1)) <<
				IPP_GF_CVDR_VP_WR_EOF_LF_A);
			irq = irq | (((unsigned long long)(1)) <<
				IPP_GF_CVDR_VP_WR_EOF_HF_B);
		}

		irq = irq | (((unsigned long long)(1)) <<
			IPP_GF_CVDR_VP_RD_EOF_CMDLST);
		irq = irq | (((unsigned long long)(1)) << IPP_GF_DONE);
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
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < gf_stripe_info->stripe_cnt; i++) {
		gf_prepare_cmd(&g_gf_devs[0],
			&cmd_link_entry[i].cmd_buf,
			&gf_cfg_tab[i]);
		cvdr_prepare_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[i].cmd_buf,
			&cvdr_cfg_tab[i]);
	}

	return ISP_IPP_OK;
}

static void mcf_convert_gf(
	struct msg_req_mcf_request_t *ctrl_mcf,
	struct msg_req_gf_request_t *ctrl_gf) {

	struct adjust_y_t  *adjust_y = NULL;
	adjust_y = &(ctrl_mcf->u.y_lf_gf_mode_param.adjust_y);
	ctrl_gf->frame_number = 
		ctrl_mcf->frame_number;
	ctrl_gf->gf_hw_cfg.mode_cfg.mode_in =
		ctrl_mcf->u.y_lf_gf_mode_param.input_mode;
	ctrl_gf->gf_hw_cfg.mode_cfg.mode_out =
		ctrl_mcf->u.y_lf_gf_mode_param.output_mode;

	ctrl_gf->gf_hw_cfg.coeff_cfg.radius = 
		adjust_y->adjust_col_gf.y_gf_radius;
	ctrl_gf->gf_hw_cfg.coeff_cfg.eps = 
		adjust_y->adjust_col_gf.y_gf_eps;

	ctrl_gf->streams[BI_GF_GUI_I].width = 
		ctrl_mcf->streams[BI_MONO].width;
	ctrl_gf->streams[BI_GF_GUI_I].height = 
		ctrl_mcf->streams[BI_MONO].height;
	ctrl_gf->streams[BI_GF_GUI_I].stride = 
		ctrl_mcf->streams[BI_MONO].stride;
	ctrl_gf->streams[BI_GF_GUI_I].buffer = 
		ctrl_mcf->streams[BI_MONO].buffer;
	ctrl_gf->streams[BI_GF_GUI_I].format = 
		ctrl_mcf->streams[BI_MONO].format;

	ctrl_gf->streams[BI_GF_SRC_P].width =
		ctrl_mcf->streams[BI_WARP_COL_Y_DS4].width;
	ctrl_gf->streams[BI_GF_SRC_P].height = 
		ctrl_mcf->streams[BI_WARP_COL_Y_DS4].height;
	ctrl_gf->streams[BI_GF_SRC_P].stride = 
		ctrl_mcf->streams[BI_WARP_COL_Y_DS4].stride;
	ctrl_gf->streams[BI_GF_SRC_P].buffer = 
		ctrl_mcf->streams[BI_WARP_COL_Y_DS4].buffer;
	ctrl_gf->streams[BI_GF_SRC_P].format = 
		ctrl_mcf->streams[BI_WARP_COL_Y_DS4].format;

	ctrl_gf->streams[BO_GF_LF_A].width =
		ctrl_mcf->streams[BO_RESULT_Y].width;
	ctrl_gf->streams[BO_GF_LF_A].height = 
		ctrl_mcf->streams[BO_RESULT_Y].height;
	ctrl_gf->streams[BO_GF_LF_A].stride = 
		ctrl_mcf->streams[BO_RESULT_Y].stride;
	ctrl_gf->streams[BO_GF_LF_A].buffer = 
		ctrl_mcf->streams[BO_RESULT_Y].buffer;
	ctrl_gf->streams[BO_GF_LF_A].format = 
		ctrl_mcf->streams[BO_RESULT_Y].format;

	ctrl_gf->streams[BO_GF_HF_B].width = 
		ctrl_mcf->streams[BO_RESULT_UV].width;
	ctrl_gf->streams[BO_GF_HF_B].height = 
		ctrl_mcf->streams[BO_RESULT_UV].height;
	ctrl_gf->streams[BO_GF_HF_B].stride = 
		ctrl_mcf->streams[BO_RESULT_UV].stride;
	ctrl_gf->streams[BO_GF_HF_B].buffer = 
		ctrl_mcf->streams[BO_RESULT_UV].buffer;
	ctrl_gf->streams[BO_GF_HF_B].format = 
		ctrl_mcf->streams[BO_RESULT_UV].format;
}

static int mcf_request_check_para(struct msg_req_mcf_request_t *mcf_request)
{
	if(mcf_request->mcf_cfg.mode != MCF_Y_GF_MODE) // only support GF
		return ISP_IPP_ERR;

	struct y_gf_mode_t *para = (struct y_gf_mode_t *) & (mcf_request->u.y_lf_gf_mode_param);

	if(mcf_request->streams[BI_MONO].buffer == 0 ||
		mcf_request->streams[BI_MONO].height > IPP_SIZE_MAX ||
		mcf_request->streams[BI_MONO].width > IPP_SIZE_MAX) {
		E("[%s] mcf check input para err", __func__);
		return ISP_IPP_ERR;
	}

	if(para->input_mode == DUAL_INPUT) {
		if(mcf_request->streams[BI_WARP_COL_Y_DS4].buffer == 0 ||
		mcf_request->streams[BI_WARP_COL_Y_DS4].height > IPP_SIZE_MAX ||
		mcf_request->streams[BI_WARP_COL_Y_DS4].width > IPP_SIZE_MAX ) {
			E("[%s] mcf check input ds4 para err", __func__);
			return ISP_IPP_ERR;
		}
	}

	if(mcf_request->streams[BO_RESULT_Y].buffer == 0 ||
	mcf_request->streams[BO_RESULT_Y].height > IPP_GF_MAX ||
	mcf_request->streams[BO_RESULT_Y].width > IPP_GF_MAX) {
		E("[%s] mcf check output y para err", __func__);
		return ISP_IPP_ERR;
	}

	if(para->output_mode == A_B || para->output_mode == LF_HF) {
		if(mcf_request->streams[BO_RESULT_UV].buffer == 0 ||
		mcf_request->streams[BO_RESULT_UV].height > IPP_GF_MAX ||
		mcf_request->streams[BO_RESULT_UV].width > IPP_GF_MAX ) {
			E("[%s] mcf check output uv para err", __func__);
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

int gf_request_handler(struct msg_req_mcf_request_t *mcf_request)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;

	if (mcf_request == NULL) {
		E(" failed : mcf_request is null");
		return ISP_IPP_ERR;
	}

	ret = mcf_request_check_para(mcf_request);
	if (ret != 0) {
		E("[%s] Failed : mcf_request_handler check.%d\n", __func__, ret);
		return ret;
	}

	struct msg_req_gf_request_t *gf_request = NULL;
	gf_request = kmalloc(sizeof(struct msg_req_gf_request_t), GFP_KERNEL);
	if (gf_request == NULL) {
		E(" fail to kmalloc gf_request\n");
		return ISP_IPP_ERR;
	}
	loge_if(memset_s(gf_request, sizeof(struct msg_req_gf_request_t),
		0, sizeof(struct msg_req_gf_request_t)));

	mcf_convert_gf(mcf_request, gf_request);
	gf_dump_request(gf_request);

	struct ipp_stripe_info_t *gf_stripe_info = NULL;
	gf_stripe_info = kmalloc(sizeof(struct ipp_stripe_info_t), GFP_KERNEL);
	if (gf_stripe_info == NULL) {
		E(" fail to kmalloc gf_stripe_info\n");
		goto fail_gf_request_mem;
	}
	loge_if(memset_s(gf_stripe_info, sizeof(struct ipp_stripe_info_t),
		0, sizeof(struct ipp_stripe_info_t)));

	ret = cpe_mem_get(MEM_ID_GF_CFG_TAB, &va, &da);
	if (ret != 0) {
		E("Failed : gf_mem_get.%d", MEM_ID_GF_CFG_TAB);
		goto fail_gf_stripe_mem;
	}

	struct gf_config_table_t *gf_cfg_tab =
		(struct gf_config_table_t *)(uintptr_t)va;
	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_GF, &va, &da);
	if (ret != 0) {
		E("Failed : gf_mem_get.%d", MEM_ID_CVDR_CFG_TAB_GF);
		goto fail_gf_cfg_tab_mem;
	}

	struct cfg_tab_cvdr_t *cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_GF, &va, &da);
	if (ret != 0) {
		E(" Failed : gf_mem_get.%d", MEM_ID_CMDLST_PARA_GF);
		goto fail_gf_cvdr_tab_mem;
	}

	struct cmdlst_para_t *cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	gf_calc_stripe_info(&gf_request->streams[BI_GF_GUI_I], gf_stripe_info);

	ret = seg_ipp_gf_module_config(gf_request,
		gf_cfg_tab, cvdr_cfg_tab, gf_stripe_info);
	if (ret != 0) {
		E(" Failed : seg_ipp_gf_module_config");
		goto fail_gf_cvdr_tab_mem;
	}

	ret = seg_ipp_cfg_gf_cmdlst(gf_request, gf_cfg_tab,
		cvdr_cfg_tab, gf_stripe_info, cmdlst_para);
	if (ret != 0) {
		E(" Failed : seg_ipp_cfg_gf_cmdlst");
		goto fail_gf_cvdr_tab_mem;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0)
		E(" Failed : seg_ipp_cfg_gf_cmdlst");

fail_gf_cvdr_tab_mem:
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_GF);
fail_gf_cfg_tab_mem:
	cpe_mem_free(MEM_ID_GF_CFG_TAB);
fail_gf_stripe_mem:
	kfree(gf_stripe_info);
fail_gf_request_mem:
	kfree(gf_request);

	return ret;
}

static void gf_dump_request(struct msg_req_gf_request_t *req)
{
#if FLAG_LOG_DEBUG
	int i = 0;

	if (req == NULL) {
		E("req is NULL!!\n");
		return;
	}

	I("size_of_gf_req = %d\n", (int)(sizeof(struct msg_req_gf_request_t)));
	I("frame_number = %d\n", req->frame_number);

	for (i = 0; i < GF_STREAM_MAX; i++) {
		D("streams[%d].width = %d,height = %d\n",
		i, req->streams[i].width, req->streams[i].height);
		D("stride = %d,buf = %d,format = %d\n",
		req->streams[i].stride, req->streams[i].buffer,
		req->streams[i].format);
	}

	D("GF DUMP: ");
	D("[GF_MODE] input mode = %d, out mode = %d\n",
	  req->gf_hw_cfg.mode_cfg.mode_in,
	  req->gf_hw_cfg.mode_cfg.mode_out);
	D("[GF_FILTER+COEFF] y_gf_radius = %d, y_gf_eps = %d\n",
	  req->gf_hw_cfg.coeff_cfg.radius,
	  req->gf_hw_cfg.coeff_cfg.eps);
#endif
}

static void gf_calc_stripe_info(
	struct ipp_stream_t *stream,
	struct ipp_stripe_info_t *p_stripe_info)
{
	if (stream == NULL || p_stripe_info == NULL) {
		E("pointer is NULL!!\n");
		return;
	}

	const unsigned int overlap = 32;
	const unsigned int max_stripe_width = 512;
	struct df_size_constrain_t  p_size_constrain = {0};
	unsigned int align_width = ALIGN_UP(stream->width, ALIGN_GF_WIDTH);
	const unsigned int constrain_cnt = 1;

	p_size_constrain.hinc      = 1 * 65536;
	p_size_constrain.pix_align = overlap;
	p_size_constrain.out_width = 8912;
	I("overlap =%d ,align_width =%d,max_stripe_width = %d ",
		overlap, align_width, max_stripe_width);
	df_size_split_stripe(constrain_cnt, &p_size_constrain,
		p_stripe_info, overlap, align_width, max_stripe_width);
	p_stripe_info->full_size.width = stream->stride;
	p_stripe_info->full_size.height = stream->height;
	df_size_dump_stripe_info(p_stripe_info, "gf_stripe");
}

/*lint +e429  +e613  +e668*/
#pragma GCC diagnostic pop
