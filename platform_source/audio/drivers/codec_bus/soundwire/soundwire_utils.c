/*
 * soundwire_utils.c -- soundwire utils
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "soundwire_utils.h"

#include <linux/io.h>
#include <linux/delay.h>
#include "soc_asp_cfg_interface.h"
#include "soundwire_master_reg.h"
#include "soundwire_slave_reg.h"
#include "soundwire_type.h"
#include "soundwire_irq.h"
#include "audio_log.h"

#define LOG_TAG "soundwire_utils"
#define WAIT_CNT 100
#define LOCK_TIME_CNT 300

enum slv_status soundwire_get_slv_status(struct soundwire_priv *priv)
{
	MST_SLV_STAT0_UNION mst_slv_stat0;

	if (priv == NULL)
		return SLV_OFFLINE;

	mst_slv_stat0.value = mst_read_reg(priv, MST_SLV_STAT0_OFFSET);

	return mst_slv_stat0.reg.slv_stat_01;
}

void mst_write_reg(struct soundwire_priv *priv, unsigned char value, unsigned int reg_offset)
{
	if (priv == NULL)
		return;

	writeb(value, priv->sdw_mst_mem + reg_offset);
}

unsigned char mst_read_reg(struct soundwire_priv *priv, unsigned int reg_offset)
{
	if (priv == NULL)
		return 0;

	return readb(priv->sdw_mst_mem + reg_offset);
}

static int is_reg_req_coming(struct soundwire_priv *priv)
{
	MST_INTSTAT2_UNION mst_intstat2;
	unsigned int cnt = WAIT_CNT;

	while (cnt) {
		mst_intstat2.value = mst_read_reg(priv, MST_INTSTAT2_OFFSET);
		if (mst_intstat2.reg.reg_req_int == 1)
			return 0;
		udelay(25);
		cnt--;
	}

	AUDIO_LOGE("check slave reg req timeout");
	return -EFAULT;
}

static void slv_try_lock(struct soundwire_priv *priv)
{
	unsigned int time_cnt = LOCK_TIME_CNT;

	while (time_cnt) {
		if (mst_read_reg(priv, MST_DATAPORT_TEST_PROC_SEL_OFFSET) == 0)
			break;
		udelay(10);
		time_cnt--;
	}

	if (time_cnt == 0) {
		AUDIO_LOGE("slv try lock timeout");
		return;
	}

	mst_write_reg(priv, 0x1, MST_DATAPORT_TEST_PROC_SEL_OFFSET);
}

static void slv_unlock(struct soundwire_priv *priv)
{
	mst_write_reg(priv, 0x0, MST_DATAPORT_TEST_PROC_SEL_OFFSET);
}

unsigned char slv_read_reg(struct soundwire_priv *priv, unsigned int reg_offset)
{
	unsigned char value;

	if (priv == NULL)
		return 0;

	slv_try_lock(priv);
	soundwire_clr_intr(priv, IRQ_REG_REQ);

	value = readb(priv->sdw_slv_mem + reg_offset);
	if (is_reg_req_coming(priv) != 0)
		goto exit;

	value = readb(priv->sdw_slv_mem + reg_offset);
	if (is_reg_req_coming(priv) != 0)
		goto exit;

exit:
	soundwire_clr_intr(priv, IRQ_REG_REQ);
	slv_unlock(priv);
	return value;
}

void slv_write_reg(struct soundwire_priv *priv, unsigned char value, unsigned int reg_offset)
{
	if (priv == NULL)
		return;

	slv_try_lock(priv);
	soundwire_clr_intr(priv, IRQ_REG_REQ);

	writeb(value, priv->sdw_slv_mem + reg_offset);
	if (is_reg_req_coming(priv) != 0)
		AUDIO_LOGE("write slv reg failed");

	soundwire_clr_intr(priv, IRQ_REG_REQ);
	slv_unlock(priv);
}

unsigned int asp_cfg_read_reg(struct soundwire_priv *priv, unsigned int reg_offset)
{
	if (priv == NULL)
		return 0;

	return readl(priv->asp_cfg_mem + reg_offset);
}

void asp_cfg_write_reg(struct soundwire_priv *priv, unsigned int value, unsigned int reg_offset)
{
	if (priv == NULL)
		return;

	writel(value, priv->asp_cfg_mem + reg_offset);
}

void write_fifo(struct soundwire_priv *priv, unsigned int offset, unsigned int value)
{
	if (priv == NULL)
		return;

	writel(value, priv->sdw_fifo_mem + offset);
}

void intercore_try_lock(struct soundwire_priv *priv)
{
	unsigned int time_cnt = LOCK_TIME_CNT;

	if (priv == NULL)
		return;

	while (time_cnt) {
		if (readl(priv->asp_cfg_mem + SOC_ASP_CFG_R_HIFIDSP_FLAG0_ADDR(0)) == 0)
			break;
		udelay(10);
		time_cnt--;
	}

	if (time_cnt == 0) {
		AUDIO_LOGE("try lock timeout");
		return;
	}

	writel(1, priv->asp_cfg_mem + SOC_ASP_CFG_R_HIFIDSP_FLAG0_ADDR(0));
}

void intercore_unlock(struct soundwire_priv *priv)
{
	if (priv == NULL)
		return;

	writel(0, priv->asp_cfg_mem + SOC_ASP_CFG_R_HIFIDSP_FLAG0_ADDR(0));
}
