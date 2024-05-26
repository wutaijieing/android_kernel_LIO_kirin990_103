// ********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_cpe_top.h
// Description:
//
// Date         2017-10-08
// ********************************************************

#ifndef __CFG_TABLE_CPE_TOP_CS_H__
#define __CFG_TABLE_CPE_TOP_CS_H__

struct cpe_top_mem_cfg0_t {
	unsigned int  common_mem_ctrl_sp;
};

struct cpe_top_mem_cfg1_t {
	unsigned int  mcf_pwr_mem_ctrl_sp;
};

struct cpe_top_mem_cfg2_t {
	unsigned int  mfnr_pwr_mem_ctrl_sp;
};

struct crop_vpwr_t {
	unsigned int  vpwr_ihleft;
	unsigned int  vpwr_ihright;
};

struct cpe_top_mode_cfg_t {
	unsigned int  to_use;
	unsigned int  cpe_op_mode;
};

struct cpe_top_mem_cfg_t {
	unsigned int  to_use;
	struct cpe_top_mem_cfg0_t  mem_cfg0;
	struct cpe_top_mem_cfg1_t  mem_cfg1;
	struct cpe_top_mem_cfg2_t  mem_cfg2;
};

struct cpe_top_crop_vpwr_t {
	unsigned int  to_use;
	struct crop_vpwr_t  crop_vpwr_0;
	struct crop_vpwr_t  crop_vpwr_1;
	struct crop_vpwr_t  crop_vpwr_2;
};

struct cpe_top_config_table_t {
	unsigned int        to_use;
	struct cpe_top_mode_cfg_t  mode_cfg;
	struct cpe_top_mem_cfg_t   mem_cfg;
	struct cpe_top_crop_vpwr_t crop_vpwr;
};

#endif /* __CFG_TABLE_CPE_TOP_CS_H__ */
