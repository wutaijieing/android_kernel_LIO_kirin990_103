

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#include <linux/types.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include "adapter_common.h"
#include "memory.h"
#include "cmdlst_drv.h"
#include "cmdlst_reg_offset.h"
#include "cvdr_drv.h"
#include "ipp_top_reg_offset.h"

#define SCALER_RATIO          4
#define UNSIGNED_INT_MAX      0xffffffff

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

void cmdlst_set_addr_align(struct cmd_buf_t *cmd_buf,
	unsigned int size, unsigned int align)
{
	unsigned int i = 0;

	if (align == 0) {
		E("align is equal to zero: %d", align);
		return;
	}

	unsigned int padding_index =
	    (align - ((cmd_buf->data_addr + (size + 2) * 4) % align)) / 4;

	padding_index = (padding_index == 4) ? 0 : padding_index;

	if ((cmd_buf->data_size + padding_index * 4) > cmd_buf->buffer_size) {
		E("cmdlst buffer is full: %u, padding_index = %d",
		  cmd_buf->data_size, padding_index);
		return;
	}

	for (i = 0; i < padding_index; i++) {
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) = CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}
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
		E("cmdlst buffer is full: %u, @0x%x", cmd_buf->data_size, reg);
		return ISP_IPP_ERR;
	}

	if ((cmd_buf->data_size + 8) <= cmd_buf->buffer_size) {
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) =
		    ((reg) & 0x000fffff);
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) = (val);
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	} else {
		E("cmdlst buffer is full: %u, @0x%x", cmd_buf->data_size, reg);
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
	cur_point->cmd_buf.next_buffer =
		(void *)&next_point->cmd_buf;
	unsigned int next_hw_prio =
		next_point->cmdlst_cfg_tab.sw_ch_mngr_cfg.sw_priority;
	unsigned int next_hw_resource =
		next_point->cmdlst_cfg_tab.sw_ch_mngr_cfg.sw_ressource_request;
	unsigned int next_hw_token_nbr =
		next_point->cmdlst_cfg_tab.sw_ch_mngr_cfg.sw_link_token_nbr;
	unsigned int next_hw_link_channel =
		next_point->cmdlst_cfg_tab.sw_ch_mngr_cfg.sw_link_channel;
	struct cmd_buf_t *cmd_buf = &cur_point->cmd_buf;
	struct cmd_buf_t *next_buf = cmd_buf->next_buffer;

	if (next_buf == NULL) {
		unsigned int i = 0;

		for (i = 0; i < 10; i++)
		*(unsigned int *)(uintptr_t)(cmd_buf->start_addr
				+ 0x4 * i) = CMDLST_PADDING_DATA; //lint !e647

		return;
	}

	/* ignore 16 bytes of padding data at the beginning */
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x0) =
	    0x000050BC + 0x80 * channel_id;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x4) =
	    (next_hw_prio << 31) | (next_hw_resource << 8) |
	    (next_hw_token_nbr << 4) | (next_hw_link_channel);
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x8) =
	    0x000050D0 + 0x80 * channel_id;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0xC) =
	    next_buf->start_addr_isp_map >> 2;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x10) =
	    0x000050C4 + 0x80 * channel_id;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x14) =
	    ((next_buf->start_addr_isp_map + next_buf->data_size)
	     & 0xffffe000) | 0x0000013E;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x18) =
	    0x000050C8 + 0x80 * channel_id;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x1C) =
	    (next_buf->data_size / 8) - 1;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x20) =
	    0x000050CC + 0x80 * channel_id;
	*(unsigned int *)(uintptr_t) (cmd_buf->start_addr + 0x24) = 0;
}

void cmdlst_update_buffer_header_last(
	struct schedule_cmdlst_link_t *cur_point,
	struct schedule_cmdlst_link_t *next_point,
	unsigned int channel_id)
{
	struct cmd_buf_t *cmd_buf = &cur_point->cmd_buf;
	unsigned int i  = 0;

	for (i = 0; i < 10; i++)
		*(unsigned int *)(uintptr_t)(cmd_buf->start_addr
			+ 0x4 * i) = CMDLST_PADDING_DATA;	//lint !e647
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
		E("cmdlst buffer is full: %u, @0x%x", cmd_buf->data_size, reg);
		return;
	}
#ifdef UT_DEBUG
	get_register_incr_info(reg, size);
#endif
	*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) =
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
	*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) = data;
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
}

