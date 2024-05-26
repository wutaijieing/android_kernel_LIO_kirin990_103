/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_common.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
#include <linux/types.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include "segment_common.h"
#include "memory.h"
#include "cmdlst_drv.h"
#include "cmdlst_reg_offset.h"
#include "cvdr_drv.h"
#include "ipp_top_reg_offset.h"

#define SCALER_RATIO          4
#define UNSIGNED_INT_MAX      0xffffffff
#define CMDLST_HEADER_PADDING_SIZE 16
#define CHECK_OFF 0

unsigned int frame_num;

struct cmdlst_channel_t {
	struct list_head ready_list;
};

struct cmdlst_priv_t {
	struct cmdlst_channel_t cmdlst_chan[CMDLST_CHANNEL_MAX];
};

static struct cmdlst_priv_t g_cmdlst_priv;

static int cmdlst_enqueue(unsigned int channel_id,
						  struct cmdlst_para_t *cmdlst_para);

static int cmdlst_start(struct cfg_tab_cmdlst_t *cmdlst_cfg, unsigned int channel_id);

static int cmdlst_set_buffer_header(unsigned int stripe_index,
									struct cmd_buf_t *cmd_buf,
									struct cmdlst_para_t *cmdlst_para);

static int cmdlst_set_irq_mode(struct cmdlst_para_t *cmdlst_para,
							   unsigned int stripe_index);

static int cmdlst_set_vpwr(struct cmd_buf_t *cmd_buf,
						   struct cmdlst_rd_cfg_info_t *rd_cfg_info);

static int ipp_update_cmdlst_cfg_tab(struct cmdlst_para_t *cmdlst_para);

void cmdlst_set_addr_align(struct cmd_buf_t *cmd_buf, unsigned int align)
{
	unsigned int i = 0;
	unsigned int size = 2;
	unsigned int padding_index = 0;

	if (align == 0) {
		loge("Failed : align is equal to zero: %d", align);
		return;
	}

	padding_index =
		(align - ((cmd_buf->data_addr + (size + 2) * 4) % align)) / 4;
	padding_index = (padding_index == 4) ? 0 : padding_index;

	if ((cmd_buf->data_size + padding_index * 4) > cmd_buf->buffer_size) {
		loge("Failed : cmdlst buffer is full: %u, padding_index = %d",
			 cmd_buf->data_size, padding_index);
		return;
	}

	for (i = 0; i < padding_index; i++) {
		*(unsigned int *)(cmd_buf->data_addr) = CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}
}

void cmdlst_set_reg_burst_data_align(struct cmd_buf_t *cmd_buf, unsigned int reg_num, unsigned int align)
{
	unsigned int padding_num = 0;
	unsigned int burst_cnt = 0;
	unsigned int data_size = 0;
	unsigned int tmp = 0xF;
	unsigned int i = 0;

	if (((cmd_buf->data_addr & tmp) != 0) || (reg_num == 0)) {
		logi("This function does not work; input para: data_addr = 0x%llx, reg_num = %d",
			 cmd_buf->data_addr, reg_num);
		return;
	}

	burst_cnt = (reg_num + (CMDLST_BURST_MAX_SIZE - 1)) / CMDLST_BURST_MAX_SIZE;
	padding_num = (4 - ((reg_num + burst_cnt) % 4));
	padding_num = (padding_num == 4) ? 0 : padding_num;
	data_size = (reg_num + burst_cnt) * 4;
	tmp = ((cmd_buf->data_addr + data_size - 1) >> 4) << 4;

	if (((tmp + 0x10) & (align - 1)) != 0)
		padding_num = padding_num + 4;

	for (i = 0; i < padding_num; i++) {
		*(unsigned int *)(cmd_buf->data_addr) = CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}

	return;
}

/**********************************************************
 * function name: cmdlst_set_reg
 *
 * description:
 *              fill current cmdlst buf
 * input:
 *     cmd_buf: current cmdlst buf to config.
 *     reg    : reg_addr
 *     value  : reg_val
 * return:
 *      NULL
 ***********************************************************/
int  cmdlst_set_reg(struct cmd_buf_t *cmd_buf,
					unsigned int reg, unsigned int val)
{
	if (cmd_buf->data_addr == 0) {
		loge("Failed : cmdlst buffer is full: %u, @0x%x", cmd_buf->data_size, reg);
		return ISP_IPP_ERR;
	}

	if ((cmd_buf->data_size + 8) <= cmd_buf->buffer_size) {
		*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) =
			((reg) & 0x000fffff);
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
		*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) = (val);
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
#if IPP_UT_DEBUG
		get_register_info(reg, val);
#endif
	} else {
		loge("Failed : cmdlst buffer is full: %u, @0x%x", cmd_buf->data_size, reg);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cmdlst_update_buffer_header
 *
 * description:
 *              fill current cmdlst buf
 * input:
 *     cmd_buf: current cmdlst buf to config.
 * return:
 *      NULL
 ***********************************************************/

void cmdlst_update_buffer_header(
	struct schedule_cmdlst_link_t *cur_point,
	struct schedule_cmdlst_link_t *next_point,
	unsigned int channel_id)
{
	unsigned int next_hw_prio = next_point->cmdlst_cfg_tab.sw_ch_mngr_cfg.sw_priority;
	unsigned int next_hw_resource = next_point->cmdlst_cfg_tab.sw_ch_mngr_cfg.sw_resource;
	struct cmd_buf_t *cmd_buf = NULL;
	struct cmd_buf_t *next_buf = NULL;
	unsigned int cmdlst_offset_addr   = JPG_CMDLST_OFFSET;
	cur_point->cmd_buf.next_buffer = (void *)&next_point->cmd_buf;
	cmd_buf = &cur_point->cmd_buf;
	next_buf = cmd_buf->next_buffer;

	if (next_buf == NULL) {
		unsigned int i = 0;

		for (i = 0; i < CMDLST_HEADER_PADDING_SIZE; i++)
			*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x4 * i) = CMDLST_PADDING_DATA;

		return;
	}

	/* ignore 16 bytes of padding data at the beginning */
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x0) = cmdlst_offset_addr + CMDLST_HW_CH_MNGR_0_REG + 0x80 *
			channel_id;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x4) = (next_hw_prio << 31) | (next_hw_resource << 8);
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x8) = cmdlst_offset_addr + CMDLST_HW_CVDR_RD_DATA_3_0_REG + 0x80 *
			channel_id;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0xC) = next_buf->start_addr_isp_map >> 2;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x10) = cmdlst_offset_addr + CMDLST_HW_CVDR_RD_DATA_0_0_REG + 0x80 *
			channel_id;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x14) = (((next_buf->start_addr_isp_map + next_buf->data_size) &
			0xFFFFE000) >> 2) | 0x000001BE;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x18) = cmdlst_offset_addr + CMDLST_HW_CVDR_RD_DATA_1_0_REG + 0x80 *
			channel_id;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x1C) = (next_buf->data_size >> 3) - 1;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x20) = cmdlst_offset_addr + CMDLST_HW_CVDR_RD_DATA_2_0_REG + 0x80 *
			channel_id;
	*(unsigned int *)(uintptr_t)(cmd_buf->start_addr + 0x24) = 0;
}

