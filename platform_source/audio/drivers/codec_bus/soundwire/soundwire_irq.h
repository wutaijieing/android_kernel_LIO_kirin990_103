/*
 * soundwire_irq.h -- soundwire irq
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
#ifndef __SOUNDWIRE_IRQ_H__
#define __SOUNDWIRE_IRQ_H__

struct soundwire_priv;

enum mst_irq_type {
	IRQ_SLV_UNATTACHED = 1,
	IRQ_SLV_COM = 2,
	IRQ_COMMAND_IGNORE = 3,
	IRQ_COMMAND_ABORT = 4,
	IRQ_MSYNC_LOST = 5,
	IRQ_MBUS_CLASH_0 = 6,
	IRQ_MBUS_CLASH_1 = 7,
	IRQ_MPAR_ERR = 8,
	IRQ_MSYNC_TIMEOUT = 9,
	IRQ_REG_REQ = 10,
	IRQ_FRMEND_FINISHED = 11,
	IRQ_SLV_ATTACHED = 12,
	IRQ_CLOCK_OPTION_FINISHED = 13,
	IRQ_ENUM_TIMEOUT = 14,
	IRQ_ENUM_FINISHED = 15,
	IRQ_CLOCKOPTIONNOW_CFG = 16,
	IRQ_CLOCKOPTIONDONE_CFG = 17,
	IRQ_MAX
};

void soundwire_enable_mst_intr(struct soundwire_priv *priv, int phy_irq);
void soundwire_disable_mst_intr(struct soundwire_priv *priv, int phy_irq);
void soundwire_enable_mst_intrs(struct soundwire_priv *priv, const int *phy_irqs, int irq_num);
void soundwire_disable_mst_intrs(struct soundwire_priv *priv, const int *phy_irqs, int irq_num);
void soundwire_clr_intr(struct soundwire_priv *priv, int phy_irq);
void soundwire_reset_irq(struct soundwire_priv *priv);
int soundwire_irq_init(struct soundwire_priv *priv);
#endif