void cmdlst_set_addr_offset(struct cmd_buf_t *cmd_buf, unsigned int size_offset)
{
	cmd_buf->data_addr += 4 * size_offset;
	cmd_buf->data_size += 4 * size_offset;
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
	D("[%s]+\n", __func__);
	if (cmd_buf->data_size > CMDLST_BUFFER_SIZE) {
		E("cmdlst datasize is overflow");
		return ISP_IPP_ERR;
	}

	unsigned int idx;
	unsigned int ret;

	/* reserve 10 words to update next buffer later */
	for (idx = 0; idx < 10; idx++) {
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) =
		    CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}

	ret = cmdlst_set_irq_mode(cmdlst_para, stripe_index);
	if (ret != 0) {
		E("Failed : cmdlst_set_irq_mode");
		return ISP_IPP_ERR;
	}

	while (cmd_buf->data_size < CMDLST_HEADER_SIZE) {
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) =
		    CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
	}

	D("[%s]-\n", __func__);
	return ISP_IPP_OK;
}

int cmdlst_read_buffer(unsigned int stripe_index,
	struct cmd_buf_t *cmd_buf,
	struct cmdlst_para_t *cmdlst_para)
{
	D("[%s]+\n", __func__);
	unsigned int idx;
	struct cmdlst_rd_cfg_info_t *rd_cfg_info =
	    &cmdlst_para->cmd_stripe_info[stripe_index].rd_cfg_info;

	D("stripe_cnt =%d, fs =0x%x", cmdlst_para->stripe_cnt, rd_cfg_info->fs);

	if (rd_cfg_info->fs != 0) {
		cmdlst_set_vpwr(cmd_buf, rd_cfg_info);

		for (idx = 0; idx < rd_cfg_info->rd_cfg_num; idx++) {
			*(unsigned int *)(uintptr_t) (cmd_buf->data_addr)
			    = rd_cfg_info->rd_cfg[idx];
			cmd_buf->data_addr += 4;
			cmd_buf->data_size += 4;
		}
	}

