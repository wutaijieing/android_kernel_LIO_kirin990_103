/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_ipp_path.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
#include "segment_ipp_path.h"
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <libhwsecurec/securec.h>
#include "segment_arfeature.h"
#include "cvdr_drv.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "ipp_top_reg_offset.h"
#include "compare_reg_offset.h"
#include "arfeature_reg_offset.h"
#include "orb_enh_drv.h"
#include "arfeature_drv.h"
#include "compare_drv.h"
#include "reorder_drv.h"
#include "mc_drv.h"
#include "cmdlst_drv.h"


#define LOG_TAG LOG_MODULE_IPP_PATH
#define CVDR_KEY_POINT_OVERFLOW  0x8000
#define CMDLST_READ_FLAG  1
#define NRT_CHANNEL (0x1 << 8)
#define ACTIVE_TOKEN_NBR_EN_SHIFT 7
#define ARF_STRIPE_MAX  20

struct global_info_ipp_t {
	// unsigned int work_module_bitset;
	unsigned int orb_enh_en;
	unsigned int arf_en;
	unsigned int matcher_en;
	unsigned int mc_en;

	unsigned int orb_enh_stripe_cnt;
	unsigned int arf_stripe_cnt;
	unsigned int rdr_stripe_cnt;
	unsigned int cmp_stripe_cnt;
	unsigned int cmp_layer_stripe_cnt;
	unsigned int mc_stripe_cnt;
	unsigned int total_stripe_cnt;

	unsigned int mc_token_num;
	unsigned int matcher_update_flag; // update rdr, cmp kpt, total_kpt, frame height
	unsigned int rdr_total_num_addr_cur;
	unsigned int rdr_kpt_num_addr_cur;
	unsigned int rdr_total_num_addr_ref;
	unsigned int rdr_kpt_num_addr_ref;
	unsigned int rdr_frame_height_addr_cur;
	unsigned int rdr_frame_height_addr_ref;
	unsigned int cmp_total_num_addr;
	unsigned int cmp_kpt_num_addr_cur;
	unsigned int cmp_kpt_num_addr_ref;
	unsigned int cmp_frame_height_addr;
	unsigned int mc_index_addr;
	unsigned int mc_matched_num_addr;
};

struct seg_enh_arf_stripe_t {
	unsigned int detect_total_stripe;
	unsigned int gauss_total_stripe;
	unsigned int layer_stripe_num;// stripes num per Detect Layer;
	unsigned int cur_frame_stripe_cnt; // cur only
	unsigned int total_stripe_num; // cur+ref  = 2*cur+2
};

struct seg_ipp_irq_t {
	unsigned long long irq[ARF_STRIPE_MAX];
};


static int seg_ipp_path_orb_enh_set_cvdr_cfg(struct req_arf_t *req_arfeature,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab, unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;

	if (req_arfeature->req_enh.streams[BI_ENH_YHIST].buffer) {
		cfmt.id = IPP_VP_RD_ORB_ENH_Y_HIST;
		cfmt.width = req_arfeature->req_enh.streams[BI_ENH_YHIST].width;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->req_enh.streams[BI_ENH_YHIST].height;
		cfmt.addr = req_arfeature->req_enh.streams[BI_ENH_YHIST].buffer;
		cfmt.full_width = req_arfeature->req_enh.streams[BI_ENH_YHIST].width / 2;
		cfmt.expand = 0;
		format = req_arfeature->req_enh.streams[BI_ENH_YHIST].format;
		line_stride = req_arfeature->req_enh.streams[BI_ENH_YHIST].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	if (req_arfeature->req_enh.streams[BI_ENH_YIMAGE].buffer) {
		cfmt.id = IPP_VP_RD_ORB_ENH_Y_IMAGE;
		cfmt.width = req_arfeature->req_enh.streams[BI_ENH_YIMAGE].width;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->req_enh.streams[BI_ENH_YIMAGE].height;
		cfmt.addr = req_arfeature->req_enh.streams[BI_ENH_YIMAGE].buffer;
		cfmt.full_width = req_arfeature->req_enh.streams[BI_ENH_YIMAGE].width / 2;
		cfmt.expand = 0;
		format = req_arfeature->req_enh.streams[BI_ENH_YIMAGE].format;
		line_stride = req_arfeature->req_enh.streams[BI_ENH_YIMAGE].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	if (req_arfeature->req_enh.streams[BO_ENH_OUT].buffer) {
		cfmt.id = IPP_VP_WR_ORB_ENH_Y;
		cfmt.width = req_arfeature->req_enh.streams[BO_ENH_OUT].width;
		cfmt.full_width = req_arfeature->req_enh.streams[BO_ENH_OUT].width / 2;
		cfmt.height = req_arfeature->req_enh.streams[BO_ENH_OUT].height;
		cfmt.expand = 0;
		format = req_arfeature->req_enh.streams[BO_ENH_OUT].format;
		cfmt.addr = req_arfeature->req_enh.streams[BO_ENH_OUT].buffer;
		line_stride = req_arfeature->req_enh.streams[BO_ENH_OUT].stride;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_cvdr_rd_port_0_1(struct req_arf_t *req_arfeature,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab, unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (req_arfeature->streams[stripe_index][BI_ARFEATURE_0].buffer) {
		cfmt.id = IPP_VP_RD_ARF_0;
		cfmt.width = req_arfeature->streams[stripe_index][BI_ARFEATURE_0].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BI_ARFEATURE_0].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BI_ARFEATURE_0].buffer;
		cfmt.expand = 0;

		if ((req_arfeature->reg_cfg[stripe_index].ctrl.mode == 0) || (req_arfeature->reg_cfg[stripe_index].ctrl.mode == 1)) {
			format = PIXEL_FMT_IPP_2PF8;
		} else {
			format = PIXEL_FMT_IPP_D32; // D32 is same as PIXEL_FMT_IPP_2PF16?
		}

		if (stripe_index == 0) {
			line_stride = req_arfeature->streams[stripe_index][BI_ARFEATURE_0].stride;
			loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
		} else {
			line_stride = 0;
			loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
		}
	}

	if (req_arfeature->streams[stripe_index][BI_ARFEATURE_1].buffer) {
		cfmt.id = IPP_VP_RD_ARF_1;
		cfmt.width = req_arfeature->streams[stripe_index][BI_ARFEATURE_1].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BI_ARFEATURE_1].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BI_ARFEATURE_1].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_cvdr_rd_port_2_3(struct req_arf_t *req_arfeature,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab, unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (req_arfeature->streams[stripe_index][BI_ARFEATURE_2].buffer) {
		cfmt.id = IPP_VP_RD_ARF_2;
		cfmt.width = req_arfeature->streams[stripe_index][BI_ARFEATURE_2].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BI_ARFEATURE_2].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BI_ARFEATURE_2].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	if (req_arfeature->streams[stripe_index][BI_ARFEATURE_3].buffer) {
		cfmt.id = IPP_VP_RD_ARF_3;
		cfmt.width = req_arfeature->streams[stripe_index][BI_ARFEATURE_3].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BI_ARFEATURE_3].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BI_ARFEATURE_3].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_cvdr_wr_port_pyr_1_2(struct req_arf_t *req_arfeature,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab, unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_1].buffer) {
		cfmt.id = IPP_VP_WR_ARF_PRY_1;

		if (req_arfeature->reg_cfg[stripe_index].ctrl.mode != ARF_DETECTION_MODE) {
			cfmt.width = req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_1].width;
			cfmt.full_width = cfmt.width / 2;
			cfmt.line_size = cfmt.width / 2;
			cfmt.height = req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_1].height;
			cfmt.addr = req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_1].buffer;
			cfmt.expand = 0;
			format = PIXEL_FMT_IPP_D32;
			line_stride = 0;
			loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
		}
	}

	if (req_arfeature->arfeature_stat_buff[DESC_BUFF] &&
		req_arfeature->reg_cfg[stripe_index].ctrl.mode == ARF_DETECTION_MODE) {
		cfmt.id = IPP_VP_WR_ARF_PRY_1;
		cfmt.addr = req_arfeature->arfeature_stat_buff[DESC_BUFF];
		loge_if_ret(cfg_tbl_cvdr_wr_cfg_d64(cvdr_cfg_tab, &cfmt, ARF_OUT_DESC_SIZE));
	}

	if (req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_2].buffer) {
		cfmt.id = IPP_VP_WR_ARF_PRY_2;
		cfmt.width = req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_2].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_2].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BO_ARFEATURE_PYR_2].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_cvdr_wr_port_dog_0_1(struct req_arf_t *req_arfeature,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab, unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_0].buffer) {
		cfmt.id = IPP_VP_WR_ARF_DOG_0;
		cfmt.width = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_0].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_0].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_0].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	if (req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_1].buffer) {
		cfmt.id = IPP_VP_WR_ARF_DOG_1;
		cfmt.width = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_1].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_1].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_1].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_cvdr_wr_port_dog_2_3(struct req_arf_t *req_arfeature,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab, unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;
	loge_if(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

	if (req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_2].buffer) {
		cfmt.id = IPP_VP_WR_ARF_DOG_2;
		cfmt.width = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_2].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_2].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_2].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	if (req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_3].buffer) {
		cfmt.id = IPP_VP_WR_ARF_DOG_3;
		cfmt.width = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_3].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_3].height;
		cfmt.addr = req_arfeature->streams[stripe_index][BO_ARFEATURE_DOG_3].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_D32;
		line_stride = 0;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(cvdr_cfg_tab, &cfmt, line_stride, ISP_IPP_CLK, format));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_set_cvdr_cfg(
	struct msg_req_ipp_path_t *req_ipp_path,
	struct cfg_enh_arf_t *enh_arf_cfg,
	unsigned int stripe_index,
	enum frame_flag_e frame_flag,
	struct global_info_ipp_t *global_info)
{
	struct cfg_tab_cvdr_t *cvdr_cfg_tab = NULL;
	struct req_arf_t *req_arfeature = NULL;
	logd("stripe_index = %d", stripe_index);
	req_arfeature = (frame_flag == FRAME_CUR) ? &(req_ipp_path->req_arf_cur) : &(req_ipp_path->req_arf_ref);
	cvdr_cfg_tab = (frame_flag == FRAME_CUR) ? &(enh_arf_cfg->arf_cvdr_cur[stripe_index]) :
				   &(enh_arf_cfg->arf_cvdr_ref[stripe_index]);

	if (global_info->orb_enh_en == 1 && stripe_index == 0)
		loge_if_ret(seg_ipp_path_orb_enh_set_cvdr_cfg(req_arfeature, cvdr_cfg_tab, stripe_index));

	seg_ipp_path_arf_cfg_cvdr_rd_port_0_1(req_arfeature, cvdr_cfg_tab, stripe_index);
	seg_ipp_path_arf_cfg_cvdr_rd_port_2_3(req_arfeature, cvdr_cfg_tab, stripe_index);
	seg_ipp_path_arf_cfg_cvdr_wr_port_pyr_1_2(req_arfeature, cvdr_cfg_tab, stripe_index);
	seg_ipp_path_arf_cfg_cvdr_wr_port_dog_0_1(req_arfeature, cvdr_cfg_tab, stripe_index);
	seg_ipp_path_arf_cfg_cvdr_wr_port_dog_2_3(req_arfeature, cvdr_cfg_tab, stripe_index);
	return ISP_IPP_OK;
}

