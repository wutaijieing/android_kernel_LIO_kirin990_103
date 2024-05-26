/* Copyright (c) 2013-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/spinlock.h>

#include "hisi_mipi_dsi.h"
#include "soc_dte_define.h"

/* global definition for the cmd queue which will be send after next vactive start */
static struct dsi_delayed_cmd_queue g_delayed_cmd_queue;
static bool g_delayed_cmd_queue_inited = false;
static bool g_mipi_trans_lock_inited = false;
static spinlock_t g_mipi_trans_lock;

static void mipi_dsi_sread_request(struct dsi_cmd_desc *cm, char __iomem *dsi_base);

static inline void mipi_dsi_cmd_send_lock(void)
{
	if (g_delayed_cmd_queue_inited)
		spin_lock(&g_delayed_cmd_queue.CmdSend_lock);
}

static inline void mipi_dsi_cmd_send_unlock(void)
{
	if (g_delayed_cmd_queue_inited)
		spin_unlock(&g_delayed_cmd_queue.CmdSend_lock);
}

void mipi_transfer_lock_init(void)
{
	disp_pr_err(" ++++ \n");
	if (!g_mipi_trans_lock_inited) {
		g_mipi_trans_lock_inited = true;
		spin_lock_init(&g_mipi_trans_lock);
	}
}
/*
 * mipi dsi short write with 0, 1 2 parameters
 * Write to GEN_HDR 24 bit register the value:
 * 1. 00h, MCS_command[15:8] ,VC[7:6],13h
 * 2. Data1[23:16], MCS_command[15:8] ,VC[7:6],23h
 */
static int mipi_dsi_swrite(struct dsi_cmd_desc *cm, char __iomem *dsi_base)
{
	uint32_t hdr = 0;
	int len;

	if (cm->dlen && cm->payload == 0) {
		disp_pr_err("NO payload error!\n");
		return 0;
	}

	if (cm->dlen > 2) { /* mipi dsi short write with 0, 1 2 parameters, total 3 param */
		disp_pr_err("cm->dlen is invalid");
		return -EINVAL;
	}
	len = cm->dlen;


	hdr |= DSI_HDR_DTYPE(cm->dtype);
	hdr |= DSI_HDR_VC(cm->vc);
	if (len == 1) {
		hdr |= DSI_HDR_DATA1((uint32_t)(cm->payload[0]));  /*lint !e571*/
		hdr |= DSI_HDR_DATA2(0);
	} else if (len == 2) {
		hdr |= DSI_HDR_DATA1((uint32_t)(cm->payload[0]));  /*lint !e571*/
		hdr |= DSI_HDR_DATA2((uint32_t)(cm->payload[1]));  /*lint !e571*/
	} else {
		hdr |= DSI_HDR_DATA1(0);
		hdr |= DSI_HDR_DATA2(0);
	}

	/* used for low power cmds trans under video mode */
	hdr |= cm->dtype & GEN_VID_LP_CMD;
	disp_pr_debug("dsi_base=0x%x!\n", dsi_base);
	dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), hdr, 25, 0);

	disp_pr_debug("hdr=0x%x!\n", hdr);
	return len;  /* 4 bytes */
}

/*
 * mipi dsi long write
 * Write to GEN_PLD_DATA 32 bit register the value:
 * Data3[31:24], Data2[23:16], Data1[15:8], MCS_command[7:0]
 * If need write again to GEN_PLD_DATA 32 bit register the value:
 * Data7[31:24], Data6[23:16], Data5[15:8], Data4[7:0]
 *
 * Write to GEN_HDR 24 bit register the value: WC[23:8] ,VC[7:6],29h
 */

static int mipi_dsi_lwrite(struct dsi_cmd_desc *cm, char __iomem *dsi_base)
{
	uint32_t hdr = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t pld = 0;

	if (cm->dlen && cm->payload == 0) {
		disp_pr_err("NO payload error!\n");
		return 0;
	}

	/* fill up payload, 4bytes set reg, remain 1 byte(8 bits) set reg */
	for (i = 0;  i < cm->dlen; i += 4) {
		if ((i + 4) <= cm->dlen) {
			pld = *((uint32_t *)(cm->payload + i));
		} else {
			for (j = i; j < cm->dlen; j++)
				pld |= (((uint32_t)cm->payload[j] & 0x0ff) << ((j - i) * 8));  /*lint !e571*/

			disp_pr_debug("pld=0x%x!\n", pld);
		}

		dpu_set_reg(DPU_DSI_APB_WR_LP_PLD_DATA_ADDR(dsi_base), pld, 32, 0);
		pld = 0;
	}

	/* fill up header */
	hdr |= DSI_HDR_DTYPE(cm->dtype);
	hdr |= DSI_HDR_VC(cm->vc);
	hdr |= DSI_HDR_WC(cm->dlen);

	/* used for low power cmds trans under video mode */
	hdr |= cm->dtype & GEN_VID_LP_CMD;
	disp_pr_debug("dsi_base=0x%x!\n", dsi_base);
	dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), hdr, 25, 0);

	disp_pr_debug("hdr=0x%x!\n", hdr);

	return cm->dlen;
}

