/******************************************************************
 * Copyright    Copyright (c) 2015- Hisilicon Technologies CO., Ltd.
 * File name    adapter_common.h
 * Description:
 *
 * Version      1.0
 * Date         2019-05-20 14:23:09
 ********************************************************************/

#ifndef ADAPTER_COMMON_CS_H_INCLUDED
#define ADAPTER_COMMON_CS_H_INCLUDED

#include <linux/types.h>
#include "ipp.h"
#include "include/config_table_cvdr.h"
#include "include/config_table_cmdlst.h"
#include "libhwsecurec/securec.h"

#define CMDLST_BUFFER_SIZE         0x10000
#define CMDLST_HEADER_SIZE         128
#define CMDLST_CHANNEL_MAX         16
#define CMDLST_RD_CFG_MAX          8
#define CMDLST_32K_PAGE            32768

#define STRIPE_NUM_EACH_CMP  2
#define STRIPE_NUM_EACH_RDR  1

/* IPP IRQ BIT */
// orb irq
#define IPP_ORB_DONE_IRQ            0
#define IPP_ENH_DONE_IRQ               1
#define IPP_ORB_CVDR_VP_WR_EOF_Y_DS         2
#define IPP_ORB_CVDR_VP_WR_EOF_LEP       3
#define IPP_ORB_CVDR_VP_RD_EOF_Y            4
#define IPP_ENH_CVDR_VP_RD_EOF_YHIST       5
#define IPP_ENH_CVDR_VP_RD_EOF_YIMG       6
#define IPP_ORB_CVDR_VP_WR_EOF_CMDLST      7
#define IPP_ORB_CVDR_VP_RD_EOF_CMDLST       8
#define IPP_ORB_CVDR_VP_WR_SOF_Y_DS       9
#define IPP_ORB_CVDR_VP_WR_SOF_LEP            10
#define IPP_ORB_CVDR_VP_WR_DROPPED_Y_DS         11
#define IPP_ORB_CVDR_VP_WR_DROPPED_LEP        12
#define IPP_ORB_CVDR_VP_RD_SOF_Y           13
#define IPP_ENH_CVDR_VP_RD_SOF_YHIST       14
#define IPP_ENH_CVDR_VP_RD_SOF_YIMG       15

// gf irq
#define IPP_GF_DONE  0
#define IPP_GF_CVDR_VP_WR_EOF_LF_A  1
#define IPP_GF_CVDR_VP_WR_EOF_HF_B  2
#define IPP_GF_CVDR_VP_RD_EOF_SRC_P 3
#define IPP_GF_CVDR_VP_RD_EOF_GUI_I 4
#define IPP_GF_CVDR_VP_WR_EOF_CMDLST  5
#define IPP_GF_CVDR_VP_RD_EOF_CMDLST   6
#define IPP_GF_CVDR_VP_WR_SOF_LF_A  7
#define IPP_GF_CVDR_VP_WR_SOF_HF_B  8
#define IPP_GF_CVDR_VP_WR_DROPPED_LF_A 9
#define IPP_GF_CVDR_VP_WR_DROPPED_HF_B 10
#define IPP_GF_CVDR_VP_RD_SOF_SRC_P     11
#define IPP_GF_CVDR_VP_RD_SOF_GUI_I       12

#define CMDLST_SET_REG(reg, val) cmdlst_set_reg(dev->cmd_buf, reg, val)
#define CMDLST_SET_REG_INCR(reg, size, incr, is_read) \
	cmdlst_set_reg_incr(dev->cmd_buf, reg, size, incr, is_read)
#define CMDLST_SET_REG_DATA(data) cmdlst_set_reg_data(dev->cmd_buf, data)
#define CMDLST_SET_ADDR_ALIGN(size, align) \
	cmdlst_set_addr_align(dev->cmd_buf, size, align)
#define CMDLST_SET_ADDR_OFFSET(size_offset) cmdlst_set_addr_offset(dev->cmd_buf, size_offset)

#define CMDLST_BURST_MAX_SIZE      256
#define CMDLST_PADDING_DATA        0xFFFFFFFD
#define CMDLST_STRIPE_MAX_NUM      80

#define UNSIGNED_INT_MAX      0xffffffff
#define LOG_NAME_LEN          64

enum cmdlst_eof_mode_e {
	CMD_EOF_GF_MODE = 0,
	CMD_EOF_ORB_MODE,
	CMD_EOF_MODE_MAX,
};

enum ipp_cmdlst_resource_share_e {
	IPP_CMD_RES_SHARE_ORB = 0,
	IPP_CMD_RES_SHARE_GF,
	IPP_CMD_RES_SHARE_MAX,
};

enum cmdlst_module_channel_e {
	IPP_GF_CHANNEL_ID = 0,
	IPP_ORB_CHANNEL_ID,
	IPP_CHANNEL_ID_MAX_NUM,
};

enum cmdlst_irq_host_e {
	IPP_CMDLST_IRQ_ACPU = 0,
	IPP_CMDLST_IRQ_ARCH,
	IPP_CMDLST_IRQ_IVP_0,
	IPP_CMDLST_IRQ_IVP_1,
	IPP_CMDLST_IRQ_ARPP,
	IPP_CMDLST_IRQ_PAC,
	IPP_CMDLST_IRQ_MAX_NUM,
};