static void seg_ipp_path_orb_enh_set_cfg_tab(
	struct cfg_tab_orb_enh_t *enh_cfg_tab, struct req_arf_t    *req_arf)
{
	enh_cfg_tab->to_use = 1;
	enh_cfg_tab->ctrl_cfg.to_use = 1;
	enh_cfg_tab->ctrl_cfg.enh_en = req_arf->req_enh.hw_config.ctrl_cfg.enh_en;
	enh_cfg_tab->ctrl_cfg.gsblur_en = req_arf->req_enh.hw_config.ctrl_cfg.gsblur_en;
	enh_cfg_tab->adjust_hist_cfg.to_use = 1;
	enh_cfg_tab->adjust_hist_cfg.lutscale = req_arf->req_enh.hw_config.adjust_hist_cfg.lutscale;
	enh_cfg_tab->gsblur_coef_cfg.to_use = 1;
	enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_0 = req_arf->req_enh.hw_config.gsblur_coef_cfg.coeff_gauss_0;
	enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_1 = req_arf->req_enh.hw_config.gsblur_coef_cfg.coeff_gauss_1;
	enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_2 = req_arf->req_enh.hw_config.gsblur_coef_cfg.coeff_gauss_2;
	enh_cfg_tab->gsblur_coef_cfg.coeff_gauss_3 = req_arf->req_enh.hw_config.gsblur_coef_cfg.coeff_gauss_3;
	enh_cfg_tab->cal_hist_cfg.to_use = 1;
	enh_cfg_tab->cal_hist_cfg.blknumy = req_arf->req_enh.hw_config.cal_hist_cfg.blknumy;
	enh_cfg_tab->cal_hist_cfg.blknumx = req_arf->req_enh.hw_config.cal_hist_cfg.blknumx;
	enh_cfg_tab->cal_hist_cfg.blk_ysize = req_arf->req_enh.hw_config.cal_hist_cfg.blk_ysize;
	enh_cfg_tab->cal_hist_cfg.blk_xsize = req_arf->req_enh.hw_config.cal_hist_cfg.blk_xsize;
	enh_cfg_tab->cal_hist_cfg.edgeex_y = req_arf->req_enh.hw_config.cal_hist_cfg.edgeex_y;
	enh_cfg_tab->cal_hist_cfg.edgeex_x = req_arf->req_enh.hw_config.cal_hist_cfg.edgeex_x;
	enh_cfg_tab->cal_hist_cfg.climit = req_arf->req_enh.hw_config.cal_hist_cfg.climit;
	enh_cfg_tab->adjust_image_cfg.to_use = 1;
	enh_cfg_tab->adjust_image_cfg.inv_xsize = req_arf->req_enh.hw_config.adjust_image_cfg.inv_xsize;
	enh_cfg_tab->adjust_image_cfg.inv_ysize = req_arf->req_enh.hw_config.adjust_image_cfg.inv_ysize;
}

static void seg_ipp_path_arf_cfg_sigma_gauss_coeff(struct cfg_tab_arfeature_t *arfeature_cfg_tab,
		struct req_arf_t *req_arfeature, unsigned int stripe_index)
{
	// sigma_coeff_cfg
	arfeature_cfg_tab->sigma_coeff_cfg.to_use = 1;
	arfeature_cfg_tab->sigma_coeff_cfg.sigma_ori = req_arfeature->reg_cfg[stripe_index].sigma_coef.sigma_ori;
	arfeature_cfg_tab->sigma_coeff_cfg.sigma_des = req_arfeature->reg_cfg[stripe_index].sigma_coef.sigma_des;
	// gauss_coeff_cfg
	arfeature_cfg_tab->gauss_coeff_cfg.to_use = 1;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_org_0 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_org_0;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_org_1 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_org_1;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_org_2 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_org_2;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_org_3 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_org_3;
	arfeature_cfg_tab->gauss_coeff_cfg.gsblur_1st_0 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gsblur_1st_0;
	arfeature_cfg_tab->gauss_coeff_cfg.gsblur_1st_1 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gsblur_1st_1;
	arfeature_cfg_tab->gauss_coeff_cfg.gsblur_1st_2 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gsblur_1st_2;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_2nd_0 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_2nd_0;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_2nd_1 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_2nd_1;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_2nd_2 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_2nd_2;
	arfeature_cfg_tab->gauss_coeff_cfg.gauss_2nd_3 = req_arfeature->reg_cfg[stripe_index].gauss_coef.gauss_2nd_3;
}

static void seg_ipp_path_arf_set_cfg_tab(
	struct msg_req_ipp_path_t *req_ipp_path,
	struct cfg_enh_arf_t *enh_arf_cfg,
	unsigned int stripe_index,
	enum frame_flag_e frame_flag,
	struct global_info_ipp_t *global_info)
{
	struct cfg_tab_arfeature_t *arfeature_cfg_tab = NULL;
	struct req_arf_t *req_arfeature = NULL;
	struct cfg_tab_orb_enh_t *orb_enh_cfg_tab = NULL;
	unsigned int i = 0;
	req_arfeature = (frame_flag == FRAME_CUR) ? &(req_ipp_path->req_arf_cur) : &(req_ipp_path->req_arf_ref);
	arfeature_cfg_tab = (frame_flag == FRAME_CUR) ?
						&(enh_arf_cfg->arf_cur[stripe_index]) : &(enh_arf_cfg->arf_ref[stripe_index]);
	arfeature_cfg_tab->top_cfg.to_use = 1;
	arfeature_cfg_tab->top_cfg.mode = req_arfeature->reg_cfg[stripe_index].ctrl.mode;
	arfeature_cfg_tab->top_cfg.orient_en = req_arfeature->reg_cfg[stripe_index].ctrl.orient_en;
	arfeature_cfg_tab->top_cfg.layer = req_arfeature->reg_cfg[stripe_index].ctrl.layer;
	arfeature_cfg_tab->top_cfg.iter_num = req_arfeature->reg_cfg[stripe_index].ctrl.iter_num;
	arfeature_cfg_tab->top_cfg.first_detect = req_arfeature->reg_cfg[stripe_index].ctrl.first_detect;
	arfeature_cfg_tab->size_cfg.to_use = 1;
	arfeature_cfg_tab->size_cfg.img_width = req_arfeature->reg_cfg[stripe_index].size_cfg.width;
	arfeature_cfg_tab->size_cfg.img_height = req_arfeature->reg_cfg[stripe_index].size_cfg.height;
	arfeature_cfg_tab->blk_cfg.to_use = 1;
	arfeature_cfg_tab->blk_cfg.blk_h_num = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_h_num;
	arfeature_cfg_tab->blk_cfg.blk_v_num = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_v_num;
	arfeature_cfg_tab->blk_cfg.blk_h_size_inv = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_h_size_inv;
	arfeature_cfg_tab->blk_cfg.blk_v_size_inv = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_v_size_inv;
	arfeature_cfg_tab->detect_num_lmt_cfg.to_use = 1;
	arfeature_cfg_tab->detect_num_lmt_cfg.max_kpnum = req_arfeature->reg_cfg[stripe_index].detect_num_lmt.max_kpnum;
	arfeature_cfg_tab->detect_num_lmt_cfg.cur_kpnum = req_arfeature->reg_cfg[stripe_index].detect_num_lmt.cur_kpnum;
	seg_ipp_path_arf_cfg_sigma_gauss_coeff(arfeature_cfg_tab, req_arfeature, stripe_index);
	arfeature_cfg_tab->dog_edge_thd_cfg.to_use = 1;
	arfeature_cfg_tab->dog_edge_thd_cfg.edge_threshold = req_arfeature->reg_cfg[stripe_index].dog_edge_thd.edge_threshold;
	arfeature_cfg_tab->dog_edge_thd_cfg.dog_threshold = req_arfeature->reg_cfg[stripe_index].dog_edge_thd.dog_threshold;
	arfeature_cfg_tab->mag_thd_cfg.to_use = 1;
	arfeature_cfg_tab->mag_thd_cfg.mag_threshold = req_arfeature->reg_cfg[stripe_index].req_descriptor_thd.mag_threshold;
	arfeature_cfg_tab->kpt_lmt_cfg.to_use = 1;

	for (i = 0; i < ARFEATURE_MAX_BLK_NUM; i++) {
		arfeature_cfg_tab->kpt_lmt_cfg.grid_max_kpnum[i] = req_arfeature->reg_cfg[stripe_index].kpt_lmt.grid_max_kpnum[i];
		arfeature_cfg_tab->kpt_lmt_cfg.grid_dog_threshold[i] =
			req_arfeature->reg_cfg[stripe_index].kpt_lmt.grid_dog_threshold[i];
	}

	if (arfeature_cfg_tab->top_cfg.mode == 4 && arfeature_cfg_tab->top_cfg.layer == 0 &&
		arfeature_cfg_tab->top_cfg.iter_num == 0) {
		// mean: the first detect layer and the first iter; and get the kpt buf from 'stream'.
		arfeature_cfg_tab->cvdr_cfg.to_use = 1;
		arfeature_cfg_tab->cvdr_cfg.wr_cfg.pre_wr_axi_fs.pre_vpwr_address_frame_start =
			req_arfeature->arfeature_stat_buff[DESC_BUFF]; // the first addr of kpt buf .
		arfeature_cfg_tab->cvdr_cfg.rd_cfg.pre_rd_fhg.pre_vprd_frame_size = 0;
		arfeature_cfg_tab->cvdr_cfg.rd_cfg.pre_rd_fhg.pre_vprd_vertical_blanking = 0;
	}

	if ((global_info->orb_enh_en == 1) && (stripe_index == 0)) {
		orb_enh_cfg_tab = (frame_flag == FRAME_CUR) ? &(enh_arf_cfg->enh_cur) : &(enh_arf_cfg->enh_ref);
		seg_ipp_path_orb_enh_set_cfg_tab(orb_enh_cfg_tab, req_arfeature);
	}
}

static int seg_ipp_path_arf_module_config(
	struct msg_req_ipp_path_t *ipp_path_req,
	struct cfg_enh_arf_t *enh_arf_cfg,
	struct global_info_ipp_t *global_info)
{
	unsigned int i = 0;
	enum frame_flag_e frame_flag = FRAME_CUR;
	logd("+");

	for (i = 0; i < ipp_path_req->mode_cnt; i++) {
		logd("config arf cur frame module cvdr and cfg_tab");
		seg_ipp_path_arf_set_cvdr_cfg(ipp_path_req, enh_arf_cfg, i, frame_flag, global_info);
		seg_ipp_path_arf_set_cfg_tab(ipp_path_req, enh_arf_cfg, i, frame_flag, global_info);
	}

	if (global_info->matcher_en == 1 && ipp_path_req->work_frame == CUR_REF) {
		logd("config arf ref frame module cvdr and cfg_tab");
		frame_flag = FRAME_REF;

		for (i = 0; i < ipp_path_req->mode_cnt; i++) {
			seg_ipp_path_arf_set_cvdr_cfg(ipp_path_req, enh_arf_cfg, i, frame_flag, global_info);
			seg_ipp_path_arf_set_cfg_tab(ipp_path_req, enh_arf_cfg, i, frame_flag, global_info);
		}
	}

	logd("-");
	return ISP_IPP_OK;
}


static int seg_ipp_path_arf_cal_stripe_num_info(struct msg_req_ipp_path_t *msg,
		struct seg_enh_arf_stripe_t *tmp_num, unsigned int i)
{
	// 1.Cyclically calculate how many cmdlst_stripe are needed in the current msg;
	if (msg->req_arf_cur.reg_cfg[i].ctrl.mode != ARF_DETECTION_MODE) {
		tmp_num->total_stripe_num += 1;
		tmp_num->gauss_total_stripe += 1;
	} else { // detect mode
		// 2: cfg current layer's arfeature and updata next layer's vp port cfg.
		tmp_num->total_stripe_num = (i != (msg->mode_cnt - 1)) ?
									(tmp_num->total_stripe_num + 2) : (tmp_num->total_stripe_num + 1);
		tmp_num->detect_total_stripe = (i != (msg->mode_cnt - 1)) ?
									   (tmp_num->detect_total_stripe + 2) : (tmp_num->detect_total_stripe + 1);
		tmp_num->layer_stripe_num = 2;

		if (msg->req_arf_cur.arfeature_stat_buff[MINSCR_KPTCNT_BUFF]) {
			tmp_num->total_stripe_num += 1;
			tmp_num->detect_total_stripe += 1;
			tmp_num->layer_stripe_num += 1;
		}

		if (msg->req_arf_cur.arfeature_stat_buff[SUM_SCORE_BUFF]) {
			tmp_num->total_stripe_num += 1;
			tmp_num->detect_total_stripe += 1;
			tmp_num->layer_stripe_num += 1;
		}
	}