static void mipi_dsi_max_return_packet_size(struct dsi_cmd_desc *cm, char __iomem *dsi_base)
{
	uint32_t hdr = 0;

	/* fill up header */
	hdr |= DSI_HDR_DTYPE(cm->dtype);
	hdr |= DSI_HDR_VC(cm->vc);
	hdr |= DSI_HDR_WC(cm->dlen);
	dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), hdr, 24, 0);
}

static uint32_t mipi_dsi_read(uint32_t *out, const char __iomem *dsi_base)
{
	uint32_t pkg_status;
	uint32_t try_times = 700;  /* 35ms(50*700) */

	do {
		pkg_status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
		if (!(pkg_status & 0x10))
			break;
		udelay(50);  /* 50us */
	} while (--try_times);

	*out = inp32(DPU_DSI_APB_WR_LP_PLD_DATA_ADDR(dsi_base));
	if (!try_times)
		disp_pr_err("CMD_PKT_STATUS[0x%x], PHY_STATUS[0x%x], INT_ST0[0x%x], INT_ST1[0x%x]\n",
			inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base)),
			inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base)),
			inp32(DPU_DSI_INT_ERROR_FORCE0_ADDR(dsi_base)),
			inp32(DPU_DSI_INT_ERROR_FORCE1_ADDR(dsi_base)));


	return try_times;
}

static void mipi_dsi_sread(uint32_t *out, const char __iomem *dsi_base)
{
	unsigned long dw_jiffies;
	uint32_t temp = 0;

	/*
	 * jiffies:Current total system clock ticks
	 * dw_jiffies:timeout
	 */
	dw_jiffies = jiffies + HZ / 2;  /* HZ / 2 = 0.5s */
	do {
		temp = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
		if ((temp & 0x00000040) == 0x00000040)
			break;
	} while (time_after(dw_jiffies, jiffies));

	dw_jiffies = jiffies + HZ / 2;  /* HZ / 2 = 0.5s */
	do {
		temp = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
		if ((temp & 0x00000040) != 0x00000040)
			break;
	} while (time_after(dw_jiffies, jiffies));

	*out = inp32(DPU_DSI_APB_WR_LP_PLD_DATA_ADDR(dsi_base));
}

static void mipi_dsi_lread(uint32_t *out, char __iomem *dsi_base)
{
	/* do something here */
}

