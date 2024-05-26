// *********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    cfg_table_matcher.h
// Description:
//
// Date         2020-05-25
// *********************************************************
#ifndef __CFG_TABLE_MATCHER_CS_H__
#define __CFG_TABLE_MATCHER_CS_H__

struct seg_matcher_rdr_cfg_t {
	struct cfg_tab_reorder_t reorder_cfg_tab[MATCHER_LAYER_MAX];
	struct cfg_tab_cvdr_t reorder_cvdr_cfg_tab[MATCHER_LAYER_MAX];
};

struct seg_matcher_cmp_cfg_t {
	struct cfg_tab_compare_t compare_cfg_tab[MATCHER_LAYER_MAX];
	struct cfg_tab_cvdr_t compare_cvdr_cfg_tab[MATCHER_LAYER_MAX];
};

struct seg_reorder_cfg_t {
	struct cfg_tab_reorder_t reorder_cfg_tab;
	struct cfg_tab_cvdr_t reorder_cvdr_cfg_tab;
};

struct seg_compare_cfg_t {
	struct cfg_tab_compare_t compare_cfg_tab;
	struct cfg_tab_cvdr_t compare_cvdr_cfg_tab;
};

#endif /* __CFG_TABLE_MATCHER_CS_H__ */
