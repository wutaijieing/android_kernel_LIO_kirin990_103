/******************************************************************
 * Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
 * File name    hispipp_orb_enh_drv.h
 * Description:
 *
 * Date         2020-04-11 17:36:21
 ******************************************************************/

#ifndef _ORB_ENH_DRV_CS_H_INCLUDED__
#define _ORB_ENH_DRV_CS_H_INCLUDED__

#include "segment_common.h"
#include "cfg_table_orb_enh.h"

struct orb_enh_ops_t;

struct orb_enh_dev_t {
	unsigned int           base_addr;
	struct orb_enh_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct orb_enh_ops_t {
	int (*prepare_cmd)(struct orb_enh_dev_t *dev,
					   struct cmd_buf_t *cmd_buf, struct cfg_tab_orb_enh_t *table);
};

int orb_enh_prepare_cmd(
	struct orb_enh_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	struct cfg_tab_orb_enh_t *table);

extern struct orb_enh_dev_t g_orb_enh_devs[];
#endif /* _ORB_ENH_DRV_CS_H_INCLUDED__ */
/* **************************************end*********************************** */