void cmdlst_update_buffer_header_last(
	struct schedule_cmdlst_link_t *cur_point,
	struct schedule_cmdlst_link_t *next_point,
	unsigned int channel_id)
{
	struct cmd_buf_t *cmd_buf = &cur_point->cmd_buf;
	unsigned int i  = 0;

	for (i = 0; i < CMDLST_HEADER_PADDING_SIZE; i++)
		*(unsigned int *)(uintptr_t)(cmd_buf->start_addr
									 + 0x4 * i) = CMDLST_PADDING_DATA;
}

/**********************************************************
 * function name: cmdlst_set_reg_incr
 *
 * description:
 *              current cmdlst buf
 * input:
 *     cmd_buf: current cmdlst buf to config.
 *     reg    : register start address
 *     size   : register numbers
 *     incr   : register address increment or not
 * return:
 *      NULL
 ***********************************************************/
void cmdlst_set_reg_incr(struct cmd_buf_t *cmd_buf,
						 unsigned int reg, unsigned int size, unsigned int incr,
						 unsigned int is_read)
{
	if ((cmd_buf->data_size + (size + 1) * 4) > cmd_buf->buffer_size) {
		loge("Failed : cmdlst buffer is full: %u, @0x%x", cmd_buf->data_size, reg);
		return;
	}

#if IPP_UT_DEBUG
	get_register_incr_info(reg, size);
#endif
	*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) =
		((reg - JPG_SUBSYS_BASE_ADDR) & 0x001ffffc) |
		(((size - 1) & 0xFF) << 24) | ((incr & 0x1) << 1) | (is_read & 0x1);
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
}

/**********************************************************
 * function name: cmdlst_set_reg_data
 *
 * description:
 *              current cmdlst buf
 * input:
 *     cmd_buf: current cmdlst buf to config.
 *     data   : register value
 * return:
 *      NULL
 ***********************************************************/
void cmdlst_set_reg_data(struct cmd_buf_t *cmd_buf, unsigned int data)
{
	*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) = data;
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
#if IPP_UT_DEBUG
	get_register_incr_data(data);
#endif
}

void cmdlst_set_addr_offset(struct cmd_buf_t *cmd_buf, unsigned int size_offset)
{
	cmd_buf->data_addr += 4 * size_offset;
	cmd_buf->data_size += 4 * size_offset;
}

static int cmdlst_set_header_cmdlst_reg(struct cmd_buf_t *cmd_buf,
										struct cmdlst_para_t *cmdlst_para,
										unsigned int stripe_index)
{
	struct cmdlst_stripe_info_t *tile_info = &cmdlst_para->cmd_stripe_info[stripe_index];
	unsigned int wait_eop = 0x1; // wait HW EOP
	unsigned int token_mode = 0x0; // EOP mode
	unsigned int token_cfg = ((tile_info->ch_link_act_nbr & 0xF) << 16) | ((
								 tile_info->ch_link & 0x1F) << 8) | token_mode;
	unsigned int cmdlst_offset_addr = JPG_CMDLST_OFFSET + CH_CFG_OFFSET * cmdlst_para->channel_id;
	cmdlst_set_reg(cmd_buf, cmdlst_offset_addr + CMDLST_CMD_CFG_0_REG, wait_eop);
	cmdlst_set_reg(cmd_buf, cmdlst_offset_addr + CMDLST_TOKEN_CFG_0_REG, token_cfg);
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cmdlst_set_buffer_header
 *
 * description:
 *               set current cmdlst buf header.
 * input:
 *     cmd_buf      : current cmdlst buf
 *     irq          : irq mode.
 *     cpu_enable   : 1 to hardware,3to hardware and cpu
 * return:
 *      0;
 ***********************************************************/
static int cmdlst_set_buffer_header(unsigned int stripe_index,
									struct cmd_buf_t *cmd_buf,
									struct cmdlst_para_t *cmdlst_para)
{
	unsigned int idx;
	unsigned int ret;
	logd("+");

	/* reserve 10 words to update next buffer later */
	for (idx = 0; idx < CMDLST_HEADER_PADDING_SIZE; idx++) {
		*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) =
			CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}

	ret = cmdlst_set_header_cmdlst_reg(cmd_buf, cmdlst_para, stripe_index);
	if (ret != 0) {
		loge("Failed : cmdlst_set_header_cmdlst_reg");
		return ISP_IPP_ERR;
	}

	ret = cmdlst_set_irq_mode(cmdlst_para, stripe_index);
	if (ret != 0) {
		loge("Failed : cmdlst_set_irq_mode");
		return ISP_IPP_ERR;
	}

	while (cmd_buf->data_size < CMDLST_HEADER_SIZE) {
		*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) =
			CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}

	logd("-");
	return ISP_IPP_OK;
}

int cmdlst_read_buffer(unsigned int stripe_index,
					   struct cmd_buf_t *cmd_buf,
					   struct cmdlst_para_t *cmdlst_para)
{
	unsigned int idx;
	struct cmdlst_rd_cfg_info_t *rd_cfg_info =
			&cmdlst_para->cmd_stripe_info[stripe_index].rd_cfg_info;
	logd("+");
	logd("stripe_cnt =%d, fs =0x%x", cmdlst_para->stripe_cnt, rd_cfg_info->fs);

	if (rd_cfg_info->fs != 0) {
		cmdlst_set_vpwr(cmd_buf, rd_cfg_info);

		for (idx = 0; idx < rd_cfg_info->rd_cfg_num; idx++) {
			*(unsigned int *)(uintptr_t)(cmd_buf->data_addr)
				= rd_cfg_info->rd_cfg[idx];
			cmd_buf->data_addr += 4;
			cmd_buf->data_size += 4;
		}
	}

	*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) = CMDLST_PADDING_DATA;
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
	logd("-");
	return ISP_IPP_OK;
}

static int set_irq_mode_arf_rdr_cmp_gf(struct cmd_buf_t *cmd_buf, unsigned int ipp_resource,
									   unsigned int channel_id, unsigned int irq_mode)
{
	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_ARFEATURE)) {
		/* clear all ARFEATURE irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_ARF_IRQ_REG0_REG, 0x3FFFFFFF); // clr
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_ARF_IRQ_REG1_REG, 0x0); // outen to software
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_ARF_IRQ_REG2_REG,
					   (0x3FFFFFFF & ~irq_mode)); // mask
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_ARFF);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_RDR)) {
		/* clear all RDR irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_RDR_IRQ_REG0_REG, 0x1F);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_RDR_IRQ_REG1_REG, 0x0);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_RDR_IRQ_REG2_REG, (0x1F & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_RDR);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_CMP)) {
		/* clear all CMP irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMP_IRQ_REG0_REG, 0x1F);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMP_IRQ_REG1_REG, 0x0);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMP_IRQ_REG2_REG, (0x1F & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_CMP);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_GF)) {
		/* clear all GF irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_GF_IRQ_REG0_REG, 0x1FFF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_GF_IRQ_REG1_REG, 0x0);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_GF_IRQ_REG2_REG, (0x1FFF & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_GF);
	}

	return ISP_IPP_OK;
}

static int set_irq_mode_hiof_enh_mc(struct cmd_buf_t *cmd_buf, unsigned int ipp_resource,
									unsigned int channel_id, unsigned int irq_mode)
{
	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_HIOF)) {
		/* clear all HIOF irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_HIOF_IRQ_REG0_REG, 0x1FFF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_HIOF_IRQ_REG2_REG, (0x1FFF & ~irq_mode)); // hiof_irq_mask
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_HIOF_IRQ_REG1_REG, 0x0); // hiof_irq_outen
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_HIOF);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_ORB_ENH)) {
		/* clear all ORB_ENH irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_ENH_IRQ_REG0_REG, 0x3FF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_ENH_IRQ_REG1_REG, 0x0);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_ENH_IRQ_REG2_REG, (0x3FF & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_ORB_ENH);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_MC)) {
		/* clear all MC irq REG1 mask */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_MC_IRQ_REG0_REG, 0x7);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_MC_IRQ_REG2_REG, (0x7 & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_MC_IRQ_REG1_REG, 0x0);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
					   0x10 * channel_id, IPP_CMDLST_SELECT_MC);
	}

	return ISP_IPP_OK;
}


