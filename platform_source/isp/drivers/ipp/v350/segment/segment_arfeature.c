#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <libhwsecurec/securec.h>
#include "segment_arfeature.h"
#include "cfg_table_arfeature.h"
#include "cvdr_drv.h"
#include "arfeature_drv.h"
#include "cfg_table_cvdr.h"
#include "cvdr_opt.h"
#include "segment_common.h"
#include "memory.h"
#include "arfeature_drv_priv.h"
#include "arfeature_reg_offset.h"
#include "ipp_top_reg_offset.h"
#include "cmdlst_drv.h"

#define LOG_TAG LOG_MODULE_ARFEATURE
#define CMDLST_READ_FLAG  1

unsigned int arf_updata_wr_addr_flag = 0;

static void arfeature_request_dump(struct msg_req_arfeature_request_t *msg);
static int arfeature_check_request_para(struct msg_req_arfeature_request_t *msg);

static int cfg_arf_cvdr_rd_port_0_1(req_arfeature_t *req_arfeature, struct cfg_tab_cvdr_t *cvdr_cfg_tab,
									unsigned int stripe_index)
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

static int cfg_arf_cvdr_rd_port_2_3(req_arfeature_t *req_arfeature, struct cfg_tab_cvdr_t *cvdr_cfg_tab,
									unsigned int stripe_index)
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

static int cfg_arf_cvdr_wr_port_pyr_1_2(req_arfeature_t *req_arfeature, struct cfg_tab_cvdr_t *cvdr_cfg_tab,
										unsigned int stripe_index)
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

static int cfg_arf_cvdr_wr_port_dog_0_1(req_arfeature_t *req_arfeature, struct cfg_tab_cvdr_t *cvdr_cfg_tab,
										unsigned int stripe_index)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int  line_stride = 0;
	loge_if_ret(memset_s(&cfmt, sizeof(struct cvdr_opt_fmt_t), 0, sizeof(struct cvdr_opt_fmt_t)));

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

static int cfg_arf_cvdr_wr_port_dog_2_3(req_arfeature_t *req_arfeature, struct cfg_tab_cvdr_t *cvdr_cfg_tab,
										unsigned int stripe_index)
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

static int arfeature_update_cvdr_cfg_tab(
	struct msg_req_arfeature_request_t *msg,
	struct seg_arfeature_cfg_t *seg_arfeature_cfg,
	unsigned int stripe_index,
	enum frame_flag_e frame_flag)
{
	req_arfeature_t *req_arfeature = NULL;
	struct cfg_tab_cvdr_t *cvdr_cfg_tab = NULL;
	req_arfeature = &(msg->req_arfeature);
	cvdr_cfg_tab = (struct cfg_tab_cvdr_t *)(uintptr_t) & (seg_arfeature_cfg->arfeature_cvdr[stripe_index]);
	loge_if_ret(cfg_arf_cvdr_rd_port_0_1(req_arfeature, cvdr_cfg_tab, stripe_index));
	loge_if_ret(cfg_arf_cvdr_rd_port_2_3(req_arfeature, cvdr_cfg_tab, stripe_index));
	loge_if_ret(cfg_arf_cvdr_wr_port_pyr_1_2(req_arfeature, cvdr_cfg_tab, stripe_index));
	loge_if_ret(cfg_arf_cvdr_wr_port_dog_0_1(req_arfeature, cvdr_cfg_tab, stripe_index));
	loge_if_ret(cfg_arf_cvdr_wr_port_dog_2_3(req_arfeature, cvdr_cfg_tab, stripe_index));
	return ISP_IPP_OK;
}
static int cfg_arf_sigma_coeff_gauss_coeff(struct cfg_tab_arfeature_t *arfeature_cfg_tab,
		req_arfeature_t *req_arfeature, unsigned int stripe_index)
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
	return ISP_IPP_OK;
}