static int mipi_dsi_cmd_is_read(struct dsi_cmd_desc *cm)
{
	int ret;

	disp_check_and_return(!cm, -1, err, "cmds is NULL!\n");

	switch (DSI_HDR_DTYPE(cm->dtype)) {
	case DTYPE_GEN_READ:
	case DTYPE_GEN_READ1:
	case DTYPE_GEN_READ2:
	case DTYPE_DCS_READ:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static int mipi_dsi_lread_reg(uint32_t *out, struct dsi_cmd_desc *cm, uint32_t len, char *dsi_base)
{
	int ret = 0;
	uint32_t i = 0;
	struct dsi_cmd_desc packet_size_cmd_set;

	disp_check_and_return(!cm, -1, err, "cmds is NULL!\n");

	disp_check_and_return(!dsi_base, -1, err, "dsi_base is NULL!\n");

	if (mipi_dsi_cmd_is_read(cm)) {
		packet_size_cmd_set.dtype = DTYPE_MAX_PKTSIZE;
		packet_size_cmd_set.vc = 0;
		packet_size_cmd_set.dlen = len;
		mipi_dsi_max_return_packet_size(&packet_size_cmd_set, dsi_base);
		mipi_dsi_sread_request(cm, dsi_base);
		for (i = 0; i < (len + 3) / 4; i++) {  /* read 4 bytes once */
			if (!mipi_dsi_read(out, dsi_base)) {
				ret = -1;
				disp_pr_err("Read register 0x%X timeout\n", cm->payload[0]);
				break;
			}
			out++;
		}
	} else {
		ret = -1;
		disp_pr_err("dtype=%x NOT supported!\n", cm->dtype);
	}

	return ret;
}

static void delay_for_next_cmd_by_sleep(uint32_t wait, uint32_t waittype)
{
	if (wait) {
		disp_pr_debug("waittype=%d, wait=%d\n", waittype, wait);
		if (waittype == WAIT_TYPE_US) {
			udelay(wait);
		} else if (waittype == WAIT_TYPE_MS) {
			if (wait <= 10)  /* less then 10ms, use mdelay() */
				mdelay((unsigned long)wait);
			else
				msleep(wait);
		} else {
			msleep(wait * 1000);  /* ms */
		}
	}
}

/*
 * prepare cmd buffer to be txed
 */
static int mipi_dsi_cmd_add(struct dsi_cmd_desc *cm, char __iomem *dsi_base)
{
	int len = 0;
	unsigned long flags;

	disp_check_and_return(!cm, -EINVAL, err, "cmds is NULL!\n");

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	mipi_dsi_cmd_send_lock();
	spin_lock_irqsave(&g_mipi_trans_lock, flags);

	switch (DSI_HDR_DTYPE(cm->dtype)) {
	case DTYPE_GEN_WRITE:
	case DTYPE_GEN_WRITE1:
	case DTYPE_GEN_WRITE2:

	case DTYPE_DCS_WRITE:
	case DTYPE_DCS_WRITE1:
	case DTYPE_DCS_WRITE2:
		len = mipi_dsi_swrite(cm, dsi_base);
		break;
	case DTYPE_GEN_LWRITE:
	case DTYPE_DCS_LWRITE:
	case DTYPE_DSC_LWRITE:

		len = mipi_dsi_lwrite(cm, dsi_base);
		break;
	default:
		disp_pr_err("dtype=%x NOT supported!\n", cm->dtype);
		break;
	}

	spin_unlock_irqrestore(&g_mipi_trans_lock, flags);
	mipi_dsi_cmd_send_unlock();

	return len;
}

int dpu_mipi_dsi_cmds_tx(struct dsi_cmd_desc *cmds, int cnt, char __iomem *dsi_base)
{
	struct dsi_cmd_desc *cm = NULL;
	int i;

	disp_check_and_return(!cmds, -EINVAL, err, "cmds is NULL!\n");

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		mipi_dsi_cmd_add(cm, dsi_base);
		delay_for_next_cmd_by_sleep(cm->wait, cm->waittype);
		cm++;
	}

	return cnt;
}

static void mipi_dsi_check_0lane_is_ready(const char __iomem *dsi_base)
{
	unsigned long dw_jiffies;
	uint32_t temp = 0;

	dw_jiffies = jiffies + HZ / 10;  /* HZ / 10 = 0.1s */
	do {
		temp = inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base));
		/* phy_stopstate0lane */
		if ((temp & 0x10) == 0x10) {
			disp_pr_info("0 lane is stopping state\n");
			return;
		}
	} while (time_after(dw_jiffies, jiffies));

	disp_pr_err("0 lane is not stopping state:tmp=0x%x\n", temp);
}

static void mipi_dsi_sread_request(struct dsi_cmd_desc *cm, char __iomem *dsi_base)
{
	uint32_t hdr = 0;

	/* fill up header */
	hdr |= DSI_HDR_DTYPE(cm->dtype);
	hdr |= DSI_HDR_VC(cm->vc);
	hdr |= DSI_HDR_DATA1((uint32_t)(cm->payload[0]));  /*lint !e571*/
	hdr |= DSI_HDR_DATA2(0);
#ifdef MIPI_DSI_HOST_VID_LP_MODE
	/* used for low power cmds trans under video mode */
	hdr |= cm->dtype & GEN_VID_LP_CMD;
	dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), hdr, 25, 0);
#else
	dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), hdr, 24, 0);
#endif
}