/**********************************************************
 * function name: cmdlst_set_irq_mode
 *
 * description:
 *               set irq mode according frame type.
 * input:
 * return:
 *      0;
 ***********************************************************/
static int cmdlst_set_irq_mode(struct cmdlst_para_t *cmdlst_para, unsigned int stripe_index)
{
	struct schedule_cmdlst_link_t *cmd_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	struct cmd_buf_t *cmd_buf = &cmd_entry[stripe_index].cmd_buf;
	unsigned int irq_mode =
		cmdlst_para->cmd_stripe_info[stripe_index].irq_mode;
	unsigned int channel_id = cmdlst_para->channel_id;
	unsigned int cpu_enable =
		cmdlst_para->cmd_stripe_info[stripe_index].is_last_stripe ? 3 : 1;
	unsigned int ipp_resource =
		cmdlst_para->cmd_stripe_info[stripe_index].resource_share;
	cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
				   IPP_TOP_CMDLST_CTRL_PM_0_REG + 0x10 * channel_id,
				   cpu_enable);
	cmdlst_set_reg(cmd_buf,
				   JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_PM_0_REG +
				   0x10 * channel_id, 0);
	set_irq_mode_arf_rdr_cmp_gf(cmd_buf, ipp_resource, channel_id, irq_mode);
	set_irq_mode_hiof_enh_mc(cmd_buf, ipp_resource, channel_id, irq_mode);

	logd("stripe_index = %d, channel_id = %d, cpu_enable = %d, irq_mode = 0x%x",
		stripe_index, channel_id, cpu_enable, irq_mode);
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cmdlst_set_vpwr
 *
 * description:
 *               set cmdlst read configuration
 * input:
 *     cmd_buf      : current cmdlst buf
 *     rd_cfg_info  : read configuration
 * return:
 *      0;
 ***********************************************************/
static int cmdlst_set_vpwr(struct cmd_buf_t *cmd_buf,
						   struct cmdlst_rd_cfg_info_t *rd_cfg_info)
{
	struct cfg_tab_cvdr_t cmdlst_w3_1_table;

	if (cmd_buf == NULL) {
		loge("Failed:cmdlst buf is null!");
		return ISP_IPP_ERR;
	}

	if (rd_cfg_info == NULL) {
		loge("Failed:cmdlst read cfg info is null!");
		return ISP_IPP_ERR;
	}

	loge_if(memset_s(&cmdlst_w3_1_table, sizeof(struct cfg_tab_cvdr_t),
					 0, sizeof(struct cfg_tab_cvdr_t)));
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].to_use = 1;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].id =
		get_cvdr_vp_wr_port_num(IPP_VP_WR_CMDLST);
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.fs_addr =
		rd_cfg_info->fs;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_fmt = DF_D32;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_expan = 1;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.last_page =
		(rd_cfg_info->fs + CMDLST_32K_PAGE - 1) >> 15;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_stride = 0;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_wrap = 0x3FFF;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter0 = 0xF;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter1 = 0xF;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter2 = 0xF;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter3 = 0xF;
	cmdlst_w3_1_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter_reload =
		0xF;
	loge_if_ret(g_cvdr_devs[0].ops->prepare_cmd(&g_cvdr_devs[0],
				cmd_buf, &cmdlst_w3_1_table));
	return ISP_IPP_OK;
}
/**********************************************************
 * function name: cmdlst_set_buffer_padding
 *
 * description:
 *               set cmd buf rest as padding data
 * input:
 *     cmd_buf      : current cmdlst buf
 * return:
 *      0;
 ***********************************************************/
int cmdlst_set_buffer_padding(struct cmd_buf_t *cmd_buf)
{
	unsigned int aligned_data_size = 0;
	unsigned int i = 0;
	/* Padding two words for D64 */
	*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) = CMDLST_PADDING_DATA;
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
	*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) = CMDLST_PADDING_DATA;
	aligned_data_size = align_up(cmd_buf->data_size, 128);

	for (i = cmd_buf->data_size; i < aligned_data_size; i += 4) {
		*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) =
			CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
		*(unsigned int *)(uintptr_t)(cmd_buf->data_addr) =
			CMDLST_PADDING_DATA;
	}

	cmd_buf->data_size = aligned_data_size;
	return ISP_IPP_OK;
}

static int cmdlst_set_branch(unsigned int channel_id)
{
	struct cmdlst_state_t st;
	st.ch_id = channel_id;
	loge_if_ret(g_cmdlst_devs[0].ops->set_branch(&g_cmdlst_devs[0],
				st.ch_id));
	udelay(1);
	return ISP_IPP_OK;
}