static void arfeature_update_request_cfg_tab(
	struct msg_req_arfeature_request_t *msg,
	struct seg_arfeature_cfg_t *seg_arfeature_cfg,
	unsigned int stripe_index,
	enum frame_flag_e frame_flag)
{
	unsigned int i = 0;
	req_arfeature_t *req_arfeature = NULL;
	struct cfg_tab_arfeature_t *arfeature_cfg_tab = NULL;
	req_arfeature = &(msg->req_arfeature);
	arfeature_cfg_tab = (struct cfg_tab_arfeature_t *)(uintptr_t)(&(seg_arfeature_cfg->arfeature_cfg[stripe_index]));
	// top_cfg
	arfeature_cfg_tab->top_cfg.to_use = 1;
	arfeature_cfg_tab->top_cfg.mode = req_arfeature->reg_cfg[stripe_index].ctrl.mode;
	arfeature_cfg_tab->top_cfg.orient_en = req_arfeature->reg_cfg[stripe_index].ctrl.orient_en;
	arfeature_cfg_tab->top_cfg.layer = req_arfeature->reg_cfg[stripe_index].ctrl.layer;
	arfeature_cfg_tab->top_cfg.iter_num = req_arfeature->reg_cfg[stripe_index].ctrl.iter_num;
	arfeature_cfg_tab->top_cfg.first_detect = req_arfeature->reg_cfg[stripe_index].ctrl.first_detect;
	// size_cfg
	arfeature_cfg_tab->size_cfg.to_use = 1;
	arfeature_cfg_tab->size_cfg.img_width = req_arfeature->reg_cfg[stripe_index].size_cfg.width;
	arfeature_cfg_tab->size_cfg.img_height = req_arfeature->reg_cfg[stripe_index].size_cfg.height;
	// blk_cfg
	arfeature_cfg_tab->blk_cfg.to_use = 1;
	arfeature_cfg_tab->blk_cfg.blk_h_num = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_h_num;
	arfeature_cfg_tab->blk_cfg.blk_v_num = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_v_num;
	arfeature_cfg_tab->blk_cfg.blk_h_size_inv = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_h_size_inv;
	arfeature_cfg_tab->blk_cfg.blk_v_size_inv = req_arfeature->reg_cfg[stripe_index].blk_cfg.blk_v_size_inv;
	// detect_num_lmt_cfg
	arfeature_cfg_tab->detect_num_lmt_cfg.to_use = 1;
	arfeature_cfg_tab->detect_num_lmt_cfg.max_kpnum = req_arfeature->reg_cfg[stripe_index].detect_num_lmt.max_kpnum;
	arfeature_cfg_tab->detect_num_lmt_cfg.cur_kpnum = req_arfeature->reg_cfg[stripe_index].detect_num_lmt.cur_kpnum;
	// sigma_coeff_cfg , gauss_coeff_cfg
	cfg_arf_sigma_coeff_gauss_coeff(arfeature_cfg_tab, req_arfeature, stripe_index);
	// dog_edge_thd_cfg
	arfeature_cfg_tab->dog_edge_thd_cfg.to_use = 1;
	arfeature_cfg_tab->dog_edge_thd_cfg.edge_threshold = req_arfeature->reg_cfg[stripe_index].dog_edge_thd.edge_threshold;
	arfeature_cfg_tab->dog_edge_thd_cfg.dog_threshold = req_arfeature->reg_cfg[stripe_index].dog_edge_thd.dog_threshold;
	// mag_thd_cfg
	arfeature_cfg_tab->mag_thd_cfg.to_use = 1;
	arfeature_cfg_tab->mag_thd_cfg.mag_threshold = req_arfeature->reg_cfg[stripe_index].req_descriptor_thd.mag_threshold;
	// kpt_lmt_cfg
	arfeature_cfg_tab->kpt_lmt_cfg.to_use = 1;

	for (i = 0; i < ARFEATURE_MAX_BLK_NUM; i++) {
		arfeature_cfg_tab->kpt_lmt_cfg.grid_max_kpnum[i] = req_arfeature->reg_cfg[stripe_index].kpt_lmt.grid_max_kpnum[i];
		arfeature_cfg_tab->kpt_lmt_cfg.grid_dog_threshold[i] =
			req_arfeature->reg_cfg[stripe_index].kpt_lmt.grid_dog_threshold[i];
	}

