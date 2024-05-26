// **********************************************************
// Copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
// File name    vbk_drv.h
// Description:
//
// Version      1.0
// Date         2019-05-21
// *********************************************************/

#ifndef _VBK_DRV_CS_H_
#define _VBK_DRV_CS_H_

#include "adapter_common.h"
#include "cfg_table_vbk.h"

struct vbk_ops_t;

struct vbk_dev_t {
	unsigned int        base_addr;
	struct vbk_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct vbk_ops_t {
	int (*prepare_cmd)(struct vbk_dev_t *dev, struct cmd_buf_t *cmd_buf,
		struct vbk_config_table_t *table);
};

int vbk_prepare_cmd(struct vbk_dev_t *dev, struct cmd_buf_t *cmd_buf,
	struct vbk_config_table_t *table);

extern struct vbk_dev_t g_vbk_devs[];
#endif /* _VBK_DRV_CS_H_ */

