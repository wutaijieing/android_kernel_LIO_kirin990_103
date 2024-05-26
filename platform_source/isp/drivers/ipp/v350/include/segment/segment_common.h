/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_common.h
 * Description:
 *
 * Date         2020-04-17 14:23:09
 ********************************************************************/

#ifndef SEGMENT_COMMON_CS_H_INCLUDED
#define SEGMENT_COMMON_CS_H_INCLUDED

#include <linux/types.h>
#include "ipp.h"
#include "cfg_table_cvdr.h"
#include "cfg_table_cmdlst.h"
#include "libhwsecurec/securec.h"

#define CMDLST_BUFFER_SIZE         0x10000
#define CMDLST_HEADER_SIZE         288   // v350 isp 288 16w header+ 54w irq+ 2w padding
#define CMDLST_CHANNEL_MAX         16
#define CMDLST_RD_CFG_MAX          8
#define CMDLST_32K_PAGE            32768

#define STRIPE_NUM_EACH_RDR  1

#define MATCHER_KPT_MAX          (4096) // This is the maximum amount of data that the rdr and cmp can process.
#define ARF_OUT_DESC_SIZE       (144*MATCHER_KPT_MAX + 0x8000)
#define RDR_OUT_DESC_SIZE        (160*MATCHER_KPT_MAX + 0x8000)
#define CMP_IN_INDEX_NUM         RDR_OUT_DESC_SIZE
#define MATCHER_LAYER_MAX          6 // v350 max support 6 frames matcher due to token number
#define ARF_AR_MODE_CNT		10
#define ARF_DMAP_MODE_CNT	3
#define ARF_DETECTION_MODE 4


/* IPP IRQ BIT */
// ipp orb_enh irq
#define IPP_ORB_ENH_DONE                        0
#define IPP_ORB_ENH_CVDR_VP_WR_EOF_YGUASS       1
#define IPP_ORB_ENH_CVDR_VP_RD_EOF_YHIST        2
#define IPP_ORB_ENH_CVDR_VP_RD_EOF_YIMG         3
#define IPP_ORB_ENH_CVDR_VP_WR_EOF_CMDLST       4
#define IPP_ORB_ENH_CVDR_VP_RD_EOF_CMDLST       5
#define IPP_ORB_ENH_CVDR_VP_WR_SOF_YGUASS       6
#define IPP_ORB_ENH_CVDR_VP_WR_DROPPED_YGUASS   7
#define IPP_ORB_ENH_CVDR_VP_RD_SOF_YHIST        8
#define IPP_ORB_ENH_CVDR_VP_RD_SOF_YIMAGE       9

// arfeature irq
#define IPP_ENH_DONE_IRQ               0
#define IPP_ARF_DONE_IRQ            1
#define IPP_ARF_CVDR_VP_WR_EOF_PRY_1       2
#define IPP_ARF_CVDR_VP_WR_EOF_PRY_2       3
#define IPP_ARF_CVDR_VP_WR_EOF_DOG_0       4
#define IPP_ARF_CVDR_VP_WR_EOF_DOG_1       5
#define IPP_ARF_CVDR_VP_WR_EOF_DOG_2       6
#define IPP_ARF_CVDR_VP_WR_EOF_DOG_3       7
#define IPP_ARF_CVDR_VP_RD_EOF_ORG_IMG     8
#define IPP_ARF_CVDR_VP_RD_EOF_PRY_2       9
#define IPP_ARF_CVDR_VP_RD_EOF_DOG_1       10
#define IPP_ARF_CVDR_VP_RD_EOF_DOG_2       11
#define IPP_ARF_CVDR_VP_WR_EOF_CMDLST      12
#define IPP_ARF_CVDR_VP_RD_EOF_CMDLST      13
#define IPP_ARF_CVDR_VP_WR_SOF_PRY_1       14
#define IPP_ARF_CVDR_VP_WR_SOF_PRY_2       15
#define IPP_ARF_CVDR_VP_WR_SOF_DOG_0       16
#define IPP_ARF_CVDR_VP_WR_SOF_DOG_1       17
#define IPP_ARF_CVDR_VP_WR_SOF_DOG_2       18
#define IPP_ARF_CVDR_VP_WR_SOF_DOG_3       19
#define IPP_ARF_CVDR_VP_WR_DROPPED_PRY_1   20
#define IPP_ARF_CVDR_VP_WR_DROPPED_PRY_2   21
#define IPP_ARF_CVDR_VP_WR_DROPPED_DOG_0   22
#define IPP_ARF_CVDR_VP_WR_DROPPED_DOG_1   23
#define IPP_ARF_CVDR_VP_WR_DROPPED_DOG_2   24
#define IPP_ARF_CVDR_VP_WR_DROPPED_DOG_3   25
#define IPP_ARF_CVDR_VP_RD_SOF_ORG_IMG     26
#define IPP_ARF_CVDR_VP_RD_SOF_PRY_2       27
#define IPP_ARF_CVDR_VP_RD_SOF_DOG_1       28
#define IPP_ARF_CVDR_VP_RD_SOF_DOG_2       29