	return ISP_IPP_OK;
}

static int seg_ipp_arf_cfg_dmap_gauss_first_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return ISP_IPP_OK;
}

static int seg_ipp_arf_cfg_ar_gauss_first_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_0);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return ISP_IPP_OK;
}

static int seg_ipp_arf_cfg_gauss_middle_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_0);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_3);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return ISP_IPP_OK;
}

static int seg_ipp_arf_cfg_gauss_last_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_3);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return ISP_IPP_OK;
}

static int seg_ipp_arf_cfg_detect_and_descriptor_irq(unsigned long long *irq, unsigned int pre_stripe_num,
		unsigned int j)
{
	if (j == pre_stripe_num) { // The configuration of the first stripe of the current layer.
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_PRY_2);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_DOG_1);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_DOG_2);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_1);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_CMDLST);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	} else {
		// These layers are all read and write operations of cmdlst,
		// so it is enough to monitor the read and write interruption of cmdlst.
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_CMDLST);
		*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_CMDLST);
	}

	return ISP_IPP_OK;
}

static int seg_ipp_arf_cfg_kptnum_totalkpt_cmdlst_stripe(struct msg_req_ipp_path_t *msg,
		struct cmdlst_stripe_info_t *cmdlst_stripe, struct seg_enh_arf_stripe_t *stripe_info,
		unsigned int pre_stripe_num)
{
	unsigned int j = 0;
	unsigned long long irq = 0;
	logd("+");

	if (msg->req_arf_cur.arfeature_stat_buff[KPT_NUM_BUFF])
		stripe_info->total_stripe_num += 1;

	if (msg->req_arf_cur.arfeature_stat_buff[TOTAL_KPT_BUFF])
		stripe_info->total_stripe_num += 1;

	for (j = pre_stripe_num; j < (stripe_info->total_stripe_num); j++) {
		irq = 0;
		cmdlst_stripe[j].resource_share  = 1 << IPP_CMD_RES_SHARE_ARFEATURE;
		cmdlst_stripe[j].is_need_set_sop = 0;
		cmdlst_stripe[j].en_link         = 0;
		cmdlst_stripe[j].ch_link         = 0;
		cmdlst_stripe[j].ch_link_act_nbr = 0;
		cmdlst_stripe[j].is_last_stripe  = 0;
		cmdlst_stripe[j].hw_priority     = CMD_PRIO_LOW;
		irq = irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_CMDLST);
		irq = irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_CMDLST);
		cmdlst_stripe[j].irq_mode  = irq;
	}

	logd("-");
	return ISP_IPP_OK;
}

static void seg_ipp_arf_init_cmdlst_stripe(struct cmdlst_stripe_info_t *cmdlst_stripe, unsigned int j)
{
	cmdlst_stripe[j].resource_share  = 1 << IPP_CMD_RES_SHARE_ARFEATURE;
	cmdlst_stripe[j].is_need_set_sop = 0;
	cmdlst_stripe[j].en_link         = 0;
	cmdlst_stripe[j].ch_link         = 0;
	cmdlst_stripe[j].ch_link_act_nbr = 0;
	cmdlst_stripe[j].is_last_stripe  = 0;
	cmdlst_stripe[j].hw_priority     = CMD_PRIO_LOW;
	return;
}

static int seg_ipp_path_arf_cal_stripe_num_and_cfg_cmdlst_stripe(
	struct msg_req_ipp_path_t *ipp_path_req, struct seg_enh_arf_stripe_t *stripe_info,
	struct cmdlst_stripe_info_t *cmdlst_stripe, struct global_info_ipp_t *global_info)
{
	unsigned int i = 0, j = 0;
	unsigned long long irq = 0;
	unsigned int pre_stripe_num = 0;

	for (i = 0; i < ipp_path_req->mode_cnt; i++) {
		// 1. calculate the total_stripe_num.
		seg_ipp_path_arf_cal_stripe_num_info(ipp_path_req, stripe_info, i);
		logd("arf_cur total_stripe_num = %d", stripe_info->total_stripe_num);

		// 2.CFG the cmdlst_stripe; (total_stripe_num - pre_stripe_num) means the current layer's cmdlst_stripe num.
		for (j = pre_stripe_num; j < stripe_info->total_stripe_num; j++) {
			irq = 0;
			seg_ipp_arf_init_cmdlst_stripe(cmdlst_stripe, j);

			switch (ipp_path_req->req_arf_cur.reg_cfg[i].ctrl.mode) {
			case DMAP_GAUSS_FIRST:
				seg_ipp_arf_cfg_dmap_gauss_first_irq(&irq);

				if (global_info->orb_enh_en == 1 && pre_stripe_num == 0) {
					// enh+arf mode 0 use one stripe;
					irq = irq & (~(((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG));
					irq = irq | (((unsigned long long)(1)) << IPP_ENH_DONE_IRQ);
				}

				break;

			case AR_GAUSS_FIRST:
				seg_ipp_arf_cfg_ar_gauss_first_irq(&irq);

				if (global_info->orb_enh_en == 1 && pre_stripe_num == 0) {
					// enh+arf mode 1 use one stripe; input from enh directly
					irq = irq & (~(((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG));
					irq = irq | (((unsigned long long)(1)) << IPP_ENH_DONE_IRQ);
				}

				break;

			case GAUSS_MIDDLE:
				seg_ipp_arf_cfg_gauss_middle_irq(&irq);
				break;

			case GAUSS_LAST:
				seg_ipp_arf_cfg_gauss_last_irq(&irq);
				break;

			case DETECT_AND_DESCRIPTOR:
				seg_ipp_arf_cfg_detect_and_descriptor_irq(&irq, pre_stripe_num, j);
				break;

			default:
				loge("Failed : arf mode is error, mode = %d", ipp_path_req->req_arf_cur.reg_cfg[i].ctrl.mode);
				break;
			}

			cmdlst_stripe[j].irq_mode  = irq;
			logd("stripe j = %d, irq=%lld", j, irq);
		}

		pre_stripe_num = stripe_info->total_stripe_num; // Updata the pre_stripe_num.;
	}

	return pre_stripe_num;
}

static int seg_ipp_path_arf_cal_stripe_info(
	struct msg_req_ipp_path_t *ipp_path_req, struct cmdlst_para_t *cmdlst_para,
	struct seg_enh_arf_stripe_t *stripe_info, struct global_info_ipp_t *global_info)
{
	unsigned int i = 0;
	unsigned int pre_stripe_num = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe = cmdlst_para->cmd_stripe_info;
	unsigned long long stat_irq = (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_CMDLST) |
								  (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_CMDLST);
	pre_stripe_num = seg_ipp_path_arf_cal_stripe_num_and_cfg_cmdlst_stripe(ipp_path_req,
					 stripe_info, cmdlst_stripe, global_info);
	// 3.Handle the case where KPT_NUM_BUFF and KPT_NUM_BUFF are not empty;
	seg_ipp_arf_cfg_kptnum_totalkpt_cmdlst_stripe(ipp_path_req, cmdlst_stripe, stripe_info, pre_stripe_num);

	// total_stripe_base_num = total_stripe_num;
	if (global_info->matcher_update_flag == 1) {
		// 4:update rdr kpt, cmp kpt, rdr total kpt, rdr frame height;
		for (i = 0; i < 4; i++) {
			seg_ipp_arf_init_cmdlst_stripe(cmdlst_stripe, stripe_info->total_stripe_num);
			cmdlst_stripe[stripe_info->total_stripe_num].irq_mode = stat_irq;
			stripe_info->total_stripe_num += 1;
		}

		stripe_info->cur_frame_stripe_cnt = stripe_info->total_stripe_num;
		logd("arf after update rdr para cur_frame_stripe_cnt = %d", stripe_info->cur_frame_stripe_cnt);

		if (ipp_path_req->work_frame == CUR_REF) {
			stripe_info->total_stripe_num  = stripe_info->total_stripe_num * 2;

			for (i = stripe_info->cur_frame_stripe_cnt; i < stripe_info->total_stripe_num; i++) {
				seg_ipp_arf_init_cmdlst_stripe(cmdlst_stripe, i);
				cmdlst_stripe[i].irq_mode = cmdlst_stripe[i - stripe_info->cur_frame_stripe_cnt].irq_mode;
			}

			seg_ipp_arf_init_cmdlst_stripe(cmdlst_stripe, stripe_info->total_stripe_num);
			cmdlst_stripe[stripe_info->total_stripe_num].irq_mode = stat_irq;
			seg_ipp_arf_init_cmdlst_stripe(cmdlst_stripe, stripe_info->total_stripe_num + 1);
			cmdlst_stripe[stripe_info->total_stripe_num + 1].irq_mode = stat_irq;
			stripe_info->total_stripe_num += 2; // 2: cmp total, cmp frame height
		}
	}

	cmdlst_stripe[stripe_info->total_stripe_num - 1].is_last_stripe  = 1;

	if (global_info->matcher_update_flag == 1) {
		cmdlst_stripe[stripe_info->total_stripe_num - 1].en_link         = 1;
		cmdlst_stripe[stripe_info->total_stripe_num - 1].ch_link         = IPP_RDR_CHANNEL_ID;
		cmdlst_stripe[stripe_info->total_stripe_num - 1].ch_link_act_nbr = (ipp_path_req->work_frame == CUR_REF) ?
				STRIPE_NUM_EACH_RDR * 2 : STRIPE_NUM_EACH_RDR;
	}

	cmdlst_para->channel_id = IPP_ARFEATURE_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_info->total_stripe_num;
	logd("final arf total_stripe_num = %d, detect_total_stripe = %d, "
		 "gauss_total_stripe = %d, layer_stripe_num = %d, cur_frame_stripe_cnt = %d",
		 stripe_info->total_stripe_num, stripe_info->detect_total_stripe,
		 stripe_info->gauss_total_stripe, stripe_info->layer_stripe_num, stripe_info->cur_frame_stripe_cnt);
	return ISP_IPP_OK;
}


static int seg_ipp_path_arf_update_matcher_cmdlst(
	struct schedule_cmdlst_link_t *cmd_link_entry,
	int cmd_index,
	struct global_info_ipp_t *global_info,
	enum frame_flag_e frame_flag)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;
	logd("+");
	logd("cmd_index = %d", cmd_index);

	if (global_info->matcher_update_flag == 1) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG_DS4;
		cmdlst_wr_cfg.is_wstrb = 1;
		cmdlst_wr_cfg.buff_wr_addr = (frame_flag == FRAME_CUR) ?
									 global_info->cmp_kpt_num_addr_cur :
									 global_info->cmp_kpt_num_addr_ref;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_KPT_NUMBER_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[cmd_index++].cmd_buf, &cmdlst_wr_cfg));
		cmdlst_wr_cfg.buff_wr_addr = (frame_flag == FRAME_CUR) ?
									 global_info->rdr_kpt_num_addr_cur : global_info->rdr_kpt_num_addr_ref;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[cmd_index++].cmd_buf, &cmdlst_wr_cfg));
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 1;
		cmdlst_wr_cfg.buff_wr_addr = (frame_flag == FRAME_CUR) ?
									 global_info->rdr_total_num_addr_cur : global_info->rdr_total_num_addr_ref;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_TOTAL_KPT_NUM_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[cmd_index++].cmd_buf, &cmdlst_wr_cfg));

		if (frame_flag == FRAME_REF) {
			cmdlst_wr_cfg.buff_wr_addr = global_info->cmp_total_num_addr;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[cmd_index++].cmd_buf, &cmdlst_wr_cfg));
		}

		cmdlst_wr_cfg.buff_wr_addr = (frame_flag == FRAME_CUR) ?
									 global_info->rdr_frame_height_addr_cur : global_info->rdr_frame_height_addr_ref;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_CVDR_RD_FHG_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[cmd_index++].cmd_buf, &cmdlst_wr_cfg));

		if (frame_flag == FRAME_REF) {
			cmdlst_wr_cfg.buff_wr_addr = global_info->cmp_frame_height_addr;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_CVDR_RD_FHG_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[cmd_index++].cmd_buf, &cmdlst_wr_cfg));
		}
	}

	logd("-");
	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_kptnum_totalkpt_cmd_buf(
	struct req_arf_t *req_arf,
	struct schedule_cmdlst_link_t *cmd_link_entry,
	unsigned int index)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;
	logd("+");

	if (req_arf->arfeature_stat_buff[KPT_NUM_BUFF]) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG_DS4;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = req_arf->arfeature_stat_buff[KPT_NUM_BUFF];
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_KPT_NUMBER_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[index].cmd_buf, &cmdlst_wr_cfg));
		index++;
	}

	if (req_arf->arfeature_stat_buff[TOTAL_KPT_BUFF]) {
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = req_arf->arfeature_stat_buff[TOTAL_KPT_BUFF];
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_TOTAL_KPT_NUM_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[index].cmd_buf, &cmdlst_wr_cfg));
		index++;
	}

	logd("-");
	return index;
}

