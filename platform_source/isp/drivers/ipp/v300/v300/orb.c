
#include <linux/printk.h>
#include <linux/string.h>
#include "ipp.h"
#include "orb_drv.h"
#include "orb_drv_priv.h"
#include "orb_reg_offset.h"
#include "orb_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_ORB_DRV

static int orb_set_ctrl(struct orb_dev_t *dev, struct orb_ctrl_cfg_t *ctrl)
{
	union U_ORB_CFG temp;

	if (dev == NULL || ctrl == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	if ((ctrl->brief_en == 1) && (ctrl->freak_en == 1)) {
		E("brief and freak cannot be enabled at the same time.\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.pyramid_en = ctrl->pyramid_en;
	temp.bits.gsblur_en = ctrl->gsblur_en;
	temp.bits.fast_en = ctrl->fast_en;
	temp.bits.nms_en = ctrl->nms_en;
	temp.bits.orient_en = ctrl->orient_en;
	temp.bits.brief_en = ctrl->brief_en;
	temp.bits.freak_en = ctrl->freak_en;
	temp.bits.gridstat_en = ctrl->gridstat_en;
	temp.bits.undistort_en = ctrl->undistort_en;
	temp.bits.first_stripe = ctrl->first_stripe;
	temp.bits.first_layer = ctrl->first_layer;
	CMDLST_SET_REG(dev->base_addr + ORB_ORB_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}
static int orb_set_size(struct orb_dev_t *dev, struct orb_size_cfg_t *size)
{
	union U_IMAGE_SIZE temp_size;
	union U_STAT_RANGE temp_range;

	if (dev == NULL || size == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_size.u32 = 0;
	temp_size.bits.width  = size->width;
	temp_size.bits.height = size->height;
	temp_range.bits.stat_right = size->stat_range.stat_right;
	temp_range.bits.stat_left  = size->stat_range.stat_left;
	CMDLST_SET_REG(dev->base_addr + ORB_IMAGE_SIZE_REG,
		temp_size.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_STAT_RANGE_REG,
		temp_range.u32);
	return ISP_IPP_OK;
}

static int orb_set_pyramid_scaler(
	struct orb_dev_t *dev, struct orb_pyramid_scaler_cfg_t *pyramid_scaler)
{
	union U_PYRAMID_INC_CFG temp_scl_inc;
	union U_PYRAMID_VCROP_CFGB temp_crop_bottom;
	union U_PYRAMID_VCROP_CFGT temp_crop_top;
	union U_PYRAMID_HCROP_CFGR temp_crop_right;
	union U_PYRAMID_HCROP_CFGL temp_crop_left;

	if (dev == NULL || pyramid_scaler == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_scl_inc.u32 = 0;
	temp_crop_bottom.u32 = 0;
	temp_crop_top.u32 = 0;
	temp_crop_right.u32 = 0;
	temp_crop_left.u32 = 0;
	temp_scl_inc.bits.scl_inc = pyramid_scaler->scl_inc;
	CMDLST_SET_REG(dev->base_addr + ORB_PYRAMID_INC_CFG_REG,
		temp_scl_inc.u32);
	temp_crop_bottom.bits.scl_vbottom = pyramid_scaler->scl_vbottom;
	CMDLST_SET_REG(dev->base_addr + ORB_PYRAMID_VCROP_CFGB_REG,
		temp_crop_bottom.u32);
	temp_crop_top.bits.scl_vtop = pyramid_scaler->scl_vtop;
	CMDLST_SET_REG(dev->base_addr + ORB_PYRAMID_VCROP_CFGT_REG,
		temp_crop_top.u32);
	temp_crop_right.bits.scl_hright = pyramid_scaler->scl_hright;
	CMDLST_SET_REG(dev->base_addr + ORB_PYRAMID_HCROP_CFGR_REG,
		temp_crop_right.u32);
	temp_crop_left.bits.scl_hleft = pyramid_scaler->scl_hleft;
	CMDLST_SET_REG(dev->base_addr + ORB_PYRAMID_HCROP_CFGL_REG,
		temp_crop_left.u32);
	return ISP_IPP_OK;
}

static int orb_set_gsblur_coef(
	struct orb_dev_t *dev, struct orb_gsblur_coef_cfg_t *gsblur_coef)
{
	union U_GSBLUR_COEF_01 temp_gsblur_01;
	union U_GSBLUR_COEF_23 temp_gsblur_23;

	if (dev == NULL || gsblur_coef == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_gsblur_01.u32 = 0;
	temp_gsblur_23.u32 = 0;
	temp_gsblur_01.bits.coeff_gauss_0  = gsblur_coef->coeff_gauss_0;
	temp_gsblur_01.bits.coeff_gauss_1  = gsblur_coef->coeff_gauss_1;
	temp_gsblur_23.bits.coeff_gauss_2  = gsblur_coef->coeff_gauss_2;
	temp_gsblur_23.bits.coeff_gauss_3  = gsblur_coef->coeff_gauss_3;
	CMDLST_SET_REG(dev->base_addr + ORB_GSBLUR_COEF_01_REG,
		temp_gsblur_01.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_GSBLUR_COEF_23_REG,
		temp_gsblur_23.u32);
	return ISP_IPP_OK;
}

static int orb_set_fast(
	struct orb_dev_t *dev,
	struct orb_fast_coef_t *fast_coef)
{
	union U_THRESHOLD_CFG temp;

	if (dev == NULL || fast_coef == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.ini_th = fast_coef->ini_th;
	temp.bits.min_th = fast_coef->min_th;
	CMDLST_SET_REG(dev->base_addr + ORB_THRESHOLD_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int orb_set_nms_win(struct orb_dev_t *dev,
	struct orb_nms_win_t *nms_win)
{
	union U_NMS_WIN_CFG temp;

	if (dev == NULL || nms_win == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.nmscell_v = nms_win->nmscell_v;
	temp.bits.nmscell_h = nms_win->nmscell_h;
	CMDLST_SET_REG(dev->base_addr + ORB_NMS_WIN_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int orb_set_gridstat(
	struct orb_dev_t *dev, struct orb_gridstat_cfg_t *gridstat_cfg)
{
	union U_BLOCK_NUM_CFG  temp_blk_num;
	union U_BLOCK_SIZE_CFG temp_blk_size;

	if (dev == NULL || gridstat_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_blk_num.u32 = 0;
	temp_blk_size.u32 = 0;
	temp_blk_num.bits.blk_v_num = gridstat_cfg->block_num.blk_v_num;
	temp_blk_num.bits.blk_h_num = gridstat_cfg->block_num.blk_h_num;
	temp_blk_size.bits.blk_v_size = gridstat_cfg->block_size.blk_v_size;
	temp_blk_size.bits.blk_h_size = gridstat_cfg->block_size.blk_h_size;
	CMDLST_SET_REG(dev->base_addr + ORB_BLOCK_NUM_CFG_REG,
		temp_blk_num.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_BLOCK_SIZE_CFG_REG,
		temp_blk_size.u32);
	return ISP_IPP_OK;
}

static int orb_set_octree(
	struct orb_dev_t *dev, struct orb_octree_cfg_t *octree_cfg)
{
	unsigned int i = 0;
	union U_OCTREE_CFG	   temp_octree_cfg;
	union U_SCORE_THESHOLD temp_score_theshold;
	union U_INC_LUT_CFG    temp_inc_lut_cfg;
	union U_LAYER0_START_H temp_h_cfg;
	union U_LAYER0_START_V temp_v_cfg;
	union U_BLOCK_SIZE_CFG_INV temp_block_size_cfg_inv;

	if (dev == NULL || octree_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_octree_cfg.u32 = 0;
	temp_score_theshold.u32 = 0;
	temp_inc_lut_cfg.u32 = 0;
	temp_h_cfg.u32 = 0;
	temp_v_cfg.u32 = 0;
	temp_block_size_cfg_inv.u32 = 0;
	temp_block_size_cfg_inv.bits.blk_h_size_inv =
		octree_cfg->blk_size_inv.blk_h_size_inv;
	temp_block_size_cfg_inv.bits.blk_v_size_inv =
		octree_cfg->blk_size_inv.blk_v_size_inv;
	temp_octree_cfg.bits.max_kpnum =
		octree_cfg->max_kpnum;
	temp_octree_cfg.bits.grid_max_kpnum =
		octree_cfg->grid_max_kpnum;
	temp_octree_cfg.bits.flag_10_11 =
		octree_cfg->flag_10_11;
	temp_inc_lut_cfg.bits.inc_level =
		octree_cfg->inc_level;
	temp_h_cfg.bits.start_blk_h_offset =
		octree_cfg->layer0_start.start_blk_h_offset;
	temp_h_cfg.bits.start_blk_h_num =
		octree_cfg->layer0_start.start_blk_h_num;
	temp_v_cfg.bits.start_blk_v_offset =
		octree_cfg->layer0_start.start_blk_v_offset;
	temp_v_cfg.bits.start_blk_v_num =
		octree_cfg->layer0_start.start_blk_v_num;
	temp_block_size_cfg_inv.bits.blk_h_size_inv =
		octree_cfg->blk_size_inv.blk_h_size_inv;
	temp_block_size_cfg_inv.bits.blk_v_size_inv =
		octree_cfg->blk_size_inv.blk_v_size_inv;
	CMDLST_SET_REG(dev->base_addr + ORB_BLOCK_SIZE_CFG_INV_REG,
		temp_block_size_cfg_inv.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_OCTREE_CFG_REG,
		temp_octree_cfg.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_INC_LUT_CFG_REG,
		temp_inc_lut_cfg.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_LAYER0_START_H_REG,
		temp_h_cfg.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_LAYER0_START_V_REG,
		temp_v_cfg.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_BLOCK_SIZE_CFG_INV_REG,
		temp_block_size_cfg_inv.u32);

	CMDLST_SET_REG_INCR(dev->base_addr + ORB_SCORE_THESHOLD_0_REG,
		ORB_SCORE_TH_RANGE, 0, 0);

	for (i = 0; i < ORB_SCORE_TH_RANGE; i++) {
		temp_score_theshold.bits.score_th = octree_cfg->score_th[i];
		CMDLST_SET_REG_DATA(temp_score_theshold.u32);
	}

	return ISP_IPP_OK;
}

static int orb_set_undistort(struct orb_dev_t *dev,
	struct orb_undistort_cfg_t *undistort_cfg)
{
	union U_UNDISTORT_CX    temp_cx;
	union U_UNDISTORT_CY    temp_cy;
	union U_UNDISTORT_FX    temp_fx;
	union U_UNDISTORT_FY    temp_fy;
	union U_UNDISTORT_INVFX temp_invfx;
	union U_UNDISTORT_INVFY temp_invfy;
	union U_UNDISTORT_K1    temp_k1;
	union U_UNDISTORT_K2    temp_k2;
	union U_UNDISTORT_K3    temp_k3;
	union U_UNDISTORT_P1    temp_p1;
	union U_UNDISTORT_P2    temp_p2;

	if (dev == NULL || undistort_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_cx.u32 = 0;
	temp_cy.u32 = 0;
	temp_fx.u32 = 0;
	temp_fy.u32 = 0;
	temp_invfx.u32 = 0;
	temp_invfy.u32 = 0;
	temp_k1.u32 = 0;
	temp_k2.u32 = 0;
	temp_k3.u32 = 0;
	temp_p1.u32 = 0;
	temp_p2.u32 = 0;
	temp_cx.bits.cx = undistort_cfg->cx;
	temp_cy.bits.cy = undistort_cfg->cy;
	temp_fx.bits.fx = undistort_cfg->fx;
	temp_fy.bits.fy = undistort_cfg->fy;
	temp_invfx.bits.invfx = undistort_cfg->invfx;
	temp_invfy.bits.invfy = undistort_cfg->invfy;
	temp_k1.bits.k1 = undistort_cfg->k1;
	temp_k2.bits.k2 = undistort_cfg->k2;
	temp_k3.bits.k3 = undistort_cfg->k3;
	temp_p1.bits.p1 = undistort_cfg->p1;
	temp_p2.bits.p2 = undistort_cfg->p2;
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_CX_REG,
		temp_cx.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_CY_REG,
		temp_cy.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_FX_REG,
		temp_fx.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_FY_REG,
		temp_fy.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_INVFX_REG,
		temp_invfx.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_INVFY_REG,
		temp_invfy.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_K1_REG,
		temp_k1.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_K2_REG,
		temp_k2.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_K3_REG,
		temp_k3.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_P1_REG,
		temp_p1.u32);
	CMDLST_SET_REG(dev->base_addr + ORB_UNDISTORT_P2_REG,
		temp_p2.u32);
	return ISP_IPP_OK;
}

static int orb_set_pattern(
	struct orb_dev_t *dev, struct orb_pattern_cfg_t *pattern_cfg)
{
	unsigned int i = 0;
	union U_BRIEF_PATTERN temp_pattern;

	if (dev == NULL || pattern_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_pattern.u32 = 0;
	CMDLST_SET_REG_INCR(dev->base_addr + ORB_BRIEF_PATTERN_0_REG,
		ORB_PATTERN_RANGE, 0, 0);
	for (i = 0; i < ORB_PATTERN_RANGE; i++) {
		temp_pattern.bits.pattern_x0 = pattern_cfg->pattern_x0[i];
		temp_pattern.bits.pattern_y0 = pattern_cfg->pattern_y0[i];
		temp_pattern.bits.pattern_x1 = pattern_cfg->pattern_x1[i];
		temp_pattern.bits.pattern_y1 = pattern_cfg->pattern_y1[i];

		CMDLST_SET_REG_DATA(temp_pattern.u32);
	}

	return ISP_IPP_OK;
}

static int orb_set_cvdr(
	struct orb_dev_t *dev, struct orb_cvdr_cfg_t *cvdr_cfg)
{
	union U_CVDR_RD_CFG temp_cvdr_rd_cfg;
	union U_CVDR_RD_LWG temp_cvdr_rd_lwg;
	union U_CVDR_RD_FHG temp_cvdr_rd_fhg;
	union U_CVDR_RD_AXI_FS temp_cvdr_rd_axi_fs;
	union U_CVDR_RD_AXI_LINE temp_cvdr_rd_axi_line;
	union U_CVDR_WR_CFG temp_cvdr_wr_cfg;
	union U_CVDR_WR_AXI_FS temp_cvdr_wr_axi_fs;
	union U_CVDR_WR_AXI_LINE temp_cvdr_wr_axi_line;

	if (dev == NULL || cvdr_cfg == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	temp_cvdr_rd_cfg.u32 = 0;
	temp_cvdr_rd_lwg.u32 = 0;
	temp_cvdr_rd_fhg.u32 = 0;
	temp_cvdr_rd_axi_fs.u32 = 0;
	temp_cvdr_rd_axi_line.u32 = 0;
	temp_cvdr_wr_cfg.u32 = 0;
	temp_cvdr_wr_axi_fs.u32 = 0;
	temp_cvdr_wr_axi_line.u32 = 0;
	temp_cvdr_rd_cfg.bits.vprd_pixel_format =
		cvdr_cfg->rd_cfg.rd_base_cfg.vprd_pixel_format;
	temp_cvdr_rd_cfg.bits.vprd_pixel_expansion =
		cvdr_cfg->rd_cfg.rd_base_cfg.vprd_pixel_expansion;
	temp_cvdr_rd_cfg.bits.vprd_allocated_du =
		cvdr_cfg->rd_cfg.rd_base_cfg.vprd_allocated_du;
	temp_cvdr_rd_cfg.bits.vprd_last_page =
		cvdr_cfg->rd_cfg.rd_base_cfg.vprd_last_page;
	temp_cvdr_rd_lwg.bits.vprd_line_size =
		cvdr_cfg->rd_cfg.rd_lwg.vprd_line_size;
	temp_cvdr_rd_lwg.bits.vprd_horizontal_blanking =
		cvdr_cfg->rd_cfg.rd_lwg.vprd_horizontal_blanking;
	temp_cvdr_rd_fhg.bits.vprd_frame_size =
		cvdr_cfg->rd_cfg.pre_rd_fhg.pre_vprd_frame_size;
	temp_cvdr_rd_fhg.bits.vprd_vertical_blanking =
		cvdr_cfg->rd_cfg.pre_rd_fhg.pre_vprd_vertical_blanking;
	temp_cvdr_rd_axi_fs.bits.vprd_axi_frame_start =
		cvdr_cfg->rd_cfg.rd_axi_fs.vprd_axi_frame_start;
	temp_cvdr_rd_axi_line.bits.vprd_line_stride =
		cvdr_cfg->rd_cfg.rd_axi_line.vprd_line_stride;
	temp_cvdr_rd_axi_line.bits.vprd_line_wrap =
		cvdr_cfg->rd_cfg.rd_axi_line.vprd_line_wrap;
	temp_cvdr_wr_cfg.bits.vpwr_pixel_format =
		cvdr_cfg->wr_cfg.wr_base_cfg.vpwr_pixel_format;
	temp_cvdr_wr_cfg.bits.vpwr_pixel_expansion =
		cvdr_cfg->wr_cfg.wr_base_cfg.vpwr_pixel_expansion;
	temp_cvdr_wr_cfg.bits.vpwr_last_page =
		cvdr_cfg->wr_cfg.wr_base_cfg.vpwr_last_page;
	temp_cvdr_wr_axi_fs.bits.vpwr_address_frame_start =
	(cvdr_cfg->wr_cfg.pre_wr_axi_fs.pre_vpwr_address_frame_start >> 2) >> 2;
	temp_cvdr_wr_axi_line.bits.vpwr_line_stride =
		cvdr_cfg->wr_cfg.wr_axi_line.vpwr_line_stride;
	temp_cvdr_wr_axi_line.bits.vpwr_line_start_wstrb =
		cvdr_cfg->wr_cfg.wr_axi_line.vpwr_line_start_wstrb;
	temp_cvdr_wr_axi_line.bits.vpwr_line_wrap =
		cvdr_cfg->wr_cfg.wr_axi_line.vpwr_line_wrap;
	CMDLST_SET_REG(dev->base_addr  + ORB_CVDR_RD_CFG_REG,
		temp_cvdr_rd_cfg.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_CVDR_RD_LWG_REG,
		temp_cvdr_rd_lwg.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_PRE_CVDR_RD_FHG_REG,
		temp_cvdr_rd_fhg.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_CVDR_RD_AXI_FS_REG,
		temp_cvdr_rd_axi_fs.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_CVDR_RD_AXI_LINE_REG,
		temp_cvdr_rd_axi_line.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_CVDR_WR_CFG_REG,
		temp_cvdr_wr_cfg.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_PRE_CVDR_WR_AXI_FS_REG,
		temp_cvdr_wr_axi_fs.u32);
	CMDLST_SET_REG(dev->base_addr  + ORB_CVDR_WR_AXI_LINE_REG,
		temp_cvdr_wr_axi_line.u32);
	return ISP_IPP_OK;
}

static int orb_do_config(struct orb_dev_t *dev, struct cfg_tab_orb_t *tab_orb)
{
	if (dev == NULL || tab_orb == NULL) {
		E("params is NULL!!\n");
		return ISP_IPP_ERR;
	}

	if (tab_orb->ctrl.to_use) {
		loge_if_ret(orb_set_ctrl(dev,
			&(tab_orb->ctrl)));
		tab_orb->ctrl.to_use = 0;
	}

	if (tab_orb->size_cfg.to_use) {
		loge_if_ret(orb_set_size(dev,
			&(tab_orb->size_cfg)));
		tab_orb->size_cfg.to_use = 0;
	}

	if (tab_orb->pyramid_scaler_cfg.to_use) {
		loge_if_ret(orb_set_pyramid_scaler(dev,
			&(tab_orb->pyramid_scaler_cfg)));
		tab_orb->pyramid_scaler_cfg.to_use = 0;
	}

	if (tab_orb->gsblur_coef_cfg.to_use) {
		loge_if_ret(orb_set_gsblur_coef(dev,
			&(tab_orb->gsblur_coef_cfg)));
		tab_orb->gsblur_coef_cfg.to_use = 0;
	}

	if ((tab_orb->fast_coef.to_use) || (tab_orb->nms_win.to_use)) {
		loge_if_ret(orb_set_fast(dev,
			&(tab_orb->fast_coef)));
		tab_orb->fast_coef.to_use = 0;
	}

	if (tab_orb->nms_win.to_use) {
		loge_if_ret(orb_set_nms_win(dev,
			&(tab_orb->nms_win)));
		tab_orb->nms_win.to_use = 0;
	}

	if (tab_orb->gridstat_cfg.to_use) {
		loge_if_ret(orb_set_gridstat(dev,
			&(tab_orb->gridstat_cfg)));
		tab_orb->gridstat_cfg.to_use = 0;
	}

	if (tab_orb->octree_cfg.to_use) {
		loge_if_ret(orb_set_octree(dev,
			&(tab_orb->octree_cfg)));
		tab_orb->octree_cfg.to_use = 0;
	}

	if (tab_orb->undistort_cfg.to_use) {
		loge_if_ret(orb_set_undistort(dev,
			&(tab_orb->undistort_cfg)));
		tab_orb->undistort_cfg.to_use = 0;
	}

	if (((tab_orb->ctrl.brief_en == 1) || (tab_orb->ctrl.freak_en == 1))
		&& tab_orb->pattern_to_use == 1) {
		loge_if_ret(orb_set_pattern(dev, &(tab_orb->pattern_cfg)));
		tab_orb->pattern_to_use = 0;
	}

	if (tab_orb->cvdr_cfg.to_use) {
		loge_if_ret(orb_set_cvdr(dev,
			&(tab_orb->cvdr_cfg)));
		tab_orb->cvdr_cfg.to_use = 0;
	}

	return ISP_IPP_OK;
}


int orb_prepare_cmd(struct orb_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_orb_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL) {
		E("failed : null pointer");
		return ISP_IPP_ERR;
	}

	dev->cmd_buf = cmd_buf;
	loge_if_ret(orb_do_config(dev, table));
	return 0;
}

static struct orb_ops_t orb_ops = {
	.prepare_cmd   = orb_prepare_cmd,
};

struct orb_dev_t g_orb_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_ORB,
		.ops = &orb_ops,
	},
};



/* ********************************* END ********************************* */



