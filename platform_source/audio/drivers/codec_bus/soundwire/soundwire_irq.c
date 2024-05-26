/*
 * soundwire_irq.c -- soundwire irq
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

#include "soundwire_irq.h"

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include "soundwire_type.h"
#include "soundwire_bandwidth_adp.h"
#include "soundwire_utils.h"
#include "soundwire_master_reg.h"
#include "soundwire_slave_reg.h"

#include "audio_log.h"

#define LOG_TAG "soundwire_irq"

#define WAIT_CNT 100
#define SOUNDWIRE_MAX_DPNUM 14
#define SDW_IRQ_REG_BITS_NUM 8
#define irq_index(num) (num / SDW_IRQ_REG_BITS_NUM)
#define irq_offset(num) (num % SDW_IRQ_REG_BITS_NUM)
#define dp_num_reg(dpnum, offset) ((dpnum) * 0x100 + (offset))

enum reset_type {
	BUS_RESET,
	FORCE_RESET
};

struct irq_reg {
	unsigned int irq_clear;
	unsigned int irq_mask;
	unsigned int irq_stat;
};

struct irq_handler {
	int phy_irq;
	void (*irq_callback)(struct soundwire_priv *priv);
};

static struct irq_reg g_irqs[] = {
	{ MST_INTSTAT1_OFFSET, MST_INTMASK1_OFFSET, MST_INTMSTAT1_OFFSET },
	{ MST_INTSTAT2_OFFSET, MST_INTMASK2_OFFSET, MST_INTMSTAT2_OFFSET },
	{ MST_INTSTAT3_OFFSET, MST_INTMASK3_OFFSET, MST_INTMSTAT3_OFFSET }
};

static void dump_slv_abnormal_irq(struct soundwire_priv *priv)
{
	SLV_DP_IntStat1_UNION slv_dp_intstat1;
	SLV_INTSTAT1_UNION slv_intstat1;

	slv_intstat1.value = slv_read_reg(priv, SLV_INTSTAT1_OFFSET);
	if (slv_intstat1.value != 0x0)
		AUDIO_LOGE("slv abnormal irq intstat1 = 0x%x", slv_intstat1.value);

	slv_dp_intstat1.value = slv_read_reg(priv, SLV_DP_IntStat1_OFFSET);
	if (slv_dp_intstat1.reg.spar_err_int ||
		slv_dp_intstat1.reg.sbus_clash_int_0 ||
		slv_dp_intstat1.reg.sbus_clash_int_1)
		AUDIO_LOGE("slv abnormal irq intstat1 = 0x%x", slv_dp_intstat1.value);
}

void enable_slv_all_irqs(struct soundwire_priv *priv)
{
	int dpnum;

	for (dpnum = 1; dpnum <= SOUNDWIRE_MAX_DPNUM; dpnum++)
		slv_write_reg(priv, 0xFF, dp_num_reg(dpnum, 0x1));

	slv_write_reg(priv, 0xFF, SLV_SCP_IntMask_1_OFFSET);
	slv_write_reg(priv, 0xFF, SLV_INTMASK1_OFFSET);
	slv_write_reg(priv, 0xFF, SLV_INTMASK3_OFFSET);
}

void enable_mst_all_irqs(struct soundwire_priv *priv)
{
	int dpnum;

	for (dpnum = 1; dpnum <= SOUNDWIRE_MAX_DPNUM; dpnum++)
		mst_write_reg(priv, 0xFF, dp_num_reg(dpnum, 0x1));

	mst_write_reg(priv, 0xFF, MST_INTMASK1_OFFSET);
	mst_write_reg(priv, 0xFF, MST_INTMASK2_OFFSET);
	mst_write_reg(priv, 0xFF, MST_INTMASK3_OFFSET);
}

static void send_bus_reset(struct soundwire_priv *priv)
{
	MST_CLKEN4_UNION mst_clken4;

	mst_clken4.value = mst_read_reg(priv, MST_CLKEN4_OFFSET);
	mst_clken4.reg.bus_rst_en = 1;
	mst_write_reg(priv, mst_clken4.value, MST_CLKEN4_OFFSET);

	/* min delay 2.67ms */
	usleep_range(2670, 3000);

	mst_clken4.value = mst_read_reg(priv, MST_CLKEN4_OFFSET);
	mst_clken4.reg.bus_rst_en = 0;
	mst_write_reg(priv, mst_clken4.value, MST_CLKEN4_OFFSET);
}

