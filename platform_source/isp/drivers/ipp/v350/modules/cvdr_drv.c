/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    cvdr_drv.c
 * Description:
 *
 * Date         2020-04-15
 ******************************************************************/
#include <linux/printk.h>
#include "cvdr_drv.h"
#include "cvdr_ipp_drv_priv.h"
#include "cvdr_ipp_reg_offset.h"
#include "cvdr_ipp_reg_offset_field.h"

#define LOG_TAG LOG_MODULE_CVDR_DRV

#define VP_WR_REG_OFFSET 0x1C
#define VP_RD_REG_OFFSET 0x20
#define NR_RD_REG_OFFSET 0xC
#define NR_WR_REG_OFFSET 0xC
#define ONE_REG_OFFSET   0x4
#define CVDR_SPARE_NUM   4

unsigned int g_cvdr_vp_wr_id[IPP_VP_WR_MAX] = {
	[IPP_VP_WR_CMDLST]      = 0,
	[IPP_VP_WR_ORB_ENH_Y]   = 3,
	[IPP_VP_WR_ARF_PRY_1]      = 12,
	[IPP_VP_WR_ARF_PRY_2]   = 13,
	[IPP_VP_WR_ARF_DOG_0]      = 14,
	[IPP_VP_WR_ARF_DOG_1]   = 15,
	[IPP_VP_WR_ARF_DOG_2]      = 16,
	[IPP_VP_WR_ARF_DOG_3]   = 17,
	[IPP_VP_WR_HIOF_DENSE_TNR] = 10,
	[IPP_VP_WR_HIOF_SPARSE_MD] = 11,
	[IPP_VP_WR_GF_LF_A] = 20,
	[IPP_VP_WR_GF_HF_B] = 21,
};

