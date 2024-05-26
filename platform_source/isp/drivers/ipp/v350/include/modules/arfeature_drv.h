// **********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    arfeature_drv.h
// Description:
//
// Date         2020-04-14
// **********************************************************/

#ifndef __ARFEATURE_DRV_CS_H_
#define __ARFEATURE_DRV_CS_H_

#include "segment_common.h"
#include "cfg_table_arfeature.h"

struct arfeature_ops_t;

struct arfeature_dev_t {
	unsigned int        base_addr;
	struct arfeature_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct arfeature_ops_t {
	int (*prepare_cmd)(struct arfeature_dev_t *dev, struct cmd_buf_t *cmd_buf, struct cfg_tab_arfeature_t *table);
};

int arfeature_prepare_cmd(struct arfeature_dev_t *dev,
						  struct cmd_buf_t *cmd_buf, struct cfg_tab_arfeature_t *table);

extern struct arfeature_dev_t g_arfeature_devs[];

#endif // __ARFEATURE_DRV_CS_H_

