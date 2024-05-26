/******************************************************************
* copyright    Copyright (c) 2017- Hisilicon Technologies CO., Ltd.
* file name    cvdr_drv.h
* Description:
*
* version      1.0
* date         2017-09-30 11:36:22
*******************************************************************/

#ifndef CVDR_DRV_CS_H
#define CVDR_DRV_CS_H

#include "ipp.h"
#include "adapter_common.h"
#include "config_table_cvdr.h"

#define    CVDR_TOTAL_BYTES_ALIGN  0x8000
#define    DEFAULT_LINE_WRAP       0x3FFF
#define    IPP_ONE_REG_OFFSET   4
#define    IPP_VP_WR_REG_OFFSET 0X10
#define    IPP_VP_RD_REG_OFFSET  0X20

struct cvdr_ops_t;

struct cvdr_dev_t {
	unsigned int base_addr;
	struct cvdr_ops_t *ops;
	struct cmd_buf_t *cmd_buf;
};

struct cvdr_ops_t {
	int (*set_debug_enable)(struct cvdr_dev_t *dev,
		unsigned char wr_peak_en, unsigned char rd_peak_en);
	int (*get_debug_info)(struct cvdr_dev_t *dev,
		unsigned char *wr_peak,
		unsigned char *rd_peak);
	int (*set_vp_wr_ready)(struct cvdr_dev_t *dev,
		unsigned char port, struct cfg_tab_cvdr_t *table);
	int (*set_vp_rd_ready)(struct cvdr_dev_t *dev,
		unsigned char port, struct cfg_tab_cvdr_t *table);

	int (*do_config)(struct cvdr_dev_t *dev,
		struct cfg_tab_cvdr_t *table);
	int (*prepare_cmd)(struct cvdr_dev_t *dev,
		struct cmd_buf_t *cmd_buf,
		struct cfg_tab_cvdr_t *table);
	int (*prepare_vprd_cmd)(struct cvdr_dev_t *dev,
		struct cmd_buf_t *cmd_buf,
		struct cfg_tab_cvdr_t *table);
	int (*prepare_vpwr_cmd)(struct cvdr_dev_t *dev,
		struct cmd_buf_t *cmd_buf,
		struct cfg_tab_cvdr_t *table);
	int (*prepare_nr_cmd)(struct cvdr_dev_t *dev,
		struct cmd_buf_t *cmd_buf,
		struct cfg_tab_cvdr_t *table);
};

int cvdr_prepare_cmd(
	struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	struct cfg_tab_cvdr_t *table);
int cvdr_prepare_vprd_cmd(
	struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	struct cfg_tab_cvdr_t *table);
int cvdr_prepare_vpwr_cmd(
	struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	struct cfg_tab_cvdr_t *table);
int cvdr_prepare_nr_cmd(
	struct cvdr_dev_t *dev,
	struct cmd_buf_t *cmd_buf,
	struct cfg_tab_cvdr_t *table);

extern struct cvdr_dev_t g_cvdr_devs[];

#endif /* CVDR_DRV_CS_H */

/* ********************************* END ********************************* */