	// cvdr_cfg
	if (arfeature_cfg_tab->top_cfg.mode == 4 &&
		arfeature_cfg_tab->top_cfg.layer == 0 &&
		arfeature_cfg_tab->top_cfg.iter_num == 0) {
		// mean: the first detect layer and the first iter; and get the descriptor buf from 'stream'.
		arfeature_cfg_tab->cvdr_cfg.to_use = 1;
		arfeature_cfg_tab->cvdr_cfg.wr_cfg.pre_wr_axi_fs.pre_vpwr_address_frame_start =
			req_arfeature->arfeature_stat_buff[DESC_BUFF]; // the first addr of kpt buf .
		arfeature_cfg_tab->cvdr_cfg.rd_cfg.pre_rd_fhg.pre_vprd_frame_size = 0;
		arfeature_cfg_tab->cvdr_cfg.rd_cfg.pre_rd_fhg.pre_vprd_vertical_blanking = 0;
	}

	return;
}

static int seg_ipp_arfeature_module_config(
	struct msg_req_arfeature_request_t *msg,
	struct seg_arfeature_cfg_t *seg_arfeature_cfg)
{
	unsigned int i = 0;
	unsigned int stripe_cnt = 0;
	enum frame_flag_e frame_flag = FRAME_CUR;
	stripe_cnt = msg->mode_cnt;

	for (i = 0; i < stripe_cnt; i++) {
		logd("arf_layer = %d, module_mode = %d", i, seg_arfeature_cfg->arfeature_cfg[i].top_cfg.mode);
		loge_if_ret(arfeature_update_cvdr_cfg_tab(msg, seg_arfeature_cfg, i, frame_flag));
		arfeature_update_request_cfg_tab(msg, seg_arfeature_cfg, i, frame_flag);
	}

	return ISP_IPP_OK;
}

static int cfg_rd_gridstat_cmd_buf(req_arfeature_t *req_arfeature,
								   struct schedule_cmdlst_link_t *cmd_link_entry, unsigned int layer_offset, unsigned int i)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;

	if (req_arfeature->arfeature_stat_buff[MINSCR_KPTCNT_BUFF]) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_arfeature->arfeature_stat_buff[MINSCR_KPTCNT_BUFF] +
			ARFEARURE_MINSCR_KPTCNT_SIZE * layer_offset;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_GRIDSTAT_1_0_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i].cmd_buf, &cmdlst_wr_cfg));
		i++;
	}

	if (req_arfeature->arfeature_stat_buff[SUM_SCORE_BUFF]) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_arfeature->arfeature_stat_buff[SUM_SCORE_BUFF] +
			ARFEARURE_SUM_SCORE_SIZE * layer_offset;
		cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_GRIDSTAT_2_0_REG;
		loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i].cmd_buf, &cmdlst_wr_cfg));
		i++;
	}

	return i;
}

