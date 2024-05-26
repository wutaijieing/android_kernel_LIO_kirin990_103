// *******************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    reorder_drv.h
// Description:
//
// Date         2020-04-10
// *******************************************************/

#ifndef __REORDER_DRV_CS_H_
#define __REORDER_DRV_CS_H_

#include "segment_common.h"
#include "cfg_table_reorder.h"

struct reorder_ops_t;

struct reorder_dev_t {
	unsigned int        base_addr;
	struct reorder_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct reorder_ops_t {
	int (*prepare_cmd)(struct reorder_dev_t *dev, struct cmd_buf_t *cmd_buf,
					   struct cfg_tab_reorder_t *table);
};

int reorder_prepare_cmd(struct reorder_dev_t *dev, struct cmd_buf_t *cmd_buf,
						struct cfg_tab_reorder_t *table);

extern struct reorder_dev_t g_reorder_devs[];

#endif /* __REORDER_DRV_CS_H_ */