static int convert_mode_to_channel_id(enum cmdlst_eof_mode_e mode, unsigned int *channel_id)
{
	switch (mode) {
	case CMD_EOF_ARFEATURE_MODE:
		*channel_id = IPP_ARFEATURE_CHANNEL_ID;
		break;

	case CMD_EOF_RDR_MODE:
		*channel_id = IPP_RDR_CHANNEL_ID;
		break;

	case CMD_EOF_CMP_MODE:
		*channel_id = IPP_CMP_CHANNEL_ID;
		break;

	case CMD_EOF_GF_MODE:
		*channel_id = IPP_GF_CHANNEL_ID;
		break;

	case CMD_EOF_HIOF_MODE:
		*channel_id = IPP_HIOF_CHANNEL_ID;
		break;

	case CMD_EOF_ORB_ENH_MODE:
		*channel_id = IPP_ORB_ENH_CHANNEL_ID;
		break;

	case CMD_EOF_MC_MODE:
		*channel_id = IPP_MC_CHANNEL_ID;
		break;

	default:
		loge("Failed : Invilid mode = %d", mode);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

/**********************************************************
 * function name: ipp_eop_handler
 *
 * description:
 *              eop handler to dequeue a done-frame
 * input:
 *     NULL
 * return:
 *      0;
 ***********************************************************/
int ipp_eop_handler(enum cmdlst_eof_mode_e mode)
{
	unsigned int channel_id = IPP_CHANNEL_ID_MAX_NUM;
	struct list_head *ready_list = NULL;
	convert_mode_to_channel_id(mode, &channel_id);
	ready_list = &g_cmdlst_priv.cmdlst_chan[channel_id].ready_list;

	while (!list_empty(ready_list))
		list_del(ready_list->next);

	if (channel_id == IPP_GF_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_GF);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_GF);
		cpe_mem_free(MEM_ID_CMDLST_PARA_GF);
	} else if (channel_id == IPP_ARFEATURE_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_ARF);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_ARF);
		cpe_mem_free(MEM_ID_CMDLST_PARA_ARF);
	} else if (channel_id == IPP_RDR_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_REORDER);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_REORDER);
		cpe_mem_free(MEM_ID_CMDLST_PARA_REORDER);
	} else if (channel_id == IPP_CMP_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_COMPARE);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_COMPARE);
		cpe_mem_free(MEM_ID_CMDLST_PARA_COMPARE);
	} else if (channel_id == IPP_HIOF_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_HIOF);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_HIOF);
		cpe_mem_free(MEM_ID_CMDLST_PARA_HIOF);
	} else if (channel_id == IPP_ORB_ENH_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_ORB_ENH);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_ORB_ENH);
		cpe_mem_free(MEM_ID_CMDLST_PARA_ORB_ENH);
	} else if (channel_id == IPP_MC_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_MC);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_MC);
		cpe_mem_free(MEM_ID_CMDLST_PARA_MC);
	} else {
		loge("Failed : Invilid channel id = %d", channel_id);
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cmdlst_enqueue
 *
 * description:
 *              a new frame to enqueue
 * input:
 *     last_exec:last exec stripe's start_addr
 *     cmd_buf:current cmdlst buf
 *     prio:priority of new frame
 *     stripe_cnt :total stripe of new frame
 *     frame_type:new frame type
 * return:
 *      0;
 ***********************************************************/
static int cmdlst_enqueue(unsigned int channel_id,
						  struct cmdlst_para_t *cmdlst_para)
{
	struct schedule_cmdlst_link_t *pos = NULL;
	struct schedule_cmdlst_link_t *n = NULL;
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	struct list_head *dump_list = NULL;
#endif
	struct list_head *ready_list = NULL;
	ready_list = &g_cmdlst_priv.cmdlst_chan[channel_id].ready_list;
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)

	for (dump_list = ready_list->next;
		 dump_list != ready_list; dump_list = dump_list->next) {
		if ((void *)dump_list == NULL)
			break;
	}

#endif
	list_for_each_entry_safe(pos, n, ready_list, list_link) {
		if (pos->list_link.next != ready_list)
			cmdlst_update_buffer_header(pos, n, channel_id);
		else
			cmdlst_update_buffer_header_last(pos, n, channel_id);
	}
	return ISP_IPP_OK;
}

/**********************************************************
 * function name: cmdlst_start
 *
 * description:
 *              start cmdlst when branch,or the first frame.
 * input:
 *     last_exec:last exec stripe's start_addr
 *     cmdlst_cfg: cmdlst config table
 * return:
 *      0;
 ***********************************************************/
static int cmdlst_start(struct cfg_tab_cmdlst_t *cmdlst_cfg, unsigned int channel_id)
{
	struct list_head *cmdlst_insert_queue = NULL;
	cmdlst_insert_queue =
		&g_cmdlst_priv.cmdlst_chan[channel_id].ready_list;

	if (cmdlst_cfg != NULL)
		loge_if_ret(cmdlst_do_config(&g_cmdlst_devs[0], cmdlst_cfg));

	return ISP_IPP_OK;
}

int df_sched_set_buffer_header(struct cmdlst_para_t *cmdlst_para)
{
	unsigned int i = 0;
	unsigned int ret;
	struct schedule_cmdlst_link_t *cmd_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		ret = cmdlst_set_buffer_header(i, &cmd_entry[i].cmd_buf, cmdlst_para);
		if (ret != 0) {
			loge("Failed : cmdlst_set_buffer_header");
			return ISP_IPP_ERR;
		}
	}

	return ISP_IPP_OK;
}

int cmdlst_priv_prepare(void)
{
	unsigned int i;

	for (i = 0; i < CMDLST_CHANNEL_MAX; i++)
		INIT_LIST_HEAD(&g_cmdlst_priv.cmdlst_chan[i].ready_list);

	return ISP_IPP_OK;
}

static int get_new_entry_and_cmdlst_buf_addr(struct schedule_cmdlst_link_t **new_entry, unsigned int channel_id,
		unsigned long long *va, unsigned int *da)
{
	if (channel_id == IPP_GF_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_GF, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_GF, va, da));
	} else if (channel_id == IPP_ARFEATURE_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_ARF, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_ARF, va, da));
	} else if (channel_id == IPP_RDR_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_REORDER, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_REORDER, va, da));
	} else if (channel_id == IPP_CMP_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_COMPARE, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_COMPARE, va, da));
	} else if (channel_id == IPP_HIOF_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_HIOF, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_HIOF, va, da));
	} else if (channel_id == IPP_ORB_ENH_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_ORB_ENH, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_ORB_ENH, va, da));
	} else if (channel_id == IPP_MC_CHANNEL_ID) {
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_ENTRY_MC, va, da));
		*new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)(*va);
		loge_if_ret(cpe_mem_get(MEM_ID_CMDLST_BUF_MC, va, da));
	}

	if (*new_entry == NULL) {
		loge("Failed : to memory_alloc new entry!");
		return ISP_IPP_ERR;
	}

	return ISP_IPP_OK;
}

int df_sched_prepare(struct cmdlst_para_t *cmdlst_para)
{
	unsigned int i;
	unsigned long long va = 0;
	unsigned int da = 0;
	unsigned int channel_id = cmdlst_para->channel_id;
	struct schedule_cmdlst_link_t *new_entry = NULL;

	if (!list_empty((&g_cmdlst_priv.cmdlst_chan[channel_id].ready_list))) {
		loge("Failed : ready list not clean out");
		return ISP_IPP_ERR;
	}

	loge_if_ret(get_new_entry_and_cmdlst_buf_addr(&new_entry, channel_id, &va, &da));

	loge_if(memset_s(new_entry, cmdlst_para->stripe_cnt * sizeof(struct schedule_cmdlst_link_t),
					 0, cmdlst_para->stripe_cnt * sizeof(struct schedule_cmdlst_link_t)));

	cmdlst_para->cmd_entry = (void *)new_entry;

	new_entry[0].cmd_buf.start_addr = va;

	new_entry[0].cmd_buf.start_addr_isp_map = da;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		new_entry[i].stripe_cnt = cmdlst_para->stripe_cnt;
		new_entry[i].stripe_index = i;
		new_entry[i].data = (void *)cmdlst_para;
		list_add_tail(&new_entry[i].list_link, &g_cmdlst_priv.cmdlst_chan[channel_id].ready_list);
		new_entry[i].cmd_buf.start_addr = new_entry[0].cmd_buf.start_addr +
										  (unsigned long long)(CMDLST_BUFFER_SIZE * (unsigned long long)i);
		new_entry[i].cmd_buf.start_addr_isp_map =
			new_entry[0].cmd_buf.start_addr_isp_map + CMDLST_BUFFER_SIZE * i;

		if (new_entry[i].cmd_buf.start_addr == 0) {
			loge("Failed : fail to get cmdlist buffer!");
			return ISP_IPP_ERR;
		}

		new_entry[i].cmd_buf.buffer_size = CMDLST_BUFFER_SIZE;
		new_entry[i].cmd_buf.header_size = CMDLST_HEADER_SIZE;
		new_entry[i].cmd_buf.data_addr = new_entry[i].cmd_buf.start_addr;
		logd("buffer_size=%d", new_entry[i].cmd_buf.buffer_size);
		new_entry[i].cmd_buf.data_size = 0;
		new_entry[i].cmd_buf.next_buffer = NULL;
		loge_if_ret(cmdlst_set_buffer_header(i, &new_entry[i].cmd_buf, cmdlst_para));
	}

	return ISP_IPP_OK;
}

