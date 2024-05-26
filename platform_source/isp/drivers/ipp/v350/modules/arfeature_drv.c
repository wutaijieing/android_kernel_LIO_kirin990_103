/*********************************************************************
 * Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
 * File name    arfeature_drv.c
 * Description:
 *
 * Date         2020-04-11 20:37:22
 ***********************************************************************/
#include <linux/printk.h>
#include "ipp.h"
#include "arfeature_drv.h"
#include "arfeature_drv_priv.h"
#include "arfeature_reg_offset.h"
#include "arfeature_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_ARFEATURE_DRV
#define REG_ADDR_LEN 4

// arfeature
static int arfeature_set_ctrl(struct arfeature_dev_t *dev, struct arfeature_top_cfg_t *top_cfg)
{
	union u_top_cfg temp;

	if (dev == NULL || top_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mode = top_cfg->mode;
	temp.bits.orient_en = top_cfg->orient_en;
	temp.bits.layer = top_cfg->layer;
	temp.bits.iter_num = top_cfg->iter_num;
	temp.bits.first_detect = top_cfg->first_detect;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_TOP_CFG_REG, temp.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_img_size(struct arfeature_dev_t *dev, struct arfeature_img_size_t *size_cfg)
{
	union u_image_size temp;

	if (dev == NULL || size_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	if (size_cfg->img_width % 2 != 1 || size_cfg->img_height % 2 != 1) {
		loge("arfeature_set_img_size error!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.width = size_cfg->img_width;
	temp.bits.height = size_cfg->img_height;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_IMAGE_SIZE_REG, temp.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_block(struct arfeature_dev_t *dev, struct arfeature_blk_cfg_t *blk_cfg)
{
	union u_block_num_cfg blk_num;
	union u_block_size_cfg_inv blk_size_inv;

	if (dev == NULL || blk_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	blk_num.u32 = 0;
	blk_size_inv.u32 = 0;
	blk_num.bits.blk_h_num = blk_cfg->blk_h_num;
	blk_num.bits.blk_v_num = blk_cfg->blk_v_num;
	blk_size_inv.bits.blk_h_size_inv = blk_cfg->blk_h_size_inv;
	blk_size_inv.bits.blk_v_size_inv = blk_cfg->blk_v_size_inv;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_BLOCK_NUM_CFG_REG, blk_num.u32);
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_BLOCK_SIZE_CFG_INV_REG, blk_size_inv.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_detect_number_lmt(struct arfeature_dev_t *dev,
		struct arfeature_detect_num_lmt_t *detect_num_lmt_cfg)
{
	union u_detect_number_limit temp;

	if (dev == NULL || detect_num_lmt_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.max_kpnum = detect_num_lmt_cfg->max_kpnum;
	temp.bits.cur_kpnum = detect_num_lmt_cfg->cur_kpnum;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_DETECT_NUMBER_LIMIT_REG, temp.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_sigma_coeff(struct arfeature_dev_t *dev,
									 struct arfeature_sigma_coeff_t *sigma_coeff_cfg)
{
	union u_sigma_coef temp;

	if (dev == NULL || sigma_coeff_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.sigma_ori = sigma_coeff_cfg->sigma_ori;
	temp.bits.sigma_des = sigma_coeff_cfg->sigma_des;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_SIGMA_COEF_REG, temp.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_gauss_coeff(struct arfeature_dev_t *dev,
									 struct arfeature_gauss_coeff_t *gauss_coeff_cfg)
{
	union u_gauss_org gauss_org;
	union u_gsblur_1st gauss_1st;
	union u_gauss_2nd gauss_2nd;

	if (dev == NULL || gauss_coeff_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	gauss_org.u32 = 0;
	gauss_org.bits.gauss_org_0 = gauss_coeff_cfg->gauss_org_0;
	gauss_org.bits.gauss_org_1 = gauss_coeff_cfg->gauss_org_1;
	gauss_org.bits.gauss_org_2 = gauss_coeff_cfg->gauss_org_2;
	gauss_org.bits.gauss_org_3 = gauss_coeff_cfg->gauss_org_3;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_GAUSS_ORG_REG, gauss_org.u32);
	gauss_1st.u32 = 0;
	gauss_1st.bits.gauss_1st_0 = gauss_coeff_cfg->gsblur_1st_0;
	gauss_1st.bits.gauss_1st_1 = gauss_coeff_cfg->gsblur_1st_1;
	gauss_1st.bits.gauss_1st_2 = gauss_coeff_cfg->gsblur_1st_2;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_GSBLUR_1ST_REG, gauss_1st.u32);
	gauss_2nd.u32 = 0;
	gauss_2nd.bits.gauss_2nd_0 = gauss_coeff_cfg->gauss_2nd_0;
	gauss_2nd.bits.gauss_2nd_1 = gauss_coeff_cfg->gauss_2nd_1;
	gauss_2nd.bits.gauss_2nd_2 = gauss_coeff_cfg->gauss_2nd_2;
	gauss_2nd.bits.gauss_2nd_3 = gauss_coeff_cfg->gauss_2nd_3;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_GAUSS_2ND_REG, gauss_2nd.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_dog_edge_thd(struct arfeature_dev_t *dev,
									  struct arfeature_dog_edge_thd_t *dog_edge_thd_cfg)
{
	union u_dog_edge_threshold temp;

	if (dev == NULL || dog_edge_thd_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.edge_threshold = dog_edge_thd_cfg->edge_threshold;
	temp.bits.dog_threshold = dog_edge_thd_cfg->dog_threshold;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_DOG_EDGE_THRESHOLD_REG, temp.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_descriptor_thd(struct arfeature_dev_t *dev,
										struct arfeature_mag_thd_t *mag_thd_cfg)
{
	union u_descriptor_threshold temp;

	if (dev == NULL || mag_thd_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp.u32 = 0;
	temp.bits.mag_threshold = mag_thd_cfg->mag_threshold;
	macro_cmdlst_set_reg(dev->base_addr + AR_FEATURE_DESCRIPTOR_THRESHOLD_REG, temp.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_kpt_lmt(struct arfeature_dev_t *dev,
								 struct arfeature_kpt_lmt_t *kpt_lmt_cfg)
{
	unsigned int i = 0;
	unsigned int j = 0;
	union u_kpt_limit temp;
	unsigned int data_size = ARFEATURE_MAX_BLK_NUM;
	unsigned int reg_size = 0;
	unsigned int index_cnt = 0;

	if (dev == NULL || kpt_lmt_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < (data_size / CMDLST_BURST_MAX_SIZE + 1); i++) {
		reg_size = ((data_size - i * CMDLST_BURST_MAX_SIZE) >
					CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE :
				   (data_size - i * CMDLST_BURST_MAX_SIZE);
		macro_cmdlst_set_reg_incr(dev->base_addr + AR_FEATURE_KPT_LIMIT_0_REG + i * CMDLST_BURST_MAX_SIZE * REG_ADDR_LEN,
								  reg_size, 0, 0);

		for (j = 0; j < reg_size; j++) {
			temp.u32 = 0;
			temp.bits.grid_max_kpnum = kpt_lmt_cfg->grid_max_kpnum[index_cnt];
			temp.bits.grid_dog_threshold = kpt_lmt_cfg->grid_dog_threshold[index_cnt];
			index_cnt++;
			macro_cmdlst_set_reg_data(temp.u32);
		}
	}

	return ISP_IPP_OK;
}

static int arfeature_set_cvdr_rd(struct arfeature_dev_t *dev, struct arfeature_cvdr_cfg_t *cvdr_cfg)
{
	union u_cvdr_rd_cfg temp_cvdr_rd_cfg;
	union u_cvdr_rd_lwg temp_cvdr_rd_lwg;
	union u_cvdr_rd_fhg temp_cvdr_rd_fhg;
	union u_cvdr_rd_axi_fs temp_cvdr_rd_axi_fs;
	union u_cvdr_rd_axi_line temp_cvdr_rd_axi_line;

	if (dev == NULL || cvdr_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_cvdr_rd_cfg.u32 = 0;
	temp_cvdr_rd_lwg.u32 = 0;
	temp_cvdr_rd_fhg.u32 = 0;
	temp_cvdr_rd_axi_fs.u32 = 0;
	temp_cvdr_rd_axi_line.u32 = 0;
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
	temp_cvdr_rd_axi_line.bits.vprd_line_stride_0 =
		cvdr_cfg->rd_cfg.rd_axi_line.vprd_line_stride;
	temp_cvdr_rd_axi_line.bits.vprd_line_wrap_0 =
		cvdr_cfg->rd_cfg.rd_axi_line.vprd_line_wrap;
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_CVDR_RD_CFG_REG, temp_cvdr_rd_cfg.u32);
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_CVDR_RD_LWG_REG, temp_cvdr_rd_lwg.u32);
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_PRE_CVDR_RD_FHG_REG, temp_cvdr_rd_fhg.u32);
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_CVDR_RD_AXI_FS_REG, temp_cvdr_rd_axi_fs.u32);
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_CVDR_RD_AXI_LINE_REG, temp_cvdr_rd_axi_line.u32);
	return ISP_IPP_OK;
}

static int arfeature_set_cvdr_wr(struct arfeature_dev_t *dev, struct arfeature_cvdr_cfg_t *cvdr_cfg)
{
	union u_cvdr_wr_cfg temp_cvdr_wr_cfg;
	union u_pre_cvdr_wr_axi_fs temp_cvdr_wr_axi_fs;
	union u_cvdr_wr_axi_line temp_cvdr_wr_axi_line;

	if (dev == NULL || cvdr_cfg == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	temp_cvdr_wr_cfg.u32 = 0;
	temp_cvdr_wr_axi_fs.u32 = 0;
	temp_cvdr_wr_axi_line.u32 = 0;
	temp_cvdr_wr_cfg.bits.vpwr_pixel_format =
		cvdr_cfg->wr_cfg.wr_base_cfg.vpwr_pixel_format;
	temp_cvdr_wr_cfg.bits.vpwr_pixel_expansion =
		cvdr_cfg->wr_cfg.wr_base_cfg.vpwr_pixel_expansion;
	temp_cvdr_wr_cfg.bits.vpwr_last_page =
		cvdr_cfg->wr_cfg.wr_base_cfg.vpwr_last_page;
	temp_cvdr_wr_axi_fs.bits.pre_vpwr_address_frame_start =
		(cvdr_cfg->wr_cfg.pre_wr_axi_fs.pre_vpwr_address_frame_start >> 2) >> 2;
	temp_cvdr_wr_axi_line.bits.vpwr_line_stride =
		cvdr_cfg->wr_cfg.wr_axi_line.vpwr_line_stride;
	temp_cvdr_wr_axi_line.bits.vpwr_line_start_wstrb =
		cvdr_cfg->wr_cfg.wr_axi_line.vpwr_line_start_wstrb;
	temp_cvdr_wr_axi_line.bits.vpwr_line_wrap =
		cvdr_cfg->wr_cfg.wr_axi_line.vpwr_line_wrap;
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_CVDR_WR_CFG_REG,
						 temp_cvdr_wr_cfg.u32);
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_PRE_CVDR_WR_AXI_FS_REG,
						 temp_cvdr_wr_axi_fs.u32);
	macro_cmdlst_set_reg(dev->base_addr  + AR_FEATURE_CVDR_WR_AXI_LINE_REG,
						 temp_cvdr_wr_axi_line.u32);
	return ISP_IPP_OK;
}

static int arfeature_cfg_reg_part_0(struct arfeature_dev_t *dev, struct cfg_tab_arfeature_t *tab_arfeature)
{
	if (tab_arfeature->size_cfg.to_use) {
		tab_arfeature->size_cfg.to_use = 0;
		loge_if_ret(arfeature_set_img_size(dev, &(tab_arfeature->size_cfg)));
	}

	if (tab_arfeature->blk_cfg.to_use) {
		tab_arfeature->blk_cfg.to_use = 0;
		loge_if_ret(arfeature_set_block(dev, &(tab_arfeature->blk_cfg)));
	}

	if (tab_arfeature->detect_num_lmt_cfg.to_use) {
		tab_arfeature->detect_num_lmt_cfg.to_use = 0;
		loge_if_ret(arfeature_set_detect_number_lmt(dev, &(tab_arfeature->detect_num_lmt_cfg)));
	}

	if (tab_arfeature->sigma_coeff_cfg.to_use) {
		tab_arfeature->sigma_coeff_cfg.to_use = 0;
		loge_if_ret(arfeature_set_sigma_coeff(dev, &(tab_arfeature->sigma_coeff_cfg)));
	}

	if (tab_arfeature->gauss_coeff_cfg.to_use) {
		tab_arfeature->gauss_coeff_cfg.to_use = 0;
		loge_if_ret(arfeature_set_gauss_coeff(dev, &(tab_arfeature->gauss_coeff_cfg)));
	}

	if (tab_arfeature->dog_edge_thd_cfg.to_use) {
		tab_arfeature->dog_edge_thd_cfg.to_use = 0;
		loge_if_ret(arfeature_set_dog_edge_thd(dev, &(tab_arfeature->dog_edge_thd_cfg)));
	}

	if (tab_arfeature->mag_thd_cfg.to_use) {
		tab_arfeature->mag_thd_cfg.to_use = 0;
		loge_if_ret(arfeature_set_descriptor_thd(dev, &(tab_arfeature->mag_thd_cfg)));
	}

	return ISP_IPP_OK;
}

static int arfeature_cfg_reg_part_1(struct arfeature_dev_t *dev, struct cfg_tab_arfeature_t *tab_arfeature)
{
	if ((tab_arfeature->kpt_lmt_cfg.to_use) && (tab_arfeature->top_cfg.mode == ARF_DETECTION_MODE)) {
		tab_arfeature->kpt_lmt_cfg.to_use = 0;
		loge_if_ret(arfeature_set_kpt_lmt(dev, &(tab_arfeature->kpt_lmt_cfg)));
	}

	if (tab_arfeature->cvdr_cfg.to_use) {
		tab_arfeature->cvdr_cfg.to_use = 0;
		loge_if_ret(arfeature_set_cvdr_rd(dev, &(tab_arfeature->cvdr_cfg)));
		loge_if_ret(arfeature_set_cvdr_wr(dev, &(tab_arfeature->cvdr_cfg)));
	}

	if (tab_arfeature->top_cfg.to_use) {
		tab_arfeature->top_cfg.to_use = 0;
		loge_if_ret(arfeature_set_ctrl(dev, &(tab_arfeature->top_cfg)));
	}

	return ISP_IPP_OK;
}

static int arfeature_do_config(struct arfeature_dev_t *dev, struct cfg_tab_arfeature_t *tab_arfeature)
{
	if (dev == NULL || tab_arfeature == NULL) {
		loge("Failed:params is NULL!!");
		return ISP_IPP_ERR;
	}

	loge_if_ret(arfeature_cfg_reg_part_0(dev, tab_arfeature));
	loge_if_ret(arfeature_cfg_reg_part_1(dev, tab_arfeature));
	return ISP_IPP_OK;
}

int arfeature_prepare_cmd(struct arfeature_dev_t *dev,
						  struct cmd_buf_t *cmd_buf, struct cfg_tab_arfeature_t *table)
{
	if (dev == NULL || cmd_buf == NULL || table == NULL)
		return ISP_IPP_ERR;

	dev->cmd_buf = cmd_buf;
	loge_if_ret(arfeature_do_config(dev, table));
	return ISP_IPP_OK;
}

static struct arfeature_ops_t arfeature_ops = {
	.prepare_cmd   = arfeature_prepare_cmd,
};

struct arfeature_dev_t g_arfeature_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_ARF,
		.ops = &arfeature_ops,
	},
};

/* ********************************* END ********************************* */