	*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) = CMDLST_PADDING_DATA;
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
	D("[%s]-\n", __func__);
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
static int cmdlst_set_irq_mode(struct cmdlst_para_t *cmdlst_para,
	unsigned int stripe_index)
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

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_ORB)) {
		/* clear all orb irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_ORB_IRQ_REG0_REG, 0xFFFF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_ORB_IRQ_REG1_REG, (0x0 << 16) |
			       (0xFFFF & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x0);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_RDR)) {
		/* clear all rdr irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_RDR_IRQ_REG0_REG, 0x1F);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_RDR_IRQ_REG1_REG, (0x0 << 16) |
			       (0x1F & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x1);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_CMP)) {
		/* clear all cmp irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_CMP_IRQ_REG0_REG, 0x1F);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMP_IRQ_REG1_REG, (0x0 << 16)
			       | (0x1F & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x2);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_GF)) {
		/* clear all gf irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_GF_IRQ_REG0_REG, 0x1FFF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_GF_IRQ_REG1_REG,
			       (0x0 << 16) | (0x1FFF & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x3);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_VBK)) {
		/* clear all vbk irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_VBK_IRQ_REG0_REG, 0xFFFFFF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_VBK_IRQ_REG1_REG,
			       (0xFFFFFF & ~irq_mode));
		cmdlst_set_reg(cmd_buf,
			       JPG_TOP_OFFSET + IPP_TOP_VBK_IRQ_REG2_REG, 0x0);
		cmdlst_set_reg(cmd_buf,
			       JPG_TOP_OFFSET + IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x4);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_ORB_ENH)) {
		/* clear all orb_enh irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_ENH_IRQ_REG0_REG, 0x3FF);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_ENH_IRQ_REG1_REG, (0x0 << 16)
			       | (0x3FF & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x6);
	}

	if (ipp_resource & (1 << IPP_CMD_RES_SHARE_MC)) {
		/* clear all cmp irq */
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET
			       + IPP_TOP_MC_IRQ_REG0_REG, 0x7);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_MC_IRQ_REG1_REG, (0x7 & ~irq_mode));
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_MC_IRQ_REG2_REG, 0x0);
		cmdlst_set_reg(cmd_buf, JPG_TOP_OFFSET +
			       IPP_TOP_CMDLST_CTRL_MAP_0_REG +
			       0x10 * channel_id, 0x7);
	}

	D("ipp cmdlst stripe_index = %d, ch_id = %d", stripe_index, channel_id);
	D("cpu_enable = %d, irq_mode = %d", cpu_enable, irq_mode);
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
		E("cmdlst buf is null!");
		return ISP_IPP_ERR;
	}

	if (rd_cfg_info == NULL) {
		E("cmdlst read cfg info is null!");
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
		cmd_buf,&cmdlst_w3_1_table));
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
	/* Padding two words for D64 */
	*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) = CMDLST_PADDING_DATA;
	cmd_buf->data_addr += 4;
	cmd_buf->data_size += 4;
	*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) = CMDLST_PADDING_DATA;
	unsigned int aligned_data_size;

	aligned_data_size = ALIGN_UP(cmd_buf->data_size, 128);
	unsigned int i = 0;

	for (i = cmd_buf->data_size; i < aligned_data_size; i += 4) {
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) =
		    CMDLST_PADDING_DATA;
		cmd_buf->data_addr += 4;
		cmd_buf->data_size += 4;
		*(unsigned int *)(uintptr_t) (cmd_buf->data_addr) =
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
	udelay(1);		/*lint !e778 !e774 !e747 */

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

	switch (mode) {
	case CMD_EOF_ANF_MODE:	/* fall-through */
		channel_id = IPP_ANF_CHANNEL_ID;
		break;
	case CMD_EOF_ORB_MODE:
		channel_id = IPP_ORB_CHANNEL_ID;
		break;
	case CMD_EOF_RDR_MODE:
		channel_id = IPP_RDR_CHANNEL_ID;
		break;
	case CMD_EOF_CMP_MODE:
		channel_id = IPP_CMP_CHANNEL_ID;
		break;
	case CMD_EOF_GF_MODE:
		channel_id = IPP_GF_CHANNEL_ID;
		break;
	case CMD_EOF_VBK_MODE:
		channel_id = IPP_VBK_CHANNEL_ID;
		break;
	case CMD_EOF_ORB_ENH_MODE:
		channel_id = IPP_ORB_ENH_CHANNEL_ID;
		break;
	case CMD_EOF_MC_MODE:
		channel_id = IPP_MC_CHANNEL_ID;
		break;
	default:
		E(" Invilid mode = %d\n", mode);
		return ISP_IPP_ERR;
	}

	struct list_head *ready_list = NULL;

	ready_list = &g_cmdlst_priv.cmdlst_chan[channel_id].ready_list;

	while (!list_empty(ready_list)) {
		list_del(ready_list->next);
	}

	 if (channel_id == IPP_GF_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_GF);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_GF);
		cpe_mem_free(MEM_ID_CMDLST_PARA_GF);
	} else if (channel_id == IPP_ORB_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_ORB);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_ORB);
		cpe_mem_free(MEM_ID_CMDLST_PARA_ORB);
	} else if (channel_id == IPP_RDR_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_REORDER);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_REORDER);
		cpe_mem_free(MEM_ID_CMDLST_PARA_REORDER);
	} else if (channel_id == IPP_CMP_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_COMPARE);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_COMPARE);
		cpe_mem_free(MEM_ID_CMDLST_PARA_COMPARE);
	} else if (channel_id == IPP_VBK_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_VBK);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_VBK);
		cpe_mem_free(MEM_ID_CMDLST_PARA_VBK);
	} else if (channel_id == IPP_ORB_ENH_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_ORB_ENH);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_ORB_ENH);
		cpe_mem_free(MEM_ID_CMDLST_PARA_ORB_ENH);
	} else if (channel_id == IPP_MC_CHANNEL_ID) {
		cpe_mem_free(MEM_ID_CMDLST_BUF_MC);
		cpe_mem_free(MEM_ID_CMDLST_ENTRY_MC);
		cpe_mem_free(MEM_ID_CMDLST_PARA_MC);
	} else {
		E(" Invilid channel id = %d\n", channel_id);
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
	struct list_head *ready_list = NULL;

	ready_list = &g_cmdlst_priv.cmdlst_chan[channel_id].ready_list;
#if FLAG_LOG_DEBUG
	struct list_head *dump_list = NULL;
	for (dump_list = ready_list->next;
	     dump_list != ready_list; dump_list = dump_list->next) {
		if ((void *)dump_list == NULL)
			break;
	}
#endif
	struct schedule_cmdlst_link_t *pos = NULL;
	struct schedule_cmdlst_link_t *n = NULL;

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
		cmdlst_do_config(&g_cmdlst_devs[0], cmdlst_cfg);

	return ISP_IPP_OK;
}

void cmdlst_priv_prepare(void)
{
        unsigned int i;

	 for (i = 0; i < CMDLST_CHANNEL_MAX; i++)	
	       INIT_LIST_HEAD(&g_cmdlst_priv.cmdlst_chan[i].ready_list);
}


