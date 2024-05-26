/******************************************************************
 * Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
 * File name    cvdr_drv.c
 * Description:
 *
 * Version      1.0
 * Date         2019-05-23
 ******************************************************************/
#include <linux/printk.h>
#include "cvdr_drv.h"
#include "ipp_cvdr_drv_priv.h"
#include "ipp_cvdr_reg_offset.h"
#include "ipp_cvdr_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_CVDR_DRV

#define VP_WR_REG_OFFSET 0x10
#define VP_RD_REG_OFFSET 0x20
#define NR_RD_REG_OFFSET 0x10
#define NR_WR_REG_OFFSET 0x10
#define ONE_REG_OFFSET   0x4
#define CVDR_SPARE_NUM   4

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

unsigned int g_cvdr_vp_wr_id[IPP_VP_WR_MAX] = {
	[IPP_VP_WR_CMDLST]      = 0,
	[IPP_VP_WR_ORB_ENH_Y]   = 3,
	[IPP_VP_WR_SLAM_Y]      = 4,
	[IPP_VP_WR_ORB_STAT]   = 5,
	[IPP_VP_WR_VBK_Y] = 9,
	[IPP_VP_WR_VBK_UV] = 8,
	[IPP_VP_WR_VBK_MASK_TF] = 10,
	[IPP_VP_WR_GF_LF_A] = 20,
	[IPP_VP_WR_GF_HF_B] = 21,
	[IPP_VP_WR_ANF_Q0] = 22,
	[IPP_VP_WR_ANF_I0] = 23,
	[IPP_VP_WR_ANF_Q1] = 24,
	[IPP_VP_WR_ANF_I1] = 25,
};

unsigned int g_cvdr_vp_rd_id[IPP_VP_RD_MAX] = {
	[IPP_VP_RD_CMDLST]  = 0,
	[IPP_VP_RD_ORB_ENH_Y_HIST]  = 6,
	[IPP_VP_RD_ORB_ENH_Y_IMAGE]  = 7,
	[IPP_VP_RD_ORB]    = 8,
	[IPP_VP_RD_RDR]     = 9,
	[IPP_VP_RD_CMP]     = 10,
	[IPP_VP_RD_VBK_SRC_Y] = 13,
	[IPP_VP_RD_VBK_SRC_UV] = 14,
	[IPP_VP_RD_VBK_SRC_Y_DS16] = 15,
	[IPP_VP_RD_VBK_SRC_UV_DS16] = 16,
	[IPP_VP_RD_VBK_SIGMA] = 17,
	[IPP_VP_RD_VBK_MASK_TF] = 18,
	[IPP_VP_RD_GF_SRC_P] = 20,
	[IPP_VP_RD_GF_GUI_I] = 21,
	[IPP_VP_RD_ANF_CURR_I0] = 22,
	[IPP_VP_RD_ANF_CURR_Q0] = 23,
	[IPP_VP_RD_ANF_PREV_I0] = 24,
	[IPP_VP_RD_ANF_PREV_Q0] = 25,
	[IPP_VP_RD_ANF_CURR_I1] = 26,
	[IPP_VP_RD_ANF_CURR_Q1] = 27,
	[IPP_VP_RD_ANF_PREV_I1] = 28,
	[IPP_VP_RD_ANF_PREV_Q1] = 29,
};

unsigned int g_cvdr_nr_wr_id[IPP_NR_WR_MAX] = {
	[IPP_NR_WR_RDR]  = 6,
};

unsigned int g_cvdr_nr_rd_id[IPP_NR_RD_MAX] = {
	[IPP_NR_RD_CMP]  = 3,
	[IPP_NR_RD_MC]   = 2,
};

