#include <linux/string.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <libhwsecurec/securec.h>
#include "orb.h"
#include "cfg_table_orb.h"
#include "cvdr_drv.h"
#include "orb_drv.h"
#include "config_table_cvdr.h"
#include "cvdr_opt.h"
#include "adapter_common.h"
#include "memory.h"
#include "orb_drv_priv.h"
#include "orb_reg_offset.h"
#include "ipp_top_reg_offset.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#define LOG_TAG LOG_MODULE_ORB
#define CVDR_KEY_POINT_OVERFLOW  0x8000
#define CMDLST_READ_FLAG  1

static int orb_request_dump(
	struct msg_req_orb_request_t *req);
static int orb_check_request_para(
	struct msg_req_orb_request_t *orb_request);

static int orb_update_cvdr_cfg_tab(
	struct msg_req_orb_request_t *req,
	struct enh_orb_cfg_t *enh_orb_cfg,
	struct ipp_orb_stripe_info_t *p_size_info,
	unsigned int stripe_index,
	enum frame_flag_e frame_flag)
{
	struct cvdr_opt_fmt_t cfmt;
	enum pix_format_e format = PIXEL_FMT_IPP_MAX;
	unsigned int orb_stride = 0;
	unsigned int  work_mode = req->work_mode;
	struct req_orb_t    *req_orb = NULL;
	struct cfg_tab_cvdr_t *orb_cvdr_cfg_tab = NULL;
	orb_cvdr_cfg_tab = (frame_flag == FRAME_CUR) ?
		&(enh_orb_cfg->orb_cvdr_cur[stripe_index]) :
		&(enh_orb_cfg->orb_cvdr_ref[stripe_index]);
	req_orb = (frame_flag == FRAME_CUR) ?
		&(req->req_orb_cur) : &(req->req_orb_ref);
	if (work_mode == 1 && req_orb->streams[0][BI_ORB_Y].buffer) {
		cfmt.id = IPP_VP_RD_ORB;
		cfmt.width = p_size_info->stripe_width[stripe_index];
		cfmt.full_width = req_orb->streams[0][BI_ORB_Y].width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height = req_orb->streams[0][BI_ORB_Y].height;
		cfmt.addr = req_orb->streams[0][BI_ORB_Y].buffer +
			p_size_info->stripe_start_point[stripe_index];
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_2PF8;
		orb_stride = req_orb->streams[0][BI_ORB_Y].stride;
		loge_if_ret(dataflow_cvdr_rd_cfg_vp(orb_cvdr_cfg_tab,
				&cfmt, orb_stride, ISP_IPP_CLK, format));
	} else if (req_orb->streams[stripe_index][BI_ORB_Y].buffer) {
		cfmt.id = IPP_VP_RD_ORB;
		cfmt.width =
			req_orb->streams[stripe_index][BI_ORB_Y].width;
		cfmt.full_width = cfmt.width / 2;
		cfmt.line_size = cfmt.width / 2;
		cfmt.height =
			req_orb->streams[stripe_index][BI_ORB_Y].height;
		cfmt.addr =
			req_orb->streams[stripe_index][BI_ORB_Y].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_2PF8;
		if (stripe_index == 0) {
			orb_stride =
				req_orb->streams[stripe_index][BI_ORB_Y].stride;
			loge_if_ret(dataflow_cvdr_rd_cfg_vp(orb_cvdr_cfg_tab,
					&cfmt, orb_stride, ISP_IPP_CLK, format));
		} else {
			loge_if_ret(dataflow_cvdr_rd_cfg_vp(orb_cvdr_cfg_tab,
					&cfmt, 0, ISP_IPP_CLK, format));
		}
	}
	if (req_orb->streams[stripe_index][BO_ORB_Y].buffer) {
		cfmt.id = IPP_VP_WR_SLAM_Y;
		cfmt.width =
			req_orb->streams[stripe_index][BO_ORB_Y].width;
		cfmt.full_width =
			req_orb->streams[stripe_index][BO_ORB_Y].width / 2;
		cfmt.line_size =
			req_orb->streams[stripe_index][BO_ORB_Y].width / 2;
		cfmt.height =
			req_orb->streams[stripe_index][BO_ORB_Y].height;
		cfmt.addr =
			req_orb->streams[stripe_index][BO_ORB_Y].buffer;
		cfmt.expand = 0;
		format = PIXEL_FMT_IPP_2PF8;
		loge_if_ret(dataflow_cvdr_wr_cfg_vp(orb_cvdr_cfg_tab,
				&cfmt, 0, ISP_IPP_CLK, format));
	}
	if (req_orb->orb_stat_buff[DESC_BUFF]) {
		cfmt.id = IPP_VP_WR_ORB_STAT;
		cfmt.addr = req_orb->orb_stat_buff[DESC_BUFF];
		loge_if_ret(cfg_tbl_cvdr_wr_cfg_d64(orb_cvdr_cfg_tab,
				&cfmt, ORB_OUT_DESC_SIZE));
	}
	return 0;
}