// rdr irq
#define IPP_RDR_IRQ_DONE                    0
#define IPP_RDR_CVDR_VP_RD_EOF_FP           1
#define IPP_RDR_CVDR_VP_WR_EOF_CMDLST       2
#define IPP_RDR_CVDR_VP_RD_EOF_CMDSLT       3
#define IPP_RDR_CVDR_VP_RD_SOF_FP           4

// cmp irq
#define IPP_CMP_IRQ_DONE                    0
#define IPP_CMP_CVDR_VP_RD_EOF_FP           1
#define IPP_CMP_CVDR_VP_WR_EOF_CMDLST       2
#define IPP_CMP_CVDR_VP_RD_EOF_CMDLST       3
#define IPP_CMP_CVDR_VP_RD_SOF_FP           4

// mc irq
#define IPP_MC_IRQ_DONE                    0
#define IPP_MC_CVDR_VP_WR_EOF_CMDLST       1
#define IPP_MC_CVDR_VP_RD_EOF_CMDLST       2

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

// hiof irq
#define IPP_HIOF_DONE  0
#define IPP_HIOF_CVDR_VP_WR_EOF_TNR  1
#define IPP_HIOF_CVDR_VP_WR_EOF_MD  2
#define IPP_HIOF_CVDR_VP_RD_EOF_CUR_FRAME 3
#define IPP_HIOF_CVDR_VP_RD_EOF_REF_FRAME 4
#define IPP_HIOF_CVDR_VP_WR_EOF_CMDLST  5
#define IPP_HIOF_CVDR_VP_RD_EOF_CMDLST   6
#define IPP_HIOF_CVDR_VP_WR_SOF_TNR  7
#define IPP_HIOF_CVDR_VP_WR_DROPPED_TNR  8
#define IPP_HIOF_CVDR_VP_WR_SOF_MD 9
#define IPP_HIOF_CVDR_VP_WR_DROPPED_MD 10
#define IPP_HIOF_CVDR_VP_RD_SOF_CUR_FRAME     11
#define IPP_HIOF_CVDR_VP_RD_SOF_REF_FRAME       12

#if 1
#define macro_cmdlst_set_reg(reg, val) cmdlst_set_reg(dev->cmd_buf, reg, val)
#else
#define macro_cmdlst_set_reg(reg, val) cmdlst_set_reg_by_cpu(reg, val)
#endif
#define macro_cmdlst_set_reg_incr(reg, size, incr, is_read) \
	cmdlst_set_reg_incr(dev->cmd_buf, reg, size, incr, is_read)
#define macro_cmdlst_set_reg_data(data) cmdlst_set_reg_data(dev->cmd_buf, data)
#define macro_cmdlst_set_addr_align(align) cmdlst_set_addr_align(dev->cmd_buf, align)
#define macro_cmdlst_set_reg_burst_data_align(reg_num, align) \
	cmdlst_set_reg_burst_data_align(dev->cmd_buf, reg_num, align)
#define macro_cmdlst_set_addr_offset(size_offset) cmdlst_set_addr_offset(dev->cmd_buf, size_offset)

#define CMDLST_BURST_MAX_SIZE      256
#define CMDLST_PADDING_DATA        0xFFFFFFFD
#define CMDLST_STRIPE_MAX_NUM      80

#define UNSIGNED_INT_MAX      0xffffffff
#define LOG_NAME_LEN          64

enum cmdlst_frame_prio_e {
	CMD_PRIO_LOW  = 0,
	CMD_PRIO_HIGH = 1,
};

enum cmdlst_eof_mode_e {
	CMD_EOF_GF_MODE = 0,
	CMD_EOF_HIOF_MODE,
	CMD_EOF_ARFEATURE_MODE,
	CMD_EOF_RDR_MODE,
	CMD_EOF_CMP_MODE,
	CMD_EOF_ORB_ENH_MODE,
	CMD_EOF_MC_MODE,
	CMD_EOF_MODE_MAX,
};

enum ipp_cmdlst_resource_share_e {
	IPP_CMD_RES_SHARE_ARFEATURE = 0,
	IPP_CMD_RES_SHARE_RDR,
	IPP_CMD_RES_SHARE_CMP,
	IPP_CMD_RES_SHARE_MC,
	IPP_CMD_RES_SHARE_HIOF,
	IPP_CMD_RES_SHARE_GF,
	IPP_CMD_RES_SHARE_ORB_ENH,
};

