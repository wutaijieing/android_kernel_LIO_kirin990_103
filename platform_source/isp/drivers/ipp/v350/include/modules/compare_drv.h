// ******************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    compare_drv.h
// Description:
//
// Date         2017-10-30
// ******************************************************

#ifndef __COMPARE_DRV_CS_H_
#define __COMPARE_DRV_CS_H_

#include "segment_common.h"
#include "cfg_table_compare.h"

struct compare_ops_t;

struct compare_dev_t {
	unsigned int        base_addr;
	struct compare_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct compare_ops_t {
	int (*prepare_cmd)(struct compare_dev_t *dev, struct cmd_buf_t *cmd_buf,
					   struct cfg_tab_compare_t *table);
};

int compare_prepare_cmd(struct compare_dev_t *dev,
						struct cmd_buf_t *cmd_buf, struct cfg_tab_compare_t *table);

extern struct compare_dev_t g_compare_devs[];

#endif /* __COMPARE_DRV_CS_H_ */

/* ******************** END ********** */