static int cfg_detect_layers_cmd_buf(req_arfeature_t *req_arfeature,
									 struct seg_arfeature_cfg_t *seg_arfeature_cfg, struct schedule_cmdlst_link_t *cmd_link_entry,
									 seg_arf_stripe_num_t *stripe_num_info, unsigned int detect_layer_start)
{
	unsigned int cur_layer = 0, layer_offset = 0;
	unsigned int i = 0;
	struct cfg_tab_arfeature_t *arfeature_cfg_tmp = NULL;
	struct cfg_tab_cvdr_t *arfeature_cvdr_tmp = NULL;
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;
	logd("detect_layer_start = %d, detect_total_stripe = %d", detect_layer_start, stripe_num_info->detect_total_stripe);

	for (i = detect_layer_start; i < (detect_layer_start + stripe_num_info->detect_total_stripe);) {
		if (detect_layer_start == i) { // first detect layer;
			cur_layer = detect_layer_start + ((i - detect_layer_start) / stripe_num_info->layer_stripe_num);
			logd("1 cur_layer = %d", cur_layer);
			arfeature_cfg_tmp = &(seg_arfeature_cfg->arfeature_cfg[cur_layer]);
			arfeature_cvdr_tmp = &(seg_arfeature_cfg->arfeature_cvdr[cur_layer]);
			loge_if(arfeature_prepare_cmd(&g_arfeature_devs[0], &cmd_link_entry[i].cmd_buf, arfeature_cfg_tmp));
			loge_if(cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i].cmd_buf, arfeature_cvdr_tmp));
			i++;
		} else {
			logd("update arf next stripe cvdr axi fs for AR mode descriptor");
			arf_updata_wr_addr_flag = 1;
			cur_layer = detect_layer_start + (((i + 1) - detect_layer_start) / stripe_num_info->layer_stripe_num);
			logd("2 cur_layer = %d", cur_layer);
			arfeature_cfg_tmp = &(seg_arfeature_cfg->arfeature_cfg[cur_layer]);
			arfeature_cvdr_tmp = &(seg_arfeature_cfg->arfeature_cvdr[cur_layer]);
			cmdlst_set_vp_wr_flush(&g_cmdlst_devs[0], &cmd_link_entry[i + 1].cmd_buf, IPP_ARFEATURE_CHANNEL_ID);
			loge_if(arfeature_prepare_cmd(&g_arfeature_devs[0], &cmd_link_entry[i + 1].cmd_buf, arfeature_cfg_tmp));
			loge_if(cvdr_prepare_cmd(&g_cvdr_devs[0], &cmd_link_entry[i + 1].cmd_buf, arfeature_cvdr_tmp));
			arf_updata_wr_addr_flag = 0;
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 1;
			cmdlst_wr_cfg.buff_wr_addr = arfeature_cvdr_tmp->arf_cvdr_wr_addr;
			cmdlst_wr_cfg.reg_rd_addr = IPP_BASE_ADDR_ARF + AR_FEATURE_CVDR_WR_AXI_FS_REG;
			logd("(layer= %d),arf_cvdr_wr_addr = 0x%x", i, arfeature_cvdr_tmp->arf_cvdr_wr_addr);
			// Update the write address of the VP port;
			loge_if(seg_ipp_set_cmdlst_wr_buf(&cmd_link_entry[i].cmd_buf, &cmdlst_wr_cfg));
			cmdlst_set_vp_wr_flush(&g_cmdlst_devs[0], &cmd_link_entry[i].cmd_buf, IPP_ARFEATURE_CHANNEL_ID);
			i = i + 2;
		}

		layer_offset = (cur_layer - detect_layer_start);
		logd("cur_layer = %d, layer_offset=%d", cur_layer, layer_offset);
		i = cfg_rd_gridstat_cmd_buf(req_arfeature, cmd_link_entry, layer_offset, i);
	}

	return i;
}