static int seg_ipp_path_arf_cfg_minscr_and_sumscore_cmd_buf(
	struct req_arf_t *req_arf, struct schedule_cmdlst_link_t *cmd_link_entry,
	unsigned int i, unsigned int offset, unsigned int base_index)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;

	if (req_arf->arfeature_stat_buff[MINSCR_KPTCNT_BUFF]) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_arf->arfeature_stat_buff[MINSCR_KPTCNT_BUFF] +
			ARFEARURE_MINSCR_KPTCNT_SIZE * offset;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_GRIDSTAT_1_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i + base_index].cmd_buf, &cmdlst_wr_cfg));
		i++;
	}

	if (req_arf->arfeature_stat_buff[SUM_SCORE_BUFF]) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr = req_arf->arfeature_stat_buff[SUM_SCORE_BUFF] +
									 ARFEARURE_SUM_SCORE_SIZE * offset;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_GRIDSTAT_2_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i + base_index].cmd_buf, &cmdlst_wr_cfg));
		i++;
	}

	return i;
}

static int seg_ipp_path_arf_cfg_detect_layers_cmd_buf(
	struct req_arf_t *req_arf,
	struct cfg_enh_arf_t *enh_arf_cfg,
	enum frame_flag_e frame_flag,
	struct schedule_cmdlst_link_t *cmd_link_entry,
	struct seg_enh_arf_stripe_t *stripe_info,
	unsigned int base_index)
{
	unsigned int cur_layer = 0;
	unsigned int i = 0, offset = 0;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	struct cfg_tab_cvdr_t *cvdr_cfg_tmp = NULL;
	struct cfg_tab_arfeature_t *arf_cfg = NULL;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;
	logd("+");
	// detect operation.
	logd("gauss_total_stripe = %d, detect_total_stripe = %d", stripe_info->gauss_total_stripe,
		 stripe_info->detect_total_stripe);
	logd("base_index = %d", base_index);

	for (i = stripe_info->gauss_total_stripe; i < (stripe_info->gauss_total_stripe + stripe_info->detect_total_stripe);) {
		if (stripe_info->gauss_total_stripe == i) { // first detect ;
			cur_layer = stripe_info->gauss_total_stripe + ((i - stripe_info->gauss_total_stripe) /
						stripe_info->layer_stripe_num);
			cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->arf_cvdr_cur[cur_layer] :
						   &enh_arf_cfg->arf_cvdr_ref[cur_layer];
			arf_cfg = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->arf_cur[cur_layer] : &enh_arf_cfg->arf_ref[cur_layer];
			arfeature_prepare_cmd(&g_arfeature_devs[0], &cmd_link_entry[i + base_index].cmd_buf, arf_cfg);
			cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i + base_index].cmd_buf, cvdr_cfg_tmp);
			i++;
		} else {
			logd("update arf next stripe cvdr axi fs for AR mode descriptor");
			arf_updata_wr_addr_flag = 1;
			cur_layer = stripe_info->gauss_total_stripe + (((i + 1) - stripe_info->gauss_total_stripe) /
						stripe_info->layer_stripe_num);
			logd("2 cur_layer = %d, i = %d, base_index = %d", cur_layer, i, base_index);
			cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->arf_cvdr_cur[cur_layer] :
						   &enh_arf_cfg->arf_cvdr_ref[cur_layer];
			arf_cfg = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->arf_cur[cur_layer] : &enh_arf_cfg->arf_ref[cur_layer];
			cmdlst_set_vp_wr_flush(&g_cmdlst_devs[0], &(cmd_link_entry[i + 1 + base_index].cmd_buf), IPP_ARFEATURE_CHANNEL_ID);
			arfeature_prepare_cmd(&g_arfeature_devs[0], &(cmd_link_entry[i + 1 + base_index].cmd_buf), arf_cfg);
			cvdr_prepare_cmd(&g_cvdr_devs[0], &(cmd_link_entry[i + 1 + base_index].cmd_buf), cvdr_cfg_tmp);
			arf_updata_wr_addr_flag = 0;
			// Update the write address of the VP port;
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 1;
			cmdlst_wr_cfg.buff_wr_addr = cvdr_cfg_tmp->arf_cvdr_wr_addr;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_CVDR_WR_AXI_FS_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&(cmd_link_entry[i + base_index].cmd_buf), &cmdlst_wr_cfg));
			i = i + 2;
		}

		logd("cur_layer = %d", cur_layer);
		offset = cur_layer - stripe_info->gauss_total_stripe;
		i = seg_ipp_path_arf_cfg_minscr_and_sumscore_cmd_buf(req_arf, cmd_link_entry, i, offset, base_index);
	}

	logd("-");
	return i + base_index;
}


static int seg_ipp_path_arf_cmdlst_para(
	struct msg_req_ipp_path_t *ipp_path_req,
	struct cmdlst_para_t *cmdlst_para,
	struct cfg_enh_arf_t *enh_arf_cfg,
	struct seg_enh_arf_stripe_t *stripe_info,
	struct global_info_ipp_t *global_info,
	enum frame_flag_e frame_flag)
{
	unsigned int i;
	struct cfg_tab_cvdr_t *cvdr_cfg_tmp;
	struct req_arf_t *req_arf = (frame_flag == FRAME_CUR) ? &(ipp_path_req->req_arf_cur) : &(ipp_path_req->req_arf_ref);
	struct cfg_tab_arfeature_t *arf_cfg;
	struct schedule_cmdlst_link_t *cmd_link_entry = (struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	unsigned int base = (frame_flag == FRAME_CUR) ? 0 : stripe_info->cur_frame_stripe_cnt;
	unsigned int index;
	logd("arf cmdlst para stripe base = %d", base);

	// gauss operation.
	for (i = 0; i < stripe_info->gauss_total_stripe; i++) {
		if (global_info->orb_enh_en == 1 && i == 0) {
			struct cfg_tab_orb_enh_t *enh_cfg = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->enh_cur : &enh_arf_cfg->enh_ref;
			loge_if_ret(orb_enh_prepare_cmd(&g_orb_enh_devs[0], &cmd_link_entry[i + base].cmd_buf, enh_cfg));
		}

		arf_cfg = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->arf_cur[i] : &enh_arf_cfg->arf_ref[i];
		loge_if_ret(arfeature_prepare_cmd(&g_arfeature_devs[0], &cmd_link_entry[i + base].cmd_buf, arf_cfg));
		// arf cvdr include enh cvdr
		cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ? &enh_arf_cfg->arf_cvdr_cur[i] : &enh_arf_cfg->arf_cvdr_ref[i];
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i + base].cmd_buf, cvdr_cfg_tmp));
	}

	index = seg_ipp_path_arf_cfg_detect_layers_cmd_buf(req_arf, enh_arf_cfg, frame_flag, cmd_link_entry,
						 stripe_info, base);
	logd("arf cmdlst para stripe index = %d", index);

	index = seg_ipp_path_arf_cfg_kptnum_totalkpt_cmd_buf(req_arf, cmd_link_entry, index);
	logd("arf cmdlst para stripe index = %d", index);

	loge_if_ret(seg_ipp_path_arf_update_matcher_cmdlst(cmd_link_entry, index, global_info,frame_flag));
	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_cfg_cmdlst_para(
	struct msg_req_ipp_path_t *ipp_path_req,
	struct cfg_enh_arf_t *enh_arf_cfg,
	struct cmdlst_para_t *cmdlst_para,
	struct global_info_ipp_t *global_info)
{
	struct seg_enh_arf_stripe_t stripe_info = {0};
	enum frame_flag_e frame_flag = FRAME_CUR;
	logd("+");
	loge_if_ret(seg_ipp_path_arf_cal_stripe_info(ipp_path_req, cmdlst_para, &stripe_info, global_info));
	loge_if_ret(df_sched_prepare(cmdlst_para));
	loge_if_ret(seg_ipp_path_arf_cmdlst_para(ipp_path_req, cmdlst_para, enh_arf_cfg, &stripe_info, global_info,
				frame_flag));

	if (global_info->matcher_en == 1 && ipp_path_req->work_frame == CUR_REF) {
		frame_flag = FRAME_REF;
		loge_if_ret(seg_ipp_path_arf_cmdlst_para(ipp_path_req, cmdlst_para, enh_arf_cfg, &stripe_info, global_info,
					frame_flag));
	}

	logd("-");
	return ISP_IPP_OK;
}

static int seg_ipp_path_arf_get_cpe_mem(struct cfg_enh_arf_t **enh_arf_cfg,
										struct cmdlst_para_t **cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1.get CFG_TAB
	ret = cpe_mem_get(MEM_ID_ARF_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_ARF_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*enh_arf_cfg = (struct cfg_enh_arf_t *)(uintptr_t)va;
	ret = memset_s(*enh_arf_cfg, sizeof(struct cfg_enh_arf_t), 0, sizeof(struct cfg_enh_arf_t));
	if (ret != 0) {
		loge("Failed : memset_s enh_arf_cfg error!");
		goto ipp_path_arf_get_cpe_mem_failed;
	}

	// 2.get CMDLST_PARA
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_ARF, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_ARF);
		goto ipp_path_arf_get_cpe_mem_failed;
	}

	*cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s error!");
		goto ipp_path_arf_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
ipp_path_arf_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_ARF_CFG_TAB);
	return ISP_IPP_ERR;
}

int seg_ipp_path_arf_request_handler(
	struct msg_req_ipp_path_t *ipp_path_req,
	struct global_info_ipp_t *global_info)
{
	int ret = 0;
	struct cfg_enh_arf_t *enh_arf_cfg = NULL;
	struct cmdlst_para_t *cmdlst_para = NULL;
	dataflow_check_para(ipp_path_req);
	dataflow_check_para(global_info);
	logd("+");
	loge_if_ret(seg_ipp_path_arf_get_cpe_mem(&enh_arf_cfg, &cmdlst_para));
	ret = seg_ipp_path_arf_module_config(ipp_path_req, enh_arf_cfg, global_info);
	if (ret != 0) {
		loge(" Failed : seg_ipp_path_arf_module_config");
		goto SEG_IPP_PATH_ARF_BUFF_FERR;
	}

	ret = seg_ipp_path_arf_cfg_cmdlst_para(ipp_path_req, enh_arf_cfg, cmdlst_para, global_info);
	if (ret != 0) {
		loge(" Failed : seg_ipp_path_arf_cfg_cmdlst_para");
		goto SEG_IPP_PATH_ARF_BUFF_FERR;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0)
		loge(" Failed : seg_ipp_path_arf_cfg_cmdlst_para");

	logd("-");
SEG_IPP_PATH_ARF_BUFF_FERR:
	cpe_mem_free(MEM_ID_ARF_CFG_TAB);
	return ret;
}

int get_global_info_ipp(
	struct msg_req_ipp_path_t *ipp_path_request,
	struct global_info_ipp_t *global_info)
{
	unsigned int enh_vpb_value = 0;
	logd("+");