/*lint -e529*/
static int mipi_dsi_read_phy_status(const struct dsi_cmd_desc *cm, const char __iomem *dsi_base)
{
	unsigned long dw_jiffies;
	uint32_t pkg_status;
	uint32_t phy_status;
	int is_timeout = 1;

	/* read status register */
	dw_jiffies = jiffies + HZ;
	do {
		pkg_status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
		phy_status = inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base));
		if ((pkg_status & 0x1) == 0x1 && !(phy_status & 0x2)) {
			is_timeout = 0;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (is_timeout) {
		disp_pr_err("mipi_dsi_read timeout :0x%x\n"
			"IPIDSI_CMD_PKT_STATUS = 0x%x\n"
			"IPIDSI_PHY_STATUS = 0x%x\n"
			"IPIDSI_INT_ST1_OFFSET = 0x%x\n",
			cm->payload[0],
			inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base)),
			inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base)),
			inp32(DPU_DSI_INT_ERROR_FORCE1_ADDR(dsi_base)));

		return -1;
	}

	return 0;
}
/*lint +e529*/

static int mipi_dsi_wait_data_status(const struct dsi_cmd_desc *cm, const char __iomem *dsi_base)
{
	unsigned long dw_jiffies;
	uint32_t pkg_status;
	int is_timeout = 1;

	/* wait dsi read data */
	dw_jiffies = jiffies + HZ;
	do {
		pkg_status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));  /*lint -e529*/
		if (!(pkg_status & 0x10)) {
			is_timeout = 0;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	if (is_timeout) {
		disp_pr_err("mipi_dsi_read timeout :0x%x\n"
			"IPIDSI_CMD_PKT_STATUS = 0x%x\n"
			"IPIDSI_PHY_STATUS = 0x%x\n",
			cm->payload[0],
			inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base)),  /*lint -e529*/
			inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base)));  /*lint -e529*/
		return -1;
	}

	return 0;
}

static int mipi_dsi_read_add(uint32_t *out, struct dsi_cmd_desc *cm, char __iomem *dsi_base)
{
	int ret = 0;

	disp_check_and_return(!cm, -EINVAL, err, "cmds is NULL!\n");

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	if (DSI_HDR_DTYPE(cm->dtype) == DTYPE_DCS_READ) {
		mipi_dsi_cmd_send_lock();

		mipi_dsi_sread_request(cm, dsi_base);

		if (!mipi_dsi_read(out, dsi_base)) {
			disp_pr_err("Read register 0x%X timeout\n", cm->payload[0]);
			ret = -1;
		}

		mipi_dsi_cmd_send_unlock();
	} else if (cm->dtype == DTYPE_GEN_READ1) {
		ret = mipi_dsi_read_phy_status(cm, dsi_base);
		if (ret)
			return ret;

		mipi_dsi_cmd_send_lock();
		/* send read cmd to fifo */
#ifdef MIPI_DSI_HOST_VID_LP_MODE
		/* used for low power cmds trans under video mode */
		dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), (((uint32_t)cm->payload[0] << 8) |  /*lint !e571*/
			(cm->dtype & GEN_VID_LP_CMD)), 25, 0);
#else
		dpu_set_reg(DPU_DSI_APB_WR_LP_HDR_ADDR(dsi_base), (((uint32_t)cm->payload[0] << 8) | cm->dtype), 24, 0);
#endif

		ret = mipi_dsi_wait_data_status(cm, dsi_base);
		if (ret) {
			mipi_dsi_cmd_send_unlock();
			return ret;
		}

		/* get read data */
		*out = inp32(DPU_DSI_APB_WR_LP_PLD_DATA_ADDR(dsi_base));

		mipi_dsi_cmd_send_unlock();
	} else {
		ret = -1;
		disp_pr_err("dtype=%x NOT supported!\n", cm->dtype);
	}

	return ret;
}

int dpu_mipi_dsi_cmds_rx(uint32_t *out, struct dsi_cmd_desc *cmds, int cnt,
	char __iomem *dsi_base)
{
	struct dsi_cmd_desc *cm = NULL;
	int i;
	int err_num = 0;

	disp_check_and_return(!cmds, -EINVAL, err, "cmds is NULL!\n");

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (mipi_dsi_read_add(&(out[i]), cm, dsi_base))
			err_num++;
		delay_for_next_cmd_by_sleep(cm->wait, cm->waittype);
		cm++;
	}

	return err_num;
}