int df_sched_prepare(struct cmdlst_para_t *cmdlst_para)
{	
	if (cmdlst_para->stripe_cnt > CMDLST_STRIPE_MAX_NUM) {
		E("stripe cnt is overflow");
		return ISP_IPP_ERR;
	}

	unsigned int i;
	unsigned long long va = 0;
	unsigned int da = 0;
	int ret = 0;
	unsigned int channel_id = cmdlst_para->channel_id;

	if (!list_empty((&g_cmdlst_priv.cmdlst_chan[channel_id].ready_list))) {
		E("ready list not clean out");
		return ISP_IPP_ERR;
	}

	struct schedule_cmdlst_link_t *new_entry = NULL;

	if (channel_id == IPP_GF_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_GF, &va, &da);
		if (ret != 0) {
			E("Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_GF);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t*)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_GF, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_GF);
			return ISP_IPP_ERR;
		}
	} else if (channel_id == IPP_ORB_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_ORB, &va, &da);
		if (ret != 0) {
			E("Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_ORB);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_ORB, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_ORB);
			return ISP_IPP_ERR;
		}
	} else if (channel_id == IPP_RDR_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_REORDER, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_REORDER);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_REORDER, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_REORDER);
			return ISP_IPP_ERR;
		}
	} else if (channel_id == IPP_CMP_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_COMPARE, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_COMPARE);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_COMPARE, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_COMPARE);
			return ISP_IPP_ERR;
		}
	} else if (channel_id == IPP_VBK_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_VBK, &va, &da);
		if (ret != 0) {
			E("Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_VBK);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_VBK, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_VBK);
			return ISP_IPP_ERR;
		}
	} else if (channel_id == IPP_ORB_ENH_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_ORB_ENH, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_ORB_ENH);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_ORB_ENH, &va, &da);
		if (ret != 0) {
			E("Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_ORB_ENH);
			return ISP_IPP_ERR;
		}
	} else if (channel_id == IPP_MC_CHANNEL_ID) {
		ret = cpe_mem_get(MEM_ID_CMDLST_ENTRY_MC, &va, &da);
		if (ret != 0) {
			E("Failed :mem_get %d\n", MEM_ID_CMDLST_ENTRY_MC);
			return ISP_IPP_ERR;
		}

		new_entry = (struct schedule_cmdlst_link_t *)(uintptr_t)va;
		ret = cpe_mem_get(MEM_ID_CMDLST_BUF_MC, &va, &da);
		if (ret != 0) {
			E(" Failed :mem_get %d\n", MEM_ID_CMDLST_BUF_MC);
			return ISP_IPP_ERR;
		}
	}

	if (new_entry == NULL) {
		E("fail to memory_alloc new entry!");
		return ISP_IPP_ERR;
	}

	loge_if(memset_s(new_entry,
			 cmdlst_para->stripe_cnt *
			 sizeof(struct schedule_cmdlst_link_t), 0,
			 cmdlst_para->stripe_cnt *
			 sizeof(struct schedule_cmdlst_link_t)));
	cmdlst_para->cmd_entry = (void *)new_entry;
	new_entry[0].cmd_buf.start_addr = va;
	new_entry[0].cmd_buf.start_addr_isp_map = da;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		new_entry[i].stripe_cnt = cmdlst_para->stripe_cnt;
		new_entry[i].stripe_index = i;
		new_entry[i].data = (void *)cmdlst_para;
		list_add_tail(&new_entry[i].list_link,
			&g_cmdlst_priv.cmdlst_chan[channel_id].ready_list);
		new_entry[i].cmd_buf.start_addr =
			new_entry[0].cmd_buf.start_addr +
				(unsigned long long)(CMDLST_BUFFER_SIZE *
					(unsigned long long)i);
		new_entry[i].cmd_buf.start_addr_isp_map =
			new_entry[0].cmd_buf.start_addr_isp_map +
				CMDLST_BUFFER_SIZE * i;

		if (new_entry[i].cmd_buf.start_addr == 0) {
			E("fail to get cmdlist buffer!");
			return ISP_IPP_ERR;
		}

		new_entry[i].cmd_buf.buffer_size = CMDLST_BUFFER_SIZE;
		new_entry[i].cmd_buf.header_size = CMDLST_HEADER_SIZE;
		new_entry[i].cmd_buf.data_addr =
		    new_entry[i].cmd_buf.start_addr;
		D("buffer_size=%d",new_entry[i].cmd_buf.buffer_size);
		new_entry[i].cmd_buf.data_size = 0;
		new_entry[i].cmd_buf.next_buffer = NULL;
		loge_if_ret(cmdlst_set_buffer_header(i,
						     &new_entry[i].cmd_buf,
						     cmdlst_para));
	}

	return ISP_IPP_OK;
}