unsigned int g_cvdr_vp_rd_id[IPP_VP_RD_MAX] = {
	[IPP_VP_RD_CMDLST]  = 0,
	[IPP_VP_RD_ORB_ENH_Y_HIST]  = 6,
	[IPP_VP_RD_ORB_ENH_Y_IMAGE]  = 7,
	[IPP_VP_RD_ARF_0]    = 12,
	[IPP_VP_RD_ARF_1]    = 13,
	[IPP_VP_RD_ARF_2]    = 14,
	[IPP_VP_RD_ARF_3]    = 15,
	[IPP_VP_RD_RDR]     = 9,
	[IPP_VP_RD_CMP]     = 10,
	[IPP_VP_RD_HIOF_CUR_Y] = 18,
	[IPP_VP_RD_HIOF_REF_Y] = 19,
	[IPP_VP_RD_GF_SRC_P] = 20,
	[IPP_VP_RD_GF_GUI_I] = 21,
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
	union u_cvdr_ipp_cvdr_debug_en tmp;
	tmp.u32 = 0;
	tmp.bits.wr_peak_en = wr_peak_en;
	tmp.bits.rd_peak_en = rd_peak_en;
	hispcpe_reg_set(CVDR_REG, CVDR_IPP_CVDR_IPP_CVDR_DEBUG_EN_REG, tmp.u32);
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
	union u_cvdr_ipp_cvdr_debug tmp;
	tmp.u32 = hispcpe_reg_get(CVDR_REG, CVDR_IPP_CVDR_IPP_CVDR_DEBUG_REG);
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
	unsigned int reg_addr = 0;
	union u_cvdr_ipp_nr_wr_cfg_6 tmp;

	// only one IPP_NR_WR_RDR case;
	if (port == IPP_NR_WR_RDR)
		reg_addr = CVDR_IPP_CVDR_IPP_NR_WR_CFG_6_REG;

	tmp.u32 = 0;
	tmp.bits.nr_wr_stop_6  = nr_wr_stop;
	tmp.bits.nrwr_enable_6 = en;

	macro_cmdlst_set_reg(dev->base_addr + reg_addr, tmp.u32);
	logd("IPP_NR_WR_CFG_%d:    0x%x = 0x%x, enable = %d",
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
	union u_cvdr_ipp_nr_rd_cfg_2 tmp;

	if (port == IPP_NR_RD_CMP)
		reg_addr = CVDR_IPP_CVDR_IPP_NR_RD_CFG_3_REG;

	if (port == IPP_NR_RD_MC)
		reg_addr = CVDR_IPP_CVDR_IPP_NR_RD_CFG_2_REG;

	tmp.u32 = 0;
	tmp.bits.nrrd_allocated_du_2 = du;
	tmp.bits.nr_rd_stop_2 = nr_rd_stop;
	tmp.bits.nrrd_enable_2 = en;

	macro_cmdlst_set_reg(dev->base_addr + reg_addr, tmp.u32);
	logd("NR_RD_CFG_%d:    0x%x = 0x%x, allocated_du = %d, enable = %d",
		 port + 1, dev->base_addr + reg_addr, tmp.u32, du, en);

	return ISP_IPP_OK;
}

static int cvdr_nr_do_config(
	struct cvdr_dev_t *dev,
	struct cfg_tab_cvdr_t *table)
{
	int i = 0;

	if (dev == NULL || table == NULL) {
		loge("Failed:input param is invalid!!");
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

static void cvdr_vp_wr_config_cfg(struct cvdr_dev_t *dev,
								  unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_wr_sid_cfg_sid0_0 tmp_cfg;
	struct cvdr_wr_fmt_desc_t *desc = &(table->vp_wr_cfg[port].fmt);
	tmp_cfg.u32  = 0;
	tmp_cfg.bits.vpwr_pixel_format_sid0_0 = desc->pix_fmt;
	tmp_cfg.bits.vpwr_pixel_expansion_sid0_0 = desc->pix_expan;
	tmp_cfg.bits.vpwr_last_page_sid0_0 = desc->last_page;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_WR_SID_CFG_SID0_0_REG +
						 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
						 tmp_cfg.u32);
	logd("VP_WR_CFG:      0x%x", dev->base_addr +
		 CVDR_IPP_CVDR_IPP_VP_WR_SID_CFG_SID0_0_REG +
		 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET);
	logd("pix_fmt = %d, pix_expan = %d, last_page = %d, port=%d",
		 desc->pix_fmt, desc->pix_expan, desc->last_page, port);
}

static void cvdr_vp_wr_config_axi_line(struct cvdr_dev_t *dev,
									   unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_wr_sid_axi_line_sid0_0 tmp_line;
	struct cvdr_wr_fmt_desc_t *desc = &(table->vp_wr_cfg[port].fmt);
	tmp_line.u32 = 0;
	tmp_line.bits.vpwr_line_stride_sid0_0 = desc->line_stride;
	tmp_line.bits.vpwr_line_start_wstrb_sid0_0 = (desc->line_shart_wstrb == 0) ?
			0xF : desc->line_shart_wstrb;
	tmp_line.bits.vpwr_line_wrap_sid0_0         = desc->line_wrap;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_WR_SID_AXI_LINE_SID0_0_REG +
						 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
						 tmp_line.u32);
	logd("VP_WR_AXI_LINE: 0x%x", dev->base_addr +
		 CVDR_IPP_CVDR_IPP_VP_WR_SID_AXI_LINE_SID0_0_REG +
		 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET);
	logd("line_stride = %d, line_wrap = %d, port = %d",
		 desc->line_stride, desc->line_wrap, port);
}

static void cvdr_vp_wr_config_if(struct cvdr_dev_t *dev,
								 unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_wr_if_cfg_0 tmp_if_cfg;
	tmp_if_cfg.u32 = 0;
	tmp_if_cfg.bits.vpwr_prefetch_bypass_0 = 1;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_WR_IF_CFG_0_REG +
						 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
						 tmp_if_cfg.u32);
}

static void cvdr_vp_wr_config_limiter(struct cvdr_dev_t *dev,
									  unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_wr_limiter_0 tmp_limiter;
	tmp_limiter.u32 = 0;
	tmp_limiter.bits.vpwr_access_limiter_0_0 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_1_0 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_2_0 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_3_0 = 0xF;
	tmp_limiter.bits.vpwr_access_limiter_reload_0 = 0xF;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_WR_LIMITER_0_REG +
						 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
						 tmp_limiter.u32);
}

