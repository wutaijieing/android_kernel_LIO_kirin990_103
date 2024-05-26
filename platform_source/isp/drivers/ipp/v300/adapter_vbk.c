#include <linux/printk.h>
#include <linux/slab.h>
#include "vbk.h"
#include "cfg_table_vbk.h"
#include "cvdr_drv.h"
#include "vbk_drv.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"

#define LOG_TAG LOG_MODULE_VBK

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
/*lint -e429  -e613  -e668*/

#define IPP_VBK_SIZE_MAX     8192
#define IPP_VBK_SIZE_MIN      32
#define IPP_VBK_UV_SIZE_MAX  4096
#define IPP_VBK_UV_SIZE_MIN  16
#define SIZE_DIV_2 2
#define SIZE_DIV_4 4
#define SIZE_DIV_8 8

static int vbk_dump_request(struct msg_req_vbk_request_t *req);
static void vbk_calc_stripe_info(struct ipp_stream_t *stream,
	struct ipp_stripe_info_t *p_stripe_info);

static int vbk_update_cvdr_cfg_tab(
	struct msg_req_vbk_request_t *vbk_req,
	struct cfg_tab_cvdr_t *cfg_tab, struct ipp_stripe_info_t *stripe_info,
	unsigned int stripe_cnt)
{
	struct cvdr_opt_fmt_t cfmt;

	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t),
		0, sizeof(struct cvdr_opt_fmt_t)));
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int vbk_stride = 0;
	unsigned int stripe_width = stripe_info->stripe_width[stripe_cnt];
	unsigned int start_point = stripe_info->stripe_start_point[stripe_cnt];
	unsigned int overlap_left = stripe_info->overlap_left[stripe_cnt];
	unsigned int overlap_right = stripe_info->overlap_right[stripe_cnt];

	if (vbk_req->streams[BO_VBK_RESULT_UV].buffer) {
		cfmt.id = IPP_VP_WR_VBK_UV;
		cfmt.width = (stripe_width -  overlap_left - overlap_right) / 2;
		cfmt.full_width = vbk_req->streams[BO_VBK_RESULT_UV].width;
		cfmt.height = vbk_req->streams[BO_VBK_RESULT_UV].height;
		cfmt.expand = 0;
		format = vbk_req->streams[BO_VBK_RESULT_UV].format;
		cfmt.addr = vbk_req->streams[BO_VBK_RESULT_UV].buffer
			+ start_point + overlap_left;
		vbk_stride = vbk_req->streams[BO_VBK_RESULT_UV].stride;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cfg_tab,
			&cfmt, vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BO_VBK_RESULT_Y].buffer) {
		cfmt.id = IPP_VP_WR_VBK_Y;
		cfmt.width = stripe_width - overlap_left - overlap_right;
		cfmt.full_width = vbk_req->streams[BO_VBK_RESULT_Y].width / 2;
		cfmt.height = vbk_req->streams[BO_VBK_RESULT_Y].height;
		cfmt.expand = 0;
		format = vbk_req->streams[BO_VBK_RESULT_Y].format;
		cfmt.addr = vbk_req->streams[BO_VBK_RESULT_Y].buffer
			+ start_point + overlap_left;
		vbk_stride = vbk_req->streams[BO_VBK_RESULT_Y].stride;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cfg_tab,
			&cfmt, vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BO_VBK_RESULT_F].buffer) {
		cfmt.id = IPP_VP_WR_VBK_MASK_TF;
		cfmt.width = (stripe_width -  overlap_left - overlap_right) / 4;
		cfmt.full_width = vbk_req->streams[BO_VBK_RESULT_F].width;
		cfmt.height = vbk_req->streams[BO_VBK_RESULT_F].height;
		cfmt.expand = 0;
		format = vbk_req->streams[BO_VBK_RESULT_F].format;
		cfmt.addr = vbk_req->streams[BO_VBK_RESULT_F].buffer
			+ (start_point + overlap_left) / 4;
		vbk_stride = vbk_req->streams[BO_VBK_RESULT_F].stride;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cfg_tab, &cfmt,
			vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BI_VBK_SRC_Y_DS16].buffer) {
		cfmt.id = IPP_VP_RD_VBK_SRC_Y_DS16;
		cfmt.width = stripe_width / 4;
		cfmt.line_size = cfmt.width;
		cfmt.height = vbk_req->streams[BI_VBK_SRC_Y_DS16].height;
		cfmt.addr = vbk_req->streams[BI_VBK_SRC_Y_DS16].buffer
			+ start_point / 4;
		cfmt.full_width = vbk_req->streams[BI_VBK_SRC_Y_DS16].width;
		cfmt.expand = 0;
		format = vbk_req->streams[BI_VBK_SRC_Y_DS16].format;
		vbk_stride = vbk_req->streams[BI_VBK_SRC_Y_DS16].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cfg_tab, &cfmt,
			vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BI_VBK_SRC_UV_DS16].buffer) {
		cfmt.id = IPP_VP_RD_VBK_SRC_UV_DS16;
		cfmt.width = stripe_width / 8;
		cfmt.line_size = cfmt.width;
		cfmt.height = vbk_req->streams[BI_VBK_SRC_UV_DS16].height;
		cfmt.addr = vbk_req->streams[BI_VBK_SRC_UV_DS16].buffer
			+  start_point / 4;
		cfmt.full_width = vbk_req->streams[BI_VBK_SRC_UV_DS16].width;
		cfmt.expand = 0;
		format = vbk_req->streams[BI_VBK_SRC_UV_DS16].format;
		vbk_stride = vbk_req->streams[BI_VBK_SRC_UV_DS16].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cfg_tab, &cfmt,
			vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BI_VBK_SRC_UV].buffer) {
		cfmt.id = IPP_VP_RD_VBK_SRC_UV;
		cfmt.width =  stripe_width / 2;
		cfmt.line_size = cfmt.width;
		cfmt.height = vbk_req->streams[BI_VBK_SRC_UV].height;
		cfmt.addr = vbk_req->streams[BI_VBK_SRC_UV].buffer
			+ start_point;
		cfmt.full_width = vbk_req->streams[BI_VBK_SRC_UV].width;
		cfmt.expand = 0;
		format = vbk_req->streams[BI_VBK_SRC_UV].format;
		vbk_stride = vbk_req->streams[BI_VBK_SRC_UV].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cfg_tab,
			&cfmt, vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BI_VBK_SIGMA].buffer) {
		cfmt.id = IPP_VP_RD_VBK_SIGMA;
		cfmt.width = stripe_width / 4;
		cfmt.line_size = cfmt.width;
		cfmt.height = vbk_req->streams[BI_VBK_SIGMA].height;
		cfmt.addr = vbk_req->streams[BI_VBK_SIGMA].buffer
			+ start_point / 4;
		cfmt.full_width = vbk_req->streams[BI_VBK_SIGMA].width;
		cfmt.expand = 0;
		format = vbk_req->streams[BI_VBK_SIGMA].format;
		vbk_stride = vbk_req->streams[BI_VBK_SIGMA].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cfg_tab,
			&cfmt, vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BI_VBK_SRC_Y].buffer) {
		cfmt.id = IPP_VP_RD_VBK_SRC_Y;
		cfmt.width = stripe_width;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = vbk_req->streams[BI_VBK_SRC_Y].height;
		cfmt.addr = vbk_req->streams[BI_VBK_SRC_Y].buffer
			+ start_point;
		cfmt.full_width = vbk_req->streams[BI_VBK_SRC_Y].width / 2;
		cfmt.expand = 0;
		format = vbk_req->streams[BI_VBK_SRC_Y].format;
		vbk_stride = vbk_req->streams[BI_VBK_SRC_Y].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cfg_tab,
			&cfmt, vbk_stride, ISP_IPP_CLK, format));
	}

	if (vbk_req->streams[BI_VBK_FOREMASK].buffer) {
		cfmt.id = IPP_VP_RD_VBK_MASK_TF;
		cfmt.width = stripe_width / 4;
		cfmt.line_size = cfmt.width;
		cfmt.height = vbk_req->streams[BI_VBK_FOREMASK].height;
		cfmt.addr = vbk_req->streams[BI_VBK_FOREMASK].buffer
			+ start_point / 4;
		cfmt.full_width = vbk_req->streams[BI_VBK_FOREMASK].width;
		cfmt.expand = 0;
		format = vbk_req->streams[BI_VBK_FOREMASK].format;
		vbk_stride = vbk_req->streams[BI_VBK_FOREMASK].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cfg_tab,
			&cfmt, vbk_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int set_ipp_cfg_vbk(
	struct msg_req_vbk_request_t *vbk_request,
	struct vbk_config_table_t *vbk_cfg_tab,
	struct ipp_stripe_info_t *vbk_stripe_info,
	unsigned int stripe_cnt)
{
	vbk_cfg_tab->to_use = 1;
	vbk_cfg_tab->vbk_ctrl.to_use = 1;
	vbk_cfg_tab->vbk_ctrl.op_mode =
		vbk_request->vbk_hw_config.op_mode;
	vbk_cfg_tab->vbk_ctrl.y_ds16_gauss_en =
		vbk_request->vbk_hw_config.y_ds16_gauss_en;
	vbk_cfg_tab->vbk_ctrl.uv_ds16_gauss_en =
		vbk_request->vbk_hw_config.uv_ds16_gauss_en;
	vbk_cfg_tab->vbk_ctrl.sigma_gauss_en =
		vbk_request->vbk_hw_config.sigma_gauss_en;
	vbk_cfg_tab->vbk_ctrl.sigma2alpha_en =
		vbk_request->vbk_hw_config.sigma2alpha_en;
	vbk_cfg_tab->vbk_ctrl.in_width_ds4 =
		vbk_stripe_info->stripe_width[stripe_cnt] / 4;
	vbk_cfg_tab->vbk_ctrl.in_height_ds4 =
		vbk_request->vbk_hw_config.in_height_ds4;
	vbk_cfg_tab->yds16_gauss_coeff.to_use =
		vbk_cfg_tab->vbk_ctrl.y_ds16_gauss_en;
	vbk_cfg_tab->yds16_gauss_coeff.g00 =
		vbk_request->vbk_hw_config.yds16_gauss_coeff_g00;
	vbk_cfg_tab->yds16_gauss_coeff.g01 =
		vbk_request->vbk_hw_config.yds16_gauss_coeff_g01;
	vbk_cfg_tab->yds16_gauss_coeff.g02 =
		vbk_request->vbk_hw_config.yds16_gauss_coeff_g02;
	vbk_cfg_tab->yds16_gauss_coeff.g03 =
		vbk_request->vbk_hw_config.yds16_gauss_coeff_g03;
	vbk_cfg_tab->yds16_gauss_coeff.g04 =
		vbk_request->vbk_hw_config.yds16_gauss_coeff_g04;
	vbk_cfg_tab->yds16_gauss_coeff.inv =
		vbk_request->vbk_hw_config.yds16_gauss_coeff_inv;
	vbk_cfg_tab->sigma_gauss_coeff.to_use =
		vbk_cfg_tab->vbk_ctrl.sigma_gauss_en;
	vbk_cfg_tab->sigma_gauss_coeff.g00 =
		vbk_request->vbk_hw_config.sigma_gauss_coeff_g00;
	vbk_cfg_tab->sigma_gauss_coeff.g01 =
		vbk_request->vbk_hw_config.sigma_gauss_coeff_g01;
	vbk_cfg_tab->sigma_gauss_coeff.g02 =
		vbk_request->vbk_hw_config.sigma_gauss_coeff_g02;
	vbk_cfg_tab->sigma_gauss_coeff.g03 =
		vbk_request->vbk_hw_config.sigma_gauss_coeff_g03;
	vbk_cfg_tab->sigma_gauss_coeff.g04 =
		vbk_request->vbk_hw_config.sigma_gauss_coeff_g04;
	vbk_cfg_tab->sigma_gauss_coeff.inv =
		vbk_request->vbk_hw_config.sigma_gauss_coeff_inv;
	vbk_cfg_tab->uvds16_gauss_coeff.to_use =
		vbk_cfg_tab->vbk_ctrl.uv_ds16_gauss_en;
	vbk_cfg_tab->uvds16_gauss_coeff.g00 =
		vbk_request->vbk_hw_config.uvds16_gauss_coeff_g00;
	vbk_cfg_tab->uvds16_gauss_coeff.g01 =
		vbk_request->vbk_hw_config.uvds16_gauss_coeff_g01;
	vbk_cfg_tab->uvds16_gauss_coeff.g02 =
		vbk_request->vbk_hw_config.uvds16_gauss_coeff_g02;
	vbk_cfg_tab->uvds16_gauss_coeff.inv =
		vbk_request->vbk_hw_config.uvds16_gauss_coeff_inv;
	vbk_cfg_tab->sigma2alpha.to_use =
		vbk_cfg_tab->vbk_ctrl.sigma2alpha_en;
	loge_if(memcpy_s(&vbk_cfg_tab->sigma2alpha.sigma2alpha[0],
		VBK_SIGMA2ALPHA_NUM * sizeof(unsigned int),
			&vbk_request->vbk_hw_config.sigma2alpha_lut[0],
				VBK_SIGMA2ALPHA_NUM * sizeof(unsigned int)));
	vbk_cfg_tab->addnoise.addnoise_th =
		vbk_request->vbk_hw_config.addnoise_th;
	vbk_cfg_tab->foremask_cfg.foremask_coeff =
		vbk_request->vbk_hw_config.foremask_coeff;
	vbk_cfg_tab->foremask_cfg.foremask_th =
		vbk_request->vbk_hw_config.foremask_th;
	vbk_cfg_tab->foremask_cfg.foremask_weighted_filter_en =
		vbk_request->vbk_hw_config.foremask_weighted_filter_en;
	vbk_cfg_tab->foremask_cfg.foremask_dilation_radius =
		vbk_request->vbk_hw_config.foremask_dilation_radius;
	vbk_cfg_tab->y_crop_cfg.to_use = 1;
	vbk_cfg_tab->uv_crop_cfg.to_use = 1;
	vbk_cfg_tab->tf_crop_cfg.to_use = 1;
	vbk_cfg_tab->y_crop_cfg.crop_left =
		vbk_stripe_info->overlap_left[stripe_cnt];
	vbk_cfg_tab->y_crop_cfg.crop_right =
		vbk_stripe_info->stripe_width[stripe_cnt] -
			vbk_stripe_info->overlap_right[stripe_cnt] - 1;
	vbk_cfg_tab->y_crop_cfg.crop_top = 0;
	vbk_cfg_tab->y_crop_cfg.crop_bottom =
		vbk_stripe_info->full_size.height - 1;
	vbk_cfg_tab->uv_crop_cfg.crop_left =
		vbk_stripe_info->overlap_left[stripe_cnt] / 2;
	vbk_cfg_tab->uv_crop_cfg.crop_right =
		vbk_stripe_info->stripe_width[stripe_cnt] / 2 -
			vbk_stripe_info->overlap_right[stripe_cnt] / 2 - 1;
	vbk_cfg_tab->uv_crop_cfg.crop_top = 0;
	vbk_cfg_tab->uv_crop_cfg.crop_bottom =
		vbk_stripe_info->full_size.height / 2 - 1;
	vbk_cfg_tab->tf_crop_cfg.crop_left =
		vbk_stripe_info->overlap_left[stripe_cnt] / 4;
	vbk_cfg_tab->tf_crop_cfg.crop_right =
		vbk_stripe_info->stripe_width[stripe_cnt] / 4 -
			vbk_stripe_info->overlap_right[stripe_cnt] / 4 - 1;
	vbk_cfg_tab->tf_crop_cfg.crop_top = 0;
	vbk_cfg_tab->tf_crop_cfg.crop_bottom =
		vbk_stripe_info->full_size.height / 4 - 1;

	if (vbk_cfg_tab->vbk_ctrl.op_mode == VBK_FRONT_FACE_MODE) {
		vbk_cfg_tab->addnoise.to_use = 1;
		vbk_cfg_tab->foremask_cfg.to_use = 0;
	} else if (vbk_cfg_tab->vbk_ctrl.op_mode == VBK_BOKEH_MODE) {
		vbk_cfg_tab->addnoise.to_use = 0;
		vbk_cfg_tab->foremask_cfg.to_use = 1;
	} else {
		E(" Failed : Wrong mode input for VBK, mode = %d\n",
			vbk_cfg_tab->vbk_ctrl.op_mode);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_vbk_module_config(
	struct msg_req_vbk_request_t *vbk_request,
	struct vbk_config_table_t *vbk_cfg_tab,
	struct cfg_tab_cvdr_t *vbk_cvdr_cfg_tab,
	struct ipp_stripe_info_t *vbk_stripe_info)
{
	unsigned int i = 0;
	int ret = 0;
	unsigned int cnt = vbk_stripe_info->stripe_cnt;

	if (cnt == 0 || cnt > MAX_VBK_STRIPE_NUM) {
		E("failed : vbk stripe cnt error");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < vbk_stripe_info->stripe_cnt; i++) {
		ret = set_ipp_cfg_vbk(vbk_request,
			&vbk_cfg_tab[i], vbk_stripe_info, i);
		if (ret < 0) {
			E(" failed : set_ipp_cfg_vbk");
			return ISP_IPP_ERR;
		}

		ret = vbk_update_cvdr_cfg_tab(vbk_request,
			&vbk_cvdr_cfg_tab[i], vbk_stripe_info, i);
		if (ret < 0) {
			E(" failed : vbk_update_cvdr_cfg_tab");
			return ISP_IPP_ERR;
		}
	}
	return ISP_IPP_OK;
}

static int seg_ipp_cfg_vbk_cmdlst(
	struct msg_req_vbk_request_t *vbk_request,
	struct vbk_config_table_t *vbk_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct ipp_stripe_info_t *vbk_stripe_info,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
		cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned long long irq = 0;

	unsigned int stripe_cnt = vbk_stripe_info->stripe_cnt;

	if (stripe_cnt == 0 || stripe_cnt > MAX_VBK_STRIPE_NUM) {
		E("failed : vbk stripe cnt error");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (vbk_request->streams[BI_VBK_SRC_Y_DS16].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_SRC_Y_DS16);

		if (vbk_request->streams[BI_VBK_SRC_UV_DS16].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_SRC_UV_DS16);

		if (vbk_request->streams[BI_VBK_SRC_UV].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_SRC_UV);

		if (vbk_request->streams[BI_VBK_SIGMA].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_SIGMA);

		if (vbk_request->streams[BI_VBK_SRC_Y].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_SRC_Y);

		if (vbk_request->vbk_hw_config.op_mode == VBK_BOKEH_MODE)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_CURR_MASK);

		if (vbk_request->streams[BO_VBK_RESULT_UV].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_WR_EOF_UV);

		if (vbk_request->streams[BO_VBK_RESULT_Y].buffer)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_WR_EOF_Y);

		if (vbk_request->vbk_hw_config.op_mode == VBK_BOKEH_MODE)
			irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_WR_EOF_NEXT_MASK);

		irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_CVDR_VP_RD_EOF_CMDLST);
		irq = irq | (((unsigned long long)(1)) <<
			IPP_VBK_IRQ_DONE);
		cmdlst_stripe[i].hw_priority = CMD_PRIO_LOW;
		cmdlst_stripe[i].resource_share = 1 << IPP_CMD_RES_SHARE_VBK;
		cmdlst_stripe[i].irq_mode = irq;
		cmdlst_stripe[i].en_link = 0;
		cmdlst_stripe[i].ch_link = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_VBK_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;

	for (i = 0; i < vbk_stripe_info->stripe_cnt; i++) {
		vbk_prepare_cmd(&g_vbk_devs[0],
			&cmd_link_entry[i].cmd_buf, &vbk_cfg_tab[i]);
		cvdr_prepare_cmd(&g_cvdr_devs[0],
			&cmd_link_entry[i].cmd_buf, &cvdr_cfg_tab[i]);
	}

	return ISP_IPP_OK;
}