static int cfg_kptnum_totalkpt_cmd_buf(req_arfeature_t *req_arfeature,
									   struct schedule_cmdlst_link_t *cmd_link_entry, unsigned int i)
{
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	loge_if_ret(memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t), 0, sizeof(struct cmdlst_wr_cfg_t)));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;

	if (req_arfeature->arfeature_stat_buff[KPT_NUM_BUFF]) {
		cmdlst_wr_cfg.data_size = GRID_NUM_RANG_DS4;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_arfeature->arfeature_stat_buff[KPT_NUM_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ARF + AR_FEATURE_KPT_NUMBER_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
						&cmd_link_entry[i].cmd_buf, &cmdlst_wr_cfg));
		i++;
	}

	if (req_arfeature->arfeature_stat_buff[TOTAL_KPT_BUFF]) {
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_arfeature->arfeature_stat_buff[TOTAL_KPT_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ARF + AR_FEATURE_TOTAL_KPT_NUM_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
						&cmd_link_entry[i].cmd_buf, &cmdlst_wr_cfg));
	}

	return ISP_IPP_OK;
}
static int arfeature_multi_layer_cmdlst_para(
	req_arfeature_t *req_arfeature,
	struct cmdlst_para_t *cmdlst_para,
	struct seg_arfeature_cfg_t *seg_arfeature_cfg,
	seg_arf_stripe_num_t *stripe_num_info)
{
	unsigned int i = 0;
	unsigned int detect_layer_start;
	struct cfg_tab_arfeature_t *arfeature_cfg_tmp = NULL;
	struct cfg_tab_cvdr_t *arfeature_cvdr_tmp = NULL;
	struct schedule_cmdlst_link_t *cmd_link_entry = (struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	logi("gauss_total_stripe = %d", stripe_num_info->gauss_total_stripe);

	// 1, configure cmd_link_entry for gauss operation.
	for (i = 0; i < stripe_num_info->gauss_total_stripe; i++) { // 1 stripe/gauss layer;
		arfeature_cfg_tmp = &seg_arfeature_cfg->arfeature_cfg[i];
		arfeature_cvdr_tmp = &seg_arfeature_cfg->arfeature_cvdr[i];
		loge_if_ret(arfeature_prepare_cmd(&g_arfeature_devs[0],
										  &cmd_link_entry[i].cmd_buf, arfeature_cfg_tmp));
		loge_if_ret(cvdr_prepare_cmd(&g_cvdr_devs[0],
									 &cmd_link_entry[i].cmd_buf, arfeature_cvdr_tmp));
	}

	// 2, configure cmd_link_entry for detect operation.
	detect_layer_start = i; // i == stripe_num_info->gauss_total_stripe;
	i = cfg_detect_layers_cmd_buf(req_arfeature, seg_arfeature_cfg,
								  cmd_link_entry, stripe_num_info, detect_layer_start);
	// 3, configure cmd_link_entry for tatalkpt and kptnum.
	loge_if_ret(cfg_kptnum_totalkpt_cmd_buf(req_arfeature, cmd_link_entry, i));
	return ISP_IPP_OK;
}

static int cal_stripe_num_info(struct msg_req_arfeature_request_t *msg,
							   seg_arf_stripe_num_t *tmp_num, unsigned int *total_stripe_num, unsigned int i)
{
	// 1.Cyclically calculate how many cmdlst_stripe are needed in the current msg;
	if (msg->req_arfeature.reg_cfg[i].ctrl.mode != 4) {
		*total_stripe_num = *total_stripe_num + 1;
		tmp_num->gauss_total_stripe += 1;
	} else { // detect mode
		// 2: cfg current layer's arfeature and updata next layer's vp port cfg.
		*total_stripe_num = (i != (msg->mode_cnt - 1)) ?
							(*total_stripe_num + 2) : (*total_stripe_num + 1);
		tmp_num->detect_total_stripe = (i != (msg->mode_cnt - 1)) ?
									   (tmp_num->detect_total_stripe + 2) : (tmp_num->detect_total_stripe + 1);
		tmp_num->layer_stripe_num = 2;

		if (msg->req_arfeature.arfeature_stat_buff[MINSCR_KPTCNT_BUFF]) {
			*total_stripe_num += 1;
			tmp_num->detect_total_stripe += 1;
			tmp_num->layer_stripe_num += 1;
		}

		if (msg->req_arfeature.arfeature_stat_buff[SUM_SCORE_BUFF]) {
			*total_stripe_num += 1;
			tmp_num->detect_total_stripe += 1;
			tmp_num->layer_stripe_num += 1;
		}
	}

	return ISP_IPP_OK;
}

static void cfg_dmap_gauss_first_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return;
}

static void cfg_ar_gauss_first_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_0);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_1);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return;
}

static void cfg_gauss_middle_irq(unsigned long long *irq)
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
	return;
}

static void cfg_gauss_last_irq(unsigned long long *irq)
{
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_RD_EOF_PRY_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_2);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_CVDR_VP_WR_EOF_DOG_3);
	*irq = *irq | (((unsigned long long)(1)) << IPP_ARF_DONE_IRQ);
	return;
}

static void cfg_detect_and_descriptor_irq(unsigned long long *irq, unsigned int pre_stripe_num, unsigned int j)
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

	return;
}

static void seg_arf_init_cmdlst_stripe(struct cmdlst_stripe_info_t *cmdlst_stripe, unsigned int j)
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

