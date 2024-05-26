// *****************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    mc_drv.h
// Description:
//
// Date         2020-04-10
// *****************************************************

#ifndef _MC_DRV_CS_H_
#define _MC_DRV_CS_H_

#include "segment_common.h"
#include "cfg_table_mc.h"
#include "mc_drv_priv.h"
#include "mc_reg_offset.h"
#include "mc_reg_offset_field.h"

struct mc_ops_t;

struct mc_dev_t {
	unsigned int        base_addr;
	struct  mc_ops_t *ops;
	struct  cmd_buf_t *cmd_buf;
};

struct mc_ops_t {
	int (*prepare_cmd)(struct mc_dev_t *dev,
					   struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table);
};

int mc_prepare_cmd(struct mc_dev_t *dev,
				   struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table);

int mc_enable_cmd(struct mc_dev_t *dev,
				  struct cmd_buf_t *cmd_buf, struct cfg_tab_mc_t *table);

int mc_clear_table(struct mc_dev_t *dev,
				   struct cmd_buf_t *cmd_buf);

extern struct mc_dev_t g_mc_devs[];
#endif /* _MC_DRV_CS_H_ */

/* ************************************** END ************** */
