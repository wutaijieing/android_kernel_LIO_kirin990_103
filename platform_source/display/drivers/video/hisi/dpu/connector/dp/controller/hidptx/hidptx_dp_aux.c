/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hidptx_dp_aux.h"
#include "hidptx_dp_core.h"
#include "hidptx_reg.h"
#include "hisi_drv_dp.h"

#define DPTX_NO_DEBUG_REG
#define AUX_WRITE_BYTES_SUCCESS 1

/*lint -save -e* */
static void dptx_aux_clear_data(struct dp_ctrl *dptx)
{
	disp_check_and_no_retval((dptx == NULL), err, "[DP] NULL Pointer\n");

	dptx_writel(dptx, DPTX_AUX_WR_DATA0, 0);
	dptx_writel(dptx, DPTX_AUX_WR_DATA1, 0);
	dptx_writel(dptx, DPTX_AUX_WR_DATA2, 0);
	dptx_writel(dptx, DPTX_AUX_WR_DATA3, 0);
}

static int dptx_aux_read_data(struct dp_ctrl *dptx, uint8_t *bytes, uint32_t len)
{
	uint32_t i;
	uint32_t *data = NULL;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL\n");
	disp_check_and_return((bytes == NULL), -EINVAL, err, "[DP] bytes is NULL\n");

	data = dptx->aux.data;
	for (i = 0; i < len; i++) {
		if ((i/4) > 3)
			return -EINVAL;

		bytes[i] = (data[i / 4] >> ((i % 4) * 8)) & 0xff;
	}

	return len;
}

static int dptx_aux_write_data(struct dp_ctrl *dptx, uint8_t const *bytes,
	uint32_t len)
{
	uint32_t i;
	uint32_t data[4];

	memset(data, 0, sizeof(uint32_t) * 4);

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL\n");
	disp_check_and_return((bytes == NULL), -EINVAL, err, "[DP] bytes is NULL\n");

	for (i = 0; i < len; i++) {
		if ((i/4) > 3)
			return -EINVAL;

		data[i / 4] |= (bytes[i] << ((i % 4) * 8));
	}

	dptx_writel(dptx, DPTX_AUX_WR_DATA0, data[0]);
	dptx_writel(dptx, DPTX_AUX_WR_DATA1, data[1]);
	dptx_writel(dptx, DPTX_AUX_WR_DATA2, data[2]);
	dptx_writel(dptx, DPTX_AUX_WR_DATA3, data[3]);

	return len;
}

static int check_aux_status(struct dp_ctrl *dptx, bool *try_again, bool rw)
{
	uint32_t aux_status;
	uint32_t cfg_aux_status;
	uint32_t aux_reply_err_detected;
	uint32_t cfg_aux_timeout;
	uint32_t cfg_aux_ready_data_byte;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL\n");
	disp_check_and_return((try_again == NULL), -EINVAL, err, "[DP] try_again is NULL\n");

	aux_status = (uint32_t)dptx_readl(dptx, DPTX_AUX_STATUS);
	cfg_aux_status = (aux_status & DPTX_CFG_AUX_STATUS_MASK) >> DPTX_CFG_AUX_STATUS_SHIFT;
	aux_reply_err_detected = (aux_status & DPTX_CFG_AUX_REPLY_ERR_DETECTED_MASK) >>
		DPTX_CFG_AUX_REPLY_ERR_DETECTED_SHIFT;
	cfg_aux_timeout = aux_status & DPTX_CFG_AUX_TIMEOUT;
	cfg_aux_ready_data_byte = (aux_status & DPTX_CFG_AUX_READY_DATA_BYTE_MASK)
		>> DPTX_CFG_AUX_READY_DATA_BYTE_SHIFT;

	//disp_pr_debug("[DP]aux_status = %x", aux_status);

	switch (cfg_aux_status) {
	case DPTX_CFG_AUX_STATUS_ACK:
		disp_pr_debug("[DP] AUX Success\n");
		if (cfg_aux_ready_data_byte == 0) {
			disp_pr_debug("[DP] rw error, Retry; cfg_aux_timeout is %d, aux_reply_err_detected is %d\n",
				cfg_aux_timeout, aux_reply_err_detected);
			//dptx_soft_reset(dptx, DPTX_AUX_RST_N);
			*try_again = true;
		}
		break;
	case DPTX_CFG_AUX_STATUS_AUX_NACK:
	case DPTX_CFG_AUX_STATUS_I2C_NACK:
		disp_pr_err("[DP] AUX Nack\n");
		return -ECONNREFUSED;
	case DPTX_CFG_AUX_STATUS_AUX_DEFER:
	case DPTX_CFG_AUX_STATUS_I2C_DEFER:
		disp_pr_err("[DP] AUX Defer\n");
		*try_again = true;
		break;
	default:
		disp_pr_err("[DP] AUX Status Invalid : 0x%x, try again\n", cfg_aux_status);
		//dptx_soft_reset(dptx, DPTX_AUX_RST_N);
		*try_again = true;
		break;
	}

	return 0;
}