static int cfg_kptnum_totalkpt_cmdlst_stripe(struct msg_req_arfeature_request_t *msg,
		struct cmdlst_stripe_info_t *cmdlst_stripe, unsigned int *total_stripe_num, unsigned int pre_stripe_num)
{
	unsigned int j = 0;
	unsigned long long irq = 0;

	if (msg->req_arfeature.arfeature_stat_buff[KPT_NUM_BUFF])
		*total_stripe_num += 1;

	if (msg->req_arfeature.arfeature_stat_buff[TOTAL_KPT_BUFF])
		*total_stripe_num += 1;

	for (j = pre_stripe_num; j < (*total_stripe_num); j++) {
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
		logi("cmdlst_stripe = %d, arfeature_irq = 0x%llx", j, irq);
	}

	return ISP_IPP_OK;
}

static int cal_multi_layer_stripe_info(struct msg_req_arfeature_request_t *msg, struct cmdlst_para_t *cmdlst_para,
									   seg_arf_stripe_num_t *stripe_num_info)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned long long irq = 0;
	unsigned int pre_stripe_num = 0;
	unsigned int total_stripe_num = 0;
	struct cmdlst_stripe_info_t *cmdlst_stripe = cmdlst_para->cmd_stripe_info;
	seg_arf_stripe_num_t tmp_num;
	loge_if_ret(memset_s(&tmp_num, sizeof(seg_arf_stripe_num_t), 0, sizeof(seg_arf_stripe_num_t)));

	for (i = 0; i < msg->mode_cnt; i++) {
		// 1.Calculate the total_stripe_num and the stripe_num_info.
		cal_stripe_num_info(msg, &tmp_num, &total_stripe_num, i);

		// 2.Cfg the cmdlst_stripe; (total_stripe_num - pre_stripe_num) means the current layer's cmdlst_stripe num.
		for (j = pre_stripe_num; j < total_stripe_num; j++) {
			irq = 0;
			seg_arf_init_cmdlst_stripe(cmdlst_stripe, j);

			switch (msg->req_arfeature.reg_cfg[i].ctrl.mode) {
			case DMAP_GAUSS_FIRST:
				cfg_dmap_gauss_first_irq(&irq);
				break;

			case AR_GAUSS_FIRST:
				cfg_ar_gauss_first_irq(&irq);
				break;

			case GAUSS_MIDDLE:
				cfg_gauss_middle_irq(&irq);
				break;

			case GAUSS_LAST:
				cfg_gauss_last_irq(&irq);
				break;

			case DETECT_AND_DESCRIPTOR:
				cfg_detect_and_descriptor_irq(&irq, pre_stripe_num, j);
				break;

			default:
				loge("Failed : arfeature's mode is error,  mode = %d", msg->req_arfeature.reg_cfg[i].ctrl.mode);
				return ISP_IPP_ERR;
			}

			cmdlst_stripe[j].irq_mode  = irq;
			logi("cmdlst_stripe = %d, arfeature_irq = 0x%llx", j, irq);
		}

		pre_stripe_num = total_stripe_num; // Updata the pre_stripe_num.;
	}

	loge_if_ret(memcpy_s(stripe_num_info, sizeof(seg_arf_stripe_num_t), &tmp_num, sizeof(seg_arf_stripe_num_t)));
	// 3.Handle the case where KPT_NUM_BUFF and KPT_NUM_BUFF are not empty;
	cfg_kptnum_totalkpt_cmdlst_stripe(msg, cmdlst_stripe, &total_stripe_num, pre_stripe_num);
	cmdlst_stripe[total_stripe_num - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_ARFEATURE_CHANNEL_ID;
	cmdlst_para->stripe_cnt = total_stripe_num;
	return ISP_IPP_OK;
}