static void orb_update_request_cfg_tab(
	struct msg_req_orb_request_t *req,
	struct enh_orb_cfg_t *enh_orb_cfg,
	struct ipp_orb_stripe_info_t *p_size_info,
	unsigned int stripe_index,
	enum frame_flag_e frame_flag)
{
	struct req_orb_t    *req_orb = NULL;
	struct cfg_tab_orb_enh_t *enh_cfg_tab = NULL;
	struct cfg_tab_orb_t *orb_cfg_tab = NULL;
	orb_cfg_tab = (frame_flag == FRAME_CUR) ?
		(struct cfg_tab_orb_t *)
		(uintptr_t)(&(enh_orb_cfg->orb_cur[stripe_index])) :
		(struct cfg_tab_orb_t *)
		(uintptr_t)(&(enh_orb_cfg->orb_ref[stripe_index]));
	enh_cfg_tab = (frame_flag == FRAME_CUR) ?
		(struct cfg_tab_orb_enh_t *)
		(uintptr_t)(&(enh_orb_cfg->enh_cur)) :
		(struct cfg_tab_orb_enh_t *)
		(uintptr_t)(&(enh_orb_cfg->enh_ref));
	req_orb = (frame_flag == FRAME_CUR) ?
		&(req->req_orb_cur) : &(req->req_orb_ref);
	unsigned int layer_index = (req->work_mode == 1) ?
		0 : stripe_index;
	orb_cfg_tab->ctrl.to_use = 1;
	orb_cfg_tab->ctrl.pyramid_en =
		req_orb->reg_cfg[layer_index].ctrl.pyramid_en;
	orb_cfg_tab->ctrl.gsblur_en =
		req_orb->reg_cfg[layer_index].ctrl.gsblur_en;
	orb_cfg_tab->ctrl.fast_en =
		req_orb->reg_cfg[layer_index].ctrl.fast_en;
	orb_cfg_tab->ctrl.nms_en =
		req_orb->reg_cfg[layer_index].ctrl.nms_en;
	orb_cfg_tab->ctrl.orient_en =
		req_orb->reg_cfg[layer_index].ctrl.orient_en;
	orb_cfg_tab->ctrl.brief_en =
		req_orb->reg_cfg[layer_index].ctrl.brief_en;
	orb_cfg_tab->ctrl.freak_en =
		req_orb->reg_cfg[layer_index].ctrl.freak_en;
	orb_cfg_tab->ctrl.gridstat_en =
		req_orb->reg_cfg[layer_index].ctrl.gridstat_en;
	orb_cfg_tab->ctrl.undistort_en =
		req_orb->reg_cfg[layer_index].ctrl.undistort_en;
	orb_cfg_tab->ctrl.first_layer =
		req_orb->reg_cfg[layer_index].ctrl.first_layer;
	orb_cfg_tab->ctrl.first_stripe =
		req_orb->reg_cfg[layer_index].ctrl.first_stripe;
	orb_cfg_tab->cvdr_cfg.to_use   = 1;
	orb_cfg_tab->size_cfg.to_use  = 1;
	orb_cfg_tab->size_cfg.width  =
		req_orb->reg_cfg[layer_index].size_cfg.width - 1;
	orb_cfg_tab->size_cfg.height =
		req_orb->reg_cfg[layer_index].size_cfg.height - 1;
	orb_cfg_tab->size_cfg.stat_range.stat_left = 0x12;
	orb_cfg_tab->size_cfg.stat_range.stat_right =
		orb_cfg_tab->size_cfg.width - 18;
	orb_cfg_tab->pyramid_scaler_cfg.to_use =
		orb_cfg_tab->ctrl.pyramid_en;
	orb_cfg_tab->pyramid_scaler_cfg.scl_inc =
		req_orb->reg_cfg[layer_index].pyramid_scaler_cfg.scl_inc;
	orb_cfg_tab->pyramid_scaler_cfg.scl_vtop =
		req_orb->reg_cfg[layer_index].pyramid_scaler_cfg.scl_vtop;
	orb_cfg_tab->pyramid_scaler_cfg.scl_vbottom =
		req_orb->reg_cfg[layer_index].pyramid_scaler_cfg.scl_vbottom;
	orb_cfg_tab->pyramid_scaler_cfg.scl_hright =
		req_orb->reg_cfg[layer_index].pyramid_scaler_cfg.scl_hright;
	orb_cfg_tab->pyramid_scaler_cfg.scl_hleft =
		req_orb->reg_cfg[layer_index].pyramid_scaler_cfg.scl_hleft;
	orb_cfg_tab->gsblur_coef_cfg.to_use = orb_cfg_tab->ctrl.gsblur_en;
	orb_cfg_tab->gsblur_coef_cfg.coeff_gauss_0 =
		req_orb->reg_cfg[layer_index].gsblur_coef_cfg.coeff_gauss_0;
	orb_cfg_tab->gsblur_coef_cfg.coeff_gauss_1 =
		req_orb->reg_cfg[layer_index].gsblur_coef_cfg.coeff_gauss_1;
	orb_cfg_tab->gsblur_coef_cfg.coeff_gauss_2 =
		req_orb->reg_cfg[layer_index].gsblur_coef_cfg.coeff_gauss_2;
	orb_cfg_tab->gsblur_coef_cfg.coeff_gauss_3 =
		req_orb->reg_cfg[layer_index].gsblur_coef_cfg.coeff_gauss_3;
	orb_cfg_tab->fast_coef.to_use =
		orb_cfg_tab->ctrl.fast_en;
	orb_cfg_tab->fast_coef.ini_th =
		req_orb->reg_cfg[layer_index].fast_coef.ini_th;
	orb_cfg_tab->fast_coef.min_th =
		req_orb->reg_cfg[layer_index].fast_coef.min_th;
	orb_cfg_tab->nms_win.to_use =
		orb_cfg_tab->ctrl.nms_en;
	orb_cfg_tab->nms_win.nmscell_h =
		req_orb->reg_cfg[layer_index].nms_win.nmscell_h;
	orb_cfg_tab->nms_win.nmscell_v =
		req_orb->reg_cfg[layer_index].nms_win.nmscell_v;
	orb_cfg_tab->gridstat_cfg.to_use = 1;
	orb_cfg_tab->gridstat_cfg.block_num.blk_h_num =
		req_orb->reg_cfg[layer_index].gridstat_cfg.block_num.blk_h_num;
	orb_cfg_tab->gridstat_cfg.block_num.blk_v_num =
		req_orb->reg_cfg[layer_index].gridstat_cfg.block_num.blk_v_num;
	orb_cfg_tab->gridstat_cfg.block_size.blk_h_size =
		req_orb->reg_cfg[layer_index].gridstat_cfg.block_size.blk_h_size;
	orb_cfg_tab->gridstat_cfg.block_size.blk_v_size =
		req_orb->reg_cfg[layer_index].gridstat_cfg.block_size.blk_v_size;
	orb_cfg_tab->octree_cfg.to_use = 1;
	orb_cfg_tab->octree_cfg.max_kpnum =
		req_orb->reg_cfg[layer_index].octree_cfg.max_kpnum;
	orb_cfg_tab->octree_cfg.grid_max_kpnum =
		req_orb->reg_cfg[layer_index].octree_cfg.grid_max_kpnum;
	orb_cfg_tab->octree_cfg.inc_level =
		req_orb->reg_cfg[layer_index].octree_cfg.inc_level;
	orb_cfg_tab->octree_cfg.flag_10_11 =
		req_orb->reg_cfg[layer_index].octree_cfg.flag_10_11;
	orb_cfg_tab->octree_cfg.layer0_start.start_blk_h_num =
		(((18 * orb_cfg_tab->octree_cfg.inc_level + (1 << 17)) >> 18) - 18)
		/ (orb_cfg_tab->gridstat_cfg.block_size.blk_h_size);
	orb_cfg_tab->octree_cfg.layer0_start.start_blk_h_offset =
		(((18 * orb_cfg_tab->octree_cfg.inc_level + (1 << 17)) >> 18) - 18)
		% (orb_cfg_tab->gridstat_cfg.block_size.blk_h_size);
	orb_cfg_tab->octree_cfg.layer0_start.start_blk_v_num =
		(((18 * orb_cfg_tab->octree_cfg.inc_level + (1 << 17)) >> 18) - 18)
		/ (orb_cfg_tab->gridstat_cfg.block_size.blk_v_size);
	orb_cfg_tab->octree_cfg.layer0_start.start_blk_v_offset =
		(((18 * orb_cfg_tab->octree_cfg.inc_level + (1 << 17)) >> 18) - 18)
		% (orb_cfg_tab->gridstat_cfg.block_size.blk_v_size);
	orb_cfg_tab->octree_cfg.blk_size_inv.blk_h_size_inv =
		req_orb->reg_cfg[layer_index].octree_cfg.blk_size_inv.blk_h_size_inv;
	orb_cfg_tab->octree_cfg.blk_size_inv.blk_v_size_inv =
		req_orb->reg_cfg[layer_index].octree_cfg.blk_size_inv.blk_v_size_inv;
	if (req->work_mode == 1) {
		orb_cfg_tab->ctrl.first_stripe = (stripe_index == 0) ? 1 : 0;
		orb_cfg_tab->size_cfg.width =
			p_size_info->stripe_width[stripe_index] - 1;
		orb_cfg_tab->size_cfg.stat_range.stat_left  =
			(stripe_index == 0) ? 18 : 32;
		orb_cfg_tab->size_cfg.stat_range.stat_right  =
			(stripe_index == 0) ? (p_size_info->stripe_width[stripe_index] - 33) :
			(orb_cfg_tab->size_cfg.width - 19);
		orb_cfg_tab->octree_cfg.layer0_start.start_blk_h_num =
			(stripe_index == 0) ? 0 : (p_size_info->stripe_width[stripe_index] - 32 - 18)
			/ (orb_cfg_tab->gridstat_cfg.block_size.blk_h_size);
		orb_cfg_tab->octree_cfg.layer0_start.start_blk_h_offset =
			(stripe_index == 0) ? 0 : (p_size_info->stripe_width[stripe_index] - 32 - 18)
			% (orb_cfg_tab->gridstat_cfg.block_size.blk_h_size);
		orb_cfg_tab->octree_cfg.layer0_start.start_blk_v_num = 0;
		orb_cfg_tab->octree_cfg.layer0_start.start_blk_v_offset = 0;
	}
	orb_cfg_tab->undistort_cfg.to_use =
		orb_cfg_tab->ctrl.undistort_en;
	orb_cfg_tab->undistort_cfg.cx =
		req_orb->reg_cfg[layer_index].undistort_cfg.cx;
	orb_cfg_tab->undistort_cfg.cy =
		req_orb->reg_cfg[layer_index].undistort_cfg.cy;
	orb_cfg_tab->undistort_cfg.fx =
		req_orb->reg_cfg[layer_index].undistort_cfg.fx;
	orb_cfg_tab->undistort_cfg.fy =
		req_orb->reg_cfg[layer_index].undistort_cfg.fy;
	orb_cfg_tab->undistort_cfg.invfx =
		req_orb->reg_cfg[layer_index].undistort_cfg.invfx;
	orb_cfg_tab->undistort_cfg.invfy =
		req_orb->reg_cfg[layer_index].undistort_cfg.invfy;
	orb_cfg_tab->undistort_cfg.first =
		req_orb->reg_cfg[layer_index].undistort_cfg.first;
	orb_cfg_tab->undistort_cfg.second =
		req_orb->reg_cfg[layer_index].undistort_cfg.second;
	orb_cfg_tab->undistort_cfg.third =
		req_orb->reg_cfg[layer_index].undistort_cfg.third;
	orb_cfg_tab->undistort_cfg.p1 =
		req_orb->reg_cfg[layer_index].undistort_cfg.p1;
	orb_cfg_tab->undistort_cfg.p2 =
		req_orb->reg_cfg[layer_index].undistort_cfg.p2;
	loge_if(memcpy_s(orb_cfg_tab->octree_cfg.score_th,
			ORB_SCORE_TH_RANGE * sizeof(unsigned int),
			req_orb->reg_cfg[layer_index].octree_cfg.score_th,
			ORB_SCORE_TH_RANGE * sizeof(unsigned int)));
	orb_cfg_tab->pattern_to_use = 0;
	if (layer_index == 0 && frame_flag == FRAME_CUR) {
		orb_cfg_tab->pattern_to_use = 1;
		loge_if(memcpy_s(&(orb_cfg_tab->pattern_cfg),
				sizeof(struct orb_pattern_cfg_t),
				&(req_orb->reg_cfg[layer_index].pattern_cfg),
				sizeof(struct orb_pattern_cfg_t)));
	};
	orb_cfg_tab->cvdr_cfg.to_use = (stripe_index == 0) ? 1 : 0;
	orb_cfg_tab->cvdr_cfg.wr_cfg.pre_wr_axi_fs.pre_vpwr_address_frame_start =
		req_orb->orb_stat_buff[DESC_BUFF];
	orb_cfg_tab->cvdr_cfg.rd_cfg.pre_rd_fhg.pre_vprd_frame_size = 0;
	orb_cfg_tab->cvdr_cfg.rd_cfg.pre_rd_fhg.pre_vprd_vertical_blanking = 0;
}