static int vbk_check_y_input_para(struct msg_req_vbk_request_t *vbk_request)
{
	if(vbk_request->streams[BI_VBK_SRC_Y].width > IPP_VBK_SIZE_MAX ||
		vbk_request->streams[BI_VBK_SRC_Y].height > IPP_VBK_SIZE_MAX ||
		vbk_request->streams[BI_VBK_SRC_Y].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BI_VBK_SRC_Y].width, vbk_request->streams[BI_VBK_SRC_Y].height,
		vbk_request->streams[BI_VBK_SRC_Y].format);
		return ISP_IPP_ERR;
	}

	if(vbk_request->streams[BI_VBK_SRC_Y_DS16].width > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
		vbk_request->streams[BI_VBK_SRC_Y_DS16].height > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
		vbk_request->streams[BI_VBK_SRC_Y_DS16].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BI_VBK_SRC_Y_DS16].width, vbk_request->streams[BI_VBK_SRC_Y_DS16].height,
		vbk_request->streams[BI_VBK_SRC_Y_DS16].format);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int vbk_check_uv_input_para(struct msg_req_vbk_request_t *vbk_request)
{
	if(vbk_request->streams[BI_VBK_SRC_UV].width > IPP_VBK_SIZE_MAX / SIZE_DIV_2 ||
		vbk_request->streams[BI_VBK_SRC_UV].height > IPP_VBK_SIZE_MAX / SIZE_DIV_2 ||
		vbk_request->streams[BI_VBK_SRC_UV].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BI_VBK_SRC_UV].width, vbk_request->streams[BI_VBK_SRC_UV].height,
		vbk_request->streams[BI_VBK_SRC_UV].format);
		return ISP_IPP_ERR;
	}

	if(vbk_request->streams[BI_VBK_SRC_UV_DS16].width > IPP_VBK_SIZE_MAX / SIZE_DIV_8 ||
		vbk_request->streams[BI_VBK_SRC_UV_DS16].height > IPP_VBK_SIZE_MAX / SIZE_DIV_8 ||
		vbk_request->streams[BI_VBK_SRC_UV_DS16].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BI_VBK_SRC_UV_DS16].width, vbk_request->streams[BI_VBK_SRC_UV_DS16].height,
		vbk_request->streams[BI_VBK_SRC_UV_DS16].format);
		return ISP_IPP_ERR;
	}
	return ISP_IPP_OK;
}