static void cvdr_vp_wr_config_axi_fs(struct cvdr_dev_t *dev,
									 unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_wr_sid_axi_fs_sid0_0 tmp_fs;
	struct cvdr_wr_fmt_desc_t *desc = &(table->vp_wr_cfg[port].fmt);
	tmp_fs.u32   = 0;
	tmp_fs.bits.vpwr_address_frame_start_sid0_0 = (desc->fs_addr >> 2) >> 2;

	if (port == IPP_VP_WR_ARF_PRY_1 && arf_updata_wr_addr_flag == 1) {
		macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);
		table->arf_cvdr_wr_addr = dev->cmd_buf->start_addr_isp_map +
								  dev->cmd_buf->data_size;
		logd("dev->cmd_buf->data_size = 0x%x, table->arf_cvdr_wr_addr  = 0x%x", dev->cmd_buf->data_size,
			 table->arf_cvdr_wr_addr);
		macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
		macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
		macro_cmdlst_set_reg(dev->base_addr +
							 CVDR_IPP_CVDR_IPP_VP_WR_SID_AXI_FS_SID0_0_REG +
							 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
							 tmp_fs.u32);
	} else {
		macro_cmdlst_set_reg(dev->base_addr +
							 CVDR_IPP_CVDR_IPP_VP_WR_SID_AXI_FS_SID0_0_REG +
							 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET,
							 tmp_fs.u32);
	}

	logd("VP_WR_AXI_FS:   0x%x", dev->base_addr +
		 CVDR_IPP_CVDR_IPP_VP_WR_SID_AXI_FS_SID0_0_REG +
		 g_cvdr_vp_wr_id[port] * IPP_VP_WR_REG_OFFSET);
	logd("desc. addr = 0x%x,.vpwr_address_frame_start_sid0_0 = 0x%x",
		 desc->fs_addr >> 2, tmp_fs.bits.vpwr_address_frame_start_sid0_0);
}

static void cvdr_vp_rd_config_cfg(struct cvdr_dev_t *dev,
								  unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_cfg_0 tmp_cfg;
	struct cvdr_rd_fmt_desc_t *desc = &(table->vp_rd_cfg[port].fmt);
	tmp_cfg.u32  = 0;
	tmp_cfg.bits.vprd_pixel_format_0    = desc->pix_fmt;
	tmp_cfg.bits.vprd_pixel_expansion_0 = desc->pix_expan;
	tmp_cfg.bits.vprd_allocated_du_0    = desc->allocated_du;
	tmp_cfg.bits.vprd_last_page_0       = desc->last_page;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_RD_CFG_0_REG +
						 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
						 tmp_cfg.u32);
	logd("VP_RD_CFG: 0x%x",
		 dev->base_addr + CVDR_IPP_CVDR_IPP_VP_RD_CFG_0_REG +
		 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET);
	logd("pix_fmt = %d, pix_expan = %d, allocated_du = %d, last_page = %d",
		 desc->pix_fmt, desc->pix_expan,
		 desc->allocated_du, desc->last_page);
}

static void cvdr_vp_rd_config_lwg(struct cvdr_dev_t *dev,
								  unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_lwg_0 tmp_lwg;
	struct cvdr_rd_fmt_desc_t *desc = &(table->vp_rd_cfg[port].fmt);
	tmp_lwg.u32  = 0;
	tmp_lwg.bits.vprd_line_size_0       = desc->line_size;
	tmp_lwg.bits.vprd_horizontal_blanking_0 = desc->hblank;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_RD_LWG_0_REG +
						 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
						 tmp_lwg.u32);
	logd("VP_RD_LWG:      0x%x = 0x%x, line_size = %d, hblank = %d",
		 dev->base_addr + CVDR_IPP_CVDR_IPP_VP_RD_LWG_0_REG +
		 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		 tmp_lwg.u32, desc->line_size, desc->hblank);
}