static void seg_ipp_orb_module_config(
	struct msg_req_orb_request_t *orb_req,
	struct enh_orb_cfg_t *enh_orb_cfg,
	struct ipp_orb_stripe_info_t *p_size_info)
{
	unsigned int i = 0;
	unsigned int stripe_cnt = 0;
	enum frame_flag_e frame_flag = FRAME_CUR;
	stripe_cnt = (orb_req->work_mode == 1) ?
		(p_size_info->stripe_cnt) : (orb_req->orb_pyramid_layer);
	for (i = 0; i < stripe_cnt; i++) {
		orb_update_cvdr_cfg_tab(
			orb_req, enh_orb_cfg,
			p_size_info, i, frame_flag);
		orb_update_request_cfg_tab(
			orb_req, enh_orb_cfg,
			p_size_info, i, frame_flag);
	}
}

static int orb_multi_layer_cmdlst_para(
	struct cmdlst_para_t *cmdlst_para,
	struct msg_req_orb_request_t *orb_request,
	struct enh_orb_cfg_t *enh_orb_cfg,
	unsigned int orb_layer_stripe_num,
	enum frame_flag_e frame_flag)
{
	if (orb_layer_stripe_num == 0) {
		return ISP_IPP_ERR;
	}
	unsigned int cur_layer = 0;
	unsigned int stripe_cnt = 0;
	unsigned int i = 0;
	unsigned int index = 0;
	unsigned int stripe_base = (cmdlst_para->stripe_cnt - 2) / 2;
	struct cfg_tab_cvdr_t *cvdr_cfg_tmp = NULL;
	struct cfg_tab_orb_t *orb_cfg_tmp = NULL;
	struct req_orb_t   *req_orb_tmp = NULL;
	struct schedule_cmdlst_link_t *cmd_link_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	D("cmdlst_para->stripe_cnt = %d, orb_layer_stripe_num=%d",
		cmdlst_para->stripe_cnt, orb_layer_stripe_num);
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t),
		0, sizeof(struct cmdlst_wr_cfg_t));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;
	req_orb_tmp = (frame_flag == FRAME_CUR) ?
		&(orb_request->req_orb_cur) :
		&(orb_request->req_orb_ref);
	unsigned int last_layer = orb_request->orb_pyramid_layer - 1;
	for (i = 0; i < orb_request->orb_pyramid_layer; i++) {
		stripe_cnt  = (i == last_layer) ?
			(stripe_cnt + 1) : (stripe_cnt + 2);
		if (req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF]) {
			stripe_cnt += 1;
		}
		if (req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF]) {
			stripe_cnt += 1;
		}
	}
	if (frame_flag == FRMAE_REF)
		index = stripe_base;
	for (i = index; i < (index + stripe_cnt);) {
		if (index == i) {
			cur_layer = (frame_flag == FRAME_CUR) ?
				(i / orb_layer_stripe_num) :
				((i - stripe_base) / orb_layer_stripe_num);
			D("for test add 3,i=%d,cur_layer=%d", i, cur_layer);
			orb_cfg_tmp = (frame_flag == FRAME_CUR) ?
				&enh_orb_cfg->orb_cur[cur_layer] :
				&enh_orb_cfg->orb_ref[cur_layer];
			orb_prepare_cmd(&g_orb_devs[0],
				&cmd_link_entry[i].cmd_buf, orb_cfg_tmp);
			cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ?
				&enh_orb_cfg->orb_cvdr_cur[cur_layer] :
				&enh_orb_cfg->orb_cvdr_ref[cur_layer];
			cvdr_prepare_cmd(&g_cvdr_devs[0],
				&cmd_link_entry[i++].cmd_buf,
				cvdr_cfg_tmp);
		} else {
			cur_layer = (frame_flag == FRAME_CUR) ?
				(i + 1) / orb_layer_stripe_num :
				((i + 1 - stripe_base) / orb_layer_stripe_num);
			D("for test add 2,i=%d,cur_layer=%d", i, cur_layer);
			orb_cfg_tmp = (frame_flag == FRAME_CUR) ?
				&enh_orb_cfg->orb_cur[cur_layer] :
				&enh_orb_cfg->orb_ref[cur_layer];
			orb_prepare_cmd(&g_orb_devs[0],
				&cmd_link_entry[i + 1].cmd_buf, orb_cfg_tmp);
			cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ?
				&enh_orb_cfg->orb_cvdr_cur[cur_layer] :
				&enh_orb_cfg->orb_cvdr_ref[cur_layer];
			cvdr_prepare_cmd(&g_cvdr_devs[0],
				&cmd_link_entry[i + 1].cmd_buf, cvdr_cfg_tmp);
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 1;
			cmdlst_wr_cfg.buff_wr_addr =
				cvdr_cfg_tmp->orb_cvdr_wr_addr;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_ORB + ORB_CVDR_WR_AXI_FS_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
					&cmd_link_entry[i].cmd_buf,
					&cmdlst_wr_cfg));
			i = i + 2;
		}
		if (req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF]) {
			cmdlst_wr_cfg.data_size = ORB_GRIDSTAT_RANGE;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr =
				req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF] +
				ORB_GRID_SOCRE_REG_SIZE * cur_layer;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_ORB + ORB_GRIDSTAT_0_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
					&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}
		if (req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF]) {
			cmdlst_wr_cfg.data_size = 1;
			cmdlst_wr_cfg.is_wstrb = 0;
			cmdlst_wr_cfg.buff_wr_addr =
				req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF]
				+ ORB_GRID_KPT_NMS_REG_SIZE * cur_layer;
			cmdlst_wr_cfg.reg_rd_addr =
				IPP_BASE_ADDR_ORB + ORB_KPNUM_NMS_REG;
			loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
					&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
		}
	}
	if (req_orb_tmp->orb_stat_buff[KPT_NUM_BUFF]) {
		cmdlst_wr_cfg.data_size = ORB_KPT_NUM_RANGE;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_orb_tmp->orb_stat_buff[KPT_NUM_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ORB + ORB_KPT_NUMBER_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}
	if (req_orb_tmp->orb_stat_buff[TOTAL_KPT_BUFF]) {
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_orb_tmp->orb_stat_buff[TOTAL_KPT_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ORB + ORB_TOTAL_KPT_NUM_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[i++].cmd_buf, &cmdlst_wr_cfg));
	}
	return ISP_IPP_OK;
}