static uint32_t dptx_get_aux_cmd(bool rw, bool i2c, bool mot,
	bool addr_only, uint32_t addr, uint32_t len)
{
	uint32_t type;
	uint32_t auxcmd;

	type = rw ? DPTX_AUX_CMD_TYPE_READ : DPTX_AUX_CMD_TYPE_WRITE;

	if (!i2c)
		type |= DPTX_AUX_CMD_TYPE_NATIVE;

	if (i2c && mot)
		type |= DPTX_AUX_CMD_TYPE_MOT;

	auxcmd = ((type << DPTX_AUX_CMD_TYPE_SHIFT) |
		(addr << DPTX_AUX_CMD_ADDR_SHIFT) |
		((len - 1) << DPTX_AUX_CMD_REQ_LEN_SHIFT));

	if (addr_only)
		auxcmd |= DPTX_AUX_CMD_I2C_ADDR_ONLY;

	return auxcmd;
}

int dptx_aux_rw(struct dp_ctrl *dptx, bool rw, bool i2c, bool mot, bool addr_only, uint32_t addr,
	uint8_t *bytes, uint32_t len)
{
	int retval;
	uint32_t aux_req_val;
	int tries = 0;
	uint32_t auxcmd;
	uint32_t aux_req;
	uint32_t cfg_aux_req;
	int aux_req_count;
	int max_aux_req_count = 5000;
	bool try_again = false;

	disp_check_and_return((dptx == NULL), -EINVAL, err, "[DP] dptx is NULL\n");
	disp_check_and_return((bytes == NULL), -EINVAL, err, "[DP] bytes is NULL\n");

again:
	mdelay(1);
	tries++;
	if (tries > 100) {
		disp_pr_err("[DP] AUX exceeded retries\n");
		return -EAGAIN;
	}

	if (dptx->dp_shutdown) {
		disp_pr_info("[DP] dp already shutdown, exit aux rw");
		return -EINVAL;
	}

	disp_pr_debug("[DP] device addr=0x%08x, len=%d, try=%d\n",
					addr, len, tries);

	if ((len > 16) || (len == 0)) {
		disp_pr_err("[DP] AUX read/write len must be 1-15, len=%d\n", len);
		return -EINVAL;
	}

	dptx_aux_clear_data(dptx);

	if (!rw)
		dptx_aux_write_data(dptx, bytes, len);

	auxcmd = dptx_get_aux_cmd(rw, i2c, mot, addr_only, addr, len);
	dptx_writel(dptx, DPTX_AUX_CMD_ADDR, auxcmd);

	aux_req_val = dptx_readl(dptx, DPTX_AUX_REQ);
	aux_req_val |= DPTX_CFG_AUX_REQ;
	dptx_writel(dptx, DPTX_AUX_REQ, aux_req_val);

	aux_req_count = 0;
	while (true) {
		aux_req = (uint32_t)dptx_readl(dptx, DPTX_AUX_REQ);
		cfg_aux_req = aux_req & DPTX_CFG_AUX_REQ;
		if (!cfg_aux_req)
			break;

		aux_req_count++;
		if (aux_req_count > max_aux_req_count) {
			disp_pr_err("[DP] wait aux_req=0 exceeded retries\n");
			return -ETIMEDOUT;
		}

		udelay(1);
	};

	try_again = false;
	retval = check_aux_status(dptx, &try_again, rw);
	if (retval)
		return retval;
	if (try_again)
		goto again;

	if (rw) {
		dptx->aux.data[0] = dptx_readl(dptx, DPTX_AUX_RD_DATA0);
		dptx->aux.data[1] = dptx_readl(dptx, DPTX_AUX_RD_DATA1);
		dptx->aux.data[2] = dptx_readl(dptx, DPTX_AUX_RD_DATA2);
		dptx->aux.data[3] = dptx_readl(dptx, DPTX_AUX_RD_DATA3);
		dptx_aux_read_data(dptx, bytes, len);
	}

	return 0;
}
/*lint -restore*/