static void send_force_reset(struct soundwire_priv *priv)
{
	SLV_SCP_Stat_UNION slv_scp_stat;

	slv_scp_stat.value = slv_read_reg(priv, SLV_SCP_Stat_OFFSET);
	slv_scp_stat.reg.forcereset = 1;
	slv_write_reg(priv, slv_scp_stat.value, SLV_SCP_Stat_OFFSET);
}

static int bus_reset(struct soundwire_priv *priv, enum reset_type type)
{
	MST_INTMSTAT2_UNION mst_intmstat2;
	unsigned int cnt = WAIT_CNT;
	int ret = 0;

	soundwire_clr_intr(priv, IRQ_SLV_UNATTACHED);
	soundwire_disable_mst_intr(priv, IRQ_SLV_UNATTACHED);
	soundwire_enable_mst_intr(priv, IRQ_ENUM_FINISHED);

	if (type == BUS_RESET) {
		send_bus_reset(priv);
	} else if (type == FORCE_RESET) {
		send_force_reset(priv);
	} else {
		AUDIO_LOGE("reset type error");
		ret = -EFAULT;
		goto exit;
	}

	while (cnt) {
		mst_intmstat2.value = mst_read_reg(priv, MST_INTMSTAT2_OFFSET);
		if (mst_intmstat2.reg.enum_finished_int_mstat == 1) {
			soundwire_clr_intr(priv, IRQ_ENUM_FINISHED);
			break;
		}
		msleep(1);
		cnt--;
	}

	if (cnt == 0) {
		AUDIO_LOGE("enum device failed");
		ret = -ETIMEDOUT;
		goto exit;
	}

	cnt = WAIT_CNT;
	while (cnt) {
		if (soundwire_get_slv_status(priv) != SLV_OFFLINE)
			break;
		msleep(1);
		cnt--;
	}

	if (cnt == 0) {
		AUDIO_LOGE("slv offline");
		ret = -EFAULT;
		goto exit;
	}

	soundwire_bandwidth_recover();
	AUDIO_LOGI("bus reset ok, reset type = %d", type);

exit:
	soundwire_reset_irq(priv);
	return ret;
}

static void slv_unattached_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("slv unattached");
	bus_reset(priv, BUS_RESET);
}

static void slv_com_irq_handler(struct soundwire_priv *priv)
{
	SLV_INTSTAT1_UNION slv_intstat1;

	AUDIO_LOGE("slv com irq coming");

	if (soundwire_get_slv_status(priv) == SLV_OFFLINE) {
		bus_reset(priv, BUS_RESET);
	} else {
		if (soundwire_get_slv_status(priv) == SLV_ONLINE_HAVE_WARNING)
			dump_slv_abnormal_irq(priv);

		slv_intstat1.value = slv_read_reg(priv, SLV_INTSTAT1_OFFSET);
		if (slv_intstat1.reg.ssync_lost_int || slv_intstat1.reg.slv_unattached_int)
			bus_reset(priv, FORCE_RESET);
	}
}

static void command_ignore_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGW("command ignore");
}

static void command_abort_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("command abort");
}

static void msync_lost_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("msync lost");
	if (soundwire_get_slv_status(priv) == SLV_OFFLINE)
		bus_reset(priv, BUS_RESET);
	else
		bus_reset(priv, FORCE_RESET);
}