static int vbk_check_sigma_foremask_input_para(struct msg_req_vbk_request_t *vbk_request)
{
	if(vbk_request->streams[BI_VBK_SIGMA].width > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
		vbk_request->streams[BI_VBK_SIGMA].height > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
		vbk_request->streams[BI_VBK_SIGMA].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BI_VBK_SIGMA].width, vbk_request->streams[BI_VBK_SIGMA].height,
		vbk_request->streams[BI_VBK_SIGMA].format);
		return ISP_IPP_ERR;
	}

	if(vbk_request->vbk_hw_config.op_mode == VBK_BOKEH_MODE) {
		if(vbk_request->streams[BI_VBK_FOREMASK].width > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
			vbk_request->streams[BI_VBK_FOREMASK].height > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
			vbk_request->streams[BI_VBK_FOREMASK].buffer == 0) {
			E("vbk stream width = %d, height = %d, format = %d err",
			vbk_request->streams[BI_VBK_FOREMASK].width, vbk_request->streams[BI_VBK_FOREMASK].height,
			vbk_request->streams[BI_VBK_FOREMASK].format);
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

static int vbk_check_output_para(struct msg_req_vbk_request_t *vbk_request)
{
	if(vbk_request->streams[BO_VBK_RESULT_Y].width > IPP_VBK_SIZE_MAX ||
		vbk_request->streams[BO_VBK_RESULT_Y].height > IPP_VBK_SIZE_MAX ||
		vbk_request->streams[BO_VBK_RESULT_Y].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BO_VBK_RESULT_Y].width, vbk_request->streams[BO_VBK_RESULT_Y].height,
		vbk_request->streams[BO_VBK_RESULT_Y].format);
		return ISP_IPP_ERR;
	}

	if(vbk_request->streams[BO_VBK_RESULT_UV].width > IPP_VBK_SIZE_MAX / SIZE_DIV_2 ||
		vbk_request->streams[BO_VBK_RESULT_UV].height > IPP_VBK_SIZE_MAX / SIZE_DIV_2 ||
		vbk_request->streams[BO_VBK_RESULT_UV].buffer == 0) {
		E("vbk stream width = %d, height = %d, format = %d err",
		vbk_request->streams[BO_VBK_RESULT_UV].width, vbk_request->streams[BO_VBK_RESULT_UV].height,
		vbk_request->streams[BO_VBK_RESULT_UV].format);
		return ISP_IPP_ERR;
	}

	if(vbk_request->vbk_hw_config.op_mode == VBK_BOKEH_MODE) {
		if(vbk_request->streams[BO_VBK_RESULT_F].width > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
			vbk_request->streams[BO_VBK_RESULT_F].height > IPP_VBK_SIZE_MAX / SIZE_DIV_4 ||
			vbk_request->streams[BO_VBK_RESULT_F].buffer == 0) {
			E("vbk stream width = %d, height = %d, format = %d err",
			vbk_request->streams[BO_VBK_RESULT_F].width, vbk_request->streams[BO_VBK_RESULT_F].height,
			vbk_request->streams[BO_VBK_RESULT_F].format);
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

static int vbk_check_request_para(struct msg_req_vbk_request_t *vbk_request)
{
	int ret;
	if(vbk_request->vbk_hw_config.op_mode != VBK_BOKEH_MODE &&
		vbk_request->vbk_hw_config.op_mode != VBK_FRONT_FACE_MODE) {
		E("vbk op_mode[%d] err ", vbk_request->vbk_hw_config.op_mode);
		return ISP_IPP_ERR;
	}
	ret = vbk_check_y_input_para(vbk_request);
	if(ret != 0) {
		E("vbk check input stream y err");
		return ISP_IPP_ERR;
	}

	ret = vbk_check_uv_input_para(vbk_request);
	if(ret != 0) {
		E("vbk check input stream uv err");
		return ISP_IPP_ERR;
	}

	ret = vbk_check_sigma_foremask_input_para(vbk_request);
	if(ret != 0) {
		E("vbk check input stream sigma or foremask err");
		return ISP_IPP_ERR;
	}

	ret = vbk_check_output_para(vbk_request);
	if(ret != 0) {
		E("vbk check output stream err");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

int vbk_request_handler(
	struct msg_req_vbk_request_t *vbk_request)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;

	if (vbk_request == NULL) {
		E(" failed : vbk_request is null");
		return ISP_IPP_ERR;
	}

	ret = vbk_check_request_para(vbk_request);
	if(ret != 0) {
		E("[%s] failed : vbk_check_request_para", __func__);
		return ISP_IPP_ERR;
	}

	if(vbk_request->streams[BI_VBK_SRC_Y].width > IPP_SIZE_MAX) {
		E("failed : vbk size[%d] overflow ",
			vbk_request->streams[BI_VBK_SRC_Y].width);
		return ISP_IPP_ERR;
	}

	ret = vbk_dump_request(vbk_request);
	if (ret != 0) {
		E(" failed : vbk_dump_request");
		return ISP_IPP_ERR;
	}

	struct ipp_stripe_info_t *vbk_stripe_info = NULL;

	vbk_stripe_info = kmalloc(sizeof(struct ipp_stripe_info_t), GFP_KERNEL);
	if (vbk_stripe_info == NULL) {
		E(" fail to kmalloc vbk_stripe_info");
		return ISP_IPP_ERR;
	}
	loge_if(memset_s(vbk_stripe_info, sizeof(struct ipp_stripe_info_t),
		0, sizeof(struct ipp_stripe_info_t)));

	ret = cpe_mem_get(MEM_ID_VBK_CFG_TAB, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d", MEM_ID_VBK_CFG_TAB);
		goto fail_vbk_stripe_mem;
	}

	struct vbk_config_table_t *vbk_cfg_tab =
		(struct vbk_config_table_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CVDR_CFG_TAB_VBK, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d", MEM_ID_CVDR_CFG_TAB_VBK);
		goto fail_vbk_cfg_tab_mem;
	}

	struct cfg_tab_cvdr_t *cvdr_cfg_tab =
		(struct cfg_tab_cvdr_t *)(uintptr_t)va;

	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_VBK, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_CMDLST_PARA_VBK);
		goto fail_vbk_cvdr_tab_mem;
	}

	struct cmdlst_para_t *cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;

	vbk_calc_stripe_info(
		&vbk_request->streams[BI_VBK_SRC_Y], vbk_stripe_info);
	ret = seg_ipp_vbk_module_config(vbk_request,
		vbk_cfg_tab, cvdr_cfg_tab, vbk_stripe_info);
	if (ret != 0) {
		E(" Failed : seg_ipp_vbk_module_config");
		goto fail_vbk_cvdr_tab_mem;
	}
	ret = seg_ipp_cfg_vbk_cmdlst(vbk_request,
		vbk_cfg_tab, cvdr_cfg_tab, vbk_stripe_info, cmdlst_para);
	if (ret != 0) {
		E(" Failed : seg_ipp_cfg_vbk_cmdlst");
		goto fail_vbk_cvdr_tab_mem;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0)
		E(" Failed : df_sched_start");

fail_vbk_cvdr_tab_mem:
	cpe_mem_free(MEM_ID_CVDR_CFG_TAB_VBK);
fail_vbk_cfg_tab_mem:
	cpe_mem_free(MEM_ID_VBK_CFG_TAB);
fail_vbk_stripe_mem:
	kfree(vbk_stripe_info);
	return ret;
}

static void vbk_calc_stripe_info(struct ipp_stream_t *stream,
	struct ipp_stripe_info_t *p_stripe_info)
{
	unsigned int overlap = 128;
	unsigned int max_stripe_width = 3840;
	struct df_size_constrain_t  p_size_constrain = {0};
	unsigned int align_width = ALIGN_UP(stream->width, ALIGN_VBK_WIDTH);
	unsigned int constrain_cnt = 1;

	p_size_constrain.hinc      = 1 * 65536;
	p_size_constrain.pix_align = overlap;
	p_size_constrain.out_width = 8912;
	df_size_split_stripe(constrain_cnt, &p_size_constrain,
		p_stripe_info, overlap, align_width, max_stripe_width);
	p_stripe_info->full_size.width = stream->stride;
	p_stripe_info->full_size.height = stream->height;
	df_size_dump_stripe_info(p_stripe_info, "vbk_stripe");
}

static int vbk_dump_request(struct msg_req_vbk_request_t *req)
{
#if FLAG_LOG_DEBUG
	int i = 0;

	I("size of msg_req_vbk_request_t = %lu",
		sizeof(struct msg_req_vbk_request_t));
	I("frame_number = %d", req->frame_number);

	for (i = 0; i < VBK_STREAM_MAX; i++)
		I("streams[%d].width = %d,height = %d,buf = 0x%x,format = %d\n",
		  i, req->streams[i].width, req->streams[i].height,
		  req->streams[i].buffer, req->streams[i].format);

	D("vbk_hw_config_t.mode = %d",
		req->vbk_hw_config.op_mode);
	D("req->vbk_hw_config_t.y_ds16_gauss_en = %d ",
		req->vbk_hw_config.y_ds16_gauss_en);
	D("req->vbk_hw_config_t.uv_ds16_gauss_en = %d ",
		req->vbk_hw_config.uv_ds16_gauss_en);
	D("req->vbk_hw_config_t.sigma_gauss_en = %d ",
		req->vbk_hw_config.sigma_gauss_en);
	D("req->vbk_hw_config_t.sigma2alpha_en = %d ",
		req->vbk_hw_config.sigma2alpha_en);
	D("req->vbk_hw_config_t.in_width_ds4 = %d ",
		req->vbk_hw_config.in_width_ds4);
	D("req->vbk_hw_config_t.in_height_ds4 = %d ",
		req->vbk_hw_config.in_height_ds4);
	D("req->vbk_hw_config_t.yds16_gauss_coeff_g00 = %d ",
		req->vbk_hw_config.yds16_gauss_coeff_g00);
	D("req->vbk_hw_config_t.yds16_gauss_coeff_inv = %d ",
		req->vbk_hw_config.yds16_gauss_coeff_inv);
	D("req->vbk_hw_config_t.sigma_gauss_coeff_g00 = %d ",
		req->vbk_hw_config.sigma_gauss_coeff_g00);
	D("req->vbk_hw_config_t.sigma_gauss_coeff_inv = %d ",
		req->vbk_hw_config.sigma_gauss_coeff_inv);
	D("req->vbk_hw_config_t.uvds16_gauss_coeff_g00 = %d ",
		req->vbk_hw_config.uvds16_gauss_coeff_g00);
	D("req->vbk_hw_config_t.uvds16_gauss_coeff_inv = %d ",
		req->vbk_hw_config.uvds16_gauss_coeff_inv);
	D("req->vbk_hw_config_t.addnoise_th = %d ",
		req->vbk_hw_config.addnoise_th);
	D("req->vbk_hw_config_t.foremask_coeff = %d ",
		req->vbk_hw_config.foremask_coeff);
	D("req->vbk_hw_config_t.foremask_th = %d ",
		req->vbk_hw_config.foremask_th);
	D("req->vbk_hw_config_t.foremask_weighted_filter_en = %d ",
		req->vbk_hw_config.foremask_weighted_filter_en);
	D("req->vbk_hw_config_t.foremask_dilation_radius = %d ",
		req->vbk_hw_config.foremask_dilation_radius);
	D("req->vbk_hw_config_t.sigma2alpha_lut[0] = %d ",
		req->vbk_hw_config.sigma2alpha_lut[0]);
	D("req->vbk_hw_config_t.sigma2alpha_lut[50] = %d ",
		req->vbk_hw_config.sigma2alpha_lut[50]);
	D("req->vbk_hw_config_t.sigma2alpha_lut[100] = %d ",
		req->vbk_hw_config.sigma2alpha_lut[100]);
	D("req->vbk_hw_config_t.sigma2alpha_lut[255] = %d ",
		req->vbk_hw_config.sigma2alpha_lut[255]);
#endif
	return ISP_IPP_OK;
}