static int mipi_dsi_read_compare(struct mipi_dsi_read_compare_data *data, char __iomem *dsi_base)
{
	uint32_t *read_value = NULL;
	uint32_t *expected_value = NULL;
	uint32_t *read_mask = NULL;
	char **reg_name = NULL;
	int log_on;
	struct dsi_cmd_desc *cmds = NULL;

	int cnt;
	int cnt_not_match = 0;
	int ret;
	int i;

	disp_check_and_return(!data, -EINVAL, err, "data is NULL!\n");

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	read_value = data->read_value;
	expected_value = data->expected_value;
	read_mask = data->read_mask;
	reg_name = data->reg_name;
	log_on = data->log_on;

	cmds = data->cmds;
	cnt = data->cnt;

	ret = dpu_mipi_dsi_cmds_rx(read_value, cmds, cnt, dsi_base);
	if (ret) {
		disp_pr_err("Read error number: %d\n", ret);
		return cnt;
	}

	for (i = 0; i < cnt; i++) {
		if (log_on)
			disp_pr_info("Read reg %s: 0x%x, value = 0x%x\n",
				reg_name[i], cmds[i].payload[0], read_value[i]);

		if (expected_value[i] != (read_value[i] & read_mask[i]))
			cnt_not_match++;
	}

	return cnt_not_match;
}

static int mipi_dsi_fifo_is_full(const char __iomem *dsi_base)
{
	uint32_t pkg_status;
	uint32_t phy_status;
	int is_timeout = 100;  /* 10ms(100*100us) */

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	/* read status register */
	do {
		pkg_status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base));
		phy_status = inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base));
		if ((pkg_status & 0x2) != 0x2 && !(phy_status & 0x2))
			break;
		udelay(100);  /* 100us */
	} while (is_timeout-- > 0);

	if (is_timeout < 0) {
		disp_pr_err("mipi check full fail:\n"
			"IPIDSI_CMD_PKT_STATUS = 0x%x\n"
			"IPIDSI_PHY_STATUS = 0x%x\n"
			"IPIDSI_INT_ST1_OFFSET = 0x%x\n",
			inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(dsi_base)),
			inp32(DPU_DSI_CDPHY_STATUS_ADDR(dsi_base)),
			inp32(DPU_DSI_INT_ERROR_FORCE1_ADDR(dsi_base)));

		return -1;
	}
	return 0;
}

static int mipi_dsi_cmds_tx_with_check_fifo(struct dsi_cmd_desc *cmds, int cnt, char __iomem *dsi_base)
{
	struct dsi_cmd_desc *cm = NULL;
	int i;

	disp_check_and_return(!cmds, -EINVAL, err, "cmds is NULL!\n");

	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (!mipi_dsi_fifo_is_full(dsi_base)) {
			mipi_dsi_cmd_add(cm, dsi_base);
		} else {
			disp_pr_err("dsi fifo full, write [%d] cmds, left [%d] cmds!!", i, cnt-i);
			break;
		}
		delay_for_next_cmd_by_sleep(cm->wait, cm->waittype);
		cm++;
	}

	return cnt;
}

static int mipi_dsi_cmds_rx_with_check_fifo(uint32_t *out, struct dsi_cmd_desc *cmds, int cnt, char __iomem *dsi_base)
{
	struct dsi_cmd_desc *cm = NULL;
	int i;
	int err_num = 0;

	disp_check_and_return(!cmds, -EINVAL, err, "cmds is NULL!\n");
	disp_check_and_return(!dsi_base, -EINVAL, err, "dsi_base is NULL!\n");

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (!mipi_dsi_fifo_is_full(dsi_base)) {
			if (mipi_dsi_read_add(&(out[i]), cm, dsi_base))
				err_num++;
		} else {
			err_num += (cnt - i);
			disp_pr_err("dsi fifo full, read [%d] cmds, left [%d] cmds!!", i, cnt - i);
			break;
		}
		delay_for_next_cmd_by_sleep(cm->wait, cm->waittype);
		cm++;
	}

	return err_num;
}

static void mipi_dsi_get_cmd_queue_resource(struct dsi_cmd_desc **cmd_queue,
	spinlock_t **spinlock, uint32_t *queue_len, bool is_low_priority)
{
	if (is_low_priority) {
		*cmd_queue = g_delayed_cmd_queue.CmdQueue_LowPriority;
		*spinlock = &g_delayed_cmd_queue.CmdQueue_LowPriority_lock;
		*queue_len = MAX_CMD_QUEUE_LOW_PRIORITY_SIZE;
	} else {
		*cmd_queue = g_delayed_cmd_queue.CmdQueue_HighPriority;
		*spinlock = &g_delayed_cmd_queue.CmdQueue_HighPriority_lock;
		*queue_len = MAX_CMD_QUEUE_HIGH_PRIORITY_SIZE;
	}
}