static int orb_multi_stripe_cmdlst_para(
	struct cmdlst_para_t *cmdlst_para,
	struct msg_req_orb_request_t *orb_request,
	struct enh_orb_cfg_t *enh_orb_cfg,
	enum frame_flag_e frame_flag)
{
	unsigned int stripe_cnt = (cmdlst_para->stripe_cnt - 2) / 2;
	unsigned int index = 0;
	struct cfg_tab_cvdr_t *cvdr_cfg_tmp = NULL;
	struct cfg_tab_orb_t *orb_cfg_tmp = NULL;
	struct req_orb_t   *req_orb_tmp = NULL;
	struct schedule_cmdlst_link_t *cmd_link_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	D("cmdlst_para->stripe_cnt = %d", cmdlst_para->stripe_cnt);
	struct cmdlst_wr_cfg_t cmdlst_wr_cfg;
	memset_s(&cmdlst_wr_cfg, sizeof(struct cmdlst_wr_cfg_t),
		0, sizeof(struct cmdlst_wr_cfg_t));
	cmdlst_wr_cfg.is_incr = 1;
	cmdlst_wr_cfg.read_mode = 0;
	req_orb_tmp = (frame_flag == FRAME_CUR) ?
		&(orb_request->req_orb_cur) :
		&(orb_request->req_orb_ref);
	index = (frame_flag == FRAME_CUR) ?
		index : (stripe_cnt);
	orb_cfg_tmp = (frame_flag == FRAME_CUR) ?
		&enh_orb_cfg->orb_cur[0] :
		&enh_orb_cfg->orb_ref[0];
	orb_prepare_cmd(&g_orb_devs[0],
		&cmd_link_entry[index].cmd_buf, orb_cfg_tmp);
	cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ?
		&enh_orb_cfg->orb_cvdr_cur[0] :
		&enh_orb_cfg->orb_cvdr_ref[0];
	cvdr_prepare_cmd(&g_cvdr_devs[0],
		&cmd_link_entry[index].cmd_buf, cvdr_cfg_tmp);
	orb_cfg_tmp = (frame_flag == FRAME_CUR) ?
		&enh_orb_cfg->orb_cur[1] :
		&enh_orb_cfg->orb_ref[1];
	orb_prepare_cmd(&g_orb_devs[0],
		&cmd_link_entry[index + 2].cmd_buf, orb_cfg_tmp);
	cvdr_cfg_tmp = (frame_flag == FRAME_CUR) ?
		&enh_orb_cfg->orb_cvdr_cur[1] :
		&enh_orb_cfg->orb_cvdr_ref[1];
	cvdr_prepare_cmd(&g_cvdr_devs[0],
		&cmd_link_entry[index + 2].cmd_buf, cvdr_cfg_tmp);
	cmdlst_wr_cfg.data_size = 1;
	cmdlst_wr_cfg.is_wstrb = 1;
	cmdlst_wr_cfg.buff_wr_addr =
		cvdr_cfg_tmp->orb_cvdr_wr_addr;
	cmdlst_wr_cfg.reg_rd_addr =
		IPP_BASE_ADDR_ORB  + ORB_CVDR_WR_AXI_FS_REG;
	D("cvdr_cfg_tmp->orb_cvdr_wr_addr = 0x%x\n",
		cvdr_cfg_tmp->orb_cvdr_wr_addr);
	loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
			&cmd_link_entry[index + 1].cmd_buf, &cmdlst_wr_cfg));
	index = index + 3;
	if (req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF]) {
		cmdlst_wr_cfg.data_size = ORB_GRIDSTAT_RANGE;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ORB + ORB_GRIDSTAT_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[index++].cmd_buf, &cmdlst_wr_cfg));
	}
	if (req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF]) {
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ORB + ORB_KPNUM_NMS_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[index++].cmd_buf, &cmdlst_wr_cfg));
	}
	if (req_orb_tmp->orb_stat_buff[KPT_NUM_BUFF]) {
		cmdlst_wr_cfg.data_size = ORB_KPT_NUM_RANGE;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_orb_tmp->orb_stat_buff[KPT_NUM_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ORB + ORB_KPT_NUMBER_0_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[index++].cmd_buf, &cmdlst_wr_cfg));
	}
	if (req_orb_tmp->orb_stat_buff[TOTAL_KPT_BUFF]) {
		cmdlst_wr_cfg.data_size = 1;
		cmdlst_wr_cfg.is_wstrb = 0;
		cmdlst_wr_cfg.buff_wr_addr =
			req_orb_tmp->orb_stat_buff[TOTAL_KPT_BUFF];
		cmdlst_wr_cfg.reg_rd_addr =
			IPP_BASE_ADDR_ORB + ORB_TOTAL_KPT_NUM_REG;
		loge_if_ret(seg_ipp_set_cmdlst_wr_buf(
				&cmd_link_entry[index++].cmd_buf, &cmdlst_wr_cfg));
	}
	return ISP_IPP_OK;
}