enum cmdlst_module_channel_e {
	IPP_GF_CHANNEL_ID = 0,
	IPP_HIOF_CHANNEL_ID = 1,
	IPP_ARFEATURE_CHANNEL_ID = 2,
	IPP_RDR_CHANNEL_ID = 3,
	IPP_CMP_CHANNEL_ID = 4,
	IPP_MC_CHANNEL_ID = 6,
	IPP_MATCHER_CHANNEL_ID = 7,
	IPP_ORB_ENH_CHANNEL_ID = 8,
	IPP_CHANNEL_ID_MAX_NUM = 9,
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

enum cmdlst_ctrl_chn_e {
	IPP_CMDLST_SELECT_ORB_ENH = 0,
	IPP_CMDLST_SELECT_ARFF = 1,
	IPP_CMDLST_SELECT_RDR = 2,
	IPP_CMDLST_SELECT_CMP = 3,
	IPP_CMDLST_SELECT_MC = 4,
	IPP_CMDLST_SELECT_GF = 5,
	IPP_CMDLST_SELECT_HIOF = 8,
};

enum work_module_e {
	ENH_ONLY = 0,
	ARFEATURE_ONLY = 1,
	MATCHER_ONLY = 2,
	MC_ONLY = 3,
	ENH_ARF = 4,
	ARF_MATCHER = 5,
	ENH_ARF_MATCHER = 6,
	MATCHER_MC = 7,
	ARF_MATCHER_MC = 8,
	ENH_ARF_MATCHER_MC = 9,
	OPTICAL_FLOW = 10,
	MODULE_MAX,
};

enum ipp_path_cascade_en_e {
	CASCADE_DISABLE  = 0,
	CASCADE_ENABLE = 1,
};

enum work_frame_e {
	CUR_ONLY = 0,
	CUR_REF = 1,
};

enum frame_flag_e {
	FRAME_CUR = 0,
	FRAME_REF  = 1,
	MAX_FRAME_FLAG,
};

typedef enum _arfeature_stat_buff_e {
	DESC_BUFF,
	MINSCR_KPTCNT_BUFF,
	SUM_SCORE_BUFF,
	KPT_NUM_BUFF,
	TOTAL_KPT_BUFF,
	STAT_BUFF_MAX,
} arfeature_stat_buff_e;

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

enum enh_vpb_cfg_e {
	CONNECT_TO_CVDR = 1,
	CONNECT_TO_ORB  = 2,
	CONNECT_TO_CVDR_AND_ORB = 3,
};

enum arfeature_work_mode_e {
	DMAP_GAUSS_FIRST = 0,
	AR_GAUSS_FIRST = 1,
	GAUSS_MIDDLE = 2,
	GAUSS_LAST = 3,
	DETECT_AND_DESCRIPTOR = 4,
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

typedef struct _split_stripe_tmp_t {
	unsigned int overlap;
	unsigned int in_full_width;
	unsigned int active_stripe_width;
	unsigned int max_in_stripe_align;
	unsigned int max_frame_width;
	unsigned int stripe_width;
	unsigned int input_align;
	unsigned int stripe_cnt;

	unsigned int stripe_start;
	unsigned int stripe_end;
	unsigned long long int active_stripe_end;
	unsigned int last_stripe_end;
} split_stripe_tmp_t;

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
int cmdlst_priv_prepare(void);

int df_sched_prepare(struct cmdlst_para_t *cmdlst_para);
int df_sched_start(struct cmdlst_para_t *cmdlst_para);

int cmdlst_set_buffer_padding(struct cmd_buf_t *cmd_buf);
int df_sched_set_buffer_header(struct cmdlst_para_t *cmdlst_para);
int cmdlst_read_buffer(unsigned int stripe_index, struct cmd_buf_t *cmd_buf, struct cmdlst_para_t *cmdlst_para);

int cmdlst_set_reg(struct cmd_buf_t *cmd_buf, unsigned int reg, unsigned int val);
void cmdlst_set_addr_align(struct cmd_buf_t *cmd_buf, unsigned int align);
void cmdlst_set_reg_burst_data_align(struct cmd_buf_t *cmd_buf, unsigned int reg_num, unsigned int align);
void cmdlst_set_reg_incr(struct cmd_buf_t *cmd_buf, unsigned int reg, unsigned int size, unsigned int incr,
						 unsigned int is_read);
void cmdlst_set_reg_data(struct cmd_buf_t *cmd_buf, unsigned int data);
void cmdlst_set_addr_offset(struct cmd_buf_t *cmd_buf, unsigned int size_offset);
void cmdlst_do_flush(struct cmdlst_para_t *cmdlst_para);
void dump_addr(unsigned long long addr, unsigned int start_addr_isp_map, int num, char *info);
void cmdlst_buff_dump(struct cmd_buf_t *cmd_buf);
void cmdlst_set_reg_by_cpu(unsigned int reg, unsigned int val);

int seg_ipp_set_cmdlst_wr_buf(struct cmd_buf_t *cmd_buf, struct cmdlst_wr_cfg_t *wr_cfg);
int seg_ipp_set_cmdlst_wr_buf_cmp(struct cmd_buf_t *cmd_buf, struct cmdlst_wr_cfg_t *wr_cfg,
								  unsigned int match_points_offset);

#endif /* SEGMENT_COMMON_CS_H_INCLUDED */