#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
static void cmdlst_dump_queue(struct cmdlst_para_t *cmdlst_para)
{
	unsigned int channel_id = cmdlst_para->channel_id;
	struct schedule_cmdlst_link_t *cmdlst_temp_link = NULL;
	struct schedule_cmdlst_link_t *n = NULL;
	struct list_head *cmdlst_insert_queue = &g_cmdlst_priv.cmdlst_chan[channel_id].ready_list;
	list_for_each_entry_safe(cmdlst_temp_link, n, cmdlst_insert_queue, list_link) {
		logi("dump cmdlst queue:");
		logi("stripe_cnt = %d, stripe_index = %d, channel_id = %d",
			 cmdlst_temp_link->stripe_cnt,
			 cmdlst_temp_link->stripe_index, channel_id);
		logi("cmd_buf: start_addr = 0x%llx, start_addr_isp_map = 0x%08x, data_size = 0x%x",
			 cmdlst_temp_link->cmd_buf.start_addr,
			 cmdlst_temp_link->cmd_buf.start_addr_isp_map,
			 cmdlst_temp_link->cmd_buf.data_size);
		logi("en_link = %d, ch_link = %d, ch_link_act_nbr = %d, irq_mode = 0x%x",
			 cmdlst_para->cmd_stripe_info[cmdlst_temp_link->stripe_index].en_link,
			 cmdlst_para->cmd_stripe_info[cmdlst_temp_link->stripe_index].ch_link,
			 cmdlst_para->cmd_stripe_info[cmdlst_temp_link->stripe_index].ch_link_act_nbr,
			 cmdlst_para->cmd_stripe_info[cmdlst_temp_link->stripe_index].irq_mode);
	}
}
#endif

int df_sched_start(struct cmdlst_para_t *cmdlst_para)
{
	unsigned int channel_id = cmdlst_para->channel_id;
	struct schedule_cmdlst_link_t *cmd_link_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	loge_if_ret(ipp_update_cmdlst_cfg_tab(cmdlst_para));
	loge_if_ret(cmdlst_set_branch(channel_id));
	loge_if_ret(cmdlst_enqueue(channel_id, cmdlst_para));
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	cmdlst_dump_queue(cmdlst_para);
#endif
#if defined(IPP_UT_DEBUG) && (IPP_UT_DEBUG == 1)
	ut_dump_register(cmdlst_para->stripe_cnt);
#endif
	loge_if_ret(cmdlst_start(&(cmd_link_entry[0].cmdlst_cfg_tab), channel_id));
#if defined(FLAG_LOG_DEBUG) && (FLAG_LOG_DEBUG == 1)
	list_for_each_entry(cmd_link_entry, &g_cmdlst_priv.cmdlst_chan
						[cmdlst_para->channel_id].ready_list, list_link) {
		logi(" cmd_link_entry->stripe_index = %d", cmd_link_entry->stripe_index);
		cmdlst_buff_dump(&cmd_link_entry->cmd_buf);
	}
#endif
	frame_num++;
	return ISP_IPP_OK;
}


void dump_addr(unsigned long long addr, unsigned int start_addr_isp_map, int num, char *info)
{
#define DATA_PERLINE    16
	int i = 0;
	logd("%s Dump ......, num=%d", info, num);

	for (i = 0; i < num; i += DATA_PERLINE)
		logi("0x%llx(0x%08x): 0x%08x 0x%08x 0x%08x 0x%08x",
			 addr + i,
			 start_addr_isp_map + i,
			 readl((void __iomem *)(uintptr_t)(addr + i + 0x00)),
			 readl((void __iomem *)(uintptr_t)(addr + i + 0x04)),
			 readl((void __iomem *)(uintptr_t)(addr + i + 0x08)),
			 readl((void __iomem *)(uintptr_t)(addr + i + 0x0C)));
}

void cmdlst_buff_dump(struct cmd_buf_t *cmd_buf)
{
	logi("CMDLST BUFF: Size:0x%x, Addr(VA): 0x%llx, start_addr_isp_map(DA):0x%llx",
		 cmd_buf->data_size, cmd_buf->start_addr, (unsigned long long)(cmd_buf->start_addr_isp_map));
	dump_addr(cmd_buf->start_addr, cmd_buf->start_addr_isp_map, cmd_buf->data_size, "cmdlst_buff");
}

void cmdlst_set_reg_by_cpu(unsigned int reg, unsigned int val)
{
	unsigned int temp_reg;
	temp_reg = ((reg) & 0x000fffff);

	if (temp_reg >= 0x4000 && temp_reg < 0x5000)
		hispcpe_reg_set(CPE_TOP, ((temp_reg) & 0x00000fff), val);
	else if (temp_reg >= 0x5000 && temp_reg < 0x6000)
		hispcpe_reg_set(CMDLIST_REG, ((temp_reg) & 0x00000fff), val);
	else if (temp_reg >= 0x6000 && temp_reg < 0x8000)
		hispcpe_reg_set(CVDR_REG, ((temp_reg) & 0x00000fff), val);
}

void df_size_dump_stripe_info(struct ipp_stripe_info_t *p_stripe_info, char *s)
{
	unsigned int i = 0;
	char type_name[LOG_NAME_LEN];

	if (p_stripe_info->stripe_cnt == 0)
		return;

	loge_if(memset_s((void *)type_name, LOG_NAME_LEN, 0, LOG_NAME_LEN));
	logd("%s stripe_cnt = %d", type_name, p_stripe_info->stripe_cnt);

	for (i = 0; i < p_stripe_info->stripe_cnt; i++) {
		logd("%s: stripe_width[%d] = %u",
			 type_name, i, p_stripe_info->stripe_width[i]);
		logd("start_point[%d] = %d", i,
			 p_stripe_info->stripe_start_point[i]);
		logd("end_point[%d] = %d", i,
			 p_stripe_info->stripe_end_point[i]);
		logd("overlap_left[%d] = %d", i,
			 p_stripe_info->overlap_left[i]);
		logd("overlap_right[%d] = %d", i,
			 p_stripe_info->overlap_right[i]);
	}
}