/**********************************************************
 * function name: cvdr_set_debug_enable
 *
 * description:
 *     set cvdr debug enable
 *
 * input:
 *     dev        : cvdr device
 *     wr_peak_en : enable the FIFO peak functionality over the write port
 *     rd_peak_en : enable the FIFO peak functionality over the read port
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_set_debug_enable(struct cvdr_dev_t *dev,
	unsigned char wr_peak_en, unsigned char rd_peak_en)
{
	union U_AXI_IPP_CVDR_DEBUG_EN tmp;
	tmp.u32 = 0;
	tmp.bits.wr_peak_en = wr_peak_en;
	tmp.bits.rd_peak_en = rd_peak_en;
	hispcpe_reg_set(CVDR_REG, IPP_CVDR_AXI_JPEG_CVDR_DEBUG_EN_REG, tmp.u32);
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cvdr_get_debug_info
 *
 * description:
 *     get cvdr debug info
 *
 * input:
 *     dev     : cvdr device
 *     wr_peak : peak number of Data Units used for the write functionality
 *     rd_peak : peak number of Data Units used for the read functionality
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_get_debug_info(struct cvdr_dev_t *dev,
	unsigned char *wr_peak, unsigned char *rd_peak)
{
	union U_AXI_IPP_CVDR_DEBUG tmp;
	tmp.u32 = hispcpe_reg_get(CVDR_REG, IPP_CVDR_AXI_JPEG_CVDR_DEBUG_REG);
	*wr_peak = tmp.bits.wr_peak;
	*rd_peak = tmp.bits.rd_peak;
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cvdr_set_nr_wr_config
 *
 * description:
 *     config cvdr non-raster write port(nr_wr) configurations
 *
 * input:
 *     dev  : cvdr device
 *     port : nr write port number
 *     en   : enable or bypass nr write port
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_set_nr_wr_config(struct cvdr_dev_t *dev,
	unsigned char port,  unsigned char nr_wr_stop, unsigned char en)
{
	unsigned int reg_addr;
	union U_AXI_IPP_NR_WR_CFG tmp;
	reg_addr = IPP_CVDR_AXI_JPEG_NR_WR_CFG_6_REG + port * NR_WR_REG_OFFSET;
	tmp.u32 = 0;
	tmp.bits.nr_wr_stop  = nr_wr_stop;
	tmp.bits.nrwr_enable = en;
	switch (port) {
	default:
		CMDLST_SET_REG(dev->base_addr + reg_addr, tmp.u32);
		break;
	}
	D("IPP_NR_WR_CFG_%d:    0x%x = 0x%x, enable = %d",
		port, dev->base_addr + reg_addr, tmp.u32, en);
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cvdr_set_nr_rd_config
 *
 * description:
 *     config ipp cvdr non-raster read port(nr_rd) configurations
 *
 * input:
 *     dev  : cvdr device
 *     port : nr read port number
 *     du   : number of allocated DUs
 *     en   : enable or bypass nr read port
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_set_nr_rd_config(struct cvdr_dev_t *dev,
	unsigned char port, unsigned char du,
	unsigned char nr_rd_stop, unsigned char en)
{
	unsigned int reg_addr = 0;
	union U_AXI_IPP_NR_RD_CFG tmp;
	if (port == IPP_NR_RD_CMP)
		reg_addr = IPP_CVDR_AXI_JPEG_NR_RD_CFG_3_REG;
	if (port == IPP_NR_RD_MC)
		reg_addr = IPP_CVDR_AXI_JPEG_NR_RD_CFG_2_REG;
	tmp.u32 = 0;
	tmp.bits.nrrd_allocated_du = du;
	tmp.bits.nr_rd_stop = nr_rd_stop;
	tmp.bits.nrrd_enable = en;
	switch (port) {
	default:
		CMDLST_SET_REG(dev->base_addr + reg_addr, tmp.u32);
		break;
	}
	D("NR_RD_CFG_%d:    0x%x = 0x%x, allocated_du = %d, enable = %d",
		port + 1, dev->base_addr + reg_addr, tmp.u32, du, en);
	return ISP_IPP_OK;
}

static int cvdr_nr_do_config(
	struct cvdr_dev_t *dev,
	struct cfg_tab_cvdr_t *table)
{
	int i = 0;
	if (dev == NULL || table == NULL) {
		E("input param is invalid!!");
		return ISP_IPP_ERR;
	}
	for (i = 0; i < IPP_NR_WR_MAX; ++i) {
		if (table->nr_wr_cfg[i].to_use == 1) {
			loge_if_ret(cvdr_set_nr_wr_config(dev, i,
					table->nr_wr_cfg[i].nr_wr_stop,
					table->nr_wr_cfg[i].en));
			table->nr_wr_cfg[i].to_use = 0;
		}
	}
	for (i = 0; i < IPP_NR_RD_MAX ; ++i) {
		if (table->nr_rd_cfg[i].to_use == 1) {
			loge_if_ret(cvdr_set_nr_rd_config(dev, i,
					table->nr_rd_cfg[i].allocated_du,
					table->nr_rd_cfg[i].nr_rd_stop,
					table->nr_rd_cfg[i].en));
			table->nr_rd_cfg[i].to_use = 0;
		}
	}
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cvdr_set_vp_wr_ready
 *
 * description:
 *     config cvdr video port write(vp_wr) configurations
 *
 * input:
 *     dev  : cvdr device
 *     port : write port number
 *     desc : vp wr configurations info
 *     addr : start address
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_set_vp_wr_ready(struct cvdr_dev_t *dev,
	unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union U_AXI_IPP_VP_WR_CFG tmp_cfg;
	union U_AXI_IPP_VP_WR_AXI_FS tmp_fs;
	union U_AXI_IPP_VP_WR_AXI_LINE tmp_line;
	union U_AXI_IPP_VP_WR_IF_CFG tmp_if_cfg;
	union U_AXI_IPP_LIMITER_VP_WR tmp_limiter;
	struct cvdr_wr_fmt_desc_t *desc = &(table->vp_wr_cfg[port].fmt);
	struct cvdr_bw_cfg_t *bw = &(table->vp_wr_cfg[port].bw);
	D("set wr port %d addr = 0x%x, dev->base_addr = 0x%x",
		port, desc->fs_addr, dev->base_addr);
	loge_if_ret(desc->fs_addr & 0xF);
	tmp_cfg.u32  = 0;
	tmp_fs.u32   = 0;
	tmp_line.u32 = 0;
	tmp_limiter.u32 = 0;
	tmp_if_cfg.u32 = 0;
	if (bw == NULL) {
		E("vdr_bw_cfg_t* bw NULL!");
		return ISP_IPP_ERR;
	}
	tmp_cfg.bits.vpwr_pixel_format = desc->pix_fmt;
	tmp_cfg.bits.vpwr_pixel_expansion = desc->pix_expan;
	tmp_cfg.bits.vpwr_last_page = desc->last_page;
	tmp_fs.bits.vpwr_address_frame_start = (desc->fs_addr >> 2) >> 2;
	tmp_line.bits.vpwr_line_stride = desc->line_stride;
	tmp_line.bits.vpwr_line_start_wstrb = (desc->line_shart_wstrb == 0) ?
		0xF : desc->line_shart_wstrb;
	tmp_line.bits.vpwr_line_wrap         = desc->line_wrap;
	tmp_limiter.bits.vpwr_access_limiter_0 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_1 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_2 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_3 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_reload = 0xF;
	tmp_if_cfg.bits.vpwr_prefetch_bypass = 1;
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_LIMITER_VP_WR_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_ONE_REG_OFFSET,
		tmp_limiter.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_WR_CFG_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
		tmp_cfg.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_WR_AXI_LINE_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
		tmp_line.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_WR_IF_CFG_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
		tmp_if_cfg.u32);
	if (port == IPP_VP_WR_ORB_STAT) {
		CMDLST_SET_ADDR_ALIGN(2, CVDR_ALIGN_BYTES);
		table->orb_cvdr_wr_addr = dev->cmd_buf->start_addr_isp_map +
			dev->cmd_buf->data_size;
		D("dev->cmd_buf->data_size = 0x%x\n",
			dev->cmd_buf->data_size);
		CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
		CMDLST_SET_REG(dev->base_addr +
			IPP_CVDR_AXI_JPEG_VP_WR_AXI_FS_0_REG +
			g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
			tmp_fs.u32);
		CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	} else {
		CMDLST_SET_REG(dev->base_addr +
			IPP_CVDR_AXI_JPEG_VP_WR_AXI_FS_0_REG +
			g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
			tmp_fs.u32);
	}
	D("VP_WR_CFG:      0x%x\n", dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_WR_CFG_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET);
	D("pix_fmt = %d, pix_expan = %d, last_page = %d\n",
		desc->pix_fmt, desc->pix_expan, desc->last_page);
	D("VP_WR_AXI_LINE: 0x%x\n", dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_WR_AXI_LINE_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET);
	D("line_stride = %d, line_wrap = %d\n",
		desc->line_stride, desc->line_wrap);
	D("VP_WR_AXI_FS:   0x%x\n", dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_WR_AXI_FS_0_REG +
		g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET);
	D("desc. addr = 0x%x,.vpwr_address_frame_start = 0x%x\n",
		desc->fs_addr >> 2, tmp_fs.bits.vpwr_address_frame_start);
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cvdr_set_vp_rd_ready
 *
 * description:
 *     config cvdr video port read(vp_rd) configurations
 *
 * input:
 *     dev  : cvdr device
 *     port : read port number
 *     desc : vp rd configurations info
 *     addr : start address
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_set_vp_rd_ready(struct cvdr_dev_t *dev,
	unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union U_AXI_IPP_VP_RD_CFG tmp_cfg;
	union U_AXI_IPP_VP_RD_LWG tmp_lwg;
	union U_AXI_IPP_VP_RD_FHG tmp_fhg;
	union U_AXI_IPP_VP_RD_AXI_FS tmp_fs;
	union U_AXI_IPP_VP_RD_AXI_LINE tmp_line;
	union U_AXI_IPP_VP_RD_IF_CFG tmp_if_cfg;
	union U_AXI_IPP_LIMITER_VP_RD tmp_limiter;
	struct cvdr_rd_fmt_desc_t *desc = &(table->vp_rd_cfg[port].fmt);
	loge_if_ret(desc->fs_addr & 0xF);
	tmp_cfg.u32  = 0;
	tmp_lwg.u32  = 0;
	tmp_fhg.u32  = 0;
	tmp_fs.u32   = 0;
	tmp_line.u32 = 0;
	tmp_limiter.u32 = 0;
	tmp_if_cfg.u32 = 0;
	tmp_cfg.bits.vprd_pixel_format    = desc->pix_fmt;
	tmp_cfg.bits.vprd_pixel_expansion = desc->pix_expan;
	tmp_cfg.bits.vprd_allocated_du    = desc->allocated_du;
	tmp_cfg.bits.vprd_last_page       = desc->last_page;
	tmp_lwg.bits.vprd_line_size       = desc->line_size;
	tmp_lwg.bits.vprd_horizontal_blanking = desc->hblank;
	tmp_fhg.bits.vprd_frame_size      = desc->frame_size;
	tmp_fhg.bits.vprd_vertical_blanking   = desc->vblank;
	tmp_fs.bits.vprd_axi_frame_start  = (desc->fs_addr >> 2) >> 2;
	tmp_line.bits.vprd_line_stride    = desc->line_stride;
	tmp_line.bits.vprd_line_wrap      = desc->line_wrap;
	tmp_limiter.bits.vprd_access_limiter_0 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_1 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_2 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_3 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_reload = 0xF;
	tmp_if_cfg.bits.vprd_prefetch_bypass = 1;
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_LIMITER_VP_RD_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_ONE_REG_OFFSET,
		tmp_limiter.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_RD_LWG_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_lwg.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_RD_AXI_LINE_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_line.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_RD_CFG_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_cfg.u32);
	if (port == IPP_VP_RD_RDR || port == IPP_VP_RD_CMP) {
		CMDLST_SET_ADDR_ALIGN(2, CVDR_ALIGN_BYTES);
		if (port == IPP_VP_RD_RDR)
			table->rdr_cvdr_frame_size_addr = dev->cmd_buf->start_addr_isp_map +
				dev->cmd_buf->data_size;
		if (port == IPP_VP_RD_CMP) {
			table->cmp_cvdr_frame_size_addr = dev->cmd_buf->start_addr_isp_map +
				dev->cmd_buf->data_size;
		}
		CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
		CMDLST_SET_REG(dev->base_addr +
			IPP_CVDR_AXI_JPEG_VP_RD_FHG_0_REG +
			g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
			tmp_fhg.u32);
		CMDLST_SET_REG_DATA(CMDLST_PADDING_DATA);
	} else {
		CMDLST_SET_REG(dev->base_addr +
			IPP_CVDR_AXI_JPEG_VP_RD_FHG_0_REG +
			g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
			tmp_fhg.u32);
	}
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_RD_IF_CFG_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_if_cfg.u32);
	CMDLST_SET_REG(dev->base_addr +
		IPP_CVDR_AXI_JPEG_VP_RD_AXI_FS_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_fs.u32);
	D("VP_RD_CFG: 0x%x",
		dev->base_addr + IPP_CVDR_AXI_JPEG_VP_RD_CFG_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET);
	D("pix_fmt = %d, pix_expan = %d, allocated_du = %d, last_page = %d",
		desc->pix_fmt, desc->pix_expan,
		desc->allocated_du, desc->last_page);
	D("VP_RD_LWG:      0x%x = 0x%x, line_size = %d, hblank = %d",
		dev->base_addr + IPP_CVDR_AXI_JPEG_VP_RD_LWG_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_lwg.u32, desc->line_size, desc->hblank);
	D("VP_RD_FHG:      0x%x = 0x%x, frame_size = %d, vblank = %d",
		dev->base_addr + IPP_CVDR_AXI_JPEG_VP_RD_FHG_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_fhg.u32, desc->frame_size, desc->vblank);
	D("VP_RD_AXI_LINE: 0x%x = 0x%x, line_stride = %d, line_wrap = %d",
		dev->base_addr + IPP_CVDR_AXI_JPEG_VP_RD_AXI_LINE_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_line.u32, desc->line_stride, desc->line_wrap);
	D("VP_RD_AXI_FS:   0x%x = 0x%x",
		dev->base_addr + IPP_CVDR_AXI_JPEG_VP_RD_AXI_FS_0_REG +
		g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		tmp_fs.u32);
	D("desc.addr = 0x%x,tmp_fs.bits.vprd_axi_frame_start = 0x%x\n",
		desc->fs_addr >> 2, tmp_fs.bits.vprd_axi_frame_start);
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cvdr_do_config
 *
 * description:
 *     according to cvdr_config_table config cvdr
 *
 * input:
 *     dev    : cvdr device
 *     table  : cvdr configurations info table
 *
 * output:
 *     0  : success
 *     -1 : failed
 ***********************************************************/