static void mipi_dsi_get_cmd_queue_prio_status(uint32_t **w_ptr, uint32_t **r_ptr,
	bool **is_queue_full, bool **is_queue_working, bool is_low_priority)
{
	if (is_low_priority) {
		*w_ptr = &g_delayed_cmd_queue.CmdQueue_LowPriority_wr;
		*r_ptr = &g_delayed_cmd_queue.CmdQueue_LowPriority_rd;
		*is_queue_full = &g_delayed_cmd_queue.isCmdQueue_LowPriority_full;
		*is_queue_working = &g_delayed_cmd_queue.isCmdQueue_LowPriority_working;
	} else {
		*w_ptr = &g_delayed_cmd_queue.CmdQueue_HighPriority_wr;
		*r_ptr = &g_delayed_cmd_queue.CmdQueue_HighPriority_rd;
		*is_queue_full = &g_delayed_cmd_queue.isCmdQueue_HighPriority_full;
		*is_queue_working = &g_delayed_cmd_queue.isCmdQueue_HighPriority_working;
	}
}

static int mipi_dsi_cmd_queue_init(struct dsi_cmd_desc *cmd_queue_elem, struct dsi_cmd_desc *cmd)
{
	u32 j;

	cmd_queue_elem->dtype = cmd->dtype;
	cmd_queue_elem->vc = cmd->vc;
	cmd_queue_elem->wait = cmd->wait;
	cmd_queue_elem->waittype = cmd->waittype;
	cmd_queue_elem->dlen = cmd->dlen;
	if (cmd->dlen > 0) {
		cmd_queue_elem->payload = (char *)kmalloc(cmd->dlen * sizeof(char), GFP_ATOMIC);
		if (!cmd_queue_elem->payload) {
			disp_pr_err("cmd[%d/%d] payload malloc [%d] fail!\n", cmd->dtype, cmd->vc, cmd->dlen);
			return -1;  /* skip this cmd */
		}
		memset(cmd_queue_elem->payload, 0, cmd->dlen * sizeof(char));
		for (j = 0; j < cmd->dlen; j++)
			cmd_queue_elem->payload[j] = cmd->payload[j];
	}

	return 0;
}

static int mipi_dsi_delayed_cmd_queue_write(struct dsi_cmd_desc *cmd_set,
	int cmd_set_cnt, bool is_low_priority)
{
	spinlock_t *spinlock = NULL;
	uint32_t *w_ptr = NULL;
	uint32_t *r_ptr = NULL;
	struct dsi_cmd_desc *cmd_queue = NULL;
	bool *is_queue_full = NULL;
	bool *is_queue_working = NULL;
	uint32_t queue_len;
	int i;
	struct dsi_cmd_desc *cmd = cmd_set;

	disp_check_and_return(!cmd_set, 0, err, "cmd is NULL!\n");

	if (!g_delayed_cmd_queue_inited) {
		disp_pr_err("delayed cmd queue is not inited yet!\n");
		return 0;
	}

	mipi_dsi_get_cmd_queue_resource(&cmd_queue, &spinlock, &queue_len, is_low_priority);

	spin_lock(spinlock);

	mipi_dsi_get_cmd_queue_prio_status(&w_ptr, &r_ptr, &is_queue_full, &is_queue_working, is_low_priority);

	if (*is_queue_full) {
		disp_pr_err("Fail, delayed cmd queue [%d] is full!\n", is_low_priority);
		spin_unlock(spinlock);
		return 0;
	}

	for (i = 0; i < cmd_set_cnt; i++) {
		if (mipi_dsi_cmd_queue_init(&cmd_queue[(*w_ptr)], cmd) < 0)
			continue;

		(*w_ptr) = (*w_ptr) + 1;
		if ((*w_ptr) >= queue_len)
			(*w_ptr) = 0;

		(*is_queue_working) = true;

		if ((*w_ptr) == (*r_ptr)) {
			(*is_queue_full) = true;
			disp_pr_err("Fail, delayed cmd queue [%d] become full, %d cmds are not added in queue!\n",
				is_low_priority, (cmd_set_cnt - i));
			break;
		}

		cmd++;
	}

	spin_unlock(spinlock);

	disp_pr_debug("%d cmds are added to delayed cmd queue [%d]\n", i, is_low_priority);

	return i;
}