static void mbus_clash_0_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("mbus clash0");
}

static void mbus_clash_1_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("mbus clash1");
}

static void mpar_err_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("mpar err");
}

static void msync_timeout_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("msync timeout");
}

static void frmend_finished_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGI("scp frmend finished");
}

static void slv_attached_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGI("slv attached");
}

static void clock_option_finished_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGI("clock option finished");
}

static void enum_timeout_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGE("enum timeout");
}

static void enum_finished_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGI("enum finished");
	complete(&priv->enum_comp);
}

static void clockoptionnow_cfg_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGI("clockoptionnow cfg");
}

static void clockoptiondone_cfg_irq_handler(struct soundwire_priv *priv)
{
	AUDIO_LOGI("clockoptiondone cfg");
}

static struct irq_handler g_irq_thread_callback[] = {
	{ IRQ_SLV_UNATTACHED, slv_unattached_irq_handler },
	{ IRQ_SLV_COM, slv_com_irq_handler },
	{ IRQ_COMMAND_IGNORE, command_ignore_irq_handler },
	{ IRQ_COMMAND_ABORT, command_abort_irq_handler },
	{ IRQ_MSYNC_LOST, msync_lost_irq_handler },
	{ IRQ_MBUS_CLASH_0, mbus_clash_0_irq_handler },
	{ IRQ_MBUS_CLASH_1, mbus_clash_1_irq_handler },
	{ IRQ_MPAR_ERR, mpar_err_irq_handler },
	{ IRQ_MSYNC_TIMEOUT, msync_timeout_irq_handler },
	{ IRQ_FRMEND_FINISHED, frmend_finished_irq_handler },
	{ IRQ_SLV_ATTACHED, slv_attached_irq_handler },
	{ IRQ_CLOCK_OPTION_FINISHED, clock_option_finished_irq_handler },
	{ IRQ_ENUM_TIMEOUT, enum_timeout_irq_handler },
	{ IRQ_ENUM_FINISHED, enum_finished_irq_handler },
	{ IRQ_CLOCKOPTIONNOW_CFG, clockoptionnow_cfg_handler },
	{ IRQ_CLOCKOPTIONDONE_CFG, clockoptiondone_cfg_irq_handler }
};

static int handle_irq(struct soundwire_priv *priv, int phy_irq,
	struct irq_handler *handler, int handler_cnt)
{
	int i;

	if (!priv) {
		AUDIO_LOGE("data is null");
		return -EINVAL;
	}

	for (i = 0; i < handler_cnt; i++) {
		if (handler[i].phy_irq == phy_irq) {
			handler[i].irq_callback(priv);
			return 0;
		}
	}

	return -EEXIST;
}