static int cvdr_do_config(struct cvdr_dev_t *dev, struct cfg_tab_cvdr_t *table)
{
	int i = 0;
	if (dev == NULL || table == NULL) {
		E("input param is invalid!!");
		return ISP_IPP_ERR;
	}
	for (i = 0; i < IPP_VP_WR_MAX; ++i) {
		if (table->vp_wr_cfg[i].to_use == 1) {
			loge_if_ret(cvdr_set_vp_wr_ready(dev, i, table));
			table->vp_wr_cfg[i].to_use = 0;
		}
	}
	for (i = IPP_VP_RD_MAX - 1; i >= 0; --i) {
		if (table->vp_rd_cfg[i].to_use == 1) {
			loge_if_ret(cvdr_set_vp_rd_ready(dev, i, table));
			table->vp_rd_cfg[i].to_use = 0;
		}
	}
	for (i = 0; i < IPP_NR_WR_MAX; ++i) {
		if (table->nr_wr_cfg[i].to_use == 1) {
			loge_if_ret(cvdr_set_nr_wr_config(dev, i,
					table->nr_wr_cfg[i].nr_wr_stop,
					table->nr_wr_cfg[i].en));
			table->nr_wr_cfg[i].to_use = 0;
		}
	}
	for (i = 0; i < IPP_NR_RD_MAX ; ++i) {
		if (table->nr_rd_cfg[i].to_use == 1) {
			loge_if_ret(cvdr_set_nr_rd_config(dev, i,
					table->nr_rd_cfg[i].allocated_du,
					table->nr_rd_cfg[i].nr_rd_stop,
					table->nr_rd_cfg[i].en));
			table->nr_rd_cfg[i].to_use = 0;
		}
	}
	return ISP_IPP_OK;
}