int df_sched_start(struct cmdlst_para_t *cmdlst_para)
{
	unsigned int channel_id = cmdlst_para->channel_id;
	struct schedule_cmdlst_link_t *cmd_link_entry =
	    (struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;
	loge_if_ret(ipp_update_cmdlst_cfg_tab(cmdlst_para));
	loge_if_ret(cmdlst_set_branch(channel_id));
	loge_if_ret(cmdlst_enqueue(channel_id, cmdlst_para));

	loge_if_ret(cmdlst_start(&(cmd_link_entry[0].cmdlst_cfg_tab),
				 channel_id));
	list_for_each_entry(cmd_link_entry,
			    &g_cmdlst_priv.cmdlst_chan
			    [cmdlst_para->channel_id].ready_list, list_link) {
		cmdlst_buff_dump(&cmd_link_entry->cmd_buf);
	}
	frame_num++;
	return ISP_IPP_OK;
}

void dump_addr(unsigned long long addr, int num, char *info)
{
#define DATA_PERLINE    16
	int i;

	D("%s Dump ......\n", info);

	for (i = 0; i < num; i += DATA_PERLINE)
		pr_info("0x%llx : 0x%08x 0x%08x 0x%08x 0x%08x\n",
			addr + i,
			readl((void __iomem *)(uintptr_t) (addr + i + 0x00)),
			readl((void __iomem *)(uintptr_t) (addr + i + 0x04)),
			readl((void __iomem *)(uintptr_t) (addr + i + 0x08)),
			readl((void __iomem *)(uintptr_t) (addr + i + 0x0C)));
}

void cmdlst_buff_dump(struct cmd_buf_t *cmd_buf)
{
#if FLAG_LOG_DEBUG
	I("CMDLST BUFF: Size:0x%x   Addr: 0x%llx\n",
	  cmd_buf->data_size, cmd_buf->start_addr);
	dump_addr(cmd_buf->start_addr, cmd_buf->data_size, "cmdlst_buff");
#endif
}

void df_size_dump_stripe_info(struct ipp_stripe_info_t *p_stripe_info, char *s)
{
	unsigned int i = 0;
	char type_name[LOG_NAME_LEN];

	if (p_stripe_info->stripe_cnt == 0)
		return;

	loge_if(memset_s((void *)type_name, LOG_NAME_LEN, 0, LOG_NAME_LEN));
	D("%s stripe_cnt = %d\n", type_name, p_stripe_info->stripe_cnt);

	for (i = 0; i < p_stripe_info->stripe_cnt; i++) {
		D("%s: stripe_width[%d] = %u",
		  type_name, i, p_stripe_info->stripe_width[i]);
		D("start_point[%d] = %d", i,
		  p_stripe_info->stripe_start_point[i]);
		D("end_point[%d] = %d", i,
			p_stripe_info->stripe_end_point[i]);
		D("overlap_left[%d] = %d", i,
			p_stripe_info->overlap_left[i]);
		D("overlap_right[%d] = %d", i,
			p_stripe_info->overlap_right[i]);
	}

}

static unsigned int df_size_calc_stripe_width(unsigned int active_stripe_width,
	unsigned int input_align,
	unsigned int overlap)
{
	if (active_stripe_width == UNSIGNED_INT_MAX || input_align == 0)
		return UNSIGNED_INT_MAX;

	unsigned int tmp_active_stripe_width = active_stripe_width / (1 << 16);

	if (active_stripe_width % (1 << 16))
		tmp_active_stripe_width++;

	unsigned int stripe_width = tmp_active_stripe_width + overlap * 2;

	stripe_width = (stripe_width / input_align) * input_align;

	if (stripe_width % input_align)
		stripe_width += input_align;

	return stripe_width;
}

void df_size_split_stripe(unsigned int constrain_cnt,
	struct df_size_constrain_t *p_size_constrain,
	struct ipp_stripe_info_t *p_stripe_info,
	unsigned int overlap, unsigned int width,
	unsigned int max_stripe_width)
{
	const unsigned int input_align = 16;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int in_full_width = width;
	unsigned int active_stripe_width = UNSIGNED_INT_MAX;
	unsigned int max_in_stripe_align = 0;
	unsigned int max_frame_width = 0;
	unsigned int stripe_width = 0;

	for (i = 0; i < constrain_cnt; i++) {
		if (p_size_constrain[i].out_width == UNSIGNED_INT_MAX)
			continue;

		unsigned int max_out_width = p_size_constrain[i].pix_align *
		    (p_size_constrain[i].out_width /
		     p_size_constrain[i].pix_align);

		if (p_size_constrain[i].out_width != max_out_width)
			p_size_constrain[i].out_width = max_out_width;

		unsigned int tmp_input_width = max_out_width *
		    p_size_constrain[i].hinc;

		if (tmp_input_width < active_stripe_width)
			active_stripe_width = tmp_input_width;

		if (p_size_constrain[i].hinc * p_size_constrain[i].pix_align >
		    max_in_stripe_align) {
			max_in_stripe_align = p_size_constrain[i].hinc *
			    p_size_constrain[i].pix_align;
			max_frame_width = p_size_constrain[i].hinc *
			    ((unsigned int)(in_full_width << 16) /
			     p_size_constrain[i].hinc);
		}
	}

	// aligning boundAR on the pixel limit
	if (active_stripe_width != UNSIGNED_INT_MAX &&
	    active_stripe_width % (1 << 16))
		active_stripe_width = ((active_stripe_width >> 16) + 1) << 16;

	// dn_ar is the max ar of coarsest scaling device
	if (max_in_stripe_align == 0) {
		pr_info("max_in_stripe_align is zero\n");
		return;
	}

	unsigned int tmp_frame_width = max_in_stripe_align *
		(active_stripe_width / max_in_stripe_align);	//lint !e414
	unsigned int stripe_cnt = 0;

	if (tmp_frame_width == 0) {
		pr_info("tmp_frame_width:error\n");
		return;
	}

	if ((in_full_width <= max_stripe_width)
		&& (max_frame_width <= tmp_frame_width)) {
		stripe_cnt = 1;
	} else if ((in_full_width <= 2 * (max_stripe_width - overlap))
		&& (max_frame_width <= 2 * tmp_frame_width)) {
		stripe_cnt = 2;
	} else {
		stripe_width =
		    df_size_calc_stripe_width(active_stripe_width, input_align,
					      overlap);

		if (stripe_width > max_stripe_width) {
			stripe_width = max_stripe_width;
			active_stripe_width =
			    (stripe_width - overlap * 2) << 16;
		}

		if (max_in_stripe_align > active_stripe_width)
			pr_err("The most downscaler wrong\n");

		active_stripe_width =
			(active_stripe_width / max_in_stripe_align)
			* max_in_stripe_align;	//lint !e414
		stripe_cnt = max_frame_width / active_stripe_width;//lint !e414

		if (max_frame_width % active_stripe_width)	//lint !e414
			stripe_cnt++;
	}

	unsigned int is_again_calc = 0;

	do {
		is_again_calc = 0;
		unsigned int tmp_last_end = 0;
		unsigned int last_stripe_end = 0;

		for (i = 0; i < stripe_cnt; i++) {
			unsigned long long int active_stripe_end =
				((i + 1) * ((unsigned long long int)
				(in_full_width) << 16)) / stripe_cnt;

			if (i != stripe_cnt - 1) {
				active_stripe_end = (active_stripe_end
						     / max_in_stripe_align)
				    * max_in_stripe_align;	//lint !e414

				if (active_stripe_end <= tmp_last_end)
					pr_err("The most downscaler wrong\n");
				tmp_last_end = active_stripe_end;
			}

			unsigned int stripe_end = 0;
			unsigned int stripe_start = UNSIGNED_INT_MAX;

			for (j = 0; j != constrain_cnt; j++) {
				unsigned int active_pix_align =
				    p_size_constrain[j].pix_align
				    * p_size_constrain[j].hinc;
				unsigned int tmp_stripe_start =
				    (i == 0) ? 0 : last_stripe_end;
				unsigned int tmp_stripe_end = 0;

				if (tmp_stripe_start < stripe_start)
					stripe_start = tmp_stripe_start;

				if (i == stripe_cnt - 1)
					tmp_stripe_end = active_stripe_end;
				else
					tmp_stripe_end =
					    (active_stripe_end /
					     active_pix_align)
					    * active_pix_align;

				if (active_stripe_end - tmp_stripe_end
				    > active_pix_align / 2)
					tmp_stripe_end += active_pix_align;

				if (tmp_stripe_end > stripe_end)
					stripe_end = tmp_stripe_end;
			}

			last_stripe_end = stripe_end;
			p_stripe_info->stripe_start_point[i] = stripe_start;
			p_stripe_info->stripe_end_point[i] = stripe_end;
			p_stripe_info->overlap_left[i] =
				(i == 0) ? 0 : overlap;
			p_stripe_info->overlap_right[i] =
				(i == stripe_cnt - 1) ? 0 : overlap;
			p_stripe_info->stripe_cnt = stripe_cnt;
			unsigned int start_point =
			    p_stripe_info->stripe_start_point[i]
			    - (p_stripe_info->overlap_left[i] << 16);
			p_stripe_info->stripe_start_point[i] =
			    (start_point / (input_align << 16)) * input_align;
			p_stripe_info->overlap_left[i] = (i == 0) ? 0 :
			    (p_stripe_info->stripe_end_point[i - 1]
			     - p_stripe_info->overlap_right[i - 1]
			     - p_stripe_info->stripe_start_point[i]);

			unsigned int end_point =
			    p_stripe_info->stripe_end_point[i]
			    + (p_stripe_info->overlap_right[i] << 16);
			p_stripe_info->stripe_end_point[i] =
			    (end_point / (input_align << 16)) * input_align;

			if (end_point % (input_align << 16))
				p_stripe_info->stripe_end_point[i] +=
				    input_align;

			if ((stripe_cnt - 1) == i)
				p_stripe_info->stripe_end_point[i] = width;

			p_stripe_info->overlap_right[i] =
			    (i == stripe_cnt - 1) ? 0 :
			    (p_stripe_info->stripe_end_point[i]
			     - ALIGN_UP(p_stripe_info->stripe_end_point[i]
					- overlap, CVDR_ALIGN_BYTES / 2));
			p_stripe_info->stripe_width[i] =
			    p_stripe_info->stripe_end_point[i]
			    - p_stripe_info->stripe_start_point[i];

			if (p_stripe_info->stripe_width[i]
				> max_stripe_width) {
				stripe_cnt++;
				is_again_calc = 1;
				break;
			}
		}
	} while (is_again_calc);
}

int seg_ipp_set_cmdlst_wr_buf(struct cmd_buf_t *cmd_buf,
	struct cmdlst_wr_cfg_t *wr_cfg)
{	
	if (wr_cfg->data_size > CMDLST_BUFFER_SIZE) {
		E("cmdlst datasize is overflow");
		return ISP_IPP_ERR;
	}

	unsigned int tmp_data_size;
	unsigned int burst_work_mode;	// 0: add incr 1:addr no incr

	tmp_data_size = wr_cfg->data_size + wr_cfg->data_size /
	    CMDLST_BURST_MAX_SIZE + 1;
	tmp_data_size = (wr_cfg->read_mode == 1) ?
	    (tmp_data_size) : wr_cfg->data_size;

	burst_work_mode = (wr_cfg->is_incr == 1) ? 0 : 1;
	unsigned int line_shart_wstrb = CVDR_ALIGN_BYTES -
	    (CVDR_ALIGN_BYTES / 4 - 1 - tmp_data_size % 4) * 4 - 1;
	line_shart_wstrb = (wr_cfg->is_wstrb == 1) ? line_shart_wstrb : 0xFF;
	wr_cfg->buff_wr_addr = HIPP_ALIGN_DOWN(wr_cfg->buff_wr_addr,
					  CVDR_ALIGN_BYTES);
	D("set cmdlst wr, wr_addr = 0x%x, data_size = 0x%x",
	  wr_cfg->buff_wr_addr, wr_cfg->data_size);

	D("line_shart_wstrb = %d", line_shart_wstrb);
	struct cfg_tab_cvdr_t cmdlst_wr_table;
	unsigned int reg_addr_offset = 0;

	loge_if(memset_s(&cmdlst_wr_table, sizeof(struct cfg_tab_cvdr_t),
			 0, sizeof(struct cfg_tab_cvdr_t)));
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].to_use = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].id = IPP_VP_WR_CMDLST;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.fs_addr =
	    wr_cfg->buff_wr_addr;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_fmt = DF_D32;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_expan = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.last_page =
	    (wr_cfg->buff_wr_addr + wr_cfg->data_size) >> 15;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_stride = 0;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_shart_wstrb =
	    line_shart_wstrb;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_wrap =
	    DEFAULT_LINE_WRAP;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter0 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter1 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter2 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter3 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter_reload = 0xF;
	cvdr_prepare_cmd(&g_cvdr_devs[0], cmd_buf, &cmdlst_wr_table);
	unsigned int i = 0;
	unsigned int reg_size = 0;
	unsigned int temp_reg_addr = 0;
	unsigned int temp_val = 0;

	for (i = 0; i < (wr_cfg->data_size / CMDLST_BURST_MAX_SIZE + 1); i++) {
		reg_size = ((wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE) >
			    CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE
		    : (wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE);

		if (wr_cfg->read_mode == 1) {
			temp_reg_addr = IPP_BASE_ADDR_TOP + 0x40;
			temp_val = 0x00011082 | ((reg_size - 1) << 24);
			cmdlst_set_reg(cmd_buf, temp_reg_addr, temp_val);
			cmdlst_set_reg_incr(cmd_buf, temp_reg_addr, 1, 0, 1);
		}

		reg_addr_offset = (burst_work_mode == 1) ?
		    0 : (4 * i * CMDLST_BURST_MAX_SIZE);
		cmdlst_set_reg_incr(cmd_buf, wr_cfg->reg_rd_addr
				    + reg_addr_offset, reg_size,
				    burst_work_mode, 1);
	}

	return ISP_IPP_OK;
}