static irqreturn_t irq_handler(int irq, void *data)
{
	struct soundwire_priv *priv = (struct soundwire_priv *)data;

	if (!priv) {
		AUDIO_LOGE("priv is null");
		return IRQ_NONE;
	}

	__pm_stay_awake(priv->wake_lock);
	disable_irq_nosync(irq);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t irq_handler_thread(int irq, void *data)
{
	struct soundwire_priv *priv = (struct soundwire_priv *)data;
	unsigned long status;
	int offset = 0;
	int irq_index;
	int i;

	for (i = 0; i < ARRAY_SIZE(g_irqs); i++) {
		status = mst_read_reg(priv, g_irqs[i].irq_stat);
		if (status != 0) {
			for_each_set_bit(offset, &status, SDW_IRQ_REG_BITS_NUM) {
				irq_index = i * SDW_IRQ_REG_BITS_NUM + offset;
				if (handle_irq(priv, irq_index, g_irq_thread_callback, ARRAY_SIZE(g_irq_thread_callback)) != 0)
					AUDIO_LOGI("can't find callback irq = %d", irq_index);
				/* clr irq whether or not find call back */
				soundwire_clr_intr(priv, irq_index);
			}
		}
	}

	enable_irq(irq);
	__pm_relax(priv->wake_lock);
	return IRQ_HANDLED;
}

void soundwire_enable_mst_intr(struct soundwire_priv *priv, int phy_irq)
{
	int status;

	if (priv == NULL || phy_irq >= IRQ_MAX) {
		AUDIO_LOGE("data invalid phy irq = %d", phy_irq);
		return;
	}

	status = mst_read_reg(priv, g_irqs[irq_index(phy_irq)].irq_mask);
	mst_write_reg(priv, (status | (1 << irq_offset(phy_irq))), g_irqs[irq_index(phy_irq)].irq_mask);
}

void soundwire_disable_mst_intr(struct soundwire_priv *priv, int phy_irq)
{
	int status;

	if (priv == NULL || phy_irq >= IRQ_MAX) {
		AUDIO_LOGE("data invalid phy irq = %d", phy_irq);
		return;
	}

	status = mst_read_reg(priv, g_irqs[irq_index(phy_irq)].irq_mask);
	mst_write_reg(priv, (status & (~(1 << irq_offset(phy_irq)))), g_irqs[irq_index(phy_irq)].irq_mask);
}

void soundwire_enable_mst_intrs(struct soundwire_priv *priv, const int *phy_irqs, int irq_num)
{
	int i;

	if (priv == NULL) {
		AUDIO_LOGE("data is null");
		return ;
	}

	for (i = 0; i < irq_num; i++)
		soundwire_enable_mst_intr(priv, phy_irqs[i]);
}

void soundwire_disable_mst_intrs(struct soundwire_priv *priv, const int *phy_irqs, int irq_num)
{
	int i;

	if (priv == NULL) {
		AUDIO_LOGE("data is null");
		return ;
	}

	for (i = 0; i < irq_num; i++)
		soundwire_disable_mst_intr(priv, phy_irqs[i]);
}

void soundwire_clr_intr(struct soundwire_priv *priv, int phy_irq)
{
	if (priv == NULL || phy_irq >= IRQ_MAX) {
		AUDIO_LOGE("data invalid phy irq = %d", phy_irq);
		return;
	}

	mst_write_reg(priv, 1 << irq_offset(phy_irq), g_irqs[irq_index(phy_irq)].irq_clear);
}

void soundwire_reset_irq(struct soundwire_priv *priv)
{
	/* disable unnecessary irqs */
	soundwire_disable_mst_intr(priv, IRQ_REG_REQ);
	soundwire_disable_mst_intr(priv, IRQ_ENUM_TIMEOUT);
	soundwire_disable_mst_intr(priv, IRQ_ENUM_FINISHED);
	/* enable irqs */
	soundwire_enable_mst_intr(priv, IRQ_MSYNC_LOST);
	soundwire_enable_mst_intr(priv, IRQ_SLV_COM);
	soundwire_enable_mst_intr(priv, IRQ_COMMAND_IGNORE);
	soundwire_enable_mst_intr(priv, IRQ_COMMAND_ABORT);
	soundwire_enable_mst_intr(priv, IRQ_SLV_UNATTACHED);
	soundwire_enable_mst_intr(priv, IRQ_MBUS_CLASH_0);
	soundwire_enable_mst_intr(priv, IRQ_MBUS_CLASH_1);
	soundwire_enable_mst_intr(priv, IRQ_FRMEND_FINISHED);
	soundwire_enable_mst_intr(priv, IRQ_MPAR_ERR);
}

int soundwire_irq_init(struct soundwire_priv *priv)
{
	int ret;

	ret = devm_request_threaded_irq(priv->dev, get_irq_id(), irq_handler, irq_handler_thread,
		IRQF_TRIGGER_HIGH | IRQF_SHARED, "asp_irq_soundwire", priv);
	if (ret != 0) {
		AUDIO_LOGE("request asp soundwire irq failed");
		return ret;
	}

	return 0;
}