static unsigned int df_size_calc_stripe_width(unsigned int active_stripe_width,
		unsigned int input_align,
		unsigned int overlap)
{
	unsigned int tmp_active_stripe_width = 0;
	unsigned int stripe_width = 0;
	if (active_stripe_width == UNSIGNED_INT_MAX || input_align == 0)
		return UNSIGNED_INT_MAX;

	tmp_active_stripe_width = active_stripe_width / (1 << 16);

	if (active_stripe_width % (1 << 16))
		tmp_active_stripe_width++;

	stripe_width = tmp_active_stripe_width + overlap * 2;
	stripe_width = (stripe_width / input_align) * input_align;

	if (stripe_width % input_align)
		stripe_width += input_align;

	return stripe_width;
}

static void df_size_split_stripe_part0(unsigned int constrain_cnt, struct df_size_constrain_t *p_size_constrain,
									   split_stripe_tmp_t *tmp,  unsigned int max_stripe_width)
{
	unsigned int i = 0;
	unsigned int tmp_frame_width = 0;

	for (i = 0; i < constrain_cnt; i++) {
		unsigned int max_out_width = 0;
		unsigned int tmp_input_width = 0;
		if (p_size_constrain[i].out_width == UNSIGNED_INT_MAX) continue;

		max_out_width = p_size_constrain[i].pix_align * (p_size_constrain[i].out_width /
									 p_size_constrain[i].pix_align);

		if (p_size_constrain[i].out_width != max_out_width)
			p_size_constrain[i].out_width = max_out_width;

		tmp_input_width = max_out_width * p_size_constrain[i].hinc;

		if (tmp_input_width < tmp->active_stripe_width) tmp->active_stripe_width = tmp_input_width;

		if (p_size_constrain[i].hinc * p_size_constrain[i].pix_align > tmp->max_in_stripe_align) {
			tmp->max_in_stripe_align = p_size_constrain[i].hinc * p_size_constrain[i].pix_align;
			tmp->max_frame_width =
				p_size_constrain[i].hinc * ((unsigned int)(tmp->in_full_width << 16) / p_size_constrain[i].hinc);
		}
	}

	// aligning boundAR on the pixel limit
	if (tmp->active_stripe_width != UNSIGNED_INT_MAX && tmp->active_stripe_width % (1 << 16))
		tmp->active_stripe_width = ((tmp->active_stripe_width >> 16) + 1) << 16;

	// dn_ar is the max ar of coarsest scaling device
	if (tmp->max_in_stripe_align == 0) {
		loge("Failed : max_in_stripe_align is zero");
		return;
	}

	tmp_frame_width = tmp->max_in_stripe_align *
								   (tmp->active_stripe_width / tmp->max_in_stripe_align);

	if (tmp_frame_width == 0) {
		loge("Failed : tmp_frame_width:error");
		return;
	}

	if ((tmp->in_full_width <= max_stripe_width)
		&& (tmp->max_frame_width <= tmp_frame_width)) {
		tmp->stripe_cnt = 1;
	} else if ((tmp->in_full_width <= 2 * (max_stripe_width - tmp->overlap))
			   && (tmp->max_frame_width <= 2 * tmp_frame_width)) {
		tmp->stripe_cnt = 2;
	} else {
		tmp->stripe_width =
			df_size_calc_stripe_width(tmp->active_stripe_width, tmp->input_align, tmp->overlap);

		if (tmp->stripe_width > max_stripe_width) {
			tmp->stripe_width = max_stripe_width;
			tmp->active_stripe_width = (tmp->stripe_width - tmp->overlap * 2) << 16;
		}

		if (tmp->max_in_stripe_align > tmp->active_stripe_width)
			loge("Failed :The most downscaler wrong");

		tmp->active_stripe_width = (tmp->active_stripe_width / tmp->max_in_stripe_align)
								   * tmp->max_in_stripe_align;
		tmp->stripe_cnt = tmp->max_frame_width / tmp->active_stripe_width;

		if (tmp->max_frame_width % tmp->active_stripe_width) tmp->stripe_cnt++;
	}

	return;
}

static void get_stripe_start_stripe_end(split_stripe_tmp_t *tmp, struct df_size_constrain_t *p_size_constrain,
										unsigned int constrain_cnt, unsigned int i)
{
	unsigned int j = 0;

	for (j = 0; j != constrain_cnt; j++) {
		unsigned int active_pix_align = p_size_constrain[j].pix_align * p_size_constrain[j].hinc;
		unsigned int tmp_stripe_start = (i == 0) ? 0 : tmp->last_stripe_end;
		unsigned int tmp_stripe_end = 0;

		if (tmp_stripe_start < tmp->stripe_start)
			tmp->stripe_start = tmp_stripe_start;

		if (i == tmp->stripe_cnt - 1)
			tmp_stripe_end = tmp->active_stripe_end;
		else
			tmp_stripe_end = (tmp->active_stripe_end / active_pix_align) * active_pix_align;

		if (tmp->active_stripe_end - tmp_stripe_end > active_pix_align / 2)
			tmp_stripe_end += active_pix_align;

		if (tmp_stripe_end > tmp->stripe_end)
			tmp->stripe_end = tmp_stripe_end;
	}

	return;
}

static void df_size_split_stripe_part1(unsigned int constrain_cnt, struct df_size_constrain_t *p_size_constrain,
									   struct ipp_stripe_info_t *p_stripe_info, unsigned int width,
									   split_stripe_tmp_t *tmp, unsigned int max_stripe_width)
{
	unsigned int is_again_calc;
	unsigned int i;

	do {
		unsigned int tmp_last_end = 0;
		is_again_calc = 0;
		tmp->last_stripe_end = 0;

		for (i = 0; i < tmp->stripe_cnt; i++) {
			unsigned int start_point;
			unsigned int end_point;
			tmp->active_stripe_end = ((i + 1) * ((unsigned long long int)(tmp->in_full_width) << 16)) / tmp->stripe_cnt;

			if (i != tmp->stripe_cnt - 1) {
				tmp->active_stripe_end = (tmp->active_stripe_end / tmp->max_in_stripe_align) * tmp->max_in_stripe_align;

				if (tmp->active_stripe_end <= tmp_last_end) loge("Failed :The most downscaler wrong");

				tmp_last_end = tmp->active_stripe_end;
			}

			tmp->stripe_end = 0;
			tmp->stripe_start = UNSIGNED_INT_MAX;
			get_stripe_start_stripe_end(tmp, p_size_constrain, constrain_cnt, i);
			tmp->last_stripe_end = tmp->stripe_end;
			p_stripe_info->stripe_start_point[i] = tmp->stripe_start;
			p_stripe_info->stripe_end_point[i] = tmp->stripe_end;
			p_stripe_info->overlap_left[i] = (i == 0) ? 0 : tmp->overlap;
			p_stripe_info->overlap_right[i] = (i == tmp->stripe_cnt - 1) ? 0 : tmp->overlap;
			p_stripe_info->stripe_cnt = tmp->stripe_cnt;
			start_point = p_stripe_info->stripe_start_point[i] - (p_stripe_info->overlap_left[i] << 16);
			p_stripe_info->stripe_start_point[i] = (start_point / (tmp->input_align << 16)) * tmp->input_align;
			p_stripe_info->overlap_left[i] = (i == 0) ? 0 : (p_stripe_info->stripe_end_point[i - 1] -
											 p_stripe_info->overlap_right[i - 1] - p_stripe_info->stripe_start_point[i]);
			end_point = p_stripe_info->stripe_end_point[i] + (p_stripe_info->overlap_right[i] << 16);
			p_stripe_info->stripe_end_point[i] = (end_point / (tmp->input_align << 16)) * tmp->input_align;

			if (end_point % (tmp->input_align << 16))
				p_stripe_info->stripe_end_point[i] += tmp->input_align;

			if ((tmp->stripe_cnt - 1) == i)
				p_stripe_info->stripe_end_point[i] = width;

			p_stripe_info->overlap_right[i] = (i == tmp->stripe_cnt - 1) ? 0 :
											  (p_stripe_info->stripe_end_point[i] -
											   align_up(p_stripe_info->stripe_end_point[i] - tmp->overlap, CVDR_ALIGN_BYTES / 2));
			p_stripe_info->stripe_width[i] = p_stripe_info->stripe_end_point[i] - p_stripe_info->stripe_start_point[i];

			if (p_stripe_info->stripe_width[i] > max_stripe_width) {
				tmp->stripe_cnt++;
				is_again_calc = 1;
				break;
			}
		}
	} while (is_again_calc);

	return;
}