static void cvdr_vp_rd_config_fhg(struct cvdr_dev_t *dev,
								  unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_fhg_0 tmp_fhg;
	struct cvdr_rd_fmt_desc_t *desc = &(table->vp_rd_cfg[port].fmt);
	tmp_fhg.u32  = 0;
	tmp_fhg.bits.vprd_frame_size_0      = desc->frame_size;
	tmp_fhg.bits.vprd_vertical_blanking_0   = desc->vblank;

	if (port == IPP_VP_RD_RDR || port == IPP_VP_RD_CMP) {
		macro_cmdlst_set_addr_align(CVDR_ALIGN_BYTES);

		if (port == IPP_VP_RD_RDR)
			table->rdr_cvdr_frame_size_addr = dev->cmd_buf->start_addr_isp_map +
											  dev->cmd_buf->data_size;

		if (port == IPP_VP_RD_CMP)
			table->cmp_cvdr_frame_size_addr = dev->cmd_buf->start_addr_isp_map +
											  dev->cmd_buf->data_size;

		macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
		macro_cmdlst_set_reg_data(CMDLST_PADDING_DATA);
		macro_cmdlst_set_reg(dev->base_addr +
							 CVDR_IPP_CVDR_IPP_VP_RD_FHG_0_REG +
							 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
							 tmp_fhg.u32);
	} else {
		macro_cmdlst_set_reg(dev->base_addr +
							 CVDR_IPP_CVDR_IPP_VP_RD_FHG_0_REG +
							 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
							 tmp_fhg.u32);
	}

	logd("VP_RD_FHG:      0x%x = 0x%x, frame_size = %d, vblank = %d",
		 dev->base_addr + CVDR_IPP_CVDR_IPP_VP_RD_FHG_0_REG +
		 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		 tmp_fhg.u32, desc->frame_size, desc->vblank);
}

static void cvdr_vp_rd_config_axi_line(struct cvdr_dev_t *dev,
									   unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_axi_line_0 tmp_line;
	struct cvdr_rd_fmt_desc_t *desc = &(table->vp_rd_cfg[port].fmt);
	tmp_line.u32 = 0;
	tmp_line.bits.vprd_line_stride_0    = desc->line_stride;
	tmp_line.bits.vprd_line_wrap_0      = desc->line_wrap;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_RD_AXI_LINE_0_REG +
						 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
						 tmp_line.u32);
	logd("VP_RD_AXI_LINE: 0x%x = 0x%x, line_stride = %d, line_wrap = %d",
		 dev->base_addr + CVDR_IPP_CVDR_IPP_VP_RD_AXI_LINE_0_REG +
		 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		 tmp_line.u32, desc->line_stride, desc->line_wrap);
}

static void cvdr_vp_rd_config_if(struct cvdr_dev_t *dev,
								 unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_if_cfg_0 tmp_if_cfg;
	tmp_if_cfg.u32 = 0;
	tmp_if_cfg.bits.vprd_prefetch_bypass_0 = 1;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_RD_IF_CFG_0_REG +
						 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
						 tmp_if_cfg.u32);
}

static void cvdr_vp_rd_config_limiter(struct cvdr_dev_t *dev,
									  unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_limiter_0 tmp_limiter;
	tmp_limiter.u32 = 0;
	tmp_limiter.bits.vprd_access_limiter_0_0 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_1_0 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_2_0 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_3_0 = 0xF;
	tmp_limiter.bits.vprd_access_limiter_reload_0 = 0xF;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_RD_LIMITER_0_REG +
						 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
						 tmp_limiter.u32);
}

