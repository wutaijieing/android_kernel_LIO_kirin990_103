// *******************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    hiof_drv.h
// Description:
//
// Date         2020-04-10
// *******************************************************/

#ifndef __HIOF_DRV_CS_H_
#define __HIOF_DRV_CS_H_

#include "segment_common.h"
#include "cfg_table_hiof.h"

struct hiof_ops_t;

struct hiof_dev_t {
	unsigned int        base_addr;
	struct hiof_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct hiof_ops_t {
	int (*prepare_cmd)(struct hiof_dev_t *dev, struct cmd_buf_t *cmd_buf, struct cfg_tab_hiof_t *table);
};

int hiof_prepare_cmd(struct hiof_dev_t *dev, struct cmd_buf_t *cmd_buf, struct cfg_tab_hiof_t *table);

extern struct hiof_dev_t g_hiof_devs[];

#endif /* __HIOF_DRV_CS_H_ */
