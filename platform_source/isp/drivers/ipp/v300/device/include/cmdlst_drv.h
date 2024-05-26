/* *****************************************************************
 *   Copyright    Copyright (c) 2015- Hisilicon Technologies CO., Ltd.
 *   File name    cmdlst_drv.h
 *   Description:
 *
 *   Version      1.0
 *   Date         2015-05-17 16:28:35
 *   Author       sunjikun_s00236180
 *******************************************************************/


#ifndef __CMDLST_DRV_CS_H_INCLUDED__
#define __CMDLST_DRV_CS_H_INCLUDED__

#include "ipp.h"
#include "config_table_cmdlst.h"

struct cmdlst_ops_t;

struct cmdlst_dev_t {
	unsigned int base_addr;
	struct cmdlst_ops_t *ops;
};

struct cmdlst_ops_t {
	int (*set_config)(struct cmdlst_dev_t *dev,
					struct cmdlst_cfg_t *cfg);
	int (*set_sw_ch)(struct cmdlst_dev_t *dev,
					struct cmdlst_ch_cfg_t *cfg,
					unsigned int channel_id);
	int (*set_sw_ch_mngr)(struct cmdlst_dev_t *dev,
					struct cmdlst_sw_ch_mngr_cfg_t *cfg,
					unsigned int channel_id);
	int (*set_vp_rd)(struct cmdlst_dev_t *dev,
					struct cmdlst_vp_rd_cfg_t *cfg);
	int (*do_config)(struct cmdlst_dev_t *dev,
					struct cfg_tab_cmdlst_t *table);
	int (*set_branch)(struct cmdlst_dev_t *dev, int ch_id);
	int (*get_state)(struct cmdlst_dev_t *dev,
				struct cmdlst_state_t *st);
};

extern int cmdlst_get_state(
	struct cmdlst_dev_t *dev,
	struct cmdlst_state_t *st);
extern int cmdlst_do_config(
	struct cmdlst_dev_t *dev,
	struct cfg_tab_cmdlst_t *config_table);
int cmdlst_set_vp_rd(
	struct cmdlst_dev_t *dev,
	struct cmdlst_vp_rd_cfg_t *cfg);
extern  struct cmdlst_dev_t g_cmdlst_devs[];

#endif /*__CMDLST_DRV_CS_H_INCLUDED__*/