static void cvdr_vp_rd_config_axi_fs(struct cvdr_dev_t *dev,
									 unsigned char port, struct cfg_tab_cvdr_t *table)
{
	union u_cvdr_ipp_vp_rd_axi_fs_0 tmp_fs;
	struct cvdr_rd_fmt_desc_t *desc = &(table->vp_rd_cfg[port].fmt);

	if (desc->fs_addr & 0xF) return;

	tmp_fs.u32   = 0;
	tmp_fs.bits.vprd_axi_frame_start_0  = (desc->fs_addr >> 2) >> 2;
	macro_cmdlst_set_reg(dev->base_addr +
						 CVDR_IPP_CVDR_IPP_VP_RD_AXI_FS_0_REG +
						 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
						 tmp_fs.u32);
	logd("(port = %d), VP_RD_AXI_FS:   0x%x = 0x%x", port,
		 dev->base_addr + CVDR_IPP_CVDR_IPP_VP_RD_AXI_FS_0_REG +
		 g_cvdr_vp_rd_id[port] * IPP_VP_RD_REG_OFFSET,
		 tmp_fs.u32);
	logd("desc.addr = 0x%x,tmp_fs.bits.vprd_axi_frame_start_0 = 0x%x",
		 desc->fs_addr, tmp_fs.bits.vprd_axi_frame_start_0);
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
	struct cvdr_bw_cfg_t *bw = &(table->vp_wr_cfg[port].bw);
	struct cvdr_wr_fmt_desc_t *desc = &(table->vp_wr_cfg[port].fmt);
	logd("set wr port %d addr = 0x%x, dev->base_addr = 0x%x",
		 port, desc->fs_addr, dev->base_addr);
	loge_if_ret(desc->fs_addr & 0xF);

	if (bw == NULL) {
		loge("Failed:vdr_bw_cfg_t* bw NULL!");
		return ISP_IPP_ERR;
	}

	cvdr_vp_wr_config_cfg(dev, port, table);
	cvdr_vp_wr_config_axi_line(dev, port, table);
	cvdr_vp_wr_config_if(dev, port, table);
	cvdr_vp_wr_config_limiter(dev, port, table);
	cvdr_vp_wr_config_axi_fs(dev, port, table);
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
	cvdr_vp_rd_config_cfg(dev, port, table);
	cvdr_vp_rd_config_lwg(dev, port, table);
	cvdr_vp_rd_config_fhg(dev, port, table);
	cvdr_vp_rd_config_axi_line(dev, port, table);
	cvdr_vp_rd_config_if(dev, port, table);
	cvdr_vp_rd_config_limiter(dev, port, table);
	cvdr_vp_rd_config_axi_fs(dev, port, table);
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
		loge("Failed:input param is invalid!!");
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
	int i = 0;
	int ret = ISP_IPP_OK;
	ippdev_lock();
	dev->cmd_buf = cmd_buf;

	if (dev == NULL || table == NULL) {
		ippdev_unlock();
		loge("Failed:input param is invalid!!");
		return ISP_IPP_ERR;
	}

	for (i = IPP_VP_RD_MAX - 1; i >= 0; --i) {
		if (table->vp_rd_cfg[i].to_use == 1) {
			ret = cvdr_set_vp_rd_ready(dev, i, table);
			if(ret != ISP_IPP_OK) {
				ippdev_unlock();
				return ret;
			}
			table->vp_rd_cfg[i].to_use = 0;
		}
	}

	ippdev_unlock();
	return ISP_IPP_OK;
}

int cvdr_prepare_vpwr_cmd(struct cvdr_dev_t *dev,
						  struct cmd_buf_t *cmd_buf, struct cfg_tab_cvdr_t *table)
{
	int ret = ISP_IPP_OK;
	int i = 0;
	ippdev_lock();
	dev->cmd_buf = cmd_buf;

	if (dev == NULL || table == NULL) {
		ippdev_unlock();
		loge("Failed:input param is invalid!!");
		return ISP_IPP_ERR;
	}

	for (i = 0; i < IPP_VP_WR_MAX; ++i) {
		if (table->vp_wr_cfg[i].to_use == 1) {
			ret = cvdr_set_vp_wr_ready(dev, i, table);
			if(ret != ISP_IPP_OK) {
				ippdev_unlock();
				return ret;
			}
			table->vp_wr_cfg[i].to_use = 0;
		}
	}

	ippdev_unlock();
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