static int cal_split_stripe_info(
	struct msg_req_orb_request_t *req,
	struct cmdlst_para_t *cmdlst_para)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned long long irq = 0;
	unsigned int orb_stripe_num = 0;
	unsigned int orb_stripe_base_num = 0;
	unsigned int temp_layer_stripe = 0;
	unsigned int stripe_index = 0;
	struct req_orb_t   *req_orb_tmp = NULL;
	req_orb_tmp = &(req->req_orb_cur);
	orb_stripe_num += 3;
	if (req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF])
		orb_stripe_num += 1;
	if (req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF])
		orb_stripe_num += 1;
	if (req_orb_tmp->orb_stat_buff[KPT_NUM_BUFF])
		orb_stripe_num += 1;
	if (req_orb_tmp->orb_stat_buff[TOTAL_KPT_BUFF])
		orb_stripe_num += 1;
	orb_stripe_base_num = orb_stripe_num;
	I("orb_stripe_num = %d, orb_stripe_base_num = %d",
		orb_stripe_num, orb_stripe_base_num);
	for (i = 0; i < orb_stripe_num ; i++) {
		irq = 0;
		stripe_index = (i >= orb_stripe_base_num)
			? (i - orb_stripe_base_num) : i;
		temp_layer_stripe = 2;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_ORB;
		cmdlst_stripe[i].is_need_set_sop = 0;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		if ((stripe_index % 2 != 0) ||
			(stripe_index > temp_layer_stripe)) {
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_WR_EOF_CMDLST);
			cmdlst_stripe[i].irq_mode  = irq;
		} else {
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_DONE_IRQ);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_WR_EOF_LEP);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_RD_EOF_Y);
		}
		cmdlst_stripe[i].irq_mode = irq;
	}
	if (orb_stripe_num == 0)
		return ISP_IPP_ERR;
	cmdlst_stripe[orb_stripe_num - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_ORB_CHANNEL_ID;
	cmdlst_para->stripe_cnt = orb_stripe_num;
	return ISP_IPP_OK;
}

