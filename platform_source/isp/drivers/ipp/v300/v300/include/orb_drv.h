// **********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    orb_drv.h
// Description:
//
// Version      1.0
// Date         2017-10-30
// **********************************************************/

#ifndef __ORB_DRV_CS_H_
#define __ORB_DRV_CS_H_

#include "adapter_common.h"
#include "cfg_table_orb.h"

struct orb_ops_t;

struct orb_dev_t {
	unsigned int        base_addr;
	struct orb_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct orb_ops_t {
	int (*prepare_cmd)(struct orb_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_orb_t *table);
};

int orb_prepare_cmd(struct orb_dev_t *dev,
	struct cmd_buf_t *cmd_buf, struct cfg_tab_orb_t *table);

extern struct orb_dev_t g_orb_devs[];

#endif /* __ORB_DRV_CS_H_ */