int seg_ipp_set_cmdlst_wr_buf_cmp(struct cmd_buf_t* cmd_buf,
                                  struct cmdlst_wr_cfg_t *wr_cfg,unsigned int match_points_offset)
{
	unsigned int tmp_data_size;
	unsigned int burst_work_mode;	// 0: add incr 1:addr no incr

	tmp_data_size = wr_cfg->data_size + wr_cfg->data_size /
	    CMDLST_BURST_MAX_SIZE + 1;
	tmp_data_size = (wr_cfg->read_mode == 1) ?
	    (tmp_data_size) : wr_cfg->data_size;
	burst_work_mode = (wr_cfg->is_incr == 1) ? 0 : 1;
	unsigned int line_shart_wstrb = CVDR_ALIGN_BYTES -
	    (CVDR_ALIGN_BYTES / 4 - 1 - tmp_data_size % 4) * 4 - 1;
	line_shart_wstrb = (wr_cfg->is_wstrb == 1) ? line_shart_wstrb : 0xFF;
	wr_cfg->buff_wr_addr =
		HIPP_ALIGN_DOWN(wr_cfg->buff_wr_addr, CVDR_ALIGN_BYTES);
	I("set cmdlst wr, wr_addr = 0x%x, data_size = 0x%x",
	  wr_cfg->buff_wr_addr, wr_cfg->data_size);
	I("line_shart_wstrb = %d", line_shart_wstrb);