static int cal_multi_layer_stripe_info(
	struct msg_req_orb_request_t *req,
	struct cmdlst_para_t *cmdlst_para,
	unsigned int *orb_layer_stripe_num)
{
	struct cmdlst_stripe_info_t *cmdlst_stripe =
			cmdlst_para->cmd_stripe_info;
	unsigned int i = 0;
	unsigned long long irq = 0;
	unsigned int orb_stripe_num = 0;
	unsigned int orb_stripe_base_num = 0;
	unsigned int cur_layer = 0;
	unsigned int temp_layer_stripe = 0;
	unsigned int stripe_index = 0;
	unsigned int layer_stripe_tmp = 0;
	unsigned int last_layer = req->orb_pyramid_layer - 1;
	struct req_orb_t   *req_orb_tmp = NULL;
	req_orb_tmp = &(req->req_orb_cur);
	for (i = 0; i < req->orb_pyramid_layer; i++) {
		orb_stripe_num  = (i == last_layer) ?
			(orb_stripe_num + 1) : (orb_stripe_num + 2);
		layer_stripe_tmp = 2;
		if (req_orb_tmp->orb_stat_buff[GRID_SCORE_BUFF]) {
			orb_stripe_num += 1;
			layer_stripe_tmp += 1;
		}
		if (req_orb_tmp->orb_stat_buff[KPNUM_NMS_BUFF]) {
			orb_stripe_num += 1;
			layer_stripe_tmp += 1;
		}
	}
	if (req_orb_tmp->orb_stat_buff[KPT_NUM_BUFF])
		orb_stripe_num += 1;
	if (req_orb_tmp->orb_stat_buff[TOTAL_KPT_BUFF])
		orb_stripe_num += 1;
	orb_stripe_base_num = orb_stripe_num;
	if (layer_stripe_tmp == 0 || orb_stripe_num == 0) {
		I("layer_stripe_tmp = %d,orb_stripe_num = %d",
			layer_stripe_tmp, orb_stripe_num);
		return ISP_IPP_ERR;
	}
	*orb_layer_stripe_num = layer_stripe_tmp;
	I("orb_stripe_num = %d,orb_layer_stripe_num = %d",
		orb_stripe_num, layer_stripe_tmp);
	for (i = 0; i < orb_stripe_num ; i++) {
		irq = 0;
		stripe_index = (i >= orb_stripe_base_num) ? (i - orb_stripe_base_num) : i;
		cur_layer = stripe_index / layer_stripe_tmp;
		temp_layer_stripe = (req->orb_pyramid_layer - 1) * layer_stripe_tmp;
		cmdlst_stripe[i].resource_share  = 1 << IPP_CMD_RES_SHARE_ORB;
		cmdlst_stripe[i].is_need_set_sop = 0;
		cmdlst_stripe[i].en_link         = 0;
		cmdlst_stripe[i].ch_link         = 0;
		cmdlst_stripe[i].ch_link_act_nbr = 0;
		cmdlst_stripe[i].is_last_stripe  = 0;
		cmdlst_stripe[i].hw_priority     = CMD_PRIO_LOW;
		if (((stripe_index % layer_stripe_tmp != 0) ||
				(stripe_index > temp_layer_stripe))) {
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_WR_EOF_CMDLST);
			cmdlst_stripe[i].irq_mode  = irq;
		} else {
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_RD_EOF_CMDLST);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_DONE_IRQ);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_WR_EOF_LEP);
			irq = irq | (((unsigned long long)(1)) <<
					IPP_ORB_CVDR_VP_RD_EOF_Y);
			if (req_orb_tmp->streams[cur_layer][BO_ORB_Y].buffer)
				irq = irq | (((unsigned long long)(1)) <<
						IPP_ORB_CVDR_VP_WR_EOF_Y_DS);
			cmdlst_stripe[i].irq_mode = irq;
		}
	}
	cmdlst_stripe[orb_stripe_num - 1].is_last_stripe  = 1;
	cmdlst_para->channel_id = IPP_ORB_CHANNEL_ID;
	cmdlst_para->stripe_cnt = orb_stripe_num;
	return ISP_IPP_OK;
}