static int seg_ipp_cfg_arfeature_cmdlst_para(
	struct msg_req_arfeature_request_t *msg,
	struct seg_arfeature_cfg_t *seg_arfeature_cfg,
	struct cmdlst_para_t *cmdlst_para)
{
	seg_arf_stripe_num_t stripe_num_info;
	loge_if_ret(cal_multi_layer_stripe_info(msg, cmdlst_para, &stripe_num_info));
	loge_if_ret(df_sched_prepare(cmdlst_para));
	loge_if_ret(arfeature_multi_layer_cmdlst_para(&(msg->req_arfeature),
				cmdlst_para, seg_arfeature_cfg, &stripe_num_info));
	return ISP_IPP_OK;
}

static int seg_ipp_arfeature_get_cpe_mem(struct seg_arfeature_cfg_t **seg_arfeature_cfg,
		struct cmdlst_para_t **arfeature_cmdlst_para)
{
	int ret = 0;
	unsigned long long va = 0;
	unsigned int da = 0;
	// 1.get CFG_TAB
	ret = cpe_mem_get(MEM_ID_ARF_CFG_TAB, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_ARF_CFG_TAB);
		return ISP_IPP_ERR;
	}

	*seg_arfeature_cfg = (struct seg_arfeature_cfg_t *)(uintptr_t)va;
	ret = memset_s(*seg_arfeature_cfg, sizeof(struct seg_arfeature_cfg_t), 0, sizeof(struct seg_arfeature_cfg_t));
	if (ret != 0) {
		loge("Failed : memset_s error!");
		goto arfeature_get_cpe_mem_failed;
	}

	// 2.get CMDLST_PARA
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_ARF, &va, &da);
	if (ret != 0) {
		loge("Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_ARF);
		goto arfeature_get_cpe_mem_failed;
	}

	*arfeature_cmdlst_para = (struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(*arfeature_cmdlst_para, sizeof(struct cmdlst_para_t), 0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		loge("Failed : memset_s error!");
		goto arfeature_get_cpe_mem_failed;
	}

	return ISP_IPP_OK;
arfeature_get_cpe_mem_failed:
	cpe_mem_free(MEM_ID_ARF_CFG_TAB);
	return ISP_IPP_ERR;
}

int arfeature_request_handler(struct msg_req_arfeature_request_t *msg)
{
	int ret = 0;
	struct seg_arfeature_cfg_t *seg_arfeature_cfg = NULL;
	struct cmdlst_para_t *arfeature_cmdlst_para = NULL;
	dataflow_check_para(msg);
	logd("+");
	loge_if_ret(arfeature_check_request_para(msg));
	arfeature_request_dump(msg);
#if IPP_UT_DEBUG
	set_dump_register_init(UT_REG_ADDR, IPP_MAX_REG_OFFSET, 0);
#endif
	ret = seg_ipp_arfeature_get_cpe_mem(&seg_arfeature_cfg, &arfeature_cmdlst_para);
	if (ret != 0) {
		loge(" Failed : seg_ipp_arfeature_get_cpe_mem");
		return ISP_IPP_ERR;
	}

	ret = seg_ipp_arfeature_module_config(msg, seg_arfeature_cfg);
	if (ret  != 0) {
		loge(" Failed : seg_ipp_arfeature_module_config");
		goto SEG_ARF_BUFF_FREE;
	}

	ret = seg_ipp_cfg_arfeature_cmdlst_para(msg, seg_arfeature_cfg, arfeature_cmdlst_para);
	if (ret  != 0) {
		loge(" Failed : seg_ipp_cfg_arfeature_cmdlst_para");
		goto SEG_ARF_BUFF_FREE;
	}

	ret = df_sched_start(arfeature_cmdlst_para);
	if (ret  != 0)
		loge(" Failed : df_sched_start");

	logd("-");
SEG_ARF_BUFF_FREE:
	cpe_mem_free(MEM_ID_ARF_CFG_TAB);
	return ret;
}

static int arfeature_check_request_para(struct msg_req_arfeature_request_t *msg)
{
	if ((msg->mode_cnt > 10) || (msg->mode_cnt < 3)) {
		loge(" Failed : mode_cnt %d is error", msg->mode_cnt);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
static void arf_reg_cfg_dump(arfeature_reg_cfg_t *reg_cfg)
{
	logi("ctrl:mode = %d", reg_cfg->ctrl.mode);
	logi("ctrl:orient_en = %d", reg_cfg->ctrl.orient_en);
	logi("ctrl:layer = %d", reg_cfg->ctrl.layer);
	logi("ctrl:iter_num = %d", reg_cfg->ctrl.iter_num);
	logi("ctrl:first_detect = %d", reg_cfg->ctrl.first_detect);
	logi("size_cfg:width = %d, height = %d",
		 reg_cfg->size_cfg.width, reg_cfg->size_cfg.height);
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
	logi("gauss_org_0~3:%d, %d, %d, %d",
		 reg_cfg->gauss_coef.gauss_org_0, reg_cfg->gauss_coef.gauss_org_1,
		 reg_cfg->gauss_coef.gauss_org_2, reg_cfg->gauss_coef.gauss_org_3);
	logi("gauss_1st_0~2:%d, %d, %d",
		 reg_cfg->gauss_coef.gsblur_1st_0, reg_cfg->gauss_coef.gsblur_1st_1,
		 reg_cfg->gauss_coef.gsblur_1st_2);
	logi("gauss_2nd_0~3:%d, %d, %d, %d",
		 reg_cfg->gauss_coef.gauss_2nd_0, reg_cfg->gauss_coef.gauss_2nd_1,
		 reg_cfg->gauss_coef.gauss_2nd_2, reg_cfg->gauss_coef.gauss_2nd_3);
	logi("grid_dog_threshold 0~3: %d, %d, %d, %d",
		 reg_cfg->kpt_lmt.grid_dog_threshold[0], reg_cfg->kpt_lmt.grid_dog_threshold[1],
		 reg_cfg->kpt_lmt.grid_dog_threshold[2], reg_cfg->kpt_lmt.grid_dog_threshold[3]);
	logi("grid_max_kpnum 0~3: %d, %d, %d, %d",
		 reg_cfg->kpt_lmt.grid_max_kpnum[0], reg_cfg->kpt_lmt.grid_max_kpnum[1],
		 reg_cfg->kpt_lmt.grid_max_kpnum[2], reg_cfg->kpt_lmt.grid_max_kpnum[3]);
}
#endif

static void arfeature_request_dump(struct msg_req_arfeature_request_t *msg)
{
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
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

#endif

	logi("sizeof(struct msg_req_arfeature_request_t) = 0x%x (Byte)",
		 (int)(sizeof(struct msg_req_arfeature_request_t)));
	logi("req->frame_num = %d", msg->frame_num);
	logi("req->secure_flag = %d", msg->secure_flag);
	logi("req->arfeature_rate_value = %d", msg->arfeature_rate_value);
	logi("req->layer_cnt  = %d", msg->mode_cnt);


#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	for (i = 0; i < msg->mode_cnt; i++) { // one mode corresponds to  one layer
		arfeature_reg_cfg_t *reg_cfg_ptr = NULL;
		logi("now_arfeature_layer_index = %d:", i);
		logi("ARFEATURE STREAM DUMP:");

		for (j = 0; j < ARFEATURE_STREAM_MAX; j++) {
			if ((msg->req_arfeature.streams[i][j].width != 0) && (msg->req_arfeature.streams[i][j].height != 0)) {
				logi("streams[%d][%s].width = %d", i, stream_name[j], msg->req_arfeature.streams[i][j].width);
				logi("streams[%d][%s].height = %d", i, stream_name[j], msg->req_arfeature.streams[i][j].height);
				logi("streams[%d][%s].stride = %d", i, stream_name[j], msg->req_arfeature.streams[i][j].stride);
				logi("streams[%d][%s].format = %d", i, stream_name[j], msg->req_arfeature.streams[i][j].format);
				logi("streams[%d][%s].buf = 0x%x", i, stream_name[j], msg->req_arfeature.streams[i][j].buffer);
			}
		}

		logi("ARFEATURE CFG DUMP:");
		reg_cfg_ptr = &msg->req_arfeature.reg_cfg[i];
		arf_reg_cfg_dump(reg_cfg_ptr);
	}
#endif
	return;
}

