/******************************************************************
* Copyright    Copyright (c) 2015- Hisilicon Technologies CO., Ltd.
* File name    config_table_cmdlst.h
* Description:
*
* Version      1.0
* Date         2015-05-17 16:54:26
* Author       sunjikun_s00236180
******************************************************************/

#ifndef __CONFIG_TABLE_CMDLST_CS_H_INCLUDED__
#define __CONFIG_TABLE_CMDLST_CS_H_INCLUDED__

struct cmdlst_cfg_t {
	unsigned char   to_use;
	unsigned int    slowdown_nrt_channel;
	unsigned int    prefetch;
};

struct cmdlst_ch_cfg_t {
	unsigned char   to_use;
	unsigned int    nrt_channel;
	unsigned int    active_token_nbr_en;
	unsigned int    active_token_nbr;
};

struct cmdlst_sw_ch_mngr_cfg_t {
	unsigned char   to_use;
	unsigned int    sw_link_channel;
	unsigned int    sw_link_token_nbr;
	unsigned int    sw_ressource_request;
	unsigned int    sw_priority;
};

struct cmdlst_vp_rd_cfg_t {
	unsigned char      to_use;
	unsigned int       vp_rd_id;
	unsigned int       rd_addr;
	unsigned int       rd_size;
};

struct cfg_tab_cmdlst_t {
	struct cmdlst_cfg_t     cfg;
	struct cmdlst_ch_cfg_t  ch_cfg;
	struct cmdlst_sw_ch_mngr_cfg_t  sw_ch_mngr_cfg;
	struct cmdlst_vp_rd_cfg_t  vp_rd_cfg;
};

struct cmdlst_state_t {
	unsigned int ch_id;
	unsigned int ch_state;
	unsigned int last_exec;
};

#endif /*__CONFIG_TABLE_CMDLST_CS_H_INCLUDED__*/

/* ********************************* END ********************************* */