struct cmdlst_wr_cfg_t {
	unsigned int        buff_wr_addr;
	unsigned int        reg_rd_addr;
	unsigned int        data_size;
	unsigned int        is_incr;
	unsigned int        is_wstrb;
	unsigned int        read_mode;
};

struct cmd_buf_t {
	unsigned long long start_addr;
	unsigned int start_addr_isp_map;
	unsigned int buffer_size;
	unsigned int header_size;
	unsigned long long data_addr;
	unsigned int data_size;
	void *next_buffer;
};

struct schedule_cmdlst_link_t {
	unsigned int stripe_cnt;
	unsigned int stripe_index;
	struct cmd_buf_t    cmd_buf;
	struct cfg_tab_cmdlst_t cmdlst_cfg_tab;
	void *data;
	struct list_head list_link;
};

struct cmdlst_rd_cfg_info_t {
	// read buffer address
	unsigned int fs;
	// read reg num in one stripe
	unsigned int rd_cfg_num;
	// read reg cfgs
	unsigned int rd_cfg[CMDLST_RD_CFG_MAX];
};

struct cmdlst_stripe_info_t {
	unsigned int  is_first_stripe;
	unsigned int  is_last_stripe;
	unsigned int  is_need_set_sop;
	unsigned int  irq_mode;
	unsigned long long  irq_mode_sop;
	unsigned int  hw_priority;
	unsigned int  resource_share;
	unsigned int  en_link;
	unsigned int  ch_link;
	unsigned int  ch_link_act_nbr;
	struct cmdlst_rd_cfg_info_t rd_cfg_info;
};

struct cmdlst_para_t {
	unsigned int stripe_cnt;
	struct cmdlst_stripe_info_t cmd_stripe_info[CMDLST_STRIPE_MAX_NUM];
	void *cmd_entry;
	unsigned int channel_id;
};

enum cfg_irq_type_e {
	SET_IRQ = 0,
	CLR_IRQ,
};

struct crop_region_info_t {
	unsigned int   x;
	unsigned int   y;
	unsigned int   width;
	unsigned int   height;
};

struct isp_size_t {
	unsigned int   width;
	unsigned int   height;
};

struct ipp_stripe_info_t {
	struct crop_region_info_t  crop_region;
	struct isp_size_t  pipe_work_size;
	struct isp_size_t  full_size;

	unsigned int        stripe_cnt;
	unsigned int        overlap_left[MAX_CPE_STRIPE_NUM];
	unsigned int        overlap_right[MAX_CPE_STRIPE_NUM];
	unsigned int        stripe_width[MAX_CPE_STRIPE_NUM];
	unsigned int        stripe_start_point[MAX_CPE_STRIPE_NUM];
	unsigned int        stripe_end_point[MAX_CPE_STRIPE_NUM];
};

struct df_size_constrain_t {
	unsigned int hinc;
	unsigned int pix_align;
	unsigned int out_width;
};

extern void df_size_dump_stripe_info(
	struct ipp_stripe_info_t *p_stripe_info, char *s);
void df_size_split_stripe(
	unsigned int constrain_cnt,
	struct df_size_constrain_t *p_size_constrain,
	struct ipp_stripe_info_t *p_stripe_info,
	unsigned int overlap,
	unsigned int width,
	unsigned int max_stripe_width);

int ipp_eop_handler(enum cmdlst_eof_mode_e mode);
void cmdlst_priv_prepare(void);

int df_sched_prepare(struct cmdlst_para_t *cmdlst_para);
int df_sched_start(struct cmdlst_para_t *cmdlst_para);

int cmdlst_set_buffer_padding(struct cmd_buf_t *cmd_buf);
int df_sched_set_buffer_header(struct cmdlst_para_t *cmdlst_para);
int cmdlst_read_buffer(unsigned int stripe_index,
	struct cmd_buf_t *cmd_buf, struct cmdlst_para_t *cmdlst_para);

void cmdlst_set_addr_align(struct cmd_buf_t *cmd_buf,
	unsigned int size, unsigned int align);
int cmdlst_set_reg(struct cmd_buf_t *cmd_buf,
	unsigned int reg, unsigned int val);
void cmdlst_set_reg_incr(struct cmd_buf_t *cmd_buf,
	unsigned int reg, unsigned int size,
	unsigned int incr, unsigned int is_read);
void cmdlst_set_reg_data(struct cmd_buf_t *cmd_buf, unsigned int data);
void cmdlst_set_addr_offset(struct cmd_buf_t *cmd_buf, unsigned int size_offset);
void cmdlst_do_flush(struct cmdlst_para_t *cmdlst_para);
void dump_addr(unsigned long long addr, int num, char *info);
void cmdlst_buff_dump(struct cmd_buf_t *cmd_buf);
int seg_ipp_set_cmdlst_wr_buf(struct cmd_buf_t *cmd_buf,
	struct cmdlst_wr_cfg_t *wr_cfg);
int seg_ipp_set_cmdlst_wr_buf_cmp(struct cmd_buf_t *cmd_buf,
	struct cmdlst_wr_cfg_t *wr_cfg, unsigned int match_points_offset);

#endif /* ADAPTER_COMMON_CS_H_INCLUDED */