static int mipi_dsi_delayed_cmd_queue_read(struct dsi_cmd_desc *cmd, bool is_low_priority)
{
	spinlock_t *spinlock = NULL;
	uint32_t *w_ptr = NULL;
	uint32_t *r_ptr = NULL;
	struct dsi_cmd_desc *cmd_queue = NULL;
	bool *is_queue_full = NULL;
	bool *is_queue_working = NULL;
	uint32_t queue_len;

	disp_check_and_return(!cmd, -1, err, "cmd is NULL!\n");

	if (!g_delayed_cmd_queue_inited) {
		disp_pr_err("delayed cmd queue is not inited yet!\n");
		return -1;
	}

	mipi_dsi_get_cmd_queue_resource(&cmd_queue, &spinlock, &queue_len, is_low_priority);

	spin_lock(spinlock);

	mipi_dsi_get_cmd_queue_prio_status(&w_ptr, &r_ptr, &is_queue_full, &is_queue_working, is_low_priority);

	if ((!(*is_queue_full)) && ((*w_ptr) == (*r_ptr))) {
		spin_unlock(spinlock);
		return -1;
	}

	cmd->dtype = cmd_queue[(*r_ptr)].dtype;
	cmd->vc = cmd_queue[(*r_ptr)].vc;
	cmd->wait = cmd_queue[(*r_ptr)].wait;
	cmd->waittype = cmd_queue[(*r_ptr)].waittype;
	cmd->dlen = cmd_queue[(*r_ptr)].dlen;
	/* Note: just copy the pointer. The malloc mem will be free by the caller after useless */
	cmd->payload = cmd_queue[(*r_ptr)].payload;
	memset(&(cmd_queue[(*r_ptr)]), 0, sizeof(struct dsi_cmd_desc));

	(*r_ptr) = (*r_ptr) + 1;
	if ((*r_ptr) >= queue_len)
		(*r_ptr) = 0;

	(*is_queue_full) = false;

	if ((*r_ptr) == (*w_ptr)) {
		(*is_queue_working) = false;
		disp_pr_debug("The last cmd in delayed queue [%d] is sent!\n", is_low_priority);
	}

	spin_unlock(spinlock);

	disp_pr_debug("Acquire one cmd from delayed cmd queue [%d] successl!\n", is_low_priority);

	return 0;
}

static uint32_t mipi_dsi_get_delayed_cmd_queue_send_count(bool is_low_priority)
{
	spinlock_t *spinlock = NULL;
	uint32_t w_ptr = 0;
	uint32_t r_ptr = 0;
	bool is_queue_full = false;
	uint32_t queue_len;
	uint32_t send_count = 0;


	if (!g_delayed_cmd_queue_inited) {
		disp_pr_err("delayed cmd queue is not inited yet!\n");
		return 0;
	}

	if (is_low_priority) {
		spinlock = &g_delayed_cmd_queue.CmdQueue_LowPriority_lock;
		queue_len = MAX_CMD_QUEUE_LOW_PRIORITY_SIZE;
	} else {
		spinlock = &g_delayed_cmd_queue.CmdQueue_HighPriority_lock;
		queue_len = MAX_CMD_QUEUE_HIGH_PRIORITY_SIZE;
	}

	spin_lock(spinlock);

	if (is_low_priority) {
		w_ptr = g_delayed_cmd_queue.CmdQueue_LowPriority_wr;
		r_ptr = g_delayed_cmd_queue.CmdQueue_LowPriority_rd;
		is_queue_full = g_delayed_cmd_queue.isCmdQueue_LowPriority_full;
	} else {
		w_ptr = g_delayed_cmd_queue.CmdQueue_HighPriority_wr;
		r_ptr = g_delayed_cmd_queue.CmdQueue_HighPriority_rd;
		is_queue_full = g_delayed_cmd_queue.isCmdQueue_HighPriority_full;
	}

	spin_unlock(spinlock); /* no need lock any more */

	if (w_ptr > r_ptr)
		send_count = w_ptr - r_ptr;
	else if (w_ptr < r_ptr)
		send_count = w_ptr + queue_len - r_ptr;
	else if (is_queue_full)
		send_count = queue_len;
	/* else: means queue empty, send_count is 0 */

	if (send_count > 0)
		disp_pr_debug("delay cmd queue [%d]: %d cmds need to be sent\n", is_low_priority, send_count);

	return send_count;
}