	struct cfg_tab_cvdr_t cmdlst_wr_table;
	unsigned int reg_addr_offset = 0;

	loge_if(memset_s(&cmdlst_wr_table, sizeof(struct cfg_tab_cvdr_t),
			 0, sizeof(struct cfg_tab_cvdr_t)));
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].to_use = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].id = IPP_VP_WR_CMDLST;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.fs_addr =
	    wr_cfg->buff_wr_addr;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_fmt = DF_D32;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.pix_expan = 1;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.last_page =
	    (wr_cfg->buff_wr_addr + wr_cfg->data_size) >> 15;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_stride = 0;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_shart_wstrb =
	    line_shart_wstrb;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].fmt.line_wrap =
	    DEFAULT_LINE_WRAP;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter0 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter1 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter2 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter3 = 0xF;
	cmdlst_wr_table.vp_wr_cfg[IPP_VP_WR_CMDLST].bw.bw_limiter_reload = 0xF;
	cvdr_prepare_cmd(&g_cvdr_devs[0], cmd_buf, &cmdlst_wr_table);
	unsigned int i = 0;
	unsigned int reg_size = 0;
	unsigned int match_points_reg_addr = wr_cfg->reg_rd_addr - match_points_offset;
	unsigned int burst_num = 0;

	cmdlst_set_reg_incr(cmd_buf, match_points_reg_addr, 1, 0, 1);

	burst_num = (wr_cfg->data_size % CMDLST_BURST_MAX_SIZE == 0)? 
		(wr_cfg->data_size / CMDLST_BURST_MAX_SIZE) :
		(wr_cfg->data_size / CMDLST_BURST_MAX_SIZE + 1);

	for (i = 0; i < burst_num; i++) {
		reg_size = ((wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE)
			    > CMDLST_BURST_MAX_SIZE) ? CMDLST_BURST_MAX_SIZE
		    : (wr_cfg->data_size - i * CMDLST_BURST_MAX_SIZE);
		reg_addr_offset = (burst_work_mode == 1) ?
		    0 : (4 * i * CMDLST_BURST_MAX_SIZE);
		cmdlst_set_reg_incr(cmd_buf, wr_cfg->reg_rd_addr
				    + reg_addr_offset, reg_size,
				    burst_work_mode, 1);
	}

	return ISP_IPP_OK;
}