void df_size_split_stripe(unsigned int constrain_cnt,
						  struct df_size_constrain_t *p_size_constrain,
						  struct ipp_stripe_info_t *p_stripe_info,
						  unsigned int overlap, unsigned int width,
						  unsigned int max_stripe_width)
{
	split_stripe_tmp_t tmp = {0};
	tmp.overlap = overlap;
	tmp.input_align = 16;
	tmp.in_full_width = width;
	tmp.active_stripe_width = UNSIGNED_INT_MAX;
	df_size_split_stripe_part0(constrain_cnt, p_size_constrain, &tmp, max_stripe_width);
	df_size_split_stripe_part1(constrain_cnt, p_size_constrain, p_stripe_info, width, &tmp, max_stripe_width);
	return;
}

int seg_ipp_set_cmdlst_wr_buf(struct cmd_buf_t *cmd_buf,
							  struct cmdlst_wr_cfg_t *wr_cfg)
{
	struct cfg_tab_cvdr_t cmdlst_wr_table;
	unsigned int i;
	unsigned int tmp_data_size;
	unsigned int burst_work_mode; // 0: add incr 1:addr no incr
	unsigned int line_shart_wstrb;
	unsigned int total_frame_end = wr_cfg->buff_wr_addr + wr_cfg->data_size * 4 - 1;
	// mc index update read_mode is 1 and +1 for update 0xEEC04040
	tmp_data_size = wr_cfg->data_size + wr_cfg->data_size / CMDLST_BURST_MAX_SIZE + 1;
	tmp_data_size = (wr_cfg->read_mode == 1) ? (tmp_data_size) : wr_cfg->data_size;

	logd("wr_addr = 0x%x, wr_cfg->read_mode = %d, wr_cfg->is_incr = %d",
		wr_cfg->buff_wr_addr, wr_cfg->read_mode, wr_cfg->is_incr);
	burst_work_mode = (wr_cfg->is_incr == 1) ? 0 : 1;
	total_frame_end = wr_cfg->buff_wr_addr + tmp_data_size * 4 - 1;
	// v300 contain 1 reg len, v350 not contain this.
	line_shart_wstrb = CVDR_ALIGN_BYTES - (CVDR_ALIGN_BYTES / 4 - tmp_data_size % 4) * 4 - 1;
	line_shart_wstrb = (wr_cfg->is_wstrb == 1) ? line_shart_wstrb : 0xF;
	wr_cfg->buff_wr_addr = align_down(wr_cfg->buff_wr_addr, CVDR_ALIGN_BYTES);
	logd("set cmdlst wr, wr_addr = 0x%x, data_size = 0x%x, line_shart_wstrb = %d",
		wr_cfg->buff_wr_addr, wr_cfg->data_size, line_shart_wstrb);

	loge_if(memset_s(&cmdlst_wr_table, sizeof(struct cfg_tab_cvdr_t), 0, sizeof(struct cfg_tab_cvdr_t)));
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].to_use = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].id = IPP_VP_WR_CMDLST;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.fs_addr = wr_cfg->buff_wr_addr;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_fmt = DF_D32;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_expan = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.last_page = total_frame_end >> 15;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_stride = 0;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_shart_wstrb = line_shart_wstrb;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_wrap = DEFAULT_LINE_WRAP;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter0 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter1 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter2 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter3 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter_reload = 0xF;
	cvdr_prepare_cmd(&g_cvdr_devs[0], cmd_buf, &cmdlst_wr_table);

	for (i = 0; i < (wr_cfg->data_size / CMDLST_BURST_MAX_SIZE + 1); i++) {
		unsigned int reg_addr_offset;
		unsigned int reg_size = ((wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE) >
								CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE
								: (wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE);

		if (wr_cfg->read_mode == 1) {
			// reg top 0xEEC04040 0xff009082  0x9080 is mc index reg, 2 is addr not increase
			cmdlst_set_reg(cmd_buf, IPP_BASE_ADDR_TOP + 0x40,
						   0x00009082 | ((reg_size - 1) << 24));
			cmdlst_set_reg_incr(cmd_buf, IPP_BASE_ADDR_TOP + 0x40, 1, 0, 1);
		}

		reg_addr_offset = (burst_work_mode == 1) ? 0 : (4 * i * CMDLST_BURST_MAX_SIZE);
		logd("(i=%d),wr_cfg->reg_rd_addr + reg_addr_offset=0x%08x",  i, wr_cfg->reg_rd_addr + reg_addr_offset);
		cmdlst_set_reg_incr(cmd_buf, wr_cfg->reg_rd_addr + reg_addr_offset, reg_size,
							burst_work_mode, 1);
	}

	return ISP_IPP_OK;
}

