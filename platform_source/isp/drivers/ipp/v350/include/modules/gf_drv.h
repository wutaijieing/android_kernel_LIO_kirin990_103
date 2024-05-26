/******************************************************************
 * Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
 * File name    hispipp_gf_drv.h
 * Description:
 *
 * Date         2020-04-11 17:36:21
 ******************************************************************/

#ifndef _GF_DRV_CS_H_INCLUDED__
#define _GF_DRV_CS_H_INCLUDED__

#include "segment_common.h"
#include "cfg_table_gf.h"

struct gf_ops_t;

struct gf_dev_t {
	unsigned int  base_addr;
	struct gf_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct gf_ops_t {
	int (*prepare_cmd)(struct gf_dev_t *dev, struct cmd_buf_t *cmd_buf,
					   struct gf_config_table_t *table);
};

int gf_prepare_cmd(struct gf_dev_t *dev, struct cmd_buf_t *cmd_buf,
				   struct gf_config_table_t *table);

extern struct gf_dev_t g_gf_devs[];

#endif /* _GF_DRV_CS_H_INCLUDED__ */
/* **************************************end*********************************** */