int cvdr_prepare_vprd_cmd(struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_cvdr_t *table)
{
	dev->cmd_buf = cmd_buf;
	return ISP_IPP_OK;
}

int cvdr_prepare_vpwr_cmd(struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_cvdr_t *table)
{
	dev->cmd_buf = cmd_buf;
	return ISP_IPP_OK;
}

int cvdr_prepare_cmd(struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_cvdr_t *table)
{
	int ret = ISP_IPP_OK;
	ippdev_lock();
	dev->cmd_buf = cmd_buf;
	ret = cvdr_do_config(dev, table);
	ippdev_unlock();
	return ret;
}

int cvdr_prepare_nr_cmd(
	struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	struct cfg_tab_cvdr_t *table)
{
	int ret = ISP_IPP_OK;
	ippdev_lock();
	dev->cmd_buf = cmd_buf;
	ret = cvdr_nr_do_config(dev, table);
	ippdev_unlock();
	return ret;
}

static struct cvdr_ops_t cvdr_ops = {
	.set_debug_enable     = cvdr_set_debug_enable,
	.get_debug_info       = cvdr_get_debug_info,
	.set_vp_wr_ready      = cvdr_set_vp_wr_ready,
	.set_vp_rd_ready      = cvdr_set_vp_rd_ready,

	.do_config   = cvdr_do_config,
	.prepare_cmd = cvdr_prepare_cmd,
	.prepare_vprd_cmd = cvdr_prepare_vprd_cmd,
	.prepare_vpwr_cmd = cvdr_prepare_vpwr_cmd,
};

struct cvdr_dev_t g_cvdr_devs[] = {
	[0] = {
		.base_addr = IPP_BASE_ADDR_CVDR,
		.ops = &cvdr_ops,
	},
};

#pragma GCC diagnostic pop