static int seg_ipp_cfg_orb_cmdlst_para(
	struct msg_req_orb_request_t *req,
	struct enh_orb_cfg_t *enh_orb_cfg,
	struct cmdlst_para_t *cmdlst_para)
{
	unsigned int orb_layer_stripe_num = 0;
	enum frame_flag_e frame_flag = FRAME_CUR;
	if (req->work_mode == 0) {
		loge_if_ret(cal_multi_layer_stripe_info(req, cmdlst_para,
				&orb_layer_stripe_num));
		loge_if_ret(df_sched_prepare(cmdlst_para));
		loge_if_ret(orb_multi_layer_cmdlst_para(cmdlst_para, req,
				enh_orb_cfg, orb_layer_stripe_num, frame_flag));
	} else if (req->work_mode == 1) {
		loge_if_ret(cal_split_stripe_info(req, cmdlst_para));
		loge_if_ret(df_sched_prepare(cmdlst_para));
		loge_if_ret(orb_multi_stripe_cmdlst_para(cmdlst_para, req,
				enh_orb_cfg, frame_flag));
	}
	return ISP_IPP_OK;
}

static int df_size_split_ipp_orb_stripe(struct ipp_stream_t *stream,
	struct ipp_orb_stripe_info_t *p_size_info)
{
	unsigned int width = stream->width;
	unsigned int stripe_width;
	p_size_info->stripe_cnt = 2;
	p_size_info->stripe_height = stream->height;
	p_size_info->stride = ALIGN_UP(stream->width, CVDR_ALIGN_BYTES);
	p_size_info->stripe_width[0] = (stream->width / 2 + 15) / 16 * 16 + 32;
	p_size_info->overlap_left[0] = 0;
	p_size_info->overlap_right[0] = 32;
	p_size_info->stripe_start_point[0] = 0;
	stripe_width = p_size_info->stripe_width[0];
	p_size_info->stripe_width[1] = width - stripe_width + 64;
	p_size_info->overlap_left[1] = 32;
	p_size_info->overlap_right[1] = 0;
	p_size_info->stripe_start_point[1] = p_size_info->stripe_width[0] - 64;
	return ISP_IPP_OK;
}

int orb_request_handler(
	struct msg_req_orb_request_t *orb_request)
{
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	D("+\n");
	if (orb_request == NULL) {
		E(" failed : orb_request is null");
		return ISP_IPP_ERR;
	}
	ret = orb_check_request_para(orb_request);
	if (ret != 0) {
		E("failed : Error request data");
		return ISP_IPP_ERR;
	}
	loge_if_ret(orb_request_dump(orb_request));
	struct ipp_orb_stripe_info_t *orb_stripe_info = NULL;
	orb_stripe_info =
		kmalloc(sizeof(struct ipp_orb_stripe_info_t), GFP_KERNEL);
	if (orb_stripe_info == NULL) {
		E(" fail to kmalloc orb_stripe_info");
		return ISP_IPP_ERR;
	}
	loge_if(memset_s(orb_stripe_info, sizeof(struct ipp_orb_stripe_info_t),
			0, sizeof(struct ipp_orb_stripe_info_t)));
	ret = cpe_mem_get(MEM_ID_ORB_CFG_TAB, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d\n", MEM_ID_ORB_CFG_TAB);
		goto fail_get_mem;
	}
	struct enh_orb_cfg_t *enh_orb_cfg =
		(struct enh_orb_cfg_t *)(uintptr_t)va;
	ret = memset_s(enh_orb_cfg, sizeof(struct enh_orb_cfg_t),
			0, sizeof(struct enh_orb_cfg_t));
	if (ret != 0) {
		pr_err("[%s] memset_s error!\n", __func__);
		goto fail_get_mem;
	}
	ret = cpe_mem_get(MEM_ID_CMDLST_PARA_ORB, &va, &da);
	if (ret != 0) {
		E(" Failed : cpe_mem_get %d", MEM_ID_CMDLST_PARA_ORB);
		goto fail_get_mem;
	}
	struct cmdlst_para_t *orb_cmdlst_para =
		(struct cmdlst_para_t *)(uintptr_t)va;
	ret = memset_s(orb_cmdlst_para, sizeof(struct cmdlst_para_t),
			0, sizeof(struct cmdlst_para_t));
	if (ret != 0) {
		pr_err("[%s] memset_s error!\n", __func__);
		goto fail_get_mem;
	}
	if (orb_request->work_mode == 1) {
		ret = df_size_split_ipp_orb_stripe(
				&orb_request->req_orb_cur.streams[0][BI_ORB_Y],
				orb_stripe_info);
		if (ret  != 0) {
			E(" Failed : df_size_split_ipp_orb_stripe");
			goto fail_get_mem;
		}
	}
	seg_ipp_orb_module_config(orb_request, enh_orb_cfg, orb_stripe_info);
	ret = seg_ipp_cfg_orb_cmdlst_para(orb_request, enh_orb_cfg, orb_cmdlst_para);
	if (ret != 0) {
		E("Failed : seg_ipp_cfg_orb_cmdlst_para");
		goto fail_get_mem;
	}
	ret = df_sched_start(orb_cmdlst_para);
	if (ret != 0) {
		E("Failed : df_sched_start");
		goto fail_get_mem;
	}
	cpe_mem_free(MEM_ID_ORB_CFG_TAB);
	D("-\n");
	kfree(orb_stripe_info);
	return ret;
fail_get_mem:
	cpe_mem_free(MEM_ID_ORB_CFG_TAB);
	kfree(orb_stripe_info);
	return ret;
}