	if (ipp_path_request->work_module == ENH_ARF) {
		global_info->orb_enh_en = 1;
		global_info->arf_en = 1;
	} else if (ipp_path_request->work_module == ARFEATURE_ONLY) {
		global_info->arf_en = 1;
	} else if (ipp_path_request->work_module == ARF_MATCHER) {
		global_info->matcher_en = 1;
		global_info->arf_en = 1;
		global_info->matcher_update_flag = 1;
	} else if (ipp_path_request->work_module == ENH_ARF_MATCHER) {
		global_info->orb_enh_en = 1;
		global_info->arf_en = 1;
		global_info->matcher_en = 1;
		global_info->matcher_update_flag = 1;
	} else if (ipp_path_request->work_module == MATCHER_MC) {
		global_info->matcher_en = 1;
		global_info->mc_en = 1;
	} else if (ipp_path_request->work_module == ARF_MATCHER_MC) {
		global_info->arf_en = 1;
		global_info->mc_en = 1;
		global_info->matcher_en = 1;
		global_info->matcher_update_flag = 1;
	} else if (ipp_path_request->work_module == ENH_ARF_MATCHER_MC) {
		global_info->orb_enh_en = 1;
		global_info->arf_en = 1;
		global_info->matcher_en = 1;
		global_info->mc_en = 1;
		global_info->matcher_update_flag = 1;
	} else if (ipp_path_request->work_module == MATCHER_ONLY) {
		global_info->matcher_en = 1;
	}

	if (global_info->orb_enh_en == 1) {
		if (ipp_path_request->req_arf_cur.req_enh.streams[BO_ENH_OUT].buffer)
			enh_vpb_value = CONNECT_TO_CVDR_AND_ORB;
		else
			enh_vpb_value = CONNECT_TO_ORB;

		hispcpe_reg_set(CPE_TOP, IPP_TOP_ENH_VPB_CFG_REG, enh_vpb_value);
	}