static int ipp_update_cmdlst_cfg_tab(struct cmdlst_para_t *cmdlst_para)
{	
	if (cmdlst_para->stripe_cnt > CMDLST_STRIPE_MAX_NUM) {
		E("stripe cnt is overflow");
		return ISP_IPP_ERR;
	}

	unsigned int i = 0;
	struct schedule_cmdlst_link_t *cmd_link_entry = NULL;

	cmd_link_entry =
	    (struct schedule_cmdlst_link_t *)cmdlst_para->cmd_entry;

	for (i = 0; i < cmdlst_para->stripe_cnt; i++) {
		cmdlst_set_buffer_padding(&cmd_link_entry[i].cmd_buf);
		struct cfg_tab_cmdlst_t *p_cfgtab = &cmd_link_entry[i].cmdlst_cfg_tab;

		p_cfgtab->cfg.to_use = 1;
		p_cfgtab->cfg.prefetch = 0;
		p_cfgtab->cfg.slowdown_nrt_channel = 0;
		p_cfgtab->ch_cfg.to_use = 1;
		p_cfgtab->ch_cfg.nrt_channel = 1;
		p_cfgtab->ch_cfg.active_token_nbr_en = 0;
		p_cfgtab->ch_cfg.active_token_nbr = 0;
		p_cfgtab->sw_ch_mngr_cfg.to_use = 1;
		p_cfgtab->sw_ch_mngr_cfg.sw_link_channel =
			cmdlst_para->cmd_stripe_info[i].ch_link;
		p_cfgtab->sw_ch_mngr_cfg.sw_link_token_nbr =
			cmdlst_para->cmd_stripe_info[i].ch_link_act_nbr;
		p_cfgtab->sw_ch_mngr_cfg.sw_ressource_request =
			cmdlst_para->cmd_stripe_info[i].resource_share;
		p_cfgtab->sw_ch_mngr_cfg.sw_priority =
			cmdlst_para->cmd_stripe_info[i].hw_priority;
		p_cfgtab->vp_rd_cfg.to_use = 1;
		p_cfgtab->vp_rd_cfg.vp_rd_id = cmdlst_para->channel_id;
		p_cfgtab->vp_rd_cfg.rd_addr =
			cmd_link_entry[i].cmd_buf.start_addr_isp_map;
		p_cfgtab->vp_rd_cfg.rd_size =
			cmd_link_entry[i].cmd_buf.data_size;
	}

	return ISP_IPP_OK;
}

#pragma GCC diagnostic pop
