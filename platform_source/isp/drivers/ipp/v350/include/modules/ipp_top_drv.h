// *********************************************************
// Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
// File name    hispcpe_cpe_top_drv.h
// Description:
//
// Date         2020-04-15
// *********************************************************

#ifndef _IPP_TOP_DRV_CS_H_
#define _IPP_TOP_DRV_CS_H_

#include "cfg_table_cpe_top.h"
#include "segment_common.h"

struct cpe_top_dev_t {
	unsigned int        base_addr;
	struct _mcf_ops_t *ops;
	struct _cmd_buf_t *cmd_buf;
};

struct cpe_top_ops_t {
	int (*prepare_cmd)(struct cpe_top_dev_t *dev, struct cmd_buf_t *cmd_buf,
					   struct cpe_top_config_table_t *table);
};

int cpe_top_prepare_cmd(struct cpe_top_dev_t *dev,
						struct cmd_buf_t *cmd_buf, struct cpe_top_config_table_t *table);
extern struct cpe_top_dev_t g_cpe_top_devs[];

#endif /* _IPP_TOP_DRV_CS_H_ */
/* *********************** END ************** */