static int orb_check_request_para(struct msg_req_orb_request_t *req)
{
	unsigned int orb_pyramid_layer;
	unsigned int i = 0;
	struct orb_req_block_size_t *blk_size = NULL;
	blk_size = &req->req_orb_cur.reg_cfg[i].gridstat_cfg.block_size;
	if (req == NULL) {
		E(" failed : orb_request is null");
		return ISP_IPP_ERR;
	}
	if (req->orb_pyramid_layer == 0
		|| req->orb_pyramid_layer > 8) {
		E(" failed : pyramid_layer is incorrect");
		return ISP_IPP_ERR;
	}
	if ((req->req_orb_cur.orb_stat_buff[DESC_BUFF] == 0
			&& req->work_frame == CUR_ONLY) ||
		((req->req_orb_ref.orb_stat_buff[DESC_BUFF] == 0
				|| req->req_orb_cur.orb_stat_buff[DESC_BUFF] == 0)
			&& req->work_frame == CUR_REF)) {
		E(" failed : orb_stat_buff is incorrect");
		return ISP_IPP_ERR;
	}
	orb_pyramid_layer = (req->work_mode == 1) ?
		1 : (req->orb_pyramid_layer);
	for (i = 0; i < orb_pyramid_layer; i++) {
		if ((blk_size->blk_h_size == 0) ||
			(blk_size->blk_v_size == 0)) {
			E(" failed : blk_h/v_size is zero");
			return ISP_IPP_ERR;
		}
	}
	for (i = 0; i < orb_pyramid_layer; i++)
		if ((req->req_orb_cur.streams[i][BI_ORB_Y].buffer == 0)
			|| (req->req_orb_ref.streams[i][BI_ORB_Y].buffer == 0
				&& req->work_frame == CUR_REF)) {
			E(" orb_y_in is zero");
			return ISP_IPP_ERR;
		}
	return ISP_IPP_OK;
}

static int orb_request_dump(struct msg_req_orb_request_t *req)
{
#if FLAG_LOG_DEBUG
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int stripe_cnt;
	stripe_cnt = (req->work_mode == 1) ?
		1 : (req->orb_pyramid_layer);
	I("size_of_orb_req = %d\n",
		(int)(sizeof(struct msg_req_orb_request_t)));
	I("req->frame_num = %d\n", req->frame_num);
	D("req->orb_pyramid_layer = %d\n", req->orb_pyramid_layer);
	for (i = 0; i < stripe_cnt; i++) {
		for (j = 0; j < ORB_STREAM_MAX; j++) {
			D("orb streams[%d][%d].width = %d\n",
				i, j, req->req_orb_cur.streams[i][j].width);
			D("orb streams[%d][%d].height = %d\n",
				i, j, req->req_orb_cur.streams[i][j].height);
			D("orb streams[%d][%d].stride = %d\n",
				i, j, req->req_orb_cur.streams[i][j].stride);
			D("orb streams[%d][%d].buffer = 0x%x",
				i, j, req->req_orb_cur.streams[i][j].buffer);
			D("orb streams[%d][%d].format = %d\n",
				i, j, req->req_orb_cur.streams[i][j].format);
		}
		D("reg_cfg[%d]: pyramid_en = %d gsblur_en = %d", i,
			req->req_orb_cur.reg_cfg[i].ctrl.pyramid_en,
			req->req_orb_cur.reg_cfg[i].ctrl.gsblur_en);
		D("reg_cfg[%d]: fast_en = %d nms_en = %d", i,
			req->req_orb_cur.reg_cfg[i].ctrl.fast_en,
			req->req_orb_cur.reg_cfg[i].ctrl.nms_en);
		D("reg_cfg[%d]: orient_en = %d brief_en = %d", i,
			req->req_orb_cur.reg_cfg[i].ctrl.brief_en,
			req->req_orb_cur.reg_cfg[i].ctrl.freak_en);
		D("reg_cfg[%d]: gridstat_en = %d undistort_en = %d", i,
			req->req_orb_cur.reg_cfg[i].ctrl.gridstat_en,
			req->req_orb_cur.reg_cfg[i].ctrl.undistort_en);
		D("reg_cfg[%d]: first_stripe = %d first_layer = %d", i,
			req->req_orb_cur.reg_cfg[i].ctrl.first_stripe,
			req->req_orb_cur.reg_cfg[i].ctrl.first_layer);
		D("size_cfg[%d]:width = %d height = %d", i,
			req->req_orb_cur.reg_cfg[i].size_cfg.width,
			req->req_orb_cur.reg_cfg[i].size_cfg.height);
		D("reg_cfg[%d].pattern_cfg.pattern_x0[0] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x0[0]);
		D("reg_cfg[%d].pattern_cfg.pattern_y0[0]  = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y0[0]);
		D("reg_cfg[%d].pattern_cfg.pattern_x1[0]   = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x1[0]);
		D("reg_cfg[%d].pattern_cfg.pattern_y1[0]   = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y1[0]);
		D("reg_cfg[%d].pattern_cfg.pattern_x0[50]  = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x0[50]);
		D("reg_cfg[%d].pattern_cfg.pattern_y0[50]  = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y0[50]);
		D("reg_cfg[%d].pattern_cfg.pattern_x1[50]   = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x1[50]);
		D("reg_cfg[%d].pattern_cfg.pattern_y1[50]   = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y1[50]);
		D("reg_cfg[%d].pattern_cfg.pattern_x0[150]  = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x0[150]);
		D("reg_cfg[%d].pattern_cfg.pattern_y0[150] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y0[150]);
		D("reg_cfg[%d].pattern_cfg.pattern_x1[150] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x1[150]);
		D("reg_cfg[%d].pattern_cfg.pattern_y1[150] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y1[150]);
		D("reg_cfg[%d].pattern_cfg.pattern_x0[255] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x0[255]);
		D("reg_cfg[%d].pattern_cfg.pattern_y0[255] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y0[255]);
		D("reg_cfg[%d].pattern_cfg.pattern_x1[255] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_x1[255]);
		D("reg_cfg[%d].pattern_cfg.pattern_y1[255] = %d\n", i,
			req->req_orb_cur.reg_cfg[i].pattern_cfg.pattern_y1[255]);
	}
	D("req->secure_flag = %d\n", req->secure_flag);
	D("req->work_mode  = %d\n", req->work_mode);
	D("req->orb_rate_value = %d\n", req->orb_rate_value);
	D("req->work_module = %d\n", req->work_module);
#else
	I("req->slam_pyramid_layer = %d\n", req->orb_pyramid_layer);
#endif
	return ISP_IPP_OK;
}

#pragma GCC diagnostic pop