	logd("-");
	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_set_cvdr_cfg(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_cvdr_t *p_cvdr_cfg)
{
	if (mc_request->mc_buff_cfg.ref_first_page &&
		mc_request->mc_buff_cfg.cur_first_page)
		loge_if_ret(cfg_tbl_cvdr_nr_rd_cfg(p_cvdr_cfg, IPP_NR_RD_MC));

	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_set_index_pairs_cfg_tbl(struct msg_req_mc_request_t *req,
		struct cfg_tab_mc_t *mc_cfg_tab)
{
	if (mc_cfg_tab->mc_ctrl_cfg.cfg_mode == CFG_MODE_INDEX_PAIRS) {
		mc_cfg_tab->mc_prefech_cfg.to_use = 1;
		mc_cfg_tab->mc_prefech_cfg.ref_prefetch_enable = 0;
		mc_cfg_tab->mc_prefech_cfg.cur_prefetch_enable = 0;
		mc_cfg_tab->mc_prefech_cfg.ref_first_page =
			req->mc_buff_cfg.ref_first_page >> 12;
		mc_cfg_tab->mc_prefech_cfg.cur_first_page =
			req->mc_buff_cfg.cur_first_page >> 12;
		mc_cfg_tab->mc_index_pairs_cfg.to_use = 1;
		mc_cfg_tab->mc_coord_pairs_cfg.to_use = 0;
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_index_pairs_cfg.cfg_best_dist[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_index_pairs_cfg.cfg_best_dist[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_index_pairs_cfg.cfg_ref_index[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_index_pairs_cfg.cfg_ref_index[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_index_pairs_cfg.cfg_cur_index[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_index_pairs_cfg.cfg_cur_index[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_set_coord_pairs_cfg_tbl(struct msg_req_mc_request_t *req,
		struct cfg_tab_mc_t *mc_cfg_tab)
{
	if (mc_cfg_tab->mc_ctrl_cfg.cfg_mode == CFG_MODE_COOR_PAIRS) {
		mc_cfg_tab->mc_prefech_cfg.to_use = 0;
		mc_cfg_tab->mc_index_pairs_cfg.to_use = 0;
		mc_cfg_tab->mc_coord_pairs_cfg.to_use = 1;
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_coord_pairs_cfg.cur_cfg_coordinate_y[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_y[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_coord_pairs_cfg.cur_cfg_coordinate_x[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_coord_pairs_cfg.cur_cfg_coordinate_x[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_coord_pairs_cfg.ref_cfg_coordinate_y[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_coord_pairs_cfg.ref_cfg_coordinate_y[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
		loge_if(memcpy_s(
					&mc_cfg_tab->mc_coord_pairs_cfg.ref_cfg_coordinate_x[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int),
					&req->mc_hw_config.mc_coord_pairs_cfg.ref_cfg_coordinate_x[0],
					MC_KPT_NUM_MAX * sizeof(unsigned int)));
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_set_cfg_tab(struct msg_req_mc_request_t *req,
									   struct cfg_tab_mc_t *mc_cfg_tab, unsigned int mc_update_flag)
{
	mc_cfg_tab->to_use = 1;
	mc_cfg_tab->mc_ctrl_cfg.to_use = 1;
	mc_cfg_tab->mc_ctrl_cfg.max_iterations =
		req->mc_hw_config.mc_ctrl_cfg.max_iterations;
	mc_cfg_tab->mc_ctrl_cfg.svd_iterations =
		req->mc_hw_config.mc_ctrl_cfg.svd_iterations;
	mc_cfg_tab->mc_ctrl_cfg.flag_10_11 =
		req->mc_hw_config.mc_ctrl_cfg.flag_10_11;
	mc_cfg_tab->mc_ctrl_cfg.push_inliers_en =
		req->mc_hw_config.mc_ctrl_cfg.push_inliers_en;
	mc_cfg_tab->mc_ctrl_cfg.h_cal_en =
		req->mc_hw_config.mc_ctrl_cfg.h_cal_en;
	mc_cfg_tab->mc_ctrl_cfg.cfg_mode =
		req->mc_hw_config.mc_ctrl_cfg.cfg_mode;
	mc_cfg_tab->mc_ctrl_cfg.mc_en =
		req->mc_hw_config.mc_ctrl_cfg.mc_en;
	mc_cfg_tab->mc_inlier_thld_cfg.to_use = 1;
	mc_cfg_tab->mc_inlier_thld_cfg.inlier_th =
		req->mc_hw_config.mc_inlier_thld_cfg.inlier_th;
	mc_cfg_tab->mc_kpt_num_cfg.to_use = 1;
	mc_cfg_tab->mc_kpt_num_cfg.matched_kpts =
		req->mc_hw_config.mc_kpt_num_cfg.matched_kpts;
	mc_cfg_tab->mc_cascade_en = (mc_update_flag == 0) ?
								CASCADE_DISABLE : CASCADE_ENABLE;
	seg_ipp_path_mc_set_index_pairs_cfg_tbl(req, mc_cfg_tab);
	seg_ipp_path_mc_set_coord_pairs_cfg_tbl(req, mc_cfg_tab);
	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_module_config(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_mc_t *mc_cfg_tab,
	struct cfg_tab_cvdr_t *mc_cvdr_cfg_tab,
	unsigned int mc_update_flag)
{
	int ret = 0;
	ret = seg_ipp_path_mc_set_cfg_tab(mc_request, &mc_cfg_tab[0], mc_update_flag);
	if (ret != 0) {
		loge("Failed : set_ipp_cfg_mc");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_path_mc_set_cvdr_cfg(mc_request, &mc_cvdr_cfg_tab[0]);
	if (ret != 0) {
		loge("Failed : mc_update_cvdr_cfg_tab");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_config_cmdlst_buf(struct msg_req_mc_request_t *mc_request,
		struct cfg_tab_mc_t *mc_cfg_tab,
		struct cfg_tab_cvdr_t *cvdr_cfg_tab,
		struct cmdlst_para_t *cmdlst_para,
		struct global_info_ipp_t *global_info)
{
	unsigned int i = 0;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	struct schedule_cmdlst_link_t *cmd_link_entry = cmdlst_para->cmd_entry;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.read_mode = 0;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i].cmd_buf, &cvdr_cfg_tab[0]));
		loge_if_ret(mc_prepare_cmd(&g_mc_devs[0], &cmd_link_entry[i].cmd_buf, &mc_cfg_tab[0]));

		if (global_info->mc_en == 1) {
			global_info->mc_matched_num_addr = mc_cfg_tab[0].mc_match_points_addr;
			global_info->mc_index_addr = mc_cfg_tab[0].mc_index_addr;
		}

		loge_if_ret(mc_enable_cmd(&g_mc_devs[0], &cmd_link_entry[i++].cmd_buf, &mc_cfg_tab[0]));

		if (mc_request->mc_buff_cfg.inlier_num_buff) {
			cmdlst_wr_cfg.is_incr = 0;
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = mc_request->mc_buff_cfg.inlier_num_buff;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_MC + MC_INLIER_NUMBER_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (mc_request->mc_buff_cfg.h_matrix_buff) {
			cmdlst_wr_cfg.is_incr = 1;
			cmdlst_wr_cfg.data_size = MC_H_MATRIX_NUM;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = mc_request->mc_buff_cfg.h_matrix_buff;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_MC + MC_H_MATRIX_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (mc_request->mc_buff_cfg.coordinate_pairs_buff) {
			cmdlst_wr_cfg.is_incr = 0;
			cmdlst_wr_cfg.data_size = MC_COORDINATE_PAIRS;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr = mc_request->mc_buff_cfg.coordinate_pairs_buff;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_MC + MC_COORDINATE_PAIRS_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_cfg_cmdlst(
	struct msg_req_mc_request_t *mc_request,
	struct cfg_tab_mc_t *mc_cfg_tab,
	struct cfg_tab_cvdr_t *cvdr_cfg_tab,
	struct cmdlst_para_t *cmdlst_para,
	struct global_info_ipp_t *global_info)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned int stripe_cnt = 1;
	unsigned long long irq = 0;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.read_mode = 0;
	stripe_cnt = (mc_request->mc_buff_cfg.inlier_num_buff == 0) ?
				 (stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (mc_request->mc_buff_cfg.h_matrix_buff == 0) ?
				 (stripe_cnt) : (stripe_cnt + 1);
	stripe_cnt = (mc_request->mc_buff_cfg.coordinate_pairs_buff == 0) ?
				 (stripe_cnt) : (stripe_cnt + 1);
	global_info->mc_token_num = stripe_cnt;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i != 0) {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) <<
						 IPP_MC_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) <<
						 IPP_MC_IRQ_DONE);
		}

		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_MC;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_MC_CHANNEL_ID;
	cmdlst_para->stripe_cnt = stripe_cnt;
	loge_if_ret(df_sched_prepare(cmdlst_para));
	seg_ipp_path_mc_config_cmdlst_buf(mc_request, mc_cfg_tab, cvdr_cfg_tab, cmdlst_para, global_info);
	return ISP_IPP_OK;
}

static int seg_ipp_path_mc_get_cpe_mem(struct seg_mc_cfg_t **seg_mc_cfg, struct cmdlst_para_t **cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1. mc cfg_tbl
	ret = cpe_mem_get(MEM_ID_CFG_TAB_MC, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CFG_TAB_MC);
		return ISP_IPP_ERR;
	}

	*seg_mc_cfg = (struct seg_mc_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_mc_cfg, sizeof(struct seg_mc_cfg_t), 0, sizeof(struct seg_mc_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_mc_cfg error!");
		goto ipp_path_mc_get_cpe_mem_failed;
	}

	// 2. mc cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_MC, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_MC);
		goto ipp_path_mc_get_cpe_mem_failed;
	}

	*cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s cmdlst_para error!");
		goto ipp_path_mc_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
ipp_path_mc_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_CFG_TAB_MC);
	return ISP_IPP_ERR;
}

int seg_ipp_path_mc_request_handler(
	struct msg_req_mc_request_t *mc_request,
	struct global_info_ipp_t *global_info)
{
	unsigned int mc_mode = CFG_MODE_INDEX_PAIRS;
	int ret = 0;
	struct seg_mc_cfg_t *seg_mc_cfg = NULL;
	struct cmdlst_para_t *cmdlst_para = NULL;
	dataflow_check_para(mc_request);
	dataflow_check_para(global_info);
	logd("+");
	mc_mode = mc_request->mc_hw_config.mc_ctrl_cfg.cfg_mode;

	if ((mc_mode == CFG_MODE_INDEX_PAIRS) &&
		(mc_request->mc_buff_cfg.cur_first_page == 0
		 || mc_request->mc_buff_cfg.ref_first_page == 0)) {
		loge(" Failed : mc_addr is incorrect");
		return ISP_IPP_ERR;
	}

	hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_MC_CHANNEL_ID * 0x80,
					(global_info->mc_en << ACTIVE_TOKEN_NBR_EN_SHIFT) | NRT_CHANNEL);
	loge_if_ret(seg_ipp_path_mc_get_cpe_mem(&seg_mc_cfg, &cmdlst_para));
	ret = seg_ipp_path_mc_module_config(mc_request, &(seg_mc_cfg->mc_cfg_tab),
										&(seg_mc_cfg->mc_cvdr_cfg_tab), global_info->mc_en);
	if (ret != 0) {
		loge("Failed : seg_ipp_path_mc_module_config");
		goto SEG_IPP_PATH_MC_BUFF_FERR;
	}

	ret = seg_ipp_path_mc_cfg_cmdlst(mc_request, &(seg_mc_cfg->mc_cfg_tab),
									 &(seg_mc_cfg->mc_cvdr_cfg_tab), cmdlst_para, global_info);
	if (ret != 0) {
		loge("Failed : seg_ipp_path_mc_cfg_cmdlst");
		goto SEG_IPP_PATH_MC_BUFF_FERR;
	}

	ret = df_sched_start(cmdlst_para);
	if (ret != 0)
		loge("Failed : df_sched_start");

	logd("-");
SEG_IPP_PATH_MC_BUFF_FERR:
	cpe_mem_free(MEM_ID_CFG_TAB_MC);
	return ret;
}

static int seg_ipp_path_cmp_set_cvdr_cfg(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_cvdr_t *compare_cfg_tab, unsigned int layer_num)
{
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
								(align_up(CMP_IN_INDEX_NUM,
										  CVDR_ALIGN_BYTES)), stride);
	}

	if (req_cmp->streams[layer_num][BI_CMP_CUR_FP].buffer)
		cfg_tbl_cvdr_nr_rd_cfg(compare_cfg_tab, IPP_NR_RD_CMP);

	return 0;
}

static void seg_ipp_path_cmp_set_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_compare_t *compare_cfg_tab,
	unsigned int layer_num,
	unsigned int cmp_update_flag)
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
	compare_cfg_tab->compare_search_cfg.dis_ratio = req_cmp->search_cfg[layer_num].dis_ratio;
	compare_cfg_tab->compare_search_cfg.dis_threshold = req_cmp->search_cfg[layer_num].dis_threshold;
	compare_cfg_tab->compare_offset_cfg.cenh_offset = req_cmp->offset_cfg[layer_num].cenh_offset;
	compare_cfg_tab->compare_offset_cfg.cenv_offset = req_cmp->offset_cfg[layer_num].cenv_offset;
	compare_cfg_tab->compare_stat_cfg.stat_en = req_cmp->stat_cfg[layer_num].stat_en;
	compare_cfg_tab->compare_stat_cfg.max3_ratio = req_cmp->stat_cfg[layer_num].max3_ratio;
	compare_cfg_tab->compare_stat_cfg.bin_factor = req_cmp->stat_cfg[layer_num].bin_factor;
	compare_cfg_tab->compare_stat_cfg.bin_num = req_cmp->stat_cfg[layer_num].bin_num;
	compare_cfg_tab->compare_prefetch_cfg.prefetch_enable = 0;
	compare_cfg_tab->compare_prefetch_cfg.first_4k_page = req_cmp->streams[layer_num][BI_CMP_CUR_FP].buffer >> 12;
	compare_cfg_tab->cmp_cascade_en = (cmp_update_flag == 0) ? CASCADE_DISABLE : CASCADE_ENABLE;

	for (i = 0; i < CMP_KPT_NUM_RANGE_DS4; i++) {
		compare_cfg_tab->compare_kptnum_cfg.compare_ref_kpt_num[i] =
			req_cmp->kptnum_cfg[layer_num].compare_ref_kpt_num[i];
		compare_cfg_tab->compare_kptnum_cfg.compare_cur_kpt_num[i] =
			req_cmp->kptnum_cfg[layer_num].compare_cur_kpt_num[i];
	}
}

static int seg_ipp_path_cmp_cfg_cmdlst(
	struct msg_req_matcher_t *matcher_request,
	struct cmdlst_para_t *cmp_cmdlst_para,
	struct global_info_ipp_t *global_info)
{
	unsigned int mc_token_num = global_info->mc_token_num;
	unsigned int stripe_cnt = 0;
	unsigned int i = 0;
	unsigned long long irq = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe = NULL;

	if (matcher_request->cmp_pyramid_layer == 0) {
		loge("matcher_request->cmp_pyramid_layer = 0");
		return ISP_IPP_OK;
	}

	logd("mc_token_num = %d", mc_token_num);
	global_info->cmp_layer_stripe_cnt = 1;

	if (matcher_request->req_cmp.streams[0][BO_CMP_MATCHED_INDEX_OUT].buffer)
		global_info->cmp_layer_stripe_cnt += 1;

	if (matcher_request->req_cmp.streams[0][BO_CMP_MATCHED_DIST_OUT].buffer)
		global_info->cmp_layer_stripe_cnt += 1;

	stripe_cnt = matcher_request->cmp_pyramid_layer * global_info->cmp_layer_stripe_cnt;
	stripe_cnt = (global_info->mc_en == 0) ? stripe_cnt : (stripe_cnt + 2);
	cmdlst_stripe = cmp_cmdlst_para->cmd_stripe_info;
	cmp_cmdlst_para->stripe_cnt = stripe_cnt;

	for (i = 0; i < stripe_cnt; i++) {
		irq = 0;

		if (i % global_info->cmp_layer_stripe_cnt != 0 || (i != 0 && global_info->mc_en == 1)) {
			cmdlst_stripe[i].hw_priority  = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_WR_EOF_CMDLST);
		} else {
			cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_IRQ_DONE);
			irq = irq | (((unsigned long long)(1)) << IPP_CMP_CVDR_VP_RD_EOF_FP);
		}

		logd("ipp_path_cmp: stripe=%d, irq=0x%llx", i, irq);
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_CMP;
		cmdlst_stripe[i].irq_mode        = irq;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
	}

	cmdlst_stripe[stripe_cnt - 1].ch_link = (global_info->mc_en == 0) ? 0 : IPP_MC_CHANNEL_ID;
	cmdlst_stripe[stripe_cnt - 1].ch_link_act_nbr = (global_info->mc_en == 0) ? 0 : mc_token_num;
	cmdlst_stripe[stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int seg_ipp_path_cmp_update_mc_cmdlst_buf(struct schedule_cmdlst_link_t *cmd_link_entry,
		struct global_info_ipp_t *global_info, unsigned int i)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;
	// updata
	cmdlst_wr_cfg.data_size = 1;
	cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_MATCH_POINTS_REG;
	cmdlst_wr_cfg.is_wstrb = 1;
	cmdlst_wr_cfg.read_mode = 0;
	cmdlst_wr_cfg.buff_wr_addr = global_info->mc_matched_num_addr;
	loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	cmdlst_wr_cfg.data_size = MC_KPT_NUM_MAX;
	cmdlst_wr_cfg.is_wstrb = 1;
	cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_INDEX_0_REG;
	cmdlst_wr_cfg.read_mode = 1;
	cmdlst_wr_cfg.buff_wr_addr = global_info->mc_index_addr;
	loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	return i;
}

static int seg_ipp_path_cmp_set_cmdlst_para(
	struct cmdlst_para_t *cmdlst_para,
	struct msg_req_matcher_t *matcher_request,
	struct seg_matcher_cmp_cfg_t *compare_cfg_tab,
	struct global_info_ipp_t *global_info)
{
	unsigned int i = 0;
	unsigned int cur_layer = 0;
	unsigned int cmp_update_flag =  global_info->matcher_update_flag;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 0;
	cmdlst_wr_cfg.read_mode = 0;
	cmd_link_entry = (struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt;) {
		cur_layer = i / global_info->cmp_layer_stripe_cnt;
		cvdr_prepare_nr_cmd(&g_cvdr_devs[0], &cmd_link_entry[i].cmd_buf, &(compare_cfg_tab->compare_cvdr_cfg_tab[cur_layer]));
		compare_prepare_cmd(&g_compare_devs[0], &cmd_link_entry[i].cmd_buf, &(compare_cfg_tab->compare_cfg_tab[cur_layer]));
		cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i++].cmd_buf, &(compare_cfg_tab->compare_cvdr_cfg_tab[cur_layer]));

		if (cmp_update_flag == 1) {
			global_info->cmp_kpt_num_addr_cur =
				compare_cfg_tab->compare_cfg_tab[cur_layer].cmp_cur_kpt_addr;
			global_info->cmp_kpt_num_addr_ref =
				compare_cfg_tab->compare_cfg_tab[cur_layer].cmp_ref_kpt_addr;
			global_info->cmp_total_num_addr =
				compare_cfg_tab->compare_cfg_tab[cur_layer].cmp_total_kpt_addr;
			global_info->cmp_frame_height_addr =
				compare_cfg_tab->compare_cvdr_cfg_tab[cur_layer].cmp_cvdr_frame_size_addr;
		}

		if (matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_INDEX_OUT].buffer) {
			cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_INDEX_0_REG;
			cmdlst_wr_cfg.buff_wr_addr = matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_INDEX_OUT].buffer;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf_cmp(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg, 0x10));
		}

		if (matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_DIST_OUT].buffer) {
			cmdlst_wr_cfg.data_size = CMP_INDEX_RANGE_MAX;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_COMPARE + COMPARE_DISTANCE_0_REG;
			cmdlst_wr_cfg.buff_wr_addr = matcher_request->req_cmp.streams[cur_layer][BO_CMP_MATCHED_DIST_OUT].buffer;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}

		if (global_info->mc_en == 1) {
			logd("cmp update mc_matched_num_addr and mc_index_addr");
			i = seg_ipp_path_cmp_update_mc_cmdlst_buf(cmd_link_entry, global_info, i);
		}
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_cmp_get_cpe_mem(struct seg_matcher_cmp_cfg_t **compare_cfg_tab,
										struct cmdlst_para_t **compare_cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1. cmp cfg_tbl
	ret = cpe_mem_get(MEM_ID_COMPARE_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge(" Failed : cpe_mem_get %d", MEM_ID_COMPARE_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*compare_cfg_tab = (struct seg_matcher_cmp_cfg_t *)(uintptr_t)va;
	ret = memset_s(*compare_cfg_tab, sizeof(struct seg_matcher_cmp_cfg_t), 0, sizeof(struct seg_matcher_cmp_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s compare_cfg_tab error!");
		goto ipp_path_cmp_get_cpe_mem_failed;
	}

	// 2. cmp cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_COMPARE, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_COMPARE);
		goto ipp_path_cmp_get_cpe_mem_failed;
	}

	*compare_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*compare_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s compare_cmdlst_para error!");
		goto ipp_path_cmp_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
ipp_path_cmp_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	return ISP_IPP_ERR;
}

int seg_ipp_path_cmp_request_handler(
	struct msg_req_matcher_t *matcher_request,
	struct global_info_ipp_t *global_info)
{
	int ret = 0;
	unsigned int i = 0;
	struct seg_matcher_cmp_cfg_t *compare_cfg_tab = NULL;
	struct cmdlst_para_t *compare_cmdlst_para = NULL;
	logd("+");

	if (matcher_request->cmp_pyramid_layer == 0)
		return ISP_IPP_OK;

	loge_if_ret(seg_ipp_path_cmp_get_cpe_mem(&compare_cfg_tab, &compare_cmdlst_para));
	compare_cmdlst_para->channel_id = IPP_CMP_CHANNEL_ID;

	for (i = 0; i < matcher_request->cmp_pyramid_layer; i++) {
		seg_ipp_path_cmp_set_cfg_tab(matcher_request, &(compare_cfg_tab->compare_cfg_tab[i]),
									 i, global_info->matcher_update_flag);
		loge_if(seg_ipp_path_cmp_set_cvdr_cfg(matcher_request,
											  &(compare_cfg_tab->compare_cvdr_cfg_tab[i]), i));
	}

	loge_if(seg_ipp_path_cmp_cfg_cmdlst(matcher_request, compare_cmdlst_para, global_info));
	ret = df_sched_prepare(compare_cmdlst_para);
	if (ret != 0) {
		loge("Failed : df_sched_prepare");
		goto SEG_IPP_PATH_CMP_BUFF_FERR;
	}

	ret = seg_ipp_path_cmp_set_cmdlst_para(compare_cmdlst_para,
										   matcher_request, compare_cfg_tab, global_info);
	if (ret != 0) {
		loge("Failed : seg_ipp_path_cmp_set_cmdlst_para");
		goto SEG_IPP_PATH_CMP_BUFF_FERR;
	}

	ret = df_sched_start(compare_cmdlst_para);
	if (ret != 0)
		loge("Failed : df_sched_start");

SEG_IPP_PATH_CMP_BUFF_FERR:
	cpe_mem_free(MEM_ID_COMPARE_CFG_TAB);
	logd("-");
	return ret;
}

static int seg_ipp_path_rdr_set_cvdr_cfg(
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
		cfmt.expand = 0;
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

static void seg_ipp_path_rdr_set_cfg_tab(
	struct msg_req_matcher_t *matcher_request,
	struct cfg_tab_reorder_t *reorder_cfg_tab,
	unsigned int  layer_num,
	unsigned int  rdr_update_flag)
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
	reorder_cfg_tab->rdr_cascade_en = (rdr_update_flag == 0) ?
									  CASCADE_DISABLE : CASCADE_ENABLE;
	loge_if(memcpy_s(
				&(reorder_cfg_tab->reorder_kptnum_cfg.reorder_kpt_num[0]),
				RDR_KPT_NUM_RANGE_DS4 * sizeof(unsigned int),
				&(req_rdr->reorder_kpt_num[layer_num][0]),
				RDR_KPT_NUM_RANGE_DS4 * sizeof(unsigned int)));
}

static int seg_ipp_path_rdr_cfg_cmdlst(
	struct msg_req_matcher_t *matcher_request,
	struct cmdlst_para_t *rdr_cmdlst_para,
	struct global_info_ipp_t *global_info)
{
	unsigned int i = 0;
	unsigned long long irq = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe = rdr_cmdlst_para->cmd_stripe_info;
	rdr_cmdlst_para->stripe_cnt = matcher_request->rdr_pyramid_layer * STRIPE_NUM_EACH_RDR;

	for (i = 0; i < rdr_cmdlst_para->stripe_cnt; i++) {
		irq = 0;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_RDR;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].is_need_set_sop = 0;
		irq = irq | (((unsigned long long)(1)) << IPP_RDR_CVDR_VP_RD_EOF_CMDSLT);
		irq = irq | (((unsigned long long)(1)) << IPP_RDR_IRQ_DONE);
		irq = irq | (((unsigned long long)(1)) << IPP_RDR_CVDR_VP_RD_EOF_FP);
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		cmdlst_stripe[i].irq_mode        = irq;

		if ((matcher_request->work_mode & (1 << 1)) == 0) {
			// 2rdr + 1cmp case,first rdr not release token to cmp
			cmdlst_stripe[i].en_link = 1;

			if (i > 0) {
				cmdlst_stripe[i].ch_link = IPP_CMP_CHANNEL_ID;
				cmdlst_stripe[i].ch_link_act_nbr = (global_info->mc_en == 0) ?
												   global_info->cmp_layer_stripe_cnt : global_info->cmp_layer_stripe_cnt + 2;
			}
		} else {
			// 1rdr + 7cmp case, release all token to cmp
			cmdlst_stripe[i].en_link = 1;
			cmdlst_stripe[i].ch_link = IPP_CMP_CHANNEL_ID;
			cmdlst_stripe[i].ch_link_act_nbr = (global_info->mc_en == 0) ?
											   matcher_request->cmp_pyramid_layer * global_info->cmp_layer_stripe_cnt :
											   global_info->cmp_layer_stripe_cnt + 2;
		}
	}

	cmdlst_stripe[rdr_cmdlst_para->stripe_cnt - 1].is_last_stripe  = 1;
	return ISP_IPP_OK;
}

static int seg_ipp_path_rdr_cfg_cmdlst_para(
	struct msg_req_matcher_t *matcher_request,
	struct seg_matcher_rdr_cfg_t *seg_reorder_cfg,
	struct cmdlst_para_t *reorder_cmdlst_para,
	struct global_info_ipp_t *global_info)
{
	unsigned int i = 0;
	struct schedule_cmdlst_link_t *rdr_cmd_link_entry = NULL;
	loge_if_ret(seg_ipp_path_rdr_cfg_cmdlst(matcher_request, reorder_cmdlst_para, global_info));
	loge_if_ret(df_sched_prepare(reorder_cmdlst_para));
	rdr_cmd_link_entry = (struct schedule_cmdlst_link_t *)reorder_cmdlst_para->cmd_entry;

	for (i = 0; i < reorder_cmdlst_para->stripe_cnt; i++) {
		loge_if_ret(cvdr_prepare_nr_cmd(&g_cvdr_devs[0], &(rdr_cmd_link_entry[i].cmd_buf),
										&(seg_reorder_cfg->reorder_cvdr_cfg_tab[i])));
		loge_if_ret(reorder_prepare_cmd(&g_reorder_devs[0], &(rdr_cmd_link_entry[i].cmd_buf),
										&(seg_reorder_cfg->reorder_cfg_tab[i])));
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0], &(rdr_cmd_link_entry[i].cmd_buf),
									 &(seg_reorder_cfg->reorder_cvdr_cfg_tab[i])));

		if (global_info->matcher_update_flag == 1 && i == 0) {
			global_info->rdr_kpt_num_addr_cur = seg_reorder_cfg->reorder_cfg_tab[i].rdr_kpt_num_addr;
			global_info->rdr_total_num_addr_cur = seg_reorder_cfg->reorder_cfg_tab[i].rdr_total_num_addr;
			global_info->rdr_frame_height_addr_cur = seg_reorder_cfg->reorder_cvdr_cfg_tab[i].rdr_cvdr_frame_size_addr;
		} else if (global_info->matcher_update_flag == 1 && i == 1) {
			global_info->rdr_kpt_num_addr_ref = seg_reorder_cfg->reorder_cfg_tab[i].rdr_kpt_num_addr;
			global_info->rdr_total_num_addr_ref = seg_reorder_cfg->reorder_cfg_tab[i].rdr_total_num_addr;
			global_info->rdr_frame_height_addr_ref = seg_reorder_cfg->reorder_cvdr_cfg_tab[i].rdr_cvdr_frame_size_addr;
		}
	}

	return ISP_IPP_OK;
}

static int seg_ipp_path_rdr_get_cpe_mem(struct seg_matcher_rdr_cfg_t **seg_reorder_cfg,
										struct cmdlst_para_t **reorder_cmdlst_para)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	// 1.rdr cfg_tbl
	ret = cpe_mem_get(MEM_ID_REORDER_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge(" Failed : cpe_mem_get %d", MEM_ID_REORDER_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_reorder_cfg = (struct seg_matcher_rdr_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_reorder_cfg, sizeof(struct seg_matcher_rdr_cfg_t), 0, sizeof(struct seg_matcher_rdr_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s seg_reorder_cfg error!");
		goto ipp_path_rdr_get_cpe_mem_failed;
	}

	// 2. rdr cmdlst_para
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_REORDER, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_REORDER);
		goto ipp_path_rdr_get_cpe_mem_failed;
	}

	*reorder_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*reorder_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s reorder_cmdlst_para error!");
		goto ipp_path_rdr_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
ipp_path_rdr_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	return ISP_IPP_ERR;
}

int seg_ipp_path_rdr_request_handler(
	struct msg_req_matcher_t *matcher_request,
	struct global_info_ipp_t *global_info)
{
	int ret = 0;
	unsigned int i = 0;
	struct seg_matcher_rdr_cfg_t *seg_reorder_cfg = NULL;
	struct cmdlst_para_t *reorder_cmdlst_para = NULL;
	unsigned int rdr_update_flag = global_info->matcher_update_flag;
	logd("+");

	if (matcher_request->rdr_pyramid_layer == 0)
		return ISP_IPP_OK;

	loge_if_ret(seg_ipp_path_rdr_get_cpe_mem(&seg_reorder_cfg, &reorder_cmdlst_para));
	reorder_cmdlst_para->channel_id = IPP_RDR_CHANNEL_ID;
	reorder_cmdlst_para->stripe_cnt = matcher_request->rdr_pyramid_layer;

	for (i = 0; i < reorder_cmdlst_para->stripe_cnt; i++) {
		seg_ipp_path_rdr_set_cfg_tab(matcher_request, &(seg_reorder_cfg->reorder_cfg_tab[i]), i, rdr_update_flag);
		loge_if(seg_ipp_path_rdr_set_cvdr_cfg(matcher_request, &(seg_reorder_cfg->reorder_cvdr_cfg_tab[i]), i));
	}

	ret = seg_ipp_path_rdr_cfg_cmdlst_para(matcher_request, seg_reorder_cfg, reorder_cmdlst_para, global_info);
	if (ret != 0) {
		loge("Failed : seg_ipp_path_rdr_cfg_cmdlst_para");
		goto SEG_IPP_PATH_RDR_BUFF_FERR;
	}

	ret = df_sched_start(reorder_cmdlst_para);
	if (ret != 0)
		loge("Failed : df_sched_start");

SEG_IPP_PATH_RDR_BUFF_FERR:
	cpe_mem_free(MEM_ID_REORDER_CFG_TAB);
	logd("-");
	return ret;
}

static int seg_ipp_path_matcher_only_mode_check_para(
	struct msg_req_matcher_t *matcher_req)
{
	unsigned int i = 0;

	if (matcher_req->cmp_pyramid_layer > MATCHER_LAYER_MAX
		|| matcher_req->rdr_pyramid_layer > MATCHER_LAYER_MAX ||
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

static int ipp_path_check_arf_para(struct msg_req_ipp_path_t *req)
{
	unsigned int i = 0;

	if ((req->req_arf_cur.arfeature_stat_buff[DESC_BUFF] == 0 && req->work_frame == CUR_ONLY) ||
		((req->req_arf_ref.arfeature_stat_buff[DESC_BUFF] == 0 ||
		  req->req_arf_cur.arfeature_stat_buff[DESC_BUFF] == 0) && req->work_frame == CUR_REF)) {
		loge(" Failed : arfeature DESC_BUFF is NULL");
		return ISP_IPP_ERR;
	}

	if (req->work_module == ARFEATURE_ONLY || req->work_module == ARF_MATCHER || req->work_module == ARF_MATCHER_MC) {
		for (i = 0; i < req->mode_cnt; i++)
			if ((req->req_arf_cur.streams[i][BI_ARFEATURE_0].buffer == 0)
				|| (req->req_arf_ref.streams[i][BI_ARFEATURE_0].buffer == 0 && req->work_frame == CUR_REF)) {
				loge("Failed : stream %d BI_ARFEATURE_0 is zero", i);
				return ISP_IPP_ERR;
			}
	}

	return ISP_IPP_OK;
}

static int ipp_path_check_request_para(struct msg_req_ipp_path_t *req)
{
	unsigned int i = 0;

	if ((req->mode_cnt > ARF_AR_MODE_CNT) || (req->mode_cnt < ARF_DMAP_MODE_CNT)) {
		loge(" Failed : mode_cnt %d is error", req->mode_cnt);
		return ISP_IPP_ERR;
	}

	loge_if_ret(ipp_path_check_arf_para(req));

	if (req->work_module == ENH_ARF || req->work_module == ENH_ARF_MATCHER || req->work_module == ENH_ARF_MATCHER_MC) {
		req->req_arf_cur.streams[0][BI_ARFEATURE_0].buffer = 0;
		req->req_arf_ref.streams[0][BI_ARFEATURE_0].buffer = 0;

		if ((req->req_arf_cur.req_enh.streams[BI_ENH_YHIST].buffer == 0
			 || req->req_arf_cur.req_enh.streams[BI_ENH_YIMAGE].buffer == 0)
			|| ((req->req_arf_ref.req_enh.streams[BI_ENH_YHIST].buffer == 0
				 || req->req_arf_ref.req_enh.streams[BI_ENH_YHIST].buffer == 0) && req->work_frame == CUR_REF)) {
			loge("Failed : BI_ENH_YHIST or BI_ENH_YIMAGE is zero");
			return ISP_IPP_ERR;
		}

		for (i = 1; i < req->mode_cnt; i++)
			if ((req->req_arf_cur.streams[i][BI_ARFEATURE_0].buffer == 0) ||
				(req->req_arf_ref.streams[i][BI_ARFEATURE_0].buffer == 0 && req->work_frame == CUR_REF)) {
				loge("Failed : BI_ARFEATURE_0 is zero");
				return ISP_IPP_ERR;
			}
	}

	return ISP_IPP_OK;
}

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
static void ipp_path_arf_reg_cfg_dump(arfeature_reg_cfg_t *reg_cfg)
{
	logi("ctrl:mode = %d", reg_cfg->ctrl.mode);
	logi("ctrl:orient_en = %d", reg_cfg->ctrl.orient_en);
	logi("ctrl:layer = %d", reg_cfg->ctrl.layer);
	logi("ctrl:iter_num = %d", reg_cfg->ctrl.iter_num);
	logi("ctrl:first_detect = %d", reg_cfg->ctrl.first_detect);
	logi("size_cfg:width = %d, height = %d", reg_cfg->size_cfg.width, reg_cfg->size_cfg.height);
	logi("blk_cfg:blk_h_num = %d, blk_v_num = %d",
		 reg_cfg->blk_cfg.blk_h_num, reg_cfg->blk_cfg.blk_v_num);
	logi("blk_cfg:blk_h_size_inv = %d, blk_v_size_inv = %d",
		 reg_cfg->blk_cfg.blk_h_size_inv, reg_cfg->blk_cfg.blk_v_size_inv);
	logi("detect_num_lmt:cur_kpnum = %d, max_kpnum = %d",
		 reg_cfg->detect_num_lmt.cur_kpnum, reg_cfg->detect_num_lmt.max_kpnum);
	logi("sigma_coef:sigma_des = %d, sigma_ori = %d",
		 reg_cfg->sigma_coef.sigma_des, reg_cfg->sigma_coef.sigma_ori);
	logi("dog_edge_thd:dog_threshold = %d, edge_threshold = %d",
		 reg_cfg->dog_edge_thd.dog_threshold, reg_cfg->dog_edge_thd.edge_threshold);
	logi("req_descriptor_thd:mag_threshold = %d",
		 reg_cfg->req_descriptor_thd.mag_threshold);
	logi("kpt_lmt:grid_dog_threshold[0] = %d, grid_max_kpnum[0] = %d",
		 reg_cfg->kpt_lmt.grid_dog_threshold[0], reg_cfg->kpt_lmt.grid_max_kpnum[0]);
	logi("gauss_org_0~3:%d, %d, %d, %d",
		 reg_cfg->gauss_coef.gauss_org_0, reg_cfg->gauss_coef.gauss_org_1,
		 reg_cfg->gauss_coef.gauss_org_2, reg_cfg->gauss_coef.gauss_org_3);
	logi("gauss_1st_0~2:%d, %d, %d",
		 reg_cfg->gauss_coef.gsblur_1st_0, reg_cfg->gauss_coef.gsblur_1st_1,
		 reg_cfg->gauss_coef.gsblur_1st_2);
	logi("gauss_2nd_0~3:%d, %d, %d, %d",
		 reg_cfg->gauss_coef.gauss_2nd_0, reg_cfg->gauss_coef.gauss_2nd_1,
		 reg_cfg->gauss_coef.gauss_2nd_2, reg_cfg->gauss_coef.gauss_2nd_3);
}

static void ipp_path_arf_stream_dump(struct msg_req_ipp_path_t *req)
{
	unsigned int i = 0;
	unsigned int j = 0;
	char *stream_name[] = {
		"BI_ARFEATURE_0",
		"BI_ARFEATURE_1",
		"BI_ARFEATURE_2",
		"BI_ARFEATURE_3",
		"BO_ARFEATURE_PYR_1",
		"BO_ARFEATURE_PYR_2",
		"BO_ARFEATURE_DOG_0",
		"BO_ARFEATURE_DOG_1",
		"BO_ARFEATURE_DOG_2",
		"BO_ARFEATURE_DOG_3",
	};
	char *arfeature_stat_buff_name[] = {
		"DESC_BUFF",
		"MINSCR_KPTCNT_BUFF",
		"SUM_SCORE_BUFF",
		"KPT_NUM_BUFF",
		"TOTAL_KPT_BUFF",
	};

	for (i = 0; i < req->mode_cnt; i++) {
		logi("arfeature cur frame mode_cnt %d stream dump:", i);
		for (j=0; j < ARFEATURE_STREAM_MAX; j++)
		{
			if ((req->req_arf_cur.streams[i][j].width != 0) && (req->req_arf_cur.streams[i][j].height != 0))
			{
				logi("streams[%d][%s].width = %d", i, stream_name[j], req->req_arf_cur.streams[i][j].width);
				logi("streams[%d][%s].height = %d", i, stream_name[j], req->req_arf_cur.streams[i][j].height);
				logi("streams[%d][%s].stride = %d", i, stream_name[j], req->req_arf_cur.streams[i][j].stride);
				logi("streams[%d][%s].format = %d", i, stream_name[j], req->req_arf_cur.streams[i][j].format);
				logi("streams[%d][%s].buf = 0x%x", i, stream_name[j], req->req_arf_cur.streams[i][j].buffer);
			}
		}

		logi("arfeature cur frame mode_cnt %d cfg dump:", i);
		ipp_path_arf_reg_cfg_dump(&req->req_arf_cur.reg_cfg[i]);
	}

	for (i = 0; i < STAT_BUFF_MAX; i++) {
		if (req->req_arf_cur.arfeature_stat_buff[i] != 0)
			logi("cur frame arfeature_stat_buff[%s] = 0x%x",
				arfeature_stat_buff_name[i], req->req_arf_cur.arfeature_stat_buff[i]);
	}

	for (i = 0; i < req->mode_cnt; i++) {
		logi("arfeature ref frame mode_cnt %d stream dump:", i);
		for (j=0; j < ARFEATURE_STREAM_MAX; j++)
		{
			if ((req->req_arf_ref.streams[i][j].width != 0) && (req->req_arf_ref.streams[i][j].height != 0))
			{
				logi("streams[%d][%s].width = %d", i, stream_name[j], req->req_arf_ref.streams[i][j].width);
				logi("streams[%d][%s].height = %d", i, stream_name[j], req->req_arf_ref.streams[i][j].height);
				logi("streams[%d][%s].stride = %d", i, stream_name[j], req->req_arf_ref.streams[i][j].stride);
				logi("streams[%d][%s].format = %d", i, stream_name[j], req->req_arf_ref.streams[i][j].format);
				logi("streams[%d][%s].buf = 0x%x", i, stream_name[j], req->req_arf_ref.streams[i][j].buffer);
			}
		}
		logi("arfeature ref frame mode_cnt %d cfg dump:", i);
		ipp_path_arf_reg_cfg_dump(&req->req_arf_ref.reg_cfg[i]);
	}

	for (i = 0; i < STAT_BUFF_MAX; i++) {
		if (req->req_arf_ref.arfeature_stat_buff[i] != 0)
			logi("ref frame arfeature_stat_buff[%s] = 0x%x",
				arfeature_stat_buff_name[i], req->req_arf_ref.arfeature_stat_buff[i]);
	}
}
#endif

static void ipp_path_request_dump(struct msg_req_ipp_path_t *req)
{
	char *work_mod[] = {
		"ENH_ONLY",
		"ARFEATURE_ONLY",
		"MATCHER_ONLY",
		"MC_ONLY",
		"ENH_ARF",
		"ARF_MATCHER",
		"ENH_ARF_MATCHER",
		"MATCHER_MC",
		"ARF_MATCHER_MC",
		"ENH_ARF_MATCHER_MC",
		"OPTICAL_FLOW",
	};

	char *work_fra[] = {
		"CUR_ONLY",
		"CUR_REF",
	};

	logi("ipp_path_req: frame_num = %d, mode_cnt = %d, work_module = %s, work_frame = %s",
			req->frame_num, req->mode_cnt, work_mod[req->work_module], work_fra[req->work_frame]);
	logi("ipp_path_req: secure_flag = %d, arfeature_rate_value = %d",
		 req->secure_flag, req->arfeature_rate_value);
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	orb_enh_dump_request(&(req->req_arf_cur.req_enh));
	ipp_path_arf_stream_dump(req);
	rdr_request_dump(&(req->req_matcher));
	cmp_request_dump(&(req->req_matcher));
	mc_dump_request(&(req->req_mc));
#endif
	return;
}

int ipp_path_request_handler(struct msg_req_ipp_path_t *ipp_path_request)
{
	int ret = 0;
	struct global_info_ipp_t global_info = {0};

	if (ipp_path_request == NULL) {
		loge(" Failed : ipp_path_request is null");
		return ISP_IPP_ERR;
	}

	if (ipp_path_request->work_module == MATCHER_ONLY) {
		loge_if_ret(seg_ipp_path_matcher_only_mode_check_para(&ipp_path_request->req_matcher));
	} else if (ipp_path_request->work_module != MATCHER_MC) {
		loge_if_ret(ipp_path_check_request_para(ipp_path_request));
	}

	ipp_path_request_dump(ipp_path_request);
	loge_if_ret(get_global_info_ipp(ipp_path_request, &global_info));
	logd("global_info: orb_enh_en = %d, arf_en = %d, matcher_en = %d, mc_en = %d",
		 global_info.orb_enh_en, global_info.arf_en,
		 global_info.matcher_en, global_info.mc_en);

	if (global_info.mc_en != 0)
		loge_if_ret(seg_ipp_path_mc_request_handler(&ipp_path_request->req_mc, &global_info));

	if (global_info.matcher_en != 0) {
		unsigned int cmp_token_nbr_en = 0;
		unsigned int cmdlst_channel_value = 0;
		unsigned int rdr_token_nbr_en = global_info.matcher_en;

		if (global_info.arf_en == 0) // for matcher only, matcher_mc work module
			rdr_token_nbr_en = 0;

		cmp_token_nbr_en =	(ipp_path_request->req_matcher.rdr_pyramid_layer == 0) ? (0) : (1);
		logd("rdr_token_nbr_en = %d, cmp_token_nbr_en = %d", rdr_token_nbr_en, cmp_token_nbr_en);
		cmdlst_channel_value = (rdr_token_nbr_en << ACTIVE_TOKEN_NBR_EN_SHIFT) | NRT_CHANNEL;
		hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_RDR_CHANNEL_ID * 0x80, cmdlst_channel_value);
		cmdlst_channel_value = (cmp_token_nbr_en << ACTIVE_TOKEN_NBR_EN_SHIFT) | NRT_CHANNEL;
		hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_CMP_CHANNEL_ID * 0x80, cmdlst_channel_value);
		loge_if_ret(seg_ipp_path_cmp_request_handler(&ipp_path_request->req_matcher, &global_info));
		loge_if_ret(seg_ipp_path_rdr_request_handler(&ipp_path_request->req_matcher, &global_info));
	}

	if (global_info.arf_en != 0)
		loge_if_ret(seg_ipp_path_arf_request_handler(ipp_path_request, &global_info));

	return ret;
}