static void mipi_dsi_init_delayed_cmd_queue(void)
{
	disp_pr_info("+\n");

	memset(&g_delayed_cmd_queue, 0, sizeof(struct dsi_delayed_cmd_queue));
	sema_init(&g_delayed_cmd_queue.work_queue_sem, 1);
	spin_lock_init(&g_delayed_cmd_queue.CmdSend_lock);
	spin_lock_init(&g_delayed_cmd_queue.CmdQueue_LowPriority_lock);
	spin_lock_init(&g_delayed_cmd_queue.CmdQueue_HighPriority_lock);
	g_delayed_cmd_queue_inited = true;
}

/* TODO */
#if 0
void mipi_dsi_delayed_cmd_queue_handle_func(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct dsi_cmd_desc Cmd = {0};
	uint32_t send_count_high_priority;
	uint32_t send_count_low_priority;
	s64 oneframe_time;

	disp_check_and_no_retval(!work, err, "work is NULL\n");

	if (!g_delayed_cmd_queue_inited) {
		disp_pr_err("delayed cmd queue is not inited yet!\n");
		return;
	}

	hisifd = container_of(work, struct hisi_fb_data_type, delayed_cmd_queue_work);
	disp_check_and_no_retval(!hisifd, err, "hisifd is NULL\n");

	disp_pr_info("+\n");

	down(&g_delayed_cmd_queue.work_queue_sem);

	send_count_low_priority = mipi_dsi_get_delayed_cmd_queue_send_count(true);
	send_count_high_priority = mipi_dsi_get_delayed_cmd_queue_send_count(false);

	while (send_count_high_priority > 0) {
		if (mipi_dsi_delayed_cmd_queue_read(&Cmd, false)) { /* get the next cmd in high priority queue */
			/* send the cmd with normal mode */
			mipi_dual_dsi_tx_normal_same_delay(&Cmd, &Cmd, 1,
				hisifd->mipi_dsi0_base, hisifd->mipi_dsi1_base);
			if (Cmd.payload) {
				kfree(Cmd.payload);
				Cmd.payload = NULL;
			}
		}
		send_count_high_priority--;
	}

	while (send_count_low_priority > 0) {
		if (mipi_dsi_delayed_cmd_queue_read(&Cmd, true)) {  /* get the next cmd in low priority queue */
			/* send the cmd with normal mode */
			mipi_dual_dsi_tx_normal_same_delay(&Cmd, &Cmd, 1,
				hisifd->mipi_dsi0_base, hisifd->mipi_dsi1_base);
			if (Cmd.payload) {
				kfree(Cmd.payload);
				Cmd.payload = NULL;
			}
		}
		send_count_low_priority--;
	}

	up(&g_delayed_cmd_queue.work_queue_sem);

	/* re-fresh one frame time unit:ms */
	oneframe_time = (s64)(hisifd->panel_info.xres + hisifd->panel_info.ldi.h_back_porch +
		hisifd->panel_info.ldi.h_front_porch + hisifd->panel_info.ldi.h_pulse_width) *
		(hisifd->panel_info.yres + hisifd->panel_info.ldi.v_back_porch + hisifd->panel_info.ldi.v_front_porch +
		hisifd->panel_info.ldi.v_pulse_width) * 1000000000UL / hisifd->panel_info.pxl_clk_rate;
	if (oneframe_time != g_delayed_cmd_queue.oneframe_time) {
		g_delayed_cmd_queue.oneframe_time = oneframe_time;
		disp_pr_info("update one frame time to %lld\n", oneframe_time);
	}

	disp_pr_info("-\n");
}
#endif

static void mipi_dsi_set_timestamp(void)
{
	struct timespec64 ts;
	s64 timestamp = 0;

	if (g_delayed_cmd_queue_inited) {
		ktime_get_ts64(&ts);
		timestamp = ts.tv_sec;
		timestamp *= NSEC_PER_SEC;
		timestamp += ts.tv_nsec;
		g_delayed_cmd_queue.timestamp_frame_start = timestamp;
	}
}

static bool mipi_dsi_check_delayed_cmd_queue_working(void)
{
	bool ret = false;

	if (g_delayed_cmd_queue.isCmdQueue_HighPriority_working || g_delayed_cmd_queue.isCmdQueue_LowPriority_working)
		ret = true;

	return ret;
}