int seg_ipp_set_cmdlst_wr_buf_cmp(struct cmd_buf_t *cmd_buf,
								  struct cmdlst_wr_cfg_t *wr_cfg, unsigned int match_points_offset)
{
	struct cfg_tab_cvdr_t cmdlst_wr_table;
	unsigned int burst_num;
	unsigned int i;
	unsigned int tmp_data_size;
	unsigned int line_shart_wstrb;
	unsigned int total_frame_end = wr_cfg->buff_wr_addr + wr_cfg->data_size * 4  + 4 - 1; // +matched_points bytes
	tmp_data_size = wr_cfg->data_size + wr_cfg->data_size /
					CMDLST_BURST_MAX_SIZE + 1;
	tmp_data_size = (wr_cfg->read_mode == 1) ? (tmp_data_size) : wr_cfg->data_size;
	line_shart_wstrb = CVDR_ALIGN_BYTES -
									(CVDR_ALIGN_BYTES / 4 - 1 - tmp_data_size % 4) * 4 - 1;
	line_shart_wstrb = (wr_cfg->is_wstrb == 1) ? line_shart_wstrb : 0xFF;
	wr_cfg->buff_wr_addr = align_down(wr_cfg->buff_wr_addr, CVDR_ALIGN_BYTES);
	logd("set cmdlst wr, wr_addr = 0x%x, data_size = 0x%x", wr_cfg->buff_wr_addr, wr_cfg->data_size);
	logd("line_shart_wstrb = %d", line_shart_wstrb);
	loge_if(memset_s(&cmdlst_wr_table, sizeof(struct cfg_tab_cvdr_t), 0, sizeof(struct cfg_tab_cvdr_t)));
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].to_use = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].id = IPP_VP_WR_CMDLST;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.fs_addr = wr_cfg->buff_wr_addr;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_fmt = DF_D32;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_expan = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.last_page = total_frame_end >> 15;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_stride = 0;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_shart_wstrb =
		line_shart_wstrb;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_wrap = DEFAULT_LINE_WRAP;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter0 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter1 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter2 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter3 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter_reload = 0xF;
	cvdr_prepare_cmd(&g_cvdr_devs[0], cmd_buf, &cmdlst_wr_table);
	// below cmd read matched points 0x4f0 reg.
	cmdlst_set_reg_incr(cmd_buf, wr_cfg->reg_rd_addr - match_points_offset, 1, 0, 1);
	burst_num = (wr_cfg->data_size % CMDLST_BURST_MAX_SIZE == 0) ?
				(wr_cfg->data_size / CMDLST_BURST_MAX_SIZE) : (wr_cfg->data_size / CMDLST_BURST_MAX_SIZE + 1);

	for (i = 0; i < burst_num; i++) {
		unsigned int reg_addr_offset;
		unsigned int burst_work_mode = (wr_cfg->is_incr == 1) ? 0 : 1; // 0: add incr 1:addr no incr
		unsigned int reg_size = ((wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE)
								> CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE
								: (wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE);
		reg_addr_offset = (burst_work_mode == 1) ? 0 : (4 * i * CMDLST_BURST_MAX_SIZE);
		cmdlst_set_reg_incr(cmd_buf, wr_cfg->reg_rd_addr + reg_addr_offset, reg_size,
							burst_work_mode, 1);
	}

	return ISP_IPP_OK;
}

static int ipp_update_cmdlst_cfg_tab(struct cmdlst_para_t *cmdlst_para)
{
	unsigned int i = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;
	cmd_link_entry =
		(struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		struct cfg_tab_cmdlst_t *p_cfgtab = NULL;
		cmdlst_set_buffer_padding(&cmd_link_entry[i].cmd_buf);
		p_cfgtab = &cmd_link_entry[i].cmdlst_cfg_tab;
		p_cfgtab->cfg.to_use = 1;
		p_cfgtab->cfg.slowdown_nrt_channel = 0;
		p_cfgtab->ch_cfg.to_use = 1;
		p_cfgtab->ch_cfg.nrt_channel = 1;
		p_cfgtab->ch_cfg.active_token_nbr_en = 0;
		p_cfgtab->ch_cfg.active_token_nbr = 0;
		p_cfgtab->sw_ch_mngr_cfg.to_use = 1;
		p_cfgtab->sw_ch_mngr_cfg.sw_resource = cmdlst_para->cmd_stripe_info[i].resource_share;
		p_cfgtab->sw_ch_mngr_cfg.sw_priority = cmdlst_para->cmd_stripe_info[i].hw_priority;
		p_cfgtab->vp_rd_cfg.to_use = 1;
		p_cfgtab->vp_rd_cfg.vp_rd_id = cmdlst_para->channel_id;
		p_cfgtab->vp_rd_cfg.rd_addr = cmd_link_entry[i].cmd_buf.start_addr_isp_map;
		p_cfgtab->vp_rd_cfg.rd_size = cmd_link_entry[i].cmd_buf.data_size;
	}

	return ISP_IPP_OK;
}

// This part is used to print DIO during the UT test.
#if IPP_UT_DEBUG
typedef struct _reg_val_t {
	unsigned int reg_val;
	unsigned char flag;
} reg_val_t;

reg_val_t reg_val[MAX_DUMP_STRIPE_CNT][IPP_MAX_REG_OFFSET / 4 + 1] = {0};
unsigned int ut_module_addr = 0;
unsigned int ut_register_num = 0;
unsigned int ut_reg_addr_incr = 0;
unsigned int ut_reg_incr_num = 0;
unsigned int ut_reg_count = 0;
unsigned int ut_input_format = 0;

void set_dump_register_init(unsigned int addr, unsigned int offset, unsigned int input_fmt)
{
	ut_module_addr = addr;
	ut_register_num = 1 + offset / 4;
	ut_input_format = input_fmt;
	loge_if(memset_s(reg_val, sizeof(reg_val_t)*MAX_DUMP_STRIPE_CNT * ut_register_num, 0,
			 sizeof(reg_val_t)*MAX_DUMP_STRIPE_CNT * ut_register_num));
}

void get_register_info(unsigned int reg, unsigned int val)
{
	if (((reg & ut_module_addr) == ut_module_addr)
		&& ((reg - ut_module_addr) <= (ut_register_num * 4))) {
		for (int i = 0; i < MAX_DUMP_STRIPE_CNT; i++) {
			if (reg_val[i][(reg - ut_module_addr) / 4].flag == 0) {
				reg_val[i][(reg - ut_module_addr) / 4].reg_val = val;
				reg_val[i][(reg - ut_module_addr) / 4].flag = 1;
				break;
			}
		}
	}
}

void get_register_incr_info(unsigned int reg, unsigned int reg_num)
{
	if ((reg & ut_module_addr) == ut_module_addr) {
		ut_reg_addr_incr = reg;
		ut_reg_incr_num = reg_num;
		ut_reg_count = 0;
	}
}

void get_register_incr_data(unsigned int data)
{
	unsigned int index;
	index = ((ut_reg_addr_incr - ut_module_addr) / 4) + ut_reg_count;

	if (((ut_reg_addr_incr & ut_module_addr) == ut_module_addr)
		&& (ut_reg_count < ut_reg_incr_num)
		&& (index < (IPP_MAX_REG_OFFSET / 4 + 1))) {
		for (int i = 0; i < MAX_DUMP_STRIPE_CNT; i++) {
			if (reg_val[i][index].flag == 0) {
				reg_val[i][index].reg_val = data;
				reg_val[i][index].flag = 1;
				break;
			}
		}

		ut_reg_count++;
	}
}

void ut_dump_register(unsigned int stripe_cnt)
{
	logi("----------------start_dump_dio----------------");
	pr_err("START_NUM                          0\n");
	pr_err("END_NUM                            %d\n", stripe_cnt);

	for (int j = 0; j < stripe_cnt; j++) {
		pr_err("SUB_FRAME_INDEX                    %d\n", j);

		for (int i = 0; i < ut_register_num; i++)
			pr_err("1    %06X    %08X\n", i * 4, reg_val[j][i].reg_val);

		pr_err("1    %06X    %08X\n", 0xFFFFFE, 1);
	}

	memset_s(reg_val, sizeof(reg_val_t)*MAX_DUMP_STRIPE_CNT * ut_register_num, 0,
			 sizeof(reg_val_t)*MAX_DUMP_STRIPE_CNT * ut_register_num);
	logi("----------------end_dump_dio----------------");
}
#endif

