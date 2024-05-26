// ***************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_hiof.h
// Description:
//
// Date         2020-04-10
// ***************************************************
#include "cfg_table_cvdr.h"

#ifndef __CFG_TABLE_HIOF_CS_H__
#define __CFG_TABLE_HIOF_CS_H__

struct hiof_ctrl_cfg_t {
	unsigned int to_use;
	unsigned int mode;
	unsigned int spena;
	unsigned int iter_num;
	unsigned int iter_check_en;
};

struct hiof_size_cfg_t {
	unsigned int       to_use;

	unsigned int       width;
	unsigned int       height;
};

struct hiof_coeff_cfg_t {
	unsigned int       to_use;

	unsigned int coeff0;
	unsigned int coeff1;
};

struct hiof_threshold_cfg_t {
	unsigned int       to_use;

	unsigned int flat_thr;
	unsigned int ismotion_thr;
	unsigned int confilv_thr;
	unsigned int att_max;
};

struct cfg_tab_hiof_t {
	struct hiof_ctrl_cfg_t    hiof_ctrl_cfg;
	struct hiof_size_cfg_t    hiof_size_cfg;
	struct hiof_coeff_cfg_t    hiof_coeff_cfg;
	struct hiof_threshold_cfg_t hiof_threshold_cfg;
};

struct seg_hiof_cfg_t {
	struct cfg_tab_hiof_t  hiof_cfg;
	struct cfg_tab_cvdr_t cvdr_cfg;
};

#endif // __CFG_TABLE_HIOF_CS_H__
